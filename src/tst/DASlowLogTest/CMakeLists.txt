# DASlowLogTest 的 CMakeLists.txt
# 测试 DAMessageHandler 模块的慢日志功能（DASlowLog / DA_SLOW 分流 / slow.log 路由）

# spdlog::spdlog 是 DAMessageHandler 的 PRIVATE 依赖，测试目标不可直接引用
# 此处 find_package 仅为让 COPY_DLL_TARGETS 能解析到 spdlog 的 DLL 路径
find_package(spdlog CONFIG QUIET)

da_add_test(
    NAME DASlowLogTest
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAMessageHandler
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../DAMessageHandler
    # 输出目录到 build/bin（与 DA DLLs 同目录），部署 Qt DLLs，复制 spdlog DLL
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAMessageHandler DAWorkbench::DAUtils
    COPY_DLL_TARGETS spdlog::spdlog
)
