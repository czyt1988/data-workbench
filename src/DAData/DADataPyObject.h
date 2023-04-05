#ifndef DADATAPYOBJECT_H
#define DADATAPYOBJECT_H
#include "DADataAPI.h"
#include <memory>
#include "DAAbstractData.h"
#include "DAPyObjectWrapper.h"
#include "pandas/DAPyDataFrame.h"
namespace DA
{
/**
 * @brief python object变量
 */
class DADATA_API DADataPyObject : public DAAbstractData
{
public:
    DADataPyObject();
    DADataPyObject(const DAPyObjectWrapper& d);
    ~DADataPyObject();
    //变量类型
    DataType getDataType() const override;
    //变量值
    QVariant toVariant() const override;
    bool setValue(const QVariant& v) override;
    //判断是否为null
    bool isNull() const;
    //获取python object
    DAPyObjectWrapper& object();
    const DAPyObjectWrapper& object() const;

protected:
    DAPyObjectWrapper _pyObject;
};
}  // namespace DA
#endif  // DADATAPYOBJECT_H
