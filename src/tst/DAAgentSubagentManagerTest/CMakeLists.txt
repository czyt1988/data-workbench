# DAAgentSubagentManagerTest 的 CMakeLists.txt
# 测试 DAAgent 库的子 agent 定义库（subagent-phase1 P1）：
# frontmatter 往返（含 permissions 原样存留）/ ensureDefaultSubagents 播种 /
# registerBuiltin 已存在跳过 / 删除 / 重命名（save oldName）。
#
# 与 DAAgentSessionStoreTest 相同的隔离与部署模式：只测 C++ 逻辑，不调 Python，
# 只链接 DAWorkbench::DAAgent；damacro_setup_test 把 exe 输出到 ${CMAKE_BINARY_DIR}/bin
# 并部署 Qt/DAAgent 依赖 DLL。

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

# spdlog::spdlog 是 DAMessageHandler 的 PRIVATE 依赖（DAAgent 经 DAInterface 传递依赖
# DAMessageHandler，运行时需 spdlog.dll）。测试目标不可直接引用，此处 find_package
# 仅为让 COPY_DLL_TARGETS 能解析到 spdlog 的 DLL 路径（仿 DAAgentSessionStoreTest）。
find_package(spdlog CONFIG QUIET)

add_executable(DAAgentSubagentManagerTest main.cpp)

target_link_libraries(DAAgentSubagentManagerTest PRIVATE
    DAWorkbench::DAAgent
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

# 显式补齐传递依赖的头文件路径（仿 DAAgentSessionStoreTest）
target_include_directories(DAAgentSubagentManagerTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/DAAgent
    ${CMAKE_SOURCE_DIR}/src/DAUtils
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(DAAgentSubagentManagerTest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE OFF
)

# 部署 Qt DLLs 和 DAAgent.dll 依赖（仿 DAAgentSessionStoreTest）：
# windeployqt 只扫描直接传入文件的 Qt 依赖，不递归扫描同目录的 DA DLL，
# 故须把携带 Qt 依赖的 DA DLL 显式传入，否则测试 exe 启动报 0xC0000135。
damacro_setup_test(DAAgentSubagentManagerTest
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAAgent DAWorkbench::DAInterface DAWorkbench::DAGui
    COPY_DLL_TARGETS spdlog::spdlog
)

# 添加测试
add_test(NAME DAAgentSubagentManagerTest COMMAND DAAgentSubagentManagerTest)

# 将测试可执行文件添加到全局变量，以便在顶级测试目录中使用
set(ALL_TEST_EXECUTABLES ${ALL_TEST_EXECUTABLES} DAAgentSubagentManagerTest PARENT_SCOPE)

# 添加运行此特定测试的自定义目标
add_custom_target(run_DAAgentSubagentManagerTest
    COMMAND $<TARGET_FILE:DAAgentSubagentManagerTest>
    DEPENDS DAAgentSubagentManagerTest
    WORKING_DIRECTORY $<TARGET_FILE_DIR:DAAgentSubagentManagerTest>
    COMMENT "Running DAAgentSubagentManagerTest"
)
