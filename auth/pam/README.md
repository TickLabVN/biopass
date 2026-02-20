# PAM module for FaceID login

Reference: https://www.redhat.com/en/blog/pluggable-authentication-modules-pam.

Linux Pluggable Authentication Modules (PAM) is a suite of libraries that allow a Linux system administrator to configure methods to authenticate users. We can use PAM to create a FaceID login method for Linux.

Sample login method using PAM: `sshd`, `sudo`, `ftpd`, or any cloud login methods.

## Dev guide

After compile `libbiopass_pam.so`, copy to `/lib/security/`:

```bash
sudo mkdir -p /lib/security
sudo cp ./build/auth/libbiopass_pam.so /lib/security/
```

Add the following line on the top of file `/etc/pam.d/common-auth`:

```bash
auth    required libbiopass_pam.so
account required libbiopass_pam.so
```

> WARN: This action may crash sudo, make sure your user has permission to edit this file without root permission first.