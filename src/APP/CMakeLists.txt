
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.10)
damacro_app_setting(
    "DAWorkBench"
    "DAWorkBench App"
    0
    2
    4
)
# 将 App 目标名传递到父作用域，供根 CMakeLists.txt 的翻译块 add_dependencies 使用
set(DA_APP_NAME ${DA_APP_NAME} PARENT_SCOPE)



########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
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

add_executable(${DA_APP_NAME}
            ${DA_APP_HEADER_FILES}
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
            ${DA_GLOBAL_HEADER}
)	

# Windows 平台下设置为窗口应用程序
if(WIN32)
    set_target_properties(${DA_APP_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 构建APP定义的宏
target_compile_definitions(${DA_APP_NAME} PRIVATE QT_DEPRECATED_WARNINGS)#!!!!** 注意变更 **!!!!

# 公共宏
target_compile_definitions(${DA_APP_NAME} PUBLIC
    DA_APP_ORGANIZATION="DA"
    DA_APP_APPLICATION_NAME="WorkBench"
)
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# onTestConnection 用 QNetworkAccessManager——Network 在顶层 find_package 未声明，
# DAGui 子目录里的 find_package(Network) 产生的 IMPORTED 目标对 APP（兄弟子目录）不可见，
# 故 APP 须自带 find_package。
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network REQUIRED)
# -------------link Qt--------------------------
target_link_libraries(${DA_APP_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Network
)
if(WIN32)
    target_link_libraries(${DA_APP_NAME} PUBLIC
        Qt${QT_VERSION_MAJOR}::AxContainer
    )
endif()
# -------------DAWorkbench--------------------------
target_link_libraries(${DA_APP_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAPluginSupport
    ${DA_PROJECT_NAME}::DAAgent
)
# -------------link DAPyWorkFlow--------------------------
target_link_libraries(${DA_APP_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAPyWorkFlow
)

# -------------link 3rdparty DALiteCtk--------------------------
damacro_import_DALiteCtk(${DA_APP_NAME})
# -------------link 3rdparty SARibbon--------------------------
damacro_import_SARibbonBar(${DA_APP_NAME})
# -------------link 3rdparty QtAdvancedDocking--------------------------
damacro_import_QtAdvancedDocking(${DA_APP_NAME})
# -------------link 3rdparty--------------------------
damacro_import_qwt(${DA_APP_NAME})
# -------------link python--------------------------
damacro_import_Python(${DA_APP_NAME})
# -------------link 3rdparty pybind11--------------------------
damacro_import_pybind11(${DA_APP_NAME})



# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${DA_APP_NAME})
endif()
set_target_properties(${DA_APP_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ${DA_APP_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${DA_APP_NAME})
endif()


# -------------link Dbghelp--------------------------
if(WIN32)
    target_link_libraries(${DA_APP_NAME} PRIVATE
        Dbghelp
    )
endif()

########################################################
# app 属性设置
########################################################
damacro_set_app_properties(${DA_APP_NAME} ${DA_APP_VERSION})

########################################################
# 部署第三方库 DLL 到构建输出目录
# APP 是最终可执行文件，依赖所有 DA 模块和第三方库。
# 在 POST_BUILD 阶段把 ${CMAKE_INSTALL_PREFIX}/bin/*.dll 复制到 APP 输出目录，
# 确保开发调试时 DAWidgets/SARibbonBar/qwt/ads/DALiteCtk/quazip/zlib/spdlog 等都能正确加载。
# 同时复制到插件输出目录，确保插件 DLL 也能找到第三方依赖。
########################################################
dafun_deploy_3rdparty_dlls(${DA_APP_NAME}
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

if(WIN32)
    # Debug 模式使用控制台子系统
    target_link_options(${DA_APP_NAME} PRIVATE "$<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>")
    # Release 模式使用 Windows 子系统（无控制台窗口）
    target_link_options(${DA_APP_NAME} PRIVATE "$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>")
endif()
########################################################
# 写入rc信息
########################################################
if(WIN32)

    create_win32_resource_version(
        TARGET ${DA_APP_NAME}
        VERSION ${DA_APP_VERSION}
        COMPANY_NAME "DA"
        COPYRIGHT "Copyright @ 2025 DA All rights reserved."
        DESCRIPTION ${DA_APP_FULL_DESCRIPTION}
        ICONS "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
        FILE_EXTENSION exe
    )
endif()
########################################################
# 安装
########################################################
damacro_app_install()
