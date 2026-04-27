#pragma once

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdint>
#include <cerrno>
#include <optional>
#include <string>
#include <vector>

#include "auth_manager.h"

namespace biopass {

// ---------------------------------------------------------------------------
// Per-method config structs (mirrors Tauri config.rs)
// ---------------------------------------------------------------------------

struct StrategyConfig {
  bool debug = false;
  std::string execution_mode = "parallel";
  std::vector<std::string> order = {"face", "fingerprint"};
  std::vector<std::string> ignore_services = {"polkit-1", "pkexec"};
};

struct DetectionConfig {
  std::string model = "models/yolov8n-face.onnx";
  float threshold = 0.8f;
};

struct RecognitionConfig {
  std::string model = "models/edgeface_s_gamma_05.onnx";
  float threshold = 0.8f;
};

struct AntiSpoofingModelConfig {
  std::string path = "models/mobilenetv3_antispoof.onnx";
  float threshold = 0.8f;
};

struct AntiSpoofingConfig {
  bool enable = false;
  AntiSpoofingModelConfig model;
  // Linux device path, e.g. "/dev/video2". nullopt means disabled.
  std::optional<std::string> ir_camera = std::nullopt;
};

struct FaceMethodConfig {
  bool enable = true;
  uint32_t retries = 5;
  uint32_t retry_delay = 200;
  DetectionConfig detection;
  RecognitionConfig recognition;
  AntiSpoofingConfig anti_spoofing;
};

struct FingerConfig {
  std::string name;
  uint64_t created_at = 0;
};

struct FingerprintMethodConfig {
  bool enable = false;
  uint32_t retries = 1;
  uint32_t timeout = 5000;
  std::vector<FingerConfig> fingers;
};

struct MethodsConfig {
  FaceMethodConfig face;
  FingerprintMethodConfig fingerprint;
};

struct ModelConfig {
  std::string path;
  std::string model_type;
};

// ---------------------------------------------------------------------------
// Top-level config
// ---------------------------------------------------------------------------

/**
 * Complete configuration for biopass.
 * Loaded from ~/.config/com.ticklab.biopass/config.yaml
 */
struct BiopassConfig {
  StrategyConfig strategy = {};
  MethodsConfig methods = {};
  std::vector<ModelConfig> models = {};
  std::string appearance = "system";
};
std::string getConfigPath(const std::string &username);
BiopassConfig readConfig(const std::string &username);
bool configExists(const std::string &username);
bool migrateConfigSchema(const std::string &username, std::string *error = nullptr);

std::vector<std::string> listFaces(const std::string &username);
std::string getDebugPath(const std::string &username);
int setupConfig(const std::string &username);

}  // namespace biopass
