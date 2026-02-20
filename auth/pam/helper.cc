#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth_config.h"
#include "auth_manager.h"
#include "face_auth.h"
#include "fingerprint_auth.h"
#include "voice_auth.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    spdlog::error("Usage: {} <username>", argv[0]);
    return 2;  // PAM_IGNORE logic / error
  }

  const char *pUsername = argv[1];

  // Load configuration from file
  if (!facepass::config_exists(pUsername)) {
    // User has not configured facepass — skip this module transparently
    return 2;  // PAM_IGNORE
  }
  facepass::FacePassConfig config = facepass::load_config(pUsername);

  if (config.debug) {
    spdlog::set_level(spdlog::level::debug);
  } else {
    spdlog::set_level(spdlog::level::info);
  }

  // Create and configure AuthManager
  facepass::AuthManager manager;
  manager.set_mode(config.mode);
  manager.set_config(config.auth);

  // Add requested authentication methods
  int methods_count = 0;
  for (const auto &method_name : config.methods) {
    if (method_name == "face") {
      manager.add_method(std::make_unique<facepass::FaceAuth>(config.methods_config.face));
      methods_count++;
    } else if (method_name == "voice") {
      manager.add_method(std::make_unique<facepass::VoiceAuth>(config.methods_config.voice));
      methods_count++;
    } else if (method_name == "fingerprint") {
      manager.add_method(
          std::make_unique<facepass::FingerprintAuth>(config.methods_config.fingerprint));
      methods_count++;
    }
  }

  // If no methods are enabled, ignore this module and let PAM jump to the next one
  if (methods_count == 0) {
    return 2;  // PAM_IGNORE
  }

  // Authenticate
  int retval = manager.authenticate(pUsername);

  if (retval == 0 /* PAM_SUCCESS is usually 0 */) {
    return 0;  // PAM_SUCCESS
  } else {
    return 1;  // PAM_AUTH_ERR
  }
}
