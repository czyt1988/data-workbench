#ifndef DADATAVARIANT_H
#define DADATAVARIANT_H
#include <QVariant>
#include <QDebug>
#include "DAWorkFlowGlobal.h"

/**
 * @brief 基础数据
 *
 * 节点的传递过程就是基础数据包的传递过程，基础数据条码管理的单个数据信息
 */
class DAWORKFLOW_API DADataVariant
{
public:
    DADataVariant();
    DADataVariant(const QVariant& v, const QString& n);
    //变量值
    QVariant getValue() const;
    void setValue(const QVariant& v);
    QVariant& value();
    const QVariant& value() const;

    //变量的注释
    QString getName() const;
    void setName(const QString& n);
    QString& name();
    const QString& name() const;

    //设置值和名字
    void set(const QVariant& v, const QString& n);

private:
    QVariant m_variant;
};
Q_DECLARE_METATYPE(DADataVariant)

//等操作，对比value和comment
DAWORKFLOW_API bool operator==(const DADataVariant& a, const DADataVariant& b);
DAWORKFLOW_API bool operator<(const DADataVariant& a, const DADataVariant& b);
DAWORKFLOW_API QDebug operator<<(QDebug dbg, const DADataVariant& a);

#endif  // FCDATAITEM_H
