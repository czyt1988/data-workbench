# DAChartSeriesDataTest 的 CMakeLists.txt
# 测试图表取数层 DAChartSeriesDataResult：无界面的 xy 取数与校验（含多 y 等长校验）

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

da_add_test(
    NAME DAChartSeriesDataTest
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAGui
        DAWorkbench::DAData
        DAWorkbench::DAPyBindQt
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
        ${Python3_LIBRARIES}
        pybind11::headers
    INCLUDE_PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src/DAGui
        ${CMAKE_SOURCE_DIR}/src/DAData
        ${CMAKE_SOURCE_DIR}/src/DAPyBindQt
        ${CMAKE_SOURCE_DIR}/src/DAPyBindQt/pandas
        ${CMAKE_SOURCE_DIR}/src/DAUtils
    # 部署 Qt DLLs 和依赖库
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAGui DAWorkbench::DAData DAWorkbench::DAPyBindQt
)
