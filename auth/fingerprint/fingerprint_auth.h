#pragma once

#include <vector>

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
  std::vector<std::string> list_enrolled_fingers(const std::string &username);
  bool enroll(const std::string &username, const std::string &finger_name);
  bool remove_finger(const std::string &username, const std::string &finger_name);
};

}  // namespace facepass
