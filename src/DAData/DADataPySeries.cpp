#include "DADataPySeries.h"
#include <algorithm>
#include <QMetaType>
#include "numpy/DAPyDType.h"
// DAMessageHandler
#include "DALogCategory.h"
namespace DA
{

/**
 * @brief 构造函数
 * @param d series
 */
DADataPySeries::DADataPySeries(const DAPySeries& d) : DADataPyObject(d)
{
}

/**
 * @brief 获取数据类型
 * @return 返回TypePythonSeries
 */
DAAbstractData::DataType DADataPySeries::getDataType() const
{
    return TypePythonSeries;
}

/**
 * @brief 获取变量值
 * @param dim1 第一维索引
 * @param dim2 第二维索引（未使用）
 * @return 对应位置的QVariant
 */
QVariant DADataPySeries::toVariant(std::size_t dim1, std::size_t dim2) const
{
    Q_UNUSED(dim2);
    DAPySeries ser(mPyObject.object());
    return ser.value(dim1);
}

/**
 * @brief 设置值
 * @param dim1 第一维索引
 * @param dim2 第二维索引（未使用）
 * @param v 要设置的值
 * @return 设置成功返回true
 */
bool DADataPySeries::setValue(std::size_t dim1, std::size_t dim2, const QVariant& v)
{
    Q_UNUSED(dim2);
    DAPySeries ser(mPyObject.object());
    return ser.setValue(dim1, v);
}

/**
 * @brief 获取series
 * @return DAPySeries
 */
DAPySeries DADataPySeries::series() const
{
    return DAPySeries(mPyObject.object());
}

/**
 * @brief 获取表格数据源接口
 * @return 返回this
 */
DATableDataSource* DADataPySeries::tableSource()
{
    return this;
}

/**
 * @brief 获取表格数据源接口（const版本）
 * @return 返回this
 */
const DATableDataSource* DADataPySeries::tableSource() const
{
    return this;
}

/**
 * @brief 总行数
 * @return series的元素数量
 */
std::size_t DADataPySeries::tableRowCount() const
{
    return series().size();
}

/**
 * @brief 总列数
 * @return series视为单列表格，返回1
 */
std::size_t DADataPySeries::tableColumnCount() const
{
    return 1;
}

/**
 * @brief 列名
 * @param column 列号，仅0有效
 * @return series的name，无名字或列号非0返回空字符串
 */
QString DADataPySeries::tableColumnName(std::size_t column) const
{
    if (column != 0) {
        return QString();
    }
    return series().name();
}

/**
 * @brief 列类型（QMetaType类型id）
 * @param column 列号，仅0有效
 * @return 与QVariant caster转换语义对齐的类型id，无法确定返回QMetaType::UnknownType
 */
int DADataPySeries::tableColumnType(std::size_t column) const
{
    if (column != 0) {
        return QMetaType::UnknownType;
    }
    try {
        DAPyDType dt = series().dtypeObject();
        return dt.toMetaType();
    } catch (const std::exception& e) {
        qWarning() << QString("DADataPySeries::tableColumnType failed: %1").arg(e.what());
    }
    return QMetaType::UnknownType;
}

/**
 * @brief 批量取数据块
 *
 * 单次python调用取[startRow, startRow+rowCount)区间的元素值与index，
 * 组装为单列数据块，转换语义与逐元素的toVariant()一致
 * @param startRow 起始绝对行号
 * @param rowCount 期望行数，超出实际行数时自动截断
 * @return 数据块，startRow越界或python异常时返回无效块
 */
DATableDataBlock DADataPySeries::fetchBlock(std::size_t startRow, std::size_t rowCount)
{
    try {
        DAPySeries ser       = series();
        std::size_t totalRow = ser.size();
        if (startRow >= totalRow) {
            return DATableDataBlock();
        }
        std::size_t n = std::min(rowCount, totalRow - startRow);
        DATableDataBlock block(startRow, 1);
        const QVariantList values = ser.valuesToVariantList(startRow, n);
        for (const QVariant& v : values) {
            QVariantList row;
            row.append(v);
            block.appendRow(row);
        }
        block.setRowHeaders(ser.indexToVariantList(startRow, n));
        return block;
    } catch (const std::exception& e) {
        qWarning() << QString("DADataPySeries::fetchBlock failed at row %1: %2").arg(startRow).arg(e.what());
    }
    return DATableDataBlock();
}
}
