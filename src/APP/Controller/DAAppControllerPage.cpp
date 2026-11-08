#include "DAAppController.h"
#include "DAAppTableActionHandler.h"
#include "DAAppDataActionHandler.h"
// Qt
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPointer>
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
#include "Table/Chart/DATableQuickPlotSettingWidget.h"
#include "Table/Chart/DATableQuickPlotWindow.h"
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
 * @brief 数据操作窗口添加，需要绑定相关信号槽到ribbon的页面
 * @param page
 */
void DAAppController::onDataOperatePageCreated(DADataOperatePageWidget* page)
{
    if (mCommand) {
        mCommand->addStack(page->getUndoStack());
    }
    // 撤销栈深度从配置施加（005 T1.21 R19：默认 20，历史面板可调大；配置在 APP 侧读写，
    // DAGui 只暴露 undoLimit()/setUndoLimit(int)，严禁反向依赖）
    if (mConfig && page) {
        const int cfgLimit = mConfig->ui().undoStackLimit;
        if (cfgLimit > 0) {
            page->setUndoLimit(cfgLimit);
        }
    }
    switch (page->getDataOperatePageType()) {
    case DADataOperatePageWidget::DataOperateOfDataFrame: {
        DADataOperateOfDataFrameWidget* w = static_cast< DADataOperateOfDataFrameWidget* >(page);
        connect(w,
                &DADataOperateOfDataFrameWidget::selectTypeChanged,
                mTableHandler,
                &DAAppTableActionHandler::onDataOperateDataFrameWidgetSelectTypeChanged);
        // 选中变化时反向同步 ribbon 样式控件
        connect(w,
                &DADataOperateOfDataFrameWidget::currentStyleChanged,
                mTableHandler,
                &DAAppTableActionHandler::onTableStyleCurrentChanged);
        // 选中列变化时反向同步 ribbon 显示格式下拉框
        connect(w,
                &DADataOperateOfDataFrameWidget::currentDisplayFormatChanged,
                mTableHandler,
                &DAAppTableActionHandler::onTableDisplayFormatCurrentChanged);
        // 替换生效后置工程未保存标记（002 T1.4；含超阈值不可撤销降级路径，与剪切/粘贴同口径）
        connect(w, &DADataOperateOfDataFrameWidget::replaceApplied, this, [ this ](int changedCells) {
            if (changedCells > 0) {
                setDirty();
            }
        });
        // 数据工具改动数据后置工程未保存标记（004：计算列创建/重算等，exec/undo 都发）
        connect(w, &DADataOperateOfDataFrameWidget::dataModified, this, [ this ]() {
            setDirty();
        });
        // ===== 表内快速绘图（快速图表预览）=====
        // 转入绘图区需要 DAChartOperateWidget，由 APP 注入处理器：DAGui 页面不反向依赖 APP。
        // 浮窗与正式图表共用 DAChartQuickPlot 这一份「请求 + 规格 + 构建」实现。
        w->setQuickPlotTransferHandler([ this ](const DA::DATableChartRequest& req,
                                                const DA::DATableChartSpec& spec,
                                                bool append,
                                                QString* error) -> bool {
            DAChartOperateWidget* cow = mDock ? mDock->getChartOperateWidget() : nullptr;
            if (!cow) {
                if (error) {
                    *error = tr("The chart area is not available");  // cn:绘图区不可用
                }
                return false;
            }
            if (DA::DAChartQuickPlot::is3DType(spec.type)) {
                if (append && !cow->getCurrentFigure()) {
                    if (error) {
                        *error = tr("There is no current chart to append to");  // cn:当前没有可追加的图表
                    }
                    return false;
                }
                return DA::DAChartQuickPlot::transferToFigure3D(req, spec, cow, append, nullptr, error);
            }
            if (append && !cow->getCurrentChart()) {
                if (error) {
                    *error = tr("There is no current chart to append to");  // cn:当前没有可追加的图表
                }
                return false;
            }
            return DA::DAChartQuickPlot::transferToFigure(req, spec, cow, append, nullptr, error) != nullptr;
        });
        // 没有当前图表时浮窗「追加到当前图表」置灰（实施方案 §6.1）；
        // 3D 图型追加到当前 Figure 的 3D 图表，所以有 Figure 即可
        w->setQuickPlotAppendAvailabilityProvider([ this ]() -> bool {
            DAChartOperateWidget* cow = mDock ? mDock->getChartOperateWidget() : nullptr;
            if (!cow) {
                return false;
            }
            if (cow->getCurrentChart() != nullptr) {
                return true;
            }
            DAFigureWidget* fig = cow->getCurrentFigure();
            return fig && fig->getCurrent3DChart() != nullptr;
        });
        // 快速绘图设置页在右侧「设置」Dock 里：打开/点击绘图区时把设置页绑到该窗口、
        // 切到快速绘图设置页并把 Dock 抬到顶层（Dock 归 APP 管，故由 APP 注入动作）。
        // 捕获 QPointer 而不是裸 mDock：解绑器会在页面析构路径被调用，关闭程序时
        // 停靠区可能先于页面被销毁（deleteLater），裸指针会悬空。
        const QPointer< DAAppDockingArea > dockPtr = mDock;
        const QPointer< DASettingContainerWidget > settingPtr = mDock ? mDock->getSettingContainerWidget() : nullptr;
        w->setQuickPlotSettingBinder([ this, dockPtr, settingPtr ](DA::DATableQuickPlotWindow* window) {
            if (!window || !dockPtr || !settingPtr) {
                return;
            }
            DASettingContainerWidget* setting = settingPtr.data();
            DATableQuickPlotSettingWidget* panel = setting->getQuickPlotSettingWidget();
            if (!panel) {
                return;
            }
            panel->setTarget(window);
            setting->showQuickPlotSettingWidget();
            dockPtr->raiseDockByWidget(setting);
        });
        // 绘图区关闭/切页/页面析构：设置页解绑回空态（只解绑与当前目标一致的那个窗口）
        w->setQuickPlotSettingReleaser([ settingPtr ](DA::DATableQuickPlotWindow* window) {
            if (!settingPtr) {
                return;
            }
            if (DATableQuickPlotSettingWidget* panel = settingPtr->getQuickPlotSettingWidget()) {
                panel->releaseTarget(window);
            }
        });
        // 选区变化时刷新「快速绘图」动作的可用状态（无选区置灰）
        connect(w, &DADataOperateOfDataFrameWidget::tableSelectionChanged, this, [ this ]() {
            mTableHandler->updateQuickPlotActionState();
        });
        mTableHandler->updateQuickPlotActionState();
        mTableHandler->updateSortActionState();
        // 新数据集产出（004 T1.22 分组聚合等）：置脏 + 抬起数据操作区 + 激活新页面
        //（照 DataframeOperateWorker::createPivotTable 的产出流程；注册已由页面走 addData_ 完成）
        connect(w, &DADataOperateOfDataFrameWidget::newDataCreated, this, [ this ](const DAData& newData) {
            setDirty();
            if (mDock) {
                mDock->raiseDockingArea(DADockingAreaInterface::DockingAreaDataOperate);
            }
            if (DADataOperateWidget* dow = getDataOperateWidget()) {
                dow->showData(newData);
            }
        });
        // 就地数据替换（004 T1.27 转置/melt 就地路径）：Swap 命令必须经 DAAppCommand
        // begin/end 建（统一撤销链，DAGui 侧不 new）；元数据清理与数据替换同一撤销
        // 单元——directional 回调 redo 清束 / undo 还原页面捕获的整束 XML 快照
        connect(w,
                &DADataOperateOfDataFrameWidget::inplaceDataReplaceRequested,
                this,
                [ this, w ](const DAData& data, const DAData& source, const QString& text, const QString& metadataSnapshot) {
                    if (!mCommand) {
                        return;
                    }
                    DADataAbstractUndoCommand* cmd = mCommand->beginDataOperateCommand(data, text, false /*isObjectPersist=false → Swap 内存驻留*/);
                    DATableMetadata* bundle        = nullptr;
                    if (DADataOperateWidget* dow = getDataOperateWidget()) {
                        if (DATableMetadataRegistry* reg = dow->metadataRegistry()) {
                            bundle = reg->get(data);
                        }
                    }
                    QPointer< DADataOperateOfDataFrameWidget > wp(w);
                    if (cmd) {
                        cmd->setDirectionalCallBack([ wp, bundle, metadataSnapshot ](bool isUndo) {
                            if (bundle) {
                                if (isUndo) {
                                    bundle->restoreFromString(metadataSnapshot);
                                } else {
                                    bundle->clearAll();
                                }
                            }
                            if (wp) {
                                wp->refreshAfterInplaceReplace();
                            }
                        });
                    }
                    DAData mutableData = data;  // DAData 共享底层对象，副本上 setPyObject 即生效
                    mutableData.setPyObject(source.toPyObject());  // 执行替换
                    if (cmd) {
                        // push → 首次 redo：Swap 重设新对象（幂等）+ directional 清元数据
                        mCommand->endDataOperateCommand(data);
                    } else if (bundle) {
                        // 惰性源不支持快照（begin 返回 nullptr）：不可撤销兜底执行
                        bundle->clearAll();
                    }
                    if (wp) {
                        wp->refreshAfterInplaceReplace();
                    }
                    setDirty();
                });
        // 列头右键菜单已迁入 DADataHeaderView（R12，菜单内容单点定义在 DAGui）：
        // 这里只做信号接线——排序类动作直接执行（**不预选列**：整列选择会产生上万个
        // 选区 range，拖慢排序的选区重映射且对排序无意义）；重命名/删除列直接用
        // 信号携带的 (页面, 列号) 走页面直入口（右键菜单是独立窗口会抢走 ADS 停靠区
        // 焦点，且选区可能不在目标列上——经 getCurrentDataFrameOperateWidget 的
        // 焦点检查会失败弹「请选中数据操作窗口」）；其余动作先选中该列
        // （复用 selectColumnInDataFrameWidget，槽函数经 getSelectedOneDataframeColumn 取列），
        // 再执行既有槽逻辑，行为与迁移前一致
        if (DADataTableView* tv = w->getDataTableView()) {
            if (DADataHeaderView* hv = qobject_cast< DADataHeaderView* >(tv->horizontalHeader())) {
                connect(hv,
                        &DADataHeaderView::columnActionRequested,
                        this,
                        [ this, w ](int col, DA::DADataHeaderView::ColumnAction action) {
                            using ColumnAction = DA::DADataHeaderView::ColumnAction;
                            // 排序类（2026-10-18 交互修订：排序入口=列头右键菜单，点击表头不排序）
                            if (action == ColumnAction::SortAscending || action == ColumnAction::SortDescending
                                || action == ColumnAction::AddSecondarySortAscending
                                || action == ColumnAction::AddSecondarySortDescending) {
                                const Qt::SortOrder order = (action == ColumnAction::SortAscending
                                                             || action == ColumnAction::AddSecondarySortAscending)
                                                                ? Qt::AscendingOrder
                                                                : Qt::DescendingOrder;
                                if (action == ColumnAction::AddSecondarySortAscending
                                    || action == ColumnAction::AddSecondarySortDescending) {
                                    w->addSecondarySortColumn(col, order);
                                } else {
                                    w->sortColumn(col, order);
                                }
                                // 置脏由页面 dataModified 信号驱动（排序命令回调里发，exec/undo 全覆盖）
                                return;
                            }
                            // 自定义排序（以右键列为唯一主键预置）/ 清除排序（按索引恢复原始行序）
                            if (action == ColumnAction::CustomSort) {
                                w->showSortDialog(col);
                                return;
                            }
                            if (action == ColumnAction::ClearSort) {
                                w->restoreOriginalSortOrder();
                                return;
                            }
                            // 重命名/删除列：直传 (页面, 列号)，不依赖焦点与选区
                            if (action == ColumnAction::RenameColumn) {
                                mTableHandler->renameColumnOnPage(w, col);
                                return;
                            }
                            if (action == ColumnAction::RemoveColumn) {
                                if (w->removeColumnAt(col) > 0) {
                                    setDirty();
                                }
                                return;
                            }
                            // 快速绘图/统计信息：右键列已在多选内则保留整个选区
                            // （统计窗口支持多列对比），否则只选该列再执行
                            if (action == ColumnAction::QuickPlot || action == ColumnAction::ShowColumnDescribe) {
                                const QList< int > selected = w->getSelectedDataframeCoumns();
                                if (!selected.contains(col)) {
                                    mTableHandler->selectColumnInDataFrameWidget(w, col);
                                }
                                if (action == ColumnAction::QuickPlot) {
                                    w->showQuickPlot();
                                } else {
                                    mTableHandler->onActionShowColumnDescribeTriggered();
                                }
                                return;
                            }
                            mTableHandler->selectColumnInDataFrameWidget(w, col);
                            switch (action) {
                            case ColumnAction::CopyColumnName:
                                mTableHandler->onActionCopyColumnNameTriggered();
                                break;
                            case ColumnAction::GotoMax:
                                mTableHandler->onActionGotoMaxTriggered();
                                break;
                            case ColumnAction::GotoMin:
                                mTableHandler->onActionGotoMinTriggered();
                                break;
                            // 计算列三项（004 T1.23）：页面方法按列位置解析列名后执行
                            case ColumnAction::EditComputedFormula:
                                w->showComputedColumnDialog(col);
                                break;
                            case ColumnAction::RecomputeComputedColumn:
                                w->recomputeComputedColumn(col);
                                break;
                            case ColumnAction::RemoveComputedFormula:
                                w->removeComputedColumnFormula(col);
                                break;
                            // 快速绘图/统计信息已在前面前置分支处理（保留 case 保证枚举全覆盖）
                            case ColumnAction::QuickPlot:
                            case ColumnAction::ShowColumnDescribe:
                            // 重命名/删除列已在前置分支处理（保留 case 保证枚举全覆盖）
                            case ColumnAction::RenameColumn:
                            case ColumnAction::RemoveColumn:
                            // 排序类已在上面前置分支处理（保留 case 保证枚举全覆盖）
                            case ColumnAction::SortAscending:
                            case ColumnAction::SortDescending:
                            case ColumnAction::AddSecondarySortAscending:
                            case ColumnAction::AddSecondarySortDescending:
                            case ColumnAction::CustomSort:
                            case ColumnAction::ClearSort:
                                break;
                            }
                        });
                // 按选中列复制/导出（列头右键「复制选中列为/导出选中列」）：
                // 列范围由处理器经页面解析（无选中回退右键列），执行链路与数据
                // 管理树的 dataframe 复制/导出共用（只多一层列子集）。
                // 右键菜单是独立窗口会抢走停靠区焦点——处理器经捕获的页面 w 取数，
                // 不走 getCurrentDataFrameOperateWidget 的焦点检查（触发时焦点
                // 尚未归还表格页，焦点检查会失败导致复制静默不执行）
                connect(hv,
                        &DADataHeaderView::columnDataCopyRequested,
                        this,
                        [ this, w ](int col, DA::DADataHeaderView::ColumnDataFormat format) {
                            mDataHandler->onHeaderColumnDataCopyRequested(w, col, format);
                        });
                connect(hv,
                        &DADataHeaderView::columnDataExportRequested,
                        this,
                        [ this, w ](int col, DA::DADataHeaderView::ColumnDataFormat format) {
                            mDataHandler->onHeaderColumnDataExportRequested(w, col, format);
                        });
                // 「快速绘图」子菜单项：与右键列动作同一选区口径——右键列已在多选内
                // 则保留整个选区，否则只选该列；然后按子菜单指定的图型立即出图（v3）
                connect(hv,
                        &DADataHeaderView::quickPlotRequested,
                        this,
                        [ this, w ](int col, int chartType) {
                            const QList< int > selected = w->getSelectedDataframeCoumns();
                            if (!selected.contains(col)) {
                                mTableHandler->selectColumnInDataFrameWidget(w, col);
                            }
                            w->showQuickPlot(static_cast< DA::DATableChartType >(chartType));
                        });
            }
            // 表体右键菜单（v1 骨架：剪切/复制/粘贴）信号接线：路由到既有槽逻辑
            // （与 onActionCut/Copy/PasteTriggered 的数据页分支一致），不新增执行逻辑
            connect(tv, &DADataTableView::contextMenuCutRequested, this, [ this, w ]() {
                if (w->cutSelection() > 0) {
                    setDirty();
                }
            });
            connect(tv, &DADataTableView::contextMenuCopyRequested, this, [ this, w ]() {
                if (DADataTableView* t = w->getDataTableView()) {
                    if (!t->copySelectionToClipboard()) {
                        daWarning << tr("No cells selected to copy");  // cn:没有选中可复制的单元格
                    }
                }
            });
            connect(tv, &DADataTableView::contextMenuPasteRequested, this, [ this, w ]() {
                if (w->pasteFromClipboard() > 0) {
                    setDirty();
                }
            });
        }
    } break;
    default:
        break;
    }
}

}  // end DA
