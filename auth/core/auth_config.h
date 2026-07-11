#pragma once

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "auth_manager.h"

namespace biopass {

// Bumped whenever the on-disk config.yaml shape changes in a way that isn't
// forward/backward compatible. Mirrors CURRENT_SCHEMA_VERSION in
// app/src-tauri/src/config.rs - keep both in sync. readConfig() falls back to
// defaults (same as a missing/corrupt file) whenever the on-disk
// schema_version doesn't match this exactly; there is no migration on this
// side, only the Tauri app writes config.yaml.
inline constexpr int kCurrentSchemaVersion = 2;

// ---------------------------------------------------------------------------
// Per-method config structs (mirrors Tauri config.rs)
// ---------------------------------------------------------------------------

struct StrategyConfig {
  bool debug = false;
  std::string execution_mode = "parallel";
  std::vector<std::string> order = {"face", "fingerprint"};
  std::vector<std::string> ignore_services = {"polkit-1", "pkexec"};
};

// `model_id` is read verbatim from config.yaml. Resolving it to an absolute
// .onnx path is done on demand via ModelRegistry (see model_registry.h), not
// stored here -- this struct mirrors the config.yaml schema only.
struct DetectionConfig {
  std::string model_id;
  float threshold = 0.5f;
};

struct RecognitionConfig {
  std::string model_id;
  float threshold = 0.5f;
};

struct AntiSpoofingModelConfig {
  std::string model_id;
  float threshold = 0.8f;
};

struct AntiSpoofingConfig {
  bool enable = false;
  AntiSpoofingModelConfig model;
  // Linux device path, e.g. "/dev/video2". nullopt means disabled.
  std::optional<std::string> ir_camera = std::nullopt;
  // Extra delay (ms) inserted after the warmup-frame discard and before the
  // actual IR capture. Gives IR LEDs and auto-exposure time to stabilise.
  // Configurable via anti_spoofing.ir_warmup_delay_ms in config.yaml.
  // NOTE: The IR check verifies that a face-shaped bounding box exists in the
  // IR frame (presence check). It is NOT a full liveness detector. A printed
  // photo that the YOLO model can detect in IR will still pass.
  int ir_warmup_delay_ms = 300;
  // Total wall-clock budget (ms) for the IR presence check to find a face
  // across repeated capture+inference attempts. The IR emitter blinks, so a
  // single frame may be unusable (all-white or all-dark); retrying within
  // this budget catches a good frame. 0 disables retry (single attempt).
  // Configurable via anti_spoofing.ir_presence_timeout_ms in config.yaml.
  int ir_presence_timeout_ms = 1500;
};

struct FaceMethodConfig {
  bool enable = true;
  uint32_t retries = 5;
  uint32_t retry_delay = 200;
  // Linux device path for the primary (visual) camera, e.g. "/dev/video0". nullopt means
  // auto-select.
  std::optional<std::string> camera = std::nullopt;
  DetectionConfig detection;
  RecognitionConfig recognition;
  AntiSpoofingConfig anti_spoofing;
};

// Fingerprint enrollment is not persisted anywhere in config.yaml or
// biopass.db; fprintd over D-Bus is the sole source of truth for which
// fingers are enrolled (see auth/fingerprint/fingerprint_auth.cc and
// app/src-tauri/src/fingerprint.rs), so there is no equivalent field here.
struct FingerprintMethodConfig {
  bool enable = false;
  uint32_t retries = 1;
  uint32_t timeout = 5000;
};

struct MethodsConfig {
  FaceMethodConfig face;
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
  int schema_version = 0;
  StrategyConfig strategy = {};
  MethodsConfig methods = {};
  std::string appearance = "system";
};
std::string getConfigPath(const std::string& username);
BiopassConfig readConfig(const std::string& username);
bool configExists(const std::string& username);

std::vector<std::string> listFaces(const std::string& username);
std::string getDebugPath(const std::string& username);
int setupConfig(const std::string& username);

}  // namespace biopass
