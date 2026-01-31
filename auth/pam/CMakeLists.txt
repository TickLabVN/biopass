set(PAM facepass_pam)
add_library(${PAM} SHARED
    pam.cc
    identify.cc
)
target_include_directories(${PAM} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
    ${FaceDetection_INCLUDE_DIRS}
    ${FaceAntiSpoof_INCLUDE_DIRS}
    ${FaceRecognition_INCLUDE_DIRS}
    ${FaceConfig_INCLUDE_DIRS}
)
target_link_libraries(${PAM}
    ${OpenCV_LIBS} ${TORCH_LIBRARIES}
    facepass_det facepass_reg facepass_cfg facepass_as
)