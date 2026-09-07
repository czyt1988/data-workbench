#include "DADataPyDataFrame.h"
#include <iterator>
#include <algorithm>
#include <QMetaType>
#include "numpy/DAPyDType.h"
// DAMessageHandler
#include "DALogCategory.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DADataPyDataFrame
//===================================================

/**
 * @brief 构造函数
 * @param d dataframe
 */
DADataPyDataFrame::DADataPyDataFrame(const DAPyDataFrame& d) : DADataPyObject(d)
{
}

/**
 * @brief 析构函数
 */
DADataPyDataFrame::~DADataPyDataFrame()
{
}

/**
 * @brief 获取数据类型
 * @return 返回TypePythonDataFrame
 */
DAAbstractData::DataType DADataPyDataFrame::getDataType() const
{
    return TypePythonDataFrame;
}

/**
 * @brief 获取变量值
 * @param dim1 第一维索引（行）
 * @param dim2 第二维索引（列）
 * @return 对应位置的QVariant
 */
QVariant DADataPyDataFrame::toVariant(std::size_t dim1, std::size_t dim2) const
{
    DAPyDataFrame df(mPyObject.object());
    return df.iat(dim1, dim2);
}

/**
 * @brief 设置值
 * @param dim1 第一维索引（行）
 * @param dim2 第二维索引（列）
 * @param v 要设置的值
 * @return 设置成功返回true
 */
bool DADataPyDataFrame::setValue(std::size_t dim1, std::size_t dim2, const QVariant& v)
{
    try {
        DAPyDataFrame df(mPyObject.object());
        df.iat(dim1, dim2, v);
        return true;
    } catch (const std::exception& e) {
        qWarning() << QString("DADataPyDataFrame::setValue failed: %1").arg(e.what());
        return false;
    }
}

/**
 * @brief 获取表格数据源接口
 * @return 返回this
 */
DATableDataSource* DADataPyDataFrame::tableSource()
{
    return this;
}

/**
 * @brief 获取表格数据源接口（const版本）
 * @return 返回this
 */
const DATableDataSource* DADataPyDataFrame::tableSource() const
{
    return this;
}

/**
 * @brief 总行数
 * @return dataframe的行数
 */
std::size_t DADataPyDataFrame::tableRowCount() const
{
    return dataframe().shape().first;
}

/**
 * @brief 总列数
 * @return dataframe的列数
 */
std::size_t DADataPyDataFrame::tableColumnCount() const
{
    return dataframe().shape().second;
}

/**
 * @brief 列名
 * @param column 列号
 * @return 对应列名
 */
QString DADataPyDataFrame::tableColumnName(std::size_t column) const
{
    return dataframe().columnName(column);
}

/**
 * @brief 列类型（QMetaType类型id）
 * @param column 列号
 * @return 与QVariant caster转换语义对齐的类型id，无法确定返回QMetaType::UnknownType
 */
int DADataPyDataFrame::tableColumnType(std::size_t column) const
{
    try {
        DAPyDType dt = dataframe().dtypeObject(column);
        return dt.toMetaType();
    } catch (const std::exception& e) {
        qWarning() << QString("DADataPyDataFrame::tableColumnType failed for column %1: %2").arg(column).arg(e.what());
    }
    return QMetaType::UnknownType;
}

/**
 * @brief 批量取数据块
 *
 * 单次python调用取[startRow, startRow+rowCount)区间的整块数据，
 * 转换语义与逐cell的toVariant()一致，同时携带index作为行头
 * @param startRow 起始绝对行号
 * @param rowCount 期望行数，超出实际行数时自动截断
 * @return 数据块，startRow越界或python异常时返回无效块
 */
DATableDataBlock DADataPyDataFrame::fetchBlock(std::size_t startRow, std::size_t rowCount)
{
    try {
        DAPyDataFrame df                  = dataframe();
        std::pair< std::size_t, std::size_t > sp = df.shape();
        if (startRow >= sp.first) {
            return DATableDataBlock();
        }
        std::size_t n = std::min(rowCount, sp.first - startRow);
        DATableDataBlock block(startRow, sp.second);
        const QVariantList rows = df.rowsToVariantList(startRow, n);
        for (const QVariant& rowVar : rows) {
            block.appendRow(rowVar.value< QVariantList >());
        }
        block.setRowHeaders(df.indexToVariantList(startRow, n));
        return block;
    } catch (const std::exception& e) {
        qWarning() << QString("DADataPyDataFrame::fetchBlock failed at row %1: %2").arg(startRow).arg(e.what());
    }
    return DATableDataBlock();
}

/**
 * @brief 获取dataframe
 * @return DAPyDataFrame
 */
DAPyDataFrame DADataPyDataFrame::dataframe() const
{
    return DAPyDataFrame(mPyObject.object());
}

/**
 * @brief 获取列名列表
 * @return 列名列表
 */
QList< QString > DADataPyDataFrame::columns() const
{
    return DAPyDataFrame(mPyObject.object()).columns();
}

/**
 * @brief 尝试把df[name]转换为vector<double>
 * @param name
 * @return 如果获取失败，返回一个空的vector
 */
QVector< double > DADataPyDataFrame::getSeriesByVector(const QString& name) const
{
    QVector< double > res;
    try {
        DAPyDataFrame df = dataframe();
        DAPySeries ser   = df[ name ];
        if (ser.isNone()) {
            return res;
        }
        ser.castTo< double >(std::back_insert_iterator< QVector< double > >(res));
    } catch (const std::exception& e) {
        qWarning() << QString("getSeriesByVector failed for column '%1': %2").arg(name).arg(e.what());
        return QVector< double >();
    } catch (...) {
        qWarning() << QString("getSeriesByVector failed for column '%1': unknown exception").arg(name);
        return QVector< double >();
    }
    return res;
}
