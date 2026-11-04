#include "DAAppWorkflowActionHandler.h"
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

// 工作流域动作分发器：新建工作流、图元编辑、场景视图开关、运行与终止
#define DAAPP_WORKFLOW_BIND(actionname, functionname) \
    connect(actionname, &QAction::triggered, this, &DAAppWorkflowActionHandler::functionname)

namespace DA
{

DAAppWorkflowActionHandler::DAAppWorkflowActionHandler(const DAAppActionHandlerDeps& deps, QObject* par)
    : QObject(par), mDeps(deps)
{
}

void DAAppWorkflowActionHandler::initConnections()
{
    // workflow view 工作流视图
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowViewReadOnly, onActionWorkflowViewReadOnlyTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowViewMarker, onActionWorkflowViewMarkerTriggered);
    // workflow edit 工作流编辑
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowNew, onActionNewWorkflowTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowLinkEnable, onActionWorkflowLinkEnableTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionItemGrouping, onActionItemGroupingTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionItemUngroup, onActionItemUngroupTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowRun, onActionRunCurrentWorkflowTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowTerminate, onActionTerminateCurrentWorkflowTriggered);
    // workflow edit 工作流编辑/data edit 绘图编辑
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowStartDrawRect, onActionStartDrawRectTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowStartDrawText, onActionStartDrawTextTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowAddBackgroundPixmap, onActionAddBackgroundPixmapTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowLockBackgroundPixmap, onActionLockBackgroundPixmapTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionExportWorkflowSceneToPNG, onActionExportWorkflowScenePNGTriggered);
    // 导出图片大按钮（默认 action）绑定同一个 PNG 导出槽，避免点击无反应
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionExportWorkflowSceneToImage, onActionExportWorkflowScenePNGTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowEnableItemLinkageMove,
                        onActionWorkflowEnableItemLinkageMoveTriggered);
    DAAPP_WORKFLOW_BIND(mDeps.actions->actionWorkflowEnableItemMoveWithBackground,
                        onActionEnableItemMoveWithBackgroundTriggered);
    //===================================================
    // Ribbon 工作流编辑：选中图元的字体/画笔变化
    //===================================================
    connect(mDeps.ribbon, &DAAppRibbonArea::selectedWorkflowItemFont, this,
            &DAAppWorkflowActionHandler::onCurrentWorkflowFontChanged);
    connect(mDeps.ribbon, &DAAppRibbonArea::selectedWorkflowItemFontColor, this,
            &DAAppWorkflowActionHandler::onCurrentWorkflowFontColorChanged);
    connect(mDeps.ribbon, &DAAppRibbonArea::selectedWorkflowItemBrush, this,
            &DAAppWorkflowActionHandler::onCurrentWorkflowShapeBackgroundBrushChanged);
    connect(mDeps.ribbon, &DAAppRibbonArea::selectedWorkflowItemPen, this,
            &DAAppWorkflowActionHandler::onCurrentWorkflowShapeBorderPenChanged);

    //===================================================
    // 工作流操作窗口
    //===================================================
    DAPyWorkFlowOperateWidget* workflowOpt = mDeps.dock->getWorkFlowOperateWidget();
    // 鼠标动作完成的触发
    connect(workflowOpt,
            &DAPyWorkFlowOperateWidget::sceneActionDeactived,
            this,
            &DAAppWorkflowActionHandler::onWorkFlowGraphicsSceneActionDeactive);
    connect(workflowOpt,
            &DAPyWorkFlowOperateWidget::selectionItemChanged,
            this,
            &DAAppWorkflowActionHandler::onWorkflowSceneSelectionItemChanged);
    connect(workflowOpt,
            &DAPyWorkFlowOperateWidget::currentWorkFlowWidgetChanged,
            this,
            &DAAppWorkflowActionHandler::onCurrentWorkflowWidgetChanged);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::workflowStartExecute, this, &DAAppWorkflowActionHandler::onWorkflowStartExecute);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::workflowFinished, this, &DAAppWorkflowActionHandler::onWorkflowFinished);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::itemsAdded, this, &DAAppWorkflowActionHandler::onWorkflowSceneitemsAdded);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::itemsRemoved, this, &DAAppWorkflowActionHandler::onWorkflowSceneitemsRemoved);
    connect(mDeps.actions->actionWorkflowShowGrid,
            &QAction::triggered,
            workflowOpt,
            &DAPyWorkFlowOperateWidget::setCurrentWorkflowShowGrid);
    connect(workflowOpt, &DAPyWorkFlowOperateWidget::workflowCreated, this, &DAAppWorkflowActionHandler::onWorkflowCreated);

    //===================================================
    // Python 工作流信号槽（占位）
    //===================================================
    // TODO: 在 DAPyWorkFlowScene 编辑器集成后完善信号槽连接
    // 当前 Action 的信号槽已在上方通过 DAAPP_WORKFLOW_BIND 宏绑定
    qDebug() << "Python workflow connections initialized (placeholder)";
}
/**
 * @brief GraphicsScene的鼠标动作执行完成，把action的选中标记清除
 * @param mf
 */
void DAAppWorkflowActionHandler::onWorkFlowGraphicsSceneActionDeactive(DA::DAAbstractGraphicsSceneAction* scAction)
{
    if (DAGraphicsDrawRectSceneAction* d = dynamic_cast< DAGraphicsDrawRectSceneAction* >(scAction)) {
        mDeps.actions->actionWorkflowStartDrawRect->setChecked(false);
    } else if (DAGraphicsDrawTextItemSceneAction* d = dynamic_cast< DAGraphicsDrawTextItemSceneAction* >(scAction)) {
        mDeps.actions->actionWorkflowStartDrawText->setChecked(false);
    }
}

/**
 * @brief DAPyWorkFlowOperateWidget有新的工作流窗口创建会触发此槽
 * @param wfw
 */
void DAAppWorkflowActionHandler::onWorkflowCreated(DAPyWorkFlowEditWidget* wfw)
{
    if (mDeps.command) {
        mDeps.command->addStack(wfw->getUndoStack());
    }
}

/**
 * @brief 添加背景图
 */
void DAAppWorkflowActionHandler::onActionAddBackgroundPixmapTriggered()
{
    QStringList filters;
    filters << QCoreApplication::translate("DA::DAAppController", "Image files") + QString(" (*.png *.jpg)")  // cn:图片文件
            << QCoreApplication::translate("DA::DAAppController", "Any files") + QString(" (*)")              // cn:任意文件
        ;

    QFileDialog dialog(mDeps.controller->app());
    dialog.setNameFilters(filters);

    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList f = dialog.selectedFiles();
    if (!f.isEmpty()) {
        DAPyWorkFlowOperateWidget* ow = mDeps.dock->getWorkFlowOperateWidget();
        ow->addBackgroundPixmap(f.first());
        mDeps.dock->raiseDockingArea(DAAppDockingArea::DockingAreaWorkFlowOperate);
    }
}

void DAAppWorkflowActionHandler::onActionLockBackgroundPixmapTriggered(bool on)
{
    mDeps.dock->getWorkFlowOperateWidget()->setBackgroundPixmapLock(on);
}

void DAAppWorkflowActionHandler::onActionEnableItemMoveWithBackgroundTriggered(bool on)
{
    if (DAPyWorkFlowGraphicsScene* s = mDeps.dock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->enableItemMoveWithBackground(on);
    }
}

/**
   @brief 允许移动图元时，其它和此图元链接起来的图元跟随移动
   @param a
 */
void DAAppWorkflowActionHandler::onActionWorkflowEnableItemLinkageMoveTriggered(bool on)
{
    if (DAPyWorkFlowGraphicsScene* s = mDeps.dock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->setEnableItemLinkageMove(on);
    }
}

/**
   @brief 分组
 */
void DAAppWorkflowActionHandler::onActionItemGroupingTriggered()
{
    if (DAPyWorkFlowGraphicsScene* s = mDeps.dock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->groupingSelectItems_();
    }
}

/**
   @brief 取消分组
 */
void DAAppWorkflowActionHandler::onActionItemUngroupTriggered()
{
    if (DAPyWorkFlowGraphicsScene* s = mDeps.dock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->removeSelectItemGroup_();
    }
}

/**
 * @brief 导出png
 */
void DAAppWorkflowActionHandler::onActionExportWorkflowScenePNGTriggered()
{
    if (DAPyWorkFlowGraphicsScene* s = mDeps.dock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        DAExportToPngSettingDialog dlg(mDeps.mainWindow);
        if (QDialog::Accepted != dlg.exec()) {
            return;
        }
        auto image = s->toImage(dlg.getDPI());
        QString p  = dlg.getSelectSaveFilePath();
        if (image.save(p)) {
            daInfo << QCoreApplication::translate("DA::DAAppController", "Image saved successfully to %1").arg(p);  // cn:图片保存成功：%1
        } else {
            daCritical << QCoreApplication::translate("DA::DAAppController", "Failed to save image to %1").arg(p);  // cn:图片保存失败：%1
        }
    }
}

/**
 * @brief 工作流视图锁定
 * @param on
 */
void DAAppWorkflowActionHandler::onActionWorkflowViewReadOnlyTriggered(bool on)
{
    if (DAPyWorkFlowOperateWidget* s = mDeps.dock->getWorkFlowOperateWidget()) {
        s->setCurrentWorkflowReadOnly(on);
    }
}

void DAAppWorkflowActionHandler::onActionRunCurrentWorkflowTriggered()
{
    qDebug() << "onActionRunCurrentWorkflowTriggered";
    // 先检查是否有工程
    DAAppProject* p = DA_APP_CORE.getAppProject();
    if (nullptr == p) {
        daCritical << QCoreApplication::translate("DA::DAAppController", "Received null project interface");  // cn:获取到空工程接口
        return;
    }
    QString bn = p->getProjectBaseName();
    if (bn.isEmpty()) {
        QMessageBox::warning(mDeps.controller->app(),
                             QCoreApplication::translate("DA::DAAppController", "Warning"),                                                   // cn:警告
                             QCoreApplication::translate("DA::DAAppController", "Before running the workflow, you need to save the project")  // cn:在运行工作流之前，需要先保存工程
        );
        return;
    }
    mDeps.dock->getWorkFlowOperateWidget()->runCurrentWorkFlow();
}

/**
 * @brief 终止当前的工作流
 */
void DAAppWorkflowActionHandler::onActionTerminateCurrentWorkflowTriggered()
{
    qDebug() << "onActionTerminateCurrentWorkflowTriggered";
    mDeps.dock->getWorkFlowOperateWidget()->terminateCurrentWorkFlow();
}

void DAAppWorkflowActionHandler::onCurrentWorkflowFontChanged(const QFont& f)
{
    DAPyWorkFlowOperateWidget* wf = mDeps.dock->getWorkFlowOperateWidget();
    wf->setDefaultTextFont(f);
    wf->setSelectTextFont(f);
}

void DAAppWorkflowActionHandler::onCurrentWorkflowFontColorChanged(const QColor& c)
{
    DAPyWorkFlowOperateWidget* wf = mDeps.dock->getWorkFlowOperateWidget();
    wf->setDefaultTextColor(c);
    wf->setSelectTextColor(c);
    mDeps.controller->setDirty();
}

void DAAppWorkflowActionHandler::onCurrentWorkflowShapeBackgroundBrushChanged(const QBrush& b)
{
    DAPyWorkFlowOperateWidget* wf = mDeps.dock->getWorkFlowOperateWidget();
    wf->setSelectShapeBackgroundBrush(b);
    mDeps.controller->setDirty();
}

void DAAppWorkflowActionHandler::onCurrentWorkflowShapeBorderPenChanged(const QPen& p)
{
    DAPyWorkFlowOperateWidget* wf = mDeps.dock->getWorkFlowOperateWidget();
    wf->setSelectShapeBorderPen(p);
    mDeps.controller->setDirty();
}

void DAAppWorkflowActionHandler::onWorkflowSceneSelectionItemChanged(QGraphicsItem* lastSelectItem)
{
    if (lastSelectItem == nullptr) {
        return;
    }
    if (DAGraphicsItem* daitem = dynamic_cast< DAGraphicsItem* >(lastSelectItem)) {
        // 属于DAGraphicsItem系列
        mDeps.ribbon->setWorkFlowEditBrush(daitem->getBackgroundBrush());
        mDeps.ribbon->setWorkFlowEditPen(daitem->getBorderPen());
    } else if (DAGraphicsStandardTextItem* titem = dynamic_cast< DAGraphicsStandardTextItem* >(lastSelectItem)) {

        mDeps.ribbon->setWorkFlowEditFont(titem->font());
        mDeps.ribbon->setWorkFlowEditFontColor(titem->defaultTextColor());
    }
}

/**
 * @brief 工作流开始执行的关联槽
 * @param wfw
 */
void DAAppWorkflowActionHandler::onWorkflowStartExecute(DAPyWorkFlowEditWidget* wfw)
{
    Q_UNUSED(wfw);
    mDeps.actions->actionWorkflowRun->setEnabled(false);
    mDeps.actions->actionWorkflowTerminate->setEnabled(true);
}

/**
 * @brief 工作流执行结束的关联槽
 * @param wfw
 * @param success
 */
void DAAppWorkflowActionHandler::onWorkflowFinished(DAPyWorkFlowEditWidget* wfw, bool success)
{
    mDeps.actions->actionWorkflowRun->setEnabled(true);
    mDeps.actions->actionWorkflowTerminate->setEnabled(false);
}

/**
 * @brief 场景有item添加
 * @param sc
 * @param its
 */
void DAAppWorkflowActionHandler::onWorkflowSceneitemsAdded(DAGraphicsScene* sc, const QList< QGraphicsItem* >& its)
{
    mDeps.controller->setDirty(true);
}

/**
 * @brief 场景有item删除
 * @param sc
 * @param its
 */
void DAAppWorkflowActionHandler::onWorkflowSceneitemsRemoved(DAGraphicsScene* sc, const QList< QGraphicsItem* >& its)
{
    mDeps.controller->setDirty(true);
}

/**
 * @brief 当前的wf切换
 * @param wfw
 */
void DAAppWorkflowActionHandler::onCurrentWorkflowWidgetChanged(DAPyWorkFlowEditWidget* wfw)
{
    DAPyWorkFlowOperateWidget* workflowOpt = mDeps.dock->getWorkFlowOperateWidget();
    mDeps.ribbon->updateWorkflowAboutRibbon(workflowOpt);
}

/**
 * @brief 显示标记线
 *
 * 此action有个menu，menu的action选中会设置当前action的图标，具体实现放在@ref DAAppRibbonArea::buildContextCategoryWorkflowView 函数中
 * @param on
 * @sa DAAppRibbonArea::buildContextCategoryWorkflowView
 */
void DAAppWorkflowActionHandler::onActionWorkflowViewMarkerTriggered(bool on)
{
    auto wo = mDeps.dock->getWorkFlowOperateWidget();
    if (!wo) {
        return;
    }
    if (on) {
        // 激活marker
        auto actionCross = wo->getInnerAction(DAPyWorkFlowOperateWidget::ActionCrossLineMarker);
        if (!actionCross) {
            return;
        }
        actionCross->trigger();
    } else {
        // 激活marker
        auto actionNone = wo->getInnerAction(DAPyWorkFlowOperateWidget::ActionNoneMarker);
        if (!actionNone) {
            return;
        }
        actionNone->trigger();
    }
}

/**
 * @brief 新建工作流
 */
void DAAppWorkflowActionHandler::onActionNewWorkflowTriggered()
{
    bool ok      = false;
    QString text = QInputDialog::getText(mDeps.controller->app(),
                                         QCoreApplication::translate("DA::DAAppController", "New workflow name"),   // cn:新工作流名称
                                         QCoreApplication::translate("DA::DAAppController", "New workflow name:"),  // cn:新工作流名称
                                         QLineEdit::Normal,
                                         QString(),
                                         &ok);
    if (!ok || text.isEmpty()) {
        return;
    }
    DAPyWorkFlowOperateWidget* wf = mDeps.dock->getWorkFlowOperateWidget();
    wf->appendWorkflow(text);
    mDeps.controller->setDirty();
}

/**
 * @brief 绘制矩形
 *
 * * @note 绘制完成后会触发onWorkFlowGraphicsSceneMouseActionFinished，在此函数中把这个状态消除
 * @param on
 */
void DAAppWorkflowActionHandler::onActionStartDrawRectTriggered(bool on)
{
    if (on) {
        mDeps.dock->getWorkFlowOperateWidget()->setPreDefineSceneAction(DAPyWorkFlowGraphicsScene::AddRectItemAction);
    }
}

/**
 * @brief 绘制文本
 *
 * @note 绘制完成后会触发onWorkFlowGraphicsSceneMouseActionFinished，在此函数中把这个状态消除
 * @param on
 */
void DAAppWorkflowActionHandler::onActionStartDrawTextTriggered(bool on)
{
    if (on) {
        mDeps.dock->getWorkFlowOperateWidget()->setPreDefineSceneAction(DAPyWorkFlowGraphicsScene::AddTextItemAction);
    }
}

/**
 * @brief 允许连线
 * @param on
 */
void DAAppWorkflowActionHandler::onActionWorkflowLinkEnableTriggered(bool on)
{
    if (auto wo = mDeps.dock->getWorkFlowOperateWidget()) {
        wo->setEnableWorkflowLink(on);
    }
}
}  // namespace DA
