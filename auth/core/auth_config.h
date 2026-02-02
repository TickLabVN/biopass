#pragma once

#include <string>
#include <vector>

#include "auth_manager.h"

namespace facepass {

/**
 * Complete configuration for facepass.
 * Loaded from ~/.config/facepass/config.yaml
 */
struct FacePassConfig {
  ExecutionMode mode = ExecutionMode::Sequential;
  std::vector<std::string> methods = {"face"};
  AuthConfig auth = {};
};

/**
 * Load configuration from the user's config file.
 * @param username The PAM username (used to find home directory).
 * @return FacePassConfig struct with loaded or default values.
 */
FacePassConfig load_config(const std::string &username);

/**
 * Get the path to the config file for a user.
 */
std::string get_config_path(const std::string &username);

}  // namespace facepass
