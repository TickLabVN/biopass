#pragma once

#include <memory>

#include "auth_config.h"
#include "auth_method.h"
#include "camera_capture.h"
#include "face_detection.h"
#include "face_recognition.h"

namespace biopass {

/**
 * Face authentication method.
 * Wraps existing face detection, recognition, and anti-spoofing.
 */
class FaceAuth : public IAuthMethod {
 public:
  explicit FaceAuth(const FaceMethodConfig& config) : face_config_(config) {}
  ~FaceAuth() override = default;

  std::string name() const override { return "Face"; }
  bool isAvailable() const override;
  uint32_t getRetries() const override { return face_config_.retries; }
  uint32_t getRetryDelayMs() const override { return face_config_.retry_delay; }
  void beginAuthenticationSession() override;
  void endAuthenticationSession() override;
  AuthResult authenticate(const std::string& username, const AuthConfig& config,
                          std::atomic<bool>* cancel_signal = nullptr) override;

 private:
  void ensureIrSession();
  // Loads the detection + recognition models once; returns false if either
  // model file is missing or fails to load.
  bool ensureModelsLoaded();

  FaceMethodConfig face_config_;
  std::unique_ptr<ICameraCaptureSession> camera_session_;
  std::unique_ptr<ICameraCaptureSession> ir_camera_session_;
  std::unique_ptr<FaceDetection> detector_;
  std::unique_ptr<FaceRecognition> recognizer_;
};

}  // namespace biopass
