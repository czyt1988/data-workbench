
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.5)
damacro_lib_setting(
    "DAFigure"
    "DA Figure Widgets Base On QWT | https://github.com/czyt1988"
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
    PrintSupport
    REQUIRED
)


########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_LIB_HEADER_FILES_MarkSymbol "${CMAKE_CURRENT_SOURCE_DIR}/MarkSymbol/*.h")
file(GLOB DA_LIB_SOURCE_FILES_MarkSymbol "${CMAKE_CURRENT_SOURCE_DIR}/MarkSymbol/*.cpp")
file(GLOB DA_LIB_HEADER_FILES_Models "${CMAKE_CURRENT_SOURCE_DIR}/Models/*.h")
file(GLOB DA_LIB_SOURCE_FILES_Models "${CMAKE_CURRENT_SOURCE_DIR}/Models/*.cpp")
file(GLOB DA_LIB_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_LIB_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

add_library(${DA_LIB_NAME} SHARED
            ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_HEADER_FILES_MarkSymbol}
            ${DA_LIB_SOURCE_FILES_MarkSymbol}
            ${DA_LIB_HEADER_FILES_Models}
            ${DA_LIB_SOURCE_FILES_Models}
            ${DA_LIB_QT_UI_FILES}
            ${DA_LIB_QT_RC_FILES}
            ${DA_GLOBAL_HEADER}
)	
# 构建库定义的宏,注意必须为Private
target_compile_definitions(${DA_LIB_NAME} PRIVATE DAFIGURE_BUILD)#!!!!** 注意变更 **!!!!
add_library(${DA_PROJECT_NAME}::${DA_LIB_NAME} ALIAS ${DA_LIB_NAME})
########################################################
# 依赖链接 #!!!!** 注意变更 **!!!!
########################################################
# -------------link Qt--------------------------
message(STATUS "${DA_LIB_NAME} Qt${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")
target_link_libraries(${DA_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

find_package(${DA_PROJECT_NAME} COMPONENTS DAUtils)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-linked ${DA_PROJECT_NAME}::DAUtils")
endif()
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAUtils
)
# -------------link 3rdparty qwt--------------------------
damacro_import_qwt(${DA_LIB_NAME})

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
install(FILES
    ${DA_LIB_HEADER_FILES_MarkSymbol}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/MarkSymbol
    COMPONENT headers
)

