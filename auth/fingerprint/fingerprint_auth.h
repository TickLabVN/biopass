#pragma once

#include "auth_method.h"

namespace facepass {

/**
 * Fingerprint authentication method.
 * Placeholder implementation - returns Unavailable until fingerprint auth is
 * implemented.
 */
class FingerprintAuth : public IAuthMethod {
 public:
  FingerprintAuth() = default;
  ~FingerprintAuth() override = default;

  std::string name() const override { return "Fingerprint"; }
  bool is_available() const override;
  AuthResult authenticate(const std::string &username, const AuthConfig &config) override;
};

}  // namespace facepass
