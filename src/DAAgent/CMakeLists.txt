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
# Note: file(GLOB) is non-recursive. When plan-05 adds tools/ subdirectory,
# extend the glob pattern to include subdirectories:
# file(GLOB DAAgent_TOOLS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp")
# file(GLOB DAAgent_TOOLS_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.h")
# and append both to add_library(...) + install(FILES ...) (参照 DAFigure 对
# MarkSymbol/、Models/ 子目录的分别 GLOB + 分别 install 的模式，见
# DAFigure/CMakeLists.txt 第 20-23、88-92 行)。或使用 CONFIGURE_DEPENDS 让
# CMake 在文件增减时自动重新 GLOB：
# file(GLOB ... CONFIGURE_DEPENDS)

# WebEngine/WebEngineWidgets/WebChannel 的 find_package 限定在 DAAgent 子目录内，
# 不提升为平台全局依赖（顶层 CMakeLists.txt 的 find_package 不引入 WebEngine）。
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    WebEngine
    WebEngineWidgets
    WebChannel
    Network
)

# Qt 资源（Qt5/Qt6 兼容写法，与 APP 模块一致）
file(GLOB DAAgent_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.qrc")

add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_GLOBAL_HEADER}
            ${DAAgent_RESOURCES}
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
    Qt${QT_VERSION_MAJOR}::WebEngine
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    Qt${QT_VERSION_MAJOR}::WebChannel
    Qt${QT_VERSION_MAJOR}::Network
)

# DPAPI（CryptProtectData/CryptUnprotectData）需要 Crypt32.lib
if(WIN32)
    target_link_libraries(${DA_LIB_NAME} PRIVATE Crypt32)
endif()

# -------------link DAWorkbench 模块（导出命名空间为 DAWorkbench::）--------------------------
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAInterface
    ${DA_PROJECT_NAME}::DAData
    ${DA_PROJECT_NAME}::DAFigure
    ${DA_PROJECT_NAME}::DAPyBindQt
    ${DA_PROJECT_NAME}::DAPyScripts
)

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

# 部署 Python 脚本（后续 plan 添加 agent_runner.py）
# 顶层 CMakeLists.txt 把 src/PyScripts 复制到 ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/PyScripts，
# 故 DAAgent 的 agent 脚本应落到同一根下的 PyScripts/DAWorkbench/agent/。
# file(COPY python/agent_runner.py
#      DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/PyScripts/DAWorkbench/agent/)
