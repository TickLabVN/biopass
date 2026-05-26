#include "ir_camera_as.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <thread>

#include "camera_capture.h"
#include "debug_image_io.h"
#include "face_detection.h"

namespace biopass {

bool checkAntispoofByIRCamera(const std::string& device_path,
                              const std::string& detection_model_path, float detection_threshold,
                              const std::string& username, bool debug,
                              ICameraCaptureSession* session,
                              const IRCaptureParams& ir_params) {
  if (device_path.empty()) {
    return false;
  }

  if (!std::ifstream(detection_model_path).good()) {
    spdlog::error("FaceAuth: Detection model file not found: {}", detection_model_path);
    return false;
  }

  // If no pre-opened session, create a fresh one
  std::unique_ptr<ICameraCaptureSession> local_session;
  if (!session || !session->isOpen()) {
    local_session = openCameraSession(device_path, CameraCaptureFormat::V4L2NV12,
                                      ir_params.warmup_frames, ir_params.capture_timeout_ms,
                                      ir_params.poll_interval_ms);
    if (!local_session) {
      spdlog::warn("FaceAuth: NV12 IR capture unavailable for '{}'; falling back to GREY",
                   device_path);
      local_session = openCameraSession(device_path, CameraCaptureFormat::V4L2Grey,
                                        ir_params.warmup_frames, ir_params.capture_timeout_ms,
                                        ir_params.poll_interval_ms);
    }
    if (!local_session) {
      spdlog::error("FaceAuth: IR camera failed to open {}", device_path);
      return false;
    }
    session = local_session.get();
  }

  // Camera warmup delay before first capture (AGC settling, auto-exposure, etc.)
  if (ir_params.camera_warmup_ms > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ir_params.camera_warmup_ms));
  }

  int best_face_count = 0;

  try {
    FaceDetection detector(detection_model_path, 640, {"face"}, detection_threshold);

    for (int attempt = 0; attempt < ir_params.max_attempts; ++attempt) {
      // Between attempts, let the IR camera AGC settle
      if (attempt > 0 && ir_params.agc_sleep_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ir_params.agc_sleep_ms));
      }

      ImageRGB frame = session->capture();
      if (frame.empty()) {
        spdlog::warn("FaceAuth: IR capture attempt {}/{} returned empty frame", attempt + 1,
                     ir_params.max_attempts);
        continue;
      }

      auto faces = detector.inference(frame);
      int face_count = static_cast<int>(faces.size());
      if (face_count > best_face_count) {
        best_face_count = face_count;
      }

      if (face_count > 0) {
        spdlog::debug("FaceAuth: IR anti-spoofing detected {} face(s) on attempt {}/{}", face_count,
                      attempt + 1, ir_params.max_attempts);
        return true;
      }

      spdlog::debug("FaceAuth: IR anti-spoofing no face on attempt {}/{}", attempt + 1,
                    ir_params.max_attempts);
    }

    // All attempts failed
    spdlog::error("FaceAuth: IR camera check failed, no face detected in {} capture(s) (best: {} face(s))",
                  ir_params.max_attempts, best_face_count);
    if (debug) {
      // Capture one more frame to save as debug (best-effort)
      ImageRGB debug_frame = session->capture();
      if (!debug_frame.empty()) {
        saveFailedFace(username, debug_frame, "ir_no_face");
      }
    }
    return false;

  } catch (const std::exception& e) {
    spdlog::error("FaceAuth: IR anti-spoofing check failed: {}", e.what());
    return false;
  }
}
}  // namespace biopass
