
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_LIB_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_LIB_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

da_add_library(
    NAME DAPyCommonWidgets
    BUILD_DEFINE DAPYCOMMONWIDGETS_BUILD
    DESCRIPTION "Common Widgets For Python | https://github.com/czyt1988"
    SOURCES ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_QT_UI_FILES}
            ${DA_LIB_QT_RC_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    QT_PUBLIC Core Gui Widgets
    LINK_PUBLIC DAPyBindQt DAMessageHandler
    THIRDPARTY python pybind11
    HEADERS ${DA_LIB_HEADER_FILES}
)
