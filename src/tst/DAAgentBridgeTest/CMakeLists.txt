# DAAgentBridgeTest 的 CMakeLists.txt
# 测试 DAAgent 库的 DAAgentBridge 子进程协议级行为（启动/错误/停止/崩溃恢复状态机）
#
# 测试基建：假 agent（fake agent）——测试 exe 自身以 "--fake-agent=<scenario>"
# 参数被子进程化（QProcess program=applicationFilePath），按剧本收发 JSON Lines
# 协议消息，模拟 ready/error/done/崩溃等序列。不依赖真实 Python 环境。
#
# 链接/部署仿 DAAgentSessionStoreTest：只测 C++ 逻辑，仅链接 DAWorkbench::DAAgent。

# spdlog::spdlog 是 DAMessageHandler 的 PRIVATE 依赖（DAAgent 经 DAInterface 传递依赖
# DAMessageHandler，运行时需 spdlog.dll）。find_package 仅为让 COPY_DLL_TARGETS 能解析
# 到 spdlog 的 DLL 路径（仿 DAAgentSessionStoreTest）。
find_package(spdlog CONFIG QUIET)

da_add_test(
    NAME DAAgentBridgeTest
    SOURCES main.cpp
    QT_COMPONENTS Test
    LINK_PRIVATE
        DAWorkbench::DAAgent
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${CMAKE_SOURCE_DIR}/src/DAAgent
        ${CMAKE_SOURCE_DIR}/src/DAUtils
        ${CMAKE_SOURCE_DIR}/src
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAAgent DAWorkbench::DAInterface DAWorkbench::DAGui
    COPY_DLL_TARGETS spdlog::spdlog
)
