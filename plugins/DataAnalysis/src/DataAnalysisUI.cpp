#include "DataAnalysisUI.h"

#define CHECK_VALID(action)                                                                                            \
	do {                                                                                                               \
		if (!mIsValid) {                                                                                               \
			action;                                                                                                    \
		}                                                                                                              \
	} while (0)

DataAnalysisUI::DataAnalysisUI(DA::DACoreInterface* core, DataAnalysisActions* actions, QObject* obj)
	: QObject(obj), mCore(core), mActions(actions)
{
	mRibbonArea            = mCore->getUiInterface()->getRibbonArea();
	mDockingArea           = mCore->getUiInterface()->getDockingArea();
	mRibbonCategoryDataOpt = mRibbonArea->getCategoryByObjectName("da-ribbon-category-data");
	buildDataOptCategory(mRibbonCategoryDataOpt);
}

DataAnalysisUI::~DataAnalysisUI()
{
}

void DataAnalysisUI::retranslate()
{
	CHECK_VALID(return);
	mDataTransform->setPannelName(tr("Transform"));  // cn:转换
}

/**
 * @brief 构建数据操作标签页
 * @param dataCategory
 */
void DataAnalysisUI::buildDataOptCategory(SARibbonCategory* dataCategory)
{
	CHECK_VALID(return);
	if (dataCategory == nullptr) {
		qCritical() << tr("loss \"da-ribbon-category-data\" category page");  // cn:缺失\"da-ribbon-category-data\"标签页
		mIsValid = false;
		return;
	}
	mDataTransform = dataCategory->addPannel("Transform");
}
