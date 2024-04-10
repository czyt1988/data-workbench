#include "DataAnalysController.h"
#include <QMainWindow>
#include "DataAnalysisActions.h"
#include "DAUIInterface.h"
#include "DADataManagerInterface.h"
#include "DADataManageWidget.h"
#include "pandas/DAPyDataFrame.h"
#include "Dialog/DADialogDataFrameSeriesSelector.h"
#include "Dialogs/DialogSpectrumSetting.h"
#include "DataAnalysExecutor.h"
#include "DADataManager.h"
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
	mDataMgr->addData_(d);
	// 绘图
}
