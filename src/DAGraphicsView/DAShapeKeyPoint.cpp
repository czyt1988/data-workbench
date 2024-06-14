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

QString enumToString(DAShapeKeyPoint e)
{
	switch (e.value()) {
	case DAShapeKeyPoint::TopLeft:
		return "topLeft";
	case DAShapeKeyPoint::TopCenter:
		return "topCenter";
	case DAShapeKeyPoint::TopRight:
		return "topRight";
	case DAShapeKeyPoint::CenterLeft:
		return "centerLeft";
	case DAShapeKeyPoint::Center:
		return "center";
	case DAShapeKeyPoint::CenterRight:
		return "centerRight";
	case DAShapeKeyPoint::BottomLeft:
		return "bottomLeft";
	case DAShapeKeyPoint::BottomCenter:
		return "bottomCenter";
	case DAShapeKeyPoint::BottomRight:
		return "bottomRight";
	case DAShapeKeyPoint::None:
		return "none";
	default:
		break;
	}
	return "none";
}

DAShapeKeyPoint stringToEnum(const QString& s, DAShapeKeyPoint defaultEnum)
{
	if (0 == s.compare("topLeft", Qt::CaseInsensitive)) {
		return DAShapeKeyPoint::TopLeft;
	} else if (0 == s.compare("topCenter", Qt::CaseInsensitive)) {
		return DAShapeKeyPoint::TopCenter;
	} else if (0 == s.compare("topRight", Qt::CaseInsensitive)) {
		return DAShapeKeyPoint::TopRight;
	} else if (0 == s.compare("centerLeft", Qt::CaseInsensitive)) {
		return DAShapeKeyPoint::CenterLeft;
	} else if (0 == s.compare("center", Qt::CaseInsensitive)) {
		return DAShapeKeyPoint::Center;
	} else if (0 == s.compare("centerRight", Qt::CaseInsensitive)) {
		return DAShapeKeyPoint::CenterRight;
	} else if (0 == s.compare("bottomLeft", Qt::CaseInsensitive)) {
		return DAShapeKeyPoint::BottomLeft;
	} else if (0 == s.compare("bottomCenter", Qt::CaseInsensitive)) {
		return DAShapeKeyPoint::BottomCenter;
	} else if (0 == s.compare("bottomRight", Qt::CaseInsensitive)) {
		return DAShapeKeyPoint::BottomRight;
	} else if (0 == s.compare("none", Qt::CaseInsensitive)) {
		return DAShapeKeyPoint::None;
	}
	return defaultEnum;
}

}  // end DA
