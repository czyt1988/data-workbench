# DAPyWorkFlow 测试的 CMakeLists.txt

# Qt Test 与 AUTOMOC 支持
set(CMAKE_AUTOMOC ON)

# 添加包含目录 - DAPyWorkFlow 库头文件
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../../DAPyWorkFlow
)

# 创建测试可执行文件
add_executable(DAPyWorkFlowTests main.cpp tst_dapynodemetadata.cpp tst_dapynodemetadata.h tst_linkpoints.cpp tst_linkpoints.h)

# 设置目标属性
set_target_properties(DAPyWorkFlowTests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Windows NOMINMAX 宏，防止 min/max 与 std::min/std::max 冲突
if(WIN32)
    target_compile_definitions(DAPyWorkFlowTests PRIVATE NOMINMAX)
endif()

# 链接库
target_link_libraries(DAPyWorkFlowTests PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
    ${DA_PROJECT_NAME}::DAPyWorkFlow
)

# Python/pybind11 embed 支持（用于 scoped_interpreter 和代理测试）
damacro_import_Python(DAPyWorkFlowTests)
find_package(pybind11 REQUIRED)
target_link_libraries(DAPyWorkFlowTests PRIVATE pybind11::embed)

# 添加测试
add_test(NAME DAPyWorkFlowTests COMMAND DAPyWorkFlowTests)

# 将测试可执行文件添加到全局变量，以便在顶级测试目录中使用
set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} DAPyWorkFlowTests PARENT_SCOPE)

# 添加一个运行此特定测试的自定义目标
add_custom_target(run_DAPyWorkFlowTests
    COMMAND ./DAPyWorkFlowTests
    DEPENDS DAPyWorkFlowTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running DAPyWorkFlowTests"
)