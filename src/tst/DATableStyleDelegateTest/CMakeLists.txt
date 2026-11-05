# DATableStyleDelegateTest 的 CMakeLists.txt
# 测试 DAGui 的表格样式 delegate 绘制（DATableStyleItemDelegate）：
# 底色「无」不填充、保留控件默认底色，有效底色正常填充，单元格「无」可覆盖列级底色。
# 离屏 QImage 上直接调 paint()，不依赖真实窗口与 Python 解释器。

da_add_test(
    NAME DATableStyleDelegateTest
    SUITE table
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAGui
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${CMAKE_SOURCE_DIR}/src/DAGui
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAGui
)
