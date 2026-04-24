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
