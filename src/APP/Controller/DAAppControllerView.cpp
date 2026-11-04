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
 * @brief 显示 Markdown 查看器（视图页开关）
 *
 * dock 已创建（打开过文件）时直接 raise 找回窗口；
 * 从未打开过时复用「打开 Markdown」的文件选择流程创建 dock
 */
void DAAppController::onActionShowMarkdownViewTriggered()
{
    if (ads::CDockWidget* mdock = mDock->getMarkdownDock()) {
        mdock->raise();
        return;
    }
    onActionOpenMarkdownTriggered();
}

/**
 * @brief 恢复默认布局（触发前弹确认框）
 */
void DAAppController::onActionResetDefaultLayoutTriggered()
{
    if (!mMainWindow) {
        return;
    }
    if (QMessageBox::question(app(),
                              tr("Reset Layout"),  // cn:恢复默认布局
                              tr("This will restore the default window layout. Continue?"),  // cn:将恢复默认窗口布局，是否继续？
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No)
        != QMessageBox::Yes) {
        return;
    }
    mMainWindow->restoreDefaultLayout();
}

/**
 * @brief 打开布局管理对话框（保存/应用/删除布局方案）
 */
void DAAppController::onActionManageLayoutsTriggered()
{
    DAAppLayoutManager* mgr = mMainWindow ? mMainWindow->getLayoutManager() : nullptr;
    if (!mgr) {
        return;
    }
    DA::DALayoutManagerDialog dlg(mgr, app());
    dlg.exec();
}

/**
 * @brief 显示撤销历史面板（005 T1.21，Ribbon「视图」页）
 *
 * 入口选 (a)「点击即 raise」：与现有全部面板入口交互一致（APP 无勾选式入口先例，
 * 面板可由 dock 关闭按钮收起，勾选态收益低）。
 */
void DAAppController::onActionShowUndoHistoryTriggered()
{
    if (mDock && mDock->getUndoHistoryWidget()) {
        mDock->raiseDockByWidget((QWidget*)(mDock->getUndoHistoryWidget()));
    }
}

/**
 * @brief 历史面板调整撤销深度（005 T1.21 R19）
 *
 * 写回配置（dawork-config.json 的 ui.undo-stack-limit）并施加到全部已打开
 * 数据页撤销栈；新建页面在 onDataOperatePageCreated 读同一配置。调大风险只来自
 * Swap 类命令的旧对象驻留内存（快照命令 pickle 到临时文件，吃磁盘不吃内存）。
 * @param limit 新深度（>0；面板 SpinBox 下限 1）
 */
void DAAppController::onUndoHistoryLimitChangeRequested(int limit)
{
    if (limit <= 0) {
        return;
    }
    if (mConfig) {
        DAUISettings s   = mConfig->ui();
        s.undoStackLimit = limit;
        mConfig->setUI(s);
        mConfig->save();
    }
    if (DADataOperateWidget* dow = getDataOperateWidget()) {
        const auto pages = dow->getAllDataFrameWidgets();
        for (DADataOperateOfDataFrameWidget* page : pages) {
            page->setUndoLimit(limit);
        }
    }
}

/**
 * @brief 显示工作流区域
 */
void DAAppController::onActionShowWorkFlowAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getWorkFlowOperateWidget()));
}

void DAAppController::onActionShowWorkFlowManagerAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getWorkflowNodeListWidget()));
}

/**
 * @brief 显示绘图区域
 */
void DAAppController::onActionShowChartAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getChartOperateWidget()));
}

void DAAppController::onActionShowChartManagerAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getChartManageWidget()));
}

/**
 * @brief 显示数据区域
 */
void DAAppController::onActionShowDataAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getDataOperateWidget()));
}

/**
 * @brief 显示数据管理区域
 */
void DAAppController::onActionShowDataManagerAreaTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getDataManageWidget()));
}

/**
 * @brief 显示信息区域
 */
void DAAppController::onActionShowMessageLogViewTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getMessageLogViewWidget()));
}

/**
 * @brief 显示设置区域
 */
void DAAppController::onActionSettingWidgetTriggered()
{
    mDock->raiseDockByWidget((QWidget*)(mDock->getSettingContainerWidget()));
}

void DAAppController::onActionShowRightSideBarTriggered(bool on)
{
    mDock->toggleRightSidebar(on);
}

void DAAppController::onActionShowLeftSideBarTriggered(bool on)
{
    mDock->toggleLeftSidebar(on);
}

}  // end DA
