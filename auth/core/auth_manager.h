#pragma once

#include "auth_method.h"
#include <future>
#include <memory>
#include <thread>
#include <vector>

namespace facepass {

/**
 * Execution mode for authentication methods.
 */
enum class ExecutionMode {
  Sequential, // Try methods in order, fallback on failure
  Parallel    // Run all methods concurrently, succeed on first success
};

/**
 * Manages multiple authentication methods and their execution.
 */
class AuthManager {
public:
  AuthManager() = default;
  ~AuthManager() = default;

  // Non-copyable, movable
  AuthManager(const AuthManager &) = delete;
  AuthManager &operator=(const AuthManager &) = delete;
  AuthManager(AuthManager &&) = default;
  AuthManager &operator=(AuthManager &&) = default;

  /**
   * Add an authentication method to the manager.
   * Methods are tried in the order they are added (for sequential mode).
   */
  void add_method(std::unique_ptr<IAuthMethod> method);

  /**
   * Set the execution mode.
   */
  void set_mode(ExecutionMode mode);

  /**
   * Set the configuration for all methods.
   */
  void set_config(const AuthConfig &config);

  /**
   * Authenticate the user using the configured methods and mode.
   * @param username The PAM username to authenticate.
   * @return PAM_SUCCESS on success, PAM_AUTH_ERR on failure.
   */
  int authenticate(const std::string &username);

private:
  /**
   * Run methods sequentially with fallback.
   */
  int run_sequential(const std::string &username);

  /**
   * Run methods in parallel, return on first success.
   */
  int run_parallel(const std::string &username);

  std::vector<std::unique_ptr<IAuthMethod>> methods_;
  ExecutionMode mode_ = ExecutionMode::Sequential;
  AuthConfig config_;
};

} // namespace facepass
