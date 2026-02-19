#pragma once

#include "auth_config.h"
#include "auth_method.h"

namespace facepass {

/**
 * Face authentication method.
 * Wraps existing face detection, recognition, and anti-spoofing.
 */
class FaceAuth : public IAuthMethod {
 public:
  explicit FaceAuth(const FaceMethodConfig &config) : face_config_(config) {}
  ~FaceAuth() override = default;

  std::string name() const override { return "Face"; }
  bool is_available() const override;
  AuthResult authenticate(const std::string &username, const AuthConfig &config) override;

 private:
  FaceMethodConfig face_config_;
};

}  // namespace facepass
