#include "DAAbstractChartItemSettingWidget.h"
#include "qwt_plot_item.h"
#include "qwt_plot.h"
namespace DA
{
DAAbstractChartItemSettingWidget::DAAbstractChartItemSettingWidget(QWidget* parent) : QWidget(parent)
{
}

DAAbstractChartItemSettingWidget::~DAAbstractChartItemSettingWidget()
{
}

void DAAbstractChartItemSettingWidget::setPlotItem(QwtPlotItem* item)
{
    mPlotItem = item;
    // 如果item有plot，则把plot设置进来，plot可以知道item是否被delete
    QwtPlot* oldPlot = mPlot.data();
    QwtPlot* newPlot = nullptr;
    if (item) {
        newPlot = mPlotItem->plot();
    }
    if (oldPlot == newPlot) {
        return;
    }
    if (oldPlot) {
        disconnect(oldPlot, &QwtPlot::itemAttached, this, &DAAbstractChartItemSettingWidget::plotItemAttached);
    }
    if (newPlot) {
        connect(mPlot.data(), &QwtPlot::itemAttached, this, &DAAbstractChartItemSettingWidget::plotItemAttached);
    }
}

QwtPlotItem* DAAbstractChartItemSettingWidget::getPlotItem() const
{
    return mPlotItem;
}

void DAAbstractChartItemSettingWidget::plotItemAttached(QwtPlotItem* plotItem, bool on)
{
    if (!on && plotItem == mPlotItem) {
        // item脱离plot，有可能会被delete
        mPlotItem = nullptr;
    }
}

void DAAbstractChartItemSettingWidget::clear()
{
}
}
