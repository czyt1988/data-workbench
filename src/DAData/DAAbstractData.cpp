#include "DAAbstractData.h"
#include <QObject>
#include <QDateTime>
#include "DAUniqueIDGenerater.h"
namespace DA
{

//===================================================
// DAAbstractData
//===================================================
DAAbstractData::DAAbstractData() : mParent(nullptr)
{
    mID = generateID();
}

DAAbstractData::~DAAbstractData()
{
}

/**
 * @brief 获取名字
 * @return
 */
QString DAAbstractData::getName() const
{
    return mName;
}

/**
 * @brief 设置名字
 * @param n
 */
void DAAbstractData::setName(const QString& n)
{
    mName = n;
}

/**
 * @brief 获取描述
 * @return
 */
QString DAAbstractData::getDescribe() const
{
    return mDescribe;
}

/**
 * @brief 设置描述
 * @param d
 */
void DAAbstractData::setDescribe(const QString& d)
{
    mDescribe = d;
}

/**
 * @brief 获取父数据，默认为nullptr，除了一些特殊的数据如DADataPackage
 * @return
 */
DAAbstractData::Pointer DAAbstractData::getParent() const
{
    return mParent;
}

/**
 * @brief 设置父级数据
 * @param p
 */
void DAAbstractData::setParent(Pointer& p)
{
    mParent = p;
}

void DAAbstractData::write(QDataStream& out)
{
    Q_UNUSED(out);
}

bool DAAbstractData::read(QDataStream& in)
{
	Q_UNUSED(in);
	return false;
}
/**
 * @brief 获取id
 * @return
 */
DAAbstractData::IdType DAAbstractData::id() const
{
    return mID;
}

void DAAbstractData::setID(DAAbstractData::IdType d)
{
    mID = d;
}

QString DAAbstractData::typeToString(DAAbstractData::DataType d)
{
	switch (d) {
	case TypeNone:
		return QObject::tr("none");
	case TypePythonObject:
		return QObject::tr("object");
	case TypePythonDataFrame:
		return QObject::tr("dataframe");
	case TypeInnerData:
		return QObject::tr("raw");
	default:
		break;
	}
	return QString();
}

/**
 * @brief 生成一个唯一id
 * @return
 */
DAAbstractData::IdType DAAbstractData::generateID()
{
    return DAUniqueIDGenerater::id_uint64();
}

}  // end of DA
