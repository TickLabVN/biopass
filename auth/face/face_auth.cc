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

namespace {

constexpr int kIrCaptureWarmupFrames = 5;
constexpr int kIrCaptureTimeoutMs = 3000;
constexpr int kIrCapturePollIntervalMs = 10;

}  // namespace

bool FaceAuth::isAvailable() const { return checkCameraAvailability(std::nullopt); }

void FaceAuth::beginAuthenticationSession() {
  if (!camera_session_) {
    camera_session_ = openCameraSession(std::nullopt);
  }

  if (face_config_.anti_spoofing.ir_camera.has_value() &&
      !face_config_.anti_spoofing.ir_camera->empty() && !ir_camera_session_) {
    ir_camera_session_ =
        openCameraSession(*face_config_.anti_spoofing.ir_camera, CameraCaptureFormat::V4L2Grey,
                          kIrCaptureWarmupFrames, kIrCaptureTimeoutMs, kIrCapturePollIntervalMs);
  }
}

void FaceAuth::endAuthenticationSession() {
  ir_camera_session_.reset();
  camera_session_.reset();
}

AuthResult FaceAuth::authenticate(const std::string& username, const AuthConfig& config,
                                  std::atomic<bool>* cancel_signal) {
  if (!camera_session_) {
    camera_session_ = openCameraSession(std::nullopt);
  }
  if (!camera_session_ || !camera_session_->isOpen()) {
    spdlog::error("FaceAuth: Could not open camera");
    if (!checkCameraAvailability(std::nullopt)) {
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

  if (face_config_.anti_spoofing.ir_camera.has_value() &&
      !face_config_.anti_spoofing.ir_camera->empty() &&
      (!ir_camera_session_ || !ir_camera_session_->isOpen())) {
    ir_camera_session_ =
        openCameraSession(*face_config_.anti_spoofing.ir_camera, CameraCaptureFormat::V4L2Grey,
                          kIrCaptureWarmupFrames, kIrCaptureTimeoutMs, kIrCapturePollIntervalMs);
  }

  if (!checkAntiSpoof(face_config_, username, face, config, ir_camera_session_.get())) {
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
