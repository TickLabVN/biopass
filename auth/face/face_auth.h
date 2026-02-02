#pragma once

#include "auth_method.h"

namespace facepass {

/**
 * Face authentication method.
 * Wraps existing face detection, recognition, and anti-spoofing.
 */
class FaceAuth : public IAuthMethod {
 public:
  FaceAuth() = default;
  ~FaceAuth() override = default;

  std::string name() const override { return "Face"; }
  bool is_available() const override;
  AuthResult authenticate(const std::string &username, const AuthConfig &config) override;
};

}  // namespace facepass
