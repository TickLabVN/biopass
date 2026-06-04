# Polkit Integration Guide

In some desktop environments, such as GNOME and KDE, when a user edits some important settings, they are required their password or fingerprint through an interactive dialog.

![Interactive Auth](./images/interactive-auth.png)

This flow is handled by [Polkit](https://github.com/polkit-org/polkit). In some cases, Biopass's authentication is not triggered by Polkit due to the strict policy on device accesses.

Here are steps to fix the issue:

1. Create a systemd override for `polkit-agent-helper`
    ```bash
    sudo systemctl edit 'polkit-agent-helper@.service'
    ```
2. Add this override, then save:
    ```ini
    [Service]
    PrivateDevices=no
    DevicePolicy=auto

    DeviceAllow=char-video4linux rw
    DeviceAllow=char-media rw
    DeviceAllow=char-drm rw
    DeviceAllow=/dev/uinput rw

    ProtectHome=read-only
    ```
3. Reload systemd and restart polkit
    ```bash
    sudo systemctl daemon-reload
    sudo systemctl restart polkit.service
    ```
4. Check the configuration in Biopass's UI. Please make sure to remove `pkexec`, `polkit-1` from the ignore service list ![alt text](images/ignore-services.png)
4. Finally, run `pkexec id` to check if the camera opens. If the override is working, the `polkit` authentication window will not open.

