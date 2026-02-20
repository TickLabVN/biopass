#include "auth_config.h"

#include <pwd.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>
#include <iostream>

namespace biopass {

std::string get_config_path(const std::string &username) {
  struct passwd *pw = getpwnam(username.c_str());
  if (pw == nullptr) {
    const char *home = getenv("HOME");
    if (home) {
      return std::string(home) + "/.config/com.ticklab.biopass/config.yaml";
    }
    return "/etc/com.ticklab.biopass/config.yaml";
  }
  return std::string(pw->pw_dir) + "/.config/com.ticklab.biopass/config.yaml";
}

bool config_exists(const std::string &username) {
  std::ifstream f(get_config_path(username));
  return f.good();
}

std::string user_data_dir(const std::string &username) {
  struct passwd *pw = getpwnam(username.c_str());
  if (pw != nullptr) {
    return std::string(pw->pw_dir) + "/.local/share/com.ticklab.biopass";
  }
  const char *home = getenv("HOME");
  if (home)
    return std::string(home) + "/.local/share/com.ticklab.biopass";
  return "";
}

static ExecutionMode parse_mode(const std::string &mode_str) {
  if (mode_str == "parallel")
    return ExecutionMode::Parallel;
  return ExecutionMode::Sequential;
}

BiopassConfig load_config(const std::string &username) {
  BiopassConfig config;

  std::string config_path = get_config_path(username);

  try {
    YAML::Node yaml = YAML::LoadFile(config_path);

    // 1. Strategy
    if (yaml["strategy"]) {
      const auto &s = yaml["strategy"];
      if (s["debug"]) {
        config.debug = s["debug"].as<bool>();
        config.auth.debug = config.debug;
      }
      if (s["execution_mode"])
        config.mode = parse_mode(s["execution_mode"].as<std::string>());
      if (s["order"] && s["order"].IsSequence()) {
        config.methods.clear();
        for (const auto &m : s["order"]) config.methods.push_back(m.as<std::string>());
      }
    }

    // 2. Methods — enable flags + model paths
    if (yaml["methods"]) {
      const auto &m = yaml["methods"];

      // Face
      if (m["face"]) {
        const auto &f = m["face"];
        if (f["enable"])
          config.methods_config.face.enable = f["enable"].as<bool>();
        if (f["retries"])
          config.methods_config.face.retries = f["retries"].as<int>();
        if (f["retry_delay"])
          config.methods_config.face.retry_delay_ms = f["retry_delay"].as<int>();
        if (f["detection"]) {
          if (f["detection"]["model"])
            config.methods_config.face.detection.model = f["detection"]["model"].as<std::string>();
          if (f["detection"]["threshold"])
            config.methods_config.face.detection.threshold =
                f["detection"]["threshold"].as<float>();
        }
        if (f["recognition"]) {
          if (f["recognition"]["model"])
            config.methods_config.face.recognition.model =
                f["recognition"]["model"].as<std::string>();
          if (f["recognition"]["threshold"])
            config.methods_config.face.recognition.threshold =
                f["recognition"]["threshold"].as<float>();
        }
        if (f["anti_spoofing"]) {
          if (f["anti_spoofing"]["enable"]) {
            config.methods_config.face.anti_spoofing.enable =
                f["anti_spoofing"]["enable"].as<bool>();
            config.auth.anti_spoof = config.methods_config.face.anti_spoofing.enable;
          }
          if (f["anti_spoofing"]["model"])
            config.methods_config.face.anti_spoofing.model =
                f["anti_spoofing"]["model"].as<std::string>();
          if (f["anti_spoofing"]["threshold"])
            config.methods_config.face.anti_spoofing.threshold =
                f["anti_spoofing"]["threshold"].as<float>();
        }
      }

      // Voice
      if (m["voice"]) {
        const auto &v = m["voice"];
        if (v["enable"])
          config.methods_config.voice.enable = v["enable"].as<bool>();
        if (v["retries"])
          config.methods_config.voice.retries = v["retries"].as<int>();
        if (v["retry_delay"])
          config.methods_config.voice.retry_delay_ms = v["retry_delay"].as<int>();
        if (v["model"])
          config.methods_config.voice.model = v["model"].as<std::string>();
        if (v["threshold"])
          config.methods_config.voice.threshold = v["threshold"].as<float>();
      }

      // Fingerprint
      if (m["fingerprint"]) {
        const auto &fp = m["fingerprint"];
        if (fp["enable"])
          config.methods_config.fingerprint.enable = fp["enable"].as<bool>();
        if (fp["retries"])
          config.methods_config.fingerprint.retries = fp["retries"].as<int>();
        if (fp["retry_delay"])
          config.methods_config.fingerprint.retry_delay_ms = fp["retry_delay"].as<int>();
      }

      // Filter method list to only enabled methods
      std::vector<std::string> enabled;
      for (const auto &name : config.methods) {
        if (name == "face" && config.methods_config.face.enable)
          enabled.push_back(name);
        else if (name == "voice" && config.methods_config.voice.enable)
          enabled.push_back(name);
        else if (name == "fingerprint" && config.methods_config.fingerprint.enable)
          enabled.push_back(name);
      }
      config.methods = enabled;
    }
  } catch (const YAML::BadFile &e) {
    std::cerr << "Biopass: Config file not found at " << config_path << ", using defaults"
              << std::endl;
  } catch (const YAML::Exception &e) {
    std::cerr << "Biopass: Failed to parse config: " << e.what() << ", using defaults" << std::endl;
  }

  return config;
}

// ---------------------------------------------------------------------------
// Directory / path helpers
// ---------------------------------------------------------------------------

static int mkdir_p(const std::string &path) {
  size_t pos = 0;
  std::string dir;
  int ret;
  while ((pos = path.find('/', pos)) != std::string::npos) {
    dir = path.substr(0, pos++);
    if (dir.empty())
      continue;
    ret = mkdir(dir.c_str(), 0777);
    if (ret == -1 && errno != EEXIST) {
      perror("Failed to create directory");
      return 1;
    }
  }
  ret = mkdir(path.c_str(), 0777);
  if (ret == -1 && errno != EEXIST) {
    perror("Failed to create directory");
    return 1;
  }
  return 0;
}

std::string user_faces_dir(const std::string &username) {
  return user_data_dir(username) + "/faces";
}

std::vector<std::string> list_user_faces(const std::string &username) {
  std::vector<std::string> faces;
  std::string dir = user_faces_dir(username);
  DIR *dp = opendir(dir.c_str());
  if (!dp)
    return faces;

  struct dirent *entry;
  while ((entry = readdir(dp)) != nullptr) {
    std::string name(entry->d_name);
    if (name.size() > 4) {
      std::string ext = name.substr(name.size() - 4);
      if (ext == ".jpg" || ext == ".JPG" || ext == ".png" || ext == ".PNG") {
        faces.push_back(dir + "/" + name);
      }
    }
  }
  closedir(dp);
  std::sort(faces.begin(), faces.end());
  return faces;
}

std::string debug_path(const std::string &username) { return user_data_dir(username) + "/debugs"; }

int setup_config(const std::string &username) {
  const std::string dataDir = user_data_dir(username);
  if (mkdir_p(dataDir + "/faces") != 0)
    return 1;
  return mkdir_p(dataDir + "/debugs");
}

}  // namespace biopass
