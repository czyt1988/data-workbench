#include "DAAbstractStatsChartAddWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
namespace DA
{
DAAbstractStatsChartAddWidget::DAAbstractStatsChartAddWidget(QWidget* parent) : DAAbstractChartAddItemWidget(parent)
{
}

DAAbstractStatsChartAddWidget::~DAAbstractStatsChartAddWidget()
{
}

QwtPlotItem* DAAbstractStatsChartAddWidget::createPlotItem()
{
    // 统计绘图不走 createPlotItem() 契约，返回 nullptr
    return nullptr;
}

void DAAbstractStatsChartAddWidget::setFigureWidget(DAFigureWidget* fig)
{
    mFigureWidget = fig;
}

DAFigureWidget* DAAbstractStatsChartAddWidget::getFigureWidget() const
{
    return mFigureWidget;
}

void DAAbstractStatsChartAddWidget::setChartWidget(DAChartWidget* chart)
{
    mChartWidget = chart;
}

DAChartWidget* DAAbstractStatsChartAddWidget::getChartWidget() const
{
    return mChartWidget;
}

}  // namespace DA
