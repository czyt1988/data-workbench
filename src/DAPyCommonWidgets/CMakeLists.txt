
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAPyCommonWidgets"
    "Common Widgets For Python | https://github.com/czyt1988"
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
file(GLOB DA_LIB_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_LIB_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_QT_UI_FILES}
            ${DA_LIB_QT_RC_FILES}
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏,必须为Private
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAPYCOMMONWIDGETS_BUILD)#!!!!** 注意变更 **!!!!
# 定义别名让DA::${DA_LIB_NAME}也能获取到
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
target_link_libraries(${DA_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Widgets
)
message(STATUS "${DA_LIB_NAME} Qt${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")

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
# set(pybind11_DIR "${CMAKE_INSTALL_PREFIX}/../pybind11")
#include_directories(${pybind11_DIR}/include)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/pybind11>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/pybind11/pybind11/include>
)
message(STATUS "  |-pybind11 include: ${pybind11_DIR}/include")

# DAPyBindQt
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到DAWorkbench
find_package(${DA_PROJECT_NAME} COMPONENTS DAPyBindQt)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAPyBindQt")
    message(STATUS "  | |-include dir:${DAPyBindQt_INCLUDE_DIR}")
endif()
target_link_libraries(${DA_LIB_NAME} PRIVATE
    ${DA_PROJECT_NAME}::DAPyBindQt
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
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

########################################################
# 目标依赖目录
########################################################
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
)

########################################################
# 安装
########################################################
message(STATUS "${DA_LIB_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")

# DALibConfig.cmake.in中，会让此变量和“${PACKAGE_PREFIX_DIR}/”进行拼接，也就是${PACKAGE_PREFIX_DIR}/@DA_LIB_INCLUDE_INSTALL_DIR@
# PACKAGE_PREFIX_DIR = ${CMAKE_CURRENT_LIST_DIR}/../..
# 最终变为${CMAKE_CURRENT_LIST_DIR}/../../include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
set(DA_LIB_INCLUDE_INSTALL_DIR include/${DA_PROJECT_NAME}/${DA_LIB_NAME})

# Generate library version info which will generate xxxConfigVersion.cmake,
# the ${PACKAGE_VERSION} is the version defined in project()
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}ConfigVersion.cmake
    VERSION ${DA_LIB_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../DALibConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}Config.cmake"
  INSTALL_DESTINATION lib/cmake/${DA_PROJECT_NAME}
  PATH_VARS DA_LIB_INCLUDE_INSTALL_DIR
)

# 声明导出target的名称
install(TARGETS ${DA_LIB_NAME}
    EXPORT "${DA_LIB_NAME}Targets"
    INCLUDES DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(EXPORT "${DA_LIB_NAME}Targets"
    FILE ${DA_LIB_NAME}Targets.cmake # 导出的文件基准名。
    NAMESPACE ${DA_PROJECT_NAME}::
    DESTINATION lib/cmake/${DA_PROJECT_NAME}
)


install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}ConfigVersion.cmake"
    DESTINATION lib/cmake/${DA_PROJECT_NAME}
)
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

export(EXPORT ${DA_LIB_NAME}Targets
       FILE ${CMAKE_CURRENT_BINARY_DIR}/${DA_LIB_NAME}Targets.cmake
       NAMESPACE ${DA_PROJECT_NAME}::
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
