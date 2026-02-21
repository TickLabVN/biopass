#include "auth_manager.h"

#include <spdlog/spdlog.h>

#include <atomic>
#include <iostream>

#include "retry_strategy.h"

namespace biopass {

void AuthManager::add_method(std::unique_ptr<IAuthMethod> method) {
  this->methods_.push_back(std::move(method));
}

void AuthManager::set_mode(ExecutionMode mode) { this->mode_ = mode; }

void AuthManager::set_config(const AuthConfig &config) { this->config_ = config; }

int AuthManager::authenticate(const std::string &username) {
  if (this->methods_.empty()) {
    spdlog::error("AuthManager: No authentication methods configured");
    return PAM_AUTH_ERR;
  }

  // Set default logger level according to debug config
  if (this->config_.debug) {
    spdlog::set_level(spdlog::level::debug);
  } else {
    spdlog::set_level(spdlog::level::info);
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
  bool any_attempted = false;

  for (auto &method : this->methods_) {
    if (!method->is_available()) {
      spdlog::debug("AuthManager: {} is not available, skipping", method->name());
      continue;
    }

    RetryStrategy retry_strategy(method->get_retries());
    int attempts = 0;
    AuthResult result;

    do {
      if (attempts > 0) {
        spdlog::debug("AuthManager: Retrying {} (attempt {}/{})", method->name(), attempts + 1,
                      method->get_retries());
        std::this_thread::sleep_for(std::chrono::milliseconds(method->get_retry_delay_ms()));
      } else {
        spdlog::debug("AuthManager: Trying {} authentication", method->name());
      }

      result = method->authenticate(username, this->config_);
      attempts++;

    } while (retry_strategy.should_retry(result, attempts));

    switch (result) {
      case AuthResult::Success:
        spdlog::debug("AuthManager: {} authentication succeeded", method->name());
        return PAM_SUCCESS;
      case AuthResult::Unavailable:
        spdlog::debug("AuthManager: {} became unavailable, skipping", method->name());
        break;
      case AuthResult::Failure:
        any_attempted = true;
        spdlog::debug("AuthManager: {} authentication failed, trying next", method->name());
        break;
      case AuthResult::Retry:
        any_attempted = true;
        spdlog::debug("AuthManager: {} requested retry but max retries exceeded", method->name());
        break;
    }
  }

  if (!any_attempted) {
    spdlog::debug("AuthManager: No methods were able to run for this user, skipping module");
    return PAM_IGNORE;
  }

  spdlog::error("AuthManager: All authentication methods failed");
  return PAM_AUTH_ERR;
}

int AuthManager::run_parallel(const std::string &username) {
  std::atomic<bool> success_found{false};
  std::vector<std::future<AuthResult>> futures;

  // Launch all methods in parallel
  for (auto &method : this->methods_) {
    if (!method->is_available()) {
      spdlog::debug("AuthManager: {} is not available, skipping", method->name());
      continue;
    }

    // Capture by reference - methods_ lifetime is guaranteed during
    // authenticate()
    futures.push_back(std::async(
        std::launch::async, [&method, &username, &config = this->config_, &success_found]() {
          RetryStrategy retry_strategy(method->get_retries());
          int attempts = 0;
          AuthResult result;

          do {
            // Early exit if another method already succeeded
            if (success_found.load()) {
              return AuthResult::Failure;
            }

            if (attempts > 0) {
              spdlog::debug("AuthManager: Retrying {} (parallel attempt {})", method->name(),
                            attempts + 1);
              std::this_thread::sleep_for(std::chrono::milliseconds(method->get_retry_delay_ms()));
            } else {
              spdlog::debug("AuthManager: Starting {} authentication (parallel)", method->name());
            }

            result = method->authenticate(username, config, &success_found);
            attempts++;
          } while (retry_strategy.should_retry(result, attempts) && !success_found.load());

          if (result == AuthResult::Success) {
            success_found.store(true);
            spdlog::debug("AuthManager: {} authentication succeeded (parallel)", method->name());
          }

          return result;
        }));
  }

  // If no method was launched at all, skip this module
  if (futures.empty()) {
    spdlog::debug("AuthManager: No methods were able to run for this user, skipping module");
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
    spdlog::debug("AuthManager: All parallel methods became unavailable, skipping module");
    return PAM_IGNORE;
  }

  spdlog::error("AuthManager: All parallel authentication methods failed");
  return PAM_AUTH_ERR;
}

}  // namespace biopass
