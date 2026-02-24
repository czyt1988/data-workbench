
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DACommonWidgets"
    "DA Common Widgets | https://github.com/czyt1988"
    0
    0
    2
)

########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_LIB_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_LIB_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
			${DA_LIB_QT_UI_FILES}
			${DA_LIB_QT_RC_FILES}
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏
target_compile_definitions(${DA_LIB_NAME} PRIVATE DACOMMONWIDGETS_BUILD)#!!!!** 注意变更 **!!!!
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})

# -------------link Qt--------------------------
target_link_libraries(${DA_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
)
# -------------link 3rdparty DALiteCtk--------------------------
damacro_import_DALiteCtk(${DA_LIB_NAME})

# -------------link 3rdparty SARibbon--------------------------
damacro_import_SARibbonBar(${DA_LIB_NAME})

damacro_import_QtPropertyBrowser(${DA_LIB_NAME})

# -------------link DAWorkBench--------------------------
target_link_libraries(${DA_LIB_NAME} PRIVATE
    ${DA_PROJECT_NAME}::DAUtils
)

########################################################
# 设置通用库属性
########################################################
damacro_set_lib_properties(${DA_LIB_NAME} ${DA_LIB_VERSION})

########################################################
# 安装
########################################################
damacro_lib_install()

install(FILES
    ${DA_LIB_HEADER_FILES}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}
    COMPONENT headers
)


