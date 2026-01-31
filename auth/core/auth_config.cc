#include "auth_config.h"
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

namespace facepass {

std::string get_config_path(const std::string &username) {
  // Get home directory for the user
  struct passwd *pw = getpwnam(username.c_str());
  if (pw == nullptr) {
    // Fallback to HOME environment variable
    const char *home = getenv("HOME");
    if (home) {
      return std::string(home) + "/.config/facepass/config.yaml";
    }
    return "/etc/facepass/config.yaml"; // System-wide fallback
  }
  return std::string(pw->pw_dir) + "/.config/facepass/config.yaml";
}

ExecutionMode parse_mode(const std::string &mode_str) {
  if (mode_str == "parallel") {
    return ExecutionMode::Parallel;
  }
  return ExecutionMode::Sequential; // Default
}

FacePassConfig load_config(const std::string &username) {
  FacePassConfig config;

  std::string config_path = get_config_path(username);

  try {
    YAML::Node yaml = YAML::LoadFile(config_path);

    // Parse mode
    if (yaml["mode"]) {
      config.mode = parse_mode(yaml["mode"].as<std::string>());
    }

    // Parse methods
    if (yaml["methods"] && yaml["methods"].IsSequence()) {
      config.methods.clear();
      for (const auto &method : yaml["methods"]) {
        config.methods.push_back(method.as<std::string>());
      }
    }

    // Parse auth config
    if (yaml["auth"]) {
      const auto &auth = yaml["auth"];

      if (auth["retries"]) {
        config.auth.retries = auth["retries"].as<int>();
      }

      if (auth["retry_delay_ms"]) {
        config.auth.retry_delay_ms = auth["retry_delay_ms"].as<int>();
      }

      if (auth["anti_spoof"]) {
        config.auth.anti_spoof = auth["anti_spoof"].as<bool>();
      }
    }

    std::cout << "FacePass: Loaded config from " << config_path << std::endl;

  } catch (const YAML::BadFile &e) {
    std::cerr << "FacePass: Config file not found at " << config_path
              << ", using defaults" << std::endl;
  } catch (const YAML::Exception &e) {
    std::cerr << "FacePass: Failed to parse config: " << e.what()
              << ", using defaults" << std::endl;
  }

  return config;
}

} // namespace facepass
