#include "DAPyScriptsDataProcess.h"
#include "DAPybind11QtTypeCast.h"
namespace DA
{

DAPyScriptsDataProcess::DAPyScriptsDataProcess(bool autoImport) : DAPyModule()
{
	if (autoImport) {
		if (!import()) {
			qCritical() << QObject::tr("can not import da_data_processing module");
		}
	}
}

DAPyScriptsDataProcess::DAPyScriptsDataProcess(const pybind11::object& obj) : DAPyModule(obj)
{
	if (isModule()) {
		qCritical() << QObject::tr("can not import DAWorkBench.data_processing");
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
DAPyDataFrame
DAPyScriptsDataProcess::spectrum_analysis(const DAPySeries& wave, double fs, const QVariantMap& args, QString* err)
{
	try {
		pybind11::object fn = attr("da_spectrum_analysis");
		if (fn.is_none()) {
			qDebug() << "DAWorkbench.data_processing.py have no attr da_spectrum_analysis";
			return DAPyDataFrame();
		}
		pybind11::object v = fn(wave.object(), fs, DA::PY::toPyDict(args));
		return DAPyDataFrame(std::move(v));
	} catch (const std::exception& e) {
		if (err) {
			*err = e.what();
		}
		qDebug() << e.what();
	}
	return DAPyDataFrame();
}

DAPyDataFrame
DAPyScriptsDataProcess::butterworth_filter(const DAPySeries& wave, double fs, int fo, const QVariantMap& args, QString* err)
{
	try {
		pybind11::object fn = attr("da_butterworth_filter");
		if (fn.is_none()) {
			qDebug() << "DAWorkbench.data_processing.py have no attr da_butterworth_filter";
			return DAPyDataFrame();
		}
		pybind11::object v = fn(wave.object(), fs, fo, DA::PY::toPyDict(args));
		return DAPyDataFrame(std::move(v));
	} catch (const std::exception& e) {
		if (err) {
			*err = e.what();
		}
		qDebug() << e.what();
	}
	return DAPyDataFrame();
}

DAPyDataFrame DAPyScriptsDataProcess::peak_analysis(const DAPySeries& wave, double fs, const QVariantMap& args, QString* err)
{
	try {
		pybind11::object fn = attr("da_peak_analysis");
		if (fn.is_none()) {
			qDebug() << "DAWorkbench.data_processing.py have no attr da_peak_analysis";
			return DAPyDataFrame();
		}
		pybind11::object v = fn(wave.object(), fs, DA::PY::toPyDict(args));
		return DAPyDataFrame(std::move(v));
	} catch (const std::exception& e) {
		if (err) {
			*err = e.what();
		}
		qDebug() << e.what();
	}
	return DAPyDataFrame();
}

pybind11::dict DAPyScriptsDataProcess::stft_analysis(const DAPySeries& wave, double fs, const QVariantMap& args, QString* err)
{
	try {
		pybind11::object fn = attr("da_stft_analysis");
		if (fn.is_none()) {
			qDebug() << "DAWorkbench.data_processing.py have no attr da_stft_analysis";
			return pybind11::dict();
		}
		pybind11::object v = fn(wave.object(), fs, DA::PY::toPyDict(args));
		return v.cast< pybind11::dict >();
	} catch (const std::exception& e) {
		if (err) {
			*err = e.what();
		}
		qDebug() << e.what();
	}
	return pybind11::dict();
}

pybind11::dict DAPyScriptsDataProcess::wavelet_cwt(const DAPySeries& wave,
                                                   double fs,
                                                   const DA::DAPySeries& scales,
                                                   const QVariantMap& args,
                                                   QString* err)
{
	try {
		pybind11::object fn = attr("da_wavelet_cwt");
		if (fn.is_none()) {
			qDebug() << "DAWorkbench.data_processing.py have no attr da_wavelet_cwt";
            return pybind11::dict();
		}
		pybind11::object v = fn(wave.object(), fs, scales.object(), DA::PY::toPyDict(args));
        return v.cast< pybind11::dict >();
	} catch (const std::exception& e) {
		if (err) {
			*err = e.what();
		}
		qDebug() << e.what();
	}
    return pybind11::dict();
}

DAPyDataFrame DAPyScriptsDataProcess::wavelet_dwt(const DAPySeries& wave, double fs, const QVariantMap& args, QString* err)
{
	try {
		pybind11::object fn = attr("da_wavelet_dwt");
		if (fn.is_none()) {
			qDebug() << "DAWorkbench.data_processing.py have no attr da_wavelet_dwt";
			return DAPyDataFrame();
		}
		pybind11::object v = fn(wave.object(), fs, DA::PY::toPyDict(args));
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
	try {
		pybind11::module m = pybind11::module::import("DAWorkbench");
		object()           = m.attr("data_processing");
	} catch (const std::exception& e) {
		qCritical() << e.what();
		return false;
	}
	return true;
}

}  // end DA
