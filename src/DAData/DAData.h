#ifndef DADATA_H
#define DADATA_H
#include "DADataAPI.h"
#include "DAAbstractData.h"
#include <QtCore/qglobal.h>
#include "pandas/DAPyDataFrame.h"
#include "DADataPackage.h"
#include <memory>
namespace DA
{
class DADataManager;
/**
 * @brief DAAbstractData的封装
 * 可以放入QMap，QHash中，DAData的等于操作相当于创建一个引用
 */
class DADATA_API DAData
{
    friend class DADataManager;

public:
    using Pointer = DAAbstractData::Pointer;
    using IdType  = DAAbstractData::IdType;

public:
    DAData();
    virtual ~DAData();
    DAData(const DAData& d);
    DAData(DAData&& d);
    DAData(const DAAbstractData::Pointer& d);
    //注意这个等于只是判断指针等于，不是对值进行操作
    bool operator==(const DAData& d) const;
    bool operator<(const DAData& d) const;
    DAData& operator=(const DAData& d);
    operator bool() const;
    //是否为空
    bool isNull() const;

public:
    //以下操作符是为了实现业务的快速响应，会带来一定的耦合
    DAData(const DAPyDataFrame& d);
    DAData& operator=(const DAPyDataFrame& d);
    DAData(const DAPySeries& d);
    DAData& operator=(const DAPySeries& d);

public:  // DAAbstractData Wrapper
    DAAbstractData::DataType getDataType() const;
    //变量值操作
    QVariant value() const;
    bool setValue(const QVariant& v) const;
    //变量名操作
    QString getName() const;
    void setName(const QString& n);
    //变量描述
    QString getDescribe() const;
    void setDescribe(const QString& d);
    //返回原始指针
    DAAbstractData* rawPointer();
    const DAAbstractData* rawPointer() const;
    //返回智能指针
    Pointer getPointer();
    const Pointer getPointer() const;
    //获取id
    IdType id() const;
    //是否为dataframe
    bool isDataFrame() const;
    bool isSeries() const;
    //是否为datapackage
    bool isDataPackage() const;
    //转换为pyDataframe
    DAPyDataFrame toDataFrame() const;
    DAPySeries toSeries() const;
    //转换为datapackage
    DADataPackage::Pointer toDataPackage() const;
    //数据类型转换为文字
    QString typeToString() const;
    //获取数据对应的datamanager
    DADataManager* getDataManager() const;

protected:
    //设置变量管理器，在data添加如变量管理器后，data内部就会维护变量管理器的指针
    void setDataManager(DADataManager* mgr);

private:
    DAAbstractData::Pointer _data;
    DADataManager* _dmgr;
};
// ADL原则，需要把qHash也放入DA命名空间中
DADATA_API uint qHash(const DA::DAData& key, uint seed);

}  // namespace DA

Q_DECLARE_METATYPE(DA::DAData)
#endif  // DADATA_H
