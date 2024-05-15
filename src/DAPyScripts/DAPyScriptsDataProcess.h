#ifndef DAPYSCRIPTSDATAPROCESS_H
#define DAPYSCRIPTSDATAPROCESS_H
#include "DAPyScriptsGlobal.h"
#include "DAPyModule.h"
#include <QVariant>
#include "numpy/DAPyDType.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
namespace DA
{

/**
 * @brief 封装的da_io.py
 */
class DAPYSCRIPTS_API DAPyScriptsDataProcess : public DAPyModule
{
public:
	DAPyScriptsDataProcess(bool autoImport = true);
	DAPyScriptsDataProcess(const pybind11::object& obj);
	~DAPyScriptsDataProcess();
	// 频谱分析da_spectrum_analysis
	DAPyDataFrame spectrum_analysis(const DAPySeries& wave, double fs, const QVariantMap& args, QString* err = nullptr);
	// 巴特沃斯滤波器da_butterworth_filter
	DAPyDataFrame butterworth_filter(const DAPySeries& wave, double fs, int fo, const QVariantMap& args, QString* err = nullptr);
	// 引入
	bool import();
};

}  // end DA
#endif
