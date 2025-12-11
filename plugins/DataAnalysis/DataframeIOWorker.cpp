#include "DataframeIOWorker.h"
#include <QFileDialog>
#include <QFileInfo>
#include "DAData.h"
#include "DAWaitCursorScoped.h"
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataManagerInterface.h"
DataframeIOWorker::DataframeIOWorker(QObject* par) : DataAnalysisBaseWorker(par)
{
}

DataframeIOWorker::~DataframeIOWorker()
{
}

/**
 * @brief 导出单一数据
 */
void DataframeIOWorker::exportIndividualData()
{
    // 获取当前Data
    QList< DA::DAData > selDatas = dataManagerInterface()->getSelectDatas();
    if (selDatas.empty()) {
        uiInterface()->addWarningLogMessage(
            tr("No data is selected. Please select the data to export first."));  // cn:没有选中任何数据，请先选中要导出的数据
        return;
    }
    QString dataPath = QFileDialog::getSaveFileName(
        mainWindow(),
        tr("Export Data"),  // 导出数据
        QString(),
        tr("Text Files (*.txt *.csv);;Excel Files (*.xlsx);;Python Files (*.pkl);;All Files(*.*)")  // 数据文件
    );
    if (dataPath.isEmpty()) {
        // 取消退出
        return;
    }
    DA_WAIT_CURSOR_SCOPED_NS();

    QFileInfo fi(dataPath);
    QString dataName     = fi.completeBaseName();
    QString dataSuffix   = fi.suffix();
    QString baseDir      = fi.absolutePath();
    QString dataFilePath = QString("%1/%2.%3").arg(baseDir, dataName, dataSuffix);
    DA::DAData data      = selDatas.first();
    if (data.isDataFrame()) {
        if (!DA::DAData::exportToFile(data, dataFilePath)) {
            uiInterface()->addInfoLogMessage(
                tr("An exception occurred while serializing the dataframe named %1").arg(dataFilePath));  // cn:把名称为%1的dataframe序列化时出现异常
        } else {
            uiInterface()->addInfoLogMessage(
                tr("Data %1 successfully exported to %2.").arg(data.getName(), dataFilePath));  // cn:成功导出数据到%1
        }
    }
}

void DataframeIOWorker::exportMultipleData()
{
}
