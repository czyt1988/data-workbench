#include "DAAppActions.h"
#include <QActionGroup>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppActions
//===================================================
DAAppActions::DAAppActions(DAUIInterface* u) : DAActionsInterface(u)
{
	buildActions();
}

DAAppActions::~DAAppActions()
{
}

void DAAppActions::buildActions()
{
	// 构建主页action
	buildMainAction();
	// 构建数据相关的action
	buildDataAction();
	// 构建绘图相关的action
	buildChartAction();
	// 构建view相关的action
	buildViewAction();
	// 构建workflow相关的action
	buildWorkflowAction();
	//
	buildOtherActions();
}

void DAAppActions::buildMainAction()
{
	// Main Category
	actionOpen          = createAction("actionOpen", ":/app/bright/Icon/file.svg");
	actionSave          = createAction("actionSave", ":/app/bright/Icon/save.svg");
	actionSaveAs        = createAction("actionSaveAs", ":/app/bright/Icon/save-as.svg");
	actionAppendProject = createAction("actionAppendProject", ":/app/bright/Icon/appendProject.svg");
	// 注意Redo/undo action在ribbonArea中生成
	actionRedo    = nullptr;
	actionUndo    = nullptr;
	actionSetting = createAction("actionSetting", ":/app/bright/Icon/setting.svg");
	// Config Category
	actionPluginManager = createAction("actionPluginManager", ":/app/bright/Icon/plugin.svg");
	//
	actionAbout = createAction("actionAbout", ":/app/bright/Icon/about.svg");
}

void DAAppActions::buildDataAction()
{
	// Data Category
	actionAddData    = createAction("actionAddData", ":/app/bright/Icon/addData.svg");
	actionRemoveData = createAction("actionRemoveData", ":/app/bright/Icon/removeData.svg");
	// 数据操作的上下文标签 Data Operate Context Category
	actionRemoveRow          = createAction("actionRemoveRow", ":/app/bright/Icon/removeRow.svg");
	actionRemoveColumn       = createAction("actionRemoveColumn", ":/app/bright/Icon/removeColumn.svg");
	actionInsertRow          = createAction("actionInsertRow", ":/app/bright/Icon/insertRow.svg");
	actionInsertRowAbove     = createAction("actionInsertRowAbove", ":/app/bright/Icon/insertRowAbove.svg");
	actionInsertColumnRight  = createAction("actionInsertColumnRight", ":/app/bright/Icon/insertColumnRight.svg");
	actionInsertColumnLeft   = createAction("actionInsertColumnLeft", ":/app/bright/Icon/insertColumnLeft.svg");
	actionRenameColumns      = createAction("actionRenameColumns", ":/app/bright/Icon/renameColumns.svg");
	actionRemoveCell         = createAction("actionRemoveCell", ":/app/bright/Icon/removeCell.svg");
	actionCastToNum          = createAction("actionCastToNum", ":/app/bright/Icon/castToNum.svg");
	actionCastToString       = createAction("actionCastToString", ":/app/bright/Icon/castToString.svg");
	actionCastToDatetime     = createAction("actionCastToDatetime", ":/app/bright/Icon/castToDatetime.svg");
	actionCreateDataDescribe = createAction("actionCreateDataDescribe", ":/app/bright/Icon/dataDescribe.svg");
	actionChangeToIndex      = createAction("actionChangeToIndex", ":/app/bright/Icon/changeToIndex.svg");
	actionDataFrameDropNone  = createAction("actionDataFrameDropNone", ":/app/bright/Icon/dataframe-drop-none.svg");
	actionDropDuplicates     = createAction("actionDropDuplicates", ":/app/bright/Icon/process-duplicate-data.svg");
}

void DAAppActions::buildChartAction()
{
	// 绘图标签 Chart Category
	actionAddFigure = createAction("actionAddFigure", ":/app/bright/Icon/addFigure.svg");
	actionFigureResizeChart =
		createAction("actionFigureResizeChart", ":/app/bright/Icon/figureResizeChart.svg", true, false);
	actionFigureNewXYAxis = createAction("actionFigureNewXYAxis", ":/app/bright/Icon/newAxis.svg");
	actionChartAddCurve   = createAction("actionChartAddCurve", ":/app/chart-type/Icon/chart-type/chart-curve.svg");
	actionChartAddScatter2D =
		createAction("actionChartAddScatter2D", ":/app/chart-type/Icon/chart-type/chart-scatter-2d.svg");
	actionChartAddErrorBar =
		createAction("actionChartAddErrorBar", ":/app/chart-type/Icon/chart-type/chart-intervalcurve.svg");
	actionChartAddBoxPlot = createAction("actionChartAddBoxPlot", ":/app/chart-type/Icon/chart-type/chart-OHLC.svg");
	actionChartAddBar     = createAction("actionChartAddBar", ":/app/chart-type/Icon/chart-type/chart-bar.svg");
	actionChartAddMultiBar = createAction("actionChartAddMultiBar", ":/app/chart-type/Icon/chart-type/chart-multibar.svg");
	actionChartAddHistogramBar =
		createAction("actionChartAddHistogramBar", ":/app/chart-type/Icon/chart-type/chart-histogram.svg");
	actionChartAddContourMap =
		createAction("actionChartAddContourMap", ":/app/chart-type/Icon/chart-type/chart-spectrocurve.svg");
	actionChartAddCloudMap =
		createAction("actionChartAddCloudMap", ":/app/chart-type/Icon/chart-type/chart-spectrogram.svg");
	actionChartAddVectorfield =
		createAction("actionChartAddVectorfield", ":/app/chart-type/Icon/chart-type/chart-vectorfield.svg");

	actionChartEnableGrid  = createAction("actionChartEnableGrid", ":/app/bright/Icon/chart-grid.svg", true, false);
	actionChartEnableGridX = createAction("actionChartEnableGridX", ":/app/bright/Icon/chart-grid-x.svg", true, false);
	actionChartEnableGridY = createAction("actionChartEnableGridY", ":/app/bright/Icon/chart-grid-y.svg", true, false);
	actionChartEnableGridXMin =
		createAction("actionChartEnableGridXMin", ":/app/bright/Icon/chart-grid-xmin.svg", true, false);
	actionChartEnableGridYMin =
		createAction("actionChartEnableGridYMin", ":/app/bright/Icon/chart-grid-ymin.svg", true, false);
	actionChartEnableZoom = createAction("actionChartEnableZoom", ":/app/bright/Icon/chart-zoomer.svg", true, false);
	actionChartZoomIn     = createAction("actionChartZoomIn", ":/app/bright/Icon/zoomIn.svg");
	actionChartZoomOut    = createAction("actionChartZoomOut", ":/app/bright/Icon/zoomOut.svg");
	actionChartZoomAll    = createAction("actionChartZoomAll", ":/app/bright/Icon/viewAll.svg");
	actionChartEnablePan  = createAction("actionChartEnablePan", ":/app/bright/Icon/chart-pan.svg", true, false);

	actionGroupChartPickers = new QActionGroup(this);
	actionGroupChartPickers->setObjectName(QStringLiteral("actionGroupChartPickers"));
	actionGroupChartPickers->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);  // 允许都不选中
	actionChartEnablePickerCross = createAction(
		"actionChartEnablePickerCross", ":/app/bright/Icon/chart-picker.svg", true, false, actionGroupChartPickers);
	actionChartEnablePickerY = createAction(
		"actionChartEnablePickerY", ":/app/bright/Icon/chart-picker-y.svg", true, false, actionGroupChartPickers);
	actionChartEnablePickerXY = createAction(
		"actionChartEnablePickerXY", ":/app/bright/Icon/chart-picker-xy.svg", true, false, actionGroupChartPickers);

	actionChartEnableLegend = createAction("actionChartEnableLegend", ":/app/bright/Icon/chart-legend.svg", true, false);
}

void DAAppActions::buildViewAction()
{
	// View Category
	actionShowWorkFlowArea = createAction("actionShowWorkFlowArea", ":/app/bright/Icon/showWorkFlow.svg");
	actionShowWorkFlowManagerArea =
		createAction("actionShowWorkFlowManagerArea", ":/app/bright/Icon/workflow-manager-view.svg");
	actionShowChartArea        = createAction("actionShowChartArea", ":/app/bright/Icon/showChart.svg");
	actionShowChartManagerArea = createAction("actionShowChartManagerArea", ":/app/bright/Icon/chart-manager-view.svg");
	actionShowDataArea         = createAction("actionShowDataArea", ":/app/bright/Icon/showTable.svg");
	actionShowDataManagerArea  = createAction("actionShowDataManagerArea", ":/app/bright/Icon/data-manager-view.svg");
	actionShowMessageLogView   = createAction("actionShowMessageLogView", ":/app/bright/Icon/showInfomation.svg");
	actionShowSettingWidget    = createAction("actionShowSettingWidget", ":/app/bright/Icon/showSettingWidget.svg");
}

void DAAppActions::buildWorkflowAction()
{
	// workflow 编辑
	actionWorkflowNew = createAction("actionWorkflowNew", ":/app/bright/Icon/newWorkflow.svg");
	actionWorkflowEnableItemLinkageMove =
		createAction("actionWorkflowEnableItemLinkageMove", ":/app/bright/Icon/itemLinkageMove.svg", true, false);
	actionItemGrouping       = createAction("actionItemSetGroup", ":/app/bright/Icon/item-set-group.svg");
	actionItemUngroup        = createAction("actionItemCancelGroup", ":/app/bright/Icon/item-cancel-group.svg");
	actionWorkflowLinkEnable = createAction("actionWorkflowLinkEnable", ":/app/bright/Icon/link.svg", true, true);
	// workflow下面的状态action都是checkable状态的
	actionGroupWorkflowStartEdit = new QActionGroup(this);
	actionGroupWorkflowStartEdit->setObjectName(QStringLiteral("actionGroupWorkflowStartEdit"));
	actionGroupWorkflowStartEdit->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);  // 允许都不选中
	actionWorkflowStartDrawRect =
		createAction("actionStartDrawRect", ":/app/bright/Icon/drawRect.svg", true, false, actionGroupWorkflowStartEdit);
	actionWorkflowStartDrawText =
		createAction("actionStartDrawText", ":/app/bright/Icon/drawText.svg", true, false, actionGroupWorkflowStartEdit);
	// workflow-背景图相关
	actionWorkflowAddBackgroundPixmap = createAction("actionAddBackgroundPixmap", ":/app/bright/Icon/backgroundPixmap.svg");
	actionWorkflowLockBackgroundPixmap =
		createAction("actionLockBackgroundPixmap", ":/app/bright/Icon/lock-bk.svg", true, false);
	actionWorkflowEnableItemMoveWithBackground =
		createAction("actionEnableItemMoveWithBackground", ":/app/bright/Icon/itemMoveWithBackground.svg", true, false);
	// workflow-视图操作
	actionWorkflowShowGrid     = createAction("actionWorkflowShowGrid", ":/app/bright/Icon/showGrid.svg", true, true);
	actionWorkflowViewReadOnly = createAction("actionWorkflowViewLock", ":/app/bright/Icon/lock-view.svg", true, false);
	actionWorkflowViewMarker = createAction("actionWorkflowViewMarker", ":/app/bright/Icon/view-marker.svg", true, false);
	// 运行
	actionWorkflowRun       = createAction("actionWorkflowRun", ":/app/bright/Icon/run.svg");
	actionWorkflowTerminate = createAction("actionWorkflowTerminate", ":/app/bright/Icon/stop.svg");
	actionWorkflowTerminate->setEnabled(false);
	// 导出
	actionExportWorkflowSceneToImage = createAction("actionExportWorkflowSceneToImage", ":/app/bright/Icon/exportToPic.svg");
	actionExportWorkflowSceneToPNG = createAction("actionExportWorkflowSceneToPNG", ":/app/bright/Icon/exportToPng.svg");
}

void DAAppActions::buildOtherActions()
{
	actionGroupRibbonTheme = new QActionGroup(this);
	actionGroupRibbonTheme->setObjectName(QStringLiteral("actionGroupRibbonTheme"));
	actionRibbonThemeOffice2013     = createAction("actionRibbonThemeOffice2013", true, true, actionGroupRibbonTheme);
	actionRibbonThemeOffice2016Blue = createAction("actionRibbonThemeOffice2016Blue", true, false, actionGroupRibbonTheme);
	actionRibbonThemeOffice2021Blue = createAction("actionRibbonThemeOffice2021Blue", true, false, actionGroupRibbonTheme);
	actionRibbonThemeDark           = createAction("actionRibbonThemeDark", true, false, actionGroupRibbonTheme);
}

void DAAppActions::retranslateUi()
{
	// Main Category
	actionOpen->setText(tr("Open"));                     // cn:打开
	actionOpen->setToolTip(tr("Open file or project"));  // cn:打开文件或项目
	actionSave->setText(tr("Save"));
	actionSaveAs->setText(tr("Save As"));
	actionAppendProject->setText(tr("Append To Project"));

	actionRenameColumns->setText(tr("Rename Columns"));
	actionSetting->setText(tr("Setting"));  // cn:设置
	actionAbout->setText(tr("About"));      // cn:关于
	// Data Category
	actionAddData->setText(tr("Add \nData"));        // cn:添加\n数据
	actionRemoveData->setText(tr("Remove \nData"));  // cn:移除\n数据
	// Chart Category
	actionAddFigure->setText(tr("Add \nFigure"));                  // cn:添加\n绘图
	actionFigureResizeChart->setText(tr("Resize \nChart"));        // cn:绘图\n尺寸
	actionFigureNewXYAxis->setText(tr("New \nXY Axis"));           // cn:新建\n坐标系
	actionChartAddCurve->setText(tr("Add \nCurve"));               // cn:折线图
	actionChartAddScatter2D->setText(tr("Add \nScatter"));         // cn:散点图
	actionChartAddErrorBar->setText(tr("Add \nError Bar"));        // cn:误差棒图
	actionChartAddBoxPlot->setText(tr("Add \nBox Plot"));          // cn:箱线图
	actionChartAddBar->setText(tr("Add \nBar"));                   // cn:柱状图
	actionChartAddMultiBar->setText(tr("Add \nMultiBar"));         // cn:多重柱状图
	actionChartAddHistogramBar->setText(tr("Add \nHistogram"));    // cn:分布图
	actionChartAddContourMap->setText(tr("Add \nContour Map"));    // cn:等高线图
	actionChartAddCloudMap->setText(tr("Add \nCloud Map"));        // cn:云图
	actionChartAddVectorfield->setText(tr("Add \nVector Field"));  // cn:向量场图

	actionChartEnableGrid->setText(tr("Enable Grid"));    // cn:网格
	actionChartEnableGridX->setText(tr("X Grid"));        // cn:横向网格
	actionChartEnableGridY->setText(tr("Y Grid"));        // cn:纵向网格
	actionChartEnableGridXMin->setText(tr("Xmin Grid"));  // cn:横向密集网格
	actionChartEnableGridYMin->setText(tr("Ymin Grid"));  // cn:纵向密集网格
	actionChartEnableZoom->setText(tr("Zoom"));           // cn:缩放
	actionChartZoomIn->setText(tr("Zoom In"));            // cn:放大
	actionChartZoomOut->setText(tr("Zoom Out"));          // cn:缩小
	actionChartZoomAll->setText(tr("Show \nAll"));        // cn:显示\n全部
	actionChartEnablePan->setText(tr("Pan"));             // cn:拖动
	actionChartEnablePickerCross->setText(tr("Cross"));   // cn:十字标记
	actionChartEnablePickerY->setText(tr("Y Picker"));    // cn:y值拾取
	actionChartEnablePickerXY->setText(tr("XY Picker"));  // cn:点拾取
	actionChartEnableLegend->setText(tr("legend"));       // cn:图例

	// 数据操作的上下文标签 Data Operate Context Category
	actionRemoveRow->setText(tr("Remove Row"));                  // cn:删除行
	actionRemoveColumn->setText(tr("Remove Column"));            // cn:删除列
	actionRemoveCell->setText(tr("Remove Cell"));                // cn:移除单元格
	actionInsertRow->setText(tr("Insert \nRow"));                // cn:插入行
	actionInsertRowAbove->setText(tr("Insert Row(Above)"));      // cn:插入行(上)
	actionInsertColumnRight->setText(tr("Insert \nColumn"));     // cn:插入\n列(右)
	actionInsertColumnLeft->setText(tr("Insert Column(Left)"));  // cn:插入列(左)
	actionCastToNum->setText(tr("to num"));
	actionCastToNum->setToolTip(tr("cast to num type"));  // cn:转换为数值类型
	actionCastToString->setText(tr("to str"));
	actionCastToString->setToolTip(tr("cast to string type"));  // cn:转换为字符串类型
	actionCastToDatetime->setText(tr("to datetime"));
	actionCastToDatetime->setToolTip(tr("cast to datetime type"));                      // cn:转换为日期类型
	actionCreateDataDescribe->setText(tr("Data Describe"));                             // cn:数据描述
	actionChangeToIndex->setText(tr("To Index"));                                       // cn:转换为\n索引
	actionDataFrameDropNone->setText(tr("Drop None"));                                  // cn:删除\n缺失值
	actionDataFrameDropNone->setToolTip(tr("Drop rows which contain missing values"));  // cn:删除包含缺失值的行
	actionDropDuplicates->setText(tr("Drop Duplicates"));                               // cn:删除\n重复值
	actionDropDuplicates->setToolTip(tr("Drop duplicate datas"));  // cn:删除数据中的重复记录
	// workflow 编辑
	actionWorkflowNew->setText(tr("New \nWorkflow"));                    // cn:新建\n工作流
	actionWorkflowEnableItemLinkageMove->setText(tr("Linkage \nMove"));  // cn:联动
	actionWorkflowEnableItemLinkageMove->setToolTip(
	    tr("When moving elements, other elements linked to this element follow the movement"));  // cn:允许移动图元时，其它和此图元链接起来的图元跟随移动
	actionItemGrouping->setText(tr("grouping"));                                      // cn:分组
	actionItemUngroup->setText(tr("ungroup"));                                        // cn:取消分组
	actionWorkflowStartDrawRect->setText(tr("Draw \nRect"));                          // cn:绘制\n矩形
	actionWorkflowStartDrawText->setText(tr("Draw \nText"));                          // cn:绘制\n文本
	actionWorkflowShowGrid->setText(tr("Show \nGrid"));                               // cn:显示\n网格
	actionWorkflowViewReadOnly->setText(tr("Lock \nView"));                           // cn:锁定\n视图
	actionWorkflowRun->setText(tr("Run \nWorkflow"));                                 // cn:运行\n工作流
	actionWorkflowTerminate->setText(tr("Terminate \nWorkflow"));                     // cn:停止\n工作流
	actionWorkflowLinkEnable->setText(tr("Link"));                                    // cn:连线
	actionWorkflowAddBackgroundPixmap->setText(tr("Add \nBackground"));               // cn:添加\n背景
	actionWorkflowLockBackgroundPixmap->setText(tr("Lock Background"));               // cn:锁定背景
	actionWorkflowEnableItemMoveWithBackground->setText(tr("Move With Background"));  // cn:元件随背景移动
	                                                                                  // workflow 视图
	actionExportWorkflowSceneToImage->setText(tr("Export To Image"));                 // cn:导出为图片
	actionExportWorkflowSceneToPNG->setText(tr("Export To PNG"));                     // cn:导出为PNG
	actionWorkflowViewMarker->setText(tr("Show Marker"));                             // cn:显示标记
	// View Category
	actionShowWorkFlowArea->setText(tr("Show \nWorkflow Area"));            // cn:工作流\n区域
	actionShowWorkFlowManagerArea->setText(tr("Show \nWorkflow Manager"));  // cn:工作流\n管理
	actionShowChartArea->setText(tr("Show \nChart Area"));                  // cn:绘图\n区域
	actionShowChartManagerArea->setText(tr("Show \nChart Manager"));        // cn:绘图\n管理
	actionShowDataArea->setText(tr("Show \nTable Area"));                   // cn:表格\n区域
	actionShowDataManagerArea->setText(tr("Show \nData Manager"));          // cn:数据\n管理
	actionShowMessageLogView->setText(tr("Show Infomation Window"));        // cn:信息窗口
	actionShowSettingWidget->setText(tr("Show Setting Window"));            // cn:设置窗口
	// Config Category
	actionPluginManager->setText(tr("Plugin \nConfig"));  // cn:插件\n设置
	// Other
	actionRibbonThemeOffice2013->setText(tr("Office 2013 theme"));
	actionRibbonThemeOffice2016Blue->setText(tr("Office 2016 blue theme"));
	actionRibbonThemeOffice2021Blue->setText(tr("Office 2021 blue theme"));
	actionRibbonThemeDark->setText(tr("Dark theme"));

	//
	if (actionRedo) {
		actionRedo->setText(tr("Redo"));
	}
	if (actionUndo) {
		actionUndo->setText(tr("Undo"));
	}
}
