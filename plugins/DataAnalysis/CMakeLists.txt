
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DataAnalysis 插件

cmake_minimum_required(VERSION 3.5)


project(DataAnalysis
        DESCRIPTION "this cmake file is the top cmake file of DataAnalysis"
)

########################################################
# 获取安装目录名称
########################################################

set(DA_MIN_QT_VERSION 5.14)

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(_platform_name "x86")
else()
    set(_platform_name "x64")
endif()

########################################################
# 安装路径设置
########################################################
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
set(DAWorkbench_INSTALL_FOLDER_NAME bin_${CMAKE_BUILD_TYPE}_qt${QT_VERSION}_${CMAKE_CXX_COMPILER_ID}_${_platform_name})
set(DAWorkbench_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/../../${DAWorkbench_INSTALL_FOLDER_NAME}")
set(DAWorkbench_DIR "${DAWorkbench_INSTALL_DIR}/lib/cmake/DAWorkbench")

message(STATUS "DAWorkbench_INSTALL_DIR=${DAWorkbench_INSTALL_DIR}")
message(STATUS "DAWorkbench_DIR=${DAWorkbench_DIR}")

list(APPEND CMAKE_MODULE_PATH ${DAWorkbench_INSTALL_DIR})
list(APPEND CMAKE_MODULE_PATH ${DAWorkbench_DIR})
include(${DAWorkbench_DIR}/daworkbench_plugin_utils.cmake)


set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_DIR})


add_subdirectory(src)
