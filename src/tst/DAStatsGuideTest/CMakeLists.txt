# DAStatsGuideTest 的 CMakeLists.txt
# 覆盖统计绘图向导参数面板的口径：分组柱状图估计器下拉（itemData 为规范 key、默认 mean、含 min/max）

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

da_add_test(
    NAME DAStatsGuideTest
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAData
        DAWorkbench::DAGui
        DAWorkbench::DAPyBindQt
        DAWorkbench::DAPyScripts
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
        ${Python3_LIBRARIES}
        pybind11::headers
    INCLUDE_PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src/DAData
        ${CMAKE_SOURCE_DIR}/src/DAGui
        ${CMAKE_SOURCE_DIR}/src/DAGui/ChartAddItem
        ${CMAKE_SOURCE_DIR}/src/DAPyBindQt
        ${CMAKE_SOURCE_DIR}/src/DAPyBindQt/pandas
        ${CMAKE_SOURCE_DIR}/src/DAPyScripts
        ${CMAKE_SOURCE_DIR}/src/DAUtils
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAData DAWorkbench::DAGui DAWorkbench::DAPyBindQt DAWorkbench::DAPyScripts
)
