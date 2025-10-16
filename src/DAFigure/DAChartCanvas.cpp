#include "DAChartCanvas.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
namespace DA
{

DAChartCanvas::DAChartCanvas(QwtPlot* p) : QwtPlotCanvas(p)
{
}

DAChartCanvas::~DAChartCanvas()
{
}

}  // End Of Namespace DA
