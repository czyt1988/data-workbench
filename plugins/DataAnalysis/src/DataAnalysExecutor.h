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

	// 滤波da_butterworth_filter
	DA::DAPyDataFrame
	butterworth_filter(const DA::DAPySeries& wave, double fs, int fo, const QVariantMap& args, QString* err = nullptr);

	//峰值分析da_peak_analysis
	DA::DAPyDataFrame peak_analysis(const DA::DAPySeries& wave, double fs, const QVariantMap& args, QString* err = nullptr);

	//
	DA::DAPyScriptsDataProcess& dataProcessModule();
};

#endif  // DATAANALYSEXECUTOR_H
