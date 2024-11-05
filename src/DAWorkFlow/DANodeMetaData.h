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

	// 节点名
	QString getNodeName() const;
	void setNodeName(const QString& name);

	// 图标
	QIcon getIcon() const;
	void setIcon(const QIcon& icon);

	// 所属分组
	QString getGroup() const;
	void setGroup(const QString& group);

	// 说明
	QString getNodeTooltip() const;
	void setNodeTooltip(const QString& tp);

	// 判断是否正常
	bool isValid() const;
	// 重载bool操作符
	explicit operator bool() const;
	// 重载<操作符
	bool operator<(const DANodeMetaData& other) const;
	bool operator==(const DANodeMetaData& other) const;
	bool operator!=(const DANodeMetaData& other) const;

public:
	static QString fullName(const DANodeMetaData& m);

private:
	QString mPrototype;
	QString mNodeName;
	QString mNodeToolTip;
	QIcon mNodeIcon;
	QString mGroup;
};
// qHash
#if QT_VERSION_MAJOR >= 6
inline std::size_t qHash(const DANodeMetaData& key, std::size_t seed = 0)
{
	return qHash(DANodeMetaData::fullName(key), seed);
}
#else
inline uint qHash(const DANodeMetaData& key, uint seed = 0)
{
	return qHash(DANodeMetaData::fullName(key), seed);
}
#endif

DAWORKFLOW_API QDataStream& operator<<(QDataStream& out, const DANodeMetaData& b);
DAWORKFLOW_API QDataStream& operator>>(QDataStream& in, DANodeMetaData& b);
DAWORKFLOW_API QDebug operator<<(QDebug debug, const DANodeMetaData& c);

}  // end of namespace DA
Q_DECLARE_METATYPE(DA::DANodeMetaData)

#endif  // FCNODEMETADATA_H
