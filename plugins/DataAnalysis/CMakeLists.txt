
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
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_plugin_utils.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_utils.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_3rdparty.cmake)
    set(DAWorkbench_INSTALL_FOLDER_NAME)
    dafun_set_bin_name(DAWorkbench_INSTALL_FOLDER_NAME)

    if(NOT DEFINED DAWorkbench_INSTALL_PATH)
        set(DAWorkbench_INSTALL_PATH "${CMAKE_CURRENT_LIST_DIR}/../../${DAWorkbench_INSTALL_FOLDER_NAME}")
        message(STATUS "NOT DEFINED DAWorkbench_INSTALL_PATH,set to ${DAWorkbench_INSTALL_PATH}")
    endif()

    set(DAWorkbench_DIR ${DAWorkbench_INSTALL_PATH}/lib/cmake/DAWorkbench)

    set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_DIR})
endif()

add_subdirectory(src)
