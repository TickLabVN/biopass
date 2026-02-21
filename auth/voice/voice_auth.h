#pragma once

#include "auth_config.h"
#include "auth_method.h"

namespace biopass {

/**
 * Voice authentication method.
 * Placeholder implementation - returns Unavailable until voice auth is
 * implemented.
 */
class VoiceAuth : public IAuthMethod {
 public:
  explicit VoiceAuth(const VoiceMethodConfig &config) : config_(config) {}
  ~VoiceAuth() override = default;

  std::string name() const override { return "Voice"; }
  bool is_available() const override;
  int get_retries() const override { return config_.retries; }
  int get_retry_delay_ms() const override { return config_.retry_delay_ms; }
  AuthResult authenticate(const std::string &username, const AuthConfig &config,
                          std::atomic<bool> *cancel_signal = nullptr) override;

 private:
  VoiceMethodConfig config_;
};

}  // namespace biopass
