
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAGui 库

cmake_minimum_required(VERSION 3.16)

########################################################
# 文件加载
# 根目录与部分子目录用显式清单（防漏编），简单目录用 file(GLOB)
########################################################
set(DA_LIB_HEADER_FILES
    DAGuiAPI.h
    DAAbstractNodeSettingWidget.h
    DAAbstractOperateWidget.h
    DACacheWindowTableView.h
    DADataManagerComboBox.h
    DADataOperatePageWidget.h
    DADataTableView.h
    DAFigureFactory.h
    DAGraphicsPixmapItemSettingWidget.h
    DADataListView.h
    DADataManageTableView.h
    DADataManageWidget.h
    DADataManagerTreeWidget.h
    DADataOperateWidget.h
    DAGuiEnumStringUtils.h
    DAMessageLogViewWidget.h
    DANodeItemSettingWidget.h
    DANodeLinkItemSettingWidget.h
    DANodeMimeData.h
    DANodeSettingWidget.h
    DANodeTreeWidget.h
    DARecentFilesManager.h
    DAToolBox.h
    DAPyWorkFlowEditWidget.h
    DAPyWorkFlowEditWidgetDockWidget.h
    DAPyWorkFlowGraphicsScene.h
    DAPyWorkFlowGraphicsView.h
    DAPyWorkFlowNodeListWidget.h
    DAPyWorkFlowNodeItemSettingWidget.h
    DAPyWorkFlowOperateWidget.h
    DASettingContainerWidget.h
    DAStatusBarWidget.h
    DAAbstractArchive.h
    DAAbstractArchiveTask.h
    DAZipArchive.h
    DAZipArchiveThreadWrapper.h
    DAZipArchiveTask_ByteArray.h
    DAZipArchiveTask_Xml.h
    DAZipArchiveTask_ArchiveFile.h
    DAZipArchiveTask_ChartItem.h
    DAXmlHelper.h
    DASplashScreen.h
    DAAbstractSettingPage.h
    DAFormEditorRegistry.h
    DAPropertyFormDialog.h
    DAPropertyFormWidget.h
    DAPropertyPanelContainerWidget.h
    DAPropertyPanelWidget.h
    DASettingDialog.h
    DASettingWidget.h
    DATableCellStyle.h
    DATableDisplayFormat.h
    DATableDisplayFormatComboBox.h
    DATableStyleManager.h
    DATableStyleRegistry.h
    DATableStyleItemDelegate.h
)
set(DA_LIB_SOURCE_FILES
    DAAbstractNodeSettingWidget.cpp
    DAAbstractOperateWidget.cpp
    DACacheWindowTableView.cpp
    DADataListView.cpp
    DADataManageTableView.cpp
    DADataManagerTreeWidget.cpp
    DADataManageWidget.cpp
    DADataManagerComboBox.cpp
    DADataOperatePageWidget.cpp
    DADataOperateWidget.cpp
    DADataTableView.cpp
    DAFigureFactory.cpp
    DAGraphicsPixmapItemSettingWidget.cpp
    DAGuiEnumStringUtils.cpp
    DAMessageLogViewWidget.cpp
    DANodeItemSettingWidget.cpp
    DANodeLinkItemSettingWidget.cpp
    DANodeListWidget.cpp
    DANodeMimeData.cpp
    DANodeSettingWidget.cpp
    DANodeTreeWidget.cpp
    DARecentFilesManager.cpp
    DAToolBox.cpp
    DAPyWorkFlowEditWidget.cpp
    DAPyWorkFlowEditWidgetDockWidget.cpp
    DAPyWorkFlowGraphicsScene.cpp
    DAPyWorkFlowGraphicsView.cpp
    DAPyWorkFlowNodeListWidget.cpp
    DAPyWorkFlowNodeItemSettingWidget.cpp
    DAPyWorkFlowOperateWidget.cpp
    DASettingContainerWidget.cpp
    DAStatusBarWidget.cpp
    DAAbstractArchive.cpp
    DAAbstractArchiveTask.cpp
    DAZipArchive.cpp
    DAZipArchiveThreadWrapper.cpp
    DAZipArchiveTask_ByteArray.cpp
    DAZipArchiveTask_Xml.cpp
    DAZipArchiveTask_ArchiveFile.cpp
    DAZipArchiveTask_ChartItem.cpp
    DAXmlHelper.cpp
    DASplashScreen.cpp
    DAAbstractSettingPage.cpp
    DAFormEditorRegistry.cpp
    DAPropertyFormDialog.cpp
    DAPropertyFormWidget.cpp
    DAPropertyPanelContainerWidget.cpp
    DAPropertyPanelWidget.cpp
    DASettingDialog.cpp
    DASettingWidget.cpp
    DATableCellStyle.cpp
    DATableDisplayFormat.cpp
    DATableDisplayFormatComboBox.cpp
    DATableStyleManager.cpp
    DATableStyleRegistry.cpp
    DATableStyleItemDelegate.cpp
)
set(DA_LIB_QT_UI_FILES
    DADataManageWidget.ui
    DADataManagerTreeWidget.ui
    DAGraphicsPixmapItemSettingWidget.ui
    DAMessageLogViewWidget.ui
    DANodeItemSettingWidget.ui
    DANodeLinkItemSettingWidget.ui
    DANodeSettingWidget.ui
    DAPyWorkFlowEditWidget.ui
    DAPyWorkFlowNodeListWidget.ui
    DAPyWorkFlowNodeItemSettingWidget.ui
    DASettingDialog.ui
    DASettingWidget.ui
)
set(DA_LIB_QT_RC_FILES
    icon.qrc
)
# python about
list(APPEND DA_LIB_HEADER_FILES
    DADataframeToVectorPointWidget.h
    DADataOperateOfDataFrameWidget.h
    DAPyDataFrameTableView.h
    DAPySeriesTableView.h
    DAPySeriesListView.h
)
list(APPEND DA_LIB_SOURCE_FILES
    DADataframeToVectorPointWidget.cpp
    DADataOperateOfDataFrameWidget.cpp
    DAPyDataFrameTableView.cpp
    DAPySeriesTableView.cpp
    DAPySeriesListView.cpp
)
list(APPEND DA_LIB_QT_UI_FILES
    DADataframeToVectorPointWidget.ui
    DADataOperateOfDataFrameWidget.ui
)
# 子目录
set(DA_LIB_SUBDIR_Models ${CMAKE_CURRENT_SOURCE_DIR}/Models)
set(DA_LIB_SUBDIR_Dialog ${CMAKE_CURRENT_SOURCE_DIR}/Dialog)
set(DA_LIB_SUBDIR_MimeData ${CMAKE_CURRENT_SOURCE_DIR}/MimeData)
set(DA_LIB_SUBDIR_Commands ${CMAKE_CURRENT_SOURCE_DIR}/Commands)
set(DA_LIB_SUBDIR_ChartSetting ${CMAKE_CURRENT_SOURCE_DIR}/ChartSetting)
set(DA_LIB_SUBDIR_Chart3DSetting ${CMAKE_CURRENT_SOURCE_DIR}/Chart3DSetting)
set(DA_LIB_SUBDIR_NodeSetting ${CMAKE_CURRENT_SOURCE_DIR}/NodeSetting)
set(DA_LIB_SUBDIR_Chart ${CMAKE_CURRENT_SOURCE_DIR}/Chart)
set(DA_LIB_SUBDIR_ChartAddItem ${CMAKE_CURRENT_SOURCE_DIR}/ChartAddItem)
set(DA_LIB_SUBDIR_Agent ${CMAKE_CURRENT_SOURCE_DIR}/Agent)
set(DA_LIB_SUBDIR_MarkdownView ${CMAKE_CURRENT_SOURCE_DIR}/MarkdownView)

# Models
file(GLOB DA_LIB_HEADER_FILES_Models ${DA_LIB_SUBDIR_Models}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_Models ${DA_LIB_SUBDIR_Models}/*.cpp)

#Dialog
set(DA_LIB_HEADER_FILES_Dialog
    ${DA_LIB_SUBDIR_Dialog}/DARenameColumnsNameDialog.h
)
set(DA_LIB_SOURCE_FILES_Dialog
    ${DA_LIB_SUBDIR_Dialog}/DARenameColumnsNameDialog.cpp
)
set(DA_LIB_QT_UI_FILES_Dialog
    ${DA_LIB_SUBDIR_Dialog}/DARenameColumnsNameDialog.ui
)
list(APPEND DA_LIB_HEADER_FILES_Dialog
    ${DA_LIB_SUBDIR_Dialog}/DATxtFileImportDialog.h
    ${DA_LIB_SUBDIR_Dialog}/DADialogChartGuide.h
    ${DA_LIB_SUBDIR_Dialog}/DADialogStatsChartGuide.h
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToDatetime.h
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToNumeric.h
    ${DA_LIB_SUBDIR_Dialog}/DADialogInsertNewColumn.h
    ${DA_LIB_SUBDIR_Dialog}/DADialogPythonArgs.h
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataFrameSeriesSelector.h
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnDescribe.h
    ${DA_LIB_SUBDIR_Dialog}/DADialogAgentSessionManager.h
    ${DA_LIB_SUBDIR_Dialog}/DADialogTableDisplayFormat.h
)
list(APPEND DA_LIB_SOURCE_FILES_Dialog
    ${DA_LIB_SUBDIR_Dialog}/DATxtFileImportDialog.cpp
    ${DA_LIB_SUBDIR_Dialog}/DADialogChartGuide.cpp
    ${DA_LIB_SUBDIR_Dialog}/DADialogStatsChartGuide.cpp
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToDatetime.cpp
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToNumeric.cpp
    ${DA_LIB_SUBDIR_Dialog}/DADialogInsertNewColumn.cpp
    ${DA_LIB_SUBDIR_Dialog}/DADialogPythonArgs.cpp
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataFrameSeriesSelector.cpp
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnDescribe.cpp
    ${DA_LIB_SUBDIR_Dialog}/DADialogAgentSessionManager.cpp
    ${DA_LIB_SUBDIR_Dialog}/DADialogTableDisplayFormat.cpp
)
list(APPEND DA_LIB_QT_UI_FILES_Dialog
    ${DA_LIB_SUBDIR_Dialog}/DATxtFileImportDialog.ui
    ${DA_LIB_SUBDIR_Dialog}/DADialogChartGuide.ui
    ${DA_LIB_SUBDIR_Dialog}/DADialogStatsChartGuide.ui
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToDatetime.ui
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnCastToNumeric.ui
    ${DA_LIB_SUBDIR_Dialog}/DADialogInsertNewColumn.ui
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataFrameSeriesSelector.ui
    ${DA_LIB_SUBDIR_Dialog}/DADialogDataframeColumnDescribe.ui
)
#MimeData
file(GLOB DA_LIB_HEADER_FILES_MimeData ${DA_LIB_SUBDIR_MimeData}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_MimeData ${DA_LIB_SUBDIR_MimeData}/*.cpp)

#Commands
set(DA_LIB_HEADER_FILES_Commands
    ${DA_LIB_SUBDIR_Commands}/DACommandWithRedoCount.h
    ${DA_LIB_SUBDIR_Commands}/DACommandsForWorkFlow.h
    ${DA_LIB_SUBDIR_Commands}/DACommandsTableStyle.h
    ${DA_LIB_SUBDIR_Commands}/DACommandsTableColumnFormat.h
)
set(DA_LIB_SOURCE_FILES_Commands
    ${DA_LIB_SUBDIR_Commands}/DACommandWithRedoCount.cpp
    ${DA_LIB_SUBDIR_Commands}/DACommandsForWorkFlow.cpp
    ${DA_LIB_SUBDIR_Commands}/DACommandsTableStyle.cpp
    ${DA_LIB_SUBDIR_Commands}/DACommandsTableColumnFormat.cpp
)
list(APPEND DA_LIB_HEADER_FILES_Commands
    ${DA_LIB_SUBDIR_Commands}/DACommandWithTemporaryData.h
    ${DA_LIB_SUBDIR_Commands}/DACommandsDataFrame.h
)
list(APPEND DA_LIB_SOURCE_FILES_Commands
    ${DA_LIB_SUBDIR_Commands}/DACommandWithTemporaryData.cpp
    ${DA_LIB_SUBDIR_Commands}/DACommandsDataFrame.cpp
)

#ChartSetting（直接遍历，不需要区分）
file(GLOB DA_LIB_HEADER_FILES_ChartSetting ${DA_LIB_SUBDIR_ChartSetting}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_ChartSetting ${DA_LIB_SUBDIR_ChartSetting}/*.cpp)
file(GLOB DA_LIB_UI_FILES_ChartSetting ${DA_LIB_SUBDIR_ChartSetting}/*.ui)

#Chart3DSetting
file(GLOB DA_LIB_HEADER_FILES_Chart3DSetting ${DA_LIB_SUBDIR_Chart3DSetting}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_Chart3DSetting ${DA_LIB_SUBDIR_Chart3DSetting}/*.cpp)
file(GLOB DA_LIB_UI_FILES_Chart3DSetting ${DA_LIB_SUBDIR_Chart3DSetting}/*.ui)

#NodeSetting
file(GLOB DA_LIB_HEADER_FILES_NodeSetting ${DA_LIB_SUBDIR_NodeSetting}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_NodeSetting ${DA_LIB_SUBDIR_NodeSetting}/*.cpp)

#Chart
set(DA_LIB_HEADER_FILES_Chart
    ${DA_LIB_SUBDIR_Chart}/DAChartOperateWidget.h
    ${DA_LIB_SUBDIR_Chart}/DAChartManageWidget.h
    ${DA_LIB_SUBDIR_Chart}/DAChartSettingWidget.h
    ${DA_LIB_SUBDIR_Chart}/DAChartItemsManager.h
    ${DA_LIB_SUBDIR_Chart}/DAFigureDockWidget.h
    ${DA_LIB_SUBDIR_Chart}/DAFigureDockWidgetTab.h
    ${DA_LIB_SUBDIR_Chart}/DAFigureComponentsFactory.h
)
set(DA_LIB_SOURCE_FILES_Chart
    ${DA_LIB_SUBDIR_Chart}/DAChartOperateWidget.cpp
    ${DA_LIB_SUBDIR_Chart}/DAChartManageWidget.cpp
    ${DA_LIB_SUBDIR_Chart}/DAChartSettingWidget.cpp
    ${DA_LIB_SUBDIR_Chart}/DAChartItemsManager.cpp
    ${DA_LIB_SUBDIR_Chart}/DAFigureDockWidget.cpp
    ${DA_LIB_SUBDIR_Chart}/DAFigureDockWidgetTab.cpp
    ${DA_LIB_SUBDIR_Chart}/DAFigureComponentsFactory.cpp
)
set(DA_LIB_QT_UI_FILES_Chart
    ${DA_LIB_SUBDIR_Chart}/DAChartManageWidget.ui
    ${DA_LIB_SUBDIR_Chart}/DAChartSettingWidget.ui
)

#ChartAddItem
# 非 Python 部分（保持原门控语义：两个基类 .cpp 不依赖 Python）
set(DA_LIB_HEADER_FILES_ChartAddItem
    ${DA_LIB_SUBDIR_ChartAddItem}/DAAbstractChartAddItemWidget.h
    ${DA_LIB_SUBDIR_ChartAddItem}/DAAbstractStatsChartAddWidget.h
)
set(DA_LIB_SOURCE_FILES_ChartAddItem
    ${DA_LIB_SUBDIR_ChartAddItem}/DAAbstractChartAddItemWidget.cpp
    ${DA_LIB_SUBDIR_ChartAddItem}/DAAbstractStatsChartAddWidget.cpp
)
# Python 部分（DAAbstractChart3DAddItemWidget 等）
list(APPEND DA_LIB_HEADER_FILES_ChartAddItem
        ${DA_LIB_SUBDIR_ChartAddItem}/DAAbstractChart3DAddItemWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartSeriesPickerWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartSeriesSelectWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddXYSeriesWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddXYESeriesWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddtGridRasterDataWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddOHLCSeriesWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddCurveWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddBarWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddIntervalCurveWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddTradingCurveWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddErrorBarWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddBoxChartWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddMultiBarWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddHistogramWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddContourWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddVectorFieldWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddSpectrogramWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAdd3DSurfaceWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAdd3DBarWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAdd3DLineWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsHistplotWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsKdeplot1dWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsKdeplot2dWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsBoxplotWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsHeatmapWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsScatterplotWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsBarplotWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsRegplotWidget.h
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsEcdfplotWidget.h
    )
    list(APPEND DA_LIB_SOURCE_FILES_ChartAddItem
        ${DA_LIB_SUBDIR_ChartAddItem}/DAAbstractChart3DAddItemWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartSeriesPickerWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartSeriesSelectWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddXYSeriesWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddXYESeriesWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddtGridRasterDataWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddOHLCSeriesWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddCurveWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddBarWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddIntervalCurveWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddSpectrogramWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddTradingCurveWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddErrorBarWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddBoxChartWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddMultiBarWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddHistogramWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddContourWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddVectorFieldWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAdd3DSurfaceWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAdd3DBarWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAdd3DLineWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsHistplotWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsKdeplot1dWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsKdeplot2dWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsBoxplotWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsHeatmapWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsScatterplotWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsBarplotWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsRegplotWidget.cpp
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsEcdfplotWidget.cpp
    )
    list(APPEND DA_LIB_QT_UI_FILES_ChartAddItem
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartSeriesPickerWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartSeriesSelectWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddXYSeriesWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddXYESeriesWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddOHLCSeriesWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddtGridRasterDataWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddBoxChartWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddMultiBarWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddHistogramWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddContourWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddVectorFieldWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAdd3DSurfaceWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAdd3DBarWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAdd3DLineWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsHistplotWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsKdeplot1dWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsKdeplot2dWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsBoxplotWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsHeatmapWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsScatterplotWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsBarplotWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsRegplotWidget.ui
        ${DA_LIB_SUBDIR_ChartAddItem}/DAChartAddStatsEcdfplotWidget.ui
        # 注：DAChartAddBarWidget / DAChartAddErrorBarWidget / DAChartAddIntervalCurveWidget /
        #     DAChartAddTradingCurveWidget 复用基类 .ui，无独立 .ui 文件
        # 注：DAChartAddCurveWidget.ui / DAChartAddSpectrogramWidget.ui 存在于磁盘但当前未纳入
        #     构建（其 .cpp 未 include 对应 ui_ 头），保持原有构建行为不纳入 CMake UI 列表
    )

#Agent
file(GLOB DA_LIB_HEADER_FILES_Agent ${DA_LIB_SUBDIR_Agent}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_Agent ${DA_LIB_SUBDIR_Agent}/*.cpp)
file(GLOB DA_LIB_RC_FILES_Agent ${DA_LIB_SUBDIR_Agent}/*.qrc ${DA_LIB_SUBDIR_Agent}/resources/*.qrc)

#MarkdownView
file(GLOB DA_LIB_HEADER_FILES_MarkdownView ${DA_LIB_SUBDIR_MarkdownView}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_MarkdownView ${DA_LIB_SUBDIR_MarkdownView}/*.cpp)
file(GLOB DA_LIB_RC_FILES_MarkdownView ${DA_LIB_SUBDIR_MarkdownView}/*.qrc ${DA_LIB_SUBDIR_MarkdownView}/resources/*.qrc)

# Agent UI 需要 WebEngine/WebChannel/Network（DAGui/Agent 子目录的 DAAgentDockWidget 等）
# Qt5 的一体化模块 QtWebEngine 在 Qt6 中拆分为 WebEngineCore/WebEngineQuick/WebEngineWidgets，
# 官方 Qt6 安装不再提供 Qt6WebEngine 兼容包，组件名需按大版本区分
if(${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
        WebEngineCore
        WebEngineWidgets
        WebChannel
        Network
    )
    set(DAGUI_WEBENGINE_COMPS WebEngineCore WebEngineWidgets WebChannel Network)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
        WebEngine
        WebEngineWidgets
        WebChannel
        Network
    )
    set(DAGUI_WEBENGINE_COMPS WebEngine WebEngineWidgets WebChannel Network)
endif()

da_add_library(
    NAME DAGui
    BUILD_DEFINE DAGUI_BUILDLIB
    DESCRIPTION "DA GUI | https://github.com/czyt1988"
    SOURCES ${DA_LIB_HEADER_FILES}
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
            ${DA_LIB_UI_FILES_ChartSetting}
            #Chart3DSetting
            ${DA_LIB_HEADER_FILES_Chart3DSetting}
            ${DA_LIB_SOURCE_FILES_Chart3DSetting}
            ${DA_LIB_UI_FILES_Chart3DSetting}
            #NodeSetting
            ${DA_LIB_HEADER_FILES_NodeSetting}
            ${DA_LIB_SOURCE_FILES_NodeSetting}
            #Chart
            ${DA_LIB_HEADER_FILES_Chart}
            ${DA_LIB_SOURCE_FILES_Chart}
            ${DA_LIB_QT_UI_FILES_Chart}
            #ChartAddItem
            ${DA_LIB_HEADER_FILES_ChartAddItem}
            ${DA_LIB_SOURCE_FILES_ChartAddItem}
            ${DA_LIB_QT_UI_FILES_ChartAddItem}
            #Agent
            ${DA_LIB_HEADER_FILES_Agent}
            ${DA_LIB_SOURCE_FILES_Agent}
            ${DA_LIB_RC_FILES_Agent}
            #MarkdownView
            ${DA_LIB_HEADER_FILES_MarkdownView}
            ${DA_LIB_SOURCE_FILES_MarkdownView}
            ${DA_LIB_RC_FILES_MarkdownView}
            ${CMAKE_CURRENT_SOURCE_DIR}/../DAGlobals.h
    QT_PUBLIC Core Gui Widgets Xml Svg ${DAGUI_WEBENGINE_COMPS}
    LINK_PUBLIC DAUtils DAMessageHandler DAData DAPyWorkFlow DAFigure
                DAPyBindQt DAPyScripts DAPyCommonWidgets
    # DAWidgets 提供基础 UI 控件（DAColorPickerButton/DAPenEditWidget/DAPropertyItemWidget 等），
    # 迁移自 DACommonWidgets 的 DAFormEditorRegistry/DAPropertyPanelWidget 等依赖这些控件。
    # DAGui 的头文件暴露 DAWidgets/ADS/qwt 的头（ui 提升与图表接口），需 PUBLIC
    THIRDPARTY_PUBLIC DAWidgets ads qwt quazip
    THIRDPARTY python pybind11 SARibbonBar ctk
    # ui 文件进行窗口提升时需要找到这些子目录的头文件
    PUBLIC_SUBDIRS Dialog ChartSetting Chart3DSetting NodeSetting Chart ChartAddItem Agent MarkdownView
    # 头文件安装：根 + 各子目录
    HEADERS_DIRS Models MimeData Commands ChartSetting Chart3DSetting NodeSetting Chart ChartAddItem Agent MarkdownView
    HEADERS ${DA_LIB_HEADER_FILES} ${DA_LIB_HEADER_FILES_Dialog}
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    target_link_libraries(DAGui PRIVATE Qt6::Core5Compat)
endif()
