#pragma once

#include <vector>

#include "auth_config.h"
#include "auth_method.h"

namespace facepass {

/**
 * Fingerprint authentication method.
 * Placeholder implementation - returns Unavailable until fingerprint auth is
 * implemented.
 */
class FingerprintAuth : public IAuthMethod {
 public:
  explicit FingerprintAuth(const FingerprintMethodConfig &config) : config_(config) {}
  ~FingerprintAuth() override = default;

  std::string name() const override { return "Fingerprint"; }
  bool is_available() const override;
  int get_retries() const override { return config_.retries; }
  int get_retry_delay_ms() const override { return config_.retry_delay_ms; }
  AuthResult authenticate(const std::string &username, const AuthConfig &config) override;
  std::vector<std::string> list_enrolled_fingers(const std::string &username);
  bool enroll(const std::string &username, const std::string &finger_name,
              void (*callback)(bool done, const char *status, void *user_data) = nullptr,
              void *user_data = nullptr);
  bool remove_finger(const std::string &username, const std::string &finger_name);

 private:
  FingerprintMethodConfig config_;
};

}  // namespace facepass
