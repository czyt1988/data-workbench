# DAFigurePointerToolTest 的 CMakeLists.txt
# 指针工具（PointerSelector）交互逻辑测试：
# 1. 首次点击非激活子图 = 切换激活 + 选中元素
# 2. 箭头（含 StartLengthAngle 模式）命中与紧凑选中框
# 3. 曲线选中高亮为沿数据采样点路径（itemSelectionPath）而非包围盒矩形

# qwt 头文件（qwt_plot_curve 等）不在 DAFigure 的公开 include 路径里，显式查找
find_package(qwt CONFIG REQUIRED)

da_add_test(
    NAME DAFigurePointerToolTest
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAFigure
        DAWorkbench::DAUtils
        qwt::plot
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
    INCLUDE_PRIVATE
        ${CMAKE_SOURCE_DIR}/src/DAFigure
        ${CMAKE_SOURCE_DIR}/src/DAUtils
    # 部署 Qt DLLs 和依赖库
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAFigure DAWorkbench::DAUtils
)
