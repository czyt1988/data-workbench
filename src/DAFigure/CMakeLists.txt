
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAFigure 库

cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
########################################################
file(GLOB DA_LIB_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DA_LIB_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB DA_LIB_HEADER_FILES_MarkSymbol "${CMAKE_CURRENT_SOURCE_DIR}/MarkSymbol/*.h")
file(GLOB DA_LIB_SOURCE_FILES_MarkSymbol "${CMAKE_CURRENT_SOURCE_DIR}/MarkSymbol/*.cpp")
file(GLOB DA_LIB_HEADER_FILES_Models "${CMAKE_CURRENT_SOURCE_DIR}/Models/*.h")
file(GLOB DA_LIB_SOURCE_FILES_Models "${CMAKE_CURRENT_SOURCE_DIR}/Models/*.cpp")
file(GLOB DA_LIB_QT_UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB DA_LIB_QT_RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

da_add_library(
    NAME DAFigure
    BUILD_DEFINE DAFIGURE_BUILD
    DESCRIPTION "DA Figure Widgets Base On QWT | https://github.com/czyt1988"
    SOURCES ${DA_LIB_HEADER_FILES}
            ${DA_LIB_SOURCE_FILES}
            ${DA_LIB_HEADER_FILES_MarkSymbol}
            ${DA_LIB_SOURCE_FILES_MarkSymbol}
            ${DA_LIB_HEADER_FILES_Models}
            ${DA_LIB_SOURCE_FILES_Models}
            ${DA_LIB_QT_UI_FILES}
            ${DA_LIB_QT_RC_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    QT_PUBLIC Core Gui Widgets PrintSupport Concurrent OpenGL
    LINK_PUBLIC DAPyBindQt
    LINK_PRIVATE DAUtils DAMessageHandler
    # DAWidgets 提供 DAFontEditPannelWidget/DAColorPickerButton 等基础 UI 控件，
    # 用于 DAChartTextEditorPopup 富文本编辑弹窗，仅 .cpp 内部使用（PIMPL），私有链接即可
    THIRDPARTY qwt python pybind11 DAWidgets
    HEADERS_DIRS . MarkSymbol
)
