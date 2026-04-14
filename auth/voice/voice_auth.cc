#include "voice_auth.h"

#include <spdlog/spdlog.h>

namespace biopass {

bool VoiceAuth::isAvailable() const {
  // TODO: Check if microphone is available
  // For now, return false since voice auth is not implemented
  return false;
}

AuthResult VoiceAuth::authenticate(const std::string &username, const AuthConfig &config,
                                   std::atomic<bool> *cancel_signal) {
  (void)username;  // Suppress unused parameter warning
  (void)config;

  spdlog::error("VoiceAuth: Not implemented yet");
  return AuthResult::Unavailable;
}

}  // namespace biopass
