add_subdirectory(detection)
add_subdirectory(recognition)
add_subdirectory(antispoofing)

# stb image library (JPEG/PNG/BMP/TGA read+write, vendored via FetchContent)
add_library(biopass_stb STATIC
    stb_impl.cc
)
set_target_properties(biopass_stb PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(biopass_stb PUBLIC
    ${STB_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

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
)

target_link_libraries(biopass_face
    biopass_core
    biopass_det
    biopass_reg
    biopass_as
    biopass_stb
    openpnp-capture
    ${ONNXRUNTIME_LIB}
)
