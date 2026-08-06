# DAAgent 库 —— 严格参照 DAFigure/CMakeLists.txt 的宏调用模式
cmake_minimum_required(VERSION 3.10)

# damacro_lib_setting 设定 DA_LIB_NAME / DA_LIB_VERSION / DA_GLOBAL_HEADER、
# C++17、DEBUG_POSTFIX、安装前缀等（见 cmake/daworkbench_utils.cmake）
damacro_lib_setting(
    "DAAgent"
    "DA Agent Module | https://github.com/czyt1988"
    0
    0
    1
)

########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
# tools/ 子目录的源文件和头文件（plan-05 平台内置工具）
file(GLOB DAAgent_TOOLS_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp")
file(GLOB DAAgent_TOOLS_HDRS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.h")

# DAAgent 仅需要 Core/Gui/Widgets/PrintSupport/Svg；WebEngine/WebChannel/Network
# 已随 GUI 文件迁移至 DAGui/Agent，由 DAGui/CMakeLists.txt 引入。
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    PrintSupport
    Svg
)

# Qt 资源（Qt5/Qt6 兼容写法，与 APP 模块一致）

add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DAAgent_TOOLS_HDRS}
            ${DAAgent_TOOLS_SRCS}
            ${DA_GLOBAL_HEADER}
)
# AUTORCC ON 由 damacro_set_lib_properties 设置，会自动编译 .qrc

# 构建库定义的宏（Private），与 DAFigure 的 DAFIGURE_BUILD 对应
# 配合 DAAgentAPI.h：DAAGENT_BUILD 在编译本库时使 DAAgent_API 展开为 Q_DECL_EXPORT，
# 使用方（含 plan-07 跨 DLL 派生 DAAbstractAgentTool 的插件）展开为 Q_DECL_IMPORT，
# 缺少则 MSVC 报 unresolved RTTI/external symbol。
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAAGENT_BUILD)
# 导出命名空间为 DAWorkbench::DAAgent
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})

# -------------link Qt--------------------------
target_link_libraries(${DA_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)
# PrintSupport (QPrinter for PDF export) and Svg (QSvgGenerator for SVG export)
# are used only in tool .cpp files, so PRIVATE.
target_link_libraries(${DA_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Svg
)

# -------------link DAWorkbench 模块（导出命名空间为 DAWorkbench::）--------------------------
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAInterface
    ${DA_PROJECT_NAME}::DAData
    ${DA_PROJECT_NAME}::DAFigure
    ${DA_PROJECT_NAME}::DAPyBindQt
    ${DA_PROJECT_NAME}::DAPyScripts
    ${DA_PROJECT_NAME}::DAGui
)
# DAAxOfficeWrapper (Word COM for docx export) — Windows only, PRIVATE
if(WIN32)
    target_link_libraries(${DA_LIB_NAME} PRIVATE ${DA_PROJECT_NAME}::DAAxOfficeWrapper)
endif()

# DPAPI（CryptProtectData/CryptUnprotectData）——DAAgentModule 内化的 API Key 加密
# （plan-01 加解密内化：从 DAGui/DAAgentSettingsWidget 搬到本模块匿名命名空间静态函数）。
# 仅 Windows 走 DPAPI，PRIVATE link；plan-05 再从 DAGui CMake 移除 Crypt32。
if(WIN32)
    target_link_libraries(${DA_LIB_NAME} PRIVATE Crypt32)
endif()

# DAAgentModule.cpp includes DAPyInterpreter.h (→ pybind11 → Python.h),
# 需要 Python 和 pybind11 的头文件路径。DAPyBindQt 以 PRIVATE 方式导入这两个第三方库，
# 其 include 目录不会传递给 DAAgent，故在此显式导入。
damacro_import_Python(${DA_LIB_NAME})
damacro_import_pybind11(${DA_LIB_NAME})
# tools/DAAgentToolBase.h includes DADockingAreaInterface.h which includes ads_globals.h
# from QtAdvancedDocking. DAInterface imports it but the include dir doesn't propagate
# (PRIVATE), so we must import it here.
damacro_import_QtAdvancedDocking(${DA_LIB_NAME})
# tools/DAAgentToolBase.h includes DAChartWidget.h → DAChartDataInterface.h which includes
# qwt_samples.h. DAFigure imports qwt but the include dir doesn't propagate (PRIVATE).
damacro_import_qwt(${DA_LIB_NAME})

########################################################
# 设置通用库属性（AUTOMOC/AUTOUIC/AUTORCC ON、EXPORT_NAME、
# DEBUG_POSTFIX、VERSION、输出目录，见 damacro_set_lib_properties）
########################################################
damacro_set_lib_properties(${DA_LIB_NAME} ${DA_LIB_VERSION})

########################################################
# 安装
# damacro_lib_install 自动完成（见 daworkbench_utils.cmake）：
#  - Windows .rc 版本资源（create_win32_resource_version）
#  - target_include_directories 的 BUILD_INTERFACE 与
#    INSTALL_INTERFACE:include/DAWorkbench/DAAgent（无需手写）
#  - install(TARGETS ... EXPORT DAWorkbenchTargets ...) 导出到统一导出集
########################################################
damacro_lib_install()

# 安装公开头文件（与 DAFigure 一致）
# DA_LIB_HEADER_FILES 由 file(GLOB *.h) 收集，已包含 DAAgentAPI.h（导出宏头），
# 故 DAAgentAPI.h 随 ${DA_LIB_HEADER_FILES} 一并安装到 include/DAWorkbench/DAAgent/。
install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
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
