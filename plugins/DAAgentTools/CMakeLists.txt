cmake_minimum_required(VERSION 3.16)
# DAAgentTools：平台内置 agent 工具插件（21 工具）
# 顶层构建：由根 CMakeLists include 工具文件并设置环境
# standalone 构建：da_plugin_bootstrap 完成 project/安装目录/工具文件引导

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_plugin_utils.cmake)
    da_plugin_bootstrap("DAAgentTools" "Platform built-in agent tools")
endif()

########################################################
# 文件加载
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp")

# -------------link DAWorkBench--------------------------
# 顶层构建时 DAWorkbench::* targets 已由 add_subdirectory(src) 创建，无需 find_package；
# standalone 构建时按组件清单查找。
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

da_add_plugin(
    NAME DAAgentTools
    BUILD_DEFINE DAAGENTTOOLS_PLUGIN_BUILD
    SOURCES ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
    # 工具不用 Network（save_chart_image 用 QSvgGenerator/QPrinter → Svg/PrintSupport）
    QT_PRIVATE Core Gui Widgets Xml Svg PrintSupport
    # 工具继承 DAAgentToolBase（瘦身版）+ DAAgentChartToolBase（图表方法）。
    # 跨 DLL moc：DAAgentToolBase::staticMetaObject 随 DAAgent.dll 导出（已加 DAAgent_API），
    # 且 DAAgentChartToolBase.h 暴露 DAGui 头（DAChartOperateWidget/DAFigureWidget），
    # 故 DAAgent/DAGui 均 PUBLIC。插件基类 DAAbstractPlugin 在 DAPluginSupport，不经
    # 传递，必须显式 PRIVATE。save_report 用 DAAxOfficeWrapper（Win only）。
    LINK_PUBLIC DAAgent DAGui
    LINK_PRIVATE DAPluginSupport DAAxOfficeWrapper
    # DAAgentChartToolBase.h 的 include 链需要 ads/qwt（DADockingAreaInterface.h →
    # ads_globals.h；DAChartWidget.h → qwt_samples.h），显式导入（INSTALL_DIR 自动指向
    # DAWorkbench 安装目录）；pybind11.h 经 DAAgent→DAPyBindQt PUBLIC 链已传递。
    THIRDPARTY ads qwt python
    # 插件根目录：tools/ 下头文件 #include "DAAgentChartToolBase.h"（在插件根目录），
    # quoted-include 先查当前文件目录再查 -I 路径，须把插件根加入 -I
    INCLUDE_PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
