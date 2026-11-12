# DASignalAnalysisTest 的 CMakeLists.txt
# 覆盖信号分析插件的参数序列化、峰值表模型、STFT 栅格换算与 T7 视图网格/图元化
#
# 白盒测试：直接编译被测源文件（参数结构体 / 峰值表模型 / STFT 栅格 / 信号视图），
# 不链接插件 DLL（插件类未导出内部实现，避免为测试扩大导出面）。
# T7 新增：DASignalAnalysisView 依赖 DAFigure（DAChartWidget/DAFigureWidget/3D 子图），
# 因此显式链接 DAFigure + qwt(plot/plot3d) + Qt OpenGL。

set(_signal_plugin_dir ${CMAKE_CURRENT_LIST_DIR}/../../../plugins/DASignalAnalysis)

# STFT 栅格继承 QwtMatrixRasterData（qwt::core）；视图还用 qwt::plot3d 的 3D 瀑布
# （插件以 PRIVATE 链接 qwt，不会传递到测试）
find_package(qwt CONFIG REQUIRED)

da_add_test(
    NAME DASignalAnalysisTest
    SUITE data
    SOURCES main.cpp
            ${_signal_plugin_dir}/Analysis/DASignalAnalysisParams.cpp
            ${_signal_plugin_dir}/Analysis/DAPeakTableModel.cpp
            ${_signal_plugin_dir}/Page/DAPeakCurvePlotSamples.cpp
            ${_signal_plugin_dir}/Page/DASignalAnalysisView.cpp
            ${_signal_plugin_dir}/Signal/DASignal.cpp
            ${_signal_plugin_dir}/Render/DAStftRasterData.cpp
            ${_signal_plugin_dir}/Render/DAWaterfallBuilder.cpp
            ${_signal_plugin_dir}/Form/DASignalFormSpecBuilder.cpp
    QT_COMPONENTS Test Widgets Core Gui OpenGL
    LINK_PRIVATE
        DAWorkbench::DAFigure
        DAWorkbench::DAUtils
        qwt::plot
        qwt::plot3d
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${_signal_plugin_dir}
        ${CMAKE_SOURCE_DIR}/src/DAFigure
        ${CMAKE_SOURCE_DIR}/src/DAUtils
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAFigure DAWorkbench::DAUtils
)

# qwt（STFT 栅格继承 QwtMatrixRasterData、视图 3D 瀑布）
da_link_3rdparty(DASignalAnalysisTest LIBS qwt INSTALL_DIR "${DAWorkbench_INSTALL_DIR}")
