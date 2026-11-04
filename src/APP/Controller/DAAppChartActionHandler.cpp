#include "DAAppChartActionHandler.h"
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

// 图表域动作分发器：添加绘图/坐标系/各类图表与统计图、图表属性开关、图表生命周期
#define DAAPP_CHART_BIND(actionname, functionname) \
    connect(actionname, &QAction::triggered, this, &DAAppChartActionHandler::functionname)

namespace DA
{

DAAppChartActionHandler::DAAppChartActionHandler(const DAAppActionHandlerDeps& deps, QObject* par)
    : QObject(par), mDeps(deps)
{
}

void DAAppChartActionHandler::initConnections()
{
    // Chart Category
    DAAPP_CHART_BIND(mDeps.actions->actionAddFigure, onActionAddFigureTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionFigureNewXYAxis, onActionFigureNewXYAxisTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAddCurve, onActionChartAddCurveTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAddScatter2D, onActionChartAddScatterTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAddBar, onActionChartAddBarTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAddErrorBar, onActionactionChartAddErrorBarTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAddBoxPlot, onActionChartAddBoxPlotTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAddCloudMap, onActionChartAddCloudMapTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAddMultiBar, onActionChartAddMultiBarTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAddHistogramBar, onActionChartAddHistogramTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAddContourMap, onActionChartAddContourMapTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAddVectorfield, onActionChartAddVectorfieldTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAdd3DSurface, onActionChartAdd3DSurfaceTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAdd3DBar, onActionChartAdd3DBarTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartAdd3DLine, onActionChartAdd3DLineTriggered);
    // Stats Plot
    DAAPP_CHART_BIND(mDeps.actions->actionStatsHistplot, onActionStatsHistplotTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionStatsKdeplot1d, onActionStatsKdeplot1dTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionStatsKdeplot2d, onActionStatsKdeplot2dTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionStatsBoxplot, onActionStatsBoxplotTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionStatsHeatmap, onActionStatsHeatmapTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionStatsScatterplot, onActionStatsScatterplotTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionStatsBarplot, onActionStatsBarplotTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionStatsRegplot, onActionStatsRegplotTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionStatsECDFplot, onActionStatsECDFplotTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnableGrid, onActionChartEnableGridTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnableGridX, onActionChartEnableGridXTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnableGridY, onActionChartEnableGridYTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnableGridXMin, onActionChartEnableGridXMinEnableTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnableGridYMin, onActionChartEnableGridYMinTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnableZoom, onActionChartEnableZoomTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartZoomIn, onActionChartZoomInTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartZoomOut, onActionChartZoomOutTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartZoomAll, onActionChartZoomAllTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnablePan, onActionChartEnablePanTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartDisableZoomX, onActionChartDisableZoomXTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartDisableZoomY, onActionChartDisableZoomYTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnablePickerCross, onActionChartEnablePickerCrossTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnablePickerY, onActionChartEnablePickerYTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnablePickerXY, onActionChartEnablePickerXYTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartLinkAllPickerEnabled, onActionChartLinkAllPickerEnabledTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartEnableLegend, onActionChartEnableLegendTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionCopyFigureInClipboard, onActionCopyFigureToClipboardTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartDataPickerSetting, onActionChartDataPickerSettingTriggered);
    // 拾取器文字位置
    connect(mDeps.actions->actionGroupChartPickerTextRegion,
            &QActionGroup::triggered,
            this,
            &DAAppChartActionHandler::onActionGroupChartPickerTextRegionTriggered);
    DAAPP_CHART_BIND(mDeps.actions->actionChartYPickerShowXValueEnabled,
                     onActionChartYPickerShowXValueEnabledTriggered);
    // 图例位置菜单（挂在 actionChartEnableLegend 下）
    connect(mDeps.actions->actionGroupChartLegendPosition,
            &QActionGroup::triggered,
            this,
            &DAAppChartActionHandler::onActionGroupChartLegendPositionTriggered);
    // 颜色主题
    for (QAction* act : std::as_const(mDeps.actions->actionListOfColorTheme)) {
        connect(act, &QAction::triggered, this, [ this, act ]() { onActionGroupFigureThemeTriggered(act); });
    }
    // 图表编辑器工具组
    connect(mDeps.actions->actionGroupChartEditor,
            &QActionGroup::triggered,
            this,
            &DAAppChartActionHandler::onActionGroupChartEditorTriggered);

    // 绘图管理树：元素点击/双击与 figure 设置按钮
    DAChartManageWidget* cmw = mDeps.dock->getChartManageWidget();
    connect(cmw, &DAChartManageWidget::figureElementClicked, this, &DAAppChartActionHandler::onFigureElementClicked);
    connect(cmw, &DAChartManageWidget::figureElementDbClicked, this, &DAAppChartActionHandler::onFigureElementDbClicked);
    connect(cmw, &DAChartManageWidget::requestFigureSetting, this, [ this ](DA::DAFigureWidget* fig) {
        DASettingContainerWidget* setting = mDeps.controller->getSettingContainerWidget();
        if (setting) {
            DAFigureWidgetSettingPanel* panel = setting->getFigureWidgetSettingWidget();
            if (panel) {
                panel->setTarget(fig);
                setting->showFigureWidgetSettingWidget();
            }
        }
        // 显示设置窗口 dock
        mDeps.dock->raiseDockByWidget((QWidget*)(mDeps.dock->getSettingContainerWidget()));
    });

    // 绘图操作窗口
    DAChartOperateWidget* cow = mDeps.dock->getChartOperateWidget();
    connect(cow, &DAChartOperateWidget::figureCreated, this, &DAAppChartActionHandler::onFigureCreated);
    connect(cow, &DAChartOperateWidget::currentFigureChanged, this, &DAAppChartActionHandler::onCurrentFigureChanged);
    // 工程加载图表恢复完成 -> 数据联动表按探针重建（纯视图架构：探针已由图表序列化恢复）
    connect(cow, &DAChartOperateWidget::figureCreated, this, [ this ](DAFigureWidget* fig) {
        if (DADataLinkTableWidget* dlt = mDeps.dock->getDataLinkTableWidget()) {
            dlt->refreshFigure(fig);
        }
    });
    // 绘图项创建完成时提升绘图 dock，让用户能看到新绘图
    if (DAAppChartOperateWidget* appCow = qobject_cast< DAAppChartOperateWidget* >(cow)) {
        connect(appCow, &DAAppChartOperateWidget::plotItemCreated, this, &DAAppChartActionHandler::onPlotItemCreated);
    }

    // 「转入绘图区」：DAGui 的统计窗口不持有绘图区，由 APP 注入绘图区容器
    if (DADataStatsWidget* statsWidget = mDeps.dock->getDataStatsWidget()) {
        connect(statsWidget, &DADataStatsWidget::transferToFigureRequested, this,
                &DAAppChartActionHandler::onStatsTransferToFigureRequested);
    }
}
/**
 * @brief 绘图元素选中
 *
 * 信号由DAChartManageWidget发出
 * @param selection
 */
void DAAppChartActionHandler::onFigureElementClicked(const DAFigureElementSelection& selection)
{
    // 这里和设置页面同步
    DASettingContainerWidget* setting = mDeps.controller->getSettingContainerWidget();
    if (!setting) {
        return;
    }

    // === 3D 选择分支 ===
    if (selection.isSelectedPlot3D() || selection.isSelectedPlot3DItem()) {
        // 切换到 3D 设置面板
        DAChart3DSettingWidget* chart3DSetting = setting->getChart3DSettingWidget();
        if (!chart3DSetting) {
            return;
        }
        chart3DSetting->setSelection(selection);
        setting->showChart3DSettingWidget();
        // 如果选中了 3D plot，切换 figure 的 current 3D chart
        if (selection.isSelectedPlot3D()) {
            selection.figureWidget->setCurrent3DChart(selection.plot3D);
        }
        // 唤起设置停靠窗（隐藏/被覆盖时用户否则看不到任何反馈）
        mDeps.dock->raiseDockByWidget((QWidget*)(setting));
        return;
    }

    // === 2D 选择分支 ===
    // 引入 3D 面板后，2D 分支必须显式切换回 2D 面板，
    // 否则用户从 3D 选中切换到 2D 时面板仍停留在 3D 设置。
    DAChartSettingWidget* chartSetting = setting->getChartSettingWidget();
    if (!chartSetting) {
        return;
    }
    setting->showChartSettingWidget();
    chartSetting->setSelection(selection);
    // 唤起设置停靠窗（隐藏/被覆盖时用户否则看不到任何反馈）
    mDeps.dock->raiseDockByWidget((QWidget*)(setting));
    if (selection.isSelectedPlot()) {
        // 单独选中plot，那么把figure的current plot进行切换
        if (selection.plot->isHostPlot()) {
            selection.figureWidget->setCurrentChart(selection.plot);
        } else {
            selection.figureWidget->setCurrentChart(selection.plot->hostPlot());
        }
    }
    // setSelection 在 plot 未变时不会调用 updateUI，需显式刷新坐标轴面板的可见性 checkbox，
    // 确保右键菜单等途径修改可见性后面板状态同步
    if (selection.isSelectedScaleWidget()) {
        if (DAChartAxisSettingPanel* p = chartSetting->getChartAxisSetWidget(selection.axisId)) {
            p->updateUI();
        }
    }
}

/**
 * @brief 双击显示绘图和设置对话框
 * @param selection
 */
void DAAppChartActionHandler::onFigureElementDbClicked(const DAFigureElementSelection& selection)
{
    // === 3D 双击：与单击一致，同步设置面板并提升 dock ===
    if (selection.isSelectedPlot3D() || selection.isSelectedPlot3DItem()) {
        onFigureElementClicked(selection);
        DASettingContainerWidget* setting = mDeps.controller->getSettingContainerWidget();
        if (setting) {
            setting->showChart3DSettingWidget();
        }
        mDeps.dock->raiseDockingArea(DADockingAreaInterface::DockingAreaChartOperate);
        return;
    }

    // === 2D 逻辑 ===
    // 和单击事件一致
    onFigureElementClicked(selection);
    // 同步查看是否设置对话框在前台
    DASettingContainerWidget* setting = mDeps.controller->getSettingContainerWidget();
    if (setting) {
        setting->showChartSettingWidget();
    }
    // 把绘图界面显示在前台
    mDeps.dock->raiseDockingArea(DADockingAreaInterface::DockingAreaChartOperate);
    // 如果是visible双击，那么改变item的visible属性

    if (DAFigureElementSelection::ColumnVisible == selection.selectionColumn) {
        DAChartManageWidget* cmw = mDeps.dock->getChartManageWidget();
        if (selection.isSelectedPlotItem()) {
            selection.plotItem->setVisible(!selection.plotItem->isVisible());
            // plotitem可见性改变后，需要通知刷新
            selection.plot->replot();
            // 刷新树形控件的可见性列显示
            if (cmw) {
                cmw->refreshPlotItemVisibility(selection.plotItem);
            }
        } else if (selection.isSelectedScaleWidget()) {
            bool isAxisVisible = selection.plot->isAxisVisible(selection.axisId);
            // 优先走 DAChartWidget::setAxisVisible 以触发 AxisVisibilityChanged 通知（联动探针徽章重算）
            if (DAChartWidget* dacw = qobject_cast< DAChartWidget* >(selection.plot)) {
                dacw->setAxisVisible(selection.axisId, !isAxisVisible);
            } else {
                selection.plot->setAxisVisible(selection.axisId, !isAxisVisible);
            }
            // 对于设置窗口要进行更新
            if (setting) {
                if (DAChartSettingWidget* chartSetting = setting->getChartSettingWidget()) {
                    if (DAChartAxisSettingPanel* axisSettingWidget = chartSetting->getChartAxisSetWidget(selection.axisId)) {
                        axisSettingWidget->updateUI();
                    }
                }
            }
            // 刷新树形控件的可见性列显示
            if (cmw) {
                cmw->refreshAxisVisibility(selection.plot, selection.axisId);
            }
        }
    }
}

/**
 * @brief 新fig创建
 * @param f
 */
void DAAppChartActionHandler::onFigureCreated(DAFigureWidget* f)
{
    if (nullptr == f) {
        return;
    }
    if (mDeps.command) {
        QUndoStack* stack = f->getUndoStack();
        mDeps.command->addStack(stack);
        stack->setActive();
    }
    // updateFigureAboutRibbon(f);//在onActionAddFigureTriggered中调用了
    mDeps.controller->setDirty();
    connect(f, &DAFigureWidget::currentChartChanged, this, &DAAppChartActionHandler::onCurrentChartChanged);
    connect(f, &DAFigureWidget::chartEditorStatusChanged, this, &DAAppChartActionHandler::onChartEditorStatusChanged);
    // 指针工具在图中选中元素 → 联动属性设置面板（与树形控件点击同链路）
    connect(f, &DAFigureWidget::figureElementClicked, this, &DAAppChartActionHandler::onFigureElementClicked);
}

/**
 * @brief 当前的fig变化
 * @param f
 * @param index
 */
void DAAppChartActionHandler::onCurrentFigureChanged(DAFigureWidget* f, int index)
{
    Q_UNUSED(index);
    if (nullptr == f) {
        return;
    }
    qDebug() << "DAAppChartActionHandler::onCurrentFigureChanged";
    mDeps.ribbon->updateFigureAboutRibbon(f);
    QUndoStack* stack = f->getUndoStack();
    if (stack) {
        stack->setActive();
    }
}

/**
 * @brief 当前的chart改变
 * @param c
 */
void DAAppChartActionHandler::onCurrentChartChanged(DAChartWidget* c)
{
    if (nullptr == c) {
        return;
    }
    mDeps.ribbon->updateChartAboutRibbon(c);
}

/**
 * @brief 图表编辑器状态改变
 * @param status
 */
void DAAppChartActionHandler::onChartEditorStatusChanged(DAFigureWidget::ChartEditorStatus status)
{
    if (DAFigureWidget::ChartEditorStatus::EndEdit == status) {
        // 结束编辑
        // 触发结束编辑有可能是按键，或一些异常状态，为了避免和按钮的状态不同步，这里结束编辑后要把actiongroup管理的action都设置为unchecked
        const QList< QAction* > actions = mDeps.actions->actionGroupChartEditor->actions();
        for (QAction* a : actions) {
            if (a->isChecked()) {
                a->setChecked(false);
            }
        }
    }
}

/**
 * @brief 绘图项创建完成（DADialogChartGuide 确认后）
 *
 * 此时用户已在绘图引导对话框中确认添加绘图，提升绘图 dock 让用户看到新创建的绘图
 * @param fig 绘图窗口
 * @param plot 图表
 * @param item 创建的绘图项
 */
void DAAppChartActionHandler::onPlotItemCreated(DAFigureWidget* f, DAChartWidget* plot, QwtPlotItem* item)
{
    Q_UNUSED(f);
    Q_UNUSED(plot);
    Q_UNUSED(item);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}


/**
 * @brief 添加一个figure
 */
void DAAppChartActionHandler::onActionAddFigureTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    // 添加绘图
    DAFigureWidget* fig = chartopt->createFigure();
    // 这里不需要回退
    DAChartWidget* chart = fig->createChart();
    chart->setAxisLabel(QwtAxis::XBottom, "x");
    chart->setAxisLabel(QwtAxis::YLeft, "y");
    // 把fig的undostack添加
    mDeps.command->addStack(fig->getUndoStack());
    mDeps.ribbon->updateFigureAboutRibbon(fig);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    chartopt->setCurrentFigure(fig);
    mDeps.controller->setDirty();
}

/**
 * @brief 统计窗口「转入绘图区」
 *
 * DAGui 的统计窗口不持有绘图区（分层约束：DAGui 不能反向依赖 APP 的窗口编排），
 * 由 APP 在这里取出绘图区容器注入；建图与撤销宏都在统计窗口里完成。
 */
void DAAppChartActionHandler::onStatsTransferToFigureRequested()
{
    if (!mDeps.dock) {
        return;
    }
    DADataStatsWidget* statsWidget = mDeps.dock->getDataStatsWidget();
    DAChartOperateWidget* chartOperate = mDeps.dock->getChartOperateWidget();
    if (!statsWidget || !chartOperate) {
        return;
    }
    if (statsWidget->transferToFigure(chartOperate)) {
        // 转入后把绘图区抬到前面，用户能立刻看到新生成的绘图
        mDeps.dock->raiseDockingArea(DADockingAreaInterface::DockingAreaChartOperate);
    }
}

/**
 * @brief 新建一个坐标系
 */
void DAAppChartActionHandler::onActionFigureNewXYAxisTriggered()
{
    DAFigureWidget* fig = mDeps.controller->getCurrentFigure();
    if (!fig) {
        daWarning << QCoreApplication::translate("DA::DAAppController", "Before creating a new coordinate, you need to create a figure");  // cn:在创建一个坐标系之前，需要先创建一个绘图窗口
        return;
    }
    DAChartWidget* w = fig->createChart_(QRectF(0.1, 0.1, 0.4, 0.4));
    w->enableGrid();
    w->enablePan();
    //    w->addCurve({ 1, 2, 3, 4, 5 }, { 3, 5, 8, 0, -3 })->setTitle("curve1");
    //    w->addCurve({ 1, 2, 3, 4, 5 }, { 5, 7, 0, -1, 1 })->setTitle("curve2");
    mDeps.ribbon->updateChartAboutRibbon(w);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    mDeps.controller->setDirty();
}

/**
 * @brief 创建一个曲线
 */
void DAAppChartActionHandler::onActionChartAddCurveTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Curve);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加散点图
 */
void DAAppChartActionHandler::onActionChartAddScatterTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Scatter);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加柱状图
 */
void DAAppChartActionHandler::onActionChartAddBarTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Bar);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加误差棒图
 */
void DAAppChartActionHandler::onActionactionChartAddErrorBarTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::ErrorBar);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加误差棒图
 */
void DAAppChartActionHandler::onActionChartAddBoxPlotTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Box);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加谱图
 */
void DAAppChartActionHandler::onActionChartAddCloudMapTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Spectrogram);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加多重柱状图
 */
void DAAppChartActionHandler::onActionChartAddMultiBarTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::MultiBar);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加直方图
 */
void DAAppChartActionHandler::onActionChartAddHistogramTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Histogram);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加等高线图
 */
void DAAppChartActionHandler::onActionChartAddContourMapTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Contour);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加向量场图
 */
void DAAppChartActionHandler::onActionChartAddVectorfieldTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::VectorField);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加3D曲面图
 */
void DAAppChartActionHandler::onActionChartAdd3DSurfaceTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Surface3D);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加3D柱状图
 */
void DAAppChartActionHandler::onActionChartAdd3DBarTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Bar3D);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加3D线图
 */
void DAAppChartActionHandler::onActionChartAdd3DLineTriggered()
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Line3D);
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 统计直方图
 */
void DAAppChartActionHandler::onActionStatsHistplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsHistplot);
}

/**
 * @brief 一维核密度图
 */
void DAAppChartActionHandler::onActionStatsKdeplot1dTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsKdeplot1d);
}

/**
 * @brief 二维核密度图
 */
void DAAppChartActionHandler::onActionStatsKdeplot2dTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsKdeplot2d);
}

/**
 * @brief 统计箱线图
 */
void DAAppChartActionHandler::onActionStatsBoxplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsBoxplot);
}

/**
 * @brief 热力图
 */
void DAAppChartActionHandler::onActionStatsHeatmapTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsHeatmap);
}

/**
 * @brief 统计散点图
 */
void DAAppChartActionHandler::onActionStatsScatterplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsScatterplot);
}

/**
 * @brief 统计柱状图
 */
void DAAppChartActionHandler::onActionStatsBarplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsBarplot);
}

/**
 * @brief 回归图
 */
void DAAppChartActionHandler::onActionStatsRegplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsRegplot);
}

/**
 * @brief 经验累积分布图
 */
void DAAppChartActionHandler::onActionStatsECDFplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsECDFplot);
}

/**
 * @brief 确保当前 Figure 和 Chart 存在（不存在则创建）
 * @param fig 输出 Figure 指针
 * @param chart 输出 Chart 指针
 * @return 如果 fig 和 chart 都有效返回 true
 */
bool DAAppChartActionHandler::ensureFigureChart(DAFigureWidget*& fig, DAChartWidget*& chart)
{
    DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget();
    fig                               = chartopt->getCurrentFigure();
    if (!fig) {
        fig = chartopt->createFigure();
    }
    if (!fig) {
        return false;
    }
    chart = fig->getCurrentChart();
    if (!chart) {
        chart = fig->gca();
    }
    if (!chart) {
        chart = fig->createChart();
    }
    if (!chart) {
        return false;
    }
    return true;
}

/**
 * @brief 显示统计绘图引导对话框并预选指定类型
 * @param type 统计绘图类型
 */
void DAAppChartActionHandler::showStatsChartGuide(DA::DAChartTypes type)
{
    DAFigureWidget* fig  = nullptr;
    DAChartWidget* chart = nullptr;
    if (!ensureFigureChart(fig, chart)) {
        return;
    }
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);

    if (!mStatsChartGuideDlg) {
        mStatsChartGuideDlg = new DADialogStatsChartGuide(mDeps.controller->app());
        mStatsChartGuideDlg->setDataManager(mDeps.datas->dataManager());
        connect(mStatsChartGuideDlg, &DADialogStatsChartGuide::plotRequested, this, &DAAppChartActionHandler::onStatsGuideAccepted);
    }
    mStatsChartGuideDlg->setFigureWidget(fig);
    mStatsChartGuideDlg->setChartWidget(chart);
    mStatsChartGuideDlg->setCurrentChartType(type);
    mStatsChartGuideDlg->show();
    mStatsChartGuideDlg->raise();
    mStatsChartGuideDlg->activateWindow();
}

/**
 * @brief 允许网格
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnableGridTriggered(bool on)
{
    bool res = mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGrid(on);
        w->replot();
        return true;
    });
    if (res) {
        // grid生效与否决定了X，Y以及Xmin,Ymin能否显示
        mDeps.ribbon->updateChartGridAboutRibbon(mDeps.controller->getCurrentChart());
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 横向网格
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnableGridXTriggered(bool on)
{
    bool res = mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridX(on);
        w->replot();
        return true;
    });
    if (res) {
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 纵向网格
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnableGridYTriggered(bool on)
{
    bool res = mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridY(on);
        w->replot();
        return true;
    });
    if (res) {
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 横向密集网格
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnableGridXMinEnableTriggered(bool on)
{
    bool res = mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridXMin(on);
        w->replot();
        return true;
    });
    if (res) {
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 纵向密集网格
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnableGridYMinTriggered(bool on)
{
    bool res = mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridYMin(on);
        w->replot();
        return true;
    });
    if (res) {
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 当前图表允许缩放
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnableZoomTriggered(bool on)
{
    bool res = mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableZoom(on);
        return true;
    });
    if (res) {
        mDeps.ribbon->updateChartZoomPanAboutRibbon(mDeps.controller->getCurrentChart());
    }
}

/**
 * @brief 当前图表放大
 */
void DAAppChartActionHandler::onActionChartZoomInTriggered()
{
    mDeps.controller->applyToCharts([](DAChartWidget* w) -> bool {
        w->zoomIn();
        return true;
    });
}

/**
 * @brief 当前图表缩小
 */
void DAAppChartActionHandler::onActionChartZoomOutTriggered()
{
    mDeps.controller->applyToCharts([](DAChartWidget* w) -> bool {
        w->zoomOut();
        return true;
    });
}

/**
 * @brief 当前图表全部显示
 */
void DAAppChartActionHandler::onActionChartZoomAllTriggered()
{
    bool res = mDeps.controller->applyToCharts([](DAChartWidget* w) -> bool {
        w->rescaleAxes();
        w->replot();
        return true;
    });
    if (res) {
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 允许绘图拖动
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnablePanTriggered(bool on)
{
    bool res = mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enablePan(on);
        return true;
    });
    if (res) {
        mDeps.ribbon->updateChartZoomPanAboutRibbon(mDeps.controller->getCurrentChart());
    }
}

/**
 * @brief 禁止水平缩放
 * @param on 选中时禁止x轴参与缩放
 */
void DAAppChartActionHandler::onActionChartDisableZoomXTriggered(bool on)
{
    mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableXAxisZoom(!on);
        return true;
    });
}

/**
 * @brief 禁止垂直缩放
 * @param on 选中时禁止y轴参与缩放
 */
void DAAppChartActionHandler::onActionChartDisableZoomYTriggered(bool on)
{
    mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableYAxisZoom(!on);
        return true;
    });
}

/**
 * @brief 允许绘图拾取
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnablePickerCrossTriggered(bool on)
{
    mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableCrosshair(on);
        return true;
    });
}

/**
 * @brief 允许绘图拾取Y
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnablePickerYTriggered(bool on)
{
    bool res = mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableYValuePicking(on);
        return true;
    });
    if (res) {
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 设置pick的区域
 * @param act
 */
void DAAppChartActionHandler::onActionGroupChartPickerTextRegionTriggered(QAction* act)
{
    mDeps.controller->applyToCharts([ act ](DAChartWidget* w) -> bool {
        QwtPlotSeriesDataPicker* picker = w->getDataPicker();
        QwtPlotSeriesDataPicker::TextPlacement tp =
            static_cast< QwtPlotSeriesDataPicker::TextPlacement >(act->data().toInt());
        if (picker) {
            picker->setTextArea(tp);
        }
        return true;
    });
}

/**
 * @brief 允许绘图拾取XY
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnablePickerXYTriggered(bool on)
{
    bool res = mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableXYValuePicking(on);
        return true;
    });
    if (res) {
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 连接所有picker
 * @param on
 */
void DAAppChartActionHandler::onActionChartLinkAllPickerEnabledTriggered(bool on)
{
    DAFigureWidget* fig = mDeps.controller->getCurrentFigure();
    if (fig) {
        fig->setDataPickerGroupEnabled(on);
    }
}

/**
 * @brief
 * @param on
 */
void DAAppChartActionHandler::onActionChartYPickerShowXValueEnabledTriggered(bool on)
{
    mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        QwtPlotSeriesDataPicker* picker = w->getDataPicker();
        if (picker) {
            picker->setEnableShowXValue(on);
        }
        return true;
    });
}

/**
 * @brief 允许图例
 * @param on
 */
void DAAppChartActionHandler::onActionChartEnableLegendTriggered(bool on)
{
    bool res = mDeps.controller->applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableLegend(on);
        w->replot();
        return true;
    });
    if (res) {
        mDeps.controller->setDirty();
    }
}

/**
 * @brief 绘图样式选择
 * @param act
 */
void DAAppChartActionHandler::onActionGroupFigureThemeTriggered(QAction* act)
{
    DAColorTheme::ColorThemeStyle style = static_cast< DAColorTheme::ColorThemeStyle >(act->data().toInt());
    DAColorTheme theme(style);
    DAFigureWidget* fig = mDeps.controller->getCurrentFigure();
    if (!fig) {
        return;
    }
    fig->setColorTheme(theme);
}

void DAAppChartActionHandler::onActionCopyFigureToClipboardTriggered()
{
    DAFigureWidget* fig = mDeps.controller->getCurrentFigure();
    if (!fig) {
        return;
    }
    fig->copyToClipboard();
}

/**
 * @brief 图例位置菜单切换（挂在actionChartEnableLegend下的上/下/左/右互斥组）
 */
void DAAppChartActionHandler::onActionGroupChartLegendPositionTriggered(QAction* act)
{
    DAChartWidget* chart = mDeps.controller->getCurrentChart();
    if (!chart || !act || !act->isChecked()) {
        return;
    }
    bool isok  = false;
    int alignV = act->data().toInt(&isok);
    if (!isok) {
        return;
    }
    chart->setLegendPosition(static_cast< Qt::Alignment >(alignV));
    chart->replot();
    mDeps.controller->setDirty();
}

void DAAppChartActionHandler::onActionChartDataPickerSettingTriggered()
{
    DAFigureWidget* fig = mDeps.controller->getCurrentFigure();
    if (!fig) {
        return;
    }
    DAChartWidget* chart = fig->getCurrentChart();
    if (!chart) {
        chart = fig->gca();
    }
    if (!chart) {
        return;
    }
    DASettingContainerWidget* setting = mDeps.controller->getSettingContainerWidget();
    if (!setting) {
        return;
    }
    DAChartSettingWidget* chartSetting = setting->getChartSettingWidget();
    if (!chartSetting) {
        return;
    }
    chartSetting->setPlot(chart);
    chartSetting->showDataPickerSetting();
    setting->showChartSettingWidget();
    mDeps.dock->raiseDockByWidget((QWidget*)(mDeps.dock->getSettingContainerWidget()));
}

void DAAppChartActionHandler::onActionGroupChartEditorTriggered(QAction* a)
{
    DAFigureWidget* fig = mDeps.controller->getCurrentFigure();
    if (!fig || !a) {
        return;
    }
    if (!a->isChecked()) {
        // 都不选中
        fig->endChartEditor();
        return;
    }
    bool isok = false;
    int type  = a->data().toInt(&isok);
    if (!isok) {
        return;
    }
    fig->beginChartEditor(static_cast< DAFigureWidget::ChartEditorType >(type));
    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief 统计绘图引导对话框确认后的处理槽
 *
 * 类型化请求由信号携带（含参数、数据源与目标 figure/chart），widget 侧
 * isValid 已完成校验；这里执行编排并做事后 UI（提升绘图区 / 更新 ribbon /
 * 重设坐标轴）。
 */
void DAAppChartActionHandler::onStatsGuideAccepted(const DA::DAStatsPlotRequest& req)
{
    if (nullptr == req.chart || nullptr == req.figure) {
        return;
    }

    DAWaitCursorScoped wait;
    Q_UNUSED(wait);

    DAStatsPlotCoordinator coordinator;
    coordinator.execute(req);

    mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    mDeps.ribbon->updateFigureAboutRibbon(req.figure);
    if (DAAppChartOperateWidget* chartopt = mDeps.controller->getChartOperateWidget()) {
        chartopt->setCurrentFigure(req.figure);
    }
    // 让坐标轴显示正常
    req.chart->rescaleAxes();
}
}  // namespace DA
