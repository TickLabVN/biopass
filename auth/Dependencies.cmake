# ==============================================================================
# Dependency Management
# ==============================================================================

# ONNX Runtime
set(ONNXRUNTIME_VERSION "1.19.2")
include(FetchContent)
FetchContent_Declare(
    onnxruntime
    URL https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(onnxruntime)

set(ONNXRUNTIME_ROOT "${onnxruntime_SOURCE_DIR}")
set(ONNXRUNTIME_INCLUDE_DIRS "${ONNXRUNTIME_ROOT}/include")
set(ONNXRUNTIME_LIB_DIR "${ONNXRUNTIME_ROOT}/lib")
find_library(ONNXRUNTIME_LIB onnxruntime PATHS ${ONNXRUNTIME_LIB_DIR} NO_DEFAULT_PATH)

# openpnp-capture (vendored, replaces OpenCV for camera capture)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(
    openpnp-capture
    GIT_REPOSITORY https://github.com/openpnp/openpnp-capture.git
    GIT_TAG        v0.0.30
)
FetchContent_MakeAvailable(openpnp-capture)
unset(CMAKE_POLICY_VERSION_MINIMUM CACHE)

# stb (header-only image I/O: JPEG/PNG/BMP/TGA read+write)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)
set(STB_INCLUDE_DIRS "${stb_SOURCE_DIR}")

# yaml-cpp for config parsing
find_package(yaml-cpp REQUIRED)

# CLI11 for command line parsing
find_package(CLI11 REQUIRED)

# spdlog for logging
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)
