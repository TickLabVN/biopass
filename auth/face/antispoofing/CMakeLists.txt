add_library(biopass_as SHARED
    face_as.cc
)
set(FaceAntiSpoof_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(biopass_as PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
)
target_include_directories(biopass_as PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
