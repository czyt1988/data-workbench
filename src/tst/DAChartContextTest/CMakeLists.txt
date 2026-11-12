# DAChartContextTest 的 CMakeLists.txt
# T6 WP4：图元树目标抽象 + 通用回退图元面板 + 当前子图导出的 chart 域回归测试：
# 1. DAChartManageWidget 的 setFigures/setCurrentFigure/figures() 与清空重建（不依赖 APP）；
# 2. 未注册 rtti 图元的工厂回退（DAChartGenericItemSettingPanel）；
# 3. DAChartCommonItemsSettingWidget::updateUI 在已注册/未注册 rtti 之间切换面板；
# 4. DAFigureWidget::exportChartToFile 的 PNG/SVG/PDF 与 copyChartToClipboard；
# 5. 空 chart/空路径导出返回 false 且不崩溃。

# DAFigure 以 PRIVATE 链接 qwt，测试直接使用 Qwt* 类型需显式导入
find_package(qwt CONFIG REQUIRED)

da_add_test(
    NAME DAChartContextTest
    SUITE chart
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAGui
        DAWorkbench::DAFigure
        DAWorkbench::DAUtils
        qwt::plot
        qwt::plot3d
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/DAGui
        ${CMAKE_SOURCE_DIR}/src/DAFigure
        ${CMAKE_SOURCE_DIR}/src/DAUtils
        ${CMAKE_SOURCE_DIR}/src/DAShared
    # 部署 Qt DLLs 和依赖库（DAGui 的 Qt 传递依赖需显式传给 windeployqt）
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAFigure DAWorkbench::DAGui DAWorkbench::DAUtils
    COPY_DLL_TARGETS qwt::core qwt::plot qwt::plot3d DA::DAWidgets spdlog::spdlog
                     DAWorkbench::DAMessageHandler DAWorkbench::DAPyBindQt
)

da_link_3rdparty(DAChartContextTest LIBS qwt INSTALL_DIR "${DAWorkbench_INSTALL_DIR}")
