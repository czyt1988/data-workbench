# DASignalAnalysisTest 的 CMakeLists.txt
# 覆盖信号分析插件的参数序列化、峰值表模型、STFT 栅格换算、T7 视图网格/图元化与 T9 存档往返
#
# 白盒测试：直接编译被测源文件（参数结构体 / 峰值表模型 / STFT 栅格 / 信号视图 / 存档任务 /
# figure 存档助手），不链接插件 DLL（插件类未导出内部实现，避免为测试扩大导出面）。
# T7 新增：DASignalAnalysisView 依赖 DAFigure（DAChartWidget/DAFigureWidget/3D 子图）。
# T9 新增：DASignalArchiveTask / DASignalFigureArchive 依赖 DAGui（DAChartItemArchive /
# DAXmlHelper / DAChartItemsManager）与 Qt Xml；DAXmlHelper.h 会间接拉入 pybind11 头，
# 需要 Python3/pybind11 的 include（写法同 src/tst/DAChartSerializeTest）。

set(_signal_plugin_dir ${CMAKE_CURRENT_LIST_DIR}/../../../plugins/DASignalAnalysis)

# STFT 栅格继承 QwtMatrixRasterData（qwt::core）；视图还用 qwt::plot3d 的 3D 瀑布
# （插件以 PRIVATE 链接 qwt，不会传递到测试）
find_package(qwt CONFIG REQUIRED)
# DAXmlHelper.h 会间接拉入 DAPyNode/pybind11 头文件，需要 Python 开发头文件
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

da_add_test(
    NAME DASignalAnalysisTest
    SUITE data
    SOURCES main.cpp
            ${_signal_plugin_dir}/Analysis/DASignalAnalysisParams.cpp
            ${_signal_plugin_dir}/Analysis/DAPeakTableModel.cpp
            ${_signal_plugin_dir}/Archive/DASignalArchiveTask.cpp
            ${_signal_plugin_dir}/Archive/DASignalFigureArchive.cpp
            ${_signal_plugin_dir}/Dialogs/DAPeakMarkerDisplayDialog.cpp
            ${_signal_plugin_dir}/Page/DAPeakCurvePlotSamples.cpp
            ${_signal_plugin_dir}/Page/DAPeakMarkerLabel.cpp
            ${_signal_plugin_dir}/Page/DAPeakMarkerSamples.cpp
            ${_signal_plugin_dir}/Page/DASignalAnalysisView.cpp
            ${_signal_plugin_dir}/Page/DASignalChartExport.cpp
            ${_signal_plugin_dir}/Page/DASignalTimeText.cpp
            ${_signal_plugin_dir}/Signal/DASignal.cpp
            ${_signal_plugin_dir}/Render/DAStftRasterData.cpp
            ${_signal_plugin_dir}/Render/DAWaterfallBuilder.cpp
            ${_signal_plugin_dir}/Form/DASignalFormSpecBuilder.cpp
    QT_COMPONENTS Test Widgets Core Gui OpenGL Xml
    LINK_PRIVATE
        DAWorkbench::DAFigure
        DAWorkbench::DAGui
        DAWorkbench::DAUtils
        qwt::plot
        qwt::plot3d
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Xml
        Qt${QT_VERSION_MAJOR}::Test
        ${Python3_LIBRARIES}
        pybind11::headers
    INCLUDE_PRIVATE
        ${_signal_plugin_dir}
        ${CMAKE_SOURCE_DIR}/src/DAFigure
        ${CMAKE_SOURCE_DIR}/src/DAGui
        ${CMAKE_SOURCE_DIR}/src/DAUtils
        ${CMAKE_SOURCE_DIR}/src/DAShared
        ${Python3_INCLUDE_DIRS}
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAFigure DAWorkbench::DAGui DAWorkbench::DAUtils
    COPY_DLL_TARGETS qwt::core qwt::plot qwt::plot3d DA::DAWidgets spdlog::spdlog
                     DAWorkbench::DAMessageHandler DAWorkbench::DAPyBindQt
)

# qwt（STFT 栅格继承 QwtMatrixRasterData、视图 3D 瀑布）
da_link_3rdparty(DASignalAnalysisTest LIBS qwt INSTALL_DIR "${DAWorkbench_INSTALL_DIR}")
