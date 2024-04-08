#include "DataAnalysExecutor.h"
#include "DAPyScripts.h"
DataAnalysExecutor::DataAnalysExecutor()
{
}

DA::DAPyDataFrame DataAnalysExecutor::spectrum_analysis(const DA::DAPySeries& wave,
                                                        double fs,
                                                        const QVariantMap& args,
                                                        QString* err)
{
    return dataProcessModule().spectrum_analysis(wave, fs, args, err);
}

DA::DAPyScriptsDataProcess& DataAnalysExecutor::dataProcessModule()
{
    return DA::DAPyScripts::getInstance().getDataProcess();
}
