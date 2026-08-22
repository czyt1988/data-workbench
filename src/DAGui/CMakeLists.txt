
# Cmake的命令不区分打下写，例如message，set等命令；但Cmake的变量区分大小写
# 为统一风格，本项目的Cmake命令全部采用小写，变量全部采用大写加下划线组合。
# DAPyCommonWidgets 库

cmake_minimum_required(VERSION 3.10)
damacro_lib_setting(
    "DAGui"
    "DA GUI | https://github.com/czyt1988"
    0
    1
    5
)

########################################################
# 文件加载 #!!!!** 注意变更 **!!!!
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
    DANodeListWidget.h
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

# Models
set(DA_LIB_HEADER_FILES_Models
    ${DA_LIB_SUBDIR_Models}/DAAbstractCacheWindowTableModel.h
    ${DA_LIB_SUBDIR_Models}/DADataManagerTreeModel.h
    ${DA_LIB_SUBDIR_Models}/DADataManagerTreeFilterProxyModel.h
    ${DA_LIB_SUBDIR_Models}/DATreeModel.h
    ${DA_LIB_SUBDIR_Models}/DADataTableModel.h
    ${DA_LIB_SUBDIR_Models}/DADataManagerTableModel.h
    ${DA_LIB_SUBDIR_Models}/DAMessageLogsModel.h
    ${DA_LIB_SUBDIR_Models}/DAVariantTableModel.h
)
set(DA_LIB_SOURCE_FILES_Models
    ${DA_LIB_SUBDIR_Models}/DAAbstractCacheWindowTableModel.cpp
    ${DA_LIB_SUBDIR_Models}/DADataManagerTreeModel.cpp
    ${DA_LIB_SUBDIR_Models}/DADataManagerTreeFilterProxyModel.cpp
    ${DA_LIB_SUBDIR_Models}/DATreeModel.cpp
    ${DA_LIB_SUBDIR_Models}/DADataTableModel.cpp
    ${DA_LIB_SUBDIR_Models}/DADataManagerTableModel.cpp
    ${DA_LIB_SUBDIR_Models}/DAMessageLogsModel.cpp
    ${DA_LIB_SUBDIR_Models}/DAVariantTableModel.cpp
)
list(APPEND DA_LIB_HEADER_FILES_Models
    ${DA_LIB_SUBDIR_Models}/DAPySeriesTableModel.h
    ${DA_LIB_SUBDIR_Models}/DAPyDataFrameTableModel.h
    ${DA_LIB_SUBDIR_Models}/DAPyGridDataTableModel.h
)
list(APPEND DA_LIB_SOURCE_FILES_Models
    ${DA_LIB_SUBDIR_Models}/DAPySeriesTableModel.cpp
    ${DA_LIB_SUBDIR_Models}/DAPyDataFrameTableModel.cpp
    ${DA_LIB_SUBDIR_Models}/DAPyGridDataTableModel.cpp
)

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

#ChartSetting
# 这里直接遍历，不需要区分
file(GLOB DA_LIB_HEADER_FILES_ChartSetting ${DA_LIB_SUBDIR_ChartSetting}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_ChartSetting ${DA_LIB_SUBDIR_ChartSetting}/*.cpp)
file(GLOB DA_LIB_UI_FILES_ChartSetting ${DA_LIB_SUBDIR_ChartSetting}/*.ui)

#Chart3DSetting
file(GLOB DA_LIB_HEADER_FILES_Chart3DSetting ${DA_LIB_SUBDIR_Chart3DSetting}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_Chart3DSetting ${DA_LIB_SUBDIR_Chart3DSetting}/*.cpp)
file(GLOB DA_LIB_UI_FILES_Chart3DSetting ${DA_LIB_SUBDIR_Chart3DSetting}/*.ui)

#NodeSetting
set(DA_LIB_HEADER_FILES_NodeSetting)
list(APPEND DA_LIB_HEADER_FILES_NodeSetting
    ${DA_LIB_SUBDIR_NodeSetting}/DANodeParamSettingPanel.h
    ${DA_LIB_SUBDIR_NodeSetting}/DANodeParamSettingPanelFactory.h
    ${DA_LIB_SUBDIR_NodeSetting}/DANodeParamSettingPanelWidget.h
    ${DA_LIB_SUBDIR_NodeSetting}/DANodeParameterFormAdapter.h
)
list(APPEND DA_LIB_SOURCE_FILES_NodeSetting
    ${DA_LIB_SUBDIR_NodeSetting}/DANodeParamSettingPanel.cpp
    ${DA_LIB_SUBDIR_NodeSetting}/DANodeParamSettingPanelFactory.cpp
    ${DA_LIB_SUBDIR_NodeSetting}/DANodeParamSettingPanelWidget.cpp
    ${DA_LIB_SUBDIR_NodeSetting}/DANodeParameterFormAdapter.cpp
)

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
set(DA_LIB_SUBDIR_Agent ${CMAKE_CURRENT_SOURCE_DIR}/Agent)
file(GLOB DA_LIB_HEADER_FILES_Agent ${DA_LIB_SUBDIR_Agent}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_Agent ${DA_LIB_SUBDIR_Agent}/*.cpp)
file(GLOB DA_LIB_RC_FILES_Agent ${DA_LIB_SUBDIR_Agent}/*.qrc ${DA_LIB_SUBDIR_Agent}/resources/*.qrc)

#MarkdownView
set(DA_LIB_SUBDIR_MarkdownView ${CMAKE_CURRENT_SOURCE_DIR}/MarkdownView)
file(GLOB DA_LIB_HEADER_FILES_MarkdownView ${DA_LIB_SUBDIR_MarkdownView}/*.h)
file(GLOB DA_LIB_SOURCE_FILES_MarkdownView ${DA_LIB_SUBDIR_MarkdownView}/*.cpp)
file(GLOB DA_LIB_RC_FILES_MarkdownView ${DA_LIB_SUBDIR_MarkdownView}/*.qrc ${DA_LIB_SUBDIR_MarkdownView}/resources/*.qrc)

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
    target_link_libraries(${DA_LIB_NAME} PUBLIC
        Qt${QT_VERSION_MAJOR}::WebEngineCore
        Qt${QT_VERSION_MAJOR}::WebEngineWidgets
        Qt${QT_VERSION_MAJOR}::WebChannel
        Qt${QT_VERSION_MAJOR}::Network
    )
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
        WebEngine
        WebEngineWidgets
        WebChannel
        Network
    )
    target_link_libraries(${DA_LIB_NAME} PUBLIC
        Qt${QT_VERSION_MAJOR}::WebEngine
        Qt${QT_VERSION_MAJOR}::WebEngineWidgets
        Qt${QT_VERSION_MAJOR}::WebChannel
        Qt${QT_VERSION_MAJOR}::Network
    )
endif()
if(${QT_VERSION_MAJOR} EQUAL 6)
    target_link_libraries(${DA_LIB_NAME} PRIVATE Qt6::Core5Compat)
endif()
target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAUtils
    ${DA_PROJECT_NAME}::DAMessageHandler
    ${DA_PROJECT_NAME}::DAData
    ${DA_PROJECT_NAME}::DAPyWorkFlow
    ${DA_PROJECT_NAME}::DAFigure
)

# -------------link 3rdparty DAWidgets--------------------------
# DAWidgets 提供基础 UI 控件（DAColorPickerButton/DAPenEditWidget/DAPropertyItemWidget 等），
# 迁移自 DACommonWidgets 的 DAFormEditorRegistry/DAPropertyPanelWidget 等依赖这些控件。
damacro_import_DAWidgets(${DA_LIB_NAME})
target_link_libraries(${DA_LIB_NAME} PUBLIC DA::DAWidgets)

target_link_libraries(${DA_LIB_NAME} PUBLIC
    ${DA_PROJECT_NAME}::DAPyBindQt
    ${DA_PROJECT_NAME}::DAPyScripts
    ${DA_PROJECT_NAME}::DAPyCommonWidgets
)

# -------------link python--------------------------
damacro_import_Python(${DA_LIB_NAME})
# -------------link 3rdparty pybind11--------------------------
damacro_import_pybind11(${DA_LIB_NAME})

# -------------link SARibbon--------------------------
damacro_import_SARibbonBar(${DA_LIB_NAME})

# -------------link QtAdvancedDockingSystem--------------------------
damacro_import_QtAdvancedDocking(${DA_LIB_NAME})

# -------------link qwt--------------------------
damacro_import_qwt(${DA_LIB_NAME})

# -------------link ctk--------------------------
damacro_import_DALiteCtk(${DA_LIB_NAME})

# -------------link quazip--------------------------
damacro_import_quazip(${DA_LIB_NAME})
########################################################
# 设置通用库属性
########################################################
damacro_set_lib_properties(${DA_LIB_NAME} ${DA_LIB_VERSION})

# 添加这两个位置主要是为了ui文件进行窗口提升时能找到头文件
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/Dialog>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Dialog>
)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/ChartSetting>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ChartSetting>
)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/Chart3DSetting>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Chart3DSetting>
)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/NodeSetting>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/NodeSetting>
)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/Chart>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Chart>
)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/ChartAddItem>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ChartAddItem>
)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/Agent>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Agent>
)
target_include_directories(${DA_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/MarkdownView>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/MarkdownView>
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
#Chart3DSetting
install(FILES
    ${DA_LIB_HEADER_FILES_Chart3DSetting}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/Chart3DSetting
    COMPONENT headers
)
#NodeSetting
install(FILES
    ${DA_LIB_HEADER_FILES_NodeSetting}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/NodeSetting
    COMPONENT headers
)
#Chart
install(FILES
    ${DA_LIB_HEADER_FILES_Chart}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/Chart
    COMPONENT headers
)
#ChartAddItem
install(FILES
    ${DA_LIB_HEADER_FILES_ChartAddItem}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/ChartAddItem
    COMPONENT headers
)
#Agent
install(FILES
    ${DA_LIB_HEADER_FILES_Agent}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/Agent
    COMPONENT headers
)
#MarkdownView
install(FILES
    ${DA_LIB_HEADER_FILES_MarkdownView}
    DESTINATION include/${DA_PROJECT_NAME}/${DA_LIB_NAME}/MarkdownView
    COMPONENT headers
)

