﻿
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAWorkFlow"
    "DA WorkFlow Lib | https://github.com/czyt1988"
    0
    0
    1
)

########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core
    Gui
    Widgets
    Xml
    Svg
    REQUIRED
)


########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_LIB_STANDARDNODES_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/StandardNodes/*.h")
file(GLOB DA_LIB_STANDARDNODES_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/StandardNodes/*.cpp")
add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_STANDARDNODES_HEADER_FILES}
            ${DA_LIB_STANDARDNODES_SOURCE_FILES}
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAWORKFLOW_BUILDLIB)#!!!!** 注意变更 **!!!!
########################################################
# 添加预定义宏NOMINMAX,windows系统会定义max和min宏，这个宏和std::max冲突
########################################################
if(WIN32)
    target_compile_definitions(${DA_LIB_NAME} PRIVATE -DNOMINMAX)
endif()

add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# -------------link Qt--------------------------
message(STATUS "${DA_LIB_NAME} Qt${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")
target_link_libraries(${DA_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
)
# -------------link DAWorkbench--------------------------
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到DAWorkbench
find_package(${DA_PROJECT_NAME} COMPONENTS DAUtils DAGraphicsView)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAUtils")
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAGraphicsView")
endif()

target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAUtils
    ${DA_PROJECT_NAME}::DAGraphicsView
)

########################################################
# 第三方库
########################################################
damacro_import_orderedmap(${DA_LIB_NAME})

########################################################
# 设置通用库属性
########################################################
damacro_set_lib_properties(${DA_LIB_NAME} ${DA_LIB_VERSION})



########################################################
# 安装
########################################################
damacro_lib_install()

install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)

