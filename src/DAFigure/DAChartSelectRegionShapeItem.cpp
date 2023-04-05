#include "DAChartSelectRegionShapeItem.h"
#include "DAChartUtil.h"
#include <QPen>
#include <QBrush>
namespace DA
{
DAChartSelectRegionShapeItem::DAChartSelectRegionShapeItem(const QString& title) : QwtPlotShapeItem(title)
{
    setItemAttribute(QwtPlotItem::Legend, false);
    setRenderHint(QwtPlotItem::RenderAntialiased, true);
    QColor fillColor(Qt::blue);
    fillColor.setAlpha(10);
    QPen pen(Qt::black, 1);
    pen.setStyle(Qt::DashDotLine);
    pen.setJoinStyle(Qt::MiterJoin);
    setPen(pen);
    setBrush(fillColor);
}

void DAChartSelectRegionShapeItem::draw(QPainter* p, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& rect) const
{
    QwtPlotShapeItem::draw(p, xMap, yMap, rect);
}

void DAChartSelectRegionShapeItem::setEllipse(const QRectF& rect)
{
    QPainterPath path;
    path.addEllipse(rect);
    setShape(path);
}
}  // End Of Namespace DA
