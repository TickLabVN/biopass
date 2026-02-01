#include "voice_auth.h"

#include <iostream>

namespace facepass {

bool VoiceAuth::is_available() const {
  // TODO: Check if microphone is available
  // For now, return false since voice auth is not implemented
  return false;
}

AuthResult VoiceAuth::authenticate(const std::string &username, const AuthConfig &config) {
  (void)username;  // Suppress unused parameter warning
  (void)config;

  std::cerr << "VoiceAuth: Not implemented yet" << std::endl;
  return AuthResult::Unavailable;
}

}  // namespace facepass
