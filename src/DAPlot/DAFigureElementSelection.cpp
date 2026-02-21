#include "DAFigureElementSelection.h"
#include "DAFigureScrollArea.h"
#include "plot/QImPlotNode.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotItemNode.h"
namespace DA
{
DAFigureElementSelection::DAFigureElementSelection()
{
}

DAFigureElementSelection::DAFigureElementSelection(DAFigureScrollArea* fig, QIM::QImPlotNode* p, SelectionColumns col)
    : figureWidget(fig), plot(p), selectionType(SelectPlot), selectionColumn(col)
{
}

DAFigureElementSelection::DAFigureElementSelection(
    DAFigureScrollArea* fig, QIM::QImPlotNode* p, QIM::QImPlotItemNode* item, SelectionColumns col
)
    : figureWidget(fig), plot(p), plotItem(item), selectionType(SelectPlotItem), selectionColumn(col)
{
}

DAFigureElementSelection::DAFigureElementSelection(
    DAFigureScrollArea* fig, QIM::QImPlotNode* p, QIM::QImPlotAxisInfo* axis, SelectionColumns col
)
    : figureWidget(fig), plot(p), axisInfo(axis), selectionType(SelectScaleWidget), selectionColumn(col)
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
