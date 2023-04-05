#include "DANodeMetaData.h"

namespace DA
{
QString full_name(const DANodeMetaData& a);

QString full_name(const DANodeMetaData& a)
{
    return (a.getGroup() + "/" + a.getNodePrototype());
}

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

//===================================================
// DANodeMetaData
//===================================================

DANodeMetaData::DANodeMetaData()
{
}

DANodeMetaData::DANodeMetaData(const QString& prototype) : m_prototype(prototype)
{
}

DANodeMetaData::DANodeMetaData(const QString& prototype, const QString& name, const QIcon& icon, const QString& group)
    : m_prototype(prototype), m_nodeName(name), m_nodeIcon(icon), m_group(group)
{
}

QString DANodeMetaData::getNodePrototype() const
{
    return (m_prototype);
}

void DANodeMetaData::setNodePrototype(const QString& prototype)
{
    m_prototype = prototype;
}

/**
 * @brief 节点名
 * @return
 */
QString DANodeMetaData::getNodeName() const
{
    return (m_nodeName);
}

/**
 * @brief 设置节点名
 * @param name
 */
void DANodeMetaData::setNodeName(const QString& name)
{
    m_nodeName = name;
}

/**
 * @brief 图标
 * @return
 */
QIcon DANodeMetaData::getIcon() const
{
    return (m_nodeIcon);
}

/**
 * @brief 设置图标
 * @param icon
 */
void DANodeMetaData::setIcon(const QIcon& icon)
{
    m_nodeIcon = icon;
}

/**
 * @brief 所属分组
 * @return
 */
QString DANodeMetaData::getGroup() const
{
    return (m_group);
}

/**
 * @brief 设置所属分组
 * @param group
 */
void DANodeMetaData::setGroup(const QString& group)
{
    m_group = group;
}

/**
 * @brief 说明
 * @return
 */
QString DANodeMetaData::getNodeTooltip() const
{
    return m_nodeToolTip;
}

/**
 * @brief 设置说明
 * @param tp
 */
void DANodeMetaData::setNodeTooltip(const QString& tp)
{
    m_nodeToolTip = tp;
}

/**
 * @brief 是否正常
 * @return
 */
bool DANodeMetaData::isValid() const
{
    return !(m_prototype.isEmpty());
}

DANodeMetaData::operator bool() const
{
    return isValid();
}

// 把qHash放入DA命名空间为了ADL查找
// DANodeLinkPoint是DA命名空间，根据ADL原则，会去DA命名空间查找qHash
uint qHash(const DA::DANodeMetaData& key, uint seed)
{
    return (qHash(full_name(key), seed));
}

}  // end of namespace DA

//==============================================================
// DANodeMetaData Global Function
//==============================================================

bool operator<(const DA::DANodeMetaData& a, const DA::DANodeMetaData& b)
{
    return (DA::full_name(a) < DA::full_name(b));
}

bool operator==(const DA::DANodeMetaData& a, const DA::DANodeMetaData& b)
{
    return (DA::full_name(a) == DA::full_name(b));
}

QDataStream& operator<<(QDataStream& out, const DA::DANodeMetaData& b)
{
    out << b.getNodePrototype() << b.getNodeName() << b.getGroup() << b.getNodeTooltip() << b.getIcon();
    return out;
}

QDataStream& operator>>(QDataStream& in, DA::DANodeMetaData& b)
{
    QString s;
    in >> s;
    b.setNodePrototype(s);
    in >> s;
    b.setNodeName(s);
    in >> s;
    b.setGroup(s);
    in >> s;
    b.setNodeTooltip(s);
    QIcon ic;
    in >> ic;
    b.setIcon(ic);
    return in;
}
