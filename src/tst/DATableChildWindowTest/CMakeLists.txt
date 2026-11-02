# DATableChildWindowTest 的 CMakeLists.txt
# 测试 DAGui 的表格子窗口框架与容器（贴入/夹取/标题栏拖动/手柄缩放/最大化/还原/关闭）

da_add_test(
    NAME DATableChildWindowTest
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAGui
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${CMAKE_SOURCE_DIR}/src/DAGui
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAGui
)
