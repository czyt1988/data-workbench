#include "DataAnalysExecutor.h"

DataAnalysExecutor::DataAnalysExecutor()
{
}

DA::DAPyDataFrame DataAnalysExecutor::spectrum_analysis(const DA::DAPySeries& wave,
                                                        double fs,
                                                        const QVariantMap& args,
                                                        QString* err)
{
    return mPyDP.spectrum_analysis(wave, fs, args, err);
}
