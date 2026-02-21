#include "DAAppChartOperateWidget.h"
#include "DAAppFigureWidget.h"
#include "DAWaitCursorScoped.h"
#include "DAChartUtil.h"
#include "DAEvenFilterDragPlotWithGuide.h"
#if DA_ENABLE_PYTHON
#include "Dialog/DADialogChartGuide.h"
#endif
namespace DA
{

DAAppChartOperateWidget::DAAppChartOperateWidget(QWidget* parent) : DAPlotOperateWidget(parent)
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
DAFigureWidget* DAAppChartOperateWidget::createFigure(const QString& name)
{
    DAFigureWidget* fig = DAPlotOperateWidget::createFigure(name);
    if (DAAppFigureWidget* appFig = qobject_cast< DAAppFigureWidget* >(fig)) {
        appFig->installEventFilter(mFigEventFilter);
    }
    return fig;
}

/*
//!
//! \brief 生成绘图引导窗口
//! \param t
//! \return
//!
int DAAppChartOperateWidget::execPlotGuideDialog(DAChartTypes t)
{
    if (nullptr == mChartGuideDlg) {
        initChartGuideDialog();
    }

    mChartGuideDlg->setCurrentChartType(t);
    return mChartGuideDlg->exec();
}
*/
#if DA_ENABLE_PYTHON
void DAAppChartOperateWidget::showPlotGuideDialog(DAChartTypes t)
{
    if (nullptr == mChartGuideDlg) {
        initChartGuideDialog();
    }

    mChartGuideDlg->setCurrentChartType(t);
    mChartGuideDlg->show();
}

DADialogChartGuide* DAAppChartOperateWidget::getChartGuideDlg()
{
    if (nullptr == mChartGuideDlg) {
        initChartGuideDialog();
    }
    return mChartGuideDlg;
}
#endif

void DAAppChartOperateWidget::onChartGuideAccept()
{
    DAWaitCursorScoped wait;
    Q_UNUSED(wait);
    QwtPlotItem* item = mChartGuideDlg->createPlotItem();
    if (nullptr == item) {
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
    DAChartUtil::setPlotItemColor(item, fig->getDefaultColor());
    fig->addItem_(chart, item);
    if (chart) {
        chart->rescaleAxes();
        chart->replot();
    }
    Q_EMIT plotItemCreated(fig, chart, item);
}

void DAAppChartOperateWidget::initChartGuideDialog()
{
    if (nullptr == mChartGuideDlg) {
        mChartGuideDlg = new DADialogChartGuide(this);
        mChartGuideDlg->setDataManager(mDataMgr);
        connect(mChartGuideDlg, &DADialogChartGuide::accepted, this, &DAAppChartOperateWidget::onChartGuideAccept);
    }
}

}  // end DA
