
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# {{plugin-base-name}}Plugin 库

cmake_minimum_required(VERSION 3.5)

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
# DA安装路径设置
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

########################################################
# 定义插件的信息
########################################################
# 定义插件的信息，自动设置安装位置
# 此宏会定义DA_PLUGIN_NAME变量
damacro_plugin_setting(
    "{{plugin-base-name}}"
    "{{plugin-description}}"
    0
    0
    1
    ${DAWorkbench_INSTALL_DIR}
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
    PrintSupport
    REQUIRED
)
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_PLUGIN_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_PLUGIN_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")


add_library(${DA_PLUGIN_NAME} SHARED
            ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
            ${DA_PLUGIN_QT_UI_FILES}
            ${DA_PLUGIN_QT_RC_FILES}
)	
# 构建库定义的宏
target_compile_definitions(${DA_PLUGIN_NAME} PRIVATE {{PLUGIN-BASE-NAME}}_PLUGIN_BUILD)#!!!!** 注意变更 **!!!!
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# -------------link Qt--------------------------
target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

# 以下这些宏由daworkbench_plugin_utils.cmake提供，此文件会在DAWorkBench安装的时候一同安装到lib/cmake目录下
# 只需要把DAWorkbench的cmake文件安装目录(lib/cmake)append到CMAKE_MODULE_PATH即可调用
# list(APPEND CMAKE_MODULE_PATH ${DAWorkbench_CMake_DIR})
damacro_import_SARibbonBar(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_DALiteCtk(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtAdvancedDocking(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtPropertyBrowser(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_qwt(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_orderedmap(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_Python(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
# -------------link DAWorkBench--------------------------
message(STATUS "DAWorkbench_DIR=${DAWorkbench_DIR}")
# DAInterface
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到${DA_PROJECT_NAME}
find_package(DAWorkbench COMPONENTS 
    DAUtils 
    DAMessageHandler
    DAAxOfficeWrapper
    DAPyScripts
    DAPyBindQt
    DAData
    DAGraphicsView
    DAWorkFlow
    DAFigure
    DACommonWidgets
    DAPyCommonWidgets
    DAGui 
    DAInterface 
    DAPluginSupport
)
# 
target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
    DAWorkbench::DAUtils 
    DAWorkbench::DAMessageHandler
    DAWorkbench::DAAxOfficeWrapper
    DAWorkbench::DAPyBindQt
    DAWorkbench::DAPyScripts
    DAWorkbench::DAData
    DAWorkbench::DAGraphicsView
    DAWorkbench::DAWorkFlow
    DAWorkbench::DAFigure
    DAWorkbench::DACommonWidgets
    DAWorkbench::DAPyCommonWidgets
    DAWorkbench::DAGui
    DAWorkbench::DAInterface
    DAWorkbench::DAPluginSupport
)

########################################################
# Qt的moc
########################################################
set_target_properties(${DA_PLUGIN_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${DA_PLUGIN_VERSION}
    EXPORT_NAME ${DA_PLUGIN_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/plugins"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/plugins"
)

########################################################
# 安装插件
########################################################
damacro_plugin_install()

