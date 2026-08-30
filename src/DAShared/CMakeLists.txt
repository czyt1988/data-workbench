
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAShared 库（INTERFACE 纯头文件库）

cmake_minimum_required(VERSION 3.16)

########################################################
# Qt
########################################################
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core
    REQUIRED
)
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()


########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_HPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

da_add_library(
    NAME DAShared
    TYPE INTERFACE
    SOURCES ${DA_LIB_HEADER_FILES} ${DA_LIB_HPP_FILES}
    QT_PUBLIC Core
    HEADERS ${DA_LIB_HEADER_FILES} ${DA_LIB_HPP_FILES}
)
