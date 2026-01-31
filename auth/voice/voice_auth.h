#pragma once

#include "auth_method.h"

namespace facepass {

/**
 * Voice authentication method.
 * Placeholder implementation - returns Unavailable until voice auth is
 * implemented.
 */
class VoiceAuth : public IAuthMethod {
public:
  VoiceAuth() = default;
  ~VoiceAuth() override = default;

  std::string name() const override { return "Voice"; }
  bool is_available() const override;
  AuthResult authenticate(const std::string &username,
                          const AuthConfig &config) override;
};

} // namespace facepass
