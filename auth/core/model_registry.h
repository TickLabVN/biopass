#pragma once

#include <optional>
#include <string>

struct sqlite3;

namespace biopass {

// Path to the per-user biopass.db sqlite database (sibling of config.yaml,
// resolved the same getpwnam-first way as getConfigPath()).
std::string getDbPath(const std::string& username);

// Read-only handle onto a user's biopass.db model catalog. Opens the
// connection once at construction and reuses it for every subsequent lookup
// -- callers that need to resolve several model_ids (e.g. detection +
// recognition + anti-spoofing, or repeated lookups across an authentication
// session) should keep one instance around instead of resolving through a
// function that opens/closes sqlite per call.
class ModelRegistry {
 public:
  explicit ModelRegistry(const std::string& username);
  ~ModelRegistry();

  ModelRegistry(const ModelRegistry&) = delete;
  ModelRegistry& operator=(const ModelRegistry&) = delete;

  // Resolves a model_id (as read from config.yaml's detection/recognition/
  // anti_spoofing model_id fields) to an absolute .onnx file path by looking
  // it up in the `models` table of biopass.db. Returns nullopt on any
  // failure (connection never opened, missing row, sqlite error) rather than
  // throwing -- callers should treat that the same as "model not
  // configured".
  std::optional<std::string> resolveModelPath(const std::string& model_id) const;

 private:
  sqlite3* db_ = nullptr;
};

}  // namespace biopass
