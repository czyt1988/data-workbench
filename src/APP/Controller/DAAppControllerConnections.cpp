#include "DAAppController.h"
// Qt
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTimer>
#include <QFontComboBox>
#include <QInputDialog>
#include <QMenu>
#include <QApplication>
#include <QActionGroup>
#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QClipboard>
#include <QFile>
#include <QMimeData>
#include <QUrl>
// qwt
#include "qwt_figure.h"
#include "qwt_plot_series_data_picker.h"
#include <QwtPlotMarker>
#include <QwtPlotTextLabel>
#include <QwtText>
// SARibbon
#include "SARibbonGlobal.h"
#include "SARibbonMainWindow.h"

// API
#include "AppMainWindow.h"
#include "DAAppCore.h"
#include "Ribbon/DAAppRibbonArea.h"
#include "DAAppDockingArea.h"
#include "DAChartOperateWidget.h"
#include "DAAppCommand.h"
#include "DAUndoHistoryWidget.h"
#include "DAAppActions.h"
#include "DAAppDataManager.h"
#include "DAProjectInterface.h"
// plugin
// Qt-Advanced-Docking-System
#include "DockManager.h"
#include "DockAreaWidget.h"
// command
// Widget
#include "DAWaitCursorScoped.h"
#include "Table/DADataOperateWidget.h"
#include "Table/DADataOperatePageWidget.h"
#include "Common/DAAbstractOperateWidget.h"
#include "DataManager/DADataManageWidget.h"
#include "DAAppChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "Chart/Setting/DAChartAxisSettingPanel.h"
#include "DAAppChartManageWidget.h"
#include "SettingPages/DASettingPageGeneral.h"
#include "Common/Settings/DASettingContainerWidget.h"
#include "Common/DARecentFilesManager.h"
#include "Chart/DAChartSettingWidget.h"
#include "Chart/DADataLinkTableWidget.h"
#include "DAColorTheme.h"
#include "Chart/Setting/DAFigureWidgetSettingPanel.h"
#include "Chart/Setting3D/DAChart3DSettingWidget.h"
// Dialog
#include "DAPluginManagerDialog.h"
#include "DAAppSettingDialog.h"
#include "Dialog/DAExportToPngSettingDialog.h"
#include "Dialog/DALayoutManagerDialog.h"
#include "Dialog/DAWorkbenchAboutDialog.h"
// DAWidgets
#include "DAFontEditPannelWidget.h"
#include "SARibbonColorToolButton.h"
#include "DAShapeEditPannelWidget.h"
// Table style
#include "Table/Metadata/DATableCellStyle.h"
// 表格元数据与数据撤销命令（004 T1.27 就地形状变换：Swap 命令 + 元数据束快照/清理）
#include "Table/Metadata/DATableMetadata.h"
#include "Table/Metadata/DATableMetadataRegistry.h"
#include "DADataUndoCommand.h"
// Workflow
#include "Workflow/DAPyWorkFlowOperateWidget.h"
#include "Workflow/DAPyWorkFlowGraphicsView.h"
#include "DAGraphicsDrawRectSceneAction.h"
#include "DAGraphicsDrawTextItemSceneAction.h"
// project
#include "DAAppProject.h"
#include "DAAppProjectActionPolicy.h"
// Py
#include "DAPybind11QtCaster.hpp"
#include "Table/Dialogs/DATxtFileImportDialog.h"
#include "DAPyDTypeComboBox.h"
#include "DAPyScripts.h"
#include "pandas/DAPyDataFrame.h"
#include "numpy/DAPyDType.h"
// Widget
#include "Table/DADataOperateOfDataFrameWidget.h"
#include "Table/Views/DADataHeaderView.h"
#include "Table/Views/DATableDisplayFormatComboBox.h"
#include "Table/Dialogs/DADialogTableDisplayFormat.h"
#include "Table/Views/DADataTableView.h"
// Python workflow
#include "DAPyWorkFlowScene.h"
// Stats plot widgets
#include "Chart/DADialogStatsChartGuide.h"
#include "Chart/AddItem/DAChartAddStatsHistplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsKdeplot1dWidget.h"
#include "Chart/AddItem/DAChartAddStatsKdeplot2dWidget.h"
#include "Chart/AddItem/DAChartAddStatsBoxplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsHeatmapWidget.h"
#include "Chart/AddItem/DAChartAddStatsScatterplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsBarplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsRegplotWidget.h"
#include "Chart/AddItem/DAChartAddStatsEcdfplotWidget.h"
#include "Chart/AddItem/DAChartSeriesSelectWidget.h"
#include "Chart/AddItem/DAAbstractStatsChartAddWidget.h"
#include "Chart/DAStatsPlotCoordinator.h"
// 类型化统计绘图请求（onStatsGuideAccepted 消费字段，需完整类型）
#include "Chart/DAStatsPlotRequest.h"
#include "Chart/DAStatsPlotRequests.h"
// 数据统计分析浮动 dock（PMF connect notifyDataChanged 需完整类型）
#include "Chart/DADataStatsWidget.h"
// 表内快速绘图（转入绘图区的编排器）
#include "Table/Chart/DAChartQuickPlot.h"
#include "Table/Chart/DATableChartTypes.h"
// Agent
#include "DAAgentModule.h"
#include "DAAgentInterface.h"        // PMF connect 到接口信号/方法需完整类型
#include "DAAgentDockWidget.h"       // DAAppDockingArea 仅前向声明；PMF connect 需完整类型（plan-02）
#include "DAAgentLinkDispatcher.h"   // Agent 本地跳转链接分发（da-<kind>: 协议注册表）
#include "DADataManagerInterface.h"  // da-data: 链接按名字/id 查找数据集
//
#include "SettingPages/DAAppConfig.h"
#include "DAAppLayoutManager.h"
#include "DALogCategory.h"

#ifndef DAAPPRIBBONAREA_WINDOW_NAME
#define DAAPPRIBBONAREA_WINDOW_NAME QCoreApplication::translate("DAAppController", "DA", nullptr)
#endif

// #ifndef DAAPPRIBBONAREA_WINDOW_NAME
// #define DAAPPRIBBONAREA_WINDOW_NAME QCoreApplication::translate("DAAppController", u8"试验数据分析系统", nullptr)
// #endif

// 未实现的功能标记
#define DAAPPCONTROLLER_PASS()                                                                                         \
    QMessageBox::warning(                                                                                              \
        app(),                                                                                                         \
        QCoreApplication::translate("DAAppRibbonArea", "Warning", nullptr),                                             \
        QCoreApplication::translate("DAAppRibbonArea",                                                                 \
                                    "The current function is not implemented, only the UI is reserved, "               \
                                    "please pay attention: https://gitee.com/czyt1988/data-work-flow",                 \
                                    nullptr))

// 快速链接信号槽
#define DAAPPCONTROLLER_ACTION_BIND(actionname, functionname)                                                          \
    connect(actionname, &QAction::triggered, this, &DAAppController::functionname)


namespace DA
{

/**
 * @brief 基本绑定
 * @note 在setDockAreaInterface函数中还有很多绑定操作
 */
void DAAppController::initConnection()
{
    // Main Category
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionNewProject, newProject);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionOpen, open);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionSave, save);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionSaveAs, saveAs);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionAppendProject, onActionAppendProjectTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionOpenMarkdown, onActionOpenMarkdownTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionSetting, onActionSettingTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionPluginManager, onActionPluginManagerTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionAbout, onActionAboutTriggered);
    // Data Category
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionAddData, onActionAddDataTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionRemoveData, onActionRemoveDataTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionRenameData, onActionRenameDataTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionExportData, onActionExportDataTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionExportDataCsv, onActionExportDataAsTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionExportDataExcel, onActionExportDataAsTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionExportDataPickle, onActionExportDataAsTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionExportDataParquet, onActionExportDataAsTriggered);
    // Chart Category
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionAddFigure, onActionAddFigureTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionFigureNewXYAxis, onActionFigureNewXYAxisTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddCurve, onActionChartAddCurveTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddScatter2D, onActionChartAddScatterTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddBar, onActionChartAddBarTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddErrorBar, onActionactionChartAddErrorBarTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddBoxPlot, onActionChartAddBoxPlotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddCloudMap, onActionChartAddCloudMapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddMultiBar, onActionChartAddMultiBarTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddHistogramBar, onActionChartAddHistogramTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddContourMap, onActionChartAddContourMapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAddVectorfield, onActionChartAddVectorfieldTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAdd3DSurface, onActionChartAdd3DSurfaceTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAdd3DBar, onActionChartAdd3DBarTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartAdd3DLine, onActionChartAdd3DLineTriggered);

    // Stats Plot
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsHistplot, onActionStatsHistplotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsKdeplot1d, onActionStatsKdeplot1dTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsKdeplot2d, onActionStatsKdeplot2dTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsBoxplot, onActionStatsBoxplotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsHeatmap, onActionStatsHeatmapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsScatterplot, onActionStatsScatterplotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsBarplot, onActionStatsBarplotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsRegplot, onActionStatsRegplotTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionStatsECDFplot, onActionStatsECDFplotTriggered);

    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableGrid, onActionChartEnableGridTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableGridX, onActionChartEnableGridXTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableGridY, onActionChartEnableGridYTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableGridXMin, onActionChartEnableGridXMinEnableTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableGridYMin, onActionChartEnableGridYMinTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableZoom, onActionChartEnableZoomTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartZoomIn, onActionChartZoomInTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartZoomOut, onActionChartZoomOutTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartZoomAll, onActionChartZoomAllTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnablePan, onActionChartEnablePanTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartDisableZoomX, onActionChartDisableZoomXTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartDisableZoomY, onActionChartDisableZoomYTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnablePickerCross, onActionChartEnablePickerCrossTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnablePickerY, onActionChartEnablePickerYTriggered);
    connect(mActions->actionGroupChartPickerTextRegion,
            &QActionGroup::triggered,
            this,
            &DAAppController::onActionGroupChartPickerTextRegionTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartYPickerShowXValueEnabled,
                                onActionChartYPickerShowXValueEnabledTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnablePickerXY, onActionChartEnablePickerXYTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartLinkAllPickerEnabled, onActionChartLinkAllPickerEnabledTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartEnableLegend, onActionChartEnableLegendTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionCopyFigureInClipboard, onActionCopyFigureToClipboardTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionChartDataPickerSetting, onActionChartDataPickerSettingTriggered);
    // 图例位置菜单（挂在actionChartEnableLegend下）
    connect(mActions->actionGroupChartLegendPosition,
            &QActionGroup::triggered,
            this,
            &DAAppController::onActionGroupChartLegendPositionTriggered);
    for (QAction* act : std::as_const(mActions->actionListOfColorTheme)) {
        connect(act, &QAction::triggered, this, [ this, act ]() { onActionGroupFigureThemeTriggered(act); });
    }
    connect(mActions->actionGroupChartEditor, &QActionGroup::triggered, this, &DAAppController::onActionGroupChartEditorTriggered);

    // View Category
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowWorkFlowArea, onActionShowWorkFlowAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowWorkFlowManagerArea, onActionShowWorkFlowManagerAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowChartArea, onActionShowChartAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowChartManagerArea, onActionShowChartManagerAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowDataArea, onActionShowDataAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowDataManagerArea, onActionShowDataManagerAreaTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowMessageLogView, onActionShowMessageLogViewTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowUndoHistory, onActionShowUndoHistoryTriggered);  // 005 T1.21 撤销历史面板
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowSettingWidget, onActionSettingWidgetTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowRightSideBar, onActionShowRightSideBarTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowLeftSideBar, onActionShowLeftSideBarTriggered);
    // Markdown 查看器（懒创建 dock：已创建时 raise；未打开过文件时弹文件选择框）
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionShowMarkdownView, onActionShowMarkdownViewTriggered);
    // workflow view 工作流视图
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowViewReadOnly, onActionWorkflowViewReadOnlyTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowViewMarker, onActionWorkflowViewMarkerTriggered);
    // workflow edit 工作流编辑
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowNew, onActionNewWorkflowTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowEnableItemLinkageMove,
                                onActionWorkflowEnableItemLinkageMoveTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowLinkEnable, onActionWorkflowLinkEnableTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionItemGrouping, onActionItemGroupingTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionItemUngroup, onActionItemUngroupTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowRun, onActionRunCurrentWorkflowTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowTerminate, onActionTerminateCurrentWorkflowTriggered);
    // workflow edit 工作流编辑/data edit 绘图编辑
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowStartDrawRect, onActionStartDrawRectTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowStartDrawText, onActionStartDrawTextTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowAddBackgroundPixmap, onActionAddBackgroundPixmapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowLockBackgroundPixmap, onActionLockBackgroundPixmapTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionWorkflowEnableItemMoveWithBackground,
                                onActionEnableItemMoveWithBackgroundTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionExportWorkflowSceneToPNG, onActionExportWorkflowScenePNGTriggered);
    // 导出图片大按钮（默认 action）绑定同一个 PNG 导出槽，避免点击无反应
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionExportWorkflowSceneToImage, onActionExportWorkflowScenePNGTriggered);
    //===================================================
    // 主页剪贴板（按焦点路由）
    //===================================================
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionCut, onActionCutTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionCopy, onActionCopyTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionPaste, onActionPasteTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionDelete, onActionDeleteTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionSelectAll, onActionSelectAllTriggered);
    // 视图页-布局（外观面板已删除，Ribbon 主题切换由 设置→常规 页提供）
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionResetDefaultLayout, onActionResetDefaultLayoutTriggered);
    DAAPPCONTROLLER_ACTION_BIND(mActions->actionManageLayouts, onActionManageLayoutsTriggered);
    //===================================================
    // setDockAreaInterface 有其他的绑定
    //===================================================
    //! 注意！！
    //! 在setDockAreaInterface函数中还有很多绑定操作
    //
    DAAppProject* p = mCore->getAppProject();
    if (p) {
        connect(p, &DAAppProject::projectSaved, this, &DAAppController::onProjectSaved);
        connect(p, &DAAppProject::projectLoaded, this, &DAAppController::onProjectLoaded);
        connect(p, &DAAppProject::dirtyStateChanged, this, &DAAppController::onProjectDirtyStateChanged);
    }

    //===================================================
    // workflow窗口字体相关信号槽
    //===================================================

    connect(mRibbon, &DAAppRibbonArea::selectedWorkflowItemFont, this, &DAAppController::onCurrentWorkflowFontChanged);
    connect(mRibbon, &DAAppRibbonArea::selectedWorkflowItemFontColor, this, &DAAppController::onCurrentWorkflowFontColorChanged);
    connect(mRibbon,
            &DAAppRibbonArea::selectedWorkflowItemBrush,
            this,
            &DAAppController::onCurrentWorkflowShapeBackgroundBrushChanged);
    connect(mRibbon, &DAAppRibbonArea::selectedWorkflowItemPen, this, &DAAppController::onCurrentWorkflowShapeBorderPenChanged);

    //===================================================
    // name
    //===================================================
    connect(mDock->dockManager(), &ads::CDockManager::focusedDockWidgetChanged, this, &DAAppController::onFocusedDockWidgetChanged);
    // 程序化raise dock窗口时也激活context category
    connect(mDock, &DADockingAreaInterface::dockWidgetRaised, this, &DAAppController::onDockWidgetRaised);
    //===================================================
    // 统计窗口（DADataStatsWidget）
    //===================================================
    // 「统计」上下文标签页只跟随统计 dock 的可见性（不跟随焦点，否则点 Ribbon 上的
    // 统计按钮会因焦点离开浮动 dock 而让标签页消失，动作根本点不到）
    if (mRibbon) {
        connect(mDock,
                &DAAppDockingArea::dataStatsDockVisibilityChanged,
                mRibbon,
                &DAAppRibbonArea::setStatsContextCategoryVisible);
    }
    // 「转入绘图区」：DAGui 的统计窗口不持有绘图区，由 APP 注入绘图区容器
    if (DADataStatsWidget* statsWidget = mDock->getDataStatsWidget()) {
        connect(statsWidget,
                &DADataStatsWidget::transferToFigureRequested,
                this,
                &DAAppController::onStatsTransferToFigureRequested);
    }
    // DADataManageWidget 数据操作
    // DADataOperateWidget
    DADataOperateWidget* dow = mDock->getDataOperateWidget();
    connect(dow, &DADataOperateWidget::dataTableCreated, this, &DAAppController::onDataOperatePageCreated);
    // 撤销历史面板（005 T1.21）：QUndoGroup 由 APP 注入（DAGui 不依赖 DAInterface，D0 分层）；
    // 深度初值读配置；面板调深度 → 写回配置 + 施加全部已打开数据页（R19）
    if (DAUndoHistoryWidget* uh = mDock->getUndoHistoryWidget()) {
        if (mCommand) {
            uh->setUndoGroup(&mCommand->undoGroup());
        }
        if (mConfig) {
            const int cfgLimit = mConfig->ui().undoStackLimit;
            if (cfgLimit > 0) {
                uh->setUndoLimit(cfgLimit);
            }
        }
        connect(uh, &DAUndoHistoryWidget::undoLimitChangeRequested, this, &DAAppController::onUndoHistoryLimitChangeRequested);
    }
    // DAChartManager
    DAChartManageWidget* cmw = mDock->getChartManageWidget();
    connect(cmw, &DAChartManageWidget::figureElementClicked, this, &DAAppController::onFigureElementClicked);
    connect(cmw, &DAChartManageWidget::figureElementDbClicked, this, &DAAppController::onFigureElementDbClicked);
    // figure 窗口设置按钮信号
    connect(cmw, &DAChartManageWidget::requestFigureSetting, this, [ this ](DA::DAFigureWidget* fig) {
        DASettingContainerWidget* setting = getSettingContainerWidget();
        if (setting) {
            DAFigureWidgetSettingPanel* panel = setting->getFigureWidgetSettingWidget();
            if (panel) {
                panel->setTarget(fig);
                setting->showFigureWidgetSettingWidget();
            }
        }
        // 显示设置窗口 dock
        mDock->raiseDockByWidget((QWidget*)(mDock->getSettingContainerWidget()));
    });
    // 坐标轴设置面板的可见性变化 -> 刷新DAChartManageWidget树形控件的可见性列
    if (DASettingContainerWidget* setting = getSettingContainerWidget()) {
        if (DAChartSettingWidget* chartSetting = setting->getChartSettingWidget()) {
            connect(chartSetting, &DAChartSettingWidget::axisVisibilityChanged, cmw, &DAChartManageWidget::refreshAxisVisibility);
        }
    }
    // DAChartOperateWidget
    DAChartOperateWidget* cow = mDock->getChartOperateWidget();
    connect(cow, &DAChartOperateWidget::figureCreated, this, &DAAppController::onFigureCreated);
    connect(cow, &DAChartOperateWidget::currentFigureChanged, this, &DAAppController::onCurrentFigureChanged);
    // 工程加载图表恢复完成 -> 数据联动表按探针重建（纯视图架构：探针已由图表序列化恢复）
    connect(cow, &DAChartOperateWidget::figureCreated, this, [this](DAFigureWidget* fig) {
        if (DADataLinkTableWidget* dlt = mDock->getDataLinkTableWidget()) {
            dlt->refreshFigure(fig);
        }
    });
    // 绘图项创建完成时提升绘图 dock，让用户能看到新绘图
    if (DAAppChartOperateWidget* appCow = qobject_cast< DAAppChartOperateWidget* >(cow)) {
        connect(appCow, &DAAppChartOperateWidget::plotItemCreated, this, &DAAppController::onPlotItemCreated);
        connect(appCow, &DAAppChartOperateWidget::plot3DItemCreated, this, &DAAppController::onPlot3DItemCreated);
    }
    //
    DAPyWorkFlowOperateWidget* workflowOpt = mDock->getWorkFlowOperateWidget();
    // 鼠标动作完成的触发
    connect(workflowOpt,
            &DAPyWorkFlowOperateWidget::sceneActionDeactived,
            this,
            &DAAppController::onWorkFlowGraphicsSceneActionDeactive);
    connect(workflowOpt,
            &DAPyWorkFlowOperateWidget::selectionItemChanged,
            this,
            &DAAppController::onWorkflowSceneSelectionItemChanged);
    connect(workflowOpt,
            &DAPyWorkFlowOperateWidget::currentWorkFlowWidgetChanged,
            this,
            &DAAppController::onCurrentWorkflowWidgetChanged);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::workflowStartExecute, this, &DAAppController::onWorkflowStartExecute);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::workflowFinished, this, &DAAppController::onWorkflowFinished);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::itemsAdded, this, &DAAppController::onWorkflowSceneitemsAdded);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::itemsRemoved, this, &DAAppController::onWorkflowSceneitemsRemoved);
    connect(mActions->actionWorkflowShowGrid,
            &QAction::triggered,
            workflowOpt,
            &DAPyWorkFlowOperateWidget::setCurrentWorkflowShowGrid);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::workflowCreated, this, &DAAppController::onWorkflowCreated);

    connect(mActions->recentFilesManager, &DARecentFilesManager::fileSelected, this, &DAAppController::onRecentFileSelected);
}

/**
 * @brief 初始化Python工作流信号槽连接
 *
 * 此函数在initialize()中调用，用于设置Python工作流相关的
 * 信号槽连接。当前为占位实现，待DAPyWorkFlowScene完整集成后添加具体连接逻辑。
 */
void DAAppController::initPyWorkflowConnections()
{
    // TODO: 在DAPyWorkFlowScene编辑器集成后完善信号槽连接
    // 当前Action的信号槽已在initConnection中通过DAAPPCONTROLLER_ACTION_BIND宏绑定
    qDebug() << "Python workflow connections initialized (placeholder)";
}

}  // end DA
