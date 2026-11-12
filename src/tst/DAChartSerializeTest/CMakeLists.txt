# DAChartSerializeTest 的 CMakeLists.txt
# 覆盖 DAFigure 图表层的宿主回归（评审报告 B3/B4/H1/H6）与 T5 存档增强：
# - 谱图 Z 轴区间必须始终有效（QwtMatrixRasterData 不推导 Z 区间，无效时渲染空图）；
# - style 里的 vmin/vmax 必须真正压动态范围；
# - 色标条区间/色带序列化往返，且 v2 旧块版本能正确读出 scaleDrawType；
# - 区间高亮（QwtPlotZoneItem）方向往返；
# - transient 图元 API 及其不进 chart-data；
# - figure XML 网格 round-trip、旧工程无 grid 兼容；
# - DAChartItemArchive 与 DAZipArchiveTask_ChartItem 的图元存档往返。

# DAFigure 以 PRIVATE 链接 qwt，测试直接用 Qwt* 类型需显式导入
find_package(qwt CONFIG REQUIRED)
# DAXmlHelper.h 会间接拉入 DAPyNode/pybind11 头文件，需要 Python 开发头文件
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

da_add_test(
    NAME DAChartSerializeTest
    SUITE chart
    SOURCES main.cpp
    QT_COMPONENTS Core Widgets Xml Test
    LINK_PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Xml
        Qt${QT_VERSION_MAJOR}::Test
        DAWorkbench::DAFigure
        DAWorkbench::DAGui
        DAWorkbench::DAUtils
        qwt::plot
        qwt::plot3d
        ${Python3_LIBRARIES}
        pybind11::headers
    INCLUDE_PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/../../DAFigure
        ${CMAKE_CURRENT_LIST_DIR}/../../DAGui
        ${CMAKE_CURRENT_LIST_DIR}/../../DAUtils
        ${CMAKE_CURRENT_LIST_DIR}/../../DAShared
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAFigure DAWorkbench::DAGui DAWorkbench::DAUtils
    COPY_DLL_TARGETS qwt::core qwt::plot qwt::plot3d DA::DAWidgets spdlog::spdlog
                     DAWorkbench::DAMessageHandler DAWorkbench::DAPyBindQt
)

da_link_3rdparty(DAChartSerializeTest LIBS qwt INSTALL_DIR "${DAWorkbench_INSTALL_DIR}")
