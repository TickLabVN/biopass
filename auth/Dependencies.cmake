# ==============================================================================
# Dependency Management
# ==============================================================================

# Fetch libtorch
include(FetchContent)
FetchContent_Declare(
    libtorch
    URL https://download.pytorch.org/libtorch/nightly/cpu/libtorch-cxx11-abi-shared-without-deps-2.2.0.dev20231031%2Bcpu.zip
)
# CMake 3.28+ deprecation warning workaround: Since libtorch is precompiled, we must populate it manually.
FetchContent_GetProperties(libtorch)
if(NOT libtorch_POPULATED)
    FetchContent_Populate(libtorch)
endif()

# Find dependencies
set(CMAKE_PREFIX_PATH "${libtorch_SOURCE_DIR}")
set(Torch_DIR ${libtorch_SOURCE_DIR}/share/cmake/Torch)

# Hack to silence shadowing warnings by telling CMake this is an "implicit" directory
list(APPEND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${libtorch_SOURCE_DIR}/lib")

find_package(Torch REQUIRED NO_DEFAULT_PATH)

# Ensure we use full paths to libraries to avoid shadowing warnings
cmake_policy(SET CMP0060 NEW)

find_package(OpenCV REQUIRED)

# yaml-cpp for config parsing
find_package(yaml-cpp REQUIRED)

# spdlog for logging
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)
