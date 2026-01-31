cmake_minimum_required(VERSION 3.28)
project(facepass)
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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS} ${OpenCV_CFLAGS_OTHER}")

# Find dependencies
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/libtorch")
SET(Torch_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/libtorch/share/cmake/Torch)

# Hack to silence shadowing warnings by telling CMake this is an "implicit" directory
list(APPEND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/external/libtorch/lib")

find_package(Torch REQUIRED NO_DEFAULT_PATH)

# Ensure we use full paths to libraries to avoid shadowing warnings
cmake_policy(SET CMP0060 NEW)

find_package(OpenCV REQUIRED)

# yaml-cpp for config parsing
find_package(yaml-cpp REQUIRED)

add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(face)
add_subdirectory(voice)
add_subdirectory(fingerprint)
add_subdirectory(pam)

set(FaceDetection_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/face/detection)
set(FaceRecognition_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/face/recognition)
set(FaceAntiSpoof_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/face/antispoofing)
set(FaceConfig_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/config)

add_subdirectory(test/face_engine)
add_subdirectory(test/pam)

install(
    TARGETS facepass_cfg facepass_det facepass_reg facepass_as
    COMPONENT applications
    RUNTIME DESTINATION "/usr/bin"
    LIBRARY DESTINATION "/usr/lib"
)
install(
    TARGETS facepass_pam
    LIBRARY DESTINATION "/lib/security"
)

# Install libtorch libraries
install(
    DIRECTORY external/libtorch/lib/
    DESTINATION "/usr/lib"
    FILES_MATCHING PATTERN "*.so"
)

install(
    FILES face/models/edgeface_s_gamma_05_ts.pt face/models/yolov11n-face.torchscript face/models/mobilenetv3_antispoof_ts.pt
    DESTINATION /etc/xdg/${PROJECT_NAME}/models
)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_NAME ${PROJECT_NAME})
SET(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Facepass is a face recognition authentication system.")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libpam0g-dev, libopencv-dev, libyaml-cpp-dev")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Nguyen Phuc Vinh & Tran Trung Thai") #required
# SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinst")

INCLUDE(CPack)
