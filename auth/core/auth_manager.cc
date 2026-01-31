#include "auth_manager.h"
#include <atomic>
#include <iostream>

namespace facepass {

void AuthManager::add_method(std::unique_ptr<IAuthMethod> method) {
  methods_.push_back(std::move(method));
}

void AuthManager::set_mode(ExecutionMode mode) { mode_ = mode; }

void AuthManager::set_config(const AuthConfig &config) { config_ = config; }

int AuthManager::authenticate(const std::string &username) {
  if (methods_.empty()) {
    std::cerr << "AuthManager: No authentication methods configured"
              << std::endl;
    return PAM_AUTH_ERR;
  }

  switch (mode_) {
  case ExecutionMode::Sequential:
    return run_sequential(username);
  case ExecutionMode::Parallel:
    return run_parallel(username);
  default:
    return PAM_AUTH_ERR;
  }
}

int AuthManager::run_sequential(const std::string &username) {
  for (auto &method : methods_) {
    if (!method->is_available()) {
      std::cerr << "AuthManager: " << method->name()
                << " is not available, skipping" << std::endl;
      continue;
    }

    std::cout << "AuthManager: Trying " << method->name() << " authentication"
              << std::endl;
    AuthResult result = method->authenticate(username, config_);

    switch (result) {
    case AuthResult::Success:
      std::cout << "AuthManager: " << method->name()
                << " authentication succeeded" << std::endl;
      return PAM_SUCCESS;
    case AuthResult::Failure:
      std::cerr << "AuthManager: " << method->name()
                << " authentication failed, trying next" << std::endl;
      break;
    case AuthResult::Retry:
      std::cerr << "AuthManager: " << method->name()
                << " requested retry, trying next" << std::endl;
      break;
    case AuthResult::Unavailable:
      std::cerr << "AuthManager: " << method->name() << " became unavailable"
                << std::endl;
      break;
    }
  }

  std::cerr << "AuthManager: All authentication methods failed" << std::endl;
  return PAM_AUTH_ERR;
}

int AuthManager::run_parallel(const std::string &username) {
  std::atomic<bool> success_found{false};
  std::vector<std::future<AuthResult>> futures;

  // Launch all methods in parallel
  for (auto &method : methods_) {
    if (!method->is_available()) {
      std::cerr << "AuthManager: " << method->name()
                << " is not available, skipping" << std::endl;
      continue;
    }

    // Capture by reference - methods_ lifetime is guaranteed during
    // authenticate()
    futures.push_back(
        std::async(std::launch::async, [&method, &username, &config = config_,
                                        &success_found]() {
          // Early exit if another method already succeeded
          if (success_found.load()) {
            return AuthResult::Failure;
          }

          std::cout << "AuthManager: Starting " << method->name()
                    << " authentication (parallel)" << std::endl;
          AuthResult result = method->authenticate(username, config);

          if (result == AuthResult::Success) {
            success_found.store(true);
            std::cout << "AuthManager: " << method->name()
                      << " authentication succeeded (parallel)" << std::endl;
          }

          return result;
        }));
  }

  // Wait for all futures and check results
  bool any_success = false;
  for (auto &future : futures) {
    AuthResult result = future.get();
    if (result == AuthResult::Success) {
      any_success = true;
    }
  }

  if (any_success) {
    return PAM_SUCCESS;
  }

  std::cerr << "AuthManager: All parallel authentication methods failed"
            << std::endl;
  return PAM_AUTH_ERR;
}

} // namespace facepass
