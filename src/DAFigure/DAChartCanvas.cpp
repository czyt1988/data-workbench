#include "DAChartCanvas.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
namespace DA
{

DAChartCanvas::DAChartCanvas(QwtPlot* p) : QwtPlotCanvas(p)
{
    //    setFrameStyle(QFrame::Box | QFrame::Plain);
    setFrameStyle(QFrame::Panel);
    setLineWidth(1);
    setBorderRadius(0);
}

DAChartCanvas::~DAChartCanvas()
{
}

}  // End Of Namespace DA
