
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAGui 库

cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
# 目录约定：一级目录 = 领域（Table/Chart/Workflow/Agent/Archive/Common），
# 领域内按角色再分子目录（如 Table/Core、Chart/Setting）。
# 各领域用 GLOB_RECURSE + CONFIGURE_DEPENDS 自动收集，领域内新增文件无需手工登记。
# 根目录只保留 API 头、枚举工具与图标 qrc。
########################################################
set(DA_LIB_HEADER_FILES
    DAGuiAPI.h
    DAGuiEnumStringUtils.h
)
set(DA_LIB_SOURCE_FILES
    DAGuiEnumStringUtils.cpp
)
set(DA_LIB_QT_UI_FILES
)
set(DA_LIB_QT_RC_FILES
    icon.qrc
)

# 领域列表：新增领域时在此追加一级目录名
set(DA_GUI_DOMAINS
    Table
    Chart
    Workflow
    Agent
    Archive
    DataManager
    Common
)

set(DA_LIB_HEADER_FILES_Domain)
set(DA_LIB_SOURCE_FILES_Domain)
set(DA_LIB_QT_UI_FILES_Domain)
set(DA_LIB_QT_RC_FILES_Domain)
foreach(_domain IN LISTS DA_GUI_DOMAINS)
    file(GLOB_RECURSE _domain_headers CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/${_domain}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/${_domain}/*.hpp)
    file(GLOB_RECURSE _domain_sources CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/${_domain}/*.cpp)
    file(GLOB_RECURSE _domain_uis CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/${_domain}/*.ui)
    file(GLOB_RECURSE _domain_rcs CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/${_domain}/*.qrc)
    list(APPEND DA_LIB_HEADER_FILES_Domain ${_domain_headers})
    list(APPEND DA_LIB_SOURCE_FILES_Domain ${_domain_sources})
    list(APPEND DA_LIB_QT_UI_FILES_Domain ${_domain_uis})
    list(APPEND DA_LIB_QT_RC_FILES_Domain ${_domain_rcs})
endforeach()

# 领域内子目录：用于头文件安装（include 统一写 "领域/子目录/头文件.h"）
set(DA_GUI_HEADER_DIRS
    Table
    Table/Core
    Table/Controllers
    Table/Views
    Table/Models
    Table/Metadata
    Table/Ops
    Table/Chart
    Table/Commands
    Table/Dialogs
    Chart
    Chart/Setting
    Chart/Setting3D
    Chart/AddItem
    Workflow
    Workflow/NodeSetting
    Agent
    Archive
    DataManager
    Common
    Common/Settings
    Common/MarkdownView
)

# Agent UI 需要 WebEngine/WebChannel/Network（DAGui/Agent 子目录的 DAAgentDockWidget 等）
# Qt5 的一体化模块 QtWebEngine 在 Qt6 中拆分为 WebEngineCore/WebEngineQuick/WebEngineWidgets，
# 官方 Qt6 安装不再提供 Qt6WebEngine 兼容包，组件名需按大版本区分
if(${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
        WebEngineCore
        WebEngineWidgets
        WebChannel
        Network
    )
    set(DAGUI_WEBENGINE_COMPS WebEngineCore WebEngineWidgets WebChannel Network)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
        WebEngine
        WebEngineWidgets
        WebChannel
        Network
    )
    set(DAGUI_WEBENGINE_COMPS WebEngine WebEngineWidgets WebChannel Network)
endif()

da_add_library(
    NAME DAGui
    BUILD_DEFINE DAGUI_BUILDLIB
    DESCRIPTION "DA GUI | https://github.com/czyt1988"
    SOURCES ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_QT_UI_FILES}
            ${DA_LIB_QT_RC_FILES}
            ${DA_LIB_HEADER_FILES_Domain}
            ${DA_LIB_SOURCE_FILES_Domain}
            ${DA_LIB_QT_UI_FILES_Domain}
            ${DA_LIB_QT_RC_FILES_Domain}
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    QT_PUBLIC Core Gui Widgets Xml Svg ${DAGUI_WEBENGINE_COMPS}
    LINK_PUBLIC DAUtils DAMessageHandler DAData DAPyWorkFlow DAFigure
                DAPyBindQt DAPyScripts DAPyCommonWidgets
    # DAWidgets 提供基础 UI 控件（DAColorPickerButton/DAPenEditWidget/DAPropertyItemWidget 等），
    # DAGui 的头文件暴露 DAWidgets/ADS/qwt 的头（ui 提升与图表接口），需 PUBLIC
    THIRDPARTY_PUBLIC DAWidgets ads qwt quazip
    THIRDPARTY python pybind11 SARibbonBar ctk
    # Chart/Agent 目录内仍有同目录短名 include 与外部短名引用，保留这两个 include 路径；
    # 其余目录统一使用 "领域/子目录/头文件" 全路径（从 DAGui 根解析）
    PUBLIC_SUBDIRS Chart Agent
    # 头文件安装：根 + 各领域子目录
    HEADERS_DIRS ${DA_GUI_HEADER_DIRS}
    HEADERS ${DA_LIB_HEADER_FILES}
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    target_link_libraries(DAGui PRIVATE Qt6::Core5Compat)
endif()

if(WIN32)
    # DAMarkdownExporter 的 docx 分支经 Word COM（DAAxOfficeWrapper）转换，
    # L3→L1 依赖方向合法；仅 Windows 存在（AxContainer 技术），PRIVATE 足够
    # （DAMarkdownExporter.h 不暴露该库头文件）
    target_link_libraries(DAGui PRIVATE DAAxOfficeWrapper)
endif()
