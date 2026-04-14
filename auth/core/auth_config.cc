#include "auth_config.h"

#include <pwd.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>

namespace biopass {

std::string getConfigPath(const std::string& username) {
  struct passwd* pw = getpwnam(username.c_str());
  if (pw == nullptr) {
    const char* home = getenv("HOME");
    if (home) {
      return std::string(home) + "/.config/com.ticklab.biopass/config.yaml";
    }
    return "/etc/com.ticklab.biopass/config.yaml";
  }
  return std::string(pw->pw_dir) + "/.config/com.ticklab.biopass/config.yaml";
}

bool configExists(const std::string& username) {
  std::ifstream f(getConfigPath(username));
  return f.good();
}

std::string user_data_dir(const std::string& username) {
  struct passwd* pw = getpwnam(username.c_str());
  if (pw != nullptr) {
    return std::string(pw->pw_dir) + "/.local/share/com.ticklab.biopass";
  }
  const char* home = getenv("HOME");
  if (home)
    return std::string(home) + "/.local/share/com.ticklab.biopass";
  return "";
}

static ExecutionMode parse_mode(const std::string& mode_str) {
  if (mode_str == "sequential")
    return ExecutionMode::Sequential;
  return ExecutionMode::Parallel;
}

BiopassConfig readConfig(const std::string& username) {
  BiopassConfig config;

  std::string config_path = getConfigPath(username);

  try {
    YAML::Node yaml = YAML::LoadFile(config_path);

    // 1. Strategy
    if (yaml["strategy"]) {
      const auto& s = yaml["strategy"];
      if (s["debug"]) {
        config.debug = s["debug"].as<bool>();
        config.auth.debug = config.debug;
      }
      if (s["execution_mode"])
        config.mode = parse_mode(s["execution_mode"].as<std::string>());
      if (s["order"] && s["order"].IsSequence()) {
        config.methods.clear();
        for (const auto& m : s["order"]) config.methods.push_back(m.as<std::string>());
      }
    }

    // 2. Methods — enable flags + model paths
    if (yaml["methods"]) {
      const auto& m = yaml["methods"];

      // Face
      if (m["face"]) {
        const auto& f = m["face"];
        if (f["enable"])
          config.methods_config.face.enable = f["enable"].as<bool>();
        if (f["retries"])
          config.methods_config.face.retries = f["retries"].as<int>();
        if (f["retry_delay"])
          config.methods_config.face.retryDelayMs = f["retry_delay"].as<int>();
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
          const auto& anti_spoofing = f["anti_spoofing"];
          if (anti_spoofing["enable"]) {
            config.methods_config.face.antiSpoofing.enable =
                anti_spoofing["enable"].as<bool>();
          }

          if (anti_spoofing["model"]) {
            const auto& model = anti_spoofing["model"];
            if (model.IsMap()) {
              if (model["path"])
                config.methods_config.face.antiSpoofing.model.path = model["path"].as<std::string>();
              if (model["threshold"])
                config.methods_config.face.antiSpoofing.model.threshold =
                    model["threshold"].as<float>();
            } else if (model.IsScalar()) {
              // Backward compatibility with old schema:
              // anti_spoofing.model: "<path>"
              config.methods_config.face.antiSpoofing.model.path = model.as<std::string>();
            }
          }

          // Backward compatibility with old schema:
          // anti_spoofing.threshold: <float>
          if (anti_spoofing["threshold"]) {
            config.methods_config.face.antiSpoofing.model.threshold =
                anti_spoofing["threshold"].as<float>();
          }

          if (anti_spoofing["ir_camera"] && !anti_spoofing["ir_camera"].IsNull()) {
            config.methods_config.face.antiSpoofing.irCamera =
                anti_spoofing["ir_camera"].as<std::string>();
          }
        }

        // Backward compatibility with old schema:
        // methods.face.ir_camera.enable + methods.face.ir_camera.device_id
        if (f["ir_camera"] && config.methods_config.face.antiSpoofing.irCamera.empty()) {
          bool ir_enable = false;
          int ir_device_id = 0;
          if (f["ir_camera"]["enable"])
            ir_enable = f["ir_camera"]["enable"].as<bool>();
          if (f["ir_camera"]["device_id"])
            ir_device_id = f["ir_camera"]["device_id"].as<int>();
          if (ir_enable) {
            config.methods_config.face.antiSpoofing.irCamera =
                "/dev/video" + std::to_string(ir_device_id);
          }
        }

        config.auth.antispoof = config.methods_config.face.antiSpoofing.enable ||
                                 !config.methods_config.face.antiSpoofing.irCamera.empty();
      }

      // Voice
      if (m["voice"]) {
        const auto& v = m["voice"];
        if (v["enable"])
          config.methods_config.voice.enable = v["enable"].as<bool>();
        if (v["retries"])
          config.methods_config.voice.retries = v["retries"].as<int>();
        if (v["retry_delay"])
          config.methods_config.voice.retryDelayMs = v["retry_delay"].as<int>();
        if (v["model"])
          config.methods_config.voice.model = v["model"].as<std::string>();
        if (v["threshold"])
          config.methods_config.voice.threshold = v["threshold"].as<float>();
      }

      // Fingerprint
      if (m["fingerprint"]) {
        const auto& fp = m["fingerprint"];
        if (fp["enable"])
          config.methods_config.fingerprint.enable = fp["enable"].as<bool>();
        if (fp["retries"])
          config.methods_config.fingerprint.retries = fp["retries"].as<int>();
        if (fp["timeout"])
          config.methods_config.fingerprint.timeout_ms = fp["timeout"].as<int>();
        else if (fp["retry_delay"])
          config.methods_config.fingerprint.timeout_ms = fp["retry_delay"].as<int>();
      }

      // Filter method list to only enabled methods
      std::vector<std::string> enabled;
      for (const auto& name : config.methods) {
        if (name == "face" && config.methods_config.face.enable)
          enabled.push_back(name);
        else if (name == "voice" && config.methods_config.voice.enable)
          enabled.push_back(name);
        else if (name == "fingerprint" && config.methods_config.fingerprint.enable)
          enabled.push_back(name);
      }
      config.methods = enabled;
    }
  } catch (const YAML::BadFile& e) {
    spdlog::warn("Biopass: Config file not found at {}, using defaults", config_path);
  } catch (const YAML::Exception& e) {
    spdlog::error("Biopass: Failed to parse config: {}, using defaults", e.what());
  }

  return config;
}

bool migrateConfigSchema(const std::string& username, std::string* error) {
  const std::string config_path = getConfigPath(username);
  try {
    YAML::Node yaml = YAML::LoadFile(config_path);
    if (!yaml["methods"] || !yaml["methods"]["face"]) {
      return true;
    }

    YAML::Node face = yaml["methods"]["face"];
    YAML::Node anti = face["anti_spoofing"];

    bool enable = false;
    std::string model_path = "models/mobilenetv3_antispoof.onnx";
    float threshold = 0.8f;
    std::string ir_camera_path;

    if (anti) {
      if (anti["enable"]) {
        enable = anti["enable"].as<bool>();
      }

      if (anti["model"]) {
        YAML::Node model = anti["model"];
        if (model.IsMap()) {
          if (model["path"])
            model_path = model["path"].as<std::string>();
          if (model["threshold"])
            threshold = model["threshold"].as<float>();
        } else if (model.IsScalar()) {
          model_path = model.as<std::string>();
        }
      }

      // Old schema path.
      if (anti["threshold"]) {
        threshold = anti["threshold"].as<float>();
      }

      if (anti["ir_camera"] && !anti["ir_camera"].IsNull()) {
        ir_camera_path = anti["ir_camera"].as<std::string>();
      }
    }

    if (ir_camera_path.empty() && face["ir_camera"]) {
      bool ir_enable = false;
      int ir_device_id = 0;
      if (face["ir_camera"]["enable"]) {
        ir_enable = face["ir_camera"]["enable"].as<bool>();
      }
      if (face["ir_camera"]["device_id"]) {
        ir_device_id = face["ir_camera"]["device_id"].as<int>();
      }
      if (ir_enable) {
        ir_camera_path = "/dev/video" + std::to_string(ir_device_id);
      }
    }

    // Idempotency guard:
    // If config is already in the new schema and there are no legacy fields left,
    // skip writing to disk.
    const bool has_legacy_face_ir = static_cast<bool>(face["ir_camera"]);
    const bool has_legacy_anti_threshold = anti && static_cast<bool>(anti["threshold"]);
    const bool has_legacy_anti_model_scalar =
        anti && static_cast<bool>(anti["model"]) && anti["model"].IsScalar();
    const bool has_new_model_map = anti && static_cast<bool>(anti["model"]) && anti["model"].IsMap() &&
                                   static_cast<bool>(anti["model"]["path"]) &&
                                   static_cast<bool>(anti["model"]["threshold"]);
    const bool has_new_ir_key = anti && static_cast<bool>(anti["ir_camera"]);
    const bool needs_migration = has_legacy_face_ir || has_legacy_anti_threshold ||
                                 has_legacy_anti_model_scalar || !has_new_model_map ||
                                 !has_new_ir_key;
    if (!needs_migration) return true;

    YAML::Node anti_new;
    anti_new["enable"] = enable;
    YAML::Node model_new;
    model_new["path"] = model_path;
    model_new["threshold"] = threshold;
    anti_new["model"] = model_new;
    if (ir_camera_path.empty()) {
      anti_new["ir_camera"] = YAML::Node();
    } else {
      anti_new["ir_camera"] = ir_camera_path;
    }

    face["anti_spoofing"] = anti_new;
    if (face["ir_camera"]) {
      face.remove("ir_camera");
    }
    yaml["methods"]["face"] = face;

    std::ofstream out(config_path, std::ios::trunc);
    if (!out.is_open()) {
      if (error)
        *error = "Failed to open config for writing: " + config_path;
      return false;
    }
    out << yaml;
    return true;
  } catch (const YAML::BadFile&) {
    // No config yet is valid; migration is a no-op.
    return true;
  } catch (const std::exception& e) {
    if (error)
      *error = e.what();
    return false;
  }
}

// ---------------------------------------------------------------------------
// Directory / path helpers
// ---------------------------------------------------------------------------

static int mkdir_p(const std::string& path) {
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

std::vector<std::string> listFaces(const std::string& username) {
  std::vector<std::string> faces;
  std::string dir = user_data_dir(username) + "/faces";
  DIR* dp = opendir(dir.c_str());
  if (!dp)
    return faces;

  struct dirent* entry;
  while ((entry = readdir(dp)) != nullptr) {
    std::string name(entry->d_name);
    if (name.size() > 4) {
      std::string ext = name.substr(name.size() - 4);
      if (ext == ".jpg" || ext == ".JPG" || ext == ".png" || ext == ".PNG" || ext == ".bmp" ||
          ext == ".BMP" || ext == ".tga" || ext == ".TGA") {
        faces.push_back(dir + "/" + name);
      } else if (name.size() > 5) {
        std::string ext5 = name.substr(name.size() - 5);
        if (ext5 == ".jpeg" || ext5 == ".JPEG") {
          faces.push_back(dir + "/" + name);
        }
      }
    }
  }
  closedir(dp);
  std::sort(faces.begin(), faces.end());
  return faces;
}

std::string getDebugPath(const std::string& username) { return user_data_dir(username) + "/debugs"; }

int setupConfig(const std::string& username) {
  const std::string dataDir = user_data_dir(username);
  if (mkdir_p(dataDir + "/faces") != 0)
    return 1;
  return mkdir_p(dataDir + "/debugs");
}

}  // namespace biopass
