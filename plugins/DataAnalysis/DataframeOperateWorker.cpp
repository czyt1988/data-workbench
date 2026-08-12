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


/**
 * @brief 构造函数
 *
 * @param par 父对象
 */
DataframeOperateWorker::DataframeOperateWorker(QObject* par) : DataAnalysisBaseWorker(par)
{
}

/**
 * @brief 析构函数
 */
DataframeOperateWorker::~DataframeOperateWorker()
{
}

/**
 * @brief 初始化Python环境
 *
 * @return 初始化是否成功
 */
bool DataframeOperateWorker::initializePythonEnv()
{
    try {
        DA::DAPyModule DADataAnalysisCore("DADataAnalysisCore");
        mDataOperateModule  = std::make_unique< DA::DAPyModule >();
        *mDataOperateModule = DADataAnalysisCore.attr("operations");
        return true;
    } catch (const std::exception& e) {
        mDataOperateModule.reset();
        qCritical() << e.what();
    }
    return false;
}

/**
 * @brief 创建数据描述统计信息
 */
void DataframeOperateWorker::createDataframeDescribe()
{
    DA::DAData optData = dataManagerInterface()->getOperateData();
    if (optData.isNull()) {
        uiInterface()->addWarningLogMessage(tr("Please first open the data table to operate on."));  // cn:请先打开要操作的数据表
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
    data.setName(tr("%1_Describe").arg(optData.getName()));  // cn:%1_描述
    data.setDescribe(tr("Generate descriptive statistics that summarize the central tendency, dispersion and "
                        "shape of the [%1]’s distribution, excluding NaN values")
                         .arg(optData.getName()));  // cn:生成描述性统计数据，总结[%1]分布的集中趋势、离散度和形状，排除NaN值
    dataManagerInterface()->addData_(data);
    uiInterface()->setDirty(true);
    // 把数据界面抬起
    dockAreaInterface()->raiseDockingArea(DA::DADockingAreaInterface::DockingAreaDataOperate);
    auto doptWidget = dockAreaInterface()->getDataOperateWidget();
    if (doptWidget) {
        doptWidget->showData(data);
    }
}

/**
 * @brief 创建数据透视表
 */
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

    if (!mPivotTableDialog) {
        mPivotTableDialog = new DataFrameCreatePivotTableDialog(uiInterface()->mainWindow());
    }
    mPivotTableDialog->setDataframe(df);
    if (QDialog::Accepted != mPivotTableDialog->exec()) {
        // 说明用户取消
        return;
    }
    // 获取创建透视表的参数
    QStringList value   = mPivotTableDialog->getPivotTableValue();
    QStringList index   = mPivotTableDialog->getPivotTableIndex();
    QStringList columns = mPivotTableDialog->getPivotTableColumn();
    QString aggfunc     = mPivotTableDialog->getPivotTableAggfunc();
    bool margins        = mPivotTableDialog->isEnableMarginsName();
    QString marginsName = mPivotTableDialog->getMarginsName();
    bool sort           = mPivotTableDialog->isEnableSort();
    // 如果用户没有选定分组，则返回
    if (index.empty()) {
        return;
    }

    DA::DAPyDataFrame df_pivottable = createPivotTable(df, value, index, columns, aggfunc, margins, marginsName, sort);
    if (df_pivottable.isNone()) {
        return;
    }
    DA::DAData data = df_pivottable;
    data.setName(tr("%1_PivotTable").arg(optData.getName()));  // cn:%1_数据透视表
    data.setDescribe(tr("Generate a pivot table of %1").arg(optData.getName()));  // cn:生成%1的数据透视表
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

/**
 * @brief 数值计算，弹出对话框获取表达式并执行
 */
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
    if (!mEvalDatasDialog) {
        mEvalDatasDialog = new DataFrameEvalDatasDialog(uiInterface()->mainWindow());
    }
    if (QDialog::Accepted != mEvalDatasDialog->exec()) {
        // 说明用户取消
        return;
    }
    // 获取填充值
    QString exper                  = mEvalDatasDialog->getExpr();
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

/**
 * @brief 数值计算
 *
 * @param df 数据帧
 * @param exper 表达式
 * @param fp 回调函数
 * @return 命令指针，返回nullptr代表执行失败
 */
QUndoCommand* DataframeOperateWorker::evalDatas(const DA::DAPyDataFrame& df, const QString& exper, Callback fp)
{
    std::unique_ptr< CommandDataFrame_evalDatas > cmd = std::make_unique< CommandDataFrame_evalDatas >(df, exper);
    cmd->setCallBack(fp);
    if (!cmd->exec()) {
        return nullptr;
    }
    return cmd.release();
}

/**
 * @brief 条件筛选，弹出对话框获取表达式并执行
 */
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
    if (!mQueryDatasDialog) {
        mQueryDatasDialog = new DataFrameQueryDatasDialog(uiInterface()->mainWindow());
    }
    if (QDialog::Accepted != mQueryDatasDialog->exec()) {
        // 说明用户取消
        return;
    }
    // 获取填充值
    QString exper                  = mQueryDatasDialog->getExpr();
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

/**
 * @brief 条件筛选
 *
 * @param df 数据帧
 * @param exper 表达式
 * @param fp 回调函数
 * @return 命令指针，返回nullptr代表执行失败
 */
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

    if (!mSearchDialog) {
        mSearchDialog = new DataFrameDataSearchDialog(uiInterface()->mainWindow());
    }
    DA::DADataTableView* tableView = dfopt->getDataTableView();
    if (!tableView) {
        return;
    }
    mSearchDialog->setDataTableView(tableView);
    mSearchDialog->exec();
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
    if (!mSelectDialog) {
        mSelectDialog = new DataFrameDataSelectDialog(uiInterface()->mainWindow());
    }
    mSelectDialog->setDataframe(df);
    // 获取选中的列
    if (dfopt->isDataframeTableHaveSelection()) {
        mSelectDialog->setFilterData(dfopt->getSelectedOneDataframeColumn());
    }
    if (QDialog::Accepted != mSelectDialog->exec()) {
        return;
    }
    // 获取过滤参数
    QString index                  = mSelectDialog->getFilterData();
    double lowervalue              = mSelectDialog->getLowerValue();
    double uppervalue              = mSelectDialog->getUpperValue();
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
    if (!mSortDialog) {
        mSortDialog = new DataFrameSortDialog(uiInterface()->mainWindow());
    }
    mSortDialog->setDataframe(df);
    // 获取选中的列
    if (dfopt->isDataframeTableHaveSelection()) {
        mSortDialog->setSortBy(dfopt->getSelectedOneDataframeColumn());
    }
    if (QDialog::Accepted != mSortDialog->exec()) {
        return;
    }
    // 获取排序参数
    QString by                     = mSortDialog->getSortBy();
    bool ascending                 = mSortDialog->getSortType();
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
