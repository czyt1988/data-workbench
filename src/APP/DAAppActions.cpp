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
    //构建主页action
    buildMainAction();
    //构建数据相关的action
    buildDataAction();
    //构建绘图相关的action
    buildChartAction();
    //构建view相关的action
    buildViewAction();
    //构建workflow相关的action
    buildWorkflowAction();
    //
    buildOtherActions();
}

void DAAppActions::buildMainAction()
{
    // Main Category
    actionOpen          = createAction("actionOpen", ":/Icon/Icon/file.svg");
    actionSave          = createAction("actionSave", ":/Icon/Icon/save.svg");
    actionSaveAs        = createAction("actionSaveAs", ":/Icon/Icon/save-as.svg");
    actionAppendProject = createAction("actionAppendProject", ":/Icon/Icon/appendProject.svg");

    actionRedo    = nullptr;
    actionUndo    = nullptr;
    actionSetting = createAction("actionSetting", ":/Icon/Icon/setting.svg");
    // Config Category
    actionPluginManager = createAction("actionPluginManager", ":/Icon/Icon/plugin.svg");
}

void DAAppActions::buildDataAction()
{
    // Data Category
    actionAddData    = createAction("actionAddData", ":/Icon/Icon/addData.svg");
    actionRemoveData = createAction("actionRemoveData", ":/Icon/Icon/removeData.svg");
    // 数据操作的上下文标签 Data Operate Context Category
    actionRemoveRow          = createAction("actionRemoveRow", ":/Icon/Icon/removeRow.svg");
    actionRemoveColumn       = createAction("actionRemoveColumn", ":/Icon/Icon/removeColumn.svg");
    actionInsertRow          = createAction("actionInsertRow", ":/Icon/Icon/insertRow.svg");
    actionInsertRowAbove     = createAction("actionInsertRowAbove", ":/Icon/Icon/insertRowAbove.svg");
    actionInsertColumnRight  = createAction("actionInsertColumnRight", ":/Icon/Icon/insertColumnRight.svg");
    actionInsertColumnLeft   = createAction("actionInsertColumnLeft", ":/Icon/Icon/insertColumnLeft.svg");
    actionRenameColumns      = createAction("actionRenameColumns", ":/Icon/Icon/renameColumns.svg");
    actionRemoveCell         = createAction("actionRemoveCell", ":/Icon/Icon/removeCell.svg");
    actionCastToNum          = createAction("actionCastToNum", ":/Icon/Icon/castToNum.svg");
    actionCastToString       = createAction("actionCastToString", ":/Icon/Icon/castToString.svg");
    actionCastToDatetime     = createAction("actionCastToDatetime", ":/Icon/Icon/castToDatetime.svg");
    actionCreateDataDescribe = createAction("actionCreateDataDescribe", ":/Icon/Icon/dataDescribe.svg");
    actionChangeToIndex      = createAction("actionChangeToIndex", ":/Icon/Icon/changeToIndex.svg");
}

void DAAppActions::buildChartAction()
{
    //绘图标签 Chart Category
    actionAddFigure         = createAction("actionAddFigure", ":/Icon/Icon/addFigure.svg");
    actionFigureResizeChart = createAction("actionFigureResizeChart", ":/Icon/Icon/figureResizeChart.svg", true, false);
    actionFigureNewXYAxis   = createAction("actionFigureNewXYAxis", ":/Icon/Icon/newAxis.svg");
    actionChartAddCurve     = createAction("actionChartAddCurve", ":/app/chart-type/Icon/chart-type/chart-curve.svg");

    actionChartEnableGrid     = createAction("actionChartEnableGrid", ":/Icon/Icon/chart-grid.svg", true, false);
    actionChartEnableGridX    = createAction("actionChartEnableGridX", ":/Icon/Icon/chart-grid-x.svg", true, false);
    actionChartEnableGridY    = createAction("actionChartEnableGridY", ":/Icon/Icon/chart-grid-y.svg", true, false);
    actionChartEnableGridXMin = createAction("actionChartEnableGridXMin", ":/Icon/Icon/chart-grid-xmin.svg", true, false);
    actionChartEnableGridYMin = createAction("actionChartEnableGridYMin", ":/Icon/Icon/chart-grid-ymin.svg", true, false);
    actionChartEnableZoom     = createAction("actionChartEnableZoom", ":/Icon/Icon/chart-zoomer.svg", true, false);
    actionChartZoomIn         = createAction("actionChartZoomIn", ":/Icon/Icon/zoomIn.svg");
    actionChartZoomOut        = createAction("actionChartZoomOut", ":/Icon/Icon/zoomOut.svg");
    actionChartZoomAll        = createAction("actionChartZoomAll", ":/Icon/Icon/viewAll.svg");
    actionChartEnablePan      = createAction("actionChartEnablePan", ":/Icon/Icon/chart-pan.svg", true, false);

    actionGroupChartPickers = new QActionGroup(this);
    actionGroupChartPickers->setObjectName(QStringLiteral("actionGroupChartPickers"));
    actionGroupChartPickers->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);  //允许都不选中
    actionChartEnablePickerCross = createAction("actionChartEnablePickerCross", ":/Icon/Icon/chart-picker.svg", true, false, actionGroupChartPickers);
    actionChartEnablePickerY = createAction("actionChartEnablePickerY", ":/Icon/Icon/chart-picker-y.svg", true, false, actionGroupChartPickers);
    actionChartEnablePickerXY = createAction("actionChartEnablePickerXY", ":/Icon/Icon/chart-picker-xy.svg", true, false, actionGroupChartPickers);

    actionChartEnableLegend = createAction("actionChartEnableLegend", ":/Icon/Icon/chart-legend.svg", true, false);
    actionChartLegendAlignmentInTopLeft = createAction("actionChartLegendAlignmentInTopLeft", true, false);
    actionChartLegendAlignmentInTopLeft->setData(int(Qt::AlignTop | Qt::AlignLeft));
    actionChartLegendAlignmentInTop = createAction("actionChartLegendAlignmentInTop", true, false);
    actionChartLegendAlignmentInTop->setData(int(Qt::AlignTop | Qt::AlignHCenter));
    actionChartLegendAlignmentInTopRight = createAction("actionChartLegendAlignmentInTopRight", true, false);
    actionChartLegendAlignmentInTopRight->setData(int(Qt::AlignTop | Qt::AlignRight));
    actionChartLegendAlignmentInRight = createAction("actionChartLegendAlignmentInRight", true, false);
    actionChartLegendAlignmentInRight->setData(int(Qt::AlignVCenter | Qt::AlignRight));
    actionChartLegendAlignmentInBottomRight = createAction("actionChartLegendAlignmentInBottomRight", true, false);
    actionChartLegendAlignmentInBottomRight->setData(int(Qt::AlignBottom | Qt::AlignRight));
    actionChartLegendAlignmentInBottom = createAction("actionChartLegendAlignmentInBottom", true, false);
    actionChartLegendAlignmentInBottom->setData(int(Qt::AlignBottom | Qt::AlignHCenter));
    actionChartLegendAlignmentInBottomLeft = createAction("actionChartLegendAlignmentInBottomLeft", true, false);
    actionChartLegendAlignmentInBottomLeft->setData(int(Qt::AlignBottom | Qt::AlignLeft));
    actionChartLegendAlignmentInLeft = createAction("actionChartLegendAlignmentInLeft", true, false);
    actionChartLegendAlignmentInLeft->setData(int(Qt::AlignVCenter | Qt::AlignLeft));
    actionGroupChartLegendAlignment = new QActionGroup(this);
    actionGroupChartLegendAlignment->setObjectName(QStringLiteral("actionGroupChartLegendAlignment"));
    actionGroupChartLegendAlignment->addAction(actionChartLegendAlignmentInTopLeft);
    actionGroupChartLegendAlignment->addAction(actionChartLegendAlignmentInTop);
    actionGroupChartLegendAlignment->addAction(actionChartLegendAlignmentInTopRight);
    actionGroupChartLegendAlignment->addAction(actionChartLegendAlignmentInRight);
    actionGroupChartLegendAlignment->addAction(actionChartLegendAlignmentInBottomRight);
    actionGroupChartLegendAlignment->addAction(actionChartLegendAlignmentInBottom);
    actionGroupChartLegendAlignment->addAction(actionChartLegendAlignmentInBottomLeft);
    actionGroupChartLegendAlignment->addAction(actionChartLegendAlignmentInLeft);
}

void DAAppActions::buildViewAction()
{
    // View Category
    actionShowWorkFlowArea   = createAction("actionShowWorkFlowArea", ":/Icon/Icon/showWorkFlow.svg");
    actionShowChartArea      = createAction("actionShowChartArea", ":/Icon/Icon/showChart.svg");
    actionShowDataArea       = createAction("actionShowDataArea", ":/Icon/Icon/showTable.svg");
    actionShowMessageLogView = createAction("actionShowMessageLogView", ":/Icon/Icon/showInfomation.svg");
    actionShowSettingWidget  = createAction("actionShowSettingWidget", ":/Icon/Icon/showSettingWidget.svg");
}

void DAAppActions::buildWorkflowAction()
{
    // workflow 编辑
    actionWorkflowNew = createAction("actionNewWorkflow", ":/Icon/Icon/newWorkflow.svg");
    actionWorkflowEnableItemLinkageMove = createAction("actionWorkflowEnableItemLinkageMove", ":/Icon/Icon/itemLinkageMove.svg", true, false);
    actionItemGrouping = createAction("actionItemSetGroup", ":/Icon/Icon/item-set-group.svg");
    actionItemUngroup  = createAction("actionItemCancelGroup", ":/Icon/Icon/item-cancel-group.svg");
    // workflow下面的状态action都是checkable状态的
    actionGroupWorkflowStartEdit = new QActionGroup(this);
    actionGroupWorkflowStartEdit->setObjectName(QStringLiteral("actionGroupWorkflowStartEdit"));
    actionGroupWorkflowStartEdit->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);  //允许都不选中
    actionWorkflowStartDrawRect = createAction("actionStartDrawRect", ":/Icon/Icon/drawRect.svg", true, false, actionGroupWorkflowStartEdit);
    actionWorkflowStartDrawText = createAction("actionStartDrawText", ":/Icon/Icon/drawText.svg", true, false, actionGroupWorkflowStartEdit);
    // workflow-背景图相关
    actionWorkflowAddBackgroundPixmap = createAction("actionAddBackgroundPixmap", ":/Icon/Icon/backgroundPixmap.svg");
    actionWorkflowLockBackgroundPixmap = createAction("actionLockBackgroundPixmap", ":/Icon/Icon/lock-bk.svg", true, false);
    actionWorkflowEnableItemMoveWithBackground = createAction("actionEnableItemMoveWithBackground",
                                                              ":/Icon/Icon/itemMoveWithBackground.svg",
                                                              true,
                                                              false);
    // workflow-视图操作
    actionWorkflowShowGrid  = createAction("actionWorkflowShowGrid", ":/Icon/Icon/showGrid.svg", true, true);
    actionWorkflowWholeView = createAction("actionWholeView", ":/Icon/Icon/viewAll.svg");
    actionWorkflowZoomIn    = createAction("actionZoomIn", ":/Icon/Icon/zoomIn.svg");
    actionWorkflowZoomOut   = createAction("actionZoomOut", ":/Icon/Icon/zoomOut.svg");
    actionWorkflowRun       = createAction("actionRunWorkflow", ":/Icon/Icon/run.svg");
    actionWorkflowTerminate = createAction("actionWorkflowTerminate", ":/Icon/Icon/stop.svg");
    actionWorkflowTerminate->setEnabled(false);
}

void DAAppActions::buildOtherActions()
{
    actionGroupRibbonTheme = new QActionGroup(this);
    actionGroupRibbonTheme->setObjectName(QStringLiteral("actionGroupRibbonTheme"));
    actionRibbonThemeOffice2013     = createAction("actionRibbonThemeOffice2013", true, true, actionGroupRibbonTheme);
    actionRibbonThemeOffice2016Blue = createAction("actionRibbonThemeOffice2013", true, false, actionGroupRibbonTheme);
    actionRibbonThemeOffice2021Blue = createAction("actionRibbonThemeOffice2013", true, false, actionGroupRibbonTheme);
    actionRibbonThemeDark           = createAction("actionRibbonThemeOffice2013", true, false, actionGroupRibbonTheme);
}

void DAAppActions::retranslateUi()
{
    // Main Category
    actionOpen->setText(tr("Open"));
    actionSave->setText(tr("Save"));
    actionSaveAs->setText(tr("Save As"));
    actionAppendProject->setText(tr("Append To Project"));

    actionRenameColumns->setText(tr("Rename Columns"));
    actionSetting->setText(tr("Setting"));
    // Data Category
    actionAddData->setText(tr("Add \nData"));        // cn:添加\n数据
    actionRemoveData->setText(tr("Remove \nData"));  // cn:移除\n数据
    // Chart Category
    actionAddFigure->setText(tr("Add \nFigure"));            // cn:添加\n绘图
    actionFigureResizeChart->setText(tr("Resize \nChart"));  // cn:绘图\n尺寸
    actionFigureNewXYAxis->setText(tr("New \nXY Axis"));     // cn:新建\n坐标系
    actionChartAddCurve->setText(tr("Add \nCurve"));         // cn:曲线

    actionChartEnableGrid->setText(tr("Enable Grid"));                     // cn:网格
    actionChartEnableGridX->setText(tr("X Grid"));                         // cn:横向网格
    actionChartEnableGridY->setText(tr("Y Grid"));                         // cn:纵向网格
    actionChartEnableGridXMin->setText(tr("Xmin Grid"));                   // cn:横向密集网格
    actionChartEnableGridYMin->setText(tr("Ymin Grid"));                   // cn:纵向密集网格
    actionChartEnableZoom->setText(tr("Zoom"));                            // cn:缩放
    actionChartZoomIn->setText(tr("Zoom In"));                             // cn:放大
    actionChartZoomOut->setText(tr("Zoom Out"));                           // cn:缩小
    actionChartZoomAll->setText(tr("Show \nAll"));                         // cn:显示\n全部
    actionChartEnablePan->setText(tr("Pan"));                              // cn:拖动
    actionChartEnablePickerCross->setText(tr("Cross"));                    // cn:十字标记
    actionChartEnablePickerY->setText(tr("Y Picker"));                     // cn:y值拾取
    actionChartEnablePickerXY->setText(tr("XY Picker"));                   // cn:点拾取
    actionChartEnableLegend->setText(tr("legend"));                        // cn:图例
    actionChartLegendAlignmentInTopLeft->setText(tr("Top Left"));          // cn:左上对齐
    actionChartLegendAlignmentInTop->setText(tr("Top"));                   // cn:上对齐
    actionChartLegendAlignmentInTopRight->setText(tr("Top Right"));        // cn:右上对齐
    actionChartLegendAlignmentInRight->setText(tr("Right"));               // cn:右对齐
    actionChartLegendAlignmentInBottomRight->setText(tr("Bottom Right"));  // cn:右下对齐
    actionChartLegendAlignmentInBottom->setText(tr("Bottom"));             // cn:下对齐
    actionChartLegendAlignmentInBottomLeft->setText(tr("Bottom Left"));    // cn:左下对齐
    actionChartLegendAlignmentInLeft->setText(tr("Left"));                 // cn:左对齐
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
    actionCastToDatetime->setToolTip(tr("cast to datetime type"));  // cn:转换为日期类型
    actionCreateDataDescribe->setText(tr("Data Describe"));         // cn:数据描述
    actionChangeToIndex->setText(tr("To Index"));                   // cn:转换为\n索引
    // workflow 编辑
    actionWorkflowNew->setText(tr("New \nWorkflow"));                    // cn:新建\n工作流
    actionWorkflowEnableItemLinkageMove->setText(tr("Linkage \nMove"));  // cn:联动
    actionWorkflowEnableItemLinkageMove->setToolTip(
        tr("When moving elements, other elements linked to this element follow the movement"));  // cn:允许移动图元时，其它和此图元链接起来的图元跟随移动
    actionItemGrouping->setText(tr("grouping"));                   // cn:分组
    actionItemUngroup->setText(tr("ungroup"));                     // cn:取消分组
    actionWorkflowStartDrawRect->setText(tr("Draw \nRect"));       // cn:绘制\n矩形
    actionWorkflowStartDrawText->setText(tr("Draw \nText"));       // cn:绘制\n文本
    actionWorkflowShowGrid->setText(tr("Show \nGrid"));            // cn:显示\n网格
    actionWorkflowWholeView->setText(tr("Whole \nView"));          // cn:全部\n可见
    actionWorkflowZoomOut->setText(tr("Zoom Out"));                // cn:缩小
    actionWorkflowZoomIn->setText(tr("Zoom In"));                  // cn:放大
    actionWorkflowRun->setText(tr("Run \nWorkflow"));              // cn:运行\n工作流
    actionWorkflowTerminate->setText(tr("Terminate \nWorkflow"));  // cn:停止\n工作流

    actionWorkflowAddBackgroundPixmap->setText(tr("Add \nBackground"));               // cn:添加\n背景
    actionWorkflowLockBackgroundPixmap->setText(tr("Lock Background"));               // cn:锁定背景
    actionWorkflowEnableItemMoveWithBackground->setText(tr("Move With Background"));  // cn:元件随背景移动
    // View Category
    actionShowWorkFlowArea->setText(tr("Show \nWork Flow Area"));     // cn:工作流\n区域
    actionShowChartArea->setText(tr("Show \nChart Area"));            // cn:绘图\n区域
    actionShowDataArea->setText(tr("Show \nTable Area"));             // cn:表格\n区域
    actionShowMessageLogView->setText(tr("Show Infomation Window"));  // cn:信息窗口
    actionShowSettingWidget->setText(tr("Show Setting Window"));      // cn:设置窗口
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
