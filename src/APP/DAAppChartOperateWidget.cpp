#include "DAAppChartOperateWidget.h"
#include "Dialog/DADialogChartGuide.h"
#include "DAAppFigureWidget.h"
#include "DAWaitCursorScoped.h"
#include "DAChartUtil.h"
#include "DAEvenFilterDragPlotWithGuide.h"
namespace DA
{

DAAppChartOperateWidget::DAAppChartOperateWidget(QWidget* parent) : DAChartOperateWidget(parent)
{
	mFigEventFilter = new DAEvenFilterDragPlotWithGuide(this);
	mFigEventFilter->setChartOptWidget(this);
}

DAAppChartOperateWidget::~DAAppChartOperateWidget()
{
}

/**
 * @brief 设置dmg
 * @param mgr
 */
void DAAppChartOperateWidget::setDataManager(DADataManager* mgr)
{
    mDataMgr = mgr;
}

/**
 * @brief 创建figure
 * @return
 */
DAFigureWidget* DAAppChartOperateWidget::createFigure()
{
	DAFigureWidget* fig = DAChartOperateWidget::createFigure();
	if (DAAppFigureWidget* appFig = qobject_cast< DAAppFigureWidget* >(fig)) {
		appFig->installEventFilter(mFigEventFilter);
	}
	return fig;
}

/**
 * @brief 生成绘图引导窗口
 * @param data
 * @return
 */
QwtPlotItem* DAAppChartOperateWidget::createPlotItemWithGuideDialog(const DAData& data, ChartTypes t)
{
	if (nullptr == mChartGuideDlg) {
		mChartGuideDlg = new DADialogChartGuide(this);
		mChartGuideDlg->setDataManager(mDataMgr);
	}
	if (data) {
		mChartGuideDlg->setCurrentData(data);
	} else {
		mChartGuideDlg->updateData();
	}
	mChartGuideDlg->setCurrentChartType(t);
	if (QDialog::Accepted != mChartGuideDlg->exec()) {
		return nullptr;
	}
	DAWaitCursorScoped wait;
	Q_UNUSED(wait);
	QwtPlotItem* item = mChartGuideDlg->createPlotItem();
	if (nullptr == item) {
		return nullptr;
	}
	return item;
}

/**
 * @brief 生成绘图通过引导窗口
 */
void DAAppChartOperateWidget::plotWithGuideDialog(DA::ChartTypes t)
{
	QwtPlotItem* item = createPlotItemWithGuideDialog(DAData(), t);
	if (!item) {
		return;
	}
	DAFigureWidget* fig = getCurrentFigure();
	if (!fig) {
		fig = createFigure();
	}
	DAChartWidget* chart = fig->getCurrentChart();
	if (!chart) {
		chart = fig->createChart();
	}
	fig->addItem_(chart, item);
}

}  // end DA
