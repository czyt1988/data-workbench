# 顶级测试目录的 CMakeLists.txt
# 这个文件管理所有子目录的单元测试
# 注意：顶层 CMakeLists.txt 已调用 project() 和 enable_testing()
# 各测试经 da_add_test 登记，目标名记录在全局属性 DA_TEST_TARGETS
#
# 测试按"套件（suite）"分组，套件标签由各测试 CMakeLists.txt 的
# `da_add_test(... SUITE <core|data|table|chart|agent|workflow>)` 声明，
# 日常按改动范围只跑相关套件（见 scripts/test.ps1）：
#     .\scripts\test.ps1 -Suite table      # 只跑表格套件
#     .\scripts\test.ps1 -Changed          # 按 git 改动自动挑套件
#     .\scripts\test.ps1 -All              # 全量（仍不含 bench）
# 性能/基准用例由目标名（Perf/Benchmark）自动打上 bench 标签，默认套件不跑。

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

# 套件完整性检查：没写 SUITE 的测试不会出现在任何套件里，按改动跑测试时会被静默漏掉。
# 这里只告警（不 FATAL_ERROR），避免新增测试的人被配置期直接挡死，但每次 configure 都会可见。
get_property(DA_TESTS_WITHOUT_SUITE GLOBAL PROPERTY DA_TESTS_WITHOUT_SUITE)
if(DA_TESTS_WITHOUT_SUITE)
    message(WARNING
        "以下测试没有声明 SUITE，按套件跑测试时不会被选中（请在其 CMakeLists.txt 的 "
        "da_add_test 里补 SUITE <core|data|table|chart|agent|workflow>）：\n    "
        "${DA_TESTS_WITHOUT_SUITE}")
endif()

# 添加一个运行所有测试的自定义目标（各测试由 da_add_test 登记到全局属性）
get_property(DA_ALL_TEST_TARGETS GLOBAL PROPERTY DA_TEST_TARGETS)
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${DA_ALL_TEST_TARGETS}
    COMMENT "Running all tests"
)
