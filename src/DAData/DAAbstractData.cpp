#include "DAAbstractData.h"
#include <QObject>

namespace DA
{

//===================================================
// DAAbstractData
//===================================================
DAAbstractData::DAAbstractData() : mParent(nullptr)
{
    mID = (IdType)this;
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

QString enumToString(DAAbstractData::DataType t)
{
    switch (t) {
    case DAAbstractData::TypeNone:
        return QStringLiteral("None");
    case DAAbstractData::TypeDataPackage:
        return QStringLiteral("Package");
    case DAAbstractData::TypePythonObject:
        return QStringLiteral("Object");
    case DAAbstractData::TypePythonDataFrame:
        return QStringLiteral("DataFrame");
    case DAAbstractData::TypePythonSeries:
        return QStringLiteral("Series");
    case DAAbstractData::TypeInnerData:
        return QStringLiteral("InnerData");
    default:
        break;
    }
    return QStringLiteral("None");
}

DAAbstractData::DataType stringToEnum(const QString& str, DAAbstractData::DataType defaultType)
{
    if (0 == str.compare(QStringLiteral("None"), Qt::CaseInsensitive)) {
        return DAAbstractData::TypeNone;
    } else if (0 == str.compare(QStringLiteral("Package"), Qt::CaseInsensitive)) {
        return DAAbstractData::TypeDataPackage;
    } else if (0 == str.compare(QStringLiteral("Object"), Qt::CaseInsensitive)) {
        return DAAbstractData::TypePythonObject;
    } else if (0 == str.compare(QStringLiteral("DataFrame"), Qt::CaseInsensitive)) {
        return DAAbstractData::TypePythonDataFrame;
    } else if (0 == str.compare(QStringLiteral("Series"), Qt::CaseInsensitive)) {
        return DAAbstractData::TypePythonSeries;
    } else if (0 == str.compare(QStringLiteral("InnerData"), Qt::CaseInsensitive)) {
        return DAAbstractData::TypeInnerData;
    }
    return defaultType;
}
}  // end of DA
