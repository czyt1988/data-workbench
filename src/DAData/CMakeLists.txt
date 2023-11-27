
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAData 库

cmake_minimum_required(VERSION 3.5)

set(DA_LIB_NAME "DAData") #!!!!** 注意变更为实际库名 **!!!!
set(DA_LIB_DESCRIPTION "Data Lib For DA | https://github.com/czyt1988")#!!!!** 注意变更 **!!!!
set(DA_LIB_VERSION_MAJOR 0)
set(DA_LIB_VERSION_MINOR 0)
set(DA_LIB_VERSION_PATCH 1)
set(DA_LIB_VERSION "${DA_LIB_VERSION_MAJOR}.${DA_LIB_VERSION_MINOR}.${DA_LIB_VERSION_PATCH}")
set(DA_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${DA_LIB_NAME})

message("")
message("${DA_LIB_NAME}${DA_LIB_VERSION} | ${DA_LIB_DESCRIPTION}")

project(${DA_LIB_NAME} 
        VERSION ${DA_LIB_VERSION} 
        LANGUAGES CXX
        DESCRIPTION "DAWorkbench | ${DA_LIB_DESCRIPTION}"
)

########################################################
# 通用常规设置
########################################################
# C++标准要求最低C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 编译选项
set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "add a postfix, usually empty on windows")
########################################################
# MSVC设置
########################################################
if(MSVC)
# msvc utf-8
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()
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
# 目录包含
########################################################
# 包含自身目录
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# 上级目录加入include_directories，DAGlobals.h需要
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
# 默认的CMAKE_INSTALL_PREFIX
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../bin_qt${QT_VERSION}/build_lib/DA")
########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
set(DA_GLOBAL_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h)
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DADATA_BUILDLIB)#!!!!** 注意变更 **!!!!
add_library(DAWorkbench::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# Qt库
target_link_libraries(${DA_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)
message(STATUS "${DA_LIB_NAME} Qt${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")

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
include_directories(${Python3_INCLUDE_DIRS})

# pybind11
# pybind11是header only
set(pybind11_DIR "${CMAKE_INSTALL_PREFIX}/../pybind11")
#include_directories(${pybind11_DIR}/include)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:../pybind11/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/pybind11/pybind11/include>
)
message(STATUS "  |-pybind11 include: ${pybind11_DIR}/include")

# DAUtils
# set(DAUtils_DIR "${CMAKE_INSTALL_PREFIX}/lib/cmake")
find_package(DAWorkbench COMPONENTS DAUtils DAPyBindQt DAPyScripts)
if(${DAUtils_FOUND})
    message(STATUS "  |-link DA::DAUtils")
    message(STATUS "  | |-include dir:${DAUtils_INCLUDE_DIR}")
    message(STATUS "  |-link DA::DAPyBindQt")
    message(STATUS "  | |-include dir:${DAPyBindQt_INCLUDE_DIR}")
    message(STATUS "  |-link DA::DAPyScripts")
    message(STATUS "  | |-include dir:${DAPyScripts_INCLUDE_DIR}")
endif()
target_link_libraries(${DA_LIB_NAME} PRIVATE
    DAWorkbench::DAUtils
    DAWorkbench::DAPyBindQt
    DAWorkbench::DAPyScripts
)



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
#    ARCHIVE_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}/lib"
#    LIBRARY_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}/lib"
#    RUNTIME_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}/bin"
)



########################################################
# 安装
########################################################
message(STATUS "${DA_LIB_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")

set(DA_LIB_INCLUDE_INSTALL_DIR
    $<INSTALL_INTERFACE:include/${DA_LIB_NAME}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
# Generate library version info which will generate xxxConfigVersion.cmake,
# the ${PACKAGE_VERSION} is the version defined in project()
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}ConfigVersion.cmake
    VERSION ${DA_LIB_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/${DA_LIB_NAME}Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}Config.cmake"
  INSTALL_DESTINATION lib/cmake
  PATH_VARS DA_LIB_INCLUDE_INSTALL_DIR
)
# 声明导出target的名称
install(TARGETS ${DA_LIB_NAME}
    EXPORT "${DA_LIB_NAME}Targets"
    INCLUDES DESTINATION include/${DA_LIB_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
export(EXPORT ${DA_LIB_NAME}Targets
       FILE ${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}Targets.cmake
       NAMESPACE DAWorkbench::
       )
install(EXPORT "${DA_LIB_NAME}Targets"
    FILE ${DA_LIB_NAME}Targets.cmake # 导出的文件基准名。
    NAMESPACE DAWorkbench::
    DESTINATION lib/cmake
)
install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION include/${DA_LIB_NAME}
    COMPONENT headers
)

#DAGlobal.h
install(FILES
    ${DA_GLOBAL_HEADER}
    DESTINATION .
    COMPONENT headers
)


install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}ConfigVersion.cmake"
    DESTINATION lib/cmake
)

target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_LIB_NAME}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)


########################################################
# dll资源信息
########################################################
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/create_win32_resource_version.cmake)
if(WIN32)
        create_win32_resource_version(
                TARGET ${DA_LIB_NAME}
                FILENAME ${DA_LIB_NAME}
                VERSION ${DA_LIB_VERSION}
                EXT "dll"
                COMPANYNAME "DA"
                COPYRIGHT "czy"
                DESCRIPTION ${DA_LIB_DESCRIPTION}
        )
endif()
