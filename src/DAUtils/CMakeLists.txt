
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAUtils 库

cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# MinGW 7 需要手动链接 filesystem 库
if(MINGW AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
    set(DA_UTILS_EXTRA_SYSTEM_LIBS stdc++fs)
endif()

da_add_library(
    NAME DAUtils
    BUILD_DEFINE DAUTILS_BUILD
    DESCRIPTION "DA Utils Lib | https://github.com/czyt1988"
    SOURCES ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    QT_PUBLIC Core Gui Widgets Xml
    HEADERS ${DA_LIB_HEADER_FILES}
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    target_link_libraries(DAUtils PRIVATE Qt6::Core5Compat)
endif()

if(DA_UTILS_EXTRA_SYSTEM_LIBS)
    target_link_libraries(DAUtils PRIVATE ${DA_UTILS_EXTRA_SYSTEM_LIBS})
endif()
