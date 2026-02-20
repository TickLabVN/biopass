add_library(biopass_reg SHARED
    face_recognition.cc
)
set(FaceRecognition_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(biopass_reg PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
)
target_include_directories(biopass_reg PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
