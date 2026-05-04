#ifndef DAPYLINKPOINT_H
#define DAPYLINKPOINT_H
#include "DAPyWorkFlowAPI.h"
#include <QtCore/qglobal.h>
#include <QDebug>
#include <QPointF>
#include <QString>
#include <QList>
#include "DAGraphicsViewGlobal.h"

namespace DA
{

/**
 * @brief 用于表征连接点信息的数据
 */
class DAPYWORKFLOW_API DAPyLinkPoint
{
public:
    /**
     * @brief 连接点的输入输出属性
     */
    enum Way
    {
        Input,   ///< 输入节点
        Output   ///< 输出节点
    };

    DAPyLinkPoint();
    DAPyLinkPoint(const QPointF& p, const QString& n, Way w = Output, AspectDirection d = AspectDirection::East);

    // 判断是否是有效的连接点
    bool isValid() const;
    // 判断是否是输入节点
    bool isInput() const;
    // 判断是否是输出节点
    bool isOutput() const;

    // 按方向延伸连接点位置
    QPointF elongation(int externLen) const;
    // 判断此连接点的方向是否与给定方向相反
    bool isDirectionOpposite(AspectDirection other) const;
    // 比较 way+name 是否相同（忽略位置和方向）
    bool isEqualWayName(const DAPyLinkPoint& other) const;

    // ==== 静态方向工具方法（委托给 DAGraphicsLinkItem） ====

    // 判断两个方向是否平行
    static bool isDirectionParallel(AspectDirection d1, AspectDirection d2);
    // 顺着点1的方向看，点2是否在前面
    static bool isPointInFront(const QPointF& p1, AspectDirection d1, const QPointF& p2);
    // 两个点是否会相遇
    static bool isPointCanMeet(const QPointF& p1, AspectDirection d1, const QPointF& p2, AspectDirection d2);
    // 翻转方向
    static AspectDirection oppositeDirection(AspectDirection d);
    // 返回点1相对点2的方位
    static AspectDirection relativeDirectionOfPoint(const QPointF& p1, const QPointF& p2);

public:
    QPointF position;           ///< 连接点相对图形项的位置
    QString name;               ///< 连接点名字
    Way way;                    ///< 连接点的属性，是输入还是输出
    AspectDirection direction;  ///< 连接点引线的伸出方向
};

DAPYWORKFLOW_API uint qHash(const DAPyLinkPoint& key, uint seed);

// ADL 操作符，适用于 QMap、QHash、QSet 等容器
DAPYWORKFLOW_API bool operator==(const DAPyLinkPoint& a, const DAPyLinkPoint& b);
DAPYWORKFLOW_API bool operator==(const DAPyLinkPoint& a, const QString& b);
DAPYWORKFLOW_API bool operator<(const DAPyLinkPoint& a, const DAPyLinkPoint& b);
DAPYWORKFLOW_API QDebug operator<<(QDebug dbg, const DAPyLinkPoint& a);

// 方便类型别名
using DAPyLinkPointList = QList< DAPyLinkPoint >;

}  // end of namespace DA

Q_DECLARE_METATYPE(DA::DAPyLinkPoint)

#endif  // DAPYLINKPOINT_H
