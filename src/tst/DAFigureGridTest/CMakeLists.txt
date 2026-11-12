# DAFigureGridTest 的 CMakeLists.txt
# 保留式网格布局（QwtFigureGrid + DAFigureWidget 转发 API）单元测试：
# 1. enableGridLayout/isGridLayoutEnabled/grid 的启用语义与等分轨道
# 2. 启用时把 qwt figure 的 min/max 尺寸复位为默认值
# 3. setGridCell 的单元格几何（含跨行/跨列）、窗口缩放后比例保持、自由布局与网格成员混用
# 4. 一次拖动会话（多次 sizesChanged + 一次 finishChange）只压入一条撤销命令
# 5. setGridSizes 程序化设置不压入撤销栈
# 6. 网格模式下子图编辑器（SubChartEditor）互斥，非网格 figure 上仍生效
# 7. 非绘图控件通过 addGridItem 分支登记进网格
# 8. 3D 子图在 OpenGL 可用时登记进网格（否则 QSKIP）

# qwt 头文件（qwt_figure.h、qwt_figure_grid.h 等）不在 DAFigure 的公开 include 路径里，显式查找
find_package(qwt CONFIG REQUIRED)

da_add_test(
    NAME DAFigureGridTest
    SUITE chart
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
