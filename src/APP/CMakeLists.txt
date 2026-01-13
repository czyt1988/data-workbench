
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.5)
damacro_app_setting(
    "DAWorkBench"
    "DAWorkBench App"
    0
    0
    2
)

########################################################
# Qt
########################################################
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
    Core
    Gui
    Widgets
    Xml
    Svg
    PrintSupport
    REQUIRED
)

if(WIN32)
    find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
        AxContainer
        REQUIRED
    )
endif()

# qt6,引入Core5Compat
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} ${DA_MIN_QT_VERSION} COMPONENTS
        Core5Compat
        REQUIRED
    )
endif()


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

if(DA_ENABLE_PYTHON)
    # python开启时才加载
    list(APPEND DA_APP_HEADER_FILES
        PythonBinding/DAAppPythonBinding.h
    )
    list(APPEND DA_APP_SOURCE_FILES
        PythonBinding/DAAppPythonBinding.cpp
    )
endif()

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
# -------------link Qt--------------------------
target_link_libraries(${DA_APP_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::AxContainer
)
if(WIN32)
    target_link_libraries(${DA_APP_NAME} PUBLIC
        Qt${QT_VERSION_MAJOR}::AxContainer
    )
endif()
# -------------link 3rdparty DALiteCtk--------------------------
damacro_import_DALiteCtk(${DA_APP_NAME})
# -------------link 3rdparty SARibbon--------------------------
damacro_import_SARibbonBar(${DA_APP_NAME})
# -------------link 3rdparty QtAdvancedDocking--------------------------
damacro_import_QtAdvancedDocking(${DA_APP_NAME})
# -------------link 3rdparty--------------------------
damacro_import_qwt(${DA_APP_NAME})
if(DA_ENABLE_PYTHON)
    # -------------link python--------------------------
    damacro_import_Python(${DA_APP_NAME})
    # -------------link 3rdparty pybind11--------------------------
    damacro_import_pybind11(${DA_APP_NAME})
endif()

find_package(${DA_PROJECT_NAME} COMPONENTS
    DAPluginSupport
)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAPluginSupport")
endif()
target_link_libraries(${DA_APP_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAPluginSupport
)

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
