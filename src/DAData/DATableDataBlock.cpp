#include "DATableDataBlock.h"
namespace DA
{

//===================================================
// DATableDataBlock
//===================================================

/**
 * @brief 默认构造，构造一个无效块
 */
DATableDataBlock::DATableDataBlock()
{
}

/**
 * @brief 构造一个有效块
 * @param startRow 块起始绝对行号
 * @param columnCount 列数
 */
DATableDataBlock::DATableDataBlock(std::size_t startRow, std::size_t columnCount)
    : mStartRow(startRow), mColumnCount(columnCount), mValid(true)
{
}

/**
 * @brief 是否有效，fetchBlock失败时返回无效块
 * @return 有效返回true
 */
bool DATableDataBlock::isValid() const
{
    return mValid;
}

/**
 * @brief 是否没有数据行
 * @return 无数据行返回true
 */
bool DATableDataBlock::isEmpty() const
{
    return mCells.isEmpty();
}

/**
 * @brief 块起始绝对行号
 * @return 起始行号
 */
std::size_t DATableDataBlock::startRow() const
{
    return mStartRow;
}

/**
 * @brief 块内数据行数
 * @return 行数
 */
std::size_t DATableDataBlock::rowCount() const
{
    return static_cast< std::size_t >(mCells.size());
}

/**
 * @brief 列数
 * @return 列数
 */
std::size_t DATableDataBlock::columnCount() const
{
    return mColumnCount;
}

/**
 * @brief 是否包含给定的绝对行号
 * @param actualRow 绝对行号
 * @return 包含返回true
 */
bool DATableDataBlock::containsRow(std::size_t actualRow) const
{
    if (!mValid || mCells.isEmpty()) {
        return false;
    }
    if (actualRow < mStartRow) {
        return false;
    }
    return (actualRow - mStartRow) < static_cast< std::size_t >(mCells.size());
}

/**
 * @brief 按绝对行号取值
 * @param actualRow 绝对行号
 * @param column 列号
 * @return 越界或无效块返回无效QVariant
 */
QVariant DATableDataBlock::cell(std::size_t actualRow, std::size_t column) const
{
    if (!containsRow(actualRow)) {
        return QVariant();
    }
    const QVariantList& row = mCells[ static_cast< int >(actualRow - mStartRow) ];
    if (column >= static_cast< std::size_t >(row.size())) {
        return QVariant();
    }
    return row[ static_cast< int >(column) ];
}

/**
 * @brief 按绝对行号取行头
 * @param actualRow 绝对行号
 * @return 无行头或越界返回无效QVariant
 */
QVariant DATableDataBlock::rowHeader(std::size_t actualRow) const
{
    if (!containsRow(actualRow)) {
        return QVariant();
    }
    std::size_t offset = actualRow - mStartRow;
    if (offset >= static_cast< std::size_t >(mRowHeaders.size())) {
        return QVariant();
    }
    return mRowHeaders[ static_cast< int >(offset) ];
}

/**
 * @brief 追加一行数据
 * @param row 一行数据，元素数量应与columnCount一致
 */
void DATableDataBlock::appendRow(const QVariantList& row)
{
    mCells.append(row);
}

/**
 * @brief 设置行头
 * @param headers 行头数据，与cells的行一一对应
 */
void DATableDataBlock::setRowHeaders(const QVariantList& headers)
{
    mRowHeaders = headers;
}

/**
 * @brief 行头数据
 * @return 行头列表
 */
const QVariantList& DATableDataBlock::rowHeaders() const
{
    return mRowHeaders;
}

/**
 * @brief 单元数据
 * @return 行数据列表
 */
const QVector< QVariantList >& DATableDataBlock::cells() const
{
    return mCells;
}

/**
 * @brief 清空为无效块
 */
void DATableDataBlock::clear()
{
    mStartRow    = 0;
    mColumnCount = 0;
    mCells.clear();
    mRowHeaders.clear();
    mValid = false;
}

}  // namespace DA
