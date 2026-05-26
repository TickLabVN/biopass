#include "face_auth.h"

#include <spdlog/spdlog.h>

#include <fstream>
#include <memory>
#include <optional>
#include <vector>

#include "antispoof_check.h"
#include "camera_capture.h"
#include "debug_image_io.h"
#include "face_detection.h"
#include "face_recognition.h"
#include "image_utils.h"

namespace biopass {

const IRCaptureParams& FaceAuth::irParams() const {
  return face_config_.advanced.ir_capture;
}

bool FaceAuth::isAvailable() const {
  return checkCameraAvailability(face_config_.camera_device);
}

uint32_t FaceAuth::getMaxAuthTimeMs() const {
  if (face_config_.advanced.auth.max_time_ms > 0) {
    return static_cast<uint32_t>(face_config_.advanced.auth.max_time_ms);
  }
  // Default: retries * retry_delay with some margin for capture time
  return face_config_.retries * face_config_.retry_delay + 5000;
}

void FaceAuth::beginAuthenticationSession() {
  if (!camera_session_) {
    camera_session_ = openCameraSession(face_config_.camera_device);
  }

  if (face_config_.anti_spoofing.ir_camera.has_value() &&
      !face_config_.anti_spoofing.ir_camera->empty() && !ir_camera_session_) {
    // If camera_device and ir_camera point to the same device, reuse the primary session
    if (face_config_.camera_device.has_value() &&
        *face_config_.camera_device == *face_config_.anti_spoofing.ir_camera) {
      spdlog::debug("FaceAuth: IR camera same as primary device, reusing session");
      ir_camera_session_ = nullptr;
    } else {
      const auto& ir = irParams();
      ir_camera_session_ =
          openCameraSession(*face_config_.anti_spoofing.ir_camera, CameraCaptureFormat::V4L2NV12,
                            ir.warmup_frames, ir.capture_timeout_ms, ir.poll_interval_ms);
      if (!ir_camera_session_) {
        spdlog::warn("FaceAuth: NV12 IR capture unavailable; falling back to GREY");
        ir_camera_session_ =
            openCameraSession(*face_config_.anti_spoofing.ir_camera, CameraCaptureFormat::V4L2Grey,
                              ir.warmup_frames, ir.capture_timeout_ms, ir.poll_interval_ms);
      }
    }
  }
}

void FaceAuth::endAuthenticationSession() {
  ir_camera_session_.reset();
  camera_session_.reset();
}

AuthResult FaceAuth::authenticate(const std::string& username, const AuthConfig& config,
                                  std::atomic<bool>* cancel_signal) {
  if (!camera_session_) {
    camera_session_ = openCameraSession(face_config_.camera_device);
  }
  if (!camera_session_ || !camera_session_->isOpen()) {
    spdlog::error("FaceAuth: Could not open camera");
    if (!checkCameraAvailability(face_config_.camera_device)) {
      return AuthResult::Unavailable;
    }
    return AuthResult::Retry;
  }

  std::vector<std::string> enrolledFaces = biopass::listFaces(username);
  if (enrolledFaces.empty()) {
    spdlog::error("FaceAuth: No face enrolled for user {}, skipping", username);
    return AuthResult::Unavailable;
  }

  std::string recogModelPath = face_config_.recognition.model;
  std::string detectModelPath = face_config_.detection.model;
  if (!std::ifstream(recogModelPath).good() || !std::ifstream(detectModelPath).good()) {
    spdlog::error("FaceAuth: Model files not found for user {}, skipping", username);
    return AuthResult::Unavailable;
  }

  std::unique_ptr<FaceRecognition> faceReg;
  std::unique_ptr<FaceDetection> detector;
  try {
    detector = std::make_unique<FaceDetection>(detectModelPath);
  } catch (const std::exception& e) {
    std::string msg = e.what();
    size_t first_line = msg.find('\n');
    if (first_line != std::string::npos)
      msg = msg.substr(0, first_line);
    spdlog::error("FaceAuth: Failed to load detection model: {}, skipping", msg);
    return AuthResult::Unavailable;
  }

  try {
    faceReg = std::make_unique<FaceRecognition>(recogModelPath);
  } catch (const std::exception& e) {
    std::string msg = e.what();
    size_t first_line = msg.find('\n');
    if (first_line != std::string::npos)
      msg = msg.substr(0, first_line);
    spdlog::error("FaceAuth: Failed to load recognition model: {}, skipping", msg);
    return AuthResult::Unavailable;
  }

  if (cancel_signal && cancel_signal->load()) {
    return AuthResult::Failure;
  }

  ImageRGB loginFace = camera_session_->capture();
  if (loginFace.empty()) {
    spdlog::error("FaceAuth: Could not read frame");
    camera_session_.reset();
    return AuthResult::Retry;
  }

  std::vector<Detection> detectedImages = detector->inference(loginFace);
  if (detectedImages.empty()) {
    spdlog::error("FaceAuth: No face detected");
    return AuthResult::Retry;
  }

  ImageRGB face = detectedImages[0].image;

  // Check if IR anti-spoofing is needed
  if (face_config_.anti_spoofing.ir_camera.has_value() &&
      !face_config_.anti_spoofing.ir_camera->empty() &&
      (!ir_camera_session_ || !ir_camera_session_->isOpen())) {
    // Reuse primary session if same device, otherwise open new session
    if (face_config_.camera_device.has_value() &&
        *face_config_.camera_device == *face_config_.anti_spoofing.ir_camera) {
      spdlog::debug("FaceAuth: Reusing primary camera session for IR anti-spoofing");
      ir_camera_session_ = nullptr;
    } else {
      const auto& ir = irParams();
      ir_camera_session_ =
          openCameraSession(*face_config_.anti_spoofing.ir_camera, CameraCaptureFormat::V4L2NV12,
                            ir.warmup_frames, ir.capture_timeout_ms, ir.poll_interval_ms);
      if (!ir_camera_session_) {
        ir_camera_session_ =
            openCameraSession(*face_config_.anti_spoofing.ir_camera, CameraCaptureFormat::V4L2Grey,
                              ir.warmup_frames, ir.capture_timeout_ms, ir.poll_interval_ms);
      }
    }
  }

  auto* ir_session = ir_camera_session_.get();
  FaceMethodConfig as_config = face_config_;
  if (!ir_session && face_config_.camera_device.has_value() &&
      *face_config_.camera_device == *face_config_.anti_spoofing.ir_camera) {
    spdlog::debug("FaceAuth: Same device for camera and IR, reusing session and disabling AI anti-spoofing");
    ir_session = camera_session_.get();
    as_config.anti_spoofing.enable = false;
  }
  if (!checkAntiSpoof(as_config, username, face, config, ir_session)) {
    spdlog::warn("FaceAuth: Anti-spoofing failed");
    if (ir_camera_session_ && !ir_camera_session_->isOpen()) {
      ir_camera_session_.reset();
    }
    return AuthResult::Retry;
  }

  // Match against all enrolled faces — succeed if any match.
  for (const auto& facePath : enrolledFaces) {
    ImageRGB preparedFace = readImage(facePath);
    if (preparedFace.empty())
      continue;

    MatchResult match = faceReg->match(preparedFace, face);
    if (match.similar) {
      return AuthResult::Success;
    }
  }

  if (config.debug) {
    saveFailedFace(username, face, "not_similar");
  }

  return AuthResult::Retry;
}

}  // namespace biopass
