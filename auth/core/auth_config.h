#pragma once

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cerrno>
#include <string>
#include <vector>

#include "auth_manager.h"

namespace facepass {

// ---------------------------------------------------------------------------
// Per-method config structs (mirrors Tauri config.rs)
// ---------------------------------------------------------------------------

struct DetectionConfig {
  std::string model = "models/yolov11n-face.torchscript";
  float threshold = 0.5f;
};

struct RecognitionConfig {
  std::string model = "models/edgeface_s_gamma_05_ts.pt";
  float threshold = 0.8f;
};

struct AntiSpoofingConfig {
  bool enable = false;
  std::string model = "models/mobilenetv3_antispoof_ts.pt";
  float threshold = 0.8f;
};

struct FaceMethodConfig {
  bool enable = true;
  DetectionConfig detection;
  RecognitionConfig recognition;
  AntiSpoofingConfig anti_spoofing;
};

struct VoiceMethodConfig {
  bool enable = false;
  std::string model = "models/voice.onnx";
  float threshold = 0.8f;
};

struct FingerprintMethodConfig {
  bool enable = false;
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
 * Complete configuration for facepass.
 * Loaded from ~/.config/com.ticklab.facepass/config.yaml
 */
struct FacePassConfig {
  bool debug = false;
  ExecutionMode mode = ExecutionMode::Sequential;
  std::vector<std::string> methods = {"face"};
  AuthConfig auth = {};
  MethodsConfig methods_config = {};
};

// ---------------------------------------------------------------------------
// Config loading
// ---------------------------------------------------------------------------

FacePassConfig load_config(const std::string &username);
std::string get_config_path(const std::string &username);
bool config_exists(const std::string &username);

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

// Returns the base data directory for a user.
std::string user_data_dir(const std::string &username);

// Returns the path to the faces directory for a user.
std::string user_faces_dir(const std::string &username);

// Returns all enrolled face image paths for a user (jpg/png).
std::vector<std::string> list_user_faces(const std::string &username);

// Returns the path to the debug directory for a user.
std::string debug_path(const std::string &username);

// Creates required data directories for a user.
int setup_config(const std::string &username);

}  // namespace facepass
