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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS} ${OpenCV_CFLAGS_OTHER}")

# Find dependencies
SET(Torch_DIR external/libtorch/share/cmake/Torch)

find_package(Torch REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

add_subdirectory(config)
add_subdirectory(face)
add_subdirectory(pam)

set(FaceDetection_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/face/detection)
set(FaceRecognition_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/face/recognition)
set(FaceAntiSpoof_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/face/antispoofing)
set(FaceConfig_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/config)

add_subdirectory(cli)

add_subdirectory(test/face_engine)
add_subdirectory(test/pam)

install(
    TARGETS ${PROJECT_NAME} facepass_cfg facepass_det facepass_reg facepass_as
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
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libpam0g-dev, libcli11-dev, libopencv-dev")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Nguyen Phuc Vinh & Tran Trung Thai") #required
# SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinst")

INCLUDE(CPack)
