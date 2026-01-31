add_library(facepass_as SHARED
    face_as.cc
)
set(FaceAntiSpoof_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(facepass_as PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
)
target_include_directories(facepass_as PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
