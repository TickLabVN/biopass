add_executable(antispoof_policy_test
    antispoof_policy_test.cc
)
target_include_directories(antispoof_policy_test PRIVATE
    ${CMAKE_SOURCE_DIR}/core
)
add_test(NAME antispoof_policy_test COMMAND antispoof_policy_test)

add_executable(auth_config_test
    auth_config_test.cc
)
target_link_libraries(auth_config_test PRIVATE
    biopass_core
    yaml-cpp::yaml-cpp
)
add_test(NAME auth_config_test COMMAND auth_config_test)
