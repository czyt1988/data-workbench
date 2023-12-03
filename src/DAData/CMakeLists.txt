
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAData 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAData"
    "Data Lib For DA | https://github.com/czyt1988"
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
    REQUIRED
)
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DADATA_BUILDLIB)#!!!!** 注意变更 **!!!!
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# -------------link Qt--------------------------
target_link_libraries(${DA_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)
message(STATUS "${DA_LIB_NAME} Qt${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")

# -------------link Python--------------------------
# Python
# 此处要依赖python
# https://zhuanlan.zhihu.com/p/666367728
# https://blog.csdn.net/weixin_40448140/article/details/112005184
# 如果使用的是非系统目录下的 Python 可以通过指定 Python3_ROOT_DIR 改变查找路径
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
if(${Python3_FOUND})
    message(STATUS "  |-find python")
    message(STATUS "  | |-include dir:${Python3_INCLUDE_DIRS}")
    message(STATUS "  | |-libs : ${Python3_LIBRARIES}")
endif()
target_link_libraries(${DA_LIB_NAME} PRIVATE ${Python3_LIBRARIES})
target_include_directories(${DA_LIB_NAME} PUBLIC
    ${Python3_INCLUDE_DIRS}
)

# -------------link pybind11--------------------------
# pybind11
# pybind11是header only
set(pybind11_DIR "${CMAKE_INSTALL_PREFIX}/../pybind11")
#include_directories(${pybind11_DIR}/include)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:../pybind11/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/pybind11/pybind11/include>
)
message(STATUS "  |-pybind11 include: ${pybind11_DIR}/include")

# -------------link pybind11--------------------------
# DAUtils DAPyBindQt DAPyScripts
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到DAWorkbench
find_package(${DA_PROJECT_NAME} COMPONENTS DAUtils DAPyBindQt DAPyScripts)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAUtils")
    message(STATUS "  | |-include dir:${DAUtils_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAPyBindQt")
    message(STATUS "  | |-include dir:${DAPyBindQt_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAPyScripts")
    message(STATUS "  | |-include dir:${DAPyScripts_INCLUDE_DIR}")
endif()
target_link_libraries(${DA_LIB_NAME} PRIVATE
    ${DA_PROJECT_NAME}::DAUtils
    ${DA_PROJECT_NAME}::DAPyScripts
)
# DAData的头文件包含了DAPyBindQt库的内容，因此DAPyBindQt需要公开链接
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAPyBindQt
)
include_directories(${DAPyBindQt_INCLUDE_DIR})


########################################################
# Qt的moc
########################################################
set_target_properties(${DA_LIB_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${DA_LIB_VERSION}
    EXPORT_NAME ${DA_LIB_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

########################################################
# 安装
########################################################
damacro_lib_install()

install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)

#DAGlobal.h
install(FILES
    ${DA_GLOBAL_HEADER}
    DESTINATION include/${DA_PROJECT_NAME}
    COMPONENT headers
)
