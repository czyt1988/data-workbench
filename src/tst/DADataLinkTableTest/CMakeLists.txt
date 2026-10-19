# DADataLinkTableTest 的 CMakeLists.txt
# 回归测试：DAChartOperateWidget::createFigure 与 DADataLinkTableWidget 联动
# 背景：onFigureCreated 曾对 lambda 槽使用 Qt::UniqueConnection，
# Qt Debug 版在 QObject::connect 内触发 Q_ASSERT 断言直接中止进程
#
# 注意：DAGui 链接的第三方 DLL（qwt/ADS/LiteCtk/DAWidgets/quazip/spdlog/python311）
# 不由 windeployqt 部署到测试输出目录（与 DATableDataTest 一致），
# 运行测试时需把第三方安装目录 bin（如 bin_Debug_qt6.7.3_MSVC_x64/bin）加入 PATH

da_add_test(
    NAME DADataLinkTableTest
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAGui
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE ${CMAKE_SOURCE_DIR}/src/DAGui
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAGui
)
