#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <stdio.h>
#include "face_config.h"

const struct pam_conv conv = {
	misc_conv,
	NULL};

int main(int argc, char *argv[])
{
	pam_handle_t *pamh = NULL;
	auto user = getenv("USER");

	if(argc < 2)
	{
		fprintf(stderr, "Usage: %s <pam_command>\n", argv[0]);
		return 1;
	}
	char *command = argv[1];

	printf("Command: %s\n", command);

	int retval = pam_start(command, user, &conv, &pamh);

	// Are the credentials correct?
	if (retval == PAM_SUCCESS)
		retval = pam_authenticate(pamh, 0);

	// Can the account be used at this time?
	if (retval == PAM_SUCCESS)
	{
		printf("Account %s is valid.\n", user);
		retval = pam_acct_mgmt(pamh, 0);
	}

	// Did everything work?
	if (retval == PAM_SUCCESS)
		printf("User %s authenticated\n", user);
	else
		printf("User %s not authenticated\n", user);

	// close PAM (end session)
	if (pam_end(pamh, retval) != PAM_SUCCESS)
	{
		pamh = NULL;
		printf("check_user: failed to release authenticator for user %s\n", user);
		exit(1);
	}
	return 0;
}