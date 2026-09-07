#ifndef DATABLEDATABLOCK_H
#define DATABLEDATABLOCK_H
#include <QVariant>
#include <QVariantList>
#include <QVector>
#include <cstddef>
#include "DADataAPI.h"
namespace DA
{
/**
 * @brief 表格数据块，DATableDataSource::fetchBlock 的批量取数单元
 *
 * 块记录一段连续的行区间 [startRow, startRow + rowCount) 与 columnCount 列的 QVariant 数据，
 * 行寻址使用绝对行号，越界访问返回无效 QVariant 而不会崩溃。
 * 默认构造的块为无效块（isValid 返回 false），表示一次失败的取数。
 */
class DADATA_API DATableDataBlock
{
public:
    DATableDataBlock();
    DATableDataBlock(std::size_t startRow, std::size_t columnCount);
    // 是否有效（fetchBlock 失败返回无效块）
    bool isValid() const;
    // 是否没有数据行
    bool isEmpty() const;
    // 块起始绝对行号
    std::size_t startRow() const;
    // 块内数据行数
    std::size_t rowCount() const;
    // 列数
    std::size_t columnCount() const;
    // 是否包含给定的绝对行号
    bool containsRow(std::size_t actualRow) const;
    // 按绝对行号取值，越界返回无效QVariant
    QVariant cell(std::size_t actualRow, std::size_t column) const;
    // 按绝对行号取行头，无行头或越界返回无效QVariant
    QVariant rowHeader(std::size_t actualRow) const;
    // 追加一行数据，行的元素数量应与columnCount一致
    void appendRow(const QVariantList& row);
    // 设置行头（与cells的行一一对应）
    void setRowHeaders(const QVariantList& headers);
    // 行头数据
    const QVariantList& rowHeaders() const;
    // 单元数据
    const QVector< QVariantList >& cells() const;
    // 清空为无效块
    void clear();

private:
    std::size_t mStartRow { 0 };
    std::size_t mColumnCount { 0 };
    QVector< QVariantList > mCells;
    QVariantList mRowHeaders;
    bool mValid { false };
};
}  // namespace DA
#endif  // DATABLEDATABLOCK_H
