
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。

cmake_minimum_required(VERSION 3.5)
project(DAWorkBench-3rdparty-build
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : 3rdparty build"
)
message(STATUS "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")
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

set(DA_BIN_DIR_NAME bin_${CMAKE_BUILD_TYPE}_qt${QT_VERSION}_${CMAKE_CXX_COMPILER_ID}_${DA_PLATFORM_NAME})
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../${DA_BIN_DIR_NAME}")
########################################################
# 第三方库参数设置
########################################################
#spdlog
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "force set SPDLOG_BUILD_EXAMPLE off" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "force set SPDLOG_BUILD_TESTS off" FORCE)
set(SPDLOG_INSTALL ON CACHE BOOL "force set SPDLOG_INSTALL ON" FORCE)
#ADS
set(BUILD_EXAMPLES OFF CACHE BOOL "force set BUILD_EXAMPLES OFF" FORCE)
#SARibbon
set(SARIBBON_BUILD_EXAMPLES OFF CACHE BOOL "force set SARIBBON_BUILD_EXAMPLES off" FORCE)

########################################################
# 安装第三方库
########################################################
add_subdirectory(qwt)

add_subdirectory(SARibbon)

add_subdirectory(ADS)

add_subdirectory(spdlog)
#file(COPY "${CMAKE_CURRENT_LIST_DIR}/spdlog/include/spdlog" DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

# pybind11直接把文件夹复制过去
file(COPY "${CMAKE_CURRENT_LIST_DIR}/pybind11/include/pybind11" DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

add_subdirectory(ctk)

add_subdirectory(QtPropertyBrowser)

add_subdirectory(ordered-map)
########################################################
# install
########################################################
