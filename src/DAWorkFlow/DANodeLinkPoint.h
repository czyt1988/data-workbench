#ifndef DANODELINKPOINT_H
#define DANODELINKPOINT_H
#include "DAWorkFlowGlobal.h"
#include <QtCore/qglobal.h>
#include <QDebug>
namespace DA
{
/**
 * @brief 用于表征连节点信息的数据
 */
class DAWORKFLOW_API DANodeLinkPoint
{
public:
    /**
     * @brief 连接点的方向
     */
    enum Direction
    {
        East,
        South,
        West,
        North
    };

    /**
     * @brief 节点的方法
     */
    enum Way
    {
        Input,  ///< 输入节点
        Output  ///< 输出节点
    };

    DANodeLinkPoint();
    DANodeLinkPoint(const QPointF& p, const QString& n, Way w = Output, Direction d = East);
    bool isValid() const;
    bool isInput() const;
    bool isOutput() const;
    //沿着点和伸出方向延伸externLen长度后的点
    QPointF elongation(int externLen);
    //判断两个方向是否平行
    static bool isDirectionParallel(Direction d1, Direction d2);
    //判断两个方向是否相对，也就是东对西，南对北就是相对，相对必定平行
    static bool isDirectionOpposite(Direction d1, Direction d2);
    //顺着点1的方向看，点2是否在前面
    static bool isPointInFront(const QPointF& p1, Direction d1, const QPointF& p2);
    //点是否会相遇
    static bool isPointCanMeet(const QPointF& p1, Direction d1, const QPointF& p2, Direction d2);
    //针对平行点线，沿着方向移动可以接近，此函数只对平行点线有用
    static bool isParallelPointApproachInDirection(const QPointF& p1, Direction d1, const QPointF& p2, Direction d2);
    //翻转方向
    static Direction oppositeDirection(Direction d);
    //返回点1相对点2的位置
    static Direction relativeDirectionOfPoint(const QPointF& p1, const QPointF& p2);
    //模糊比较
    static bool fuzzyCompare(const QPointF& p1, const QPointF& p2);
    //不对位置方向（显示属性）进行判断，仅判断方法、属性
    bool isEqualWayName(const DANodeLinkPoint& other) const;

public:
    QPointF position;     ///< 连接点相对DAAbstractNodeGraphicsItem的位置
    QString name;         ///< 连接点名字
    Way way;              ///< 连接点的属性，是输入还是输出
    Direction direction;  ///< 连接点引线的伸出方向（用于绘制连线的时候指定方向）
};

// DANodeLinkPoint::Way 的枚举转换
DAWORKFLOW_API QString enumToString(DANodeLinkPoint::Way e);
// DANodeLinkPoint::Way 的枚举转换
DAWORKFLOW_API DANodeLinkPoint::Way stringToEnum(const QString& s, DANodeLinkPoint::Way defaultEnum = DANodeLinkPoint::Input);
// DANodeLinkPoint::Direction 的枚举转换
DAWORKFLOW_API QString enumToString(DANodeLinkPoint::Direction e);
// DANodeLinkPoint::Direction 的枚举转换
DAWORKFLOW_API DANodeLinkPoint::Direction stringToEnum(const QString& s,
                                                       DANodeLinkPoint::Direction defaultEnum = DANodeLinkPoint::East);
DAWORKFLOW_API uint qHash(const DANodeLinkPoint& key, uint seed);
}  // end of namespace DA

Q_DECLARE_METATYPE(DA::DANodeLinkPoint)
// DANodeLinkPoint 的操作符,为了可以适用map hash set
DAWORKFLOW_API bool operator==(const DA::DANodeLinkPoint& a, const DA::DANodeLinkPoint& b);
DAWORKFLOW_API bool operator==(const DA::DANodeLinkPoint& a, const QString& b);
DAWORKFLOW_API bool operator<(const DA::DANodeLinkPoint& a, const DA::DANodeLinkPoint& b);

DAWORKFLOW_API QDebug operator<<(QDebug dbg, const DA::DANodeLinkPoint& a);

#endif  // FCNODELINKPOINT_H
