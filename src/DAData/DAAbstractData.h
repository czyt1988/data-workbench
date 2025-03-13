#ifndef DAABSTRACTDATA_H
#define DAABSTRACTDATA_H
#include <QVariant>
#include <memory>
#include <QMetaEnum>
#include <QDataStream>
#include "DADataAPI.h"
namespace DA
{
/**
 * @brief DA 的数据基类
 *
 * 约定：不直接使用DAAbstractData的裸指针，都应该使用智能指针
 */
class DADATA_API DAAbstractData
{
public:
    using Pointer = std::shared_ptr< DAAbstractData >;
    using IdType  = uint64_t;  ///< id类型
public:
    enum DataType
    {
        TypeNone,             ///< 空
        TypeDataPackage,      ///< 是一个数据包（类似struct）
        TypePythonObject,     ///< 说明这个是python object
        TypePythonDataFrame,  ///< 说明这个是python pandas.dataframe
        TypePythonSeries,     ///< 说明这个是python pandas.dataframe
        TypeInnerData
    };

public:
    DAAbstractData();
    virtual ~DAAbstractData();

    // 变量类型
    virtual DataType getDataType() const = 0;

    // 变量值
    virtual QVariant toVariant() const       = 0;
    virtual bool setValue(const QVariant& v) = 0;

    // 变量名
    QString getName() const;
    void setName(const QString& n);

    // 变量描述
    QString getDescribe() const;
    void setDescribe(const QString& d);

    // 返回其父节点（一般此函数之会对DADataPackage有用）
    Pointer getParent() const;
    void setParent(Pointer& p);

    //
    virtual void write(QDataStream& out);
    virtual bool read(QDataStream& in);
    // id操作
    IdType id() const;
    void setID(IdType d);

public:
    // 类型转换为文字
    static QString typeToString(DataType d);

private:
    QString mName;      ///< 名称
    QString mDescribe;  ///< 描述
    Pointer mParent;    ///< 记录父级节点
    IdType mID;         ///< id
};

QString DADATA_API enumToString(DAAbstractData::DataType t);
DAAbstractData::DataType DADATA_API stringToEnum(const QString& str,
                                                 DAAbstractData::DataType defaultType = DAAbstractData::TypeNone);

}  // namespace DA
#endif  // DAABSTRACTDATA_H
