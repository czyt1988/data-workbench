
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench 源码目录Cmake文件

cmake_minimum_required(VERSION 3.5)

project(bilud-QtPropertyBrowser
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : 3rdparty build"
        )

########################################################
# 安装路径设置
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(DA_PLATFORM_NAME "x86")
else()
    set(DA_PLATFORM_NAME "x64")
endif()
set(DA_BIN_DIR_NAME bin_qt${QT_VERSION}_${CMAKE_BUILD_TYPE}_${DA_PLATFORM_NAME})

########################################################
# 第三方库 - QtPropertyBrowser
########################################################
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../../${DA_BIN_DIR_NAME}")
message(STATUS "3rdparty QtPropertyBrowser will install to : ${CMAKE_INSTALL_PREFIX}")
# 安装 QtPropertyBrowser
set(QtPropertyBrowser_BUILD_EXAMPLES OFF)
add_subdirectory(QtPropertyBrowser)


