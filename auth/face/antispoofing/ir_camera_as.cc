#include "ir_camera_as.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <thread>

#include "auth_config.h"
#include "camera_capture.h"
#include "debug_image_io.h"
#include "face_as.h"
#include "face_detection.h"

namespace biopass {

namespace {

struct IRFrameStats {
  int min_val = 255;
  int max_val = 0;
  double mean_val = 0.0;
};

IRFrameStats calculate_ir_frame_stats(const ImageRGB& frame) {
  IRFrameStats stats;
  const int total_pixels = frame.width * frame.height;
  if (frame.empty() || total_pixels <= 0) {
    stats.min_val = 0;
    return stats;
  }

  double sum_val = 0.0;
  const uint8_t* ptr = frame.ptr();
  for (int i = 0; i < total_pixels; ++i) {
    uint8_t val = ptr[i * 3];
    if (val < stats.min_val)
      stats.min_val = val;
    if (val > stats.max_val)
      stats.max_val = val;
    sum_val += val;
  }
  stats.mean_val = sum_val / total_pixels;
  return stats;
}

}  // namespace

IRLivenessResult checkAntispoofByIRCamera(ICameraCaptureSession* ir_camera_session,
                                          const std::string& ir_camera_path,
                                          FaceDetection* ir_det_model,
                                          FaceAntiSpoofing* ir_as_model, int warmup_delay_ms,
                                          float min_face_area_ratio, const std::string& username,
                                          bool debug_enabled) {
  if (!ir_det_model) {
    spdlog::error("FaceAuth: IR liveness check — FaceDetection model is null");
    return {};
  }
  if (!ir_as_model) {
    spdlog::error("FaceAuth: IR liveness check — FaceAntiSpoofing model is null");
    return {};
  }

  // Helper lambda: capture one IR frame from an open session or new session.
  auto capture_ir = [&]() -> ImageRGB {
    if (ir_camera_session && ir_camera_session->isOpen()) {
      return ir_camera_session->capture();
    }
    return captureImageByIRCamera(ir_camera_path, /*warmup_frames=*/5,
                                  /*timeout_ms=*/3000,
                                  /*poll_interval_ms=*/10);
  };

  // LED warm-up retry: use cheap brightness stats. Face detection is kept
  // for usable-frame selection only, so AI and IR work can overlap sooner.
  constexpr int kMaxLedWarmupRetries = 10;
  constexpr int kMinLedWarmupRetryDelayMs = 50;
  constexpr int kMaxLedWarmupRetryDelayMs = 300;
  const int led_warmup_retry_delay_ms =
      std::clamp(warmup_delay_ms, kMinLedWarmupRetryDelayMs, kMaxLedWarmupRetryDelayMs);
  constexpr double kUsableIrMean = 35.0;
  constexpr int kUsableIrMax = 120;
  constexpr float kIrFacePresentThreshold = 0.45f;
  constexpr int kRequiredUsableIrFrames = 2;
  constexpr int kMaxIrFrameSelectionAttempts = 6;
  constexpr int kIrFrameSelectionIntervalMs = 80;

  auto process_and_log_ir_frame = [&](const ImageRGB& frame, int attempt) -> IRFrameStats {
    if (frame.empty()) {
      spdlog::debug("FaceAuth: IR frame stats (attempt {}) — frame is empty", attempt);
      return {};
    }
    const IRFrameStats stats = calculate_ir_frame_stats(frame);

    if (debug_enabled) {
      const std::string path =
          getDebugPath(username) + "/ir_raw_attempt_" + std::to_string(attempt) + ".png";
      saveImage(path, frame);
    }

    spdlog::debug(
        "FaceAuth: IR frame stats (attempt {}) — shape=({}x{}x3), "
        "dtype=uint8, min={}, max={}, mean={:.2f}",
        attempt, frame.width, frame.height, stats.min_val, stats.max_val, stats.mean_val);
    return stats;
  };

  spdlog::debug(
      "FaceAuth: IR liveness check — capturing frame (LED warm-up check "
      "enabled)");
  ImageRGB ir_frame = capture_ir();
  process_and_log_ir_frame(ir_frame, 0);

  try {
    IRFrameStats ir_stats = calculate_ir_frame_stats(ir_frame);
    for (int attempt = 0; attempt < kMaxLedWarmupRetries && !ir_frame.empty(); ++attempt) {
      if (ir_stats.mean_val >= kUsableIrMean && ir_stats.max_val >= kUsableIrMax) {
        if (attempt > 0) {
          spdlog::debug(
              "FaceAuth: IR liveness check — LED ready after {} "
              "recapture(s) (mean={:.2f}, max={})",
              attempt, ir_stats.mean_val, ir_stats.max_val);
        }
        break;
      }

      spdlog::debug(
          "FaceAuth: IR liveness check — IR frame is still dark "
          "(mean={:.2f} < {:.2f} or max={} < {}), LED may not be ready — "
          "recapturing in {}ms (attempt {}/{})",
          ir_stats.mean_val, kUsableIrMean, ir_stats.max_val, kUsableIrMax,
          led_warmup_retry_delay_ms, attempt + 1, kMaxLedWarmupRetries);
      std::this_thread::sleep_for(std::chrono::milliseconds(led_warmup_retry_delay_ms));
      ir_frame = capture_ir();
      ir_stats = process_and_log_ir_frame(ir_frame, attempt + 1);
    }

    std::vector<ImageRGB> ir_faces;
    for (int attempt = 0;
         attempt < kMaxIrFrameSelectionAttempts && (int)ir_faces.size() < kRequiredUsableIrFrames;
         ++attempt) {
      ImageRGB candidate = attempt == 0 ? ir_frame : capture_ir();
      const IRFrameStats stats = process_and_log_ir_frame(candidate, 100 + attempt);
      if (candidate.empty()) {
        continue;
      }

      const bool low_ir_quality = stats.mean_val < kUsableIrMean || stats.max_val < kUsableIrMax;
      if (low_ir_quality) {
        spdlog::debug(
            "FaceAuth: IR liveness check — IR frame has low illumination; still "
            "checking for face (mean={:.2f} < {:.2f} or max={} < {})",
            stats.mean_val, kUsableIrMean, stats.max_val, kUsableIrMax);
      }

      {
        const std::vector<Detection> raw = ir_det_model->inference(candidate);
        float max_conf = 0.0f;
        int best_detection_index = -1;
        for (size_t i = 0; i < raw.size(); ++i) {
          const auto& d = raw[i];
          if (d.conf > max_conf) {
            max_conf = d.conf;
            best_detection_index = static_cast<int>(i);
          }
        }

        if (best_detection_index >= 0 && max_conf >= kIrFacePresentThreshold) {
          const Detection& best_detection = raw[best_detection_index];
          const int bbox_w = best_detection.box.x2 - best_detection.box.x1;
          const int bbox_h = best_detection.box.y2 - best_detection.box.y1;
          const float frame_area =
              static_cast<float>(candidate.width) * static_cast<float>(candidate.height);
          const float bbox_area_ratio =
              frame_area > 0.0f
                  ? (static_cast<float>(bbox_w) * static_cast<float>(bbox_h)) / frame_area
                  : 0.0f;

          if (bbox_area_ratio < min_face_area_ratio) {
            spdlog::debug(
                "FaceAuth: IR liveness check — skipping IR face too small for reliable "
                "liveness (face_conf={:.4f}, bbox={}x{}, area_ratio={:.4f} < {:.4f})",
                max_conf, bbox_w, bbox_h, bbox_area_ratio, min_face_area_ratio);
          } else {
            ir_faces.push_back(best_detection.image);
            if (debug_enabled) {
              saveFailedFace(username, best_detection.image, "ir_selected_crop");
              saveFailedFace(username, resizeImage(best_detection.image, 128, 128),
                             "ir_model_input_128");
            }

            spdlog::debug(
                "FaceAuth: IR liveness check — accepted usable IR face {}/{} "
                "(face_conf={:.4f}, bbox={}x{}, area_ratio={:.4f}, mean={:.2f}, max={})",
                ir_faces.size(), kRequiredUsableIrFrames, max_conf, bbox_w, bbox_h, bbox_area_ratio,
                stats.mean_val, stats.max_val);
          }
        } else {
          spdlog::debug(
              "FaceAuth: IR liveness check — skipping IR frame with weak face "
              "confidence ({:.4f} < {:.4f})",
              max_conf, kIrFacePresentThreshold);
        }
      }

      if ((int)ir_faces.size() < kRequiredUsableIrFrames) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kIrFrameSelectionIntervalMs));
      }
    }

    if (ir_faces.size() < 2) {
      spdlog::error(
          "FaceAuth: IR liveness check — only {} usable IR face(s) captured "
          "from '{}' (required >= 2)",
          ir_faces.size(), ir_camera_path);
      return {};
    }

    return {true, checkAntispoofByIRCrops(ir_faces, ir_as_model, username, debug_enabled)};
  } catch (const std::exception& e) {
    spdlog::warn("FaceAuth: IR liveness check — frame selection error: {}", e.what());
    return {};
  }
}

bool checkAntispoofByIRCrops(const std::vector<ImageRGB>& ir_faces, FaceAntiSpoofing* face_as,
                             const std::string& username, bool debug) {
  if (!face_as) {
    spdlog::error("FaceAuth: IR liveness check — anti-spoofing model is null");
    return false;
  }

  const float antispoof_threshold = face_as->getThreshold();

  spdlog::debug("FaceAuth: IR liveness check | preselected_faces={} antispoof_threshold={:.3f}",
                ir_faces.size(), antispoof_threshold);

  if (ir_faces.empty()) {
    spdlog::error("FaceAuth: IR liveness check — received no preselected face crops, aborting");
    return false;
  }

  int passed_faces = 0;
  int valid_faces_processed = 0;
  float real_score_sum = 0.0f;
  float spoof_score_sum = 0.0f;

  try {
    for (size_t face_idx = 0; face_idx < ir_faces.size(); ++face_idx) {
      const auto& face = ir_faces[face_idx];
      if (face.empty()) {
        continue;
      }
      valid_faces_processed++;

      const SpoofResult result = face_as->inference(face);
      const bool is_face_real = result.is_real;
      real_score_sum += result.real_score;
      spoof_score_sum += result.spoof_score;
      if (!is_face_real) {
        spdlog::warn(
            "FaceAuth: IR liveness check [face {}/{}] — FAILED (crop={}x{}, real={:.4f}, "
            "spoof={:.4f}, threshold={:.3f})",
            face_idx + 1, ir_faces.size(), face.width, face.height, result.real_score,
            result.spoof_score, antispoof_threshold);
        if (debug) {
          saveFailedFace(username, face, "ir_spoof_face_" + std::to_string(face_idx));
        }
      } else {
        spdlog::debug(
            "FaceAuth: IR liveness check [face {}/{}] — PASSED (crop={}x{}, real={:.4f}, "
            "spoof={:.4f}, threshold={:.3f})",
            face_idx + 1, ir_faces.size(), face.width, face.height, result.real_score,
            result.spoof_score, antispoof_threshold);
        passed_faces++;
      }
    }

    if (valid_faces_processed == 0) {
      spdlog::error("FaceAuth: IR liveness check — all preselected face crops were empty/invalid");
      return false;
    }

    const float avg_real_score = real_score_sum / static_cast<float>(valid_faces_processed);
    const float avg_spoof_score = spoof_score_sum / static_cast<float>(valid_faces_processed);
    const bool has_strict_real_face = passed_faces >= 1;
    const bool aggregate_real =
        avg_real_score >= antispoof_threshold && avg_real_score > avg_spoof_score;
    const bool passed = has_strict_real_face && aggregate_real;

    if (passed) {
      spdlog::debug(
          "FaceAuth: IR liveness check AGGREGATE PASSED — {}/{} strict face(s) passed, "
          "avg_real={:.4f}, avg_spoof={:.4f}, threshold={:.3f}",
          passed_faces, valid_faces_processed, avg_real_score, avg_spoof_score,
          antispoof_threshold);
    } else {
      spdlog::warn(
          "FaceAuth: IR liveness check AGGREGATE FAILED — {}/{} strict face(s) passed, "
          "avg_real={:.4f}, avg_spoof={:.4f}, threshold={:.3f}",
          passed_faces, valid_faces_processed, avg_real_score, avg_spoof_score,
          antispoof_threshold);
    }
    return passed;
  } catch (const std::exception& e) {
    spdlog::error("FaceAuth: IR liveness check — exception during preselected inference: {}",
                  e.what());
    return false;
  }
}

}  // namespace biopass
