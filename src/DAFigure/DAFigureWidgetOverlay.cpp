#include "DAFigureWidgetOverlay.h"
#include <QDebug>
namespace DA
{
DAFigureWidgetOverlay::DAFigureWidgetOverlay(DAFigureWidget* fig) : QwtWidgetOverlay(fig), mFigure(fig)
{
}

DAFigureWidget* DAFigureWidgetOverlay::figure() const
{
    return mFigure;
}
}
