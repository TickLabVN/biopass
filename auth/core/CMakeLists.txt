cmake_minimum_required(VERSION 3.28)

add_library(facepass_core STATIC
    auth_manager.cc
    auth_config.cc
)

set_target_properties(facepass_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(facepass_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(facepass_core
    pthread
    yaml-cpp
)
