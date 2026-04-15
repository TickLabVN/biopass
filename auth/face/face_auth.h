#pragma once

#include <memory>

#include "auth_config.h"
#include "auth_method.h"
#include "camera_capture.h"

namespace biopass {

/**
 * Face authentication method.
 * Wraps existing face detection, recognition, and anti-spoofing.
 */
class FaceAuth : public IAuthMethod {
 public:
  explicit FaceAuth(const FaceMethodConfig &config) : face_config_(config) {}
  ~FaceAuth() override = default;

  std::string name() const override { return "Face"; }
  bool isAvailable() const override;
  int getRetries() const override { return face_config_.retries; }
  int getRetryDelayMs() const override { return face_config_.retryDelayMs; }
  void beginAuthenticationSession() override;
  void endAuthenticationSession() override;
  AuthResult authenticate(const std::string &username, const AuthConfig &config,
                          std::atomic<bool> *cancel_signal = nullptr) override;

 private:
  FaceMethodConfig face_config_;
  std::unique_ptr<ICameraCaptureSession> camera_session_;
  std::unique_ptr<ICameraCaptureSession> ir_camera_session_;
};

}  // namespace biopass
