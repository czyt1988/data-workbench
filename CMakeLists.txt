
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench 顶层Cmake文件

cmake_minimum_required(VERSION 3.5)

########################################################
# 版本定义
########################################################
set(DA_VERSION_MAJOR 0)
set(DA_VERSION_MINOR 0)
set(DA_VERSION_PATCH 2)
set(DA_VERSION "${DA_VERSION_MAJOR}.${DA_VERSION_MINOR}.${DA_VERSION_PATCH}")
message(STATUS "DA Version is ${DA_VERSION} | DA 版本为${DA_VERSION}")
########################################################
# 定义工程
########################################################
set(DA_PROJECT_NAME "DataWorkBench")

project(${DA_PROJECT_NAME} 
        VERSION ${DA_VERSION} 
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : A Workflow Integration Workbench"
        )


########################################################
# 一些常规设置
########################################################


set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "add a postfix, usually empty on windows")

# c++标准，要求为17
set(CMAKE_CXX_STANDARD 17)
# 强制要求
set(CMAKE_CXX_STANDARD_REQUIRED ON)

########################################################
# MSVC设置
########################################################
if(MSVC)
# msvc utf-8
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    # make sure __cplusplus is defined when using msvc and enable parallel build
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /MP")
endif()

########################################################
# 平台判断
########################################################
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(SARIBBON_PLATFORM "86")
else()
    set(SARIBBON_PLATFORM "64")
endif()

########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

##################################
# install
##################################
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/bin_qt${QT_VERSION}/build_lib/DA")
message(STATUS "${DA_PROJECT_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")

########################################################
# 源码路径
########################################################
add_subdirectory(src)



