# DADataFrameTest 的 CMakeLists.txt
# 测试 DAPyBindQt 模块的 DAPyDataFrame 功能

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

da_add_test(
    NAME DADataFrameTest
    SOURCES main.cpp
    QT_COMPONENTS Test
    LINK_PRIVATE
        DAWorkbench::DAPyBindQt
        DAWorkbench::DAPyScripts
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
        ${Python3_LIBRARIES}
        pybind11::headers
    INCLUDE_PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src/DAPyBindQt
        ${CMAKE_SOURCE_DIR}/src/DAPyBindQt/pandas
        ${CMAKE_SOURCE_DIR}/src/DAUtils
        ${CMAKE_SOURCE_DIR}/src/DAPyScripts
    # 部署 Qt DLLs 和 Python 依赖
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAPyBindQt
)
