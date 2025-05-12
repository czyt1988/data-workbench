#include "DAShapeKeyPoint.h"
namespace DA
{

DAShapeKeyPoint::DAShapeKeyPoint(KeyPoint kp) : mKeyPoint(kp)
{
}

DAShapeKeyPoint::~DAShapeKeyPoint()
{
}

bool DAShapeKeyPoint::isValid() const
{
	return mKeyPoint != KeyPoint::None;
}

DAShapeKeyPoint& DAShapeKeyPoint::operator=(KeyPoint kp)
{
	mKeyPoint = kp;
	return *this;
}

bool DAShapeKeyPoint::operator==(KeyPoint kp) const
{
    return mKeyPoint == kp;
}

/**
 * @brief 关键点在矩形的绝对
 * @param r
 * @return
 */
QPoint DAShapeKeyPoint::rectKeyPoint(const QRect& r) const
{
    return rectKeyPoint(r, *this);
}

/**
 * @brief 关键点在矩形的绝对
 * @param r
 * @return
 */
QPointF DAShapeKeyPoint::rectKeyPoint(const QRectF& r) const
{
    return rectKeyPoint(r, *this);
}

QPoint DAShapeKeyPoint::rectKeyPoint(const QRect& r, const DAShapeKeyPoint& kp)
{
	switch (kp.value()) {
	case KeyPoint::TopLeft:
		return r.topLeft();
	case KeyPoint::TopCenter:
		return QPoint(r.x() + r.width() / 2, r.top());
	case KeyPoint::TopRight:
		return r.topRight();
	case KeyPoint::CenterLeft:
		return QPoint(r.left(), r.y() + r.width() / 2);
	case KeyPoint::Center:
		return r.center();
	case KeyPoint::CenterRight:
		return QPoint(r.right(), r.y() + r.height() / 2);
	case KeyPoint::BottomLeft:
		return r.bottomLeft();
	case KeyPoint::BottomCenter:
		return QPoint(r.x() + r.width() / 2, r.bottom());
	case KeyPoint::BottomRight:
		return r.bottomRight();
	default:
		break;
	}
	return QPoint();
}

QPointF DAShapeKeyPoint::rectKeyPoint(const QRectF& r, const DAShapeKeyPoint& kp)
{
	switch (kp.value()) {
	case KeyPoint::TopLeft:
		return r.topLeft();
	case KeyPoint::TopCenter:
		return QPointF(r.x() + r.width() / 2, r.top());
	case KeyPoint::TopRight:
		return r.topRight();
	case KeyPoint::CenterLeft:
		return QPointF(r.left(), r.y() + r.width() / 2);
	case KeyPoint::Center:
		return r.center();
	case KeyPoint::CenterRight:
		return QPointF(r.right(), r.y() + r.height() / 2);
	case KeyPoint::BottomLeft:
		return r.bottomLeft();
	case KeyPoint::BottomCenter:
		return QPointF(r.x() + r.width() / 2, r.bottom());
	case KeyPoint::BottomRight:
		return r.bottomRight();
	default:
		break;
	}
	return QPointF();
}

/**
 * @brief DAShapeKeyPoint::value
 * @return
 */
DAShapeKeyPoint::KeyPoint DAShapeKeyPoint::value() const
{
    return mKeyPoint;
}

}  // end DA
