set(CLI ${PROJECT_NAME})

file(GLOB CMD_SOURCES "cmd/*.cc")
add_executable(${CLI} main.cc
    face.cc
    pam_conf.cc
    ${CMD_SOURCES}
)
target_include_directories(${CLI} PRIVATE 
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
    ${FaceDetection_INCLUDE_DIRS}
    ${FaceConfig_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR} # Add the parent directory
    ${CMAKE_CURRENT_SOURCE_DIR}/cmd
)
target_link_libraries(${CLI} PRIVATE
    ${OpenCV_LIBS} ${TORCH_LIBRARIES}
    facepass_det
    facepass_cfg
    CLI11::CLI11
)
