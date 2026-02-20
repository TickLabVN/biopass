set(PAM biopass_pam)
add_library(${PAM} SHARED
    pam.cc
)

# The PAM module is now lightweight and only needs basic PAM headers
target_link_libraries(${PAM}
    pam
)

# New helper binary that contains the actual heavy ML logic
add_executable(biopass-helper
    helper.cc
)

target_include_directories(biopass-helper PRIVATE
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

target_link_libraries(biopass-helper
    ${OpenCV_LIBS}
    ${TORCH_LIBRARIES}
    biopass_core
    biopass_face
    biopass_voice
    biopass_fingerprint
    spdlog::spdlog_header_only
)

# Install rules
install(TARGETS biopass-helper DESTINATION bin)