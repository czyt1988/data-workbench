
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.5)



# 定义插件的信息，自动设置安装位置
# 此宏会定义DA_PLUGIN_NAME变量
damacro_plugin_setting(
    "DataAnalysis"
    "Plugin For Data Analysis"
    0
    0
    1
    ${DAWorkbench_INSTALL_DIR}
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
    REQUIRED
)
if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
file(GLOB DA_PLUGIN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_PLUGIN_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_PLUGIN_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

file(GLOB DA_PLUGIN_HEADER_FILES_Dialogs "${CMAKE_CURRENT_SOURCE_DIR}/Dialogs/*.h")
file(GLOB DA_PLUGIN_SOURCE_FILES_Dialogs "${CMAKE_CURRENT_SOURCE_DIR}/Dialogs/*.cpp")
file(GLOB DA_PLUGIN_QT_UI_FILES_Dialogs "${CMAKE_CURRENT_SOURCE_DIR}/Dialogs/*.ui")

add_library(${DA_PLUGIN_NAME} SHARED
            ${DA_PLUGIN_HEADER_FILES}
            ${DA_PLUGIN_SOURCE_FILES}
            ${DA_PLUGIN_QT_UI_FILES}
            ${DA_PLUGIN_QT_RC_FILES}
            #Dialogs
            ${DA_PLUGIN_HEADER_FILES_Dialogs}
            ${DA_PLUGIN_SOURCE_FILES_Dialogs}
            ${DA_PLUGIN_QT_UI_FILES_Dialogs}
)	
# 构建库定义的宏
target_compile_definitions(${DA_PLUGIN_NAME} PRIVATE DADATAANALYSIS_PLUGIN_BUILD)#!!!!** 注意变更 **!!!!
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# -------------link Qt--------------------------
target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

# 以下这些宏由daworkbench_plugin_utils.cmake提供，此文件会在DAWorkBench安装的时候一同安装到lib/cmake目录下
# 只需要把DAWorkbench的cmake文件安装目录(lib/cmake)append到CMAKE_MODULE_PATH即可调用
# list(APPEND CMAKE_MODULE_PATH ${DAWorkbench_CMake_DIR})
damacro_import_SARibbonBar(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_DALiteCtk(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtAdvancedDocking(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_QtPropertyBrowser(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
damacro_import_qwt(${DA_PLUGIN_NAME} ${DAWorkbench_INSTALL_DIR})
# -------------link DAWorkBench--------------------------
message(STATUS "DAWorkbench_DIR=${DAWorkbench_DIR}")
# DAInterface
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到${DA_PROJECT_NAME}
find_package(DAWorkbench COMPONENTS 
    DAUtils 
    DAMessageHandler
    DAAxOfficeWrapper
    DAPyScripts
    DAPyBindQt
    DAData
    DAGraphicsView
    DAWorkFlow
    DAFigure
    DACommonWidgets
    DAPyCommonWidgets
    DAGui 
    DAInterface 
    DAPluginSupport
)
# 
target_link_libraries(${DA_PLUGIN_NAME} PRIVATE
    DAWorkbench::DAUtils 
    DAWorkbench::DAMessageHandler
    DAWorkbench::DAAxOfficeWrapper
    DAWorkbench::DAPyBindQt
    DAWorkbench::DAPyScripts
    DAWorkbench::DAData
    DAWorkbench::DAGraphicsView
    DAWorkbench::DAWorkFlow
    DAWorkbench::DAFigure
    DAWorkbench::DACommonWidgets
    DAWorkbench::DAPyCommonWidgets
    DAWorkbench::DAGui
    DAWorkbench::DAInterface
    DAWorkbench::DAPluginSupport
)

########################################################
# Qt的moc
########################################################
set_target_properties(${DA_PLUGIN_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${DA_PLUGIN_VERSION}
    EXPORT_NAME ${DA_PLUGIN_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${DAWorkbench_INSTALL_DIR}/bin/plugins"
)

########################################################
# 安装插件
########################################################
damacro_plugin_install()



##################################
# 翻译
##################################

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS LinguistTools QUIET)

# 命名规则一定要按照qt的命名规则，否则无法找到xx_zh_CN或者xxx_en_US
set(DA_TS_FILES
    "${CMAKE_CURRENT_LIST_DIR}/${DA_PLUGIN_NAME}_zh_CN.ts"
    "${CMAKE_CURRENT_LIST_DIR}/${DA_PLUGIN_NAME}_en_US.ts"
)
# DA 在 初始化的时候会查找translations和translations_user两个目录的翻译，插件的翻译建议在translations_user中
set(DA_BUILD_TRANSLATIONS_DIR ${DAWorkbench_INSTALL_DIR}/bin/translations_user)

# 让qm文件直接生成到DA_BUILD_TRANSLATIONS_DIR下
set_source_files_properties(${DA_TS_FILES} PROPERTIES OUTPUT_LOCATION ${DA_BUILD_TRANSLATIONS_DIR})

if("${QT_VERSION_MAJOR}" STREQUAL "5")
    qt5_create_translation(DA_QM_FILES ${CMAKE_CURRENT_LIST_DIR} ${DA_TS_FILES} OPTIONS -source-language en_US -no-obsolete)
else()
    qt6_create_translation(DA_QM_FILES ${CMAKE_CURRENT_LIST_DIR} ${DA_TS_FILES} OPTIONS -source-language en_US -no-obsolete)
endif()
# 添加更新翻译的目标
add_custom_target(daplugins_update_translations DEPENDS ${DA_TS_FILES})

# 添加发布翻译的目标
add_custom_target(daplugins_release_translations DEPENDS ${DA_QM_FILES})

# 安装翻译 - 在install上使用
set(CMAKE_INSTALL_PREFIX ${DAWorkbench_INSTALL_DIR})
install(DIRECTORY ${DA_BUILD_TRANSLATIONS_DIR} DESTINATION bin)

