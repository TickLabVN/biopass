#pragma once

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cerrno>
#include <string>
#include <vector>

#include "auth_manager.h"

namespace biopass {

// ---------------------------------------------------------------------------
// Per-method config structs (mirrors Tauri config.rs)
// ---------------------------------------------------------------------------

struct DetectionConfig {
  std::string model = "models/yolov8n-face.onnx";
  float threshold = 0.5f;
};

struct RecognitionConfig {
  std::string model = "models/edgeface_s_gamma_05.onnx";
  float threshold = 0.8f;
};

struct AntiSpoofingConfig {
  bool enable = false;
  struct ModelConfig {
    std::string path = "models/mobilenetv3_antispoof.onnx";
    float threshold = 0.8f;
  } model;
  // Linux device path, e.g. "/dev/video2". Empty means disabled.
  std::string irCamera;
};

struct FaceMethodConfig {
  bool enable = true;
  int retries = 5;
  int retryDelayMs = 200;
  DetectionConfig detection;
  RecognitionConfig recognition;
  AntiSpoofingConfig antiSpoofing;
};

struct VoiceMethodConfig {
  bool enable = false;
  int retries = 3;
  int retryDelayMs = 500;
  std::string model = "models/voice.onnx";
  float threshold = 0.8f;
};

struct FingerprintMethodConfig {
  bool enable = false;
  int retries = 3;
  int timeout_ms = 1000;
};

struct MethodsConfig {
  FaceMethodConfig face;
  VoiceMethodConfig voice;
  FingerprintMethodConfig fingerprint;
};

// ---------------------------------------------------------------------------
// Top-level config
// ---------------------------------------------------------------------------

/**
 * Complete configuration for biopass.
 * Loaded from ~/.config/com.ticklab.biopass/config.yaml
 */
struct BiopassConfig {
  bool debug = false;
  ExecutionMode mode = ExecutionMode::Parallel;
  std::vector<std::string> methods = {"face"};
  AuthConfig auth = {};
  MethodsConfig methods_config = {};
};
std::string getConfigPath(const std::string &username);
BiopassConfig readConfig(const std::string &username);
bool configExists(const std::string &username);
bool migrateConfigSchema(const std::string &username, std::string *error = nullptr);

std::vector<std::string> listFaces(const std::string &username);
std::string getDebugPath(const std::string &username);
int setupConfig(const std::string &username);

}  // namespace biopass
