#pragma once

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cerrno>
#include <string>
#include <vector>

#include "auth_manager.h"

namespace facepass {

/**
 * Complete configuration for facepass.
 * Loaded from ~/.config/com.ticklab.facepass/config.yaml
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

/**
 * Returns true if the user has a facepass config file.
 * Used to skip authentication gracefully for unconfigured users.
 */
bool config_exists(const std::string &username);

// Model types available for inference
enum ModelType {
  FACE_DETECTION,
  FACE_RECOGNITION,
  FACE_ANTI_SPOOFING,
};

// Returns the path to the registered face image for a user.
std::string user_face_path(const std::string &username);

// Returns the path to the debug directory for a user.
std::string debug_path(const std::string &username);

// Returns the path to a model file by type.
std::string model_path(const std::string &username, const ModelType &modelType);

// Creates required data directories for a user.
int setup_config(const std::string &username);

}  // namespace facepass
