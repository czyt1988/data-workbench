# 顶级测试目录的 CMakeLists.txt
# 这个文件管理所有子目录的单元测试
# 注意：顶层 CMakeLists.txt 已调用 project() 和 enable_testing()

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用 AUTOMOC 以支持 Qt Test 的 Q_OBJECT 宏和 main.moc 包含
set(CMAKE_AUTOMOC ON)

# 查找所有测试子目录
file(GLOB test_subdirectories RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} */)

# 遍历所有子目录并添加测试
foreach(subdir ${test_subdirectories})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
        # 跳过非测试目录（如CMakeFiles等）
            if(NOT subdir MATCHES "^\\..*" AND
               NOT subdir STREQUAL "CMakeFiles" AND
               NOT subdir STREQUAL "Testing" AND
               EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt)
                message(STATUS "Adding test subdirectory: ${subdir}")
                add_subdirectory(${subdir})
            endif()
    endif()
endforeach()

# 添加一个运行所有测试的自定义目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${ALL_TEST_EXECUTABLES}
    COMMENT "Running all tests"
)

# TODO: 使用 OpenCppCoverage (Windows) 或 gcov (Linux) 实现覆盖率报告
# add_custom_target(test_coverage
#     COMMAND OpenCppCoverage --export_type=cobertura --output=${CMAKE_BINARY_DIR}/coverage.xml --sources ${CMAKE_SOURCE_DIR}/src -- $<TARGET_FILE:run_all_tests>
#     COMMENT "Generating test coverage report"
# )
