#include "DAFigureElementSelection.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "qwt_scale_widget.h"
namespace DA
{
DAFigureElementSelection::DAFigureElementSelection()
{
}

DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, QwtPlot* p, SelectionColumns col)
    : figureWidget(fig), plot(p), selectionType(SelectPlot), selectionColumn(col)
{
}

DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, QwtPlot* p, QwtPlotItem* item, SelectionColumns col)
    : figureWidget(fig), plot(p), plotItem(item), selectionType(SelectPlotItem), selectionColumn(col)
{
}

DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, QwtPlot* p, QwtScaleWidget* sw, int axis, SelectionColumns col)
    : figureWidget(fig), plot(p), scaleWidget(sw), axisId(axis), selectionType(SelectScaleWidget), selectionColumn(col)
{
}

bool DAFigureElementSelection::isSelectedPlot() const
{
    return selectionType == SelectPlot;
}

bool DAFigureElementSelection::isSelectedScaleWidget() const
{
    return selectionType == SelectScaleWidget;
}

bool DAFigureElementSelection::isSelectedPlotItem() const
{
    return selectionType == SelectPlotItem;
}

}  // end namespace DA

DA_AUTO_REGISTER_META_TYPE(DA::DAFigureElementSelection)
