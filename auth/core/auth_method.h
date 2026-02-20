#pragma once

#include <security/_pam_types.h>

#include <string>

namespace facepass {
/**
 * Result of an authentication attempt.
 */
enum AuthResult {
  Success,     // Authentication succeeded
  Failure,     // Authentication failed
  Retry,       // Should retry (transient error)
  Unavailable  // Method not available (e.g., no camera)
};

/**
 * Configuration for authentication methods.
 */
struct AuthConfig {
  bool debug = false;
  bool anti_spoof = false;
};

/**
 * Interface for authentication methods.
 * All auth methods (face, voice, fingerprint) must implement this interface.
 */
struct IAuthMethod {
  virtual ~IAuthMethod() = default;

  /**
   * Human-readable name for logging.
   */
  virtual std::string name() const = 0;

  /**
   * Check if this method is available on the system.
   * For example, returns false if no camera is available for face auth.
   */
  virtual bool is_available() const = 0;

  /**
   * Get the maximum number of retries for this method.
   */
  virtual int get_retries() const = 0;

  /**
   * Get the delay in milliseconds between retries for this method.
   */
  virtual int get_retry_delay_ms() const = 0;

  /**
   * Perform authentication for the given user.
   * @param username The PAM username to authenticate.
   * @param config Configuration options (retries, delays, etc.)
   * @return AuthResult indicating success, failure, or other states.
   */
  virtual AuthResult authenticate(const std::string& username, const AuthConfig& config) = 0;
};

}  // namespace facepass
