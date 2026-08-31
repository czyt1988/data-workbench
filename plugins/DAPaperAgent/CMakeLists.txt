cmake_minimum_required(VERSION 3.16)
# DAPaperAgent：论文撰写 agent 插件（内置 agent 提示词 + 文献检索工具）
# 顶层构建：由根 CMakeLists include 工具文件并设置环境
# standalone 构建：da_plugin_bootstrap 完成 project/安装目录/工具文件引导
#
# 本插件是"插件注册内置 agent"机制（DAAgentInterface::registerBuiltinAgent）的
# 参考实现，完整开发指南见 docs/zh/dev-guide/agent/register-builtin-agent-via-plugin.md

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/daworkbench_plugin_utils.cmake)
    da_plugin_bootstrap("DAPaperAgent" "Paper writing agent plugin")
endif()

########################################################
# 文件加载
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp")
file(GLOB DA_PLUGIN_QRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.qrc")

# -------------link DAWorkBench--------------------------
# 顶层构建时 DAWorkbench::* targets 已由 add_subdirectory(src) 创建，无需 find_package；
# standalone 构建时按组件清单查找。
if(NOT TARGET DAWorkbench::DAUtils)
    find_package(DAWorkbench COMPONENTS
        DAAgent
        DAData
        DAInterface
        DAPluginSupport
    )
endif()

da_add_plugin(
    NAME DAPaperAgent
    BUILD_DEFINE DAPAPERAGENT_PLUGIN_BUILD
    SOURCES ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
            ${DA_PLUGIN_QRC_FILES}
    # 文献工具用 QNetworkAccessManager 同步 HTTP（CrossRef/OpenAlex API）
    QT_PRIVATE Core Gui Widgets Network
    # 工具继承 DAAgentToolBase：跨 DLL moc 需 DAAgent PUBLIC；
    # 插件基类 DAAbstractPlugin 在 DAPluginSupport，不经传递，必须显式 PRIVATE
    LINK_PUBLIC DAAgent
    LINK_PRIVATE DAPluginSupport
    # DAAgentToolBase.h → DAData.h → pandas/DAPyDataFrame.h 的 include 链需要
    # python 头（pybind11 经 DAAgent→DAPyBindQt PUBLIC 链传递）
    THIRDPARTY python
    INCLUDE_PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
