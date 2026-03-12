# ---------------------------------------------------------------------------
#  Biopass – Root Build Orchestrator
#
#  Drives both sub-projects from the repository root:
#    auth/  → C++ CMake PAM module + face/fingerprint/voice libs
#    app/   → Tauri desktop application
#
#  Prerequisites
#    auth : cmake, ninja/make, libopencv-dev, libpam0g-dev, libcli11-dev
#    app  : bun, rustup/cargo, tauri-cli v2, webkit2gtk, libssl-dev …
# ---------------------------------------------------------------------------

.PHONY: all build build-auth build-app \
        package package-app package-aur \
        clean clean-auth clean-app \
        install-deps

# ── Configurable defaults ──────────────────────────────────────────────────
BUILD_TYPE   ?= Release
AUTH_BUILD   := auth/build
APP_DIR      := app
VERSION      ?= $(shell git describe --tags --always 2>/dev/null || echo "0.1.0")

# ── Top-level aliases ──────────────────────────────────────────────────────
all: build

build: build-auth build-app

package: package-app

package-aur:
	@echo "==> [aur] Building the AUR package from packaging/aur/biopass-bin/"
	cd packaging/aur/biopass-bin && makepkg -si

# ── auth (CMake / C++) ────────────────────────────────────────────────────
build-auth:
	@echo "==> [auth] Configuring CMake (BUILD_TYPE=$(BUILD_TYPE))…"
	cmake -S auth -B $(AUTH_BUILD) \
	      -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	      -DPROJECT_VERSION=$(VERSION)
	@echo "==> [auth] Building…"
	cmake --build $(AUTH_BUILD) --config $(BUILD_TYPE) --parallel


clean-auth:
	@echo "==> [auth] Cleaning build directory…"
	rm -rf $(AUTH_BUILD)

# ── app (Tauri / Bun / Rust) ──────────────────────────────────────────────
# build-app depends on build-auth so the auth .so files exist before
# Tauri packages them into the combined Linux bundles.
build-app: build-auth
	@echo "==> [app] Installing JS dependencies…"
	cd $(APP_DIR) && bun install --frozen-lockfile
	@echo "==> [app] Building Tauri application…"
	cd $(APP_DIR) && bun run tauri build

# package-app produces the combined Linux bundles (Tauri app + auth libs bundled inside)
package-app: build-app
	@echo "==> [app] Tauri packages are in app/src-tauri/target/release/bundle/"
	@ls app/src-tauri/target/release/bundle/deb/*.deb \
	     app/src-tauri/target/release/bundle/appimage/*.AppImage \
	     app/src-tauri/target/release/bundle/rpm/*.rpm 2>/dev/null || true

clean-app:
	@echo "==> [app] Cleaning build artifacts…"
	cd $(APP_DIR) && cargo clean --manifest-path src-tauri/Cargo.toml 2>/dev/null || true
	rm -rf $(APP_DIR)/dist

# ── Combined clean ────────────────────────────────────────────────────────
clean: clean-auth clean-app

# ── Help ──────────────────────────────────────────────────────────────────
help:
	@echo ""
	@echo "  Usage: make [target] [VAR=value …]"
	@echo ""
	@echo "  Targets:"
	@echo "    build          Build both auth and app (default)"
	@echo "    build-auth     Configure + build the C++ PAM module only"
	@echo "    build-app      Install JS deps + build the Tauri app only"
	@echo "    package        Build + package everything into combined Linux bundles"
	@echo "    package-app    Show Tauri combined bundle output paths"
	@echo "    package-aur    Build the Arch AUR package from packaging/aur/biopass-bin/"
	@echo "    clean          Remove all build output"
	@echo "    clean-auth     Remove auth/build/"
	@echo "    clean-app      Remove app build artifacts"
	@echo ""
	@echo "  Variables:"
	@echo "    BUILD_TYPE     CMake build type (default: Release)"
	@echo "    VERSION        Package version (default: git tag or 0.1.0)"
	@echo ""
