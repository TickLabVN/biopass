#include "antispoof_check.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <future>
#include <thread>
#include <vector>

#include "camera_capture.h"
#include "debug_image_io.h"
#include "face_as.h"
#include "face_detection.h"
#include "image_utils.h"
#include "ir_camera_as.h"

namespace biopass {

namespace {

struct AntiSpoofTask {
  std::string name;
  std::future<bool> future;
};

AntiSpoofTask make_task(const std::string& name, std::future<bool> future) {
  AntiSpoofTask task;
  task.name = name;
  task.future = std::move(future);
  return task;
}

bool checkAntiSpoofByAIModel(const FaceMethodConfig& faceCfg, const std::string& username,
                             const ImageRGB& face, const AuthConfig& authCfg,
                             FaceAntiSpoofing* rgb_as_model) {
  const std::string modelPath = faceCfg.anti_spoofing.model.path;

  try {
    std::unique_ptr<FaceAntiSpoofing> temp_as;
    FaceAntiSpoofing* face_as = rgb_as_model;
    if (!face_as) {
      if (modelPath.empty() || !std::ifstream(modelPath).good()) {
        spdlog::error("FaceAuth: Anti-spoofing model file not found: {}", modelPath);
        return false;
      }
      temp_as =
          std::make_unique<FaceAntiSpoofing>(modelPath, 128, faceCfg.anti_spoofing.model.threshold);
      face_as = temp_as.get();
    }

    const SpoofResult result = face_as->inference(face);
    if (!result.is_real) {
      spdlog::warn(
          "FaceAuth: AI anti-spoofing failed — real={:.4f}, spoof={:.4f}, threshold={:.3f}",
          result.real_score, result.spoof_score, faceCfg.anti_spoofing.model.threshold);
      if (authCfg.debug) {
        saveFailedFace(username, face, "spoof");
      }
      return false;
    }

    spdlog::debug(
        "FaceAuth: AI anti-spoofing check passed — real={:.4f}, spoof={:.4f}, threshold={:.3f}",
        result.real_score, result.spoof_score, faceCfg.anti_spoofing.model.threshold);
    return true;
  } catch (const std::exception& e) {
    spdlog::error("FaceAuth: AI anti-spoofing check failed: {}", e.what());
    return false;
  }
}

}  // namespace

bool checkAntiSpoof(const FaceMethodConfig& face_config, const std::string& username,
                    const Detection& rgb_det, int rgb_width, int rgb_height,
                    const AuthConfig& config, ICameraCaptureSession* ir_camera_session,
                    FaceAntiSpoofing* rgb_as_model, FaceDetection* ir_det_model,
                    FaceAntiSpoofing* ir_as_model) {
  const bool ai_enabled = face_config.anti_spoofing.enable;
  const bool ir_enabled = face_config.anti_spoofing.ir_camera.has_value() &&
                          !face_config.anti_spoofing.ir_camera->empty();

  if (!ai_enabled && !ir_enabled) {
    spdlog::debug("FaceAuth: Anti-spoofing methods are disabled, skipping checks");
    return true;
  }

  spdlog::debug("FaceAuth: Anti-spoofing started (ai_enabled={}, ir_enabled={}, ir_camera='{}')",
                ai_enabled, ir_enabled, face_config.anti_spoofing.ir_camera.value_or(""));

  std::vector<AntiSpoofTask> tasks;

  if (ai_enabled) {
    const auto face_config_copy = face_config;
    const auto username_copy = username;
    const auto face_copy = rgb_det.image;
    const auto config_copy = config;
    tasks.push_back(make_task("AI", std::async(std::launch::async, [=]() {
                                return checkAntiSpoofByAIModel(face_config_copy, username_copy,
                                                               face_copy, config_copy,
                                                               rgb_as_model);
                              })));
  }

  if (ir_enabled) {
    const auto detection_model = face_config.detection.model;
    const auto antispoof_model = face_config.anti_spoofing.model.path;
    const auto antispoof_threshold = face_config.anti_spoofing.model.threshold;
    const auto username_copy = username;
    const auto debug_enabled = config.debug;
    const auto warmup_delay_ms = face_config.anti_spoofing.ir_warmup_delay_ms;
    const auto min_face_area_ratio = face_config.anti_spoofing.ir_min_face_area_ratio;
    const auto ir_model_hard_fail = face_config.anti_spoofing.ir_model_hard_fail;

    bool valid_models = true;
    if (!ir_det_model && (detection_model.empty() || !std::ifstream(detection_model).good())) {
      spdlog::error("FaceAuth: IR liveness check — detection model not found: {}", detection_model);
      valid_models = false;
    }
    if (!ir_as_model && (antispoof_model.empty() || !std::ifstream(antispoof_model).good())) {
      spdlog::error("FaceAuth: IR liveness check — anti-spoofing model not found: {}",
                    antispoof_model);
      valid_models = false;
    }

    if (!valid_models) {
      tasks.push_back(make_task("IR", std::async(std::launch::deferred, []() { return false; })));
    } else {
      (void)rgb_width;
      (void)rgb_height;

      const auto ir_camera_path = *face_config.anti_spoofing.ir_camera;

      tasks.push_back(make_task(
          "IR", std::async(std::launch::async, [=]() {
            std::unique_ptr<FaceDetection> temp_det;
            std::unique_ptr<FaceAntiSpoofing> temp_as;

            FaceDetection* det = ir_det_model;
            FaceAntiSpoofing* as = ir_as_model;

            if (!det) {
              temp_det = std::make_unique<FaceDetection>(detection_model, 640,
                                                         std::vector<std::string>{"face"}, 0.05f);
              det = temp_det.get();
            }
            if (!as) {
              temp_as =
                  std::make_unique<FaceAntiSpoofing>(antispoof_model, 128, antispoof_threshold);
              as = temp_as.get();
            }

            return checkAntispoofByIRCamera(ir_camera_session, ir_camera_path, det, as,
                                            warmup_delay_ms, min_face_area_ratio, username_copy,
                                            debug_enabled, ai_enabled, ir_model_hard_fail);
          })));
    }
  }

  // IMPORTANT — Thread safety contract:
  // The async tasks above capture raw pointers to model objects and the IR
  // camera session owned by the caller (FaceAuth). ALL futures MUST be joined
  // (via .get()) before this function returns. If any future is abandoned, the
  // caller may destroy the pointed-to objects while the async thread still uses
  // them, causing a use-after-free crash. Do NOT add early-return paths above
  // this loop without ensuring every launched task has completed.
  bool all_passed = true;
  for (auto& task : tasks) {
    bool ok = false;
    try {
      ok = task.future.get();
    } catch (const std::exception& e) {
      spdlog::error("FaceAuth: {} anti-spoofing task failed: {}", task.name, e.what());
      ok = false;
    }
    if (ok) {
      spdlog::debug("FaceAuth: {} anti-spoofing method passed", task.name);
    } else {
      spdlog::debug("FaceAuth: {} anti-spoofing method failed", task.name);
      all_passed = false;
    }
  }

  if (!all_passed) {
    spdlog::error("FaceAuth: Anti-spoofing failed (one or more enabled methods failed)");
  }
  return all_passed;
}

}  // namespace biopass
