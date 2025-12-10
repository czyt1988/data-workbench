#ifndef DADATAPYSERIES_H
#define DADATAPYSERIES_H
#include "DADataAPI.h"
#include <memory>
#include "DAAbstractData.h"
#include "DAPyObjectWrapper.h"
#include "pandas/DAPySeries.h"
#include "DADataPyObject.h"
namespace DA
{
/**
 * @brief DAPySeries 的封装
 */
class DADATA_API DADataPySeries : public DADataPyObject
{
public:
    DADataPySeries(const DAPySeries& d);
    // 变量类型
    DataType getDataType() const override;
    // 变量值
    QVariant toVariant(std::size_t dim1, std::size_t dim2) const override;
    bool setValue(std::size_t dim1, std::size_t dim2, const QVariant& v) override;
    //
    DAPySeries series() const;
};
}
#endif  // DADATAPYSERIES_H
