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
 * @brief GraphicsScene的鼠标动作执行完成，把action的选中标记清除
 * @param mf
 */
void DAAppController::onWorkFlowGraphicsSceneActionDeactive(DA::DAAbstractGraphicsSceneAction* scAction)
{
    if (DAGraphicsDrawRectSceneAction* d = dynamic_cast< DAGraphicsDrawRectSceneAction* >(scAction)) {
        mActions->actionWorkflowStartDrawRect->setChecked(false);
    } else if (DAGraphicsDrawTextItemSceneAction* d = dynamic_cast< DAGraphicsDrawTextItemSceneAction* >(scAction)) {
        mActions->actionWorkflowStartDrawText->setChecked(false);
    }
}

/**
 * @brief DAPyWorkFlowOperateWidget有新的工作流窗口创建会触发此槽
 * @param wfw
 */
void DAAppController::onWorkflowCreated(DAPyWorkFlowEditWidget* wfw)
{
    if (mCommand) {
        mCommand->addStack(wfw->getUndoStack());
    }
}


/**
 * @brief 绘图元素选中
 *
 * 信号由DAChartManageWidget发出
 * @param selection
 */
void DAAppController::onFigureElementClicked(const DAFigureElementSelection& selection)
{
    // 这里和设置页面同步
    DASettingContainerWidget* setting = getSettingContainerWidget();
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
        mDock->raiseDockByWidget((QWidget*)(setting));
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
    mDock->raiseDockByWidget((QWidget*)(setting));
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
void DAAppController::onFigureElementDbClicked(const DAFigureElementSelection& selection)
{
    // === 3D 双击：与单击一致，同步设置面板并提升 dock ===
    if (selection.isSelectedPlot3D() || selection.isSelectedPlot3DItem()) {
        onFigureElementClicked(selection);
        DASettingContainerWidget* setting = getSettingContainerWidget();
        if (setting) {
            setting->showChart3DSettingWidget();
        }
        mDock->raiseDockingArea(DADockingAreaInterface::DockingAreaChartOperate);
        return;
    }

    // === 2D 逻辑 ===
    // 和单击事件一致
    onFigureElementClicked(selection);
    // 同步查看是否设置对话框在前台
    DASettingContainerWidget* setting = getSettingContainerWidget();
    if (setting) {
        setting->showChartSettingWidget();
    }
    // 把绘图界面显示在前台
    mDock->raiseDockingArea(DADockingAreaInterface::DockingAreaChartOperate);
    // 如果是visible双击，那么改变item的visible属性

    if (DAFigureElementSelection::ColumnVisible == selection.selectionColumn) {
        DAChartManageWidget* cmw = mDock->getChartManageWidget();
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
 * @brief 选择的样式改变信号
 * @param column
 * @param dt
 */
void DAAppController::onDataOperateDataFrameWidgetSelectTypeChanged(const QList< int >& column, DAPyDType dt)
{
    Q_UNUSED(column);
    mRibbon->setDataframeOperateCurrentDType(dt);
    // 按 dtype 重建显示格式下拉框的可用类别
    if (mRibbon && mRibbon->mComboxDisplayFormat) {
        mRibbon->mComboxDisplayFormat->updateDType(dt);
    }
}

/**
 * @brief dataframe的列数据类型改变
 * @param index
 */
void DAAppController::onComboxColumnTypesCurrentDTypeChanged(const DA::DAPyDType& dt)
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->changeSelectColumnType(dt);
    }
}
/**
 * @brief 添加背景图
 */
void DAAppController::onActionAddBackgroundPixmapTriggered()
{
    QStringList filters;
    filters << tr("Image files") + QString(" (*.png *.jpg)")  // cn:图片文件
            << tr("Any files") + QString(" (*)")              // cn:任意文件
        ;

    QFileDialog dialog(app());
    dialog.setNameFilters(filters);

    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    QStringList f = dialog.selectedFiles();
    if (!f.isEmpty()) {
        DAPyWorkFlowOperateWidget* ow = mDock->getWorkFlowOperateWidget();
        ow->addBackgroundPixmap(f.first());
        mDock->raiseDockingArea(DAAppDockingArea::DockingAreaWorkFlowOperate);
    }
}

void DAAppController::onActionLockBackgroundPixmapTriggered(bool on)
{
    mDock->getWorkFlowOperateWidget()->setBackgroundPixmapLock(on);
}

void DAAppController::onActionEnableItemMoveWithBackgroundTriggered(bool on)
{
    if (DAPyWorkFlowGraphicsScene* s = mDock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->enableItemMoveWithBackground(on);
    }
}

/**
   @brief 允许移动图元时，其它和此图元链接起来的图元跟随移动
   @param a
 */
void DAAppController::onActionWorkflowEnableItemLinkageMoveTriggered(bool on)
{
    if (DAPyWorkFlowGraphicsScene* s = mDock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->setEnableItemLinkageMove(on);
    }
}

/**
   @brief 分组
 */
void DAAppController::onActionItemGroupingTriggered()
{
    if (DAPyWorkFlowGraphicsScene* s = mDock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->groupingSelectItems_();
    }
}

/**
   @brief 取消分组
 */
void DAAppController::onActionItemUngroupTriggered()
{
    if (DAPyWorkFlowGraphicsScene* s = mDock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        s->removeSelectItemGroup_();
    }
}

/**
 * @brief 导出png
 */
void DAAppController::onActionExportWorkflowScenePNGTriggered()
{
    if (DAPyWorkFlowGraphicsScene* s = mDock->getWorkFlowOperateWidget()->getCurrentWorkFlowScene()) {
        DAExportToPngSettingDialog dlg(mMainWindow);
        if (QDialog::Accepted != dlg.exec()) {
            return;
        }
        auto image = s->toImage(dlg.getDPI());
        QString p  = dlg.getSelectSaveFilePath();
        if (image.save(p)) {
            daInfo << tr("Image saved successfully to %1").arg(p);  // cn:图片保存成功：%1
        } else {
            daCritical << tr("Failed to save image to %1").arg(p);  // cn:图片保存失败：%1
        }
    }
}

/**
 * @brief 工作流视图锁定
 * @param on
 */
void DAAppController::onActionWorkflowViewReadOnlyTriggered(bool on)
{
    if (DAPyWorkFlowOperateWidget* s = mDock->getWorkFlowOperateWidget()) {
        s->setCurrentWorkflowReadOnly(on);
    }
}

void DAAppController::onActionRunCurrentWorkflowTriggered()
{
    qDebug() << "onActionRunCurrentWorkflowTriggered";
    // 先检查是否有工程
    DAAppProject* p = DA_APP_CORE.getAppProject();
    if (nullptr == p) {
        daCritical << tr("Received null project interface");  // cn:获取到空工程接口
        return;
    }
    QString bn = p->getProjectBaseName();
    if (bn.isEmpty()) {
        QMessageBox::warning(app(),
                             tr("Warning"),                                                   // cn:警告
                             tr("Before running the workflow, you need to save the project")  // cn:在运行工作流之前，需要先保存工程
        );
        return;
    }
    mDock->getWorkFlowOperateWidget()->runCurrentWorkFlow();
}

/**
 * @brief 终止当前的工作流
 */
void DAAppController::onActionTerminateCurrentWorkflowTriggered()
{
    qDebug() << "onActionTerminateCurrentWorkflowTriggered";
    mDock->getWorkFlowOperateWidget()->terminateCurrentWorkFlow();
}


void DAAppController::onCurrentWorkflowFontChanged(const QFont& f)
{
    DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
    wf->setDefaultTextFont(f);
    wf->setSelectTextFont(f);
}

void DAAppController::onCurrentWorkflowFontColorChanged(const QColor& c)
{
    DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
    wf->setDefaultTextColor(c);
    wf->setSelectTextColor(c);
    setDirty();
}

void DAAppController::onCurrentWorkflowShapeBackgroundBrushChanged(const QBrush& b)
{
    DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
    wf->setSelectShapeBackgroundBrush(b);
    setDirty();
}

void DAAppController::onCurrentWorkflowShapeBorderPenChanged(const QPen& p)
{
    DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
    wf->setSelectShapeBorderPen(p);
    setDirty();
}

void DAAppController::onWorkflowSceneSelectionItemChanged(QGraphicsItem* lastSelectItem)
{
    if (lastSelectItem == nullptr) {
        return;
    }
    if (DAGraphicsItem* daitem = dynamic_cast< DAGraphicsItem* >(lastSelectItem)) {
        // 属于DAGraphicsItem系列
        mRibbon->setWorkFlowEditBrush(daitem->getBackgroundBrush());
        mRibbon->setWorkFlowEditPen(daitem->getBorderPen());
    } else if (DAGraphicsStandardTextItem* titem = dynamic_cast< DAGraphicsStandardTextItem* >(lastSelectItem)) {

        mRibbon->setWorkFlowEditFont(titem->font());
        mRibbon->setWorkFlowEditFontColor(titem->defaultTextColor());
    }
}

/**
 * @brief 工作流开始执行的关联槽
 * @param wfw
 */
void DAAppController::onWorkflowStartExecute(DAPyWorkFlowEditWidget* wfw)
{
    Q_UNUSED(wfw);
    mActions->actionWorkflowRun->setEnabled(false);
    mActions->actionWorkflowTerminate->setEnabled(true);
}

/**
 * @brief 工作流执行结束的关联槽
 * @param wfw
 * @param success
 */
void DAAppController::onWorkflowFinished(DAPyWorkFlowEditWidget* wfw, bool success)
{
    mActions->actionWorkflowRun->setEnabled(true);
    mActions->actionWorkflowTerminate->setEnabled(false);
}

/**
 * @brief 场景有item添加
 * @param sc
 * @param its
 */
void DAAppController::onWorkflowSceneitemsAdded(DAGraphicsScene* sc, const QList< QGraphicsItem* >& its)
{
    setDirty(true);
}

/**
 * @brief 场景有item删除
 * @param sc
 * @param its
 */
void DAAppController::onWorkflowSceneitemsRemoved(DAGraphicsScene* sc, const QList< QGraphicsItem* >& its)
{
    setDirty(true);
}

/**
 * @brief 当前的wf切换
 * @param wfw
 */
void DAAppController::onCurrentWorkflowWidgetChanged(DAPyWorkFlowEditWidget* wfw)
{
    DAPyWorkFlowOperateWidget* workflowOpt = mDock->getWorkFlowOperateWidget();
    mRibbon->updateWorkflowAboutRibbon(workflowOpt);
}

/**
 * @brief 新fig创建
 * @param f
 */
void DAAppController::onFigureCreated(DAFigureWidget* f)
{
    if (nullptr == f) {
        return;
    }
    if (mCommand) {
        QUndoStack* stack = f->getUndoStack();
        mCommand->addStack(stack);
        stack->setActive();
    }
    // updateFigureAboutRibbon(f);//在onActionAddFigureTriggered中调用了
    setDirty();
    connect(f, &DAFigureWidget::currentChartChanged, this, &DAAppController::onCurrentChartChanged);
    connect(f, &DAFigureWidget::chartEditorStatusChanged, this, &DAAppController::onChartEditorStatusChanged);
    // 指针工具在图中选中元素 → 联动属性设置面板（与树形控件点击同链路）
    connect(f, &DAFigureWidget::figureElementClicked, this, &DAAppController::onFigureElementClicked);
}

/**
 * @brief 当前的fig变化
 * @param f
 * @param index
 */
void DAAppController::onCurrentFigureChanged(DAFigureWidget* f, int index)
{
    Q_UNUSED(index);
    if (nullptr == f) {
        return;
    }
    qDebug() << "DAAppController::onCurrentFigureChanged";
    mRibbon->updateFigureAboutRibbon(f);
    QUndoStack* stack = f->getUndoStack();
    if (stack) {
        stack->setActive();
    }
}

/**
 * @brief 当前的chart改变
 * @param c
 */
void DAAppController::onCurrentChartChanged(DAChartWidget* c)
{
    if (nullptr == c) {
        return;
    }
    mRibbon->updateChartAboutRibbon(c);
}

/**
 * @brief 图表编辑器状态改变
 * @param status
 */
void DAAppController::onChartEditorStatusChanged(DAFigureWidget::ChartEditorStatus status)
{
    if (DAFigureWidget::ChartEditorStatus::EndEdit == status) {
        // 结束编辑
        // 触发结束编辑有可能是按键，或一些异常状态，为了避免和按钮的状态不同步，这里结束编辑后要把actiongroup管理的action都设置为unchecked
        const QList< QAction* > actions = mActions->actionGroupChartEditor->actions();
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
void DAAppController::onPlotItemCreated(DAFigureWidget* f, DAChartWidget* plot, QwtPlotItem* item)
{
    Q_UNUSED(f);
    Q_UNUSED(plot);
    Q_UNUSED(item);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief 3D绘图项创建完成，提升绘图 dock 显示新3D绘图
 */
void DAAppController::onPlot3DItemCreated(DA::DAFigureWidget* f, DA::DAChart3DWidget* plot, Qwt3DPlotItem* item)
{
    Q_UNUSED(f);
    Q_UNUSED(plot);
    Q_UNUSED(item);
    // 提升绘图操作区域到前台
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief 添加数据
 *
 * 支持一次选择多个文件批量导入；选中 txt 时在导入前弹一次解析设置对话框，
 * 同一批 txt 共用此设置；多文件时显示可取消的进度对话框，结束后汇总失败原因
 */
void DAAppController::onActionAddDataTriggered()
{
    QFileDialog dialog(app());
    dialog.setNameFilters(mFileReadFilters);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    if (QDialog::Accepted != dialog.exec()) {
        return;
    }
    const QStringList fileNames = dialog.selectedFiles();
    if (fileNames.empty()) {
        return;
    }
    const int total = static_cast< int >(fileNames.size());
    // 对txt要弹出对话框进行指引，批量选择时只弹一次，同一批txt共用此解析设置
    QVariantMap txtArgs;
    bool hasTxt = false;
    for (const QString& fileName : fileNames) {
        if (QFileInfo(fileName).suffix().toLower() == "txt") {
            DATxtFileImportDialog dlg(mMainWindow);
            dlg.setTextFilePath(fileName);
            if (QDialog::Accepted != dlg.exec()) {
                return;
            }
            // 获取导入txt的配置
            txtArgs = dlg.getSetting();
            hasTxt = true;
            qDebug() << "da_read:args->" << txtArgs;
            break;
        }
    }
    // 多文件时显示进度对话框（可在文件之间取消），单文件保持原有仅等待光标的行为
    QProgressDialog progress(tr("Importing data files ..."),  // cn:正在导入数据文件...
                             tr("Cancel"),                     // cn:取消
                             0,
                             total,
                             mMainWindow);
    progress.setWindowTitle(tr("Import Data"));  // cn:导入数据
    progress.setWindowModality(Qt::WindowModal);
    if (total > 1) {
        progress.setMinimumDuration(0);
        progress.show();
    }
    int successCnt = 0;
    QStringList failedFiles;
    DA_WAIT_CURSOR_SCOPED();
    for (int i = 0; i < total; ++i) {
        const QString& fileName = fileNames.at(i);
        progress.setLabelText(tr("Importing (%1/%2): %3")  // cn:正在导入（%1/%2）：%3
                                  .arg(i + 1)
                                  .arg(total)
                                  .arg(QFileInfo(fileName).fileName()));
        progress.setValue(i);
        if (progress.wasCanceled()) {
            break;
        }
        QVariantMap args;
        if (hasTxt && QFileInfo(fileName).suffix().toLower() == "txt") {
            args = txtArgs;
        }
        QString err;
        if (importData(fileName, args, &err)) {
            ++successCnt;
        } else {
            // 压平Python异常信息里的换行，保证汇总列表每个文件占一行
            QString item = QFileInfo(fileName).fileName();
            if (!err.isEmpty()) {
                item += QStringLiteral(": ") + err.simplified();
            }
            failedFiles << item;
        }
    }
    const bool canceled = progress.wasCanceled() && successCnt < total;
    progress.setValue(total);  // 到达最大值，让进度对话框自动关闭
    // 汇总导入结果（单文件成功保持原有静默行为，不额外提示）
    if (successCnt == total) {
        if (total > 1) {
            daInfo << tr("Imported %1 file(s) successfully").arg(total);  // cn:成功导入 %1 个文件
        }
        return;
    }
    QString msg;
    if (canceled) {
        msg = tr("Import canceled, %1 of %2 file(s) imported").arg(successCnt).arg(total);  // cn:导入已取消，%2 个文件中已导入 %1 个
        if (!failedFiles.isEmpty()) {
            msg += QStringLiteral("\n");
        }
    }
    if (!failedFiles.isEmpty()) {
        msg += tr("%1 of %2 file(s) failed to import:\n%3")  // cn:%2 个文件中有 %1 个导入失败：\n%3
                  .arg(failedFiles.size())
                  .arg(total)
                  .arg(failedFiles.join('\n'));
    }
    daWarning << msg;
}

/**
 * @brief 移除数据
 */
void DAAppController::onActionRemoveDataTriggered()
{
    DADataManageWidget* dmw = mDock->getDataManageWidget();
    dmw->removeSelectData();
    setDirty();
}

/**
 * @brief 重命名数据集
 *
 * 定位数据管理树当前选中项，触发视图编辑（QTreeView::edit），
 * 提交经 DADataManagerTreeModel::setData 落 DAData::setName（拒绝重名/空名）
 */
void DAAppController::onActionRenameDataTriggered()
{
    DADataManageWidget* dmw = mDock->getDataManageWidget();
    if (!dmw) {
        return;
    }
    DADataManagerTreeWidget* tree = dmw->getTreeWidget();
    if (!tree) {
        return;
    }
    if (tree->isSelectDataframeSeries()) {
        // series 子项不支持重命名（模型 flags 已限制其编辑位）
        daWarning << tr("Cannot rename a series, please select a dataset");  // cn:无法重命名series，请选中数据集
        return;
    }
    QTreeView* tv = tree->getTreeView();
    if (!tv) {
        return;
    }
    QModelIndex cur = tv->currentIndex();
    if (!cur.isValid()) {
        daWarning << tr("Please select a dataset to rename");  // cn:请先选中要重命名的数据集
        return;
    }
    // 属性列（第二列）选中时映射回名称列
    if (cur.column() != 0) {
        cur = cur.siblingAtColumn(0);
    }
    // 先确保数据管理窗口在前端，再进入编辑
    mDock->raiseDockByWidget((QWidget*)(dmw));
    tv->edit(cur);
}

/**
 * @brief 导出数据（大按钮：弹出保存对话框，格式按所选过滤器/后缀判定）
 */
void DAAppController::onActionExportDataTriggered()
{
    exportSelectedData(QString());
}

/**
 * @brief 按指定格式导出（子菜单路径：csv/excel/pickle/parquet）
 */
void DAAppController::onActionExportDataAsTriggered()
{
    QString format;
    QAction* act = qobject_cast< QAction* >(sender());
    if (act == mActions->actionExportDataCsv) {
        format = "csv";
    } else if (act == mActions->actionExportDataExcel) {
        format = "excel";
    } else if (act == mActions->actionExportDataPickle) {
        format = "pickle";
    } else if (act == mActions->actionExportDataParquet) {
        format = "parquet";
    }
    if (!format.isEmpty()) {
        exportSelectedData(format);
    }
}

/**
 * @brief 导出当前选中的数据集
 *
 * 取数据管理器选中数据（getSelectDatas，无选中回退当前操作数据），弹保存对话框，
 * 委托 DAAppDataManager::exportToFile 完成（io.py 的 da_to_*），结果 daInfo/daWarning 反馈
 * @param format 指定格式（csv/excel/pickle/parquet）；为空时按保存对话框后缀判定
 */
void DAAppController::exportSelectedData(const QString& format)
{
    // 优先取数据管理器选中数据，无选中时回退当前操作数据
    QList< DAData > datas = mDatas->getSelectDatas();
    DAData d;
    if (!datas.isEmpty()) {
        d = datas.first();
    } else {
        d = mDatas->getOperateData();
    }
    if (d.isNull()) {
        daWarning << tr("Please select a dataset to export");  // cn:请先选中要导出的数据
        return;
    }
    QString name = d.getName();
    if (name.isEmpty()) {
        name = QStringLiteral("data");
    }
    // 过滤器与默认后缀
    QString csvFilter     = tr("CSV File") + QStringLiteral(" (*.csv)");    // cn:CSV 文件
    QString excelFilter   = tr("Excel File") + QStringLiteral(" (*.xlsx)");  // cn:Excel 文件
    QString pickleFilter  = tr("Pickle File") + QStringLiteral(" (*.pkl)");  // cn:Pickle 文件
    QString parquetFilter = tr("Parquet File") + QStringLiteral(" (*.parquet)");  // cn:Parquet 文件
    QString mdFilter      = tr("Markdown File") + QStringLiteral(" (*.md)");  // cn:Markdown 文件
    QString htmlFilter    = tr("HTML File") + QStringLiteral(" (*.html)");    // cn:HTML 文件
    QString selectedFilter;
    if (format == "csv") {
        selectedFilter = csvFilter;
    } else if (format == "excel") {
        selectedFilter = excelFilter;
    } else if (format == "pickle") {
        selectedFilter = pickleFilter;
    } else if (format == "parquet") {
        selectedFilter = parquetFilter;
    } else if (format == "markdown") {
        selectedFilter = mdFilter;
    } else if (format == "html") {
        selectedFilter = htmlFilter;
    }
    QString defaultSuffix;
    if (!selectedFilter.isEmpty()) {
        int i = selectedFilter.lastIndexOf(QLatin1String("*."));
        if (i >= 0) {
            defaultSuffix = selectedFilter.mid(i + 2, selectedFilter.indexOf(')', i) - i - 2);
        }
    }
    QString path = QFileDialog::getSaveFileName(app(),
                                                tr("Export Data"),  // cn:导出数据
                                                name,
                                                csvFilter + ";;" + excelFilter + ";;" + pickleFilter + ";;" + parquetFilter + ";;"
                                                    + mdFilter + ";;" + htmlFilter,
                                                &selectedFilter);
    if (path.isEmpty()) {
        return;
    }
    // 确定格式：优先显式 format，其次按过滤器，最后按后缀
    QString fmt = format;
    if (fmt.isEmpty()) {
        if (selectedFilter == csvFilter) {
            fmt = "csv";
        } else if (selectedFilter == excelFilter) {
            fmt = "excel";
        } else if (selectedFilter == pickleFilter) {
            fmt = "pickle";
        } else if (selectedFilter == parquetFilter) {
            fmt = "parquet";
        } else if (selectedFilter == mdFilter) {
            fmt = "markdown";
        } else if (selectedFilter == htmlFilter) {
            fmt = "html";
        }
    }
    if (fmt.isEmpty()) {
        QString suffix = QFileInfo(path).suffix().toLower();
        if (suffix == "csv") {
            fmt = "csv";
        } else if (suffix == "xlsx" || suffix == "xls") {
            fmt = "excel";
        } else if (suffix == "pkl" || suffix == "pickle") {
            fmt = "pickle";
        } else if (suffix == "parquet") {
            fmt = "parquet";
        } else if (suffix == "md" || suffix == "markdown") {
            fmt = "markdown";
        } else if (suffix == "html" || suffix == "htm") {
            fmt = "html";
        }
    }
    if (fmt.isEmpty()) {
        daWarning << tr("Cannot determine export format, please select a file suffix");  // cn:无法确定导出格式，请选择带后缀的文件名
        return;
    }
    // 补默认后缀
    if (QFileInfo(path).suffix().isEmpty() && !defaultSuffix.isEmpty()) {
        path += QStringLiteral(".") + defaultSuffix;
    }
    // Markdown/HTML 文档格式走文档导出路径（DATableExporter，含样式增量写入），
    // 其余格式走 Python io 导出；路径已选好，直接透传（不再弹保存对话框）
    if (fmt == "markdown" || fmt == "html") {
        exportSelectedDataAsDocument(fmt == "markdown" ? DATableExporter::Format::Markdown : DATableExporter::Format::Html, path);
        return;
    }
    QString err;
    DA_WAIT_CURSOR_SCOPED();
    if (mDatas->exportToFile(d, path, fmt, &err)) {
        daInfo << tr("Data exported successfully, path: %1").arg(path);  // cn:数据导出成功，路径:%1
    } else {
        daWarning << tr("Data export failed, path: %1, reason: %2").arg(path, err);  // cn:数据导出失败，路径:%1，原因:%2
    }
}

/**
 * @brief 取要操作的数据集并构造 DATableExporter（复制/导出 Markdown 与 HTML 共用）
 *
 * 数据选择口径与 exportSelectedData 一致：优先数据管理树选中，无选中回退当前
 * 操作数据。元数据束按数据查注册表（get：束随数据存在、页面关闭不销毁；无束
 * = 纯数据导出，不带样式）。exporter 有效性由调用方经 isValid() 复核。
 * columns 非空时构造列子集导出器（表头右键「按选中列」入口，列序按传入顺序）；
 * sourcePage 非空时列子集取数固定用该页面（表头右键菜单触发时停靠区焦点
 * 尚未归还，不能走焦点检查；空 = 回退当前页，同样不做焦点检查）。
 * @param exporter 出参（构造好的导出器）
 * @param dataOut 非空时回填所选数据（保存对话框默认文件名用）
 * @param columns 列子集（数据源列号；空 = 整表）
 * @param sourcePage 列子集路径的数据来源页面（空 = 当前页）
 * @return 构造成功返回 true
 */
bool DAAppController::makeExporterForSelection(std::unique_ptr< DATableExporter >* exporter,
                                               DAData* dataOut,
                                               const QList< int >& columns,
                                               DADataOperateOfDataFrameWidget* sourcePage)
{
    if (!exporter) {
        return false;
    }
    // 列子集路径（表头右键入口）：数据来自发送信号的页面（sourcePage 固定，
    // 缺省回退当前页，均不做焦点检查——表头右键菜单是独立窗口，触发时
    // 停靠区焦点尚未归还表格页），不走数据管理树选中的口径——两口径混用会导错数据集
    DAData d;
    if (!columns.isEmpty()) {
        DADataOperateOfDataFrameWidget* w = sourcePage ? sourcePage : getCurrentDataFrameOperateWidget(false, false);
        if (!w) {
            daWarning << tr("No open dataframe table");  // cn:当前没有打开的 dataframe 表格
            return false;
        }
        d = w->data();
    } else {
        QList< DAData > datas = mDatas->getSelectDatas();
        if (!datas.isEmpty()) {
            d = datas.first();
        } else {
            d = mDatas->getOperateData();
        }
    }
    if (d.isNull()) {
        daWarning << tr("Please select a dataset first");  // cn:请先选中数据集
        return false;
    }
    if (dataOut) {
        *dataOut = d;
    }
    // 元数据束（样式/条件格式/批注）：有则带样式导出，无则纯数据
    DATableMetadata* meta = nullptr;
    if (DADataOperateWidget* dow = getDataOperateWidget()) {
        if (DATableMetadataRegistry* reg = dow->metadataRegistry()) {
            meta = reg->get(d);
        }
    }
    exporter->reset(new DATableExporter(d, meta, columns));
    QString why;
    if (!(*exporter)->isValid(&why)) {
        daWarning << why;
        return false;
    }
    return true;
}

/**
 * @brief 把选中的数据集整表复制到剪贴板（数据管理树右键「复制为」入口）
 *
 * Tab 文本与表格内选区复制同口径；HTML 经 text/html mime 富文本粘贴
 * （Word/浏览器/邮件直接保留背景色与字体），纯文本回退为同内容的 Markdown
 * 之外的兜底（这里用 Tab 文本，Word 里「选择性粘贴-无格式文本」得到同款表格）。
 * 超大表（isLargeForCopy 阈值）先弹确认，避免一次性构建大字符串卡界面；
 * 构建期间进度对话框可取消（块边界生效）。
 * @param fmt 目标形态
 * @param columns 列子集（表头右键「复制选中列为」入口传入；空 = 整表）
 * @param sourcePage 列子集取数的来源页面（表头右键入口传入；空 = 整表路径不用）
 */
void DAAppController::copySelectedDataAsTable(DATableExporter::Format fmt,
                                              const QList< int >& columns,
                                              DADataOperateOfDataFrameWidget* sourcePage)
{
    std::unique_ptr< DATableExporter > exporter;
    if (!makeExporterForSelection(&exporter, nullptr, columns, sourcePage)) {
        return;
    }
    // 大小确认：复制需把整表渲染成字符串驻留内存，超大表先提醒
    if (DATableExporter::isLargeForCopy(exporter->rowCount(), exporter->columnCount())) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            mMainWindow,
            tr("Copy Large Table"),  // cn:复制大表
            tr("The table contains %1 rows x %2 columns. Copying it to the clipboard may take a while and use "
               "significant memory. Continue?")  // cn:表格共 %1 行 × %2 列，复制到剪贴板可能耗时较长并占用较多内存，是否继续？
                .arg(exporter->rowCount())
                .arg(exporter->columnCount()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (btn != QMessageBox::Yes) {
            return;
        }
    }
    QProgressDialog progress(tr("Copying table ..."),  // cn:正在复制表格...
                             tr("Cancel"),              // cn:取消
                             0,
                             100,
                             mMainWindow);
    progress.setWindowTitle(tr("Copy Table"));  // cn:复制表格
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    // 块边界推进进度并抽事件循环（进度可见 + 取消可点）
    auto processEvents = [&progress]() {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        return progress.wasCanceled();
    };
    QString text;
    QString err;
    DA_WAIT_CURSOR_SCOPED();
    const bool ok = exporter->toString(fmt, &text, &err,
                                      [ &progress ](qint64 done, qint64 total) {
                                          if (total > 0) {
                                              progress.setValue(static_cast< int >(done * 100 / total));
                                          }
                                      },
                                      processEvents);
    progress.setValue(100);
    if (!ok) {
        if (!err.isEmpty()) {
            daWarning << err;
        }
        return;
    }
    QClipboard* cb = QGuiApplication::clipboard();
    if (!cb) {
        return;
    }
    if (fmt == DATableExporter::Format::Html) {
        // HTML mime + 纯文本回退：Word/Excel/浏览器按 text/html 富文本粘贴
        QMimeData* mime = new QMimeData();
        mime->setHtml(text);
        mime->setText(text);
        cb->setMimeData(mime);
    } else {
        cb->setText(text);
    }
    daInfo << tr("Table copied to clipboard (%1 rows x %2 columns)")  // cn:表格已复制到剪贴板（%1 行 × %2 列）
                  .arg(exporter->rowCount())
                  .arg(exporter->columnCount());
}

/**
 * @brief 导出选中数据集为 Markdown/HTML 文档（右键导出子菜单与主导出按钮共用）
 *
 * 与复制共用 DATableExporter 的取数与渲染核心；文件路径增量写入
 * （QTextStream），任意大表内存占用恒定，不做大小确认。
 * 导出期间进度对话框可取消；取消时删除半成品文件。
 * path 非空时跳过保存对话框（主导出按钮路径已选好路径，按 fmt 判定格式）。
 * @param fmt Markdown 或 HTML
 * @param path 已选定的目标路径（空 = 弹保存对话框）
 * @param columns 列子集（表头右键「导出选中列」入口传入；空 = 整表）
 * @param sourcePage 列子集取数的来源页面（表头右键入口传入；空 = 整表路径不用）
 */
void DAAppController::exportSelectedDataAsDocument(DATableExporter::Format fmt,
                                                   const QString& path,
                                                   const QList< int >& columns,
                                                   DADataOperateOfDataFrameWidget* sourcePage)
{
    std::unique_ptr< DATableExporter > exporter;
    DAData d;
    if (!makeExporterForSelection(&exporter, &d, columns, sourcePage)) {
        return;
    }
    QString targetPath = path;
    if (targetPath.isEmpty()) {
        QString name = d.getName();
        if (name.isEmpty()) {
            name = QStringLiteral("data");
        }
        QString mdFilter   = tr("Markdown File") + QStringLiteral(" (*.md)");  // cn:Markdown 文件
        QString htmlFilter = tr("HTML File") + QStringLiteral(" (*.html)");    // cn:HTML 文件
        QString selectedFilter;
        QString defaultSuffix;
        if (fmt == DATableExporter::Format::Markdown) {
            selectedFilter = mdFilter;
            defaultSuffix  = QStringLiteral("md");
        } else {
            selectedFilter = htmlFilter;
            defaultSuffix  = QStringLiteral("html");
        }
        targetPath = QFileDialog::getSaveFileName(app(),
                                                  tr("Export Data"),  // cn:导出数据
                                                  name,
                                                  mdFilter + ";;" + htmlFilter,
                                                  &selectedFilter);
        if (targetPath.isEmpty()) {
            return;
        }
        if (QFileInfo(targetPath).suffix().isEmpty()) {
            targetPath += QStringLiteral(".") + defaultSuffix;
        }
    }
    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        daWarning << tr("Cannot open file for writing: %1").arg(targetPath);  // cn:无法打开文件进行写入:%1
        return;
    }
    QProgressDialog progress(tr("Exporting table ..."),  // cn:正在导出表格...
                             tr("Cancel"),                // cn:取消
                             0,
                             100,
                             mMainWindow);
    progress.setWindowTitle(tr("Export Data"));  // cn:导出数据
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    auto processEvents = [&progress]() {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        return progress.wasCanceled();
    };
    QString err;
    DA_WAIT_CURSOR_SCOPED();
    const bool ok = exporter->write(fmt, &file, &err,
                                   [ &progress ](qint64 done, qint64 total) {
                                       if (total > 0) {
                                           progress.setValue(static_cast< int >(done * 100 / total));
                                       }
                                   },
                                   processEvents);
    progress.setValue(100);
    file.close();
    if (!ok) {
        // 取消/失败不留半成品
        QFile::remove(targetPath);
        if (!err.isEmpty()) {
            daWarning << tr("Data export failed, path: %1, reason: %2").arg(targetPath, err);  // cn:数据导出失败，路径:%1，原因:%2
        }
        return;
    }
    daInfo << tr("Data exported successfully, path: %1").arg(targetPath);  // cn:数据导出成功，路径:%1
}

/**
 * @brief 表头右键「复制选中列为」信号处理器
 *
 * page 固定为发送信号表头所在的页面（右键菜单是独立窗口，触发时停靠区
 * 焦点尚未归还表格页，不能经焦点检查取当前页）。从该页面解析选中列
 * （无选中回退右键所在列），格式枚举映射后走与数据管理树「复制为」
 * 共用的 copySelectedDataAsTable（列子集路径）。
 * @param page 发送信号的表头所在页面
 * @param fallbackColumn 右键所在列（无选中列时的回退）
 * @param format 表头声明的目标形态（映射 DATableExporter::Format）
 */
void DAAppController::onHeaderColumnDataCopyRequested(DADataOperateOfDataFrameWidget* page,
                                                      int fallbackColumn,
                                                      DADataHeaderView::ColumnDataFormat format)
{
    DADataOperateOfDataFrameWidget* w = page;
    if (!w) {
        daWarning << tr("No open dataframe table");  // cn:当前没有打开的 dataframe 表格
        return;
    }
    const QList< int > columns = w->selectedColumnsForDataAction(fallbackColumn);
    if (columns.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    DATableExporter::Format fmt = DATableExporter::Format::TabText;
    switch (format) {
    case DADataHeaderView::ColumnDataFormat::Markdown:
        fmt = DATableExporter::Format::Markdown;
        break;
    case DADataHeaderView::ColumnDataFormat::Html:
        fmt = DATableExporter::Format::Html;
        break;
    default:
        break;
    }
    copySelectedDataAsTable(fmt, columns, w);
}

/**
 * @brief 表头右键「导出选中列」信号处理器（列子集版文档导出）
 *
 * 列解析与格式映射同 onHeaderColumnDataCopyRequested（page 固定为发送
 * 信号的页面，同样不做焦点检查）；执行走与数据管理树「导出 Markdown/HTML
 * 文档」共用的 exportSelectedDataAsDocument。
 */
void DAAppController::onHeaderColumnDataExportRequested(DADataOperateOfDataFrameWidget* page,
                                                        int fallbackColumn,
                                                        DADataHeaderView::ColumnDataFormat format)
{
    DADataOperateOfDataFrameWidget* w = page;
    if (!w) {
        daWarning << tr("No open dataframe table");  // cn:当前没有打开的 dataframe 表格
        return;
    }
    const QList< int > columns = w->selectedColumnsForDataAction(fallbackColumn);
    if (columns.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    DATableExporter::Format fmt = DATableExporter::Format::Markdown;
    switch (format) {
    case DADataHeaderView::ColumnDataFormat::Html:
        fmt = DATableExporter::Format::Html;
        break;
    default:
        break;
    }
    exportSelectedDataAsDocument(fmt, QString(), columns, w);
}

/**
 * @brief 添加一个figure
 */
void DAAppController::onActionAddFigureTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    // 添加绘图
    DAFigureWidget* fig = chartopt->createFigure();
    // 这里不需要回退
    DAChartWidget* chart = fig->createChart();
    chart->setAxisLabel(QwtAxis::XBottom, "x");
    chart->setAxisLabel(QwtAxis::YLeft, "y");
    // 把fig的undostack添加
    mCommand->addStack(fig->getUndoStack());
    mRibbon->updateFigureAboutRibbon(fig);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    chartopt->setCurrentFigure(fig);
    setDirty();
}


/**
 * @brief 新建一个坐标系
 */
void DAAppController::onActionFigureNewXYAxisTriggered()
{
    DAFigureWidget* fig = getCurrentFigure();
    if (!fig) {
        daWarning << tr("Before creating a new coordinate, you need to create a figure");  // cn:在创建一个坐标系之前，需要先创建一个绘图窗口
        return;
    }
    DAChartWidget* w = fig->createChart_(QRectF(0.1, 0.1, 0.4, 0.4));
    w->enableGrid();
    w->enablePan();
    //    w->addCurve({ 1, 2, 3, 4, 5 }, { 3, 5, 8, 0, -3 })->setTitle("curve1");
    //    w->addCurve({ 1, 2, 3, 4, 5 }, { 5, 7, 0, -1, 1 })->setTitle("curve2");
    mRibbon->updateChartAboutRibbon(w);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    setDirty();
}

/**
 * @brief 创建一个曲线
 */
void DAAppController::onActionChartAddCurveTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Curve);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加散点图
 */
void DAAppController::onActionChartAddScatterTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Scatter);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加柱状图
 */
void DAAppController::onActionChartAddBarTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Bar);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加误差棒图
 */
void DAAppController::onActionactionChartAddErrorBarTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::ErrorBar);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加误差棒图
 */
void DAAppController::onActionChartAddBoxPlotTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Box);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加谱图
 */
void DAAppController::onActionChartAddCloudMapTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Spectrogram);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加多重柱状图
 */
void DAAppController::onActionChartAddMultiBarTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::MultiBar);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加直方图
 */
void DAAppController::onActionChartAddHistogramTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Histogram);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加等高线图
 */
void DAAppController::onActionChartAddContourMapTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Contour);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加向量场图
 */
void DAAppController::onActionChartAddVectorfieldTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::VectorField);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加3D曲面图
 */
void DAAppController::onActionChartAdd3DSurfaceTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Surface3D);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加3D柱状图
 */
void DAAppController::onActionChartAdd3DBarTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Bar3D);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 添加3D线图
 */
void DAAppController::onActionChartAdd3DLineTriggered()
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
    chartopt->showPlotGuideDialog(DA::DAChartTypes::Line3D);
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);
}

/**
 * @brief 统计直方图
 */
void DAAppController::onActionStatsHistplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsHistplot);
}

/**
 * @brief 一维核密度图
 */
void DAAppController::onActionStatsKdeplot1dTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsKdeplot1d);
}

/**
 * @brief 二维核密度图
 */
void DAAppController::onActionStatsKdeplot2dTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsKdeplot2d);
}

/**
 * @brief 统计箱线图
 */
void DAAppController::onActionStatsBoxplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsBoxplot);
}

/**
 * @brief 热力图
 */
void DAAppController::onActionStatsHeatmapTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsHeatmap);
}

/**
 * @brief 统计散点图
 */
void DAAppController::onActionStatsScatterplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsScatterplot);
}

/**
 * @brief 统计柱状图
 */
void DAAppController::onActionStatsBarplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsBarplot);
}

/**
 * @brief 回归图
 */
void DAAppController::onActionStatsRegplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsRegplot);
}

/**
 * @brief 经验累积分布图
 */
void DAAppController::onActionStatsECDFplotTriggered()
{
    showStatsChartGuide(DA::DAChartTypes::StatsECDFplot);
}

/**
 * @brief 确保当前 Figure 和 Chart 存在（不存在则创建）
 * @param fig 输出 Figure 指针
 * @param chart 输出 Chart 指针
 * @return 如果 fig 和 chart 都有效返回 true
 */
bool DAAppController::ensureFigureChart(DAFigureWidget*& fig, DAChartWidget*& chart)
{
    DAAppChartOperateWidget* chartopt = getChartOperateWidget();
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
void DAAppController::showStatsChartGuide(DA::DAChartTypes type)
{
    DAFigureWidget* fig  = nullptr;
    DAChartWidget* chart = nullptr;
    if (!ensureFigureChart(fig, chart)) {
        return;
    }
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaDataManager);

    if (!mStatsChartGuideDlg) {
        mStatsChartGuideDlg = new DADialogStatsChartGuide(app());
        mStatsChartGuideDlg->setDataManager(mDatas->dataManager());
        connect(mStatsChartGuideDlg, &DADialogStatsChartGuide::plotRequested, this, &DAAppController::onStatsGuideAccepted);
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
void DAAppController::onActionChartEnableGridTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGrid(on);
        w->replot();
        return true;
    });
    if (res) {
        // grid生效与否决定了X，Y以及Xmin,Ymin能否显示
        mRibbon->updateChartGridAboutRibbon(getCurrentChart());
        setDirty();
    }
}

/**
 * @brief 横向网格
 * @param on
 */
void DAAppController::onActionChartEnableGridXTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridX(on);
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}
/**
 * @brief 纵向网格
 * @param on
 */
void DAAppController::onActionChartEnableGridYTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridY(on);
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}
/**
 * @brief 横向密集网格
 * @param on
 */
void DAAppController::onActionChartEnableGridXMinEnableTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridXMin(on);
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}
/**
 * @brief 纵向密集网格
 * @param on
 */
void DAAppController::onActionChartEnableGridYMinTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableGridYMin(on);
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}

/**
 * @brief 当前图表允许缩放
 * @param on
 */
void DAAppController::onActionChartEnableZoomTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableZoom(on);
        return true;
    });
    if (res) {
        mRibbon->updateChartZoomPanAboutRibbon(getCurrentChart());
    }
}

/**
 * @brief 当前图表放大
 */
void DAAppController::onActionChartZoomInTriggered()
{
    applyToCharts([](DAChartWidget* w) -> bool {
        w->zoomIn();
        return true;
    });
}

/**
 * @brief 当前图表缩小
 */
void DAAppController::onActionChartZoomOutTriggered()
{
    applyToCharts([](DAChartWidget* w) -> bool {
        w->zoomOut();
        return true;
    });
}

/**
 * @brief 当前图表全部显示
 */
void DAAppController::onActionChartZoomAllTriggered()
{
    bool res = applyToCharts([](DAChartWidget* w) -> bool {
        w->rescaleAxes();
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}

/**
 * @brief 允许绘图拖动
 * @param on
 */
void DAAppController::onActionChartEnablePanTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enablePan(on);
        return true;
    });
    if (res) {
        mRibbon->updateChartZoomPanAboutRibbon(getCurrentChart());
    }
}

/**
 * @brief 禁止水平缩放
 * @param on 选中时禁止x轴参与缩放
 */
void DAAppController::onActionChartDisableZoomXTriggered(bool on)
{
    applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableXAxisZoom(!on);
        return true;
    });
}

/**
 * @brief 禁止垂直缩放
 * @param on 选中时禁止y轴参与缩放
 */
void DAAppController::onActionChartDisableZoomYTriggered(bool on)
{
    applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableYAxisZoom(!on);
        return true;
    });
}

/**
 * @brief 允许绘图拾取
 * @param on
 */
void DAAppController::onActionChartEnablePickerCrossTriggered(bool on)
{
    applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableCrosshair(on);
        return true;
    });
}

/**
 * @brief 允许绘图拾取Y
 * @param on
 */
void DAAppController::onActionChartEnablePickerYTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableYValuePicking(on);
        return true;
    });
    if (res) {
        setDirty();
    }
}

/**
 * @brief 设置pick的区域
 * @param act
 */
void DAAppController::onActionGroupChartPickerTextRegionTriggered(QAction* act)
{
    applyToCharts([ act ](DAChartWidget* w) -> bool {
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
void DAAppController::onActionChartEnablePickerXYTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableXYValuePicking(on);
        return true;
    });
    if (res) {
        setDirty();
    }
}

/**
 * @brief 连接所有picker
 * @param on
 */
void DAAppController::onActionChartLinkAllPickerEnabledTriggered(bool on)
{
    DAFigureWidget* fig = getCurrentFigure();
    if (fig) {
        fig->setDataPickerGroupEnabled(on);
    }
}

/**
 * @brief
 * @param on
 */
void DAAppController::onActionChartYPickerShowXValueEnabledTriggered(bool on)
{
    applyToCharts([ on ](DAChartWidget* w) -> bool {
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
void DAAppController::onActionChartEnableLegendTriggered(bool on)
{
    bool res = applyToCharts([ on ](DAChartWidget* w) -> bool {
        w->enableLegend(on);
        w->replot();
        return true;
    });
    if (res) {
        setDirty();
    }
}

/**
 * @brief 绘图样式选择
 * @param act
 */
void DAAppController::onActionGroupFigureThemeTriggered(QAction* act)
{
    DAColorTheme::ColorThemeStyle style = static_cast< DAColorTheme::ColorThemeStyle >(act->data().toInt());
    DAColorTheme theme(style);
    DAFigureWidget* fig = getCurrentFigure();
    if (!fig) {
        return;
    }
    fig->setColorTheme(theme);
}

void DAAppController::onActionCopyFigureToClipboardTriggered()
{
    DAFigureWidget* fig = getCurrentFigure();
    if (!fig) {
        return;
    }
    fig->copyToClipboard();
}

/**
 * @brief 图例位置菜单切换（挂在actionChartEnableLegend下的上/下/左/右互斥组）
 */
void DAAppController::onActionGroupChartLegendPositionTriggered(QAction* act)
{
    DAChartWidget* chart = getCurrentChart();
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
    setDirty();
}

void DAAppController::onActionChartDataPickerSettingTriggered()
{
    DAFigureWidget* fig = getCurrentFigure();
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
    DASettingContainerWidget* setting = getSettingContainerWidget();
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
    mDock->raiseDockByWidget((QWidget*)(mDock->getSettingContainerWidget()));
}

void DAAppController::onActionGroupChartEditorTriggered(QAction* a)
{
    DAFigureWidget* fig = getCurrentFigure();
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
    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
}

/**
 * @brief dataframe删除行
 */
void DAAppController::onActionRemoveRowTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectRow();
        setDirty();
    }
}

/**
 * @brief dataframe删除列
 */
void DAAppController::onActionRemoveColumnTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectColumn();
        setDirty();
    }
}

/**
 * @brief 移除单元格
 */
void DAAppController::onActionRemoveCellTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->removeSelectCell();
        setDirty();
    }
}

/**
 * @brief 插入行
 */
void DAAppController::onActionInsertRowTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertRowBelowBySelect();
        setDirty();
    }
}

/**
 * @brief 在选中位置上面插入一行
 */
void DAAppController::onActionInsertRowAboveTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertRowAboveBySelect();
        setDirty();
    }
}
/**
 * @brief 在选中位置右边插入一列
 */
void DAAppController::onActionInsertColumnRightTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertColumnRightBySelect();
        setDirty();
    }
}
/**
 * @brief 在选中位置左边插入一列
 */
void DAAppController::onActionInsertColumnLeftTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->insertColumnLeftBySelect();
        setDirty();
    }
}

/**
 * @brief dataframe列重命名
 */
void DAAppController::onActionRenameColumnsTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->renameColumns();
        setDirty();
    }
}

/**
 * @brief dataframe单列重命名（表头右键）
 *
 * 弹出 QInputDialog 让用户输入新列名，调用 widget->renameColumn。
 * 依赖表头右键菜单已先选中目标列（B-2-a）。
 */
void DAAppController::onActionRenameColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    int col = dfopt->getSelectedOneDataframeColumn();
    if (col < 0) {
        return;
    }
    renameColumnOnPage(dfopt, col);
}

/**
 * @brief 在指定页面重命名列（表头右键直入口）
 *
 * 不经焦点检查、不依赖选区——右键菜单是独立窗口会抢走停靠区焦点，信号里
 * 携带的 (w, col) 即操作目标。弹输入框（预填当前列名）→ w->renameColumn。
 * @param w 目标数据页（信号接线的 lambda 捕获）
 * @param col 目标列（逻辑索引）
 */
void DAAppController::renameColumnOnPage(DADataOperateOfDataFrameWidget* w, int col)
{
    if (!w || col < 0) {
        return;
    }
    DADataTableView* tv = w->getDataTableView();
    if (!tv) {
        return;
    }
    QString oldName = tv->actualColumnName(col);
    bool ok         = false;
    QString newName = QInputDialog::getText(w,
                                            tr("Rename Column"),     // cn:重命名列
                                            tr("New column name:"),  // cn:新列名：
                                            QLineEdit::Normal,
                                            oldName,
                                            &ok);
    if (!ok) {
        return;
    }
    if (w->renameColumn(col, newName.trimmed())) {
        setDirty();
    }
}

/**
 * @brief 复制列名到剪贴板（表头右键）
 *
 * 依赖表头右键菜单已先选中目标列（B-2-a），列名来源走 view 的 actualColumnName。
 */
void DAAppController::onActionCopyColumnNameTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    int col = dfopt->getSelectedOneDataframeColumn();
    if (col < 0) {
        return;
    }
    DADataTableView* tv = dfopt->getDataTableView();
    if (!tv) {
        return;
    }
    QString name = tv->actualColumnName(col);
    if (name.isEmpty()) {
        return;
    }
    QClipboard* cb = QApplication::clipboard();
    if (cb) {
        cb->setText(name);
    }
}

/**
 * @brief 跳转到最大值（表头右键）
 *
 * 取当前选中列的最大值位置索引，经定位单入口滚动并选中。
 * 依赖表头右键菜单已先选中目标列（B-2-a）。
 */
void DAAppController::onActionGotoMaxTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    int col = dfopt->getSelectedOneDataframeColumn();
    if (col < 0) {
        return;
    }
    DAPyDataFrame df = dfopt->getDataframe();
    if (df.isNone()) {
        return;
    }
    QString colNm = df.columnName(col);
    DAPySeries s  = df[ colNm ];
    long pos      = s.idxmaxPosition();
    if (pos < 0) {
        daWarning << tr("Cannot find the maximum value in this column (empty, all-NaN, or incomparable types)");  // cn:此列无法找到最大值（空列、全为NaN或类型不可比较）
        return;
    }
    // 选中并滚动到目标单元格（007：走定位单入口，顺带获得隐藏列/冻结区处置）
    dfopt->locateActualCell(static_cast< int >(pos), col);
    // 打印结果到信息栏
    {
        pybind11::gil_scoped_acquire gil;
        try {
            pybind11::object val = s.iat(pos);
            QString valStr       = pybind11::str(val).cast< QString >();
            daInfo << tr("Column [%1] maximum value: %2, row: %3").arg(colNm, valStr).arg(pos);  // cn:列[%1] 最大值: %2, 第 %3 行
        } catch (const std::exception& e) {
            qCritical() << e.what();
            daInfo << tr("Column [%1] maximum value at row %2").arg(colNm).arg(pos);  // cn:列[%1] 最大值位于第 %2 行
        }
    }
}

/**
 * @brief 跳转到最小值（表头右键）
 *
 * 取当前选中列的最小值位置索引，经定位单入口滚动并选中。
 * 依赖表头右键菜单已先选中目标列（B-2-a）。
 */
void DAAppController::onActionGotoMinTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    int col = dfopt->getSelectedOneDataframeColumn();
    if (col < 0) {
        return;
    }
    DAPyDataFrame df = dfopt->getDataframe();
    if (df.isNone()) {
        return;
    }
    QString colNm = df.columnName(col);
    DAPySeries s  = df[ colNm ];
    long pos      = s.idxminPosition();
    if (pos < 0) {
        daWarning << tr("Cannot find the minimum value in this column (empty, all-NaN, or incomparable types)");  // cn:此列无法找到最小值（空列、全为NaN或类型不可比较）
        return;
    }
    // 选中并滚动到目标单元格（007：走定位单入口，顺带获得隐藏列/冻结区处置）
    dfopt->locateActualCell(static_cast< int >(pos), col);
    // 打印结果到信息栏
    {
        pybind11::gil_scoped_acquire gil;
        try {
            pybind11::object val = s.iat(pos);
            QString valStr       = pybind11::str(val).cast< QString >();
            daInfo << tr("Column [%1] minimum value: %2, row: %3").arg(colNm, valStr).arg(pos);  // cn:列[%1] 最小值: %2, 第 %3 行
        } catch (...) {
            daInfo << tr("Column [%1] minimum value at row %2").arg(colNm).arg(pos);  // cn:列[%1] 最小值位于第 %2 行
        }
    }
}

/**
 * @brief 显示统计信息（Ribbon「列」面板 + 列头右键）
 *
 * 用当前页选区构造快速绘图请求（数据集句柄 + 有序选中列 + 角色），交给
 * DAAppDockingArea::showDataStatsWidget 在浮动 dock 里做深度统计分析
 * （多列统计表格 + 箱线图，支持正态性检验/置信区间/离群点等扩展分析）；
 * 支持多选列：选中几列就统计并对比几列。
 */
void DAAppController::onActionShowColumnDescribeTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    const DA::DATableChartRequest request = dfopt->buildQuickPlotRequest();
    if (!request.isValid() || request.columns.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    mDock->showDataStatsWidget(request);
    // 绑定当前页的数据变化通知（刷新提示条不自动重算；重复触发防重复连接）
    if (DADataStatsWidget* statsWidget = mDock->getDataStatsWidget()) {
        connect(dfopt, &DADataOperateOfDataFrameWidget::dataModified,
                statsWidget, &DADataStatsWidget::notifyDataChanged, Qt::UniqueConnection);
    }
}

/**
 * @brief 统计窗口「转入绘图区」
 *
 * DAGui 的统计窗口不持有绘图区（分层约束：DAGui 不能反向依赖 APP 的窗口编排），
 * 由 APP 在这里取出绘图区容器注入；建图与撤销宏都在统计窗口里完成。
 */
void DAAppController::onStatsTransferToFigureRequested()
{
    if (!mDock) {
        return;
    }
    DADataStatsWidget* statsWidget = mDock->getDataStatsWidget();
    DAChartOperateWidget* chartOperate = mDock->getChartOperateWidget();
    if (!statsWidget || !chartOperate) {
        return;
    }
    if (statsWidget->transferToFigure(chartOperate)) {
        // 转入后把绘图区抬到前面，用户能立刻看到新生成的绘图
        mDock->raiseDockingArea(DADockingAreaInterface::DockingAreaChartOperate);
    }
}

/**
 * @brief 快速绘图（Ribbon「列」面板 + 列头右键）
 *
 * 只负责取当前页并转给页面：选区 → 请求 → 浮窗的规则都在页面与 DAGui 的
 * 快速绘图模块里，APP 不重复实现建图逻辑。
 */
void DAAppController::onActionQuickPlotTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    if (!dfopt->isDataframeTableHaveSelection()) {
        daWarning << tr("Select the columns to plot first");  // cn:请先选择要绘图的列
        return;
    }
    dfopt->showQuickPlot();
}

/**
 * @brief 刷新「快速绘图」动作的可用状态
 *
 * 当前没有数据表页、或表格里没有选区时置灰（实施方案 §3.1）。
 */
void DAAppController::updateQuickPlotActionState()
{
    if (!mActions || !mActions->actionQuickPlot || !mDock) {
        return;
    }
    // 早于数据操作区创建时也要安全（getCurrentDataFrameOperateWidget 内部未判空）
    DADataOperateWidget* dataWidget = mDock->getDataOperateWidget();
    DADataOperateOfDataFrameWidget* dfopt = dataWidget ? dataWidget->getCurrentDataFrameWidget() : nullptr;
    mActions->actionQuickPlot->setEnabled(dfopt && dfopt->isDataframeTableHaveSelection());
}

/**
 * @brief 表内查找（002 T1.4，Ribbon「编辑」面板）
 *
 * 唤出当前数据页的非模态浮动查找条（仅查找行）。快捷键 Ctrl+F 在 DADataTableView
 * 处理（红线5：action 不设 shortcut，避免双触发），此处仅 Ribbon 入口。
 */
void DAAppController::onActionFindTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showFindReplace(false);
}

/**
 * @brief 表内替换（002 T1.4，Ribbon「编辑」面板）
 *
 * 唤出当前数据页浮动条并展开替换行。快捷键 Ctrl+H 在 DADataTableView 处理。
 */
void DAAppController::onActionReplaceTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showFindReplace(true);
}

/**
 * @brief 条件格式（003 T1.5，Ribbon「Table Style」面板）
 *
 * 唤出当前数据页的条件格式规则编辑对话框（作用于选区所在列）。
 * 与表体右键菜单「条件格式...」共用页面 showConditionalFormatDialog 入口（R16）。
 */
void DAAppController::onActionConditionalFormatTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showConditionalFormatDialog();
}

/**
 * @brief 计算列（004 T1.23，Ribbon「Data Tools」面板）
 *
 * 唤出当前数据页的计算列对话框（新建）。与表体右键菜单「计算列...」共用页面
 * showComputedColumnDialog 入口；脏标记经页面 dataModified 信号统一接线。
 */
void DAAppController::onActionComputedColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showComputedColumnDialog(-1);
}

/**
 * @brief 全部重算计算列（004 T1.23，Ribbon「Data Tools」面板）
 *
 * 重算当前数据页全部待重算/失效的计算列（R8 手动重算；一个宏命令 = 一个撤销单元）。
 */
void DAAppController::onActionRecomputeAllComputedColumnsTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->recomputeAllStaleComputedColumns();
}

/**
 * @brief 分列（004 T1.9，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「分列...」共用页面 showSplitColumnDialog 入口：
 * 作用于当前选区的唯一数据列（页面内校验并提示）。
 */
void DAAppController::onActionSplitColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showSplitColumnDialog(-1);
}

/**
 * @brief 合并列（004 T1.9，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「合并列...」共用页面 showMergeColumnsDialog 入口：
 * 作用于当前选区的多列（页面内校验并提示）。
 */
void DAAppController::onActionMergeColumnsTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showMergeColumnsDialog();
}

/**
 * @brief 分组聚合（004 T1.22，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「分组聚合...」共用页面 showGroupByDialog 入口：产出新数据集
 * （addData_ 可撤销注册），新页面激活经页面 newDataCreated 信号统一接线。
 */
void DAAppController::onActionGroupByTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showGroupByDialog();
}

/**
 * @brief 转置（004 T1.27，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「转置...」共用页面 showReshapeDialog(false) 入口；
 * 默认产出新数据集，就地替换为对话框勾选项（R14）。
 */
void DAAppController::onActionTransposeTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showReshapeDialog(false);
}

/**
 * @brief 宽转长 melt（004 T1.27，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「宽转长（melt）...」共用页面 showReshapeDialog(true) 入口；
 * melt 规模守卫在对话框 OK 确认点（行数×展开列数 > 5000 万默认拒绝，可强制继续）。
 */
void DAAppController::onActionMeltTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showReshapeDialog(true);
}

/**
 * @brief 向下填充（005 T1.24，Ribbon「Edit」面板下拉 + Ctrl+D + 表体右键）
 *
 * 与表体右键「填充 → 向下填充」、DADataTableView 的 Ctrl+D 共用页面 fillSelection 入口；
 * 求值走 Python 算子（空值判定），一次填充一条 paste 命令入栈（D4）。
 */
void DAAppController::onActionFillDownTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->fillSelection(DADataOperateOfDataFrameWidget::FillMode::Down);
}

/**
 * @brief 向上填充（005 T1.24）
 */
void DAAppController::onActionFillUpTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->fillSelection(DADataOperateOfDataFrameWidget::FillMode::Up);
}

/**
 * @brief 向下填充空值 ffill（005 T1.24）
 */
void DAAppController::onActionFillForwardTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->fillSelection(DADataOperateOfDataFrameWidget::FillMode::Forward);
}

/**
 * @brief 向上填充空值 bfill（005 T1.24）
 */
void DAAppController::onActionFillBackwardTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->fillSelection(DADataOperateOfDataFrameWidget::FillMode::Backward);
}

/**
 * @brief 序列填充（005 T1.24，Ribbon「Edit」面板下拉 + 表体右键）
 *
 * 唤出序列填充对话框（数值等差/日期步进，Python 求值+预览）。
 */
void DAAppController::onActionFillSeriesTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showFillSeriesDialog();
}

/**
 * @brief 校验规则管理（005 T1.25，Ribbon「Data Tools」面板）
 *
 * 与表体右键菜单「校验规则...」共用页面 showValidationRulesDialog 入口；
 * 规则增删改经 setRules 命令入页撤销栈（D4）。
 */
void DAAppController::onActionValidationRulesTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showValidationRulesDialog();
}

/**
 * @brief 执行校验（005 T1.25，Ribbon「Data Tools」面板）
 *
 * 只读操作（红线2：不改数据、不入撤销栈）；违规装载 Source::Validation 高亮并弹报告。
 */
void DAAppController::onActionRunValidationTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->runValidationAndShowReport();
}

/**
 * @brief 插入/编辑批注（005 T1.26，Ribbon「Edit」面板下拉）
 *
 * 与表体右键菜单共用页面弹层入口（无批注=插入、有批注=编辑且弹层内可删除）。
 */
void DAAppController::onActionNoteTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showNoteEditorForCurrentCell();
}

/**
 * @brief 冻结列（006 T1.8，Ribbon「View」面板；主体 = 弹「冻结前 N 列」输入框）
 */
void DAAppController::onActionFreezeColumnsTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->promptFreezeColumns();
}

/**
 * @brief 隐藏列（006 T1.8）：隐藏选中的列（入撤销栈）
 */
void DAAppController::onActionHideColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    const QList< int > cols = dfopt->getSelectedDataframeCoumns();
    if (cols.isEmpty()) {
        daWarning << tr("Please select a column first");  // cn:请先选择列（复用页面既有源串，本 context 另建条目）
        return;
    }
    for (int c : cols) {
        dfopt->hideColumnAt(c);
    }
}

/**
 * @brief 显示所有列（006 T1.8）
 */
void DAAppController::onActionShowAllColumnsTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showAllColumns();
}

/**
 * @brief 列视图对话框（006 T1.8）
 */
void DAAppController::onActionColumnViewDialogTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showColumnViewDialog();
}

/**
 * @brief 冻结首列（006 T1.8，冻结菜单项）
 */
void DAAppController::onActionFreezeFirstColumnTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->freezeFirstColumn();
}

/**
 * @brief 冻结前 N 列…（006 T1.8，冻结菜单项，与主体同一实现）
 */
void DAAppController::onActionFreezeColumnsPromptTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->promptFreezeColumns();
}

/**
 * @brief 取消全部冻结（006 T1.8，冻结菜单项）
 */
void DAAppController::onActionUnfreezeColumnsTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->unfreezeAllColumns();
}

/**
 * @brief 定位（007 T1.7）：打开定位对话框（Ctrl+G 与滚动条右键共用同一入口）
 */
void DAAppController::onActionGotoCellTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showGotoDialog();
}

// ============================================================
// 排序（Ribbon「Sort」面板；与列头右键排序组共用页面入口）
// ============================================================

/**
 * @brief 刷新「排序」动作的可用状态
 *
 * 没有数据表页、或当前表为只读源（惰性/数据库表）时整组置灰——面板大按钮禁用同时
 * 也让下拉菜单不可展开。菜单项与面板按钮共用同一批 QAction，状态自然同步。
 */
void DAAppController::updateSortActionState()
{
    if (!mActions || !mActions->actionSort || !mDock) {
        return;
    }
    DADataOperateWidget* dataWidget        = mDock->getDataOperateWidget();
    DADataOperateOfDataFrameWidget* dfopt  = dataWidget ? dataWidget->getCurrentDataFrameWidget() : nullptr;
    const bool enabled                     = dfopt && dfopt->haveData() && dfopt->isTableEditable();
    mActions->actionSort->setEnabled(enabled);
    mActions->actionSortAscending->setEnabled(enabled);
    mActions->actionSortDescending->setEnabled(enabled);
    mActions->actionSortCustom->setEnabled(enabled);
    mActions->actionSortClear->setEnabled(enabled);
}

/**
 * @brief 排序大按钮 / 自定义排序…：打开自定义排序对话框
 *
 * 预置规则由页面决定：已有排序规格时按规格预填，否则以当前列为唯一主键。
 */
void DAAppController::onActionSortTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->showSortDialog();
}

/**
 * @brief 升序排序（按当前列；无当前列时提示先选列）
 */
void DAAppController::onActionSortAscendingTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    const int col = dfopt->currentDataColumnForSort();
    if (col < 0) {
        daWarning << tr("Please select a column to sort first");  // cn:请先选择要排序的列
        return;
    }
    dfopt->sortColumn(col, Qt::AscendingOrder);
}

/**
 * @brief 降序排序（按当前列；无当前列时提示先选列）
 */
void DAAppController::onActionSortDescendingTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    const int col = dfopt->currentDataColumnForSort();
    if (col < 0) {
        daWarning << tr("Please select a column to sort first");  // cn:请先选择要排序的列
        return;
    }
    dfopt->sortColumn(col, Qt::DescendingOrder);
}

/**
 * @brief 清除排序：按索引升序恢复原始行顺序（行头的 index 从乱序还原，可撤销）
 */
void DAAppController::onActionSortClearTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->restoreOriginalSortOrder();
}


/**
 * @brief 删除当前格批注（005 T1.26）
 */
void DAAppController::onActionDeleteNoteTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->deleteNoteAtCurrentCell();
}

/**
 * @brief 清除本表批注（005 T1.26，一次命令可撤销）
 */
void DAAppController::onActionClearNotesTriggered()
{
    DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget(false, false);
    if (!dfopt) {
        return;
    }
    dfopt->clearAllNotes();
}


/**
 * @brief 填充共用的列操作右键菜单项
 *
 * 供数据管理树 series 节点右键复用（表头右键菜单已迁入 DADataHeaderView，R12）。
 * 6 项 4 组：重命名列 / 删除列 ‖ 复制列名 ‖ 定位最大 / 定位最小 ‖ 描述统计。
 * 走到这里 col 已由调用方选中，菜单项全部 enable。
 * @param menu 目标菜单
 */
void DAAppController::populateColumnContextMenu(QMenu& menu)
{
    menu.addAction(mActions->actionRenameColumn);
    menu.addAction(mActions->actionRemoveColumn);
    menu.addSeparator();
    menu.addAction(mActions->actionCopyColumnName);
    menu.addSeparator();
    menu.addAction(mActions->actionGotoMax);
    menu.addAction(mActions->actionGotoMin);
    menu.addSeparator();
    menu.addAction(mActions->actionShowColumnDescribe);
}

void DAAppController::selectColumnInDataFrameWidget(DADataOperateOfDataFrameWidget* w, int col)
{
    if (!w || col < 0) {
        return;
    }
    DADataTableView* tv = w->getDataTableView();
    if (!tv) {
        return;
    }
    QItemSelectionModel* sel = tv->selectionModel();
    QAbstractItemModel* m    = tv->model();
    if (!sel || !m) {
        return;
    }
    sel->clearSelection();
    QItemSelection selRange(m->index(0, col), m->index(m->rowCount() - 1, col));
    sel->select(selRange, QItemSelectionModel::Select | QItemSelectionModel::Columns);
}

/**
 * @brief 为数据管理树注入右键菜单
 *
 * - series 节点：取 DAData + seriesName → 列名转列索引 → 找到（或打开）对应的
 *   DataFrame 操作窗口 → 选中该列 → 弹出与表头右键相同的菜单（共用列操作 action）。
 * - 数据集（dataframe）条目：右键先选中该条目（rename 槽基于树 currentIndex，
 *   remove/export 槽基于树选中集），再弹出「重命名 / 移除数据 / 导出」菜单，
 *   菜单项全部复用 ribbon 数据标签页已有 action（重命名=actionRenameData，
 *   移除=actionRemoveData，导出子菜单=actionExportData{Csv,Excel,Pickle,Parquet}）。
 * - 属性列（第二列）上的点击统一映射回名称列。
 * @param w 数据管理树窗口
 */
void DAAppController::setupDataManagerTreeContextMenu(DADataManagerTreeWidget* w)
{
    if (!w) {
        return;
    }
    QTreeView* tv = w->getTreeView();
    if (!tv) {
        return;
    }
    tv->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tv, &QWidget::customContextMenuRequested, this, [ this, w, tv ](const QPoint& pos) {
        QModelIndex proxyIndex = tv->indexAt(pos);
        if (!proxyIndex.isValid()) {
            return;
        }
        // 属性列（第二列）条目映射回名称列
        if (proxyIndex.column() != 0) {
            proxyIndex = proxyIndex.siblingAtColumn(0);
        }
        // 映射到 source model
        DADataManagerTreeFilterProxyModel* proxy = w->getProxyModel();
        QModelIndex srcIndex                     = proxy ? proxy->mapToSource(proxyIndex) : proxyIndex;
        QStandardItem* item                      = w->getModel()->itemFromIndex(srcIndex);
        if (!item) {
            return;
        }
        if (DADataManagerTreeModel::isDataframeSeriesItem(item)) {
            // ---- series 节点：与表头右键一致的列操作菜单 ----
            DAData data = DADataManagerTreeModel::itemToData(item);
            if (!data.isDataFrame()) {
                return;
            }
            QString seriesName = item->text();
            DAPyDataFrame df   = data.toDataFrame();
            if (df.isNone()) {
                return;
            }
            // 列名 → 列索引
            QList< QString > cols = df.columns();
            int col               = cols.indexOf(seriesName);
            if (col < 0) {
                return;
            }
            // 查找已打开的 DataFrame 窗口，没有则打开
            DADataOperateOfDataFrameWidget* dfopt = getDataOperateWidget()->findDataFrameWidget(data);
            if (!dfopt) {
                getDataOperateWidget()->showData(data);
                dfopt = getDataOperateWidget()->findDataFrameWidget(data);
            }
            if (!dfopt) {
                return;
            }
            // 弹共用菜单，但重命名/删除列换成临时 action 绑直入口（不经焦点检查——
            // 树右键时焦点在数据管理器 dock，共用 action 绑定的槽会弹
            // 「请选中数据操作窗口」）；其余动作先选中该列，共用槽经
            // getSelectedOneDataframeColumn 取列
            QMenu menu(w);
            populateColumnContextMenu(menu);
            {
                QList< QAction* > acts = menu.actions();
                for (int i = 0; i < acts.size(); ++i) {
                    if (acts.at(i) == mActions->actionRenameColumn) {
                        QAction* local = new QAction(mActions->actionRenameColumn->icon(),
                                                     mActions->actionRenameColumn->text(), &menu);
                        connect(local, &QAction::triggered, this, [ this, dfopt, col ]() {
                            renameColumnOnPage(dfopt, col);
                        });
                        menu.insertAction(acts.at(i), local);
                        menu.removeAction(acts.at(i));
                    } else if (acts.at(i) == mActions->actionRemoveColumn) {
                        QAction* local = new QAction(mActions->actionRemoveColumn->icon(),
                                                     mActions->actionRemoveColumn->text(), &menu);
                        connect(local, &QAction::triggered, this, [ this, dfopt, col ]() {
                            if (dfopt->removeColumnAt(col) > 0) {
                                setDirty();
                            }
                        });
                        menu.insertAction(acts.at(i), local);
                        menu.removeAction(acts.at(i));
                    }
                }
            }
            // 选中该列（非重命名/删除的共用槽经 getSelectedOneDataframeColumn 取列）
            selectColumnInDataFrameWidget(dfopt, col);
            menu.exec(tv->viewport()->mapToGlobal(pos));
            return;
        }
        if (!DADataManagerTreeModel::isDataframeItem(item)) {
            return;
        }
        // ---- 数据集（dataframe）条目：重命名 / 移除数据 / 复制为 / 导出 ----
        // 右键先选中该条目，保证后续槽函数操作的就是此条目
        tv->setCurrentIndex(proxyIndex);
        QMenu menu(w);
        menu.addAction(mActions->actionRenameData);
        menu.addAction(mActions->actionRemoveData);
        // 复制为（整表复制到剪贴板；Tab/Markdown/HTML，与导出共用 DATableExporter）
        QMenu* copyMenu = menu.addMenu(mActions->actionCopy->icon(), tr("Copy As"));  // cn:复制为
        QAction* actCopyTab = copyMenu->addAction(tr("Tab-separated Text"));  // cn:Tab 分隔文本
        QAction* actCopyMd  = copyMenu->addAction(tr("Markdown Table"));      // cn:Markdown 表格
        QAction* actCopyHtm = copyMenu->addAction(tr("HTML Table (with styles)"));  // cn:HTML 表格（含样式）
        connect(actCopyTab, &QAction::triggered, this, [ this ]() {
            copySelectedDataAsTable(DATableExporter::Format::TabText);
        });
        connect(actCopyMd, &QAction::triggered, this, [ this ]() {
            copySelectedDataAsTable(DATableExporter::Format::Markdown);
        });
        connect(actCopyHtm, &QAction::triggered, this, [ this ]() {
            copySelectedDataAsTable(DATableExporter::Format::Html);
        });
        // 导出（csv/excel/pickle/parquet 复用 ribbon action；Markdown/HTML 走文档导出）
        QMenu* exportMenu = menu.addMenu(tr("Export"));  // cn:导出
        exportMenu->setIcon(mActions->actionExportData->icon());
        exportMenu->addAction(mActions->actionExportDataCsv);
        exportMenu->addAction(mActions->actionExportDataExcel);
        exportMenu->addAction(mActions->actionExportDataPickle);
        exportMenu->addAction(mActions->actionExportDataParquet);
        exportMenu->addSeparator();
        QAction* actExportMd  = exportMenu->addAction(tr("Markdown Document..."));  // cn:Markdown 文档...
        QAction* actExportHtm = exportMenu->addAction(tr("HTML Document (with styles)..."));  // cn:HTML 文档（含样式）...
        connect(actExportMd, &QAction::triggered, this, [ this ]() {
            exportSelectedDataAsDocument(DATableExporter::Format::Markdown);
        });
        connect(actExportHtm, &QAction::triggered, this, [ this ]() {
            exportSelectedDataAsDocument(DATableExporter::Format::Html);
        });
        menu.exec(tv->viewport()->mapToGlobal(pos));
    });
}

/**
 * @brief 选中列转换为数值
 */
void DAAppController::onActionCastToNumTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->castSelectToNum();
        setDirty();
    }
}

/**
 * @brief 选中列转换为文字
 */
void DAAppController::onActionCastToStringTriggered()
{
    DAAPPCONTROLLER_PASS();
}

/**
 * @brief 选中列转换为日期
 */
void DAAppController::onActionCastToDatetimeTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->castSelectToDatetime();
        setDirty();
    }
}

/**
 * @brief 选中列转换为索引
 */
void DAAppController::onActionChangeToIndexTriggered()
{
    if (DADataOperateOfDataFrameWidget* dfopt = getCurrentDataFrameOperateWidget()) {
        dfopt->changeSelectColumnToIndex();
        setDirty();
    }
}


/**
 * @brief 显示标记线
 *
 * 此action有个menu，menu的action选中会设置当前action的图标，具体实现放在@ref DAAppRibbonArea::buildContextCategoryWorkflowView 函数中
 * @param on
 * @sa DAAppRibbonArea::buildContextCategoryWorkflowView
 */
void DAAppController::onActionWorkflowViewMarkerTriggered(bool on)
{
    auto wo = mDock->getWorkFlowOperateWidget();
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
void DAAppController::onActionNewWorkflowTriggered()
{
    bool ok      = false;
    QString text = QInputDialog::getText(app(),
                                         tr("New workflow name"),   // cn:新工作流名称
                                         tr("New workflow name:"),  // cn:新工作流名称
                                         QLineEdit::Normal,
                                         QString(),
                                         &ok);
    if (!ok || text.isEmpty()) {
        return;
    }
    DAPyWorkFlowOperateWidget* wf = mDock->getWorkFlowOperateWidget();
    wf->appendWorkflow(text);
    setDirty();
}

/**
 * @brief 绘制矩形
 *
 * * @note 绘制完成后会触发onWorkFlowGraphicsSceneMouseActionFinished，在此函数中把这个状态消除
 * @param on
 */
void DAAppController::onActionStartDrawRectTriggered(bool on)
{
    if (on) {
        mDock->getWorkFlowOperateWidget()->setPreDefineSceneAction(DAPyWorkFlowGraphicsScene::AddRectItemAction);
    }
}
/**
 * @brief 绘制文本
 *
 * @note 绘制完成后会触发onWorkFlowGraphicsSceneMouseActionFinished，在此函数中把这个状态消除
 * @param on
 */
void DAAppController::onActionStartDrawTextTriggered(bool on)
{
    if (on) {
        mDock->getWorkFlowOperateWidget()->setPreDefineSceneAction(DAPyWorkFlowGraphicsScene::AddTextItemAction);
    }
}

/**
 * @brief 允许连线
 * @param on
 */
void DAAppController::onActionWorkflowLinkEnableTriggered(bool on)
{
    if (auto wo = mDock->getWorkFlowOperateWidget()) {
        wo->setEnableWorkflowLink(on);
    }
}

/**
 * @brief 表格底色变更
 * @param c 颜色
 */
void DAAppController::onTableStyleFillColorChanged(const QColor& c)
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    DA::DATableCellStyle fragment;
    fragment.setBackground(QBrush(c));
    w->mergeStyleToSelection(fragment);
    qDebug() << "DAAppController::onTableStyleFillColorChanged(" << c << ")";
}

/**
 * @brief 表格字体变更
 * @param f 字体
 */
void DAAppController::onTableStyleFontChanged(const QFont& f)
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    DA::DATableCellStyle fragment;
    fragment.setFont(f);
    w->mergeStyleToSelection(fragment);
}

/**
 * @brief 表格字体颜色变更
 * @param c 颜色
 */
void DAAppController::onTableStyleFontColorChanged(const QColor& c)
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    DA::DATableCellStyle fragment;
    fragment.setForeground(c);
    w->mergeStyleToSelection(fragment);
}

/**
 * @brief 清除选中区样式
 */
void DAAppController::onActionClearStyleSelectedTriggered()
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (w) {
        w->clearStyleSelection();
    }
}

/**
 * @brief 清除整表所有样式
 */
void DAAppController::onActionClearStyleAllTriggered()
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (w) {
        w->clearStyleAll();
    }
}

/**
 * @brief 选中区样式反向同步 ribbon 控件
 *
 * 根据选中区代表样式更新 ribbon 的底色按钮/字体面板。
 * 各属性独立判断：valid 的属性更新对应控件，invalid 的不调整。
 * 用 blockSignals 避免控件信号触发循环。
 * @param style 选中区代表样式
 */
void DAAppController::onTableStyleCurrentChanged(const DA::DATableCellStyle& style)
{
    // 底色按钮
    if (mRibbon && mRibbon->mBtnTableFillColor) {
        QSignalBlocker block(mRibbon->mBtnTableFillColor);
        if (style.backgroundValid()) {
            mRibbon->mBtnTableFillColor->setColor(style.background().color());
        }
    }
    // 字体面板
    if (mRibbon && mRibbon->mWidgetTableFont) {
        QSignalBlocker block(mRibbon->mWidgetTableFont);
        if (style.fontValid()) {
            mRibbon->mWidgetTableFont->setCurrentFont(style.font());
        }
        if (style.foregroundValid()) {
            mRibbon->mWidgetTableFont->setCurrentFontColor(style.foreground());
        }
    }
}

/**
 * @brief 显示格式下拉框类别变化：对选中列应用该类别（默认参数）
 * @param c 选中的格式类别
 */
void DAAppController::onTableDisplayFormatCategoryChanged(DA::DATableDisplayFormat::Category c)
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    if (c == DA::DATableDisplayFormat::General) {
        w->clearDisplayFormatFromSelection();
    } else {
        DA::DATableDisplayFormat fmt(c);
        w->setDisplayFormatToSelection(fmt);
    }
    setDirty();
}

/**
 * @brief “设置单元格格式”按钮：打开对话框，OK 后应用结果
 *
 * 对话框堆分配、首次创建后复用，每次 setup(current/dtype/sample) 重置状态再 exec。
 */
void DAAppController::onActionTableFormatCellsTriggered()
{
    DADataOperateOfDataFrameWidget* w = getCurrentDataFrameOperateWidget(false, false);
    if (!w) {
        return;
    }
    int col = w->getSelectedOneDataframeColumn(false);
    if (col < 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    DAPyDataFrame df = w->getDataframe();
    DA::DAPyDType dt;
    QVariant sample;
    try {
        if (!df.isNone()) {
            dt     = df.dtypeObject(static_cast< std::size_t >(col));
            sample = df.iat(0, static_cast< std::size_t >(col));
        }
    } catch (const std::exception& e) {
        qWarning() << "DADialogTableDisplayFormat prepare:" << e.what();
    }
    if (!mDialogTableDisplayFormat) {
        mDialogTableDisplayFormat = new DA::DADialogTableDisplayFormat(app());
    }
    mDialogTableDisplayFormat->setup(w->getCurrentColumnDisplayFormat(), dt, sample);
    if (mDialogTableDisplayFormat->exec() == QDialog::Accepted) {
        DA::DATableDisplayFormat fmt = mDialogTableDisplayFormat->getResult();
        if (fmt.isValid()) {
            w->setDisplayFormatToSelection(fmt);
        } else {
            w->clearDisplayFormatFromSelection();
        }
        setDirty();
    }
}

/**
 * @brief 选中列显示格式反向同步 ribbon 格式下拉框
 * @param fmt 当前代表格式（invalid 表示无格式）
 */
void DAAppController::onTableDisplayFormatCurrentChanged(const DA::DATableDisplayFormat& fmt)
{
    if (mRibbon && mRibbon->mComboxDisplayFormat) {
        QSignalBlocker block(mRibbon->mComboxDisplayFormat);
        mRibbon->mComboxDisplayFormat->setCurrentFormat(fmt);
    }
}

/**
 * @brief 统计绘图引导对话框确认后的处理槽
 *
 * 类型化请求由信号携带（含参数、数据源与目标 figure/chart），widget 侧
 * isValid 已完成校验；这里执行编排并做事后 UI（提升绘图区 / 更新 ribbon /
 * 重设坐标轴）。
 */
void DAAppController::onStatsGuideAccepted(const DA::DAStatsPlotRequest& req)
{
    if (nullptr == req.chart || nullptr == req.figure) {
        return;
    }

    DAWaitCursorScoped wait;
    Q_UNUSED(wait);

    DAStatsPlotCoordinator coordinator;
    coordinator.execute(req);

    mDock->raiseDockingArea(DAAppDockingArea::DockingAreaChartOperate);
    mRibbon->updateFigureAboutRibbon(req.figure);
    if (DAAppChartOperateWidget* chartopt = getChartOperateWidget()) {
        chartopt->setCurrentFigure(req.figure);
    }
    // 让坐标轴显示正常
    req.chart->rescaleAxes();
}

}  // end DA
