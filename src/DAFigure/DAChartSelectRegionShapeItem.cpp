#include "DAChartSelectRegionShapeItem.h"
#include "DAChartUtil.h"
#include <QPen>
#include <QBrush>
namespace DA
{
/**
 * @brief 构造函数
 * @param title 图形项标题
 */
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

/**
 * @brief 绘制选择区域图形项
 * @param p 绘图对象
 * @param xMap X轴比例映射
 * @param yMap Y轴比例映射
 * @param rect 画布矩形区域
 */
void DAChartSelectRegionShapeItem::draw(QPainter* p, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& rect) const
{
    QwtPlotShapeItem::draw(p, xMap, yMap, rect);
}

/**
 * @brief 设置椭圆形状
 * @param rect 椭圆的包围矩形
 */
void DAChartSelectRegionShapeItem::setEllipse(const QRectF& rect)
{
    QPainterPath path;
    path.addEllipse(rect);
    setShape(path);
}
}  // End Of Namespace DA
