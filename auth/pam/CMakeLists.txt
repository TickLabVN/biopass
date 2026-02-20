set(PAM facepass_pam)
add_library(${PAM} SHARED
    pam.cc
)

# The PAM module is now lightweight and only needs basic PAM headers
target_link_libraries(${PAM}
    pam
)

# New helper binary that contains the actual heavy ML logic
add_executable(facepass-helper
    helper.cc
)

target_include_directories(facepass-helper PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
    ${FaceDetection_INCLUDE_DIRS}
    ${FaceAntiSpoof_INCLUDE_DIRS}
    ${FaceRecognition_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/face
    ${CMAKE_SOURCE_DIR}/voice
    ${CMAKE_SOURCE_DIR}/fingerprint
)

target_link_libraries(facepass-helper
    ${OpenCV_LIBS}
    ${TORCH_LIBRARIES}
    facepass_core
    facepass_face
    facepass_voice
    facepass_fingerprint
    spdlog::spdlog_header_only
)

# Install rules
install(TARGETS facepass-helper DESTINATION bin)