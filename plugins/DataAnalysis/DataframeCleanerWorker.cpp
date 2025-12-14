#include "DataframeCleanerWorker.h"
#include "DAPyModule.h"
#include "DAPybind11QtTypeCast.h"
#include "DAUIInterface.h"
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
        m_dataCleanerModule  = std::make_unique< DA::DAPyModule >();
        *m_dataCleanerModule = DADataAnalysis.attr("dataframe_cleaner");
        return true;
    } catch (const std::exception& e) {
        m_dataCleanerModule.reset();
        qCritical() << e.what();
    }
    return false;
}

void DataframeCleanerWorker::dropna()
{
    try {
        auto dropna     = m_dataCleanerModule->attr("dropna");
        auto drop_count = dropna();
        if (drop_count.is_none()) {
            return;
        }
        int v = drop_count.cast< int >();
        uiInterface()->addInfoLogMessage(tr("Removed %1 rows containing NaN values").arg(v)  // cn:删除了%1个含Nan值的行
        );
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return;
    }
}
