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

BiopassConfig readConfig(const std::string& username) {
  BiopassConfig config;

  std::string config_path = getConfigPath(username);

  try {
    YAML::Node yaml = YAML::LoadFile(config_path);
    static const std::vector<std::string> supported_methods = {"face", "fingerprint"};

    // 1. Strategy
    if (yaml["strategy"]) {
      const auto& s = yaml["strategy"];
      if (s["debug"])
        config.strategy.debug = s["debug"].as<bool>();
      if (s["execution_mode"])
        config.strategy.execution_mode = s["execution_mode"].as<std::string>();
      if (s["order"] && s["order"].IsSequence()) {
        config.strategy.order.clear();
        for (const auto& m : s["order"]) config.strategy.order.push_back(m.as<std::string>());
      }
      if (s["ignore_services"]) {
        config.strategy.ignore_services.clear();
        if (s["ignore_services"].IsSequence()) {
          for (const auto& item : s["ignore_services"]) {
            if (item.IsScalar()) {
              const auto service = item.as<std::string>();
              if (service.empty()) {
                continue;
              }
              if (std::find(config.strategy.ignore_services.begin(),
                            config.strategy.ignore_services.end(),
                            service) == config.strategy.ignore_services.end()) {
                config.strategy.ignore_services.push_back(service);
              }
            }
          }
        }
      }
    }
    std::vector<std::string> normalized_order;
    for (const auto& method : config.strategy.order) {
      if (std::find(supported_methods.begin(), supported_methods.end(), method) ==
          supported_methods.end()) {
        continue;
      }
      if (std::find(normalized_order.begin(), normalized_order.end(), method) ==
          normalized_order.end()) {
        normalized_order.push_back(method);
      }
    }
    for (const auto& method : supported_methods) {
      if (std::find(normalized_order.begin(), normalized_order.end(), method) ==
          normalized_order.end()) {
        normalized_order.push_back(method);
      }
    }
    config.strategy.order = std::move(normalized_order);

    // 2. Methods — enable flags + model paths
    if (yaml["methods"]) {
      const auto& m = yaml["methods"];

      // Face
      if (m["face"]) {
        const auto& f = m["face"];
        if (f["enable"])
          config.methods.face.enable = f["enable"].as<bool>();
        if (f["retries"])
          config.methods.face.retries = f["retries"].as<uint32_t>();
        if (f["retry_delay"])
          config.methods.face.retry_delay = f["retry_delay"].as<uint32_t>();
        if (f["detection"]) {
          if (f["detection"]["model"])
            config.methods.face.detection.model = f["detection"]["model"].as<std::string>();
          if (f["detection"]["threshold"])
            config.methods.face.detection.threshold = f["detection"]["threshold"].as<float>();
        }
        if (f["recognition"]) {
          if (f["recognition"]["model"])
            config.methods.face.recognition.model = f["recognition"]["model"].as<std::string>();
          if (f["recognition"]["threshold"])
            config.methods.face.recognition.threshold = f["recognition"]["threshold"].as<float>();
        }
        if (f["anti_spoofing"]) {
          const auto& anti_spoofing = f["anti_spoofing"];
          if (anti_spoofing["enable"]) {
            config.methods.face.anti_spoofing.enable = anti_spoofing["enable"].as<bool>();
          }

          if (anti_spoofing["model"]) {
            const auto& model = anti_spoofing["model"];
            if (model.IsMap()) {
              if (model["path"])
                config.methods.face.anti_spoofing.model.path = model["path"].as<std::string>();
              if (model["threshold"])
                config.methods.face.anti_spoofing.model.threshold = model["threshold"].as<float>();
            } else if (model.IsScalar()) {
              // Backward compatibility with old schema:
              // anti_spoofing.model: "<path>"
              config.methods.face.anti_spoofing.model.path = model.as<std::string>();
            }
          }

          // Backward compatibility with old schema:
          // anti_spoofing.threshold: <float>
          if (anti_spoofing["threshold"]) {
            config.methods.face.anti_spoofing.model.threshold =
                anti_spoofing["threshold"].as<float>();
          }

          if (anti_spoofing["ir_camera"] && !anti_spoofing["ir_camera"].IsNull()) {
            config.methods.face.anti_spoofing.ir_camera = anti_spoofing["ir_camera"].as<std::string>();
          }
        }

        // Backward compatibility with old schema:
        // methods.face.ir_camera.enable + methods.face.ir_camera.device_id
        const bool ir_camera_missing = !config.methods.face.anti_spoofing.ir_camera.has_value() ||
                                       config.methods.face.anti_spoofing.ir_camera->empty();
        if (f["ir_camera"] && ir_camera_missing) {
          bool ir_enable = false;
          int ir_device_id = 0;
          if (f["ir_camera"]["enable"])
            ir_enable = f["ir_camera"]["enable"].as<bool>();
          if (f["ir_camera"]["device_id"])
            ir_device_id = f["ir_camera"]["device_id"].as<int>();
          if (ir_enable) {
            config.methods.face.anti_spoofing.ir_camera =
                "/dev/video" + std::to_string(ir_device_id);
          }
        }
      }

      // Fingerprint
      if (m["fingerprint"]) {
        const auto& fp = m["fingerprint"];
        if (fp["enable"])
          config.methods.fingerprint.enable = fp["enable"].as<bool>();
        if (fp["retries"])
          config.methods.fingerprint.retries = fp["retries"].as<uint32_t>();
        if (fp["timeout"])
          config.methods.fingerprint.timeout = fp["timeout"].as<uint32_t>();
        else if (fp["retry_delay"])
          config.methods.fingerprint.timeout = fp["retry_delay"].as<uint32_t>();

        if (fp["fingers"] && fp["fingers"].IsSequence()) {
          config.methods.fingerprint.fingers.clear();
          for (const auto& finger : fp["fingers"]) {
            if (!finger.IsMap()) {
              continue;
            }
            FingerConfig parsed_finger;
            if (finger["name"] && finger["name"].IsScalar()) {
              parsed_finger.name = finger["name"].as<std::string>();
            }
            if (finger["created_at"] && finger["created_at"].IsScalar()) {
              parsed_finger.created_at = finger["created_at"].as<uint64_t>();
            }
            config.methods.fingerprint.fingers.push_back(parsed_finger);
          }
        }
      }
    }

    if (yaml["models"] && yaml["models"].IsSequence()) {
      config.models.clear();
      for (const auto& model : yaml["models"]) {
        if (!model.IsMap()) {
          continue;
        }
        ModelConfig parsed_model;
        if (model["path"] && model["path"].IsScalar()) {
          parsed_model.path = model["path"].as<std::string>();
        }
        if (model["type"] && model["type"].IsScalar()) {
          parsed_model.model_type = model["type"].as<std::string>();
        }
        if (parsed_model.model_type == "voice") {
          continue;
        }
        config.models.push_back(parsed_model);
      }
    }

    if (yaml["appearance"] && yaml["appearance"].IsScalar()) {
      config.appearance = yaml["appearance"].as<std::string>();
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
