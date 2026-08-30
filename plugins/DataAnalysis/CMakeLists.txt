
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DataAnalysisPlugin 库
# 顶层构建：由根 CMakeLists include 工具文件并设置环境
# standalone 构建：da_plugin_bootstrap 完成 project/安装目录/工具文件引导

cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_plugin_utils.cmake)
    da_plugin_bootstrap("DataAnalysis" "This is the fundamental data analysis plugin of the DA project")
endif()

########################################################
# 文件加载
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_PLUGIN_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_PLUGIN_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")
file(GLOB DA_PLUGIN_DIALOGS_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Dialogs/*.h")
file(GLOB DA_PLUGIN_DIALOGS_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Dialogs/*.cpp")
file(GLOB DA_PLUGIN_DIALOGS_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Dialogs/*.ui")

# -------------link DAWorkBench--------------------------
# 顶层构建时 DAWorkbench::* targets 已由 add_subdirectory(src) 创建，无需 find_package；
# standalone 构建时按组件清单查找（DAWorkbench_DIR 由 da_plugin_bootstrap 指向安装目录）。
# 导出目标 DAWorkbench::DAAxOfficeWrapper 引用 Qt5::AxContainer，须先 find
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core Gui Widgets Xml Svg PrintSupport Concurrent OpenGL REQUIRED)
if(WIN32)
    find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS AxContainer REQUIRED)
endif()
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
if(NOT TARGET DAWorkbench::DAUtils)
    find_package(DAWorkbench COMPONENTS
        DAUtils
        DAMessageHandler
        DAAxOfficeWrapper
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
    NAME DataAnalysis
    BUILD_DEFINE DATAANALYSIS_PLUGIN_BUILD
    SOURCES ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
            ${DA_PLUGIN_QT_UI_FILES}
            ${DA_PLUGIN_QT_RC_FILES}
            #Dialogs
            ${DA_PLUGIN_DIALOGS_HEADER_FILES}
            ${DA_PLUGIN_DIALOGS_SOURCE_FILES}
            ${DA_PLUGIN_DIALOGS_UI_FILES}
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
                DAAxOfficeWrapper
    THIRDPARTY SARibbonBar ctk ads qwt orderedmap python
)

# DADataAnalysisCore — 纯 pandas 核心算法包（Gui 和 Nodes 共享依赖）
install(DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/PyScripts/DADataAnalysisCore
    DESTINATION ${CMAKE_INSTALL_BINDIR}/PyScripts
    PATTERN "__pycache__" EXCLUDE
    PATTERN "*.pyc" EXCLUDE
    PATTERN "*.pyo" EXCLUDE
)

# DADataAnalysisGui — GUI 交互逻辑
install(DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/PyScripts/DADataAnalysisGui
    DESTINATION ${CMAKE_INSTALL_BINDIR}/PyScripts
    PATTERN "__pycache__" EXCLUDE
    PATTERN "*.pyc" EXCLUDE
    PATTERN "*.pyo" EXCLUDE
)

# DADataAnalysisNodes — 工作流节点插件
install(DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/PyScripts/DADataAnalysisNodes
    DESTINATION ${CMAKE_INSTALL_BINDIR}/pyplugins
    PATTERN "__pycache__" EXCLUDE
    PATTERN "*.pyc" EXCLUDE
    PATTERN "*.pyo" EXCLUDE
)

# 构建期复制（用于调试运行）
file(COPY "${CMAKE_CURRENT_LIST_DIR}/PyScripts/DADataAnalysisCore"
     DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/PyScripts
     PATTERN "__pycache__" EXCLUDE
     PATTERN "*.pyc" EXCLUDE
     PATTERN "*.pyo" EXCLUDE)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/PyScripts/DADataAnalysisGui"
     DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/PyScripts
     PATTERN "__pycache__" EXCLUDE
     PATTERN "*.pyc" EXCLUDE
     PATTERN "*.pyo" EXCLUDE)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/PyScripts/DADataAnalysisNodes"
     DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/pyplugins
     PATTERN "__pycache__" EXCLUDE
     PATTERN "*.pyc" EXCLUDE
     PATTERN "*.pyo" EXCLUDE)
