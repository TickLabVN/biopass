add_library(facepass_reg SHARED
    face_recognition.cc
)
set(FaceRecognition_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(facepass_reg PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
)
target_include_directories(facepass_reg PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
