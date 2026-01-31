add_subdirectory(detection)
add_subdirectory(recognition)
add_subdirectory(antispoofing)

# Face auth wrapper library
add_library(facepass_face STATIC
    face_auth.cc
)

set_target_properties(facepass_face PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(facepass_face PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/config
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/detection
    ${CMAKE_CURRENT_SOURCE_DIR}/recognition
    ${CMAKE_CURRENT_SOURCE_DIR}/antispoofing
)

target_link_libraries(facepass_face
    facepass_core
    facepass_cfg
    facepass_det
    facepass_reg
    facepass_as
    ${OpenCV_LIBS}
    ${TORCH_LIBRARIES}
)
