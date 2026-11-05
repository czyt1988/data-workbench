# DASignalAnalysis：通用时序信号分析插件
# 顶层构建：由根 CMakeLists include 工具文件并设置环境
# standalone 构建：da_plugin_bootstrap 完成 project/安装目录/工具文件引导

cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_plugin_utils.cmake)
    da_plugin_bootstrap("DASignalAnalysis" "General purpose time-series signal analysis plugin")
endif()

########################################################
# 文件加载
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_PLUGIN_PAGE_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Page/*.h")
file(GLOB DA_PLUGIN_PAGE_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Page/*.cpp")
file(GLOB DA_PLUGIN_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

# -------------link DAWorkBench--------------------------
# 顶层构建时 DAWorkbench::* targets 已由 add_subdirectory(src) 创建，无需 find_package；
# standalone 构建时按组件清单查找（DAWorkbench_DIR 由 da_plugin_bootstrap 指向安装目录）。
if(NOT TARGET DAWorkbench::DAUtils)
    find_package(DAWorkbench COMPONENTS
        DAUtils
        DAMessageHandler
        DAFigure
        DAGui
        DAInterface
        DAPluginSupport
    )
endif()

da_add_plugin(
    NAME DASignalAnalysis
    BUILD_DEFINE DASIGNALANALYSIS_PLUGIN_BUILD
    SOURCES ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
            ${DA_PLUGIN_PAGE_HEADER_FILES}
            ${DA_PLUGIN_PAGE_SOURCE_FILES}
            ${DA_PLUGIN_QT_RC_FILES}
    # Multimedia：播放控制（QMediaPlayer）在 M9 接入
    QT_PRIVATE Core Gui Widgets Multimedia
    LINK_PRIVATE DAUtils
                DAMessageHandler
                DAFigure
                DAInterface
                DAPluginSupport
    THIRDPARTY SARibbonBar ctk ads qwt python
    INCLUDE_PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
