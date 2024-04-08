#ifndef DATAANALYSEXECUTOR_H
#define DATAANALYSEXECUTOR_H
#include "DAPyScriptsDataProcess.h"

/**
 * @brief 执行器，实现和界面无关的算法
 */
class DataAnalysExecutor
{
public:
    DataAnalysExecutor();
    //
    // 频谱分析da_spectrum_analysis
    DA::DAPyDataFrame spectrum_analysis(const DA::DAPySeries& wave, double fs, const QVariantMap& args, QString* err = nullptr);

private:
    DA::DAPyScriptsDataProcess mPyDP;
};

#endif  // DATAANALYSEXECUTOR_H
