cmake_minimum_required(VERSION 3.16)

########################################################
# 获取安装目录名称（照 DataAnalysis L12-32）
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
# 安装目录名公式与其余插件（DataAnalysis/DASystemNodes）保持一致：
# qt${QT_VERSION}（完整版本号，如 qt5.15.16），且多配置生成器（VS）下
# CMAKE_BUILD_TYPE 为空，用 DA_INSTALL_CONFIG_TYPE 代替
if(CMAKE_CONFIGURATION_TYPES)
    set(_install_config_type "${DA_INSTALL_CONFIG_TYPE}")
    if(NOT _install_config_type)
        set(_install_config_type "Release")
    endif()
else()
    set(_install_config_type "${CMAKE_BUILD_TYPE}")
endif()
set(DAWorkbench_INSTALL_FOLDER_NAME bin_${_install_config_type}_qt${QT_VERSION}_${CMAKE_CXX_COMPILER_ID}_${_platform_name})
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
damacro_plugin_setting(
    "DAAgentTools"
    "Platform built-in agent tools"
    0
    0
    1
    ${DAWorkbench_INSTALL_DIR}
)

########################################################
# Qt
########################################################
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core
    Gui
    Widgets
    Xml
    Svg
    PrintSupport
    REQUIRED
)
if(WIN32)
    find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
        AxContainer
        REQUIRED
    )
endif()
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

########################################################
# 文件加载
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp")

add_library(${DA_PLUGIN_NAME} SHARED
    ${DA_PLUGIN_HEADER_FILES}
    ${DA_PLUGIN_SOURCE_FILES}
)
target_compile_definitions(${DA_PLUGIN_NAME} PRIVATE DAAGENTTOOLS_PLUGIN_BUILD)

# 插件自身的 include 根目录：tools/ 下的工具头文件 #include "DAAgentChartToolBase.h"
# （DAAgentChartToolBase.h 在本插件根目录，非 tools/），quoted-include 先查当前文件目录
# （tools/）再查 -I 路径，须把插件根加入 -I 才能解析。PRIVATE（仅编译本插件源码用，
# 不进 INTERFACE，install(EXPORT) 不校验 PRIVATE include，无导出报错）。
target_include_directories(${DA_PLUGIN_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

########################################################
# 依赖链接
########################################################
# -------------link Qt--------------------------
# 工具不用 Network（save_chart_image 用 QSvgGenerator/QPrinter → Svg/PrintSupport）。
target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

# -------------link DAWorkbench--------------------------
# 顶层构建时 DAWorkbench::* targets 已由 add_subdirectory(src) 创建，无需 find_package；
# standalone 构建时按组件清单查找。字面量 DAWorkbench:: 与 DataAnalysis 一致。
if(NOT TARGET DAWorkbench::DAUtils)
    find_package(DAWorkbench COMPONENTS
        DAAgent
        DAGui
        DAData
        DAInterface
        DAPluginSupport
        DAFigure
        DAAxOfficeWrapper
    )
endif()

# 工具继承 DAAgentToolBase（瘦身版，数据/响应方法）+ DAAgentChartToolBase（图表方法）。
# 跨 DLL moc：DAAgentToolBase::staticMetaObject 随 DAAgent.dll 导出（已加 DAAgent_API），
# 故 DAAgent 需 PUBLIC link（工具头暴露 DAAgentToolBase.h）。
target_link_libraries(${DA_PLUGIN_NAME} PUBLIC
    DAWorkbench::DAAgent
)
# 插件基类 DAAbstractPlugin（DAAbstractPlugin.h + 其 .cpp 符号）在 DAPluginSupport，
# 不经 DAAgent/DAInterface/DAGui 传递（DAAgent CMake L66-73 未 link DAPluginSupport），
# 必须显式 link（参照 DataAnalysis L163）。叶子插件用 PRIVATE。
target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
    DAWorkbench::DAPluginSupport
)
# 图表工具用 DAChartOperateWidget（DAGui）。DAAgent 已不再 link DAGui（plan-04 解耦），
# 插件必须显式 link。PUBLIC 因 DAAgentChartToolBase.h 暴露 DAGui 头
# （DAChartOperateWidget / DAFigureWidget / DAChartWidget）。
# DAFigure 经 DAGui PUBLIC 传递（DAGui CMakeLists L544），插件无需显式 link DAFigure。
target_link_libraries(${DA_PLUGIN_NAME} PUBLIC
    DAWorkbench::DAGui
)
if(WIN32)
    # save_report 工具用 DAAxOfficeWrapper（Word COM，Win only；工具 .cpp 已有 #ifdef Q_OS_WIN 守卫）
    target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
        DAWorkbench::DAAxOfficeWrapper
    )
endif()

# -------------link 3rdparty--------------------------
# DAAgentChartToolBase.h（图表基类，include DAGui 头）的 include 链需要两个三方库：
#   DADockingAreaInterface.h → ads_globals.h        (QtAdvancedDocking)
#   DAChartWidget.h → DAChartDataInterface.h → qwt_samples.h  (qwt)
# 实测：damacro_import_* 宏加的 include 目录不随 DAAgent PUBLIC 传递给下游链接器
# （插件编译报 C1083 ads_globals.h / qwt_samples.h 找不到），故插件须显式导入，
# 照 DataAnalysis L124-127 惯例。pybind11.h 经 DAAgent→DAPyBindQt PUBLIC 链已传递
# （DAPybind11QtCaster.hpp 可正常编译，无需重复导入）。Python3 同照 DataAnalysis L127
# 导入（数据/图表工具经 DAPyDataFrame.h → Python.h）。
damacro_import_QtAdvancedDocking(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_qwt(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_Python(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})

########################################################
# Qt moc / 输出 / 安装
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
include(GNUInstallDirs)
damacro_plugin_install()
