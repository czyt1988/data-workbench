# DAAgentToolSpecTest 的 CMakeLists.txt
# 测试 DAAgent 库的 DAAgentToolSpec → OpenAI function schema 序列化等价性
#（DA::toJson，见 src/DAAgent/DAAgentToolSpecJson.h）。
#
# 与 DAAgentSessionStoreTest 同理：只测 C++ 逻辑，不调 Python，
# 故不链接 DAPyScripts/DAPyBindQt/Python3/pybind11，只链接 DAWorkbench::DAAgent。
# DAAgent 是 SHARED 库，exe 运行时需 DAAgent.dll 及其传递依赖 DLL，
# da_add_test 把 exe 输出到 ${CMAKE_BINARY_DIR}/bin（与 DA DLL 同目录）并
# 经 windeployqt 部署 Qt DLL。

# spdlog::spdlog 是 DAMessageHandler 的 PRIVATE 依赖（DAAgent 经 DAInterface 传递依赖
# DAMessageHandler，运行时需 spdlog.dll）。测试目标不可直接引用，此处 find_package
# 仅为让 COPY_DLL_TARGETS 能解析到 spdlog 的 DLL 路径（仿 DAAgentSessionStoreTest）。
find_package(spdlog CONFIG QUIET)

da_add_test(
    NAME DAAgentToolSpecTest
    SOURCES main.cpp
    QT_COMPONENTS Test
    LINK_PRIVATE
        DAWorkbench::DAAgent
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
    # 显式补齐 DAAgentToolSpec.h 传递依赖的头文件路径。
    # DAAgent 的 PUBLIC include 理论上已传递，但仿 DAAgentSessionStoreTest 显式声明更稳健。
    INCLUDE_PRIVATE
        ${CMAKE_SOURCE_DIR}/src/DAAgent
        ${CMAKE_SOURCE_DIR}/src/DAUtils
        ${CMAKE_SOURCE_DIR}/src
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAAgent DAWorkbench::DAInterface DAWorkbench::DAGui
    COPY_DLL_TARGETS spdlog::spdlog
)
