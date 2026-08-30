
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# {{plugin-base-name}}Plugin 库
# 顶层构建：由根 CMakeLists include 工具文件并设置环境
# standalone 构建：da_plugin_bootstrap 完成 project/安装目录/工具文件引导

cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # standalone 时 include DAWorkbench 安装目录中的工具文件（占位路径由模板系统替换）
    include(${DAWorkbench_DIR}/daworkbench_plugin_utils.cmake)
    da_plugin_bootstrap("{{plugin-base-name}}" "{{plugin-description}}")
endif()

########################################################
# 文件加载
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_PLUGIN_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_PLUGIN_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

# -------------link DAWorkBench--------------------------
# 顶层构建时 DAWorkbench::* targets 已由 add_subdirectory(src) 创建，无需 find_package；
# standalone 构建时按组件清单查找（DAWorkbench_DIR 由 da_plugin_bootstrap 设置）
if(NOT TARGET DAWorkbench::DAUtils)
    find_package(DAWorkbench COMPONENTS
        DAUtils
        DAMessageHandler
        DAPyScripts
        DAPyBindQt
        DAData
        DAGraphicsView
        DAPyWorkFlow
        DAFigure
        DAPyCommonWidgets
        DAGui
        DAInterface
        DAPluginSupport
    )
endif()

da_add_plugin(
    NAME {{plugin-base-name}}
    BUILD_DEFINE {{PLUGIN-BASE-NAME}}_PLUGIN_BUILD
    SOURCES ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
            ${DA_PLUGIN_QT_UI_FILES}
            ${DA_PLUGIN_QT_RC_FILES}
    QT_PRIVATE Core Gui Widgets Xml Svg PrintSupport
    LINK_PRIVATE DAUtils
                DAMessageHandler
                DAPyBindQt
                DAPyScripts
                DAData
                DAGraphicsView
                DAPyWorkFlow
                DAFigure
                DAPyCommonWidgets
                DAGui
                DAInterface
                DAPluginSupport
    THIRDPARTY SARibbonBar ctk ads qwt orderedmap python
)
