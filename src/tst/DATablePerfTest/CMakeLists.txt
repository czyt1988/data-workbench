# DATablePerfTest 的 CMakeLists.txt
# 表格性能与防劣化用例（10 万 / 100 万行夹具），从 DATableDataTest 拆出。
# 目标名含 Perf，da_add_test 会自动打上 bench 标签并放宽 TIMEOUT 到 600s；
# 默认套件不跑，需要时：.\scripts\test.ps1 -Suite table -Bench 或 ctest -L bench

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

da_add_test(
    NAME DATablePerfTest
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
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAData DAWorkbench::DAGui DAWorkbench::DAPyBindQt DAWorkbench::DAPyScripts
)
