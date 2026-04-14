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
  bool isAvailable() const override;
  int getRetries() const override { return config_.retries; }
  int getRetryDelayMs() const override { return config_.retryDelayMs; }
  AuthResult authenticate(const std::string &username, const AuthConfig &config,
                          std::atomic<bool> *cancel_signal = nullptr) override;

 private:
  VoiceMethodConfig config_;
};

}  // namespace biopass
