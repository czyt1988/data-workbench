#include "DAShapeKeyPoint.h"
namespace DA
{

/**
 * @brief 构造函数
 * @param kp 关键点类型
 */
DAShapeKeyPoint::DAShapeKeyPoint(KeyPoint kp) : mKeyPoint(kp)
{
}

/**
 * @brief 析构函数
 */
DAShapeKeyPoint::~DAShapeKeyPoint()
{
}

/**
 * @brief 判断关键点是否有效
 * @return 如果关键点不为None返回true，否则返回false
 */
bool DAShapeKeyPoint::isValid() const
{
	return mKeyPoint != KeyPoint::None;
}

/**
 * @brief 赋值运算符
 * @param kp 关键点类型
 * @return 返回自身的引用
 */
DAShapeKeyPoint& DAShapeKeyPoint::operator=(KeyPoint kp)
{
	mKeyPoint = kp;
	return *this;
}

/**
 * @brief 相等比较运算符
 * @param kp 关键点类型
 * @return 如果内部关键点与kp相等返回true，否则返回false
 */
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

/**
 * @brief 根据矩形和关键点类型计算矩形上对应的关键点位置(整数坐标)
 * @param r 矩形
 * @param kp 关键点类型
 * @return 矩形上对应关键点的坐标
 */
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
		return QPoint(r.left(), r.y() + r.height() / 2);
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

/**
 * @brief 根据矩形和关键点类型计算矩形上对应的关键点位置(浮点坐标)
 * @param r 矩形
 * @param kp 关键点类型
 * @return 矩形上对应关键点的坐标
 */
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
		return QPointF(r.left(), r.y() + r.height() / 2);
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
