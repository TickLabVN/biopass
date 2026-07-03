add_executable(antispoof_policy_test
    antispoof_policy_test.cc
)
target_include_directories(antispoof_policy_test PRIVATE
    ${CMAKE_SOURCE_DIR}/core
)
add_test(NAME antispoof_policy_test COMMAND antispoof_policy_test)

add_executable(ir_liveness_analyzer_test
    ir_liveness_analyzer_test.cc
    ${CMAKE_SOURCE_DIR}/face/antispoofing/ir_liveness_analyzer.cc
)
target_include_directories(ir_liveness_analyzer_test PRIVATE
    ${CMAKE_SOURCE_DIR}/face/antispoofing
)
target_link_libraries(ir_liveness_analyzer_test PRIVATE
    biopass_stb
)
add_test(NAME ir_liveness_analyzer_test COMMAND ir_liveness_analyzer_test)

add_executable(auth_config_test
    auth_config_test.cc
)
target_link_libraries(auth_config_test PRIVATE
    biopass_core
    yaml-cpp::yaml-cpp
)
add_test(NAME auth_config_test COMMAND auth_config_test)
