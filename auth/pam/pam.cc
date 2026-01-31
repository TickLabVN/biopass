#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include "identify.h"

// Called by PAM when a user needs to be authenticated, for example by running
// the sudo command
PAM_EXTERN int pam_sm_authenticate(
	pam_handle_t *pamh,
	int flags,
	int argc,
	const char **argv)
{
	int retval;
	const char *pUsername;
	retval = pam_get_user(pamh, &pUsername, NULL);

	int8_t num_retries = 10;
	int retry_delay = 200;
	bool anti_spoof = false;
	for (int i = 0; i < argc; ++i) {
		if (strncmp(argv[i], "retries=", 8) == 0) {
			num_retries = atoi(argv[i] + 8);
		} else if (strncmp(argv[i], "retry_delay=", 12) == 0) {
			retry_delay = atoi(argv[i] + 12);
		} else if (strcmp(argv[i], "anti_spoof") == 0) {
			anti_spoof = true;
		}
	}
	if (retval != PAM_SUCCESS)
		return retval;

	retval = scan_face(pUsername, num_retries, retry_delay, anti_spoof);
	if (retval == PAM_SUCCESS)
		printf("Face recognized! Welcome %s\n", pUsername);
	else
		printf("Face not recognized\n");
	return retval;
}

// The functions below are required by PAM, but not needed in this module
PAM_EXTERN int pam_sm_open_session(
	pam_handle_t *pamh,
	int flags,
	int argc,
	const char **argv)
{
	return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc,
								const char **argv)
{
	return PAM_IGNORE;
}
PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc,
									const char **argv)
{
	return PAM_IGNORE;
}
PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc,
								const char **argv)
{
	return PAM_IGNORE;
}
PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc,
							  const char **argv)
{
	return PAM_IGNORE;
}