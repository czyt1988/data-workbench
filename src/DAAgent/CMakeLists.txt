# DAAgent 库 —— 声明式构建（da_add_library）
cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
# Qt 资源（da_agent.qrc：default-agent.md / subagent-explore.md 播种资源）。
# 注意：必须加入 target 源文件 AUTORCC 才会编译——此前 qrc 未登记导致
# DAAgentManager 的 qrc 播种静默失败（子 agent 一期修复）。
file(GLOB DA_LIB_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

da_add_library(
    NAME DAAgent
    BUILD_DEFINE DAAGENT_BUILD
    DESCRIPTION "DA Agent Module | https://github.com/czyt1988"
    SOURCES ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_QT_RC_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    # DAAGENT_BUILD 在编译本库时使 DAAgent_API 展开为 Q_DECL_EXPORT，
    # 使用方（含跨 DLL 派生 DAAbstractAgentTool 的插件）展开为 Q_DECL_IMPORT，
    # 缺少则 MSVC 报 unresolved RTTI/external symbol。
    # PrintSupport(QPrinter)/Svg(QSvgGenerator) 仅工具 .cpp 使用，PRIVATE
    QT_PUBLIC Core Gui Widgets
    QT_PRIVATE PrintSupport Svg
    # plan-04：DAAgent 不再 link GUI/Figure 模块（图表工具搬到 plugins/DAAgentTools）
    LINK_PUBLIC DAInterface DAData DAPyBindQt DAPyScripts
    # DAAxOfficeWrapper(Word COM) 与 DPAPI 加密（CryptProtectData）均为 Windows only
    WIN32_LINK_PRIVATE DAWorkbench::DAAxOfficeWrapper Crypt32
    # DAAgentModule.cpp includes DAPyInterpreter.h(→ pybind11 → Python.h)，
    # DAPyBindQt 以 PRIVATE 导入这两个库不传递 include，故在此显式导入
    THIRDPARTY python pybind11
    HEADERS ${DA_LIB_HEADER_FILES}
)

########################################################
# 系统提示词 markdown 文件
# 源文件 src/DAAgent/system_prompt.md，安装到与 agent_runner.py 同目录
# （bin/PyScripts/DAWorkbench/agent/），供 DAAgentModule::assembleSystemPrompt()
# 运行时读取。镜像根 CMakeLists.txt 对 PyScripts 的双写模式：
# install(...) 用于部署安装，file(COPY ...) 用于从 build 目录直接运行 exe 的调试场景。
########################################################
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/system_prompt.md
        DESTINATION ${CMAKE_INSTALL_BINDIR}/PyScripts/DAWorkbench/agent)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/system_prompt.md
     DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/PyScripts/DAWorkbench/agent)
