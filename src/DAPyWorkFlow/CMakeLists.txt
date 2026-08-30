
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyWorkFlow 库

cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_LIB_PYTHONBINDING_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/PythonBinding/*.h")
file(GLOB DA_LIB_PYTHONBINDING_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/PythonBinding/*.cpp")

da_add_library(
    NAME DAPyWorkFlow
    BUILD_DEFINE DAPYWORKFLOW_BUILDLIB
    DESCRIPTION "DA WorkFlow Python Binding Lib | https://github.com/czyt1988"
    SOURCES ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_PYTHONBINDING_HEADER_FILES}
            ${DA_LIB_PYTHONBINDING_SOURCE_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    # NOMINMAX：windows 的 min/max 宏与 std::max 冲突
    COMPILE_DEFINITIONS_PRIVATE NOMINMAX
    QT_PRIVATE Core Gui Widgets
    LINK_PUBLIC DAUtils DAGraphicsView DAPyBindQt DAMessageHandler
    THIRDPARTY python pybind11
    HEADERS_DIRS . PythonBinding
)

if(MSVC)
    target_compile_options(DAPyWorkFlow PRIVATE /bigobj)
endif()
