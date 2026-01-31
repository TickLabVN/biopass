add_library(facepass_det SHARED
    face_detection.cc
    utils.cc
)
set(FaceDetection_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(facepass_det PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
)
target_include_directories(facepass_det PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
