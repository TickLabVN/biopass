#include "model_registry.h"

#include <pwd.h>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <unistd.h>

namespace biopass {

std::string getDbPath(const std::string& username) {
  struct passwd* pw = getpwnam(username.c_str());
  if (pw == nullptr) {
    const char* home = getenv("HOME");
    if (home) {
      return std::string(home) + "/.config/com.ticklab.biopass/biopass.db";
    }
    return "/etc/com.ticklab.biopass/biopass.db";
  }
  return std::string(pw->pw_dir) + "/.config/com.ticklab.biopass/biopass.db";
}

std::optional<std::string> resolveModelPath(const std::string& username,
                                            const std::string& model_id) {
  if (model_id.empty()) {
    return std::nullopt;
  }

  const std::string db_path = getDbPath(username);

  sqlite3* db = nullptr;
  if (sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
    spdlog::warn("Biopass: Failed to open model registry at {}: {}", db_path,
                 db ? sqlite3_errmsg(db) : "unknown error");
    if (db)
      sqlite3_close(db);
    return std::nullopt;
  }
  sqlite3_busy_timeout(db, 2000);

  sqlite3_stmt* stmt = nullptr;
  static const char* kQuery = "SELECT path FROM models WHERE id = ?1";
  if (sqlite3_prepare_v2(db, kQuery, -1, &stmt, nullptr) != SQLITE_OK) {
    spdlog::warn("Biopass: Failed to prepare model lookup query: {}", sqlite3_errmsg(db));
    sqlite3_close(db);
    return std::nullopt;
  }
  sqlite3_bind_text(stmt, 1, model_id.c_str(), -1, SQLITE_TRANSIENT);

  std::optional<std::string> result = std::nullopt;
  int rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    const unsigned char* text = sqlite3_column_text(stmt, 0);
    if (text != nullptr) {
      result = std::string(reinterpret_cast<const char*>(text));
    }
  } else if (rc != SQLITE_DONE) {
    spdlog::warn("Biopass: Failed to query model '{}': {}", model_id, sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return result;
}

}  // namespace biopass
