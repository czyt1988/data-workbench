# DASignalAnalysisTest 的 CMakeLists.txt
# 覆盖信号分析插件的参数序列化、峰值表模型与 STFT 栅格换算
#
# 白盒测试：直接编译被测源文件（参数结构体 / 峰值表模型 / STFT 栅格），
# 不链接插件 DLL（插件类未导出内部实现，避免为测试扩大导出面）。

set(_signal_plugin_dir ${CMAKE_CURRENT_LIST_DIR}/../../../plugins/DASignalAnalysis)

# STFT 栅格继承 QwtMatrixRasterData（qwt::core），显式导入 qwt 依赖
# （插件以 PRIVATE 链接 qwt，不会传递到测试）
find_package(qwt CONFIG REQUIRED)

da_add_test(
    NAME DASignalAnalysisTest
    SUITE data
    SOURCES main.cpp
            ${_signal_plugin_dir}/Analysis/DASignalAnalysisParams.cpp
            ${_signal_plugin_dir}/Analysis/DAPeakTableModel.cpp
            ${_signal_plugin_dir}/Form/DASignalFormSpecBuilder.cpp
            ${_signal_plugin_dir}/Render/DAStftRasterData.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAUtils
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${_signal_plugin_dir}
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAUtils
)

# qwt（STFT 栅格继承 QwtMatrixRasterData）
da_link_3rdparty(DASignalAnalysisTest LIBS qwt INSTALL_DIR "${DAWorkbench_INSTALL_DIR}")
