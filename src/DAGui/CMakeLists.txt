
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
set(DA_LIB_HEADER_FILES
    DAAbstractChartAddItemWidget.h
    DAChartAddXYSeriesWidget.h
    DADataManagerComboBox.h
    DADataOperatePageWidget.h
    DAFigureFactory.h
    DAGraphicsPixmapItemSettingWidget.h
    DAGuiAPI.h
    DAChartListView.h
    DAChartManageWidget.h
    DAChartOperateWidget.h
    DADataListView.h
    DADataManageTableView.h
    DADataManageWidget.h
    DADataOperateWidget.h
    DAMessageLogViewWidget.h
    DANodeItemSettingWidget.h
    DANodeLinkItemSettingWidget.h
    DANodeListWidget.h
    DANodeMimeData.h
    DANodeSettingWidget.h
    DANodeTreeWidget.h
    DAToolBox.h
    DAWorkFlowEditWidget.h
    DAWorkFlowGraphicsScene.h
    DAWorkFlowGraphicsView.h
    DAWorkFlowNodeListWidget.h
    DAWorkFlowNodeItemSettingWidget.h
    DAWorkFlowOperateWidget.h
    DASettingContainerWidget.h
    DAXmlHelper.h
)
set(DA_LIB_SOURCE_FILES
    DAAbstractChartAddItemWidget.cpp
    DAChartAddXYSeriesWidget.cpp
    DAChartListView.cpp
    DAChartManageWidget.cpp
    DAChartOperateWidget.cpp
    DADataListView.cpp
    DADataManageTableView.cpp
    DADataManageWidget.cpp
    DADataManagerComboBox.cpp
    DADataOperatePageWidget.cpp
    DADataOperateWidget.cpp
    DAFigureFactory.cpp
    DAGraphicsPixmapItemSettingWidget.cpp
    DAMessageLogViewWidget.cpp
    DANodeItemSettingWidget.cpp
    DANodeLinkItemSettingWidget.cpp
    DANodeListWidget.cpp
    DANodeMimeData.cpp
    DANodeSettingWidget.cpp
    DANodeTreeWidget.cpp
    DAToolBox.cpp
    DAWorkFlowEditWidget.cpp
    DAWorkFlowGraphicsScene.cpp
    DAWorkFlowGraphicsView.cpp
    DAWorkFlowNodeListWidget.cpp
    DAWorkFlowNodeItemSettingWidget.cpp
    DAWorkFlowOperateWidget.cpp
    DASettingContainerWidget.cpp
    DAXmlHelper.cpp
)
set(DA_LIB_QT_UI_FILES
    DAChartAddXYSeriesWidget.ui
    DAChartManageWidget.ui
    DAChartOperateWidget.ui
    DADataManageWidget.ui
    DADataOperateWidget.ui
    DAGraphicsPixmapItemSettingWidget.ui
    DAMessageLogViewWidget.ui
    DANodeItemSettingWidget.ui
    DANodeLinkItemSettingWidget.ui
    DANodeSettingWidget.ui
    DAWorkFlowOperateWidget.ui
    DAWorkFlowEditWidget.ui
    DAWorkFlowNodeListWidget.ui
    DAWorkFlowNodeItemSettingWidget.ui
)
set(DA_LIB_QT_RC_FILES
    icon.qrc
)
if(DA_ENABLE_PYTHON)
    list(APPEND DA_LIB_HEADER_FILES
        DADataframeToVectorPointWidget.h
        DADataOperateOfDataFrameWidget.h
    )
    list(APPEND DA_LIB_HEADER_FILES
        DADataframeToVectorPointWidget.cpp
        DADataOperateOfDataFrameWidget.cpp
    )
    list(APPEND DA_LIB_QT_UI_FILES
        DADataframeToVectorPointWidget.ui
        DADataOperateOfDataFrameWidget.cpp
    )
endif()
# 子目录
set(DA_LIB_SUBDIR_Models ${CMAKE_CURRENT_SOURCE_DIR}/Models)
set(DA_LIB_SUBDIR_Dialog ${CMAKE_CURRENT_SOURCE_DIR}/Dialog)
set(DA_LIB_SUBDIR_MimeData ${CMAKE_CURRENT_SOURCE_DIR}/MimeData)
set(DA_LIB_SUBDIR_Commands ${CMAKE_CURRENT_SOURCE_DIR}/Commands)
set(DA_LIB_SUBDIR_ChartSetting ${CMAKE_CURRENT_SOURCE_DIR}/ChartSetting)

# Models
set(DA_LIB_HEADER_FILES_Models
    ${DA_LIB_SUBDIR_Models}/DADataManagerTreeModel.h
    ${DA_LIB_SUBDIR_Models}/DATreeModel.h
    ${DA_LIB_SUBDIR_Models}/DADataManagerTableModel.h
    ${DA_LIB_SUBDIR_Models}/DAMessageLogsModel.h
    ${DA_LIB_SUBDIR_Models}/DAVariantTableModel.h
)
set(DA_LIB_SOURCE_FILES_Models
    ${DA_LIB_SUBDIR_Models}/DADataManagerTreeModel.cpp
    ${DA_LIB_SUBDIR_Models}/DATreeModel.cpp
    ${DA_LIB_SUBDIR_Models}/DADataManagerTableModel.cpp
    ${DA_LIB_SUBDIR_Models}/DAMessageLogsModel.cpp
    ${DA_LIB_SUBDIR_Models}/DAVariantTableModel.cpp
)
if(DA_ENABLE_PYTHON)
    list(APPEND DA_LIB_HEADER_FILES_Models
        ${DA_LIB_SUBDIR_Models}/DAPySeriesTableModule.h
        ${DA_LIB_SUBDIR_Models}/DAPyDataFrameTableModule.h
    )
    list(APPEND DA_LIB_SOURCE_FILES_Models
        ${DA_LIB_SUBDIR_Models}/DAPySeriesTableModule.cpp
        ${DA_LIB_SUBDIR_Models}/DAPyDataFrameTableModule.cpp
    )
endif()

#Dialog
set(DA_LIB_HEADER_FILES_Dialog
    ${DA_LIB_SUBDIR_Dialog}/DADialogChartGuide.h
    ${DA_LIB_SUBDIR_Dialog}/DARenameColumnsNameDialog.h
)

set(DA_LIB_SOURCE_FILES_Dialog
    ${DA_LIB_SUBDIR_Dialog}/DADialogChartGuide.cpp
    
    ${DA_LIB_SUBDIR_Dialog}/DARenameColumnsNameDialog.cpp
)

set(DA_LIB_QT_UI_FILES_Dialog
    ${DA_LIB_SUBDIR_Dialog}/DADialogChartGuide.ui
    ${DA_LIB_SUBDIR_Dialog}/DARenameColumnsNameDialog.ui
)
if(DA_ENABLE_PYTHON)
    list(APPEND DA_LIB_HEADER_FILES_Dialog
        ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToDatetime.h
        ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToNumeric.h
        ${DA_LIB_SUBDIR_Dialog}/DADialogInsertNewColumn.h
        ${DA_LIB_SUBDIR_Dialog}/DADialogPythonArgs.h
    )
    list(APPEND DA_LIB_SOURCE_FILES_Dialog
        ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToDatetime.cpp
        ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToNumeric.cpp
        ${DA_LIB_SUBDIR_Dialog}/DADialogInsertNewColumn.cpp
        ${DA_LIB_SUBDIR_Dialog}/DADialogPythonArgs.cpp
    )
    list(APPEND DA_LIB_QT_UI_FILES_Dialog
        ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToDatetime.ui
        ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToNumeric.ui
        ${DA_LIB_SUBDIR_Dialog}/DADialogInsertNewColumn.ui
    )
endif()
#MimeData
file(GLOB DA_LIB_HEADER_FILES_MimeData ${DA_LIB_SUBDIR_MimeData}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_MimeData ${DA_LIB_SUBDIR_MimeData}/*.cpp)

#Commands

set(DA_LIB_HEADER_FILES_Commands
    ${DA_LIB_SUBDIR_Commands}/DACommandWithRedoCount.h
    ${DA_LIB_SUBDIR_Commands}/DACommandsForWorkFlow.h
)

set(DA_LIB_SOURCE_FILES_Commands
    ${DA_LIB_SUBDIR_Commands}/DACommandWithRedoCount.cpp
    ${DA_LIB_SUBDIR_Commands}/DACommandsForWorkFlow.cpp
)
if(DA_ENABLE_PYTHON)
    list(APPEND DA_LIB_HEADER_FILES_Commands
        ${DA_LIB_SUBDIR_Commands}/DACommandWithTemplateData.h
        ${DA_LIB_SUBDIR_Commands}/DACommandsDataFrame.h
    )
    list(APPEND DA_LIB_SOURCE_FILES_Commands
        ${DA_LIB_SUBDIR_Commands}/DACommandWithTemplateData.cpp
        ${DA_LIB_SUBDIR_Commands}/DACommandsDataFrame.cpp
    )
endif()
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
target_link_libraries(${DA_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Svg
)
# -------------link SARibbon--------------------------
damacro_import_SARibbonBar(${DA_LIB_NAME})

# -------------link QtAdvancedDockingSystem--------------------------
damacro_import_QtAdvancedDocking(${DA_LIB_NAME})

# -------------link 3rdparty--------------------------
damacro_import_qwt(${DA_LIB_NAME})


# -------------link DAWorkBench--------------------------
# DAUtils
# 指定DAWorkbench_DIR为工程的顶层BINARY_DIR，否则找不到${DA_PROJECT_NAME}
find_package(${DA_PROJECT_NAME} COMPONENTS
    DAUtils
    DAMessageHandler
    DAData
    DACommonWidgets
    DAGraphicsView
    DAWorkFlow
    DAFigure
)
if(${DA_PROJECT_NAME}_FOUND)
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAMessageHandler")
    message(STATUS "  | |-include dir:${DAMessageHandler_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAData")
    message(STATUS "  | |-include dir:${DAData_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DACommonWidgets")
    message(STATUS "  | |-include dir:${DACommonWidgets_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAWorkFlow")
    message(STATUS "  | |-include dir:${DAWorkFlow_INCLUDE_DIR}")
    message(STATUS "  |-link ${DA_PROJECT_NAME}::DAFigure")
    message(STATUS "  | |-include dir:${DAFigure_INCLUDE_DIR}")
endif()
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAUtils
    ${DA_PROJECT_NAME}::DAMessageHandler
    ${DA_PROJECT_NAME}::DAData
    ${DA_PROJECT_NAME}::DACommonWidgets
    ${DA_PROJECT_NAME}::DAGraphicsView
    ${DA_PROJECT_NAME}::DAWorkFlow
    ${DA_PROJECT_NAME}::DAFigure
)


if(DA_ENABLE_PYTHON)
    find_package(${DA_PROJECT_NAME} COMPONENTS
        DAPyScripts
        DAPyCommonWidgets
    )
    if(${DA_PROJECT_NAME}_FOUND)
        message(STATUS "  |-link ${DA_PROJECT_NAME}::DAPyScripts")
        message(STATUS "  | |-include dir:${DAPyScripts_INCLUDE_DIR}")
        message(STATUS "  |-link ${DA_PROJECT_NAME}::DAPyCommonWidgets")
        message(STATUS "  | |-include dir:${DAPyCommonWidgets_INCLUDE_DIR}")
    endif()
    target_link_libraries(${DA_LIB_NAME} PUBLIC
        ${DA_PROJECT_NAME}::DAPyScripts
        ${DA_PROJECT_NAME}::DAPyCommonWidgets
    )
endif()


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
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
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
#Models
install(FILES
    ${DA_LIB_HEADER_FILES_Models}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/Models
    COMPONENT headers
)
#Dialog
install(FILES
    ${DA_LIB_HEADER_FILES_Dialog}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/Dialog
    COMPONENT headers
)
#MimeData
install(FILES
    ${DA_LIB_HEADER_FILES_MimeData}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/MimeData
    COMPONENT headers
)
#Commands
install(FILES
    ${DA_LIB_HEADER_FILES_Commands}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/Commands
    COMPONENT headers
)
#ChartSetting
install(FILES
    ${DA_LIB_HEADER_FILES_ChartSetting}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/ChartSetting
    COMPONENT headers
)
#DAGlobal.h
install(FILES
    ${DA_GLOBAL_HEADER}
    DESTINATION include/${DA_PROJECT_NAME}
    COMPONENT headers
)
