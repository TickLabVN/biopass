cmake_minimum_required(VERSION 3.28)

add_library(facepass_fingerprint STATIC
    fingerprint_auth.cc
    fingerprint_ffi.cc
)

set_target_properties(facepass_fingerprint PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(facepass_fingerprint PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(facepass_fingerprint
    facepass_core
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0 gio-unix-2.0)

target_include_directories(facepass_fingerprint PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(facepass_fingerprint ${GLIB_LIBRARIES})
