# DAChartSerializeTest 的 CMakeLists.txt
# 覆盖 DAFigure 图表层的宿主回归（评审报告 B3/B4/H1/H6）：
# - 谱图 Z 轴区间必须始终有效（QwtMatrixRasterData 不推导 Z 区间，无效时渲染空图）；
# - style 里的 vmin/vmax 必须真正压动态范围；
# - 色标条区间/色带序列化往返，且 v2 旧块版本能正确读出 scaleDrawType；
# - 区间高亮（QwtPlotZoneItem）方向往返。

# DAFigure 以 PRIVATE 链接 qwt，测试直接用 Qwt* 类型需显式导入
find_package(qwt CONFIG REQUIRED)

da_add_test(
    NAME DAChartSerializeTest
    SUITE chart
    SOURCES main.cpp
    QT_COMPONENTS Core Widgets Test
    LINK_PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
        DAWorkbench::DAFigure
        DAWorkbench::DAUtils
        qwt::plot
    INCLUDE_PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../DAFigure
        ${CMAKE_CURRENT_LIST_DIR}/../../DAUtils
        ${CMAKE_CURRENT_LIST_DIR}/../../DAShared
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAFigure DAWorkbench::DAUtils
    COPY_DLL_TARGETS qwt::core qwt::plot qwt::plot3d DA::DAWidgets spdlog::spdlog
                     DAWorkbench::DAMessageHandler DAWorkbench::DAPyBindQt
)

da_link_3rdparty(DAChartSerializeTest LIBS qwt INSTALL_DIR "${DAWorkbench_INSTALL_DIR}")
