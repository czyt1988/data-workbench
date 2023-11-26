
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAAxOfficeWrapper 库

cmake_minimum_required(VERSION 3.5)

set(DA_LIB_NAME DAAxOfficeWrapper)
set(DA_LIB_DESCRIPTION "DAAxOfficeWrapper | office ax container qt的wrapper | https://github.com/czyt1988")

set(DA_LIB_VERSION_MAJOR 0)
set(DA_LIB_VERSION_MINOR 0)
set(DA_LIB_VERSION_PATCH 1)
set(DA_LIB_VERSION "${DA_LIB_VERSION_MAJOR}.${DA_LIB_VERSION_MINOR}.${DA_LIB_VERSION_PATCH}")
set(DA_OUTPUT_DIR ${CMAKE_BINARY_DIR}/build_libs/${DA_LIB_NAME})

project(${DA_LIB_NAME} 
        VERSION ${DA_LIB_VERSION} 
        LANGUAGES CXX
        DESCRIPTION "DA | ${DA_LIB_DESCRIPTION}"
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
    Xml
    Widgets
    AxContainer
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
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../bin_qt${QT_VERSION}/build_lib")
########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")


add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
)	
# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAAXOFFICEWRAPPER_BUILDLIB)
########################################################
# 依赖链接
########################################################
target_link_libraries(${DA_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::AxContainer
)
message(STATUS "${DA_LIB_NAME} Qt${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")

# DAAxOfficeWrapper依赖DAUtils
set(DAUtils_DIR "${CMAKE_INSTALL_PREFIX}/DAUtils/cmake")
find_package(DAUtils CONFIG)
if(${DAUtils_FOUND})
    target_link_libraries(${DA_LIB_NAME} PUBLIC DA::DAUtils)
    message(STATUS "  |-link DA::DAUtils")
    message(STATUS "  |-include dir:${DAUtils_INCLUDE_DIR}")
endif()

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
    ARCHIVE_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}"
)


########################################################
# 安装
########################################################
message(STATUS "${DA_LIB_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")

set(DA_LIB_INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${DA_LIB_NAME}/include")

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
  INSTALL_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${DA_LIB_NAME}/cmake"
  PATH_VARS DA_LIB_INCLUDE_INSTALL_DIR
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}ConfigVersion.cmake"
    DESTINATION ${DA_LIB_NAME}/cmake
)
install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION ${DA_LIB_NAME}/include
    COMPONENT headers
)
install(TARGETS ${DA_LIB_NAME}
    EXPORT "${DA_LIB_NAME}Targets"
    RUNTIME DESTINATION ${DA_LIB_NAME}/bin
    LIBRARY DESTINATION ${DA_LIB_NAME}/lib
    ARCHIVE DESTINATION ${DA_LIB_NAME}/lib
    INCLUDES DESTINATION ${DA_LIB_NAME}/include
)
install(EXPORT "${DA_LIB_NAME}Targets"
    FILE ${DA_LIB_NAME}Targets.cmake
    NAMESPACE DA::
    DESTINATION ${DA_LIB_NAME}/cmake
)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:${DA_LIB_NAME}/include>
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
