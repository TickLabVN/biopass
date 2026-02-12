#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * FFI Interface for FingerprintAuth
 * This provides C-compatible functions for calling from other languages like Rust
 */

typedef enum {
  AUTH_SUCCESS = 0,
  AUTH_FAILURE = 1,
  AUTH_UNAVAILABLE = 2,
  AUTH_RETRY = 3,
} FingerprintAuthResult;

typedef struct {
  int retries;
} FingerprintAuthConfig;

/**
 * Initialize fingerprint auth instance
 * Returns opaque pointer to FingerprintAuth instance
 */
void* fingerprint_auth_new(void);

/**
 * Free fingerprint auth instance
 */
void fingerprint_auth_free(void* auth);

/**
 * Check if fingerprint hardware is available
 */
bool fingerprint_is_available(void* auth);

/**
 * Authenticate user with fingerprint
 * Returns FingerprintAuthResult
 */
int fingerprint_authenticate(void* auth, const char* username, FingerprintAuthConfig config);

/**
 * List enrolled fingers for a user
 * Returns NULL-terminated array of strings
 * Caller must free with fingerprint_free_string_array
 */
char** fingerprint_list_enrolled_fingers(void* auth, const char* username, int* count);

/**
 * Free string array returned by list_enrolled_fingers
 */
void fingerprint_free_string_array(char** array, int count);

/**
 * Enroll a new fingerprint
 * Returns true on success, false on failure
 */
bool fingerprint_enroll(void* auth, const char* username, const char* finger_name);

/**
 * Remove an enrolled fingerprint
 * Returns true on success, false on failure
 */
bool fingerprint_remove_finger(void* auth, const char* username, const char* finger_name);

#ifdef __cplusplus
}
#endif
