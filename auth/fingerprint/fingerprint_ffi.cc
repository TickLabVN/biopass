#include "fingerprint_ffi.h"

#include <cstring>

#include "fingerprint_auth.h"

extern "C" {

void* fingerprint_auth_new(void) { return static_cast<void*>(new facepass::FingerprintAuth()); }

void fingerprint_auth_free(void* auth) {
  if (!auth)
    return;
  delete static_cast<facepass::FingerprintAuth*>(auth);
}

bool fingerprint_is_available(void* auth) {
  if (!auth)
    return false;
  return static_cast<facepass::FingerprintAuth*>(auth)->is_available();
}

int fingerprint_authenticate(void* auth, const char* username, FingerprintAuthConfig config) {
  if (!auth || !username)
    return AUTH_UNAVAILABLE;
  auto* fp_auth = static_cast<facepass::FingerprintAuth*>(auth);

  facepass::AuthConfig cpp_config;
  cpp_config.retries = config.retries;

  facepass::AuthResult result = fp_auth->authenticate(username, cpp_config);

  switch (result) {
    case facepass::AuthResult::Success:
      return AUTH_SUCCESS;
    case facepass::AuthResult::Failure:
      return AUTH_FAILURE;
    case facepass::AuthResult::Unavailable:
      return AUTH_UNAVAILABLE;
    case facepass::AuthResult::Retry:
      return AUTH_RETRY;
    default:
      return AUTH_UNAVAILABLE;
  }
}

char** fingerprint_list_enrolled_fingers(void* auth, const char* username, int* count) {
  if (!auth || !username || !count) {
    if (count)
      *count = 0;
    return nullptr;
  }

  auto* fp_auth = static_cast<facepass::FingerprintAuth*>(auth);
  std::vector<std::string> fingers = fp_auth->list_enrolled_fingers(username);

  if (fingers.empty()) {
    *count = 0;
    return nullptr;
  }

  // Allocate array of string pointers (including null terminator)
  char** result = new char*[fingers.size() + 1];

  // Copy each string
  for (size_t i = 0; i < fingers.size(); ++i) {
    result[i] = new char[fingers[i].length() + 1];
    std::strcpy(result[i], fingers[i].c_str());
  }

  // Null terminate
  result[fingers.size()] = nullptr;

  *count = fingers.size();
  return result;
}

void fingerprint_free_string_array(char** array, int count) {
  if (!array)
    return;

  // Free each string
  for (int i = 0; i < count; ++i) {
    delete[] array[i];
  }

  // Free array itself
  delete[] array;
}

bool fingerprint_enroll(void* auth, const char* username, const char* finger_name,
                        EnrollProgressCallback callback, void* user_data) {
  if (!auth || !username || !finger_name)
    return false;
  return static_cast<facepass::FingerprintAuth*>(auth)->enroll(username, finger_name, callback,
                                                               user_data);
}

bool fingerprint_remove_finger(void* auth, const char* username, const char* finger_name) {
  if (!auth || !username || !finger_name)
    return false;
  return static_cast<facepass::FingerprintAuth*>(auth)->remove_finger(username, finger_name);
}

}  // extern "C"
