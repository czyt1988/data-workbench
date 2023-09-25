#ifndef DANODEMETADATA_H
#define DANODEMETADATA_H
#include "DAWorkFlowGlobal.h"
#include <QtCore/qglobal.h>
#include <QIcon>
#include <QDataStream>
#include <QDebug>
namespace DA
{
/**
 * @brief 保存节点元数据
 * 节点元数据由工厂进行管理
 * @note 节点元数据的hash和小于操作都是针对NodePrototype，因此要保证
 */
class DAWORKFLOW_API DANodeMetaData
{
public:
    DANodeMetaData();
    DANodeMetaData(const QString& prototype);
    DANodeMetaData(const QString& prototype, const QString& name, const QIcon& icon, const QString& group);
    // ptototype
    QString getNodePrototype() const;
    void setNodePrototype(const QString& prototype);

    //节点名
    QString getNodeName() const;
    void setNodeName(const QString& name);

    //图标
    QIcon getIcon() const;
    void setIcon(const QIcon& icon);

    //所属分组
    QString getGroup() const;
    void setGroup(const QString& group);

    //说明
    QString getNodeTooltip() const;
    void setNodeTooltip(const QString& tp);

    //判断是否正常
    bool isValid() const;
    //重载bool操作符
    explicit operator bool() const;

private:
    QString mPrototype;
    QString mNodeName;
    QString mNodeToolTip;
    QIcon mNodeIcon;
    QString mGroup;
};
// qHash
DAWORKFLOW_API uint qHash(const DANodeMetaData& key, uint seed = 0);
}  // end of namespace DA
Q_DECLARE_METATYPE(DA::DANodeMetaData)

DAWORKFLOW_API bool operator<(const DA::DANodeMetaData& a, const DA::DANodeMetaData& b);
DAWORKFLOW_API bool operator==(const DA::DANodeMetaData& a, const DA::DANodeMetaData& b);
DAWORKFLOW_API QDataStream& operator<<(QDataStream& out, const DA::DANodeMetaData& b);
DAWORKFLOW_API QDataStream& operator>>(QDataStream& in, DA::DANodeMetaData& b);
DAWORKFLOW_API QDebug operator<<(QDebug debug, const DA::DANodeMetaData& c);
#endif  // FCNODEMETADATA_H
