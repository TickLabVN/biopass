set(PAM facepass_pam)
add_library(${PAM} SHARED
    pam.cc
)

target_include_directories(${PAM} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
    ${FaceDetection_INCLUDE_DIRS}
    ${FaceAntiSpoof_INCLUDE_DIRS}
    ${FaceRecognition_INCLUDE_DIRS}
    ${FaceConfig_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/face
    ${CMAKE_SOURCE_DIR}/voice
    ${CMAKE_SOURCE_DIR}/fingerprint
)

target_link_libraries(${PAM}
    ${OpenCV_LIBS}
    ${TORCH_LIBRARIES}
    facepass_core
    facepass_face
    facepass_voice
    facepass_fingerprint
)