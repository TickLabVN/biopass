cmake_minimum_required(VERSION 3.28)

add_library(facepass_voice STATIC
    voice_auth.cc
)

set_target_properties(facepass_voice PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(facepass_voice PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(facepass_voice
    facepass_core
)
