#include "DataAnalysisUI.h"
#include "DataAnalysisActions.h"
#include "SARibbonBar.h"
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
	mRibbonCategoryDataAnalysis = mRibbonArea->ribbonBar()->addCategoryPage("Data Analysis");
	buildDataAnalysisCategory(mRibbonCategoryDataAnalysis);
}

DataAnalysisUI::~DataAnalysisUI()
{
}

void DataAnalysisUI::retranslate()
{
	mRibbonCategoryDataAnalysis->setCategoryName(tr("Data Analysis"));  // cn:数据处理
	mRibbonPannelSignalProcess->setPannelName(tr("Signal Process"));    // cn:信号处理
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
}

/**
 * @brief 构建数据处理Category
 * @param dataAnalysisCategory
 */
void DataAnalysisUI::buildDataAnalysisCategory(SARibbonCategory* dataAnalysisCategory)
{
	mRibbonPannelSignalProcess = dataAnalysisCategory->addPannel("Signal Process");
	mRibbonPannelSignalProcess->addLargeAction(mActions->actionSpectrum);
}
