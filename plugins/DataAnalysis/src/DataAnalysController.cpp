﻿#include "DataAnalysController.h"
#include <QMainWindow>
#include "DataAnalysisActions.h"
#include "DAUIInterface.h"
#include "DADataManagerInterface.h"
#include "DADataManageWidget.h"
#include "pandas/DAPyDataFrame.h"
#include "Dialog/DADialogDataFrameSeriesSelector.h"
#include "Dialogs/DialogSpectrumSetting.h"
#include "Dialogs/DialogFilterSetting.h"
#include "Dialogs/DialogPeakAnalysisSetting.h"
#include "Dialogs/DialogSTFTSetting.h"
#include "DataAnalysExecutor.h"
#include "DADataManager.h"
#include "DAChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChartUtil.h"
#include "DAAutoincrementSeries.hpp"
DataAnalysController::DataAnalysController(DA::DACoreInterface* core, DataAnalysisActions* actions, QObject* p)
    : QObject(p), mCore(core), mActions(actions)
{
	mDataMgr           = mCore->getDataManagerInterface()->dataManager();
	mDockingArea       = mCore->getUiInterface()->getDockingArea();
	mDataManagerWidget = mDockingArea->getDataManageWidget();
	mExecutor          = std::make_unique< DataAnalysExecutor >();
	initConnect();
}

DataAnalysController::~DataAnalysController()
{
}

/**
 * @brief 获取选中的数据
 * @return
 */
DA::DAData DataAnalysController::getCurrentSelectData() const
{
    return mDataManagerWidget->getOneSelectData();
}

void DataAnalysController::initConnect()
{
	connect(mActions->actionSpectrum, &QAction::triggered, this, &DataAnalysController::onActionSpectrumTriggered);
	connect(mActions->actionFilter, &QAction::triggered, this, &DataAnalysController::onActionFilterTriggered);
	connect(mActions->actionPeakAnalysis, &QAction::triggered, this, &DataAnalysController::onActionPeakAnalysisTriggered);
	connect(mActions->actionSTFT, &QAction::triggered, this, &DataAnalysController::onActionSTFTriggered);
}

/**
 * @brief 频谱设置窗口
 * @return
 */
DialogSpectrumSetting* DataAnalysController::getSpectrumSettingDialog()
{
	if (mDialogSpectrumSetting) {
		return mDialogSpectrumSetting;
	}
	mDialogSpectrumSetting = new DialogSpectrumSetting(mCore->getUiInterface()->getMainWindow());
	mDialogSpectrumSetting->setDataManager(mCore->getDataManagerInterface()->dataManager());
	return mDialogSpectrumSetting;
}

/**
 * @brief 滤波设置窗口
 * @return
 */
DialogFilterSetting* DataAnalysController::getFilterSettingDialog()
{
	if (mDialogFilterSetting) {
		return mDialogFilterSetting;
	}
	mDialogFilterSetting = new DialogFilterSetting(mCore->getUiInterface()->getMainWindow());
	mDialogFilterSetting->setDataManager(mCore->getDataManagerInterface()->dataManager());
	return mDialogFilterSetting;
}

/**
 * @brief 寻峰设置窗口
 * @return
 */
DialogPeakAnalysisSetting* DataAnalysController::getPeakAnalysisSettingDialog()
{
	if (mDialogPeakAnalysisSetting) {
		return mDialogPeakAnalysisSetting;
	}
	mDialogPeakAnalysisSetting = new DialogPeakAnalysisSetting(mCore->getUiInterface()->getMainWindow());
	mDialogPeakAnalysisSetting->setDataManager(mCore->getDataManagerInterface()->dataManager());
	return mDialogPeakAnalysisSetting;
}

/**
 * @brief 短时傅里叶变换设置窗口
 * @return
 */
DialogSTFTSetting* DataAnalysController::getSTFTSettingDialog()
{
	if (mDialogSTFTSetting) {
		return mDialogSTFTSetting;
	}
	mDialogSTFTSetting = new DialogSTFTSetting(mCore->getUiInterface()->getMainWindow());
	mDialogSTFTSetting->setDataManager(mCore->getDataManagerInterface()->dataManager());
	return mDialogSTFTSetting;
}

DA::DAPySeries DataAnalysController::getCurrentSelectSeries()
{
	std::pair< DA::DAPyDataFrame, QList< int > > selDatas = mDockingArea->getCurrentSelectDataFrame();
	if (selDatas.second.size() != 1) {
		return DA::DAPySeries();
	}
	return selDatas.first[ selDatas.second.back() ];
}

/**
 * @brief 频谱
 */
void DataAnalysController::onActionSpectrumTriggered()
{
	DA::DAPySeries selSeries   = getCurrentSelectSeries();
	DialogSpectrumSetting* dlg = getSpectrumSettingDialog();
	if (!selSeries.isNone()) {
		dlg->setCurrentSeries(selSeries);
	}
	// 执行
	if (QDialog::Accepted != dlg->exec()) {
		return;
	}
	// 频谱分析的基本参数
	DA::DAPySeries wave = dlg->getCurrentSeries();
	double fs           = dlg->getSamplingRate();
	QVariantMap args    = dlg->getSpectrumSetting();
	if (wave.isNone()) {
		return;
	}
	qDebug() << "Spectrum args:" << args;
	// 执行
	QString err;
	DA::DAPyDataFrame df = mExecutor->spectrum_analysis(wave, fs, args, &err);
	if (df.isNone()) {
		if (!err.isEmpty()) {
			qCritical() << err;
			return;
		}
	}
	// 把数据装入datamanager
	DA::DAData d(df);
	d.setName(QString("%1-spectrum").arg(wave.name()));
	mDataMgr->addData(d);  // 不可撤销
	//! 绘图
	//! ----------
	//! | 波形图  |
	//! ----------
	//! |  频谱   |
	//! ----------
	auto plt = mDockingArea->getChartOperateWidget();
	auto fig = plt->createFigure();  // 注意，DAAppChartOperateWidget的createFigure会创建一个chart，因此，第一个chart是不需要创建的
	{  // wave chart
		// currentChart函数不会返回null
		auto waveChart = fig->currentChart();
		fig->setWidgetPosPercent(waveChart, 0.05, 0.05, 0.9, 0.45);  // 对应的是x位置占比，y位置占比，宽度占比，高度占比，y位置是从上往下
		auto xy    = toWave(wave, 1 / fs);
		auto curve = waveChart->addCurve(xy.first.data(), xy.second.data(), xy.first.size());
		curve->setTitle(tr("Wave"));  // cn:波形
		DA::DAChartUtil::setPlotItemColor(curve, fig->getDefaultColor());
		waveChart->setXLabel(tr("time(s)"));     // cn:时间(s)
		waveChart->setYLabel(tr("amplitudes"));  // cn:幅值
		waveChart->setTitle(tr("Wave Chart"));   // cn:波形图
	}
	{  // fft chart
		auto spectrumChart      = fig->createChart(0.05, 0.5, 0.9, 0.45);
		auto freq               = df[ "freq" ];
		auto amplitudes         = df[ "amplitudes" ];
		std::vector< double > x = DA::toVectorDouble(freq);
		std::vector< double > y = DA::toVectorDouble(amplitudes);
		auto spectrum           = spectrumChart->addCurve(x.data(), y.data(), x.size());
		spectrum->setTitle(tr("spectrum"));  // cn:频谱
		DA::DAChartUtil::setPlotItemColor(spectrum, fig->getDefaultColor());
		spectrumChart->setXLabel(tr("frequency(Hz)"));  // cn:频率(Hz)
		spectrumChart->setYLabel(tr("amplitudes"));     // cn:幅值
		spectrumChart->setTitle(tr("Spectrum Chart"));  // cn:频谱图
		spectrumChart->notifyChartPropertyHasChanged();
	}
	// 把绘图窗口抬起
	mDockingArea->raiseDockByWidget(mDockingArea->getChartOperateWidget());
}

/**
 * @brief 滤波
 */
void DataAnalysController::onActionFilterTriggered()
{
	DA::DAPySeries selSeries = getCurrentSelectSeries();
	DialogFilterSetting* dlg = getFilterSettingDialog();
	if (!selSeries.isNone()) {
		dlg->setCurrentSeries(selSeries);
	}
	// 执行
	if (QDialog::Accepted != dlg->exec()) {
		return;
	}
	// 滤波的基本参数
	DA::DAPySeries wave = dlg->getCurrentSeries();
	double fs           = dlg->getSamplingRate();
	int fo              = dlg->getFilterorder();
	QVariantMap args    = dlg->getFilterSetting();
	if (wave.isNone()) {
		return;
	}
	qDebug() << "Filter args:" << args;
	// 执行
	QString err;
	DA::DAPyDataFrame df = mExecutor->butterworth_filter(wave, fs, fo, args, &err);
	if (df.isNone()) {
		if (!err.isEmpty()) {
			qCritical() << err;
			return;
		}
	}
	// 把数据装入datamanager
	DA::DAData d(df);
	d.setName(QString("%1-filter").arg(wave.name()));
	mDataMgr->addData(d);  // 不可撤销
	//! 绘图
	//! ---------------------
	//! | 原始波形和滤波波形  |
	//! ---------------------
	auto plt = mDockingArea->getChartOperateWidget();
	auto fig = plt->createFigure();
	// origin wave chart
	auto waveChart = fig->currentChart();
	fig->setWidgetPosPercent(waveChart, 0.05, 0.05, 0.9, 0.9);  // 对应的是x位置占比，y位置占比，宽度占比，高度占比，y位置是从上往下
	auto xy    = toWave(wave, 1 / fs);
	auto curve = waveChart->addCurve(xy.first.data(), xy.second.data(), xy.first.size());
	curve->setTitle(tr("Origin Wave"));  // cn:原始波形
	DA::DAChartUtil::setPlotItemColor(curve, fig->getDefaultColor());

	// filtered wave chart
	auto filtered_wave      = df[ "filtered_wave" ];
	std::vector< double > y = DA::toVectorDouble(filtered_wave);
	auto filter             = waveChart->addCurve(xy.first.data(), y.data(), xy.first.size());
	filter->setTitle(tr("Filtered Wave"));  // cn:滤波波形
	DA::DAChartUtil::setPlotItemColor(filter, fig->getDefaultColor());

	waveChart->setXLabel(tr("time(s)"));                        // cn:时间(s)
	waveChart->setYLabel(tr("amplitudes"));                     // cn:幅值
	waveChart->setTitle(tr("Origin and Filtered Wave Chart"));  // cn:原始波形和滤波波形

	// 把绘图窗口抬起
	mDockingArea->raiseDockByWidget(mDockingArea->getChartOperateWidget());
}

void DataAnalysController::onActionPeakAnalysisTriggered()
{
	DA::DAPySeries selSeries       = getCurrentSelectSeries();
	DialogPeakAnalysisSetting* dlg = getPeakAnalysisSettingDialog();
	if (!selSeries.isNone()) {
		dlg->setCurrentSeries(selSeries);
	}
	// 执行
	if (QDialog::Accepted != dlg->exec()) {
		return;
	}

	// 寻峰的基本参数
	DA::DAPySeries data = dlg->getCurrentSeries();
	double fs           = dlg->getSamplingRate();
	QVariantMap args    = dlg->getPeakAnalysisSetting();
	if (data.isNone()) {
		return;
	}
	qDebug() << "Peak Analysis args:" << args;

	//执行
	QString err;
	DA::DAPyDataFrame df = mExecutor->peak_analysis(data, fs, args, &err);
	if (df.isNone()) {
		if (!err.isEmpty()) {
			qCritical() << err;
			return;
		}
	}
	// 把数据装入datamanager
	DA::DAData d(df);
	d.setName(QString("%1-peak").arg(data.name()));
	mDataMgr->addData(d);  // 不可撤销

	//! 绘图
	//! ---------------
	//! | 波形图及峰值  |
	//! ---------------
	auto plt = mDockingArea->getChartOperateWidget();
	auto fig = plt->createFigure();  // 注意，DAAppChartOperateWidget的createFigure会创建一个chart，因此，第一个chart是不需要创建的

	// wave chart
	// currentChart函数不会返回null
	auto waveChart = fig->currentChart();
	fig->setWidgetPosPercent(waveChart, 0.05, 0.05, 0.9, 0.9);  // 对应的是x位置占比，y位置占比，宽度占比，高度占比，y位置是从上往下
	auto xy    = toWave(data, 1 / fs);
	auto curve = waveChart->addCurve(xy.first.data(), xy.second.data(), xy.first.size());
	curve->setTitle(tr("Wave"));  // cn:波形
	DA::DAChartUtil::setPlotItemColor(curve, fig->getDefaultColor());
	waveChart->setXLabel(tr("time(s)"));     // cn:时间(s)
	waveChart->setYLabel(tr("amplitudes"));  // cn:幅值
	waveChart->setTitle(tr("Wave Chart"));   // cn:波形图

	//峰值
	auto peak_index         = df[ "index" ];
	auto peak_value         = df[ "value" ];
	std::vector< double > x = DA::toVectorDouble(peak_index);
	std::vector< double > y = DA::toVectorDouble(peak_value);
	auto peak               = waveChart->addScatter(x.data(), y.data(), x.size());
	peak->setTitle(tr("Peak"));  // cn:滤波波形
	DA::DAChartUtil::setPlotItemColor(peak, fig->getDefaultColor());

	waveChart->setXLabel(tr("time(s)"));                    // cn:时间(s)
	waveChart->setYLabel(tr("amplitudes"));                 // cn:幅值
	waveChart->setTitle(tr("Origin Wave and Peak Chart"));  // cn:原始波形和滤波波形

	// 把绘图窗口抬起
	mDockingArea->raiseDockByWidget(mDockingArea->getChartOperateWidget());
}

/**
 * @brief 短时傅里叶变换
 */
void DataAnalysController::onActionSTFTriggered()
{
	DA::DAPySeries selSeries = getCurrentSelectSeries();
	DialogSTFTSetting* dlg   = getSTFTSettingDialog();
	if (!selSeries.isNone()) {
		dlg->setCurrentSeries(selSeries);
	}
	// 执行
	if (QDialog::Accepted != dlg->exec()) {
		return;
	}
	// 频谱分析的基本参数
	DA::DAPySeries wave = dlg->getCurrentSeries();
	double fs           = dlg->getSamplingRate();
	QVariantMap args    = dlg->getSTFTSetting();
	if (wave.isNone()) {
		return;
	}
	qDebug() << "STFT args:" << args;
	// 执行
	QString err;
	DA::DAPyDataFrame df = mExecutor->stft_analysis(wave, fs, args, &err);
	if (df.isNone()) {
		if (!err.isEmpty()) {
			qCritical() << err;
			return;
		}
	}
	// 把数据装入datamanager
	DA::DAData d(df);
	d.setName(QString("%1-stft").arg(wave.name()));
	mDataMgr->addData(d);  // 不可撤销
						   //! 绘图
						   //! ----------
						   //! | 波形图  |
						   //! ----------
						   //! |  频谱   |
						   //! ----------
						   //	auto plt = mDockingArea->getChartOperateWidget();
						   //	auto fig = plt->createFigure();  //
	//注意，DAAppChartOperateWidget的createFigure会创建一个chart，因此，第一个chart是不需要创建的 	{  // wave chart
	//		// currentChart函数不会返回null
	//		auto waveChart = fig->currentChart();
	//		fig->setWidgetPosPercent(waveChart, 0.05, 0.05, 0.9, 0.45);  //
	//对应的是x位置占比，y位置占比，宽度占比，高度占比，y位置是从上往下 		auto xy    = toWave(wave, 1 / fs); 		auto curve =
	// waveChart->addCurve(xy.first.data(), xy.second.data(), xy.first.size()); 		curve->setTitle(tr("Wave"));  // cn:波形
	//		DA::DAChartUtil::setPlotItemColor(curve, fig->getDefaultColor());
	//		waveChart->setXLabel(tr("time(s)"));     // cn:时间(s)
	//		waveChart->setYLabel(tr("amplitudes"));  // cn:幅值
	//		waveChart->setTitle(tr("Wave Chart"));   // cn:波形图
	//}
	//	{  // fft chart
	//		auto spectrumChart      = fig->createChart(0.05, 0.5, 0.9, 0.45);
	//		auto freq               = df[ "freq" ];
	//		auto amplitudes         = df[ "amplitudes" ];
	//		std::vector< double > x = DA::toVectorDouble(freq);
	//		std::vector< double > y = DA::toVectorDouble(amplitudes);
	//		auto spectrum           = spectrumChart->addCurve(x.data(), y.data(), x.size());
	//		spectrum->setTitle(tr("stft"));  // cn:频谱
	//		DA::DAChartUtil::setPlotItemColor(spectrum, fig->getDefaultColor());
	//		spectrumChart->setXLabel(tr("frequency(Hz)"));  // cn:频率(Hz)
	//		spectrumChart->setYLabel(tr("amplitudes"));     // cn:幅值
	//		spectrumChart->setTitle(tr("STFT Chart"));      // cn:频谱图
	//		spectrumChart->notifyChartPropertyHasChanged();
	//	}
	// 把绘图窗口抬起
	mDockingArea->raiseDockByWidget(mDockingArea->getChartOperateWidget());
}

/**
 * @brief 转换为波形
 * @param wave
 * @return
 */
std::pair< std::vector< double >, std::vector< double > > DataAnalysController::toWave(const DA::DAPySeries& wave, double fs)
{
	std::pair< std::vector< double >, std::vector< double > > res;
	std::vector< double > y = DA::toVectorDouble(wave);
	DA::DAAutoincrementSeries< double > xgenrator(0.0, fs);
	std::vector< double > x;
	x.resize(y.size());
	xgenrator.generate(x.begin(), x.end());
	return std::make_pair(x, y);
}
