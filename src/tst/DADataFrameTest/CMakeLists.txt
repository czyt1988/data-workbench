# DADataFrameTest 的 CMakeLists.txt
# 测试 DAPyBindQt 模块的 DAPyDataFrame 功能

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

add_executable(DADataFrameTest main.cpp)

target_link_libraries(DADataFrameTest PRIVATE
    DAWorkbench::DAPyBindQt
    DAWorkbench::DAPyScripts
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    ${Python3_LIBRARIES}
    pybind11::headers
)

target_include_directories(DADataFrameTest PRIVATE
    ${Python3_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/DAPyBindQt
    ${CMAKE_SOURCE_DIR}/src/DAPyBindQt/pandas
    ${CMAKE_SOURCE_DIR}/src/DAUtils
    ${CMAKE_SOURCE_DIR}/src/DAPyScripts
)

set_target_properties(DADataFrameTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE OFF
)

# 部署 Qt DLLs 和 Python 依赖
damacro_setup_test(DADataFrameTest
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAPyBindQt
)

# 添加测试
add_test(NAME DADataFrameTest COMMAND DADataFrameTest)

# 将测试可执行文件添加到全局变量，以便在顶级测试目录中使用
set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} DADataFrameTest PARENT_SCOPE)

# 添加运行此特定测试的自定义目标
add_custom_target(run_DADataFrameTest
    COMMAND $<TARGET_FILE:DADataFrameTest>
    DEPENDS DADataFrameTest
    WORKING_DIRECTORY $<TARGET_FILE_DIR:DADataFrameTest>
    COMMENT "Running DADataFrameTest"
)
