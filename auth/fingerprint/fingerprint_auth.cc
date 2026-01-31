#include "fingerprint_auth.h"
#include <iostream>

namespace facepass {

bool FingerprintAuth::is_available() const {
  // TODO: Check if fingerprint reader is available
  // For now, return false since fingerprint auth is not implemented
  return false;
}

AuthResult FingerprintAuth::authenticate(const std::string &username,
                                         const AuthConfig &config) {
  (void)username; // Suppress unused parameter warning
  (void)config;

  std::cerr << "FingerprintAuth: Not implemented yet" << std::endl;
  return AuthResult::Unavailable;
}

} // namespace facepass
