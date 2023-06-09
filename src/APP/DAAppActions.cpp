﻿#include "DAAppActions.h"
#include <QActionGroup>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAppActions
//===================================================
DAAppActions::DAAppActions(DAAppUIInterface* u) : DAAppActionsInterface(u)
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
    actionAddData       = createAction("actionAddData", ":/Icon/Icon/addData.svg");
    actionRemoveData    = createAction("actionRemoveData", ":/Icon/Icon/removeData.svg");
    actionAddDataFolder = createAction("actionAddDataFolder", ":/Icon/Icon/folder.svg");
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
    actionChartEnableGrid   = createAction("actionChartEnableGrid", ":/Icon/Icon/chart-grid.svg", true, false);
    actionChartEnableGridX  = createAction("actionChartEnableGridX", ":/Icon/Icon/chart-grid-x.svg", true, false);
    actionChartEnableGridY  = createAction("actionChartEnableGridY", ":/Icon/Icon/chart-grid-y.svg", true, false);
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
    actionChartEnablePickerCross = createAction("actionChartEnablePickerCross", ":/Icon/Icon/chart-picker.svg", true, false);
    actionChartEnablePickerY = createAction("actionChartEnablePickerY", ":/Icon/Icon/chart-picker-y.svg", true, false);
    actionChartEnablePickerXY = createAction("actionChartEnablePickerXY", ":/Icon/Icon/chart-picker-xy.svg", true, false);
    actionGroupChartPickers->addAction(actionChartEnablePickerCross);
    actionGroupChartPickers->addAction(actionChartEnablePickerY);
    actionGroupChartPickers->addAction(actionChartEnablePickerXY);

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
    // workflow下面的状态action都是checkable状态的
    actionGroupWorkflowStartEdit = new QActionGroup(this);
    actionGroupWorkflowStartEdit->setObjectName(QStringLiteral("actionGroupWorkflowStartEdit"));
    actionGroupWorkflowStartEdit->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);  //允许都不选中
    actionWorkflowStartDrawRect = createAction("actionStartDrawRect", ":/Icon/Icon/drawRect.svg", true, false);
    actionWorkflowStartDrawText = createAction("actionStartDrawText", ":/Icon/Icon/drawText.svg", true, false);
    actionGroupWorkflowStartEdit->addAction(actionWorkflowStartDrawRect);
    actionGroupWorkflowStartEdit->addAction(actionWorkflowStartDrawText);
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
    actionAddData->setText(tr("Add Data"));          // cn:添加数据
    actionRemoveData->setText(tr("Remove Data"));    // cn:移除数据
    actionAddDataFolder->setText(tr("Add Folder"));  // cn:新建文件夹
    // Chart Category
    actionAddFigure->setText(tr("Add Figure"));                            // cn:添加绘图
    actionFigureResizeChart->setText(tr("Resize Chart"));                  // cn:绘图尺寸
    actionFigureNewXYAxis->setText(tr("New XY Axis"));                     // cn:新建坐标系
    actionChartEnableGrid->setText(tr("Enable Grid"));                     // cn:网格
    actionChartEnableGridX->setText(tr("X Grid"));                         // cn:横向网格
    actionChartEnableGridY->setText(tr("Y Grid"));                         // cn:纵向网格
    actionChartEnableGridXMin->setText(tr("Xmin Grid"));                   // cn:横向密集网格
    actionChartEnableGridYMin->setText(tr("Ymin Grid"));                   // cn:纵向密集网格
    actionChartEnableZoom->setText(tr("Zoom"));                            // cn:缩放
    actionChartZoomIn->setText(tr("Zoom In"));                             // cn:放大
    actionChartZoomOut->setText(tr("Zoom Out"));                           // cn:缩小
    actionChartZoomAll->setText(tr("Show All"));                           // cn:显示全部
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
    actionInsertRow->setText(tr("Insert Row"));                  // cn:插入行
    actionInsertRowAbove->setText(tr("Insert Row(Above)"));      // cn:插入行(上)
    actionInsertColumnRight->setText(tr("Insert Column"));       // cn:插入列(右)
    actionInsertColumnLeft->setText(tr("Insert Column(Left)"));  // cn:插入列(左)
    actionCastToNum->setText(tr("to num"));
    actionCastToNum->setToolTip(tr("cast to num type"));  // cn:转换为数值类型
    actionCastToString->setText(tr("to str"));
    actionCastToString->setToolTip(tr("cast to string type"));  // cn:转换为字符串类型
    actionCastToDatetime->setText(tr("to datetime"));
    actionCastToDatetime->setToolTip(tr("cast to datetime type"));  // cn:转换为日期类型
    actionCreateDataDescribe->setText(tr("Data Describe"));         // cn:数据描述
    actionChangeToIndex->setText(tr("To Index"));                   // cn:转换为索引
    // workflow 编辑
    actionWorkflowNew->setText(tr("New Workflow"));         // cn:新建工作流
    actionWorkflowStartDrawRect->setText(tr("Draw Rect"));  // cn:绘制矩形
    actionWorkflowStartDrawText->setText(tr("Draw Text"));  // cn:绘制文本
    actionWorkflowShowGrid->setText(tr("Show Grid"));       // cn:显示网格
    actionWorkflowWholeView->setText(tr("Whole View"));     // cn:全部可见
    actionWorkflowZoomOut->setText(tr("Zoom Out"));         // cn:缩小
    actionWorkflowZoomIn->setText(tr("Zoom In"));           // cn:放大
    actionWorkflowRun->setText(tr("Run Workflow"));         // cn:运行工作流

    actionWorkflowAddBackgroundPixmap->setText(tr("Add Background"));                 // cn:添加背景
    actionWorkflowLockBackgroundPixmap->setText(tr("Lock Background"));               // cn:锁定背景
    actionWorkflowEnableItemMoveWithBackground->setText(tr("Move With Background"));  // cn:元件随背景移动
    // View Category
    actionShowWorkFlowArea->setText(tr("Show Work Flow Area"));
    actionShowChartArea->setText(tr("Show Chart Area"));
    actionShowDataArea->setText(tr("Show Table Area"));
    actionShowMessageLogView->setText(tr("Show Infomation Window"));

    actionShowSettingWidget->setText(tr("Show Setting Window"));
    // Config Category
    actionPluginManager->setText(tr("Plugin Config"));
    //
    if (actionRedo) {
        actionRedo->setText(tr("Redo"));
    }
    if (actionUndo) {
        actionUndo->setText(tr("Undo"));
    }
}
