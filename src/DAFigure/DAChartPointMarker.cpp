#include "DAChartPointMarker.h"
namespace DA
{
DAChartPointMarker::DAChartPointMarker(const QString& title) : DAAbstractChartMarker(title)
{
}

DAChartPointMarker::DAChartPointMarker(const QwtText& title) : DAAbstractChartMarker(title)
{
}

DAChartPointMarker::DAChartPointMarker(const QPointF& point)
    : DAAbstractChartMarker(QString("PointMarker:(%1,%2)").arg(point.x()).arg(point.y()))
{
    setRenderHint(QwtPlotItem::RenderAntialiased, true);
    setValue(point);
}

DAChartPointMarker::~DAChartPointMarker()
{
}
}  // End Of Namespace DA
