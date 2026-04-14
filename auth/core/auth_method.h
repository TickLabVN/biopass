#pragma once

#include <security/_pam_types.h>

#include <string>

namespace biopass {
enum AuthResult { Success, Failure, Retry, Unavailable };

struct AuthConfig {
  bool debug = false;
  bool antispoof = false;
};

struct IAuthMethod {
  virtual ~IAuthMethod() = default;
  virtual std::string name() const = 0;
  virtual bool isAvailable() const = 0;
  virtual int getRetries() const = 0;
  virtual int getRetryDelayMs() const = 0;
  virtual AuthResult authenticate(const std::string& username, const AuthConfig& config,
                                  std::atomic<bool>* cancelSignal = nullptr) = 0;
};

struct RetryStrategy {
  RetryStrategy(int maxRetries) : maxRetries_(maxRetries) {}

  bool shouldRetry(AuthResult result, int attempts) const {
    if (result != AuthResult::Retry) {
      return false;
    }
    return attempts < maxRetries_;
  }

 private:
  int maxRetries_;
};
}  // namespace biopass
