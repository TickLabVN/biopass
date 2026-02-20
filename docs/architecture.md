## Architecture

Biopass is split into two primary components to ensure clean separation between UI logic and security-critical modules:

1. **The Application (`app/`)**: A Tauri-based cross-platform application responsible for managing configurations, enrolling biometric data (faces, fingerprints, voices), and managing models.
2. **The Security Modules (`auth/`)**: A collection of C++ modules that handle the core logic and authentication.

### Modules in `auth/`

The `auth` directory is organized into the following modules:
- **`pam/`**: The Core PAM module that integrates with Linux authentication.
- **`face/`**: Contains sub-modules for face detection, recognition, and antispoofing.
- **`config/`**: Shared library for managing Biopass settings.
- **`cli/`**: Command-line interface for development and advanced operations.
- **`external/`**: Bundled dependencies like `libtorch`.
- **`voice/`**: (Coming soon) Voice recognition module.
- **`fingerprint/`**: (Coming soon) Fingerprint recognition module.

