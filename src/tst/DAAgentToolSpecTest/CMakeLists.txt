# DAAgentToolSpecTest 的 CMakeLists.txt
# 测试 DAAgent 库的 DAAgentToolSpec → OpenAI function schema 序列化等价性
#（DA::toJson，见 src/DAAgent/DAAgentToolSpecJson.h）。
#
# 与 DAAgentSessionStoreTest 同理：只测 C++ 逻辑，不调 Python，
# 故不链接 DAPyScripts/DAPyBindQt/Python3/pybind11，只链接 DAWorkbench::DAAgent。
# DAAgent 是 SHARED 库，exe 运行时需 DAAgent.dll 及其传递依赖 DLL，
# damacro_setup_test 把 exe 输出到 ${CMAKE_BINARY_DIR}/bin（与 DA DLL 同目录）并
# 经 windeployqt 部署 Qt DLL。

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

# spdlog::spdlog 是 DAMessageHandler 的 PRIVATE 依赖（DAAgent 经 DAInterface 传递依赖
# DAMessageHandler，运行时需 spdlog.dll）。测试目标不可直接引用，此处 find_package
# 仅为让 COPY_DLL_TARGETS 能解析到 spdlog 的 DLL 路径（仿 DAAgentSessionStoreTest）。
find_package(spdlog CONFIG QUIET)

add_executable(DAAgentToolSpecTest main.cpp)

target_link_libraries(DAAgentToolSpecTest PRIVATE
    DAWorkbench::DAAgent
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

# 显式补齐 DAAgentToolSpec.h 传递依赖的头文件路径。
# DAAgent 的 PUBLIC include 理论上已传递，但仿 DAAgentSessionStoreTest 显式声明更稳健。
target_include_directories(DAAgentToolSpecTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/DAAgent
    ${CMAKE_SOURCE_DIR}/src/DAUtils
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(DAAgentToolSpecTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE OFF
)

# 部署 Qt DLLs 和 DAAgent.dll 依赖（仿 DAAgentSessionStoreTest）。
# windeployqt 只扫描直接传入文件的 Qt 依赖，不递归扫描同目录的 DA DLL 的二次 Qt 依赖，
# 故须把携带 Qt 依赖的 DA DLL 都显式传入，否则测试 exe 启动报 0xC0000135。
damacro_setup_test(DAAgentToolSpecTest
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAAgent DAWorkbench::DAInterface DAWorkbench::DAGui
    COPY_DLL_TARGETS spdlog::spdlog
)

# 添加测试
add_test(NAME DAAgentToolSpecTest COMMAND DAAgentToolSpecTest)

# 将测试可执行文件添加到全局变量，以便在顶级测试目录中使用
set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} DAAgentToolSpecTest PARENT_SCOPE)

# 添加运行此特定测试的自定义目标
add_custom_target(run_DAAgentToolSpecTest
    COMMAND $<TARGET_FILE:DAAgentToolSpecTest>
    DEPENDS DAAgentToolSpecTest
    WORKING_DIRECTORY $<TARGET_FILE_DIR:DAAgentToolSpecTest>
    COMMENT "Running DAAgentToolSpecTest"
)
