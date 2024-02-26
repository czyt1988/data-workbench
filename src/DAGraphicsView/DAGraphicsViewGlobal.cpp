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

/**
 * @brief DANodeLinkPoint::Direction 的枚举转换

 * @param e
 * @return
 */
QString enumToString(AspectDirection e)
{
    switch (e) {
    case AspectDirection::East:
        return "east";
    case AspectDirection::South:
        return "south";
    case AspectDirection::West:
        return "west";
    case AspectDirection::North:
        return "north";
    default:
        break;
    }
    return "east";
}
/**
 * @brief DANodeLinkPoint::Direction 的枚举转换

 * @param s
 * @return
 */
AspectDirection stringToEnum(const QString& s, AspectDirection defaultEnum)
{
    if (0 == s.compare("east", Qt::CaseInsensitive)) {
        return AspectDirection::East;
    } else if (0 == s.compare("south", Qt::CaseInsensitive)) {
        return AspectDirection::South;
    } else if (0 == s.compare("west", Qt::CaseInsensitive)) {
        return AspectDirection::West;
    } else if (0 == s.compare("north", Qt::CaseInsensitive)) {
        return AspectDirection::North;
    }
    return defaultEnum;
}

/**
 * @brief DA::ShapeKeyPoint枚举转字符串
 * @param e
 * @return
 */
QString enumToString(ShapeKeyPoint e)
{
    switch (e) {
    case ShapeKeyPoint::TopLeft:
        return "topLeft";
    case ShapeKeyPoint::TopCenter:
        return "topCenter";
    case ShapeKeyPoint::TopRight:
        return "topRight";
    case ShapeKeyPoint::CenterLeft:
        return "centerLeft";
    case ShapeKeyPoint::Center:
        return "center";
    case ShapeKeyPoint::CenterRight:
        return "centerRight";
    case ShapeKeyPoint::BottomLeft:
        return "bottomLeft";
    case ShapeKeyPoint::BottomCenter:
        return "bottomCenter";
    case ShapeKeyPoint::BottomRight:
        return "bottomRight";
    case ShapeKeyPoint::None:
        return "none";
    default:
        break;
    }
    return "none";
}

/**
 * @brief 字符串转为DA::ShapeKeyPoint枚举
 * @param s
 * @param defaultEnum
 * @return
 */
ShapeKeyPoint stringToEnum(const QString& s, ShapeKeyPoint defaultEnum)
{
    if (0 == s.compare("topLeft", Qt::CaseInsensitive)) {
        return ShapeKeyPoint::TopLeft;
    } else if (0 == s.compare("topCenter", Qt::CaseInsensitive)) {
        return ShapeKeyPoint::TopCenter;
    } else if (0 == s.compare("topRight", Qt::CaseInsensitive)) {
        return ShapeKeyPoint::TopRight;
    } else if (0 == s.compare("centerLeft", Qt::CaseInsensitive)) {
        return ShapeKeyPoint::CenterLeft;
    } else if (0 == s.compare("center", Qt::CaseInsensitive)) {
        return ShapeKeyPoint::Center;
    } else if (0 == s.compare("centerRight", Qt::CaseInsensitive)) {
        return ShapeKeyPoint::CenterRight;
    } else if (0 == s.compare("bottomLeft", Qt::CaseInsensitive)) {
        return ShapeKeyPoint::BottomLeft;
    } else if (0 == s.compare("bottomCenter", Qt::CaseInsensitive)) {
        return ShapeKeyPoint::BottomCenter;
    } else if (0 == s.compare("bottomRight", Qt::CaseInsensitive)) {
        return ShapeKeyPoint::BottomRight;
    } else if (0 == s.compare("none", Qt::CaseInsensitive)) {
        return ShapeKeyPoint::None;
    }
    return defaultEnum;
}

}
