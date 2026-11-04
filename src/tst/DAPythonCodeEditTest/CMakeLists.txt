# DAPythonCodeEditTest 的 CMakeLists.txt
# 测试 Python 通用编辑能力：DAPythonSyntax 词表、DAPythonHighlighter 着色、
# DAPyPythonCompletion 成员补全数据源、DAPythonCodeEdit 弹窗补全与列名注入。

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 PATHS ${DAWorkbench_INSTALL_DIR}/share/cmake)

da_add_test(
    NAME DAPythonCodeEditTest
    SUITE table
    SOURCES main.cpp
    QT_COMPONENTS Test Widgets
    LINK_PRIVATE
        DAWorkbench::DAGui
        DAWorkbench::DAPyBindQt
        DAWorkbench::DAUtils
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
        ${Python3_LIBRARIES}
        pybind11::headers
    INCLUDE_PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src/DAGui
        ${CMAKE_SOURCE_DIR}/src/DAPyBindQt
        ${CMAKE_SOURCE_DIR}/src/DAUtils
    DEPLOY_QT
    DEPLOY_TARGETS DAWorkbench::DAGui DAWorkbench::DAPyBindQt DAWorkbench::DAUtils
)
