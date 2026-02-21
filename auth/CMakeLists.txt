cmake_minimum_required(VERSION 3.28)
project(biopass)
if(DEFINED ENV{PROJECT_VERSION})
    message(STATUS "Project version: $ENV{PROJECT_VERSION}")
    set(PROJECT_VERSION $ENV{PROJECT_VERSION})
else()
    message(STATUS "Project version: latest")
    set(PROJECT_VERSION 0.1.0)
endif()

# Require C++17
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Avoid RPATH warnings about shadowed libraries in non-system directories (like libtorch)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
# Dependencies Configuration
include(Dependencies.cmake)

# Application Directories
add_subdirectory(core)
add_subdirectory(face)
add_subdirectory(voice)
add_subdirectory(fingerprint)
add_subdirectory(pam)

# Export Face Includes dynamically from Source
set(FaceDetection_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/face/detection)
set(FaceRecognition_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/face/recognition)
set(FaceAntiSpoof_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/face/antispoofing)

# Testing Directories
option(BUILD_TESTS "Build test configurations" OFF)
if(BUILD_TESTS)
    add_subdirectory(test/face_engine)
endif()

# CXX Flags Configuration
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS} ${OpenCV_CFLAGS_OTHER}")

install(
    TARGETS biopass_det biopass_reg biopass_as
    COMPONENT applications
    RUNTIME DESTINATION "/usr/bin"
    LIBRARY DESTINATION "/usr/lib/biopass"
)
install(
    TARGETS biopass_pam
    LIBRARY DESTINATION "/lib/security"
)

# Install libtorch libraries
install(
    DIRECTORY ${libtorch_SOURCE_DIR}/lib/
    DESTINATION "/usr/lib/biopass"
    FILES_MATCHING PATTERN "*.so"
)
