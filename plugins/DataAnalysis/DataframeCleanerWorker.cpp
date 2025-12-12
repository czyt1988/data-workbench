#include "DataframeCleanerWorker.h"
#include "DAPyModule.h"
#include "DAPybind11QtTypeCast.h"
DataframeCleanerWorker::DataframeCleanerWorker(QObject* par) : DataAnalysisBaseWorker(par)
{
    initializePythonEnv();
}

DataframeCleanerWorker::~DataframeCleanerWorker()
{
}

bool DataframeCleanerWorker::initializePythonEnv()
{
    try {
        DA::DAPyModule DADataAnalysis("DADataAnalysis");
        m_dataAnalysisModule  = std::make_unique< DA::DAPyModule >();
        *m_dataAnalysisModule = DADataAnalysis.attr("dataframe_io");
        return true;
    } catch (const std::exception& e) {
        m_dataAnalysisModule.reset();
        qCritical() << e.what();
    }
    return false;
}
