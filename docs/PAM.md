# PAM Manual Setup Guide (Linux)

This guide explains how to integrate Biopass with PAM manually across Linux distributions.

## Why Manual Setup

PAM stacks vary by distro, release, desktop manager, and local hardening tools (for example `authselect` on Fedora-family systems).  
Currently, Biopass only provides in-app "System Sign-in Integration" for **Ubuntu** and **Pop!_OS**.  
For other distros, use this manual guide.

## Safety First (Important)

Incorrect PAM edits can lock you out of your system.

Before editing anything:

1. Keep an existing root shell open (`sudo -s`) until testing is complete.
2. Work in a VM/snapshot when possible.
3. Always create backups of PAM files before edits.

## 1. Verify Biopass PAM Module Exists

Biopass installs `libbiopass_pam.so`. Common locations:

- `/lib/security/libbiopass_pam.so`
- `/lib64/security/libbiopass_pam.so`
- `/usr/lib/security/libbiopass_pam.so`

Quick check:

```bash
for d in /lib/security /lib64/security /usr/lib/security; do
  [ -f "$d/libbiopass_pam.so" ] && echo "Found: $d/libbiopass_pam.so"
done
```

If no path is found, install/reinstall Biopass package first.

## 2. Choose the Correct PAM File

Typical include files by distro family:

- Debian/Ubuntu/Pop!_OS/Linux Mint: `/etc/pam.d/common-auth`
- Fedora/RHEL/CentOS/Rocky/Alma: `/etc/pam.d/system-auth` (and sometimes `/etc/pam.d/password-auth`)
- Arch/Manjaro/EndeavourOS: `/etc/pam.d/system-auth`
- openSUSE/SLES: usually `common-auth*` include stacks under `/etc/pam.d/`

## 3. Backup PAM File

Example (replace with your chosen file):

```bash
PAM_FILE=/etc/pam.d/common-auth
sudo cp "$PAM_FILE" "${PAM_FILE}.bak.$(date +%Y%m%d%H%M%S)"
```

## 4. Add Biopass Module Line

Use this in your `auth` stack, placed before the first `pam_unix.so` line:

```pam
auth    [success=2 default=ignore]      libbiopass_pam.so
auth    [success=1 default=ignore]      pam_unix.so nullok
# fallback if no previous auth module succeeded
auth    requisite                       pam_deny.so
```

- Keep the control flags exactly as shown.
- Do not remove existing `pam_unix.so`, `pam_deny.so`, or other mandatory modules.
- Please remove `pam_fprintd.so` if it conflict with `libbiopass_pam.so`
- `[success=<n> default=ignore]` may not work on some distros, like Fedora. In this case, use `sufficient` flag instead.

### What `[success=2 default=ignore]` means

- `success=2`: if Biopass succeeds, PAM skips the next 2 `auth` lines.
  - In this example, it skips `pam_unix.so` and `pam_deny.so`.
  - Result: login succeeds without password fallback.
- `default=ignore`: if Biopass does not succeed, PAM continues normally to the next line.
  - Result: `pam_unix.so` still runs as fallback.

### End-to-end flow

1. Biopass succeeds -> skip password + deny -> authentication succeeds.
2. Biopass fails/ignores -> try `pam_unix.so` (password fallback).
3. Password succeeds -> `success=1` skips `pam_deny.so` -> authentication succeeds.
4. Both fail -> `pam_deny.so` is reached -> authentication is denied.

Notes:

## 5. Distro-Specific Notes

### Ubuntu / Debian / Pop!_OS / Mint

- Preferred file: `/etc/pam.d/common-auth`
- Most services include this file, so one change is often enough.

### Fedora / RHEL / CentOS / Rocky / Alma

- Preferred file: `/etc/pam.d/system-auth`
- Some services use `/etc/pam.d/password-auth` too.
- If your system uses `authselect`, direct edits can be overwritten.  
  Create/activate a custom authselect profile and apply PAM changes there.

### Arch / Manjaro / EndeavourOS

- Preferred file: `/etc/pam.d/system-auth`
- Ensure ordering stays consistent with existing `pam_unix.so` and `pam_deny.so` rules.

### openSUSE / SLES

- PAM stacks may route through `common-auth`, `common-auth-pc`, or service-specific includes.
- Trace includes from your target services before editing.

## 6. Test Safely

1. Keep your current root shell open.
2. Open a new terminal and test:
```bash
sudo -k
sudo true
```
3. Test desktop unlock/sign-in only after terminal auth succeeds.
4. If any failure occurs, rollback immediately.

## 7. Rollback

Either remove the Biopass line manually, or restore backup:

```bash
sudo cp /etc/pam.d/common-auth.bak.YYYYMMDDHHMMSS /etc/pam.d/common-auth
```

(Adjust path to your actual PAM file and backup name.)

## 8. Recovery (If Locked Out)

- Boot with rescue/live media.
- Mount root filesystem.
- Restore PAM backup or remove `libbiopass_pam.so` line from affected PAM file(s).
- Reboot.

---

If you contribute distro-specific tested instructions, please open a PR to improve this guide.
