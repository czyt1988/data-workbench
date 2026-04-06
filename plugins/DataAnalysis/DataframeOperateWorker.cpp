#include "DataframeOperateWorker.h"
#include <QDebug>
#include "DADataManagerInterface.h"
#include "DAUIInterface.h"
#include "DAData.h"
#include "DAPyModule.h"
#include "pandas/DAPyDataFrame.h"
#include "DADockingAreaInterface.h"
#include "DADataOperateWidget.h"
#include "DADataOperateOfDataFrameWidget.h"
#include "DADataTableView.h"
#include "DAPyScripts.h"
#include "DACoreInterface.h"
#include "DADataTableModel.h"
// Commands
#include "Commands.h"
// Dialogs
#include "Dialogs/DataFrameDataSearchDialog.h"
#include "Dialogs/DataFrameDataSelectDialog.h"
#include "Dialogs/DataFrameSortDialog.h"
#include "Dialogs/DataFrameQueryDatasDialog.h"
#include "Dialogs/DataFrameEvalDatasDialog.h"


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
        uiInterface()->addWarningLogMessage(tr("This function only supports data in the pandas DataFrame format."));  // cn:只支持dataframe格式数据
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
    DA::DAPyDataFrame df = dfopt->createPivotTable();
    if (df.empty()) {
        return;
    }
    DA::DAData originData = dfopt->data();
    DA::DAData data       = df;
    data.setName(tr("%1_PviotTable").arg(originData.getName()));
    data.setDescribe(tr("Generate pivot table of %1").arg(originData.getName()));
    dataManagerInterface()->addData_(data);
    uiInterface()->setDirty(true);
    // 把数据界面抬起
    dockAreaInterface()->raiseDockingArea(DA::DADockingAreaInterface::DockingAreaDataOperate);
    if (doptWidget) {
        doptWidget->showData(data);
    }
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
    if (!m_evalDatasDialog) {
        m_evalDatasDialog = new DataFrameEvalDatasDialog(this);
    }
    if (QDialog::Accepted != m_evalDatasDialog->exec()) {
        // 说明用户取消
        return false;
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

QUndoCommand* DataframeOperateWorker::evalDatas(const DAPyDataFrame& df, const QString& exper, Callback fp)
{
    std::make_unique< CommandDataFrame_evalDatas > cmd = std::make_unique< CommandDataFrame_evalDatas >(df, exper);
    cmd->setCallback(fp);
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
        m_queryDatasDialog = new DataFrameQueryDatasDialog(this);
    }
    if (QDialog::Accepted != m_queryDatasDialog->exec()) {
        // 说明用户取消
        return false;
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

QUndoCommand* DataframeOperateWorker::queryDatas(const DAPyDataFrame& df, const QString& exper, Callback fp)
{
    std::make_unique< DACommandDataFrame_querydatas > cmd = std::make_unique< DACommandDataFrame_querydatas >(df, exper);
    cmd->setCallback(fp);
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
    DA::DAPyScripts* script = DACoreInterface::getDAScripts();
    if (!script) {
        return;
    }

    if (!m_searchDialog) {
        m_searchDialog = new DataFrameDataSearchDialog(script, this);
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
        m_selectDialog = new DataFrameDataSelectDialog(this);
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
        m_sortDialog = new DataFrameSortDialog(this);
    }
    m_sortDialog->setDataframe(df);
    // 获取选中的列
    if (dfopt->childrenRect()) {
        m_sortDialog->setSortBy(dfopt->getSelectedOneDataframeColumn());
    }
    if (QDialog::Accepted != m_sortDialog->exec()) {
        return false;
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
QUndoCommand* DataframeOperateWorker::sortDatas(const DAPyDataFrame& df, const QString& by, const bool ascending, Callback fp)
{
    std::unique_ptr< CommandDataFrame_sort > cmd = std::make_unique< CommandDataFrame_sort >(df, by, ascending);
    cmd->setCallBack(fp);
    if (!cmd->exec()) {
        return nullptr;
    }
    return cmd.release();
}
