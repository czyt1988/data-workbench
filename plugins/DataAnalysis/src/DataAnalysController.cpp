#include "DataAnalysController.h"
#include <QMainWindow>
#include "DataAnalysisActions.h"
#include "DAUIInterface.h"
#include "DADataManagerInterface.h"
#include "DADataManageWidget.h"
#include "pandas/DAPyDataFrame.h"
#include "Dialog/DADialogDataFrameSeriesSelector.h"
DataAnalysController::DataAnalysController(DA::DACoreInterface* core, DataAnalysisActions* actions, QObject* p)
	: QObject(p), mCore(core), mActions(actions)
{
	mDockingArea       = mCore->getUiInterface()->getDockingArea();
	mDataManagerWidget = mDockingArea->getDataManageWidget();
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
 * @brief 频谱
 */
void DataAnalysController::onActionSpectrumTriggered()
{
	std::pair< DA::DAPyDataFrame, QList< int > > selDatas = mDockingArea->getCurrentSelectDataFrame();
	if (selDatas.first.isNone() || selDatas.second.empty()) {
		// 没选中数据，弹出数据选择窗口,或者选中了数据但没有列，也弹出
		DA::DADialogDataFrameSeriesSelector dlg(mCore->getUiInterface()->getMainWindow());
		dlg.setDataManager(mCore->getDataManagerInterface()->dataManager());
		if (dlg.exec() == QDialog::Accepted) { }
	}
}
