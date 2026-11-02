# DATableSelectionServiceTest 的 CMakeLists.txt
# 测试表格页选区服务（008 期1）：选区→数据源绝对坐标换算的边界口径
# （尾部预留列/隐藏列/冻结窗格/多段选区/重叠选区），覆盖独立上下文与整页两条构造路径

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

da_add_test(
    NAME DATableSelectionServiceTest
    SUITE table
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
