# DAAgentModuleTest 的 CMakeLists.txt
# 测试 DAAgent 库的 DAAgentModule 会话化编排层（状态机记账/持久化/信号路由）
#
# 测试基建（假 agent）：与 DAAgentBridgeTest 同源思路——Module 冷启动桥时经
# detectPythonExePath()/detectAgentScriptPath() 解析子进程命令，测试把
# python-config.json 的 interpreter 指向测试 exe 自身（initTestCase 备份-恢复），
# 子进程模式按 argv[1] 以 agent_runner.py 结尾识别，剧本经环境变量
# DA_FAKE_SCENARIO 选择（默认 module-echo）。不依赖真实 Python/LLM 环境。

find_package(spdlog CONFIG QUIET)

da_add_test(
    NAME DAAgentModuleTest
    SOURCES main.cpp
    QT_COMPONENTS Test
    LINK_PRIVATE
        DAWorkbench::DAAgent
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${CMAKE_SOURCE_DIR}/src/DAAgent
        ${CMAKE_SOURCE_DIR}/src/DAInterface
        ${CMAKE_SOURCE_DIR}/src/DAUtils
        ${CMAKE_SOURCE_DIR}/src
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAAgent DAWorkbench::DAInterface DAWorkbench::DAGui
    COPY_DLL_TARGETS spdlog::spdlog
)
