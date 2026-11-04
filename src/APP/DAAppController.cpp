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


// ============================================================
// 排序（Ribbon「Sort」面板；与列头右键排序组共用页面入口）
// ============================================================


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

}  // end DA
