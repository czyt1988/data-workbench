
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DASystemNodesPlugin 库

cmake_minimum_required(VERSION 3.10)

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
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_plugin_utils.cmake)

set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_DIR})

########################################################
# 定义插件的信息
########################################################
# 定义插件的信息，自动设置安装位置
# 此宏会定义DA_PLUGIN_NAME变量
damacro_plugin_setting(
    "DASystemNodes"
    "Provides system-level workflow nodes for UI interaction, flow control and data display"
    1
    0
    0
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
    REQUIRED
)
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

########################################################
# 文件加载
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(${DA_PLUGIN_NAME} SHARED
            ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
)

# 构建库定义的宏
target_compile_definitions(${DA_PLUGIN_NAME} PRIVATE DASYSTEMNODES_PLUGIN_BUILD)

########################################################
# 依赖链接
########################################################
# -------------link Qt--------------------------
target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

# 以下这些宏由daworkbench_plugin_utils.cmake提供，此文件会在DAWorkBench安装的时候一同安装到lib/cmake目录下
# 只需要把DAWorkbench的cmake文件安装目录(lib/cmake)append到CMAKE_MODULE_PATH即可调用
damacro_import_Python(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})

# -------------link DAWorkBench--------------------------
message(STATUS "DAWorkbench_DIR=${DAWorkbench_DIR}")
if(NOT TARGET DAWorkbench::DAUtils)
    find_package(DAWorkbench COMPONENTS
        DAUtils
        DAPyBindQt
        DAPyWorkFlow
        DAInterface
        DAPluginSupport
    )
endif()

target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
    DAWorkbench::DAUtils
    DAWorkbench::DAPyBindQt
    DAWorkbench::DAPyWorkFlow
    DAWorkbench::DAInterface
    DAWorkbench::DAPluginSupport
)

########################################################
# Qt的moc
########################################################
set_target_properties(${DA_PLUGIN_NAME} PROPERTIES
    AUTOMOC ON
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
include(GNUInstallDirs)

damacro_plugin_install()

# Python 工作流节点包 — 安装到 pyplugins/ 目录供运行时扫描
install(DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/PyScripts/DASystemNodes
    DESTINATION ${CMAKE_INSTALL_BINDIR}/pyplugins
)

# 构建时复制到 build 目录，便于调试
file(COPY "${CMAKE_CURRENT_LIST_DIR}/PyScripts/DASystemNodes"
     DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/pyplugins)
