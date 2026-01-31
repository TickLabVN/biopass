#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth_config.h"
#include "auth_manager.h"
#include "face_auth.h"
#include "fingerprint_auth.h"
#include "voice_auth.h"

// Called by PAM when a user needs to be authenticated
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc,
                                   const char **argv) {
  (void)flags; // Suppress unused parameter warning
  (void)argc;
  (void)argv;

  int retval;
  const char *pUsername;
  retval = pam_get_user(pamh, &pUsername, NULL);
  if (retval != PAM_SUCCESS) {
    return retval;
  }

  // Load configuration from file
  facepass::FacePassConfig config = facepass::load_config(pUsername);

  // Create and configure AuthManager
  facepass::AuthManager manager;
  manager.set_mode(config.mode);
  manager.set_config(config.auth);

  // Add requested authentication methods
  for (const auto &method_name : config.methods) {
    if (method_name == "face") {
      manager.add_method(std::make_unique<facepass::FaceAuth>());
    } else if (method_name == "voice") {
      manager.add_method(std::make_unique<facepass::VoiceAuth>());
    } else if (method_name == "fingerprint") {
      manager.add_method(std::make_unique<facepass::FingerprintAuth>());
    }
  }

  // Authenticate
  retval = manager.authenticate(pUsername);

  if (retval == PAM_SUCCESS) {
    printf("Authentication succeeded! Welcome %s\n", pUsername);
  } else {
    printf("Authentication failed\n");
  }

  return retval;
}

// The functions below are required by PAM, but not needed in this module
PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc,
                                   const char **argv) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;
  return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc,
                                const char **argv) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;
  return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc,
                                    const char **argv) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;
  return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc,
                                const char **argv) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;
  return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc,
                              const char **argv) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;
  return PAM_IGNORE;
}