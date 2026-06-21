#pragma once

#include <memory>

#include "auth_config.h"
#include "auth_method.h"
#include "camera_capture.h"

class FaceDetection;
class FaceRecognition;
class FaceAntiSpoofing;

namespace biopass {

/**
 * Face authentication method.
 * Wraps existing face detection, recognition, and anti-spoofing.
 */
class FaceAuth : public IAuthMethod {
 public:
  explicit FaceAuth(const FaceMethodConfig& config);
  ~FaceAuth() override;

  std::string name() const override { return "Face"; }
  bool isAvailable() const override;
  uint32_t getRetries() const override { return face_config_.retries; }
  uint32_t getRetryDelayMs() const override { return face_config_.retry_delay; }
  void beginAuthenticationSession() override;
  void endAuthenticationSession() override;
  AuthResult authenticate(const std::string& username, const AuthConfig& config,
                          std::atomic<bool>* cancel_signal = nullptr) override;

 private:
  FaceMethodConfig face_config_;
  std::unique_ptr<ICameraCaptureSession> camera_session_;
  std::unique_ptr<ICameraCaptureSession> ir_camera_session_;

  // Cached model sessions to prevent loading overhead on every attempt/retry
  std::unique_ptr<FaceDetection> rgb_detector_;
  std::unique_ptr<FaceRecognition> face_recognition_;
  std::unique_ptr<FaceAntiSpoofing> rgb_antispoof_;
  std::unique_ptr<FaceDetection> ir_detector_;
  std::unique_ptr<FaceAntiSpoofing> ir_antispoof_;
};

}  // namespace biopass
