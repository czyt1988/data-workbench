#ifndef DADATAPYSERIES_H
#define DADATAPYSERIES_H
#include "DADataAPI.h"
#include <memory>
#include "DAAbstractData.h"
#include "DAPyObjectWrapper.h"
#include "pandas/DAPySeries.h"
#include "DADataPyObject.h"
#include "DATableDataSource.h"
namespace DA
{
/**
 * @brief DAPySeries 的封装
 *
 * 同时实现 DATableDataSource 表格数据源接口，series 视为单列表格
 */
class DADATA_API DADataPySeries : public DADataPyObject, public DATableDataSource
{
public:
    DADataPySeries(const DAPySeries& d);
    // 变量类型
    DataType getDataType() const override;
    // 变量值
    QVariant toVariant(std::size_t dim1, std::size_t dim2) const override;
    bool setValue(std::size_t dim1, std::size_t dim2, const QVariant& v) override;
    // 表格数据源接口
    DATableDataSource* tableSource() override;
    const DATableDataSource* tableSource() const override;
    //
    DAPySeries series() const;

public:  // DATableDataSource 接口实现
    std::size_t tableRowCount() const override;
    std::size_t tableColumnCount() const override;
    QString tableColumnName(std::size_t column) const override;
    int tableColumnType(std::size_t column) const override;
    DATableDataBlock fetchBlock(std::size_t startRow, std::size_t rowCount) override;
};
}
#endif  // DADATAPYSERIES_H
