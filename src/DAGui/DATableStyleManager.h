#ifndef DATABLESTYLEMANAGER_H
#define DATABLESTYLEMANAGER_H
#include "DAGuiAPI.h"
#include "DATableCellStyle.h"
#include "DATableDisplayFormat.h"
#include <QObject>
#include <QHash>
#include <QPair>
#include <QList>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
/**
 * @brief 表格样式管理器，三层存储（单元格/列/行）
 *
 * 键用 actualRow/actualCol（DataFrame 真实行号），缓存窗口滑动时键不变。
 * 渲染叠加优先级：单元格 > 行 > 列（最具体优先级最高）。
 * 行列增删同步时三个维度联动（行操作不影响列级，反之亦然）。
 */
class DAGUI_API DATableStyleManager : public QObject
{
    Q_OBJECT
public:
    explicit DATableStyleManager(QObject* parent = nullptr);

    // 渲染查询：返回该单元格最终叠加样式（列→行→单元格逐层 merge）
    DATableCellStyle resolveCellStyle(int actualRow, int actualCol) const;

    // 单元格级
    bool hasCellStyle(int actualRow, int actualCol) const;
    DATableCellStyle getCellStyle(int actualRow, int actualCol) const;
    void setCellStyle(int actualRow, int actualCol, const DATableCellStyle& s, bool merge = true);
    void clearCell(int actualRow, int actualCol);

    // 列级
    bool hasColumnStyle(int actualCol) const;
    DATableCellStyle getColumnStyle(int actualCol) const;
    void setColumnStyle(int actualCol, const DATableCellStyle& s, bool merge = true);
    void clearColumn(int actualCol);
    QList< int > styledColumns() const;

    // 列级显示格式（独立于 bg/fg/font 的 DATableCellStyle）
    bool hasColumnFormat(int actualCol) const;
    DATableDisplayFormat getColumnFormat(int actualCol) const;
    void setColumnFormat(int actualCol, const DATableDisplayFormat& fmt, bool emitSignal = true);
    void clearColumnFormat(int actualCol);
    QList< int > styledFormatColumns() const;

    // 行级
    bool hasRowStyle(int actualRow) const;
    DATableCellStyle getRowStyle(int actualRow) const;
    void setRowStyle(int actualRow, const DATableCellStyle& s, bool merge = true);
    void clearRow(int actualRow);
    QList< int > styledRows() const;
    // 获取所有有样式的单元格键列表（actualRow, actualCol）
    QList< QPair< int, int > > styledCells() const;

    // 批量清除
    void clearRange(int rowStart, int colStart, int rowEnd, int colEnd);
    void clearAll();

    // 行列增删同步（actualRows/actualCols 为变更的真实行列号列表）
    void onRowsInserted(const QList< int >& actualRows);
    void onRowsRemoved(const QList< int >& actualRows);
    void onColumnsInserted(const QList< int >& actualCols);
    void onColumnsRemoved(const QList< int >& actualCols);

    // 序列化
    void toXml(QDomDocument& doc, QDomElement& e) const;
    bool fromXml(const QDomElement& e);
    bool isEmpty() const;

Q_SIGNALS:
    // 单格样式变更，参数为 actualRow/actualCol
    void styleChanged(int actualRow, int actualCol);
    // 范围样式变更，end 坐标为 -1 表示该维度延伸到末尾
    // 例如 setColumnStyle 发射 (0, col, -1, col) 表示该列所有行
    void styleRangeChanged(int rowStart, int colStart, int rowEnd, int colEnd);
    // 整表重置（clearAll 或行列大调整）
    void styleReset();

private:
    // 单元格级：(actualRow, actualCol) -> style
    QHash< QPair< int, int >, DATableCellStyle > mCellStyles;
    // 列级：actualCol -> style
    QHash< int, DATableCellStyle > mColumnStyles;
    // 列级显示格式：actualCol -> display format（独立于 mColumnStyles）
    QHash< int, DATableDisplayFormat > mColumnFormats;
    // 行级：actualRow -> style
    QHash< int, DATableCellStyle > mRowStyles;
};
}  // end of namespace DA
#endif  // DATABLESTYLEMANAGER_H
