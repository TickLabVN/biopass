set(FACE_TEST face_engine_test)
add_executable(${FACE_TEST} main.cpp)
target_include_directories(${FACE_TEST} PRIVATE 
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
    ${FaceDetection_INCLUDE_DIRS}
    ${FaceRecognition_INCLUDE_DIRS}
)
target_link_libraries(${FACE_TEST} PRIVATE
    ${OpenCV_LIBS} ${TORCH_LIBRARIES}
    facepass_det
    facepass_reg
)