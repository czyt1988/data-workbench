#include "DAAbstractStatsChartAddWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
namespace DA
{
DAAbstractStatsChartAddWidget::DAAbstractStatsChartAddWidget(QWidget* parent) : DAAbstractChartAddWidgetBase(parent)
{
}

DAAbstractStatsChartAddWidget::~DAAbstractStatsChartAddWidget()
{
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
