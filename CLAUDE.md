# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Biopass is a Linux biometric authentication suite (face + fingerprint) — an alternative to Howdy. It has two independent, differently-toolchained halves that are only connected through PAM and a shared config file:

- **`auth/`** — C++17 backend: a PAM module (`libbiopass_pam.so`) plus a `biopass-helper` binary that does the actual sensor capture and ONNX inference (YOLO-Face detection, EdgeFace recognition, MobileNetV3 anti-spoofing). Built with CMake.
- **`app/`** — Tauri v2 desktop app (Rust backend + React/TypeScript/Tailwind frontend) for registering biometrics and managing configuration. Built with Bun + Cargo.

They communicate one-way: the React UI writes to `~/.config/com.ticklab.biopass/config.yaml`, and the PAM module/helper reads it at login time. There is no other IPC between the two halves.

## Commands

### Building
```bash
make build-auth   # CMake-build the C++ auth module only
make build-app    # bun install + tauri build (also builds auth first, since .so files are bundled)
make build        # both
make package      # produce .deb/.rpm release artifacts
```
`auth/BundleLibcamera.cmake` builds a pinned libcamera from source during `build-auth` — this is intentional (libcamera's C++ ABI breaks across minor versions), don't try to link the system libcamera instead.

### Backend (`auth/`, C++/CMake)
```bash
cmake -S auth -B auth/build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build auth/build --parallel
```
- `-DBUILD_TESTS=ON` enables `auth/test/camera` and `auth/test/face_engine` (manual/interactive test binaries, not a unit test suite).
- Formatting: `clang-format -i <file>` on `*.cc`/`*.h` (enforced by lefthook pre-commit, config in root `.clang-format` if present).

### Frontend (`app/`, Bun/Tauri)
```bash
cd app
bun install
bun run tauri dev      # run the desktop app with HMR
bun run build           # tsc typecheck + vite build
bun run lint            # biome check .
bun run check           # biome check --write . (auto-fix)
```
There is no JS/TS test runner configured in `app/package.json` — don't invent test commands for it.

### Rust backend (`app/src-tauri/`)
```bash
cd app/src-tauri
cargo build
cargo fmt --all         # lefthook pre-commit runs `cargo fmt --all --check`
cargo test               # dev-dependency tempfile is used in existing tests
```

### Pre-commit
`lefthook.yml` runs biome (JS/TS), clang-format (C++), and `cargo fmt --check` (Rust) automatically per-directory on staged files. Never bypass these with `--no-verify`; fix the formatting/lint issue instead.

## Architecture notes

### Auth flow (`auth/`)
`OS login (PAM) → libbiopass_pam.so → spawns biopass-helper (isolated process) → captures from camera/fingerprint sensor → runs local ONNX inference → exit code → PAM_SUCCESS/PAM_AUTH_ERR`. See `auth/pam/pam.cc` (the PAM entrypoint) and `auth/pam/helper.cc` (the isolated capture+inference process) — the split into two binaries is deliberate, so keep that boundary when touching auth code.

- `auth/core/` — shared `auth_manager` / `auth_config` used by both face and fingerprint paths.
- `auth/face/` — `face_auth.cc` orchestrates detection (`face/detection`) → recognition (`face/recognition`) → anti-spoofing (`face/antispoofing`, either the ONNX MobileNetV3 model or `ir_camera_as.cc` for IR-camera-based liveness). Shared camera/ONNX/pixel utilities live in `face/common`.
- `auth/fingerprint/` — `fingerprint_auth.cc` plus `fingerprint_ffi.cc`, the FFI boundary consumed from the Rust side (`app/src-tauri/src/fingerprint_ffi.rs`).
- ONNX models are checked in under `auth/face/models/*.onnx`.

### Desktop app flow (`app/`)
`React UI → Tauri IPC commands (app/src-tauri/src/*.rs) → writes config.yaml / invokes capture for enrollment`. Each Rust source file under `src-tauri/src/` maps to one concern: `config.rs` (read/write config.yaml), `face.rs`/`face_session.rs` (face enrollment session state), `fingerprint.rs`/`fingerprint_ffi.rs` (bridges to the C++ FFI), `paths.rs`, `system.rs`.

### Config coupling
Both halves read/write the same `~/.config/com.ticklab.biopass/config.yaml`. When changing its schema, update both the Rust side (`app/src-tauri/src/config.rs`) and the C++ side (`auth/core/auth_config.cc`/`.h`) together - they are not generated from a shared schema.

### Debugging the PAM/auth path
Enable Debug Mode in the UI (or hand-edit `debug` in `config.yaml`) to get verbose logs and to have failed/spoofed face captures saved as `.bmp` under `~/.local/share/com.ticklab.biopass/debugs/`.

**System lockout risk**: `auth/pam` changes touch the live PAM stack (`/etc/pam.d/common-auth` or `/etc/pam.d/system-auth`). Never suggest editing distro PAM include files directly without the caveats in `docs/PAM.md` — a broken PAM module can lock the user out of their machine entirely. Prefer testing in a VM with snapshots, per `CONTRIBUTING.md`.

## Contribution norms (from CONTRIBUTING.md)

- PR/issue descriptions must be written in the contributor's own words, not AI-generated boilerplate — don't draft PR descriptions that read as generic AI text.
- Changes to authentication, installation, packaging, PAM, or application flow, or new dependencies, are expected to have a discussion/issue first — flag this to the user rather than assuming a large change here is pre-approved.

## Working with Claude Code on this repo

For non-trivial tasks (new features, cross-cutting changes, anything touching `auth/pam` or the config schema), route work across models instead of doing everything in the main thread:

- **Plan**: spawn the `Plan` subagent with `model: "fable"` (fallback `"opus"`) to draft the implementation plan. Present it back to the user for manual approval (native Plan Mode / `ExitPlanMode` is the preferred vehicle for this, since it gives an explicit accept/reject step) before editing anything.
- **Execute**: once approved, implement directly in the main thread on whatever model is currently selected (`/model` — normally Sonnet). Don't re-delegate execution to a subagent.
- **Search**: for locating files, symbols, or patterns across the polyglot tree (C++/CMake, Rust, TS), spawn the `Explore` subagent with `model: "haiku"` instead of grepping inline — it's cheaper and this repo's structure (three toolchains) makes broad search common.

There is no setting that auto-switches the main thread's model when entering/exiting Plan Mode — `/model` only affects the thread you run it in. This routing is achieved entirely through the `Agent` tool's per-call `model` override, so no changes to `.claude/settings.json` are needed for it.
