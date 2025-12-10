#include "DataframeCleanerWorker.h"
#include <QUndoStack>
#include <QUndoGroup>
#include <QPointer>
#include "DAPyModule.h"
#include "DAPybind11QtTypeCast.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DADataOperateWidget.h"
#include "DADataUndoCommand.h"
#include "DACommandInterface.h"
#include "DADataOperateOfDataFrameWidget.h"

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
    exec("dropna");
}

void DataframeCleanerWorker::drop_duplicates()
{
    exec("drop_duplicates");
}

void DataframeCleanerWorker::fillna()
{
    exec("fillna");
}

void DataframeCleanerWorker::fill_interpolate()
{
    exec("fill_interpolate");
}

void DataframeCleanerWorker::remove_outliers_iqr()
{
    exec("remove_outliers_iqr");
}

void DataframeCleanerWorker::remove_outliers_zscore()
{
    exec("remove_outliers_zscore");
}

void DataframeCleanerWorker::transform_skewed_data()
{
    exec("transform_skewed_data");
}

bool DataframeCleanerWorker::exec(const char* funname)
{
    try {
        auto fun = m_dataCleanerModule->attr(funname);
        fun();
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return false;
    }
    return true;
}
