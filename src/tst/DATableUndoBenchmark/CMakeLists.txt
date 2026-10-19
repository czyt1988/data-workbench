# DATableUndoBenchmark 的 CMakeLists.txt
# 005 T1.21 撤销成本基准：100 万行 × 20 列可写表上四类撤销命令（iat 值记录 /
# DACommandWithTemporaryData 快照对照 / sortRows 置换 / replace 变更清单）的耗时与
# 内存峰值；1 亿行合成惰性源的滑动窗启动成本 + D7 拆窗对照实验（窗口=全量行数，
# 只在测试进程内 setCacheWindowSize，不改主干源码默认值）。
# 测量结果落 docs/zh/dev-guide/general/undo-cost-benchmark.md。

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

da_add_test(
    NAME DATableUndoBenchmark
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

if(WIN32)
    # 内存峰值测量（GetProcessMemoryInfo）
    target_link_libraries(DATableUndoBenchmark PRIVATE psapi)
endif()
