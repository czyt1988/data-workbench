# DAShortcutManagerTest 的 CMakeLists.txt
# 测试 DAUtils 的 DAShortcutManager（快捷键注册/覆盖/冲突检测/JSON 持久化）

da_add_test(
    NAME DAShortcutManagerTest
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAUtils
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
    # 输出目录到 build/bin（与 DA DLLs 同目录），部署 Qt DLLs
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAUtils
)
