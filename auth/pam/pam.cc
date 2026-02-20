#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Called by PAM when a user needs to be authenticated
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  (void)flags;  // Suppress unused parameter warning
  (void)argc;
  (void)argv;

  int retval;

  const char *service = nullptr;
  retval = pam_get_item(pamh, PAM_SERVICE, (const void **)&service);
  if (retval == PAM_SUCCESS && service != nullptr) {
    // Polkit/pkexec require explicit password prompts in most DEs.
    // We bypass Facepass for these services to prevent auth hangs.
    if (strcmp(service, "polkit-1") == 0 || strcmp(service, "pkexec") == 0) {
      return PAM_IGNORE;
    }
  }

  const char *pUsername;
  retval = pam_get_user(pamh, &pUsername, NULL);
  if (retval != PAM_SUCCESS) {
    return retval;
  }

  pid_t pid = fork();
  if (pid < 0) {
    // Fork failed
    return PAM_AUTH_ERR;
  } else if (pid == 0) {
    // Child process: execute the helper binary
    // The helper binary should be installed in a standard location.
    // We pass the username as the first argument.

    // We use a known absolute path to avoid PATH spoofing attacks.
    // In a real production environment, this path should be configurable,
    // or strictly defined at compile-time (e.g. /usr/local/bin/facepass-helper).
    execl("/usr/local/bin/facepass-helper", "facepass-helper", pUsername, NULL);

    // If execl returns, it failed
    perror("execl failed");
    exit(1);
  } else {
    // Parent process (PAM module inside sudo/login): wait for the child
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
      int exit_code = WEXITSTATUS(status);
      if (exit_code == 0) {
        return PAM_SUCCESS;
      } else if (exit_code == 2) {
        return PAM_IGNORE;  // The helper skipped authentication (e.g. no config)
      } else {
        return PAM_AUTH_ERR;
      }
    } else {
      // Child did not exit normally (e.g., killed by signal)
      return PAM_AUTH_ERR;
    }
  }
}

// The functions below are required by PAM, but not needed in this module
PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;
  return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;
  return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;
  return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;
  return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  (void)pamh;
  (void)flags;
  (void)argc;
  (void)argv;
  return PAM_IGNORE;
}