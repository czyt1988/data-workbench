#include "DAPyScriptsDataProcess.h"
#include "DAPybind11QtTypeCast.h"
namespace DA
{

DAPyScriptsDataProcess::DAPyScriptsDataProcess() : DAPyModule()
{
	if (!import()) {
		qCritical() << QObject::tr("can not import da_data_processing module");
	}
}

DAPyScriptsDataProcess::~DAPyScriptsDataProcess()
{
}

/**
 * @brief 映射 da_data_processing.da_spectrum_analysis
 * @param wave
 * @param fs
 * @param args
 * @param err
 * @return
 */
DAPyDataFrame DAPyScriptsDataProcess::spectrum_analysis(const DAPySeries& wave, double fs, const QVariantMap& args, QString* err)
{
	try {
		pybind11::object fn = attr("da_spectrum_analysis");
		if (fn.is_none()) {
			qDebug() << "da_data_processing.py have no attr da_spectrum_analysis";
			return DAPyDataFrame();
		}
        pybind11::object v = fn(wave.object(), fs, DA::PY::toDict(args));
		return DAPyDataFrame(std::move(v));
	} catch (const std::exception& e) {
		if (err) {
			*err = e.what();
		}
		qDebug() << e.what();
	}
	return DAPyDataFrame();
}

bool DAPyScriptsDataProcess::import()
{
	return DAPyModule::import("da_data_processing");
}

}  // end DA
