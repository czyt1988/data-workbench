
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.5)
damacro_app_setting(
    "DAWorkBench"
    "DAWorkBench App | https://github.com/czyt1988"
    0
    0
    1
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
    Concurrent
    PrintSupport
    AxContainer
    Core5Compat
    REQUIRED
)
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
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

add_executable(${DA_APP_NAME} WIN32
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
# 构建APP定义的宏
target_compile_definitions(${DA_APP_NAME} PRIVATE QT_DEPRECATED_WARNINGS)#!!!!** 注意变更 **!!!!

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
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::AxContainer
)

# -------------link 3rdparty DALiteCtk--------------------------
damacro_import_DALiteCtk(${DA_APP_NAME})
# -------------link 3rdparty SARibbon--------------------------
damacro_import_SARibbonBar(${DA_APP_NAME})
# -------------link 3rdparty QtAdvancedDocking--------------------------
damacro_import_QtAdvancedDocking(${DA_APP_NAME})
# -------------link 3rdparty--------------------------
damacro_import_qwt(${DA_APP_NAME})

# -------------link DAWorkBench--------------------------
# DAPluginSupport
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到${DA_PROJECT_NAME}
find_package(${DA_PROJECT_NAME} COMPONENTS DAPluginSupport)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAPluginSupport")
    message(STATUS "  | |-include dir:${DAPluginSupport_INCLUDE_DIR}")
endif()


target_link_libraries(${DA_APP_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DACommonWidgets
    ${DA_PROJECT_NAME}::DAGui
    ${DA_PROJECT_NAME}::DAInterface
    ${DA_PROJECT_NAME}::DAPluginSupport
)
# -------------link Dbghelp--------------------------
if(WIN32)
    target_link_libraries(${DA_APP_NAME} PRIVATE
        Dbghelp
    )
endif()
########################################################
# Qt的moc
########################################################
set_target_properties(${DA_APP_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${DA_APP_VERSION}
    EXPORT_NAME ${DA_APP_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(MSVC)
    target_compile_options(${DA_APP_NAME} PRIVATE "/utf-8")
    target_compile_definitions(${DA_APP_NAME} PRIVATE "_UNICODE" "UNICODE")
endif()
########################################################
# 安装
########################################################
damacro_app_install(${CMAKE_CURRENT_SOURCE_DIR}/icon.ico)

