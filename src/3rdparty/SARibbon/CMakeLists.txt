
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench 源码目录Cmake文件

cmake_minimum_required(VERSION 3.5)

project(bilud-SARibbon
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : 3rdparty build"
        )

########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)


########################################################
# 第三方库 - SARibbon
########################################################
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../../bin_qt${QT_VERSION}/build_lib/SARibbon")
message(STATUS "3rdparty SARibbon will install to : ${CMAKE_INSTALL_PREFIX}")
# 安装 SARibbon
add_subdirectory(SARibbon)


