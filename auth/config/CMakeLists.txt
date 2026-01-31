add_library(facepass_cfg SHARED face_config.cc)
target_include_directories(facepass_cfg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set(FaceConfig_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})