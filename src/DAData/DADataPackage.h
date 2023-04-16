#ifndef DADATAPACKAGE_H
#define DADATAPACKAGE_H
#include "DADataAPI.h"
#include <memory>
#include "DAAbstractData.h"
namespace DA
{
/**
 * @brief 数据包，类似一个struct
 */
class DADATA_API DADataPackage : public DAAbstractData
{
public:
    using Pointer = std::shared_ptr< DADataPackage >;

public:
    DADataPackage();
    ~DADataPackage();
    //变量类型
    virtual DataType getDataType() const;

    //变量值
    virtual QVariant toVariant() const;
    virtual bool setValue(const QVariant& v);
    //子条目数量
    int getChildCount() const;
    //插入一个数据
    void append(const DAAbstractData::Pointer& p);
    //索引
    DAAbstractData::Pointer& at(int index);
    const DAAbstractData::Pointer& at(int index) const;
    //索引
    DAAbstractData::Pointer& operator[](int index);
    const DAAbstractData::Pointer& operator[](int index) const;
    //是否为空
    bool isEmpty() const;

private:
    QList< DAAbstractData::Pointer > mChildren;
};
}  // namespace DA
#endif  // DADATAPACKAGE_H
