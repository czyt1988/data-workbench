#include "DAFigureWidgetOverlay.h"
#include <QDebug>
namespace DA
{
DAFigureWidgetOverlay::DAFigureWidgetOverlay(DAFigureWidget* fig) : QwtWidgetOverlay(fig), _fig(fig)
{
}

DAFigureWidget* DAFigureWidgetOverlay::figure() const
{
    return _fig;
}
}
