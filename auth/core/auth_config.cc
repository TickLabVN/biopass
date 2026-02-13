#include "auth_config.h"

#include <pwd.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iostream>

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
    return "/etc/facepass/config.yaml";  // System-wide fallback
  }
  return std::string(pw->pw_dir) + "/.config/facepass/config.yaml";
}

ExecutionMode parse_mode(const std::string &mode_str) {
  if (mode_str == "parallel") {
    return ExecutionMode::Parallel;
  }
  return ExecutionMode::Sequential;  // Default
}

FacePassConfig load_config(const std::string &username) {
  FacePassConfig config;

  std::string config_path = get_config_path(username);

  try {
    YAML::Node yaml = YAML::LoadFile(config_path);

    // 1. Parse Strategy settings
    if (yaml["strategy"]) {
      const auto &strategy = yaml["strategy"];

      if (strategy["execution_mode"]) {
        config.mode = parse_mode(strategy["execution_mode"].as<std::string>());
      }

      if (strategy["retries"]) {
        config.auth.retries = strategy["retries"].as<int>();
      }

      if (strategy["retry_delay"]) {
        config.auth.retry_delay_ms = strategy["retry_delay"].as<int>();
      }

      // Load prioritized order of methods
      if (strategy["order"] && strategy["order"].IsSequence()) {
        config.methods.clear();
        for (const auto &method : strategy["order"]) {
          config.methods.push_back(method.as<std::string>());
        }
      }
    }

    // 2. Filter methods based on 'enable' flag in 'methods' block
    if (yaml["methods"]) {
      const auto &methods_node = yaml["methods"];
      std::vector<std::string> enabled_methods;

      for (const auto &method_name : config.methods) {
        if (methods_node[method_name] && methods_node[method_name]["enable"]) {
          if (methods_node[method_name]["enable"].as<bool>()) {
            enabled_methods.push_back(method_name);
          }
        }
      }
      config.methods = enabled_methods;

      // Check anti-spoofing specifically for face if available
      if (methods_node["face"] && methods_node["face"]["anti_spoofing"] &&
          methods_node["face"]["anti_spoofing"]["enable"]) {
        config.auth.anti_spoof = methods_node["face"]["anti_spoofing"]["enable"].as<bool>();
      }
    }

    std::cout << "FacePass: Loaded config from " << config_path << std::endl;
  } catch (const YAML::BadFile &e) {
    std::cerr << "FacePass: Config file not found at " << config_path << ", using defaults"
              << std::endl;
  } catch (const YAML::Exception &e) {
    std::cerr << "FacePass: Failed to parse config: " << e.what() << ", using defaults"
              << std::endl;
  }

  return config;
}

}  // namespace facepass
