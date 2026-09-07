#ifndef DADATAPYDATAFRAME_H
#define DADATAPYDATAFRAME_H

#include "DADataAPI.h"
#include <memory>
#include "DAAbstractData.h"
#include "DAPyObjectWrapper.h"
#include "pandas/DAPyDataFrame.h"
#include "DADataPyObject.h"
#include "DATableDataSource.h"
namespace DA
{
/**
 * @brief DAPyDataFrame 的封装
 *
 * 同时实现 DATableDataSource 表格数据源接口，schema 与块级取数
 * 均转发到 pandas（fetchBlock 为单次 python 调用的批量取数）
 */
class DADATA_API DADataPyDataFrame : public DADataPyObject, public DATableDataSource
{
public:
    DADataPyDataFrame(const DAPyDataFrame& d);
    ~DADataPyDataFrame() override;
    // 变量类型
    DataType getDataType() const override;
    // 变量值
    QVariant toVariant(std::size_t dim1, std::size_t dim2) const override;
    bool setValue(std::size_t dim1, std::size_t dim2, const QVariant& v) override;
    // 表格数据源接口
    DATableDataSource* tableSource() override;
    const DATableDataSource* tableSource() const override;
    // 获取dataframe
    DAPyDataFrame dataframe() const;
    // 以下是一些wrapper
    QList< QString > columns() const;
    //

public:  // DATableDataSource 接口实现
    std::size_t tableRowCount() const override;
    std::size_t tableColumnCount() const override;
    QString tableColumnName(std::size_t column) const override;
    int tableColumnType(std::size_t column) const override;
    DATableDataBlock fetchBlock(std::size_t startRow, std::size_t rowCount) override;

public:
    // 一些qt操作wrapper

    // 获取为QVector< double >，如果无法转换，返回一个空的vector
    QVector< double > getSeriesByVector(const QString& name) const;
};
}  // namespace DA
#endif  // DADATAPYDATAFRAME_H
