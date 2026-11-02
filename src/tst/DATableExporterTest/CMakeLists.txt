# DATableExporterTest 的 CMakeLists.txt
# 测试 DAGui 的 DATableExporter（表格导出引擎：Tab 文本/Markdown/HTML 三形态、
# 分块取数、样式/条件格式渲染、增量写入与取消）。
# 用内存表格数据源驱动（不初始化 Python 解释器，避开 DLL detach 退出挂死）；
# DAData 头引入 pybind11，需 Python 头与 pybind11::headers（仿 DATableDataTest）。

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)
find_package(spdlog CONFIG QUIET)

da_add_test(
    NAME DATableExporterTest
    SUITE table
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAGui
        DAWorkbench::DAData
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
        ${Python3_LIBRARIES}
        pybind11::headers
    INCLUDE_PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src/DAGui
        ${CMAKE_SOURCE_DIR}/src/DAData
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAGui DAWorkbench::DAData
    COPY_DLL_TARGETS spdlog::spdlog
)
