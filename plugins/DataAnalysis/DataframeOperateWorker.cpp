#include "DataframeOperateWorker.h"
#include <QDebug>
#include "DADataManagerInterface.h"
#include "DAUIInterface.h"
#include "DAData.h"
#include "DAPyModule.h"
#include "DADockingAreaInterface.h"
#include "DADataOperateWidget.h"
#include "DADataOperateOfDataFrameWidget.h"
#include "DADataTableView.h"
#include "DAPyScripts.h"
#include "DACoreInterface.h"
#include "SARibbonMainWindow.h"
#include "Models/DADataTableModel.h"
// Commands
#include "Commands.h"
// Dialogs
#include "Dialogs/DataFrameDataSearchDialog.h"
#include "Dialogs/DataFrameDataSelectDialog.h"
#include "Dialogs/DataFrameSortDialog.h"
#include "Dialogs/DataFrameQueryDatasDialog.h"
#include "Dialogs/DataFrameEvalDatasDialog.h"
#include "Dialogs/DataFrameCreatePivotTableDialog.h"


DataframeOperateWorker::DataframeOperateWorker(QObject* par) : DataAnalysisBaseWorker(par)
{
}

DataframeOperateWorker::~DataframeOperateWorker()
{
}

bool DataframeOperateWorker::initializePythonEnv()
{
    try {
        DA::DAPyModule DADataAnalysis("DADataAnalysis");
        m_dataOperateModule  = std::make_unique< DA::DAPyModule >();
        *m_dataOperateModule = DADataAnalysis.attr("dataframe_operate");
        return true;
    } catch (const std::exception& e) {
        m_dataOperateModule.reset();
        qCritical() << e.what();
    }
    return false;
}

void DataframeOperateWorker::createDataframeDescribe()
{
    DA::DAData optData = dataManagerInterface()->getOperateData();
    if (optData.isNull()) {
        uiInterface()->addWarningLogMessage(tr("Please first open the data table to be operated on."));  // cn:请先打开要操作的数据表
        return;
    }
    if (!optData.isDataFrame()) {
        uiInterface()->addWarningLogMessage(tr("This function only supports data in the pandas DataFrame format.")
        );  // cn:只支持dataframe格式数据
        return;
    }
    DA::DAPyDataFrame df          = optData.toDataFrame();
    DA::DAPyDataFrame df_describe = df.describe();
    DA::DAData data               = df;
    data.setName(tr("%1_Describe").arg(optData.getName()));
    data.setDescribe(tr("Generate descriptive statistics that summarize the central tendency, dispersion and "
                        "shape of a [%1]’s distribution, excluding NaN values")
                         .arg(optData.getName()));
    dataManagerInterface()->addData_(data);
    uiInterface()->setDirty(true);
    // 把数据界面抬起
    dockAreaInterface()->raiseDockingArea(DA::DADockingAreaInterface::DockingAreaDataOperate);
    auto doptWidget = dockAreaInterface()->getDataOperateWidget();
    if (doptWidget) {
        doptWidget->showData(data);
    }
}

void DataframeOperateWorker::createPivotTable()
{
    auto doptWidget = dockAreaInterface()->getDataOperateWidget();
    if (!doptWidget) {
        return;
    }
    DA::DADataOperateOfDataFrameWidget* dfopt = doptWidget->getCurrentDataFrameWidget();
    if (!dfopt) {
        return;
    }
    DA::DAData optData   = dfopt->data();
    DA::DAPyDataFrame df = dfopt->getDataframe();
    if (df.isNone()) {
        return;
    }

    if (!m_pivotTableDialog) {
        m_pivotTableDialog = new DataFrameCreatePivotTableDialog(uiInterface()->mainWindow());
    }
    m_pivotTableDialog->setDataframe(df);
    if (QDialog::Accepted != m_pivotTableDialog->exec()) {
        // 说明用户取消
        return;
    }
    // 获取创建透视表的参数
    QStringList value   = m_pivotTableDialog->getPivotTableValue();
    QStringList index   = m_pivotTableDialog->getPivotTableIndex();
    QStringList columns = m_pivotTableDialog->getPivotTableColumn();
    QString aggfunc     = m_pivotTableDialog->getPivotTableAggfunc();
    bool margins        = m_pivotTableDialog->isEnableMarginsName();
    QString marginsName = m_pivotTableDialog->getMarginsName();
    bool sort           = m_pivotTableDialog->isEnableSort();
    // 如果用户没有选定分组，则返回
    if (index.empty()) {
        return;
    }

    DA::DAPyDataFrame df_pivottable = createPivotTable(df, value, index, columns, aggfunc, margins, marginsName, sort);
    if (df_pivottable.isNone()) {
        return;
    }
    DA::DAData data = df_pivottable;
    data.setName(tr("%1_PviotTable").arg(optData.getName()));
    data.setDescribe(tr("Generate pivot table of %1").arg(optData.getName()));
    dataManagerInterface()->addData_(data);
    uiInterface()->setDirty(true);
    // 把数据界面抬起
    dockAreaInterface()->raiseDockingArea(DA::DADockingAreaInterface::DockingAreaDataOperate);
    if (doptWidget) {
        doptWidget->showData(data);
    }
}

/**
 * @brief 创建数据透视表
 *
 * @param df 输入的dataframe
 * @param value 数据透视表的行索引
 * @param index 数据透视表的列索引
 * @param columns 数据透视表的列
 * @param aggfunc 数据透视表的聚合函数
 * @param margins 是否显示分组
 * @param marginsName 分组名称
 * @param sort 是否排序
 * @return DA::DAPyDataFrame 数据透视表
 */
DA::DAPyDataFrame DataframeOperateWorker::createPivotTable(
    const DA::DAPyDataFrame& df,
    const QStringList value,
    const QStringList index,
    const QStringList columns,
    const QString& aggfunc,
    bool margins,
    const QString& marginsName,
    bool sort
)
{
    DA::DAPyScriptsDataFrame& pydf  = DA::DAPyScripts::getDataFrame();
    DA::DAPyDataFrame df_pivottable = pydf.pivotTable(df, value, index, columns, aggfunc, margins, marginsName, sort);
    return df_pivottable;
}

void DataframeOperateWorker::evalDatas()
{
    auto doptWidget = dockAreaInterface()->getDataOperateWidget();
    if (!doptWidget) {
        return;
    }
    DA::DADataOperateOfDataFrameWidget* dfopt = doptWidget->getCurrentDataFrameWidget();
    if (!dfopt) {
        return;
    }
    DA::DAPyDataFrame df = dfopt->getDataframe();
    if (df.isNone()) {
        return;
    }
    if (!m_evalDatasDialog) {
        m_evalDatasDialog = new DataFrameEvalDatasDialog(uiInterface()->mainWindow());
    }
    if (QDialog::Accepted != m_evalDatasDialog->exec()) {
        // 说明用户取消
        return;
    }
    // 获取填充值
    QString exper                  = m_evalDatasDialog->getExpr();
    DA::DADataTableView* tableView = dfopt->getDataTableView();
    DA::DADataTableModel* modle    = tableView->getDataModel();
    Callback fp                    = [ modle ]() {
        if (modle) {
            modle->refreshData();
        }
    };
    QUndoCommand* cmd = evalDatas(df, exper, fp);
    if (cmd) {
        dfopt->getUndoStack()->push(cmd);  // 推入后不会执行redo逻辑部分
        uiInterface()->setDirty(true);
    }
}

QUndoCommand* DataframeOperateWorker::evalDatas(const DA::DAPyDataFrame& df, const QString& exper, Callback fp)
{
    std::unique_ptr< CommandDataFrame_evalDatas > cmd = std::make_unique< CommandDataFrame_evalDatas >(df, exper);
    cmd->setCallBack(fp);
    if (!cmd->exec()) {
        return nullptr;
    }
    return cmd.release();
}

void DataframeOperateWorker::queryDatas()
{
    auto doptWidget = dockAreaInterface()->getDataOperateWidget();
    if (!doptWidget) {
        return;
    }
    DA::DADataOperateOfDataFrameWidget* dfopt = doptWidget->getCurrentDataFrameWidget();
    if (!dfopt) {
        return;
    }
    DA::DAPyDataFrame df = dfopt->getDataframe();
    if (df.isNone()) {
        return;
    }
    if (!m_queryDatasDialog) {
        m_queryDatasDialog = new DataFrameQueryDatasDialog(uiInterface()->mainWindow());
    }
    if (QDialog::Accepted != m_queryDatasDialog->exec()) {
        // 说明用户取消
        return;
    }
    // 获取填充值
    QString exper                  = m_queryDatasDialog->getExpr();
    DA::DADataTableView* tableView = dfopt->getDataTableView();
    DA::DADataTableModel* modle    = tableView->getDataModel();
    Callback fp                    = [ modle ]() {
        if (modle) {
            modle->refreshData();
        }
    };

    QUndoCommand* cmd = queryDatas(df, exper, fp);
    if (cmd) {
        dfopt->getUndoStack()->push(cmd);  // 推入后不会执行redo逻辑部分
        uiInterface()->setDirty(true);
    }
}

QUndoCommand* DataframeOperateWorker::queryDatas(const DA::DAPyDataFrame& df, const QString& exper, Callback fp)
{
    std::unique_ptr< CommandDataFrame_querydatas > cmd = std::make_unique< CommandDataFrame_querydatas >(df, exper);
    cmd->setCallBack(fp);
    if (!cmd->exec()) {
        return nullptr;
    }
    return cmd.release();
}

/**
 * @brief 搜索数据
 *
 * 此函数将会弹出搜索对话框，用户可以在对话框中输入搜索内容
 *
 */
void DataframeOperateWorker::searchData()
{
    auto doptWidget = dockAreaInterface()->getDataOperateWidget();
    if (!doptWidget) {
        return;
    }
    DA::DADataOperateOfDataFrameWidget* dfopt = doptWidget->getCurrentDataFrameWidget();
    if (!dfopt) {
        return;
    }
    DA::DAPyDataFrame df = dfopt->getDataframe();
    if (df.isNone()) {
        return;
    }

    if (!m_searchDialog) {
        m_searchDialog = new DataFrameDataSearchDialog(uiInterface()->mainWindow());
    }
    DA::DADataTableView* tableView = dfopt->getDataTableView();
    if (!tableView) {
        return;
    }
    m_searchDialog->setDataTableView(tableView);
    m_searchDialog->exec();
}

/**
 * @brief 过滤数据
 *
 * 此函数将会弹出过滤对话框，用户可以在对话框中选择过滤的列和范围值
 *
 */
void DataframeOperateWorker::filterByColumn()
{
    auto doptWidget = dockAreaInterface()->getDataOperateWidget();
    if (!doptWidget) {
        return;
    }
    DA::DADataOperateOfDataFrameWidget* dfopt = doptWidget->getCurrentDataFrameWidget();
    if (!dfopt) {
        return;
    }
    DA::DAPyDataFrame df = dfopt->getDataframe();
    if (df.isNone()) {
        return;
    }
    if (!m_selectDialog) {
        m_selectDialog = new DataFrameDataSelectDialog(uiInterface()->mainWindow());
    }
    m_selectDialog->setDataframe(df);
    // 获取选中的列
    if (dfopt->isDataframeTableHaveSelection()) {
        m_selectDialog->setFilterData(dfopt->getSelectedOneDataframeColumn());
    }
    if (QDialog::Accepted != m_selectDialog->exec()) {
        return;
    }
    // 获取过滤参数
    QString index                  = m_selectDialog->getFilterData();
    double lowervalue              = m_selectDialog->getLowerValue();
    double uppervalue              = m_selectDialog->getUpperValue();
    DA::DADataTableView* tableView = dfopt->getDataTableView();
    DA::DADataTableModel* modle    = tableView->getDataModel();
    Callback fp                    = [ modle ]() {
        if (modle) {
            modle->refreshData();
        }
    };

    QUndoCommand* cmd = filterByColumn(df, lowervalue, uppervalue, index, fp);
    if (cmd) {
        dfopt->getUndoStack()->push(cmd);  // 推入后不会执行redo逻辑部分
        uiInterface()->setDirty(true);
    }
}

/**
 * @brief 过滤数据
 *
 * @param df 数据
 * @param lower 低值
 * @param upper 上值
 * @param index 列
 * @param fp 回调函数
 * @return QUndoCommand* 过滤命令,返回nullptr代表执行失败
 */
QUndoCommand* DataframeOperateWorker::filterByColumn(
    const DA::DAPyDataFrame& df, double lower, double upper, const QString& index, Callback fp
)
{
    std::unique_ptr< CommandDataFrame_filterByColumn > cmd =
        std::make_unique< CommandDataFrame_filterByColumn >(df, lower, upper, index);
    cmd->setCallBack(fp);
    if (!cmd->exec()) {
        return nullptr;
    }
    return cmd.release();
}

/**
 * @brief 数据排序
 *
 * 此函数将会弹出排序对话框，用户可以在对话框中选择排序的列和排序方式
 */
void DataframeOperateWorker::sortDatas()
{
    auto doptWidget = dockAreaInterface()->getDataOperateWidget();
    if (!doptWidget) {
        return;
    }
    DA::DADataOperateOfDataFrameWidget* dfopt = doptWidget->getCurrentDataFrameWidget();
    if (!dfopt) {
        return;
    }
    DA::DAPyDataFrame df = dfopt->getDataframe();
    if (df.isNone()) {
        return;
    }
    if (!m_sortDialog) {
        m_sortDialog = new DataFrameSortDialog(uiInterface()->mainWindow());
    }
    m_sortDialog->setDataframe(df);
    // 获取选中的列
    if (dfopt->isDataframeTableHaveSelection()) {
        m_sortDialog->setSortBy(dfopt->getSelectedOneDataframeColumn());
    }
    if (QDialog::Accepted != m_sortDialog->exec()) {
        return;
    }
    // 获取排序参数
    QString by                     = m_sortDialog->getSortBy();
    bool ascending                 = m_sortDialog->getSortType();
    DA::DADataTableView* tableView = dfopt->getDataTableView();
    DA::DADataTableModel* modle    = tableView->getDataModel();
    Callback fp                    = [ modle ]() {
        if (modle) {
            modle->refreshData();
        }
    };

    QUndoCommand* cmd = sortDatas(df, by, ascending, fp);
    if (cmd) {
        dfopt->getUndoStack()->push(cmd);  // 推入后不会执行redo逻辑部分
        uiInterface()->setDirty(true);
    }
}

/**
 * @brief 数据排序
 *
 * 排序数据
 *
 * @param df 数据
 * @param by 排序列
 * @param ascending 是否升序
 * @param fp 回调函数
 * @return QUndoCommand* 排序命令,返回nullptr代表执行失败
 */
QUndoCommand* DataframeOperateWorker::sortDatas(const DA::DAPyDataFrame& df, const QString& by, const bool ascending, Callback fp)
{
    std::unique_ptr< CommandDataFrame_sort > cmd = std::make_unique< CommandDataFrame_sort >(df, by, ascending);
    cmd->setCallBack(fp);
    if (!cmd->exec()) {
        return nullptr;
    }
    return cmd.release();
}
