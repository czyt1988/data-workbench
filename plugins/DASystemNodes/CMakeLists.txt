
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DASystemNodesPlugin 库
# 顶层构建：由根 CMakeLists include 工具文件并设置环境
# standalone 构建：da_plugin_bootstrap 完成 project/安装目录/工具文件引导

cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_plugin_utils.cmake)
    da_plugin_bootstrap("DASystemNodes" "Provides system-level workflow nodes for UI interaction, flow control and data display")
endif()

########################################################
# 文件加载
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# -------------link DAWorkBench--------------------------
# 顶层构建时 DAWorkbench::* targets 已由 add_subdirectory(src) 创建，无需 find_package；
# standalone 构建时按组件清单查找
if(NOT TARGET DAWorkbench::DAUtils)
    find_package(DAWorkbench COMPONENTS
        DAUtils
        DAPyBindQt
        DAPyWorkFlow
        DAInterface
        DAPluginSupport
    )
endif()

da_add_plugin(
    NAME DASystemNodes
    BUILD_DEFINE DASYSTEMNODES_PLUGIN_BUILD
    SOURCES ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
    QT_PRIVATE Core Gui Widgets
    LINK_PRIVATE DAUtils DAPyBindQt DAPyWorkFlow DAInterface DAPluginSupport
    THIRDPARTY python
)

# Python 工作流节点包 — 安装到 pyplugins/ 目录供运行时扫描
install(DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/PyScripts/DASystemNodes
    DESTINATION ${CMAKE_INSTALL_BINDIR}/pyplugins
    PATTERN "__pycache__" EXCLUDE
    PATTERN "*.pyc" EXCLUDE
    PATTERN "*.pyo" EXCLUDE
)

# 构建时复制到 build 目录，便于调试
file(COPY "${CMAKE_CURRENT_LIST_DIR}/PyScripts/DASystemNodes"
     DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/pyplugins
     PATTERN "__pycache__" EXCLUDE
     PATTERN "*.pyc" EXCLUDE
     PATTERN "*.pyo" EXCLUDE)
