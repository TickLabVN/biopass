# AUR packaging

This repository ships an AUR recipe under `packaging/aur/biopass-bin/`.

It is a `-bin` package built from the upstream `.deb` release instead of a
full source build. That is the pragmatic path today because the native CMake
build still fetches third-party dependencies during configure, which is not a
good fit for `makepkg`.

## Build locally

```bash
cd packaging/aur/biopass-bin
makepkg -si
```

## Current behavior

- Extracts the official `biopass_${pkgver}_amd64.deb` release artifact.
- Relocates `biopass-helper` from `/usr/local/bin` to `/usr/bin`.
- Patches the PAM module so it executes `/usr/bin/biopass-helper`.
- Leaves model downloads as a manual post-install step instead of writing into a
  user home directory from a pacman hook.

## Updating for a new release

1. Update `pkgver` and the `.deb` checksum in `PKGBUILD`.
2. Regenerate `.SRCINFO` with `makepkg --printsrcinfo > .SRCINFO`.
3. Rebuild with `makepkg -f`.
