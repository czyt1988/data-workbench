# 顶级测试目录的 CMakeLists.txt
# 这个文件管理所有子目录的单元测试

cmake_minimum_required(VERSION 3.12)
project(DATests)

# 启用测试
enable_testing()

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加包含目录
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# 查找所有测试子目录
file(GLOB test_subdirectories RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} */)

# 遍历所有子目录并添加测试
foreach(subdir ${test_subdirectories})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
        # 跳过非测试目录（如CMakeFiles等）
        if(NOT subdir MATCHES "^\\..*" AND 
           NOT subdir STREQUAL "CMakeFiles" AND
           NOT subdir STREQUAL "Testing")
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

# 添加一个显示测试覆盖率的自定义目标（如果使用覆盖率工具）
add_custom_target(test_coverage
    COMMAND echo "Test coverage reporting would be implemented here"
    COMMENT "Generating test coverage report"
)