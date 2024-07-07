#include "DANodeMetaData.h"

namespace DA
{

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

//===================================================
// DANodeMetaData
//===================================================

DANodeMetaData::DANodeMetaData()
{
}

DANodeMetaData::DANodeMetaData(const QString& prototype) : mPrototype(prototype)
{
}

DANodeMetaData::DANodeMetaData(const QString& prototype, const QString& name, const QIcon& icon, const QString& group)
    : mPrototype(prototype), mNodeName(name), mNodeIcon(icon), mGroup(group)
{
}

QString DANodeMetaData::getNodePrototype() const
{
	return (mPrototype);
}

void DANodeMetaData::setNodePrototype(const QString& prototype)
{
    mPrototype = prototype;
}

/**
 * @brief 节点名
 * @return
 */
QString DANodeMetaData::getNodeName() const
{
    return (mNodeName);
}

/**
 * @brief 设置节点名
 * @param name
 */
void DANodeMetaData::setNodeName(const QString& name)
{
    mNodeName = name;
}

/**
 * @brief 图标
 * @return
 */
QIcon DANodeMetaData::getIcon() const
{
    return (mNodeIcon);
}

/**
 * @brief 设置图标
 * @param icon
 */
void DANodeMetaData::setIcon(const QIcon& icon)
{
    mNodeIcon = icon;
}

/**
 * @brief 所属分组
 * @return
 */
QString DANodeMetaData::getGroup() const
{
    return (mGroup);
}

/**
 * @brief 设置所属分组
 * @param group
 */
void DANodeMetaData::setGroup(const QString& group)
{
    mGroup = group;
}

/**
 * @brief 说明
 * @return
 */
QString DANodeMetaData::getNodeTooltip() const
{
    return mNodeToolTip;
}

/**
 * @brief 设置说明
 * @param tp
 */
void DANodeMetaData::setNodeTooltip(const QString& tp)
{
    mNodeToolTip = tp;
}

/**
 * @brief 是否正常
 * @return
 */
bool DANodeMetaData::isValid() const
{
    return !(mPrototype.isEmpty());
}

DANodeMetaData::operator bool() const
{
    return isValid();
}

bool DANodeMetaData::operator<(const DANodeMetaData& other) const
{
    return (fullName(*this) < fullName(other));
}

bool DANodeMetaData::operator==(const DANodeMetaData& other) const
{
    return (fullName(*this) == fullName(other));
}

bool DANodeMetaData::operator!=(const DANodeMetaData& other) const
{
    return (fullName(*this) != fullName(other));
}

/**
   @brief 获取全名
   @param m
   @return
 */
QString DANodeMetaData::fullName(const DANodeMetaData& m)
{
    return (m.getGroup() + "/" + m.getNodePrototype());
}

// 把qHash放入DA命名空间为了ADL查找
// DANodeLinkPoint是DA命名空间，根据ADL原则，会去DA命名空间查找qHash
#if QT_VERSION_MAJOR >= 6
std::size_t qHash(const DANodeMetaData& key, std::size_t seed)
{
	return qHash(DANodeMetaData::fullName(key), seed);
}
#else
uint qHash(const DANodeMetaData& key, uint seed)
{
	return qHash(DANodeMetaData::fullName(key), seed);
}
#endif

QDataStream& operator<<(QDataStream& out, const DANodeMetaData& b)
{
	out << b.getNodePrototype() << b.getNodeName() << b.getGroup() << b.getNodeTooltip() << b.getIcon();
	return out;
}

QDataStream& operator>>(QDataStream& in, DANodeMetaData& b)
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

QDebug operator<<(QDebug debug, const DANodeMetaData& c)
{
	QDebugStateSaver saver(debug);
	debug.nospace() << "[" << c.getGroup() << "/" << c.getNodePrototype() << "]" << c.getNodeName();
	return debug;
}

}  // end of namespace DA
