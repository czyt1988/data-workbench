#include "DAFigureElementSelection.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "qwt_scale_widget.h"
namespace DA
{
DAFigureElementSelection::DAFigureElementSelection()
{
}

DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, QwtPlot* p)
    : figureWidget(fig), plot(p), selectionType(SelectPlot)
{
}

DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, QwtPlot* p, QwtPlotItem* item)
    : figureWidget(fig), plot(p), plotItem(item), selectionType(SelectPlotItem)
{
}

DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, QwtPlot* p, QwtScaleWidget* sw, int axis)
    : figureWidget(fig), plot(p), scaleWidget(sw), axisId(axis), selectionType(SelectScaleWidget)
{
}

}  // end namespace DA

DA_AUTO_REGISTER_META_TYPE(DA::DAFigureElementSelection)
