#ifndef DATAANALYSISUI_H
#define DATAANALYSISUI_H
#include <QObject>
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DARibbonAreaInterface.h"
#include "DADockingAreaInterface.h"
#include "SARibbonCategory.h"
#include "SARibbonPannel.h"

class DataAnalysisActions;
class DataAnalysisUI : public QObject
{
public:
	DataAnalysisUI(DA::DACoreInterface* core, DataAnalysisActions* actions, QObject* obj = nullptr);
	~DataAnalysisUI();
	// 翻译更新
	void retranslate();

private:
	// 构建数据操作标签页
	void buildDataOptCategory(SARibbonCategory* dataCategory);
	//
private:
	bool mIsValid { true };  ///< 标定是否有效
	DA::DACoreInterface* mCore { nullptr };
	DataAnalysisActions* mActions { nullptr };
	DA::DARibbonAreaInterface* mRibbonArea { nullptr };
	DA::DADockingAreaInterface* mDockingArea { nullptr };
	SARibbonCategory* mRibbonCategoryDataOpt { nullptr };
	SARibbonPannel* mDataTransform { nullptr };  ///< 数据转化pannel
};

#endif  // DATAANALYSISUI_H
