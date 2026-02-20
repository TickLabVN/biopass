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
        package package-auth package-app \
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

# ── auth (CMake / C++) ────────────────────────────────────────────────────
build-auth:
	@echo "==> [auth] Configuring CMake (BUILD_TYPE=$(BUILD_TYPE))…"
	cmake -S auth -B $(AUTH_BUILD) \
	      -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	      -DPROJECT_VERSION=$(VERSION)
	@echo "==> [auth] Building…"
	cmake --build $(AUTH_BUILD) --config $(BUILD_TYPE) --parallel

package-auth: build-auth
	@echo "==> [auth] Packaging with CPack…"
	cd $(AUTH_BUILD) && cpack
	@echo "==> [auth] Package ready in $(AUTH_BUILD)/"

clean-auth:
	@echo "==> [auth] Cleaning build directory…"
	rm -rf $(AUTH_BUILD)

# ── app (Tauri / Bun / Rust) ──────────────────────────────────────────────
# build-app depends on build-auth so the auth .so files exist before
# Tauri packages them into the single combined .deb.
build-app: build-auth
	@echo "==> [app] Installing JS dependencies…"
	cd $(APP_DIR) && bun install --frozen-lockfile
	@echo "==> [app] Building Tauri application…"
	cd $(APP_DIR) && bun run tauri build

# package-app produces the single combined .deb (Tauri app + auth libs bundled inside)
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
	@echo "    package        Build + package everything into one combined .deb"
	@echo "    package-app    Show Tauri combined bundle output paths"
	@echo "    clean          Remove all build output"
	@echo "    clean-auth     Remove auth/build/"
	@echo "    clean-app      Remove app build artifacts"
	@echo ""
	@echo "  Variables:"
	@echo "    BUILD_TYPE     CMake build type (default: Release)"
	@echo "    VERSION        Package version (default: git tag or 0.1.0)"
	@echo ""
