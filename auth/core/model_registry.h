#pragma once

#include <optional>
#include <string>

namespace biopass {

// Path to the per-user biopass.db sqlite database (sibling of config.yaml,
// resolved the same getpwnam-first way as getConfigPath()).
std::string getDbPath(const std::string& username);

// Resolves a model_id (as read from config.yaml's detection/recognition/
// anti_spoofing model_id fields) to an absolute .onnx file path by looking it
// up in the `models` table of biopass.db. Opens the database read-only and
// never creates it. Returns nullopt on any failure (missing DB, missing row,
// sqlite error) rather than throwing -- callers should treat that the same as
// "model not configured".
std::optional<std::string> resolveModelPath(const std::string& username,
                                            const std::string& model_id);

}  // namespace biopass
