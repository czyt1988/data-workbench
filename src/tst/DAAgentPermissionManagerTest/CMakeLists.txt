# DAAgentPermissionManagerTest 的 CMakeLists.txt
# 测试 DAAgent 库的 DAAgentPermissionManager 权限引擎（模式×分级决策矩阵 / 硬 deny 优先 /
# 会话记忆 / 判官兜底 / ${workspace} 变量 / 播种与回填 / 分级回退链 / 配置 round-trip）
#
# 镜像 DAAgentSessionStoreTest：只测 C++ 逻辑，不调 Python，只链接 DAWorkbench::DAAgent。
# DAAgent 是 SHARED 库，exe 运行时需 DAAgent.dll 及其传递依赖 DLL，
# damacro_setup_test 把 exe 输出到 ${CMAKE_BINARY_DIR}/bin（与 DA DLL 同目录）并
# 经 windeployqt 部署 Qt DLL。

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

# spdlog::spdlog 是 DAMessageHandler 的 PRIVATE 依赖（DAAgent 经 DAInterface 传递依赖
# DAMessageHandler，运行时需 spdlog.dll）。此处 find_package 仅为让 COPY_DLL_TARGETS
# 能解析到 spdlog 的 DLL 路径（仿 DAAgentSessionStoreTest）。
find_package(spdlog CONFIG QUIET)

add_executable(DAAgentPermissionManagerTest main.cpp)

target_link_libraries(DAAgentPermissionManagerTest PRIVATE
    DAWorkbench::DAAgent
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

# 显式补齐传递依赖的头文件路径（DADir.h / DAGlobals.h），仿 DAAgentSessionStoreTest
target_include_directories(DAAgentPermissionManagerTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/DAAgent
    ${CMAKE_SOURCE_DIR}/src/DAUtils
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(DAAgentPermissionManagerTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE OFF
)

# 部署 Qt DLLs 和 DAAgent.dll 依赖（仿 DAAgentSessionStoreTest，说明同彼处注释）
damacro_setup_test(DAAgentPermissionManagerTest
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAAgent DAWorkbench::DAInterface DAWorkbench::DAGui
    COPY_DLL_TARGETS spdlog::spdlog
)

# 添加测试
add_test(NAME DAAgentPermissionManagerTest COMMAND DAAgentPermissionManagerTest)

# 将测试可执行文件添加到全局变量，以便在顶级测试目录中使用
set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} DAAgentPermissionManagerTest PARENT_SCOPE)

# 添加运行此特定测试的自定义目标
add_custom_target(run_DAAgentPermissionManagerTest
    COMMAND $<TARGET_FILE:DAAgentPermissionManagerTest>
    DEPENDS DAAgentPermissionManagerTest
    WORKING_DIRECTORY $<TARGET_FILE_DIR:DAAgentPermissionManagerTest>
    COMMENT "Running DAAgentPermissionManagerTest"
)
