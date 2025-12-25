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
DAFigureWidget* DAAppChartOperateWidget::createFigure(const QString& name)
{
    DAFigureWidget* fig = DAChartOperateWidget::createFigure(name);
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
    QwtPlotItem* item { nullptr };
#if DA_ENABLE_PYTHON
    if (nullptr == mChartGuideDlg) {
        initChartGuideDialog();
    }
    if (data) {
        mChartGuideDlg->setCurrentData(data);
    } else {
        mChartGuideDlg->updateData();
    }
    mChartGuideDlg->setCurrentChartType(t);
    QSignalBlocker b(mChartGuideDlg);
    // 不要发送accept信号
    if (QDialog::Accepted != mChartGuideDlg->exec()) {
        return nullptr;
    }
    DAWaitCursorScoped wait;
    Q_UNUSED(wait);
    item = mChartGuideDlg->createPlotItem();
#endif
    return item;
}

void DAAppChartOperateWidget::createPlotItemWithOpenGuideDialog(const DAData& data, ChartTypes t)
{
    if (nullptr == mChartGuideDlg) {
        initChartGuideDialog();
    }
    if (data) {
        mChartGuideDlg->setCurrentData(data);
    } else {
        mChartGuideDlg->updateData();
    }
    mChartGuideDlg->setCurrentChartType(t);
    mChartGuideDlg->open();
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
    DAChartUtil::setPlotItemColor(item, fig->getDefaultColor());
    fig->addItem_(chart, item);
}

void DAAppChartOperateWidget::onChartGuideAccept()
{
    DAWaitCursorScoped wait;
    Q_UNUSED(wait);
    QwtPlotItem* item = mChartGuideDlg->createPlotItem();
    Q_EMIT createItem(item);
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
