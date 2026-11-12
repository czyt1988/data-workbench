# APPRefactorTest 的 CMakeLists.txt
#
# 覆盖 APP 层纯逻辑单元测试（不依赖完整 AppMainWindow）：
# 1. 工程关闭/打开的动作策略（DAAppProjectActionPolicy）
# 2. 窗口状态序列化（DAAppWindowStateSerializer）
# 3. T6：设置上下文提供者注册表（DASettingContextProviderRegistry，APP 路由的核心）
#    注册表只依赖 Qt，故直接把实现源文件编入测试，不链接 DAInterface，保持 core 套件轻量。

da_add_test(
    NAME APPRefactorTest
    SUITE core
    SOURCES main.cpp
            ${CMAKE_SOURCE_DIR}/src/DAInterface/DASettingContextProviderRegistry.cpp
    QT_COMPONENTS Core Widgets Test
    LINK_PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../APP
        ${CMAKE_SOURCE_DIR}/src/DAInterface
)
