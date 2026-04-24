#include "DAPyLinkPoint.h"
//===================================================
// using DAPyLinkPoint
//===================================================
namespace DA
{

/**
 * @brief 构造函数，创建一个无效的连接点
 */
DAPyLinkPoint::DAPyLinkPoint() : position(QPointF()), name(QString()), way(Output), direction(AspectDirection::East)
{
}

/**
 * @brief 构造函数，创建一个完整的连接点
 *
 * 通过位置、名称、输入输出属性和延伸方向构造连接点数据
 * @param[in] p 连接点位置，相对于父图形项的坐标
 * @param[in] n 连接点名称
 * @param[in] w 连接点的输入输出属性，默认为 Output
 * @param[in] d 连接点引线的伸出方向，默认为 East
 */
DAPyLinkPoint::DAPyLinkPoint(const QPointF& p, const QString& n, Way w, AspectDirection d)
    : position(p), name(n), way(w), direction(d)
{
}

/**
 * @brief 判断连接点是否有效
 * @return 当 name 不为空时返回 true
 */
bool DAPyLinkPoint::isValid() const
{
    return !name.isEmpty();
}

/**
 * @brief 判断是否为输入节点
 * @return 如果是输入节点返回 true
 */
bool DAPyLinkPoint::isInput() const
{
    return way == Input;
}

/**
 * @brief 判断是否为输出节点
 * @return 如果是输出节点返回 true
 */
bool DAPyLinkPoint::isOutput() const
{
    return way == Output;
}

/**
 * @brief 计算 DAPyLinkPoint 的哈希值
 * @param[in] key 连接点
 * @param[in] seed 哈希种子
 * @return 哈希值
 */
uint qHash(const DAPyLinkPoint& key, uint seed)
{
    // 使用全局Qt的qHash避免与DA命名空间中qHash(AspectDirection)的重载歧义
    return ::qHash(key.name, seed) ^ ::qHash(static_cast< int >(key.position.x()), seed) ^ ::qHash(static_cast< int >(key.position.y()), seed) ^ ::qHash(static_cast< int >(key.way), seed);
}

/**
 * @brief 判断两个连接点是否相等（全字段比较）
 * @param[in] a 连接点 A
 * @param[in] b 连接点 B
 * @return 相等返回 true
 */
bool operator==(const DAPyLinkPoint& a, const DAPyLinkPoint& b)
{
    return (a.name == b.name) && (a.position == b.position) && (a.way == b.way) && (a.direction == b.direction);
}

/**
 * @brief 判断连接点是否与字符串相等（比较名称）
 * @param[in] a 连接点
 * @param[in] b 字符串
 * @return 相等返回 true
 */
bool operator==(const DAPyLinkPoint& a, const QString& b)
{
    return a.name == b;
}

/**
 * @brief 比较两个连接点的大小（用于排序）
 * @param[in] a 连接点 A
 * @param[in] b 连接点 B
 * @return a 小于 b 返回 true
 *
 * 比较规则：先比较名称，再比较位置，最后比较输入输出属性
 */
bool operator<(const DAPyLinkPoint& a, const DAPyLinkPoint& b)
{
    if (a.name != b.name) {
        return a.name < b.name;
    }
    if (a.position != b.position) {
        return (a.position.x() < b.position.x()) || ((a.position.x() == b.position.x()) && (a.position.y() < b.position.y()));
    }
    return static_cast< int >(a.way) < static_cast< int >(b.way);
}

/**
 * @brief 输出 DAPyLinkPoint 信息到 QDebug
 * @param[in] dbg QDebug 对象
 * @param[in] a 连接点
 * @return QDebug 对象
 */
QDebug operator<<(QDebug dbg, const DAPyLinkPoint& a)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "DAPyLinkPoint(" << a.name << "," << a.position << ","
                  << (a.way == DAPyLinkPoint::Input ? "Input" : "Output") << ","
                  << static_cast< int >(a.direction) << ")";
    return dbg;
}

}  // end of namespace DA
