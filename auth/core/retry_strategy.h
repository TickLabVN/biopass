#pragma once

#include "auth_method.h"

namespace biopass {

/**
 * Strategy that retries a fixed number of times if the result is AuthResult::Retry.
 */
struct RetryStrategy {
  RetryStrategy(int max_retries) : max_retries_(max_retries) {}

  bool should_retry(AuthResult result, int attempts) const {
    // Only retry if the method explicitly requests it
    if (result != AuthResult::Retry) {
      return false;
    }
    // Check if we have retries left
    return attempts < max_retries_;
  }

 private:
  int max_retries_;
};

}  // namespace biopass
