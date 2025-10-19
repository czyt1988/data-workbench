#include "DANodeLinkPoint.h"
namespace DA
{

DANodeLinkPoint::DANodeLinkPoint() : way(Output), direction(AspectDirection::East)
{
}

DANodeLinkPoint::DANodeLinkPoint(const QPointF& p, const QString& n, DANodeLinkPoint::Way w, AspectDirection d)
    : position(p), name(n), way(w), direction(d)
{
}

bool DANodeLinkPoint::isValid() const
{
	return (!name.isNull());
}

bool DANodeLinkPoint::isInput() const
{
	return (this->way == Input);
}

bool DANodeLinkPoint::isOutput() const
{
	return (this->way == Output);
}

QPointF DANodeLinkPoint::elongation(int externLen)
{
	switch (direction) {
	case AspectDirection::East:
		return (QPointF(position.x() + externLen, position.y()));

	case AspectDirection::South:
		return (QPointF(position.x(), position.y() + externLen));

	case AspectDirection::West:
		return (QPointF(position.x() - externLen, position.y()));

	case AspectDirection::North:
		return (QPointF(position.x(), position.y() - externLen));
	}
	return (position);
}

/**
 * @brief 判断两个方向是否平行
 * @param d1
 * @param d2
 * @return
 */
bool DANodeLinkPoint::isDirectionParallel(AspectDirection d1, AspectDirection d2)
{
	if (d1 == d2) {
		return true;
	}
	return isDirectionOpposite(d1, d2);
}

/**
 * @brief 判断两个方向是否相对，也就是东对西，南对北就是相对，相对必定平行
 * @param d1
 * @param d2
 * @return
 */
bool DANodeLinkPoint::isDirectionOpposite(AspectDirection d1, AspectDirection d2)
{
	switch (d1) {
	case AspectDirection::East:
		return d2 == AspectDirection::West;
	case AspectDirection::South:
		return d2 == AspectDirection::North;
	case AspectDirection::West:
		return d2 == AspectDirection::East;
	case AspectDirection::North:
		return d2 == AspectDirection::South;
	}
	return false;
}

/**
 * @brief 在1d的方向上，点2在点1的方向的前面

 * @param p1
 * @param d1
 * @param p2
 * @return
 */
bool DANodeLinkPoint::isPointInFront(const QPointF& p1, AspectDirection d1, const QPointF& p2)
{
	switch (d1) {
	case AspectDirection::East:
		return p2.x() > p1.x();
	case AspectDirection::South:
		return p2.y() > p1.y();
	case AspectDirection::West:
		return p2.x() < p1.x();
	case AspectDirection::North:
		return p2.y() < p1.y();
	}
	return false;
}

/**
 * @brief 判断点能否相遇,针对垂直方向才有意义
 * @param p1
 * @param d1
 * @param p2
 * @param d2
 * @return
 */
bool DANodeLinkPoint::isPointCanMeet(const QPointF& p1, AspectDirection d1, const QPointF& p2, AspectDirection d2)
{
	if (isDirectionParallel(d1, d2)) {
		return false;
	}
	switch (d1) {
	case AspectDirection::East: {
		if (d2 == AspectDirection::South) {
			// 南
			return (p1.x() < p2.x()) && (p1.y() > p2.y());
		} else {
			// 北
			return (p1.x() < p2.x()) && (p1.y() < p2.y());
		}
	} break;
	case AspectDirection::South: {
		if (d2 == AspectDirection::East) {
			// 东
			return (p1.x() > p2.x()) && (p1.y() < p2.y());
		} else {
			// 西
			return (p1.x() < p2.x()) && (p1.y() < p2.y());
		}
	} break;
	case AspectDirection::West: {
		if (d2 == AspectDirection::South) {
			// 南
			return (p1.x() > p2.x()) && (p1.y() > p2.y());
		} else {
			// 北
			return (p1.x() > p2.x()) && (p1.y() < p2.y());
		}
	} break;
	case AspectDirection::North: {
		if (d2 == AspectDirection::East) {
			// 东
			return (p1.x() > p2.x()) && (p1.y() > p2.y());
		} else {
			// 西
			return (p1.x() < p2.x()) && (p1.y() > p2.y());
		}
	} break;
	}
	return false;
}

/**
 * @brief 针对平行点线，沿着方向移动可以接近，此函数只对平行点线有用
 * @param p1
 * @param d1
 * @param p2
 * @param d2
 * @return
 */
bool DANodeLinkPoint::isParallelPointApproachInDirection(const QPointF& p1,
                                                         AspectDirection d1,
                                                         const QPointF& p2,
                                                         AspectDirection d2)
{
	if (!isDirectionOpposite(d1, d2)) {
		// 平行同向永远接近不了
		return false;
	}
	// 这里说明必定反向
	switch (d1) {
	case AspectDirection::East:
		return p1.x() < p2.x();
	case AspectDirection::South:
		return p1.y() < p2.y();
	case AspectDirection::West:
		return p1.x() > p2.x();
	case AspectDirection::North:
		return p1.y() > p2.y();
	}
	return false;
}

AspectDirection DANodeLinkPoint::oppositeDirection(AspectDirection d)
{
	switch (d) {
	case AspectDirection::East:
		return AspectDirection::West;
	case AspectDirection::South:
		return AspectDirection::North;
	case AspectDirection::West:
		return AspectDirection::East;
	case AspectDirection::North:
		return AspectDirection::South;
	default:
		break;
	}
	return AspectDirection::East;
}

/**
 * @brief 计算点1相对点2的方向
 *       |
 *       |
 *   p1———p2————
 *       |
 *       |
 * 如上面示例，此函数返回West
 *
 * @param p1
 * @param p2
 * @return 如果点重合，降返回一个不可预测的方向
 */
AspectDirection DANodeLinkPoint::relativeDirectionOfPoint(const QPointF& p1, const QPointF& p2)
{
	qreal dx = p1.x() - p2.x();
	qreal dy = p1.y() - p2.y();
	if (qAbs(dx) > qAbs(dy)) {
		// x方向大于y方向
		if (dx > 0) {
			return AspectDirection::East;
		}
		return AspectDirection::West;
	} else {
		// x方向小于y方向
		if (dy > 0) {
			return AspectDirection::South;
		}
		return AspectDirection::North;
	}
	// 不可能达到
	return AspectDirection::East;
}

/**
 * @brief 模糊比较两个点
 * @param p1
 * @param p2
 * @return
 */
bool DANodeLinkPoint::fuzzyCompare(const QPointF& p1, const QPointF& p2)
{
    return qFuzzyCompare(p1.x(), p2.x()) && qFuzzyCompare(p1.y(), p2.y());
}

/**
 * @brief 不对位置方向（显示属性）进行判断，仅判断方法、属性
 * @param other
 * @return
 */
bool DANodeLinkPoint::isEqualWayName(const DANodeLinkPoint& other) const
{
    return (other.way == this->way) && (other.name == this->name);
}

/**
 * @brief DANodeLinkPoint 的等于号操作符
 * @param a
 * @param b
 * @return
 */
bool operator==(const DANodeLinkPoint& a, const DANodeLinkPoint& b)
{
	return ((a.name == b.name) && (a.direction == b.direction) && DANodeLinkPoint::fuzzyCompare(a.position, b.position)
			&& (a.way == b.way));
}

bool operator==(const DANodeLinkPoint& a, const QString& b)
{
	return (a.name == b);
}

bool operator<(const DANodeLinkPoint& a, const DANodeLinkPoint& b)
{
	return (a.name < b.name);
}

QDebug operator<<(QDebug dbg, const DANodeLinkPoint& a)
{
	dbg.nospace() << "(" << a.name << ") [" << a.position << "],way is " << a.way << ",direct is "
				  << static_cast< int >(a.direction);
	return (dbg.space());
}

// 把qHash放入DA命名空间为了ADL查找
//  DANodeLinkPoint是DA命名空间，根据ADL原则，会去DA命名空间查找qHash

uint qHash(const DANodeLinkPoint& key, uint seed)
{
	return (qHash(key.name, seed));
}

}  // end DA

DA_AUTO_REGISTER_META_TYPE(DA::DANodeLinkPoint)
