# PAM Setup Guide

1. Verify the Biopass PAM profile exists:
    ```bash
    ls /usr/share/pam-configs/biopass
    ```
2. Enable the Biopass PAM profile.
    - On Debian-based OS (Debian, Ubuntu, Pop!_OS, Mint ...):
        ```bash
        sudo pam-auth-update
        ```
    - On Fedora-based OS (Fedora, RHEL, CentOS, Rocky, Alma ...), use `authselect`, not `pam-auth-update`.
3. Enable `Biopass` option. If `Fingerprint authentication` is selected, please disable it if you have enabled fingerprint auth in Biopass.
4. Test in a new terminal:
    ```bash
    sudo -k
    sudo true
    ```