#include "DAGraphicsViewGlobal.h"

namespace DA
{

QPoint rectShapeKeyPoint(const QRect& r, ShapeKeyPoint kp)
{
    switch (kp) {
    case ShapeKeyPoint::TopLeft:
        return r.topLeft();
    case ShapeKeyPoint::TopCenter:
        return QPoint(r.x() + r.width() / 2, r.top());
    case ShapeKeyPoint::TopRight:
        return r.topRight();
    case ShapeKeyPoint::CenterLeft:
        return QPoint(r.left(), r.y() + r.width() / 2);
    case ShapeKeyPoint::Center:
        return r.center();
    case ShapeKeyPoint::CenterRight:
        return QPoint(r.right(), r.y() + r.height() / 2);
    case ShapeKeyPoint::BottomLeft:
        return r.bottomLeft();
    case ShapeKeyPoint::BottomCenter:
        return QPoint(r.x() + r.width() / 2, r.bottom());
    case ShapeKeyPoint::BottomRight:
        return r.bottomRight();
    default:
        break;
    }
    return r.topLeft();
}

QPointF rectShapeKeyPoint(const QRectF& r, ShapeKeyPoint kp)
{
    switch (kp) {
    case ShapeKeyPoint::TopLeft:
        return r.topLeft();
    case ShapeKeyPoint::TopCenter:
        return QPointF(r.x() + r.width() / 2, r.top());
    case ShapeKeyPoint::TopRight:
        return r.topRight();
    case ShapeKeyPoint::CenterLeft:
        return QPointF(r.left(), r.y() + r.width() / 2);
    case ShapeKeyPoint::Center:
        return r.center();
    case ShapeKeyPoint::CenterRight:
        return QPointF(r.right(), r.y() + r.height() / 2);
    case ShapeKeyPoint::BottomLeft:
        return r.bottomLeft();
    case ShapeKeyPoint::BottomCenter:
        return QPointF(r.x() + r.width() / 2, r.bottom());
    case ShapeKeyPoint::BottomRight:
        return r.bottomRight();
    default:
        break;
    }
    return r.topLeft();
}

}
