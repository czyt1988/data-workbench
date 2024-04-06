#ifndef DATAANALYSCONTROLLER_H
#define DATAANALYSCONTROLLER_H
#include <QObject>
#include "DAData.h"
#include "DACoreInterface.h"
#include "DADockingAreaInterface.h"
namespace DA
{
class DADataManageWidget;
}
class DataAnalysisActions;
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
private slots:
	// 数据处理---------------------------------------------
	//  |-信号分析
	//     -频谱
	void onActionSpectrumTriggered();

private:
	DA::DACoreInterface* mCore { nullptr };
	DataAnalysisActions* mActions { nullptr };
	DA::DADockingAreaInterface* mDockingArea { nullptr };
	DA::DADataManageWidget* mDataManagerWidget { nullptr };
};

#endif  // DATAANALYSCONTROLLER_H
