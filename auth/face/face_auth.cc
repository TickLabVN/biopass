#include "face_auth.h"

#include <spdlog/spdlog.h>

#include <fstream>
#include <memory>
#include <optional>
#include <vector>

#include "antispoof_check.h"
#include "camera_capture.h"
#include "debug_image_io.h"
#include "face_as.h"
#include "face_detection.h"
#include "face_recognition.h"
#include "image_utils.h"

namespace biopass {

namespace {

constexpr int kIrCaptureWarmupFrames = 5;
constexpr int kIrCaptureTimeoutMs = 3000;
constexpr int kIrCapturePollIntervalMs = 10;

}  // namespace

FaceAuth::FaceAuth(const FaceMethodConfig& config) : face_config_(config) {}

bool FaceAuth::isAvailable() const { return checkCameraAvailability(face_config_.camera); }

void FaceAuth::beginAuthenticationSession() {
  if (!camera_session_) {
    camera_session_ = openCameraSession(face_config_.camera);
  }

  const bool ir_enabled = face_config_.anti_spoofing.ir_camera.has_value() &&
                          !face_config_.anti_spoofing.ir_camera->empty();

  if (ir_enabled && !ir_camera_session_) {
    ir_camera_session_ =
        openCameraSession(*face_config_.anti_spoofing.ir_camera, CameraCaptureFormat::V4L2Grey,
                          kIrCaptureWarmupFrames, kIrCaptureTimeoutMs, kIrCapturePollIntervalMs);
  }

  // Load models for caching. Guards ensure idempotency — if beginAuthenticationSession
  // is called more than once, models are not reloaded.
  std::string recogModelPath = face_config_.recognition.model;
  std::string detectModelPath = face_config_.detection.model;

  // Cache RGB face detector (always needed for face authentication)
  if (!rgb_detector_ && std::ifstream(detectModelPath).good()) {
    try {
      rgb_detector_ = std::make_unique<FaceDetection>(
          detectModelPath, 640, std::vector<std::string>{"face"}, face_config_.detection.threshold);
      spdlog::debug("FaceAuth: RGB detection model cached");
    } catch (const std::exception& e) {
      spdlog::error("FaceAuth: Failed to cache RGB detection model: {}", e.what());
    }
  }

  // Cache IR face detector only when IR camera is configured. Uses a lower
  // confidence threshold than RGB because the IR detector only needs to prove
  // face presence, not identity.
  if (ir_enabled && !ir_detector_ && std::ifstream(detectModelPath).good()) {
    try {
      ir_detector_ = std::make_unique<FaceDetection>(detectModelPath, 640,
                                                     std::vector<std::string>{"face"}, 0.05f);
      spdlog::debug("FaceAuth: IR detection model cached");
    } catch (const std::exception& e) {
      spdlog::error("FaceAuth: Failed to cache IR detection model: {}", e.what());
    }
  }

  // Cache recognition model
  if (!face_recognition_ && std::ifstream(recogModelPath).good()) {
    try {
      face_recognition_ = std::make_unique<FaceRecognition>(recogModelPath, 112,
                                                            face_config_.recognition.threshold);
      spdlog::debug("FaceAuth: Recognition model cached");
    } catch (const std::exception& e) {
      spdlog::error("FaceAuth: Failed to cache recognition model: {}", e.what());
    }
  }

  // Cache anti-spoofing model(s). When both AI and IR pipelines are enabled,
  // two separate FaceAntiSpoofing instances are created from the same .onnx
  // file. This is intentional: Ort::Session::Run() is NOT thread-safe, so
  // each async thread needs its own instance to avoid data races.
  const std::string asModelPath = face_config_.anti_spoofing.model.path;
  const float asThreshold = face_config_.anti_spoofing.model.threshold;
  const bool as_model_exists = !asModelPath.empty() && std::ifstream(asModelPath).good();

  if (face_config_.anti_spoofing.enable && !rgb_antispoof_ && as_model_exists) {
    try {
      rgb_antispoof_ = std::make_unique<FaceAntiSpoofing>(asModelPath, 128, asThreshold);
      spdlog::debug("FaceAuth: RGB anti-spoofing model cached");
    } catch (const std::exception& e) {
      spdlog::error("FaceAuth: Failed to cache RGB anti-spoofing model: {}", e.what());
    }
  }

  if (ir_enabled && !ir_antispoof_ && as_model_exists) {
    try {
      ir_antispoof_ = std::make_unique<FaceAntiSpoofing>(asModelPath, 128, asThreshold);
      spdlog::debug("FaceAuth: IR anti-spoofing model cached");
    } catch (const std::exception& e) {
      spdlog::error("FaceAuth: Failed to cache IR anti-spoofing model: {}", e.what());
    }
  }
}

void FaceAuth::endAuthenticationSession() {
  ir_camera_session_.reset();
  camera_session_.reset();

  rgb_detector_.reset();
  face_recognition_.reset();
  rgb_antispoof_.reset();
  ir_detector_.reset();
  ir_antispoof_.reset();
}

AuthResult FaceAuth::authenticate(const std::string& username, const AuthConfig& config,
                                  std::atomic<bool>* cancel_signal) {
  if (!camera_session_) {
    camera_session_ = openCameraSession(face_config_.camera);
  }
  if (!camera_session_ || !camera_session_->isOpen()) {
    spdlog::error("FaceAuth: Could not open camera");
    if (!checkCameraAvailability(face_config_.camera)) {
      return AuthResult::Unavailable;
    }
    return AuthResult::Retry;
  }

  std::vector<std::string> enrolledFaces = biopass::listFaces(username);
  if (enrolledFaces.empty()) {
    spdlog::error("FaceAuth: No face enrolled for user {}, skipping", username);
    return AuthResult::Unavailable;
  }

  // Check if cached models are available
  if (!rgb_detector_ || !face_recognition_) {
    spdlog::error("FaceAuth: Models not initialized in session");
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

  std::vector<Detection> detectedImages = rgb_detector_->inference(loginFace);
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

  // NOTE on thread safety: checkAntiSpoof launches async tasks that use the
  // raw pointers below. The function blocks on all task futures before
  // returning, so the pointed-to objects remain alive for the entire call.
  // Do NOT add early-return logic inside checkAntiSpoof without ensuring all
  // async tasks have completed first — otherwise ir_camera_session_.reset()
  // below would cause a use-after-free in the IR capture thread.
  if (!checkAntiSpoof(face_config_, username, detectedImages[0], loginFace.width, loginFace.height,
                      config, ir_camera_session_.get(), rgb_antispoof_.get(), ir_detector_.get(),
                      ir_antispoof_.get())) {
    spdlog::warn("FaceAuth: Anti-spoofing failed — returning Failure (no retry allowed)");
    // Always tear down the IR session so a subsequent call cannot reuse a
    // partially-warmed camera to bypass the check.
    ir_camera_session_.reset();
    return AuthResult::Failure;
  }

  // Match against all enrolled faces — succeed if any match.
  spdlog::debug("FaceAuth: Recognition | threshold={:.3f} enrolled_count={}",
                face_config_.recognition.threshold, enrolledFaces.size());
  for (const auto& facePath : enrolledFaces) {
    ImageRGB preparedFace = readImage(facePath);
    if (preparedFace.empty()) {
      spdlog::warn("FaceAuth: Recognition | could not load enrolled image: {}", facePath);
      continue;
    }

    MatchResult match = face_recognition_->match(preparedFace, face);
    spdlog::debug("FaceAuth: Recognition | face='{}' score={:.4f} threshold={:.3f} similar={}",
                  facePath, match.dist, face_config_.recognition.threshold, match.similar);
    if (match.similar) {
      spdlog::debug("FaceAuth: Recognition PASSED | matched face='{}' score={:.4f}", facePath,
                    match.dist);
      return AuthResult::Success;
    }
  }

  if (config.debug) {
    saveFailedFace(username, face, "not_similar");
  }

  return AuthResult::Retry;
}

FaceAuth::~FaceAuth() = default;

}  // namespace biopass
