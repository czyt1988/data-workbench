# DATableDataTest 的 CMakeLists.txt
# 测试 DAData 模块的表格数据源抽象层（DATableDataSource/DATableDataBlock/DADataFactory）

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

da_add_test(
    NAME DATableDataTest
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
        ${CMAKE_SOURCE_DIR}/src/DAPyBindQt
        ${CMAKE_SOURCE_DIR}/src/DAPyBindQt/pandas
        ${CMAKE_SOURCE_DIR}/src/DAUtils
        ${CMAKE_SOURCE_DIR}/src/DAPyScripts
    # 部署 Qt DLLs 和依赖库
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAData DAWorkbench::DAGui DAWorkbench::DAPyBindQt DAWorkbench::DAPyScripts
)
