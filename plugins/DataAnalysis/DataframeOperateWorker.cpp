#include "DataframeOperateWorker.h"
#include <QDebug>
#include "DAPyModule.h"
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
}
