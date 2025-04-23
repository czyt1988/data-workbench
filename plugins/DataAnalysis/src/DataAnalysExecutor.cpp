#include "DataAnalysExecutor.h"
#include "DAPyScripts.h"
DataAnalysExecutor::DataAnalysExecutor()
{
}

DA::DAPyDataFrame
DataAnalysExecutor::spectrum_analysis(const DA::DAPySeries& wave, double fs, const QVariantMap& args, QString* err)
{
	return dataProcessModule().spectrum_analysis(wave, fs, args, err);
}

DA::DAPyDataFrame
DataAnalysExecutor::butterworth_filter(const DA::DAPySeries& wave, double fs, int fo, const QVariantMap& args, QString* err)
{
	return dataProcessModule().butterworth_filter(wave, fs, fo, args, err);
}

DA::DAPyDataFrame
DataAnalysExecutor::peak_analysis(const DA::DAPySeries& wave, double fs, const QVariantMap& args, QString* err)
{
    return dataProcessModule().peak_analysis(wave, fs, args, err);
}

DA::DAPyDataFrame DataAnalysExecutor::wavelet_cwt(const DA::DAPySeries& wave,
                                                  double fs,
                                                  const DA::DAPySeries& scales,
                                                  const QVariantMap& args,
                                                  QString* err)
{
    return dataProcessModule().wavelet_cwt(wave, fs, scales, args, err);
}

DA::DAPyDataFrame DataAnalysExecutor::wavelet_dwt(const DA::DAPySeries& wave, double fs, const QVariantMap& args, QString* err)
{
    return dataProcessModule().wavelet_dwt(wave, fs, args, err);
}

DA::DAPyScriptsDataProcess& DataAnalysExecutor::dataProcessModule()
{
	return DA::DAPyScripts::getInstance().getDataProcess();
}
