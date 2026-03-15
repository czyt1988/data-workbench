#include "DAArrowSymbol.h"
#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <cmath>

namespace DA
{

DAArrowSymbol::DAArrowSymbol()
    : m_arrowSize(10.0)
    , m_arrowStyle(SimpleArrow)
    , m_arrowColor(Qt::black)
    , m_arrowAngle(0.0)
    , m_arrowLineWidth(1.0)
{
    updatePath();
}

DAArrowSymbol::DAArrowSymbol(const QColor& color, qreal arrowSize, ArrowStyle style)
    : m_arrowSize(arrowSize)
    , m_arrowStyle(style)
    , m_arrowColor(color)
    , m_arrowAngle(0.0)
    , m_arrowLineWidth(1.0)
{
    updatePath();
}

DAArrowSymbol::~DAArrowSymbol()
{
}

void DAArrowSymbol::setArrowSize(qreal size)
{
    if (qFuzzyCompare(m_arrowSize, size)) {
        return;
    }
    m_arrowSize = size;
    updatePath();
}

qreal DAArrowSymbol::getArrowSize() const
{
    return m_arrowSize;
}

void DAArrowSymbol::setArrowStyle(ArrowStyle style)
{
    if (m_arrowStyle == style) {
        return;
    }
    m_arrowStyle = style;
    updatePath();
}

DAArrowSymbol::ArrowStyle DAArrowSymbol::getArrowStyle() const
{
    return m_arrowStyle;
}

void DAArrowSymbol::setArrowColor(const QColor& color)
{
    if (m_arrowColor == color) {
        return;
    }
    m_arrowColor = color;
    
    QPen pen = this->pen();
    pen.setColor(color);
    setPen(pen);
    
    setBrush(color);
}

QColor DAArrowSymbol::getArrowColor() const
{
    return m_arrowColor;
}

void DAArrowSymbol::setArrowAngle(qreal angle)
{
    if (qFuzzyCompare(m_arrowAngle, angle)) {
        return;
    }
    m_arrowAngle = angle;
    updatePath();
}

qreal DAArrowSymbol::getArrowAngle() const
{
    return m_arrowAngle;
}

void DAArrowSymbol::setArrowLineWidth(qreal width)
{
    if (qFuzzyCompare(m_arrowLineWidth, width)) {
        return;
    }
    m_arrowLineWidth = width;
    
    QPen pen = this->pen();
    pen.setWidthF(width);
    setPen(pen);
}

qreal DAArrowSymbol::getArrowLineWidth() const
{
    return m_arrowLineWidth;
}

QPainterPath DAArrowSymbol::createArrowPath(qreal arrowSize, ArrowStyle style, qreal angle)
{
    QPainterPath path;
    
    switch (style) {
    case SimpleArrow: {
        // 简单箭头：一个V形
        path.moveTo(0, 0);
        path.lineTo(-arrowSize, arrowSize * 0.5);
        path.moveTo(0, 0);
        path.lineTo(-arrowSize, -arrowSize * 0.5);
        break;
    }
    case FilledArrow: {
        // 填充箭头：三角形
        path.moveTo(0, 0);
        path.lineTo(-arrowSize, arrowSize * 0.5);
        path.lineTo(-arrowSize * 0.7, 0);
        path.lineTo(-arrowSize, -arrowSize * 0.5);
        path.closeSubpath();
        break;
    }
    case DoubleArrow: {
        // 双箭头：两个V形
        path.moveTo(0, 0);
        path.lineTo(-arrowSize, arrowSize * 0.5);
        path.moveTo(0, 0);
        path.lineTo(-arrowSize, -arrowSize * 0.5);
        
        path.moveTo(-arrowSize * 0.5, 0);
        path.lineTo(-arrowSize * 1.5, arrowSize * 0.5);
        path.moveTo(-arrowSize * 0.5, 0);
        path.lineTo(-arrowSize * 1.5, -arrowSize * 0.5);
        break;
    }
    }
    
    // 应用旋转
    if (!qFuzzyIsNull(angle)) {
        QTransform transform;
        transform.rotate(angle);
        path = transform.map(path);
    }
    
    return path;
}

void DAArrowSymbol::updatePath()
{
    QPainterPath path = createArrowPath(m_arrowSize, m_arrowStyle, m_arrowAngle);
    
    // 设置路径
    setPath(path);
    
    // 设置锚点（箭头尖端）
    setPinPoint(QPointF(0, 0));
    
    // 设置大小
    QSize size(m_arrowSize * 2, m_arrowSize * 2);
    setSize(size);
    
    // 设置画笔和画刷
    QPen pen(m_arrowColor, m_arrowLineWidth);
    pen.setJoinStyle(Qt::MiterJoin);
    setPen(pen);
    
    if (m_arrowStyle == FilledArrow) {
        setBrush(m_arrowColor);
    } else {
        setBrush(Qt::NoBrush);
    }
}

}  // End Of Namespace DA