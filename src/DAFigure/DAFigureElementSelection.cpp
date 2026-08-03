#include "DAFigureElementSelection.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "qwt_scale_widget.h"
#include "qwt3d_plotitem.h"
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

DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, DAChart3DWidget* p3d, SelectionColumns col)
    : figureWidget(fig), plot3D(p3d), selectionType(SelectPlot3D), selectionColumn(col)
{
}

DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, DAChart3DWidget* p3d, Qwt3DPlotItem* item, SelectionColumns col)
    : figureWidget(fig), plot3D(p3d), plot3DItem(item), selectionType(SelectPlot3DItem), selectionColumn(col)
{
}

DAFigureElementSelection::DAFigureElementSelection(DAFigureWidget* fig, DAChart3DWidget* p3d, int axis3D, SelectionColumns col)
    : figureWidget(fig), plot3D(p3d), axis3DId(axis3D), selectionType(SelectPlot3DAxis), selectionColumn(col)
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

bool DAFigureElementSelection::isSelectedPlot3D() const
{
    return selectionType == SelectPlot3D;
}

bool DAFigureElementSelection::isSelectedPlot3DItem() const
{
    return selectionType == SelectPlot3DItem;
}

bool DAFigureElementSelection::isSelectedPlot3DAxis() const
{
    return selectionType == SelectPlot3DAxis;
}

}  // end namespace DA

DA_AUTO_REGISTER_META_TYPE(DA::DAFigureElementSelection)
