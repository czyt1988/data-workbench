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
        uiInterface()->addWarningLogMessage(
            tr("This function only supports data in the pandas DataFrame format."));  // cn:只支持dataframe格式数据
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
    if (dfopt->evalDatas()) {
        uiInterface()->setDirty(true);
    }
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
    if (dfopt->queryDatas()) {
        uiInterface()->setDirty(true);
    }
}

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
    if (dfopt->searchData()) {
        uiInterface()->setDirty(true);
    }
}

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
    if (dfopt->filterByColumn()) {
        uiInterface()->setDirty(true);
    }
}

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
    if (dfopt->sortDatas()) {
        uiInterface()->setDirty(true);
    }
}
