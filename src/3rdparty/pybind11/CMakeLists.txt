
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench 源码目录Cmake文件

# 注意，此cmake仅仅做复制操作，无需运行也无法运行，解析完成后，会在bin_qtx.xx.x目录下的build_lib文件夹生成pybind11文件夹

cmake_minimum_required(VERSION 3.5)

project(bilud-pybind11
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : 3rdparty build"
        )

########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)


########################################################
# 第三方库 - pybind11
########################################################
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../../bin_qt${QT_VERSION}/build_lib/pybind11")
#message(STATUS "3rdparty pybind11 will install to : ${CMAKE_INSTALL_PREFIX}")
# 安装pybind11
#add_subdirectory(pybind11)

# pybind11 是header only，因此直接把include文件夹复制过去即可
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/pybind11/include" DESTINATION ${CMAKE_INSTALL_PREFIX})
