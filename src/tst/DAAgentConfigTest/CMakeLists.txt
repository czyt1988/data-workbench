# DAAgentConfigTest 的 CMakeLists.txt
# 测试 DAAgent 库的 DAAgentConfig 配置领域模型（agent-config.json 稀疏读写 /
# 旧 agent-config.ini 一次性迁移 / .bak 恢复 / 损坏自愈 / 稀疏合并语义 /
# providers 合成与规范化 / 协议投影 / 权限标量）
#
# 镜像 DAAgentPermissionManagerTest：只测 C++ 逻辑，不调 Python，只链接 DAWorkbench::DAAgent。

find_package(spdlog CONFIG QUIET)

da_add_test(
    NAME DAAgentConfigTest
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
