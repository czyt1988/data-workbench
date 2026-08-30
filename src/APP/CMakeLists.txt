
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAWorkBench 应用主程序

cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
########################################################
file(GLOB DA_APP_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_APP_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_APP_QT_UI_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.ui)
file(GLOB DA_APP_QT_RC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.qrc)

file(GLOB DA_APP_HEADER_FILES_SettingPages "${CMAKE_CURRENT_SOURCE_DIR}/SettingPages/*.h")
file(GLOB DA_APP_SOURCE_FILES_SettingPages "${CMAKE_CURRENT_SOURCE_DIR}/SettingPages/*.cpp")
file(GLOB DA_APP_QT_UI_FILES_SettingPages ${CMAKE_CURRENT_SOURCE_DIR}/SettingPages/*.ui)
file(GLOB DA_APP_QT_RC_FILES_SettingPages ${CMAKE_CURRENT_SOURCE_DIR}/SettingPages/*.qrc)

file(GLOB DA_APP_HEADER_FILES_Dialog "${CMAKE_CURRENT_SOURCE_DIR}/Dialog/*.h")
file(GLOB DA_APP_SOURCE_FILES_Dialog "${CMAKE_CURRENT_SOURCE_DIR}/Dialog/*.cpp")
file(GLOB DA_APP_QT_UI_FILES_Dialog ${CMAKE_CURRENT_SOURCE_DIR}/Dialog/*.ui)
file(GLOB DA_APP_QT_RC_FILES_Dialog ${CMAKE_CURRENT_SOURCE_DIR}/Dialog/*.qrc)

# python binding
list(APPEND DA_APP_HEADER_FILES
    PythonBinding/DAAppPythonBinding.h
)
list(APPEND DA_APP_SOURCE_FILES
    PythonBinding/DAAppPythonBinding.cpp
)

da_add_executable(
    NAME DAWorkBench
    DESCRIPTION "DAWorkBench App"
    SOURCES ${DA_APP_HEADER_FILES}
            ${DA_APP_SOURCE_FILES}
            ${DA_APP_QT_UI_FILES}
            ${DA_APP_QT_RC_FILES}
            #SettingPages
            ${DA_APP_HEADER_FILES_SettingPages}
            ${DA_APP_SOURCE_FILES_SettingPages}
            ${DA_APP_QT_UI_FILES_SettingPages}
            ${DA_APP_QT_RC_FILES_SettingPages}
            #Dialog
            ${DA_APP_HEADER_FILES_Dialog}
            ${DA_APP_SOURCE_FILES_Dialog}
            ${DA_APP_QT_UI_FILES_Dialog}
            ${DA_APP_QT_RC_FILES_Dialog}
            # Global
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    # onTestConnection 用 QNetworkAccessManager——Network 在顶层 find_package 未声明，
    # DAGui 子目录里的 find_package(Network) 产生的 IMPORTED 目标对 APP（兄弟子目录）不可见，
    # 故 APP 须自带该组件
    QT_PUBLIC Core Gui Widgets Xml Svg Network
    QT_WIN32_PUBLIC AxContainer
    LINK_PUBLIC DAPluginSupport DAAgent DAPyWorkFlow
    THIRDPARTY ctk SARibbonBar ads qwt python pybind11
    COMPILE_DEFINITIONS_PRIVATE QT_DEPRECATED_WARNINGS
    COMPILE_DEFINITIONS_PUBLIC
        DA_APP_ORGANIZATION="DA"
        DA_APP_APPLICATION_NAME="WorkBench"
    WIN32_LINK_PRIVATE Dbghelp
    ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
    COMPANY_NAME "DA"
    COPYRIGHT "Copyright @ 2025 DA All rights reserved."
    CONSOLE_ON_DEBUG
    DEPLOY_3RDPARTY_DLLS
    PLUGIN_DIR "${CMAKE_BINARY_DIR}/bin/plugins/$<CONFIG>"
)

if(WIN32)
    # 在构建输出目录生成 qt.conf，确保从构建目录直接运行时 Qt 能正确找到资源
    # （icudtl.dat、WebEngine .pak 文件等）。
    # 构建目录中可能存在早期 windeployqt 拷贝的局部 Qt5Cored.dll，
    # 若无 qt.conf，Qt 会将前缀路径解析到构建目录而非 Qt 安装目录，
    # 导致 Qt5WebEngine 初始化时找不到资源而崩溃。
    get_target_property(_qmake_exe Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    if(_qmake_exe)
        execute_process(COMMAND ${_qmake_exe} -query QT_INSTALL_PREFIX
            OUTPUT_VARIABLE _qt_install_prefix OUTPUT_STRIP_TRAILING_WHITESPACE)
        file(WRITE ${CMAKE_BINARY_DIR}/bin/qt.conf
            "[Paths]\nPrefix=${_qt_install_prefix}\n")
        message(STATUS "Generated qt.conf -> ${CMAKE_BINARY_DIR}/bin/qt.conf (Prefix=${_qt_install_prefix})")
    endif()
endif()
