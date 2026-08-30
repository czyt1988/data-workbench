
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAGraphicsView 库

cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_LIB_RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

da_add_library(
    NAME DAGraphicsView
    BUILD_DEFINE DAGRAPHICSVIEW_BUILDLIB
    DESCRIPTION "Graphics View Lib | https://github.com/czyt1988"
    SOURCES ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_RESOURCE_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    QT_PUBLIC Core Gui Widgets Xml Svg
    LINK_PRIVATE DAUtils DAMessageHandler
    HEADERS ${DA_LIB_HEADER_FILES}
)
