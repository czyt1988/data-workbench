#include "DAAppChartOperateWidget.h"
#include "DAAppFigureWidget.h"
#include "DAWaitCursorScoped.h"
#include "DAChartUtil.h"
#include "DAEvenFilterDragPlotWithGuide.h"
#include "Dialog/DADialogChartGuide.h"
#include "DAFigureWidget.h"
#include "DAChart3DWidget.h"
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
 * @param name 绘图名称
 * @param id 持久 id（工程反序列化时传入，用于恢复停靠布局）
 * @return
 */
DAFigureWidget* DAAppChartOperateWidget::createFigure(const QString& name, const QString& id)
{
    DAFigureWidget* fig = DAChartOperateWidget::createFigure(name, id);
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

void DAAppChartOperateWidget::onChartGuideAccept()
{
    DAWaitCursorScoped wait;
    Q_UNUSED(wait);
    DA::DAChartTypes ct = mChartGuideDlg->getCurrentChartType();
    if (DADialogChartGuide::is3DChartType(ct)) {
        // === 3D 路径 ===
        Qwt3DPlotItem* item = mChartGuideDlg->create3DPlotItem();
        if (nullptr == item) {
            return;
        }
        DAFigureWidget* fig = getCurrentFigure();
        if (!fig) {
            fig = createFigure();
        }
        DAChart3DWidget* chart3d = fig->getCurrent3DChart();
        if (!chart3d) {
            chart3d = fig->create3DChart();
        }
        // Qwt3DPlotItem 不是 QwtPlotItem，不能使用 DAChartUtil::setPlotItemColor。
        // 3D item 默认使用 Qwt3DStandardColor（viridis）颜色 functor，
        // 颜色配置由 06/07 计划的 3D 设置面板处理。
        fig->add3DItem_(chart3d, item);
        chart3d->update();  // trigger GL repaint
        Q_EMIT plot3DItemCreated(fig, chart3d, item);
    } else
    {
        // === 2D 路径（现有逻辑） ===
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
        // 颜色由 qwt 颜色循环在 addItem_ -> attach 时自动分配，无需显式取色
        fig->addItem_(chart, item);
        if (chart) {
            chart->rescaleAxes();
            chart->replot();
        }
        Q_EMIT plotItemCreated(fig, chart, item);
    }
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
