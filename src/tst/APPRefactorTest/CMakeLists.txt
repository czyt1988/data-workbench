# APPRefactorTest 的 CMakeLists.txt
#
# 覆盖 APP 层纯逻辑单元测试（不依赖完整 AppMainWindow）：
# 1. 工程关闭/打开的动作策略（DAAppProjectActionPolicy）
# 2. 窗口状态序列化（DAAppWindowStateSerializer）
# 3. T6：设置上下文提供者注册表（DASettingContextProviderRegistry）与 dock 激活路由判定
#    （DAAppContextRoutePolicy）。两者只依赖 Qt，故直接把实现源文件编入测试，
#    不链接 DAInterface/APP 库，保持 core 套件轻量。

da_add_test(
    NAME APPRefactorTest
    SUITE core
    SOURCES main.cpp
            ${CMAKE_SOURCE_DIR}/src/DAInterface/DASettingContextProviderRegistry.cpp
            ${CMAKE_SOURCE_DIR}/src/APP/Controller/DAAppContextRoutePolicy.cpp
    QT_COMPONENTS Core Widgets Test
    LINK_PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../APP
        ${CMAKE_CURRENT_LIST_DIR}/../../APP/Controller
        ${CMAKE_SOURCE_DIR}/src/DAInterface
    # 输出目录 build/bin（与 DA DLLs 同目录）；QTEST_MAIN 需要 QApplication，部署 Qt DLLs
    DEPLOY_QT
)
