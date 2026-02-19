#include "auth_manager.h"

#include <atomic>
#include <iostream>

#include "retry_strategy.h"

namespace facepass {

void AuthManager::add_method(std::unique_ptr<IAuthMethod> method) {
  this->methods_.push_back(std::move(method));
}

void AuthManager::set_mode(ExecutionMode mode) { this->mode_ = mode; }

void AuthManager::set_config(const AuthConfig &config) { this->config_ = config; }

int AuthManager::authenticate(const std::string &username) {
  if (this->methods_.empty()) {
    std::cerr << "AuthManager: No authentication methods configured" << std::endl;
    return PAM_AUTH_ERR;
  }

  switch (this->mode_) {
    case ExecutionMode::Sequential:
      return this->run_sequential(username);
    case ExecutionMode::Parallel:
      return this->run_parallel(username);
    default:
      return PAM_AUTH_ERR;
  }
}

int AuthManager::run_sequential(const std::string &username) {
  RetryStrategy retry_strategy(this->config_.retries);
  bool any_attempted = false;

  for (auto &method : this->methods_) {
    if (!method->is_available()) {
      std::cerr << "AuthManager: " << method->name() << " is not available, skipping" << std::endl;
      continue;
    }

    int attempts = 0;
    AuthResult result;

    do {
      if (attempts > 0) {
        std::cerr << "AuthManager: Retrying " << method->name() << " (attempt " << attempts + 1
                  << "/" << this->config_.retries << ")" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(this->config_.retry_delay_ms));
      } else {
        std::cout << "AuthManager: Trying " << method->name() << " authentication" << std::endl;
      }

      result = method->authenticate(username, this->config_);
      attempts++;

    } while (retry_strategy.should_retry(result, attempts));

    switch (result) {
      case AuthResult::Success:
        std::cout << "AuthManager: " << method->name() << " authentication succeeded" << std::endl;
        return PAM_SUCCESS;
      case AuthResult::Unavailable:
        std::cerr << "AuthManager: " << method->name() << " became unavailable, skipping"
                  << std::endl;
        break;
      case AuthResult::Failure:
        any_attempted = true;
        std::cerr << "AuthManager: " << method->name() << " authentication failed, trying next"
                  << std::endl;
        break;
      case AuthResult::Retry:
        any_attempted = true;
        std::cerr << "AuthManager: " << method->name()
                  << " requested retry but max retries exceeded" << std::endl;
        break;
    }
  }

  if (!any_attempted) {
    std::cerr << "AuthManager: No methods were able to run for this user, skipping module"
              << std::endl;
    return PAM_IGNORE;
  }

  std::cerr << "AuthManager: All authentication methods failed" << std::endl;
  return PAM_AUTH_ERR;
}

int AuthManager::run_parallel(const std::string &username) {
  std::atomic<bool> success_found{false};
  std::vector<std::future<AuthResult>> futures;

  // Launch all methods in parallel
  for (auto &method : this->methods_) {
    if (!method->is_available()) {
      std::cerr << "AuthManager: " << method->name() << " is not available, skipping" << std::endl;
      continue;
    }

    // Capture by reference - methods_ lifetime is guaranteed during
    // authenticate()
    futures.push_back(std::async(
        std::launch::async, [&method, &username, &config = this->config_, &success_found]() {
          RetryStrategy retry_strategy(config.retries);
          int attempts = 0;
          AuthResult result;

          do {
            // Early exit if another method already succeeded
            if (success_found.load()) {
              return AuthResult::Failure;
            }

            if (attempts > 0) {
              std::cout << "AuthManager: Retrying " << method->name() << " (parallel attempt "
                        << attempts + 1 << ")" << std::endl;
              std::this_thread::sleep_for(std::chrono::milliseconds(config.retry_delay_ms));
            } else {
              std::cout << "AuthManager: Starting " << method->name()
                        << " authentication (parallel)" << std::endl;
            }

            result = method->authenticate(username, config);
            attempts++;
          } while (retry_strategy.should_retry(result, attempts) && !success_found.load());

          if (result == AuthResult::Success) {
            success_found.store(true);
            std::cout << "AuthManager: " << method->name() << " authentication succeeded (parallel)"
                      << std::endl;
          }

          return result;
        }));
  }

  // If no method was launched at all, skip this module
  if (futures.empty()) {
    std::cerr << "AuthManager: No methods were able to run for this user, skipping module"
              << std::endl;
    return PAM_IGNORE;
  }

  // Wait for all futures and check results
  bool any_success = false;
  bool any_attempted = false;
  for (auto &future : futures) {
    AuthResult result = future.get();
    if (result == AuthResult::Success) {
      any_success = true;
    } else if (result != AuthResult::Unavailable) {
      any_attempted = true;
    }
  }

  if (any_success) {
    return PAM_SUCCESS;
  }

  if (!any_attempted) {
    std::cerr << "AuthManager: All parallel methods became unavailable, skipping module"
              << std::endl;
    return PAM_IGNORE;
  }

  std::cerr << "AuthManager: All parallel authentication methods failed" << std::endl;
  return PAM_AUTH_ERR;
}

}  // namespace facepass
