#include "DAAbstractData.h"
#include <QObject>

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAbstractData
//===================================================
DAAbstractData::DAAbstractData() : _parent(nullptr)
{
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
    return _name;
}

/**
 * @brief 设置名字
 * @param n
 */
void DAAbstractData::setName(const QString& n)
{
    _name = n;
}

/**
 * @brief 获取描述
 * @return
 */
QString DAAbstractData::getDescribe() const
{
    return _describe;
}

/**
 * @brief 设置描述
 * @param d
 */
void DAAbstractData::setDescribe(const QString& d)
{
    _describe = d;
}

/**
 * @brief 获取父数据，默认为nullptr，除了一些特殊的数据如DADataPackage
 * @return
 */
DAAbstractData::Pointer DAAbstractData::getParent() const
{
    return _parent;
}

/**
 * @brief 设置父级数据
 * @param p
 */
void DAAbstractData::setParent(Pointer& p)
{
    _parent = p;
}

void DAAbstractData::write(QDataStream& out)
{
}

bool DAAbstractData::read(QDataStream& in)
{
    return false;
}
/**
 * @brief 获取id
 * @return
 */
DAAbstractData::IdType DAAbstractData::id() const
{
    return (IdType)this;
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
