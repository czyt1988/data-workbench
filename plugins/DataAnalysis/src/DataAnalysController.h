﻿#ifndef DATAANALYSCONTROLLER_H
#define DATAANALYSCONTROLLER_H
#include <QObject>
#include "DAData.h"
#include "DACoreInterface.h"
#include "DADockingAreaInterface.h"
namespace DA
{
class DADataManager;
class DADataManageWidget;
}
class DataAnalysisActions;
class DataAnalysExecutor;
class DialogSpectrumSetting;
class DialogFilterSetting;
class DialogPeakAnalysisSetting;
class DialogSTFTSetting;

class DataAnalysController : public QObject
{
	Q_OBJECT
public:
	DataAnalysController(DA::DACoreInterface* core, DataAnalysisActions* actions, QObject* p = nullptr);
	~DataAnalysController();
	// 获取当前选中的数据
	DA::DAData getCurrentSelectData() const;

private:
	void initConnect();
	DialogSpectrumSetting* getSpectrumSettingDialog();
	DialogFilterSetting* getFilterSettingDialog();
	DialogPeakAnalysisSetting* getPeakAnalysisSettingDialog();
	DialogSTFTSetting* getSTFTSettingDialog();
	// 获取当前界面选择的序列，如果没有，返回none
	DA::DAPySeries getCurrentSelectSeries();
private slots:
	// 数据处理---------------------------------------------
	//  |-信号分析
	//     -频谱
	//     -滤波
	//     -寻峰
	//     -短时傅里叶变换
	void onActionSpectrumTriggered();
	void onActionFilterTriggered();
	void onActionPeakAnalysisTriggered();
	void onActionSTFTriggered();

private:
	// 转换为波形
	std::pair< std::vector< double >, std::vector< double > > toWave(const DA::DAPySeries& wave, double fs);

private:
	DA::DACoreInterface* mCore{ nullptr };
	DataAnalysisActions* mActions{ nullptr };
	DA::DADataManager* mDataMgr{ nullptr };
	DA::DADockingAreaInterface* mDockingArea{ nullptr };
	DA::DADataManageWidget* mDataManagerWidget{ nullptr };
	DialogSpectrumSetting* mDialogSpectrumSetting{ nullptr };
	DialogFilterSetting* mDialogFilterSetting{ nullptr };
	DialogPeakAnalysisSetting* mDialogPeakAnalysisSetting{ nullptr };
	DialogSTFTSetting* mDialogSTFTSetting{ nullptr };
	std::unique_ptr< DataAnalysExecutor > mExecutor;
};

#endif  // DATAANALYSCONTROLLER_H
