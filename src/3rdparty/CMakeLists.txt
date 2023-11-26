
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DA WorkBench 源码目录Cmake文件

cmake_minimum_required(VERSION 3.5)

set(DA_PROJECT_3RDPARTY "DA_3rdParty")
project(${DA_PROJECT_3RDPARTY}
        LANGUAGES CXX
        DESCRIPTION "DataWorkBench : 3rdparty build"
        )
########################################################
# MSVC设置
########################################################
if(MSVC)
# msvc utf-8
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()



add_subdirectory(da-spdlog)

add_subdirectory(da-Qt-Advanced-Docking-System)
