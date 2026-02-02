set(TARGET_NAME facepass_fingerprint_test)
add_executable(${TARGET_NAME} main.cc)

target_link_libraries(${TARGET_NAME} PRIVATE
    facepass_fingerprint
    facepass_core
)
