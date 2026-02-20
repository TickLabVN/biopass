cmake_minimum_required(VERSION 3.28)

add_library(biopass_voice STATIC
    voice_auth.cc
)

set_target_properties(biopass_voice PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(biopass_voice PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(biopass_voice
    biopass_core
)
