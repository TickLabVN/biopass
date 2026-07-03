add_subdirectory(detection)
add_subdirectory(recognition)
add_subdirectory(antispoofing)

set(BIOPASS_OPENCV_GREY_CAPTURE "AUTO" CACHE STRING "OpenCV GREY camera fallback: AUTO, ON, or OFF")
set_property(CACHE BIOPASS_OPENCV_GREY_CAPTURE PROPERTY STRINGS AUTO ON OFF)
if(NOT BIOPASS_OPENCV_GREY_CAPTURE MATCHES "^(AUTO|ON|OFF)$")
    message(FATAL_ERROR "BIOPASS_OPENCV_GREY_CAPTURE must be AUTO, ON, or OFF")
endif()
if(NOT BIOPASS_OPENCV_GREY_CAPTURE STREQUAL "OFF")
    find_package(OpenCV QUIET COMPONENTS core imgproc videoio)
    if(BIOPASS_OPENCV_GREY_CAPTURE STREQUAL "ON" AND NOT OpenCV_FOUND)
        message(FATAL_ERROR "BIOPASS_OPENCV_GREY_CAPTURE=ON requires OpenCV core/imgproc/videoio")
    endif()
endif()

# stb image library (JPEG/PNG/BMP/TGA read+write, vendored via FetchContent)
add_library(biopass_stb STATIC
    stb_impl.cc
)
set_target_properties(biopass_stb PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(biopass_stb PUBLIC
    ${STB_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Shared face utilities (camera capture, debug image I/O).
add_library(biopass_face_common STATIC
    common/camera_capture.cc
    common/debug_image_io.cc
)
set_target_properties(biopass_face_common PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(biopass_face_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/core
)
target_link_libraries(biopass_face_common
    biopass_stb
    biopass_core
    openpnp-capture
    spdlog::spdlog_header_only
)
if(OpenCV_FOUND)
    target_include_directories(biopass_face_common PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(biopass_face_common ${OpenCV_LIBS})
    target_compile_definitions(biopass_face_common PRIVATE BIOPASS_HAS_OPENCV=1)
    message(STATUS "OpenCV GREY camera fallback enabled")
elseif(NOT BIOPASS_OPENCV_GREY_CAPTURE STREQUAL "OFF")
    message(STATUS "OpenCV GREY camera fallback disabled (OpenCV not found)")
endif()

# Face auth wrapper library
add_library(biopass_face STATIC
    face_auth.cc
)

set_target_properties(biopass_face PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(biopass_face PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${ONNXRUNTIME_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/detection
    ${CMAKE_CURRENT_SOURCE_DIR}/recognition
    ${CMAKE_CURRENT_SOURCE_DIR}/antispoofing
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

target_link_libraries(biopass_face
    biopass_core
    biopass_det
    biopass_reg
    biopass_as
    biopass_face_common
    biopass_stb
    ${ONNXRUNTIME_LIB}
)
