
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAGui"
    "DA GUI | https://github.com/czyt1988"
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
    REQUIRED
)
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
set(DA_LIB_SUBDIR_Models ${CMAKE_CURRENT_SOURCE_DIR}/Models)
set(DA_LIB_SUBDIR_Dialog ${CMAKE_CURRENT_SOURCE_DIR}/Dialog)
set(DA_LIB_SUBDIR_MimeData ${CMAKE_CURRENT_SOURCE_DIR}/MimeData)
set(DA_LIB_SUBDIR_Commands ${CMAKE_CURRENT_SOURCE_DIR}/Commands)
set(DA_LIB_SUBDIR_ChartSetting ${CMAKE_CURRENT_SOURCE_DIR}/ChartSetting)

file(GLOB DA_LIB_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB DA_LIB_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB DA_LIB_QT_UI_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.ui)
file(GLOB DA_LIB_QT_RC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.qrc)
#Models
file(GLOB DA_LIB_HEADER_FILES_Models ${DA_LIB_SUBDIR_Models}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_Models ${DA_LIB_SUBDIR_Models}/*.cpp)
#Dialog
file(GLOB DA_LIB_HEADER_FILES_Dialog ${DA_LIB_SUBDIR_Dialog}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_Dialog ${DA_LIB_SUBDIR_Dialog}/*.cpp)
file(GLOB DA_LIB_QT_UI_FILES_Dialog ${DA_LIB_SUBDIR_Dialog}/*.ui)
#MimeData
file(GLOB DA_LIB_HEADER_FILES_MimeData ${DA_LIB_SUBDIR_MimeData}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_MimeData ${DA_LIB_SUBDIR_MimeData}/*.cpp)
#Commands
file(GLOB DA_LIB_HEADER_FILES_Commands ${DA_LIB_SUBDIR_Commands}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_Commands ${DA_LIB_SUBDIR_Commands}/*.cpp)
#ChartSetting
file(GLOB DA_LIB_HEADER_FILES_ChartSetting ${DA_LIB_SUBDIR_ChartSetting}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_ChartSetting ${DA_LIB_SUBDIR_ChartSetting}/*.cpp)

add_library(${DA_LIB_NAME} SHARED
            #Root
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_QT_UI_FILES}
            ${DA_LIB_QT_RC_FILES}
            #Models
            ${DA_LIB_HEADER_FILES_Models}
            ${DA_LIB_SOURCE_FILES_Models}
            #Dialog
            ${DA_LIB_HEADER_FILES_Dialog}
            ${DA_LIB_SOURCE_FILES_Dialog}
            ${DA_LIB_QT_UI_FILES_Dialog}
            #MimeData
            ${DA_LIB_HEADER_FILES_MimeData}
            ${DA_LIB_SOURCE_FILES_MimeData}
            #Commands
            ${DA_LIB_HEADER_FILES_Commands}
            ${DA_LIB_SOURCE_FILES_Commands}
            #ChartSetting
            ${DA_LIB_HEADER_FILES_ChartSetting}
            ${DA_LIB_SOURCE_FILES_ChartSetting}
            #Global
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAGUI_BUILDLIB)#!!!!** 注意变更 **!!!!
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# -------------link Qt--------------------------
target_link_libraries(${DA_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
)
# -------------link SARibbon--------------------------
find_package(SARibbonBar)
if(SARibbonBar_FOUND)
    message(STATUS "  |-link SARibbonBar")
    message(STATUS "  | |-include dir:${SARibbonBar_INCLUDE_DIR}")
endif()
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/SARibbonBar>
    $<INSTALL_INTERFACE:include/SARibbonBar/3rdparty/framelesshelper/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/SARibbon/SARibbon/src/SARibbonBar>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/SARibbon/SARibbon/src/SARibbonBar/3rdparty/framelesshelper/include>
)
target_link_libraries(${DA_LIB_NAME} PUBLIC
    SARibbonBar
)
# -------------link QtAdvancedDockingSystem--------------------------
# 链接ADS
find_package(qt${QT_VERSION_MAJOR}advanceddocking)
if(qt${QT_VERSION_MAJOR}advanceddocking_FOUND)
    message(STATUS "  |-link qt${QT_VERSION_MAJOR}advanceddocking")
    message(STATUS "  | |-include dir:${qt${QT_VERSION_MAJOR}advanceddocking_INCLUDE_DIR}")
endif()
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/qt${QT_VERSION_MAJOR}advanceddocking>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/ADS/ADS/src>
)
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ads::qt${QT_VERSION_MAJOR}advanceddocking
)
# -------------link DAWorkBench--------------------------
# DAUtils
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到${DA_PROJECT_NAME}
find_package(${DA_PROJECT_NAME} COMPONENTS
    DAMessageHandler
    DAData
    DAPyScripts
    DACommonWidgets
    DAPyCommonWidgets
    DAGraphicsView
    DAWorkFlow
    DAFigure
)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAMessageHandler")
    message(STATUS "  | |-include dir:${DAMessageHandler_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAData")
    message(STATUS "  | |-include dir:${DAData_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAPyScripts")
    message(STATUS "  | |-include dir:${DAPyScripts_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DACommonWidgets")
    message(STATUS "  | |-include dir:${DACommonWidgets_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAPyCommonWidgets")
    message(STATUS "  | |-include dir:${DAPyCommonWidgets_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAWorkFlow")
    message(STATUS "  | |-include dir:${DAWorkFlow_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAFigure")
    message(STATUS "  | |-include dir:${DAFigure_INCLUDE_DIR}")
endif()
target_link_libraries(${DA_LIB_NAME} PRIVATE
    ${DA_PROJECT_NAME}::DAMessageHandler
    ${DA_PROJECT_NAME}::DAPyScripts
    ${DA_PROJECT_NAME}::DACommonWidgets
    ${DA_PROJECT_NAME}::DAPyCommonWidgets  
)
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAData
    ${DA_PROJECT_NAME}::DAGraphicsView
    ${DA_PROJECT_NAME}::DAWorkFlow
    ${DA_PROJECT_NAME}::DAFigure
)
########################################################
# Qt的moc
########################################################
set_target_properties(${DA_LIB_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${DA_LIB_VERSION}
    EXPORT_NAME ${DA_LIB_NAME}
#    ARCHIVE_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}/lib"
#    LIBRARY_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}/lib"
#    RUNTIME_OUTPUT_DIRECTORY "${DA_OUTPUT_DIR}/bin"
)

########################################################
# 安装
########################################################
damacro_lib_install()

install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)

#DAGlobal.h
install(FILES
    ${DA_GLOBAL_HEADER}
    DESTINATION include/${DA_PROJECT_NAME}
    COMPONENT headers
)
