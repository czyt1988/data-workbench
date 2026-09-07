#ifndef DAABSTRACTDATA_H
#define DAABSTRACTDATA_H
#include <QVariant>
#include <memory>
#include <QMetaEnum>
#include <QDataStream>
#include "DADataAPI.h"
namespace DA
{
class DATableDataSource;
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
    virtual QVariant toVariant(std::size_t dim1, std::size_t dim2) const         = 0;
    virtual bool setValue(std::size_t dim1, std::size_t dim2, const QVariant& v) = 0;

    // 表格数据源接口，非表格型数据返回nullptr
    virtual DATableDataSource* tableSource();
    virtual const DATableDataSource* tableSource() const;
    // 是否为引用式数据（工程持久化只保存引用，不保存数据本体）
    virtual bool isReferenceData() const;
    // 是否支持整表快照式undo（引用式/惰性数据通常不支持）
    virtual bool supportsUndoSnapshot() const;
    // 类型唯一标识字符串，默认返回DataType枚举文本，工程持久化经DADataFactory按此重建对象
    virtual QString typeIdentifier() const;

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
    // 生成一个uint64_t的唯一id
    static IdType generateID();

private:
    DA_DECLARE_PRIVATE(DAAbstractData)
};

}  // namespace DA
#endif  // DAABSTRACTDATA_H
