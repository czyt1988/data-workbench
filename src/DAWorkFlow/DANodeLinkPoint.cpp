#include "DANodeLinkPoint.h"
using namespace DA;
DANodeLinkPoint::DANodeLinkPoint() : way(Output), direction(East)
{
}

DANodeLinkPoint::DANodeLinkPoint(const QPointF& p, const QString& n, DANodeLinkPoint::Way w, DANodeLinkPoint::Direction d)
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
    case East:
        return (QPointF(position.x() + externLen, position.y()));

    case South:
        return (QPointF(position.x(), position.y() + externLen));

    case West:
        return (QPointF(position.x() - externLen, position.y()));

    case North:
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
bool DANodeLinkPoint::isDirectionParallel(Direction d1, Direction d2)
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
bool DANodeLinkPoint::isDirectionOpposite(Direction d1, Direction d2)
{
    switch (d1) {
    case East:
        return d2 == West;
    case South:
        return d2 == North;
    case West:
        return d2 == East;
    case North:
        return d2 == South;
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
bool DANodeLinkPoint::isPointInFront(const QPointF& p1, Direction d1, const QPointF& p2)
{
    switch (d1) {
    case East:
        return p2.x() > p1.x();
    case South:
        return p2.y() > p1.y();
    case West:
        return p2.x() < p1.x();
    case North:
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
bool DANodeLinkPoint::isPointCanMeet(const QPointF& p1, Direction d1, const QPointF& p2, Direction d2)
{
    if (isDirectionParallel(d1, d2)) {
        return false;
    }
    switch (d1) {
    case East: {
        if (d2 == South) {
            //南
            return (p1.x() < p2.x()) && (p1.y() > p2.y());
        } else {
            //北
            return (p1.x() < p2.x()) && (p1.y() < p2.y());
        }
    } break;
    case South: {
        if (d2 == East) {
            //东
            return (p1.x() > p2.x()) && (p1.y() < p2.y());
        } else {
            //西
            return (p1.x() < p2.x()) && (p1.y() < p2.y());
        }
    } break;
    case West: {
        if (d2 == South) {
            //南
            return (p1.x() > p2.x()) && (p1.y() > p2.y());
        } else {
            //北
            return (p1.x() > p2.x()) && (p1.y() < p2.y());
        }
    } break;
    case North: {
        if (d2 == East) {
            //东
            return (p1.x() > p2.x()) && (p1.y() > p2.y());
        } else {
            //西
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
bool DANodeLinkPoint::isParallelPointApproachInDirection(const QPointF& p1, Direction d1, const QPointF& p2, Direction d2)
{
    if (!isDirectionOpposite(d1, d2)) {
        //平行同向永远接近不了
        return false;
    }
    //这里说明必定反向
    switch (d1) {
    case East:
        return p1.x() < p2.x();
    case South:
        return p1.y() < p2.y();
    case West:
        return p1.x() > p2.x();
    case North:
        return p1.y() > p2.y();
    }
    return false;
}

DANodeLinkPoint::Direction DANodeLinkPoint::oppositeDirection(Direction d)
{
    switch (d) {
    case East:
        return West;
    case South:
        return North;
    case West:
        return East;
    case North:
        return South;
    default:
        break;
    }
    return East;
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
DANodeLinkPoint::Direction DANodeLinkPoint::relativeDirectionOfPoint(const QPointF& p1, const QPointF& p2)
{
    qreal dx = p1.x() - p2.x();
    qreal dy = p1.y() - p2.y();
    if (qAbs(dx) > qAbs(dy)) {
        // x方向大于y方向
        if (dx > 0) {
            return East;
        }
        return West;
    } else {
        // x方向小于y方向
        if (dy > 0) {
            return South;
        }
        return North;
    }
    //不可能达到
    return East;
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
 * @brief FCNodeLinkPoint 的等于号操作符
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
    dbg.nospace() << "(" << a.name << ") [" << a.position << "],way is " << a.way << ",direct is " << a.direction;
    return (dbg.space());
}

/**
 * @brief DANodeLinkPoint::Way 的枚举转换
 * @param e
 * @return
 */
QString DA::enumToString(DANodeLinkPoint::Way e)
{
    switch (e) {
    case DANodeLinkPoint::Input:
        return "input";
    case DANodeLinkPoint::Output:
        return "output";
    default:
        break;
    }
    return "input";
}

/**
 * @brief DANodeLinkPoint::Way 的枚举转换
 * @param s
 * @return
 */
DANodeLinkPoint::Way DA::stringToEnum(const QString& s, DANodeLinkPoint::Way defaultEnum)
{
    if (0 == s.compare("input", Qt::CaseInsensitive)) {
        return DANodeLinkPoint::Input;
    } else if (0 == s.compare("output", Qt::CaseInsensitive)) {
        return DANodeLinkPoint::Output;
    }
    return defaultEnum;
}
/**
 * @brief DANodeLinkPoint::Direction 的枚举转换
 * @param e
 * @return
 */
QString DA::enumToString(DANodeLinkPoint::Direction e)
{
    switch (e) {
    case DANodeLinkPoint::East:
        return "east";
    case DANodeLinkPoint::South:
        return "south";
    case DANodeLinkPoint::West:
        return "west";
    case DANodeLinkPoint::North:
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
DANodeLinkPoint::Direction DA::stringToEnum(const QString& s, DANodeLinkPoint::Direction defaultEnum)
{
    if (0 == s.compare("east", Qt::CaseInsensitive)) {
        return DANodeLinkPoint::East;
    } else if (0 == s.compare("south", Qt::CaseInsensitive)) {
        return DANodeLinkPoint::South;
    } else if (0 == s.compare("west", Qt::CaseInsensitive)) {
        return DANodeLinkPoint::West;
    } else if (0 == s.compare("north", Qt::CaseInsensitive)) {
        return DANodeLinkPoint::North;
    }
    return defaultEnum;
}
//把qHash放入DA命名空间为了ADL查找
// DANodeLinkPoint是DA命名空间，根据ADL原则，会去DA命名空间查找qHash
namespace DA
{
uint qHash(const DANodeLinkPoint& key, uint seed)
{
    return (qHash(key.name, seed));
}
}  // end of namespace DA
