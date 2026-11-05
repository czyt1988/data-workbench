#include "Table/Controllers/DATableStyleController.h"
#include "Table/Core/DATablePageContext.h"
#include "Table/Core/DATableSelectionService.h"
// qt
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QSet>
// stl
#include <algorithm>
#include <limits>
#include <memory>
// DAGui
#include "Table/Models/DADataTableModel.h"
#include "Table/Commands/DACommandsTableStyle.h"
#include "Table/Commands/DACommandsTableColumnFormat.h"
#include "Table/Metadata/DATableStyleManager.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"
// 性能埋点（不进界面消息队列，内容纯英文）
#include "DASlowLog.h"

namespace DA
{
namespace
{
// 小选区逐格精确比对的格数上限：选区不超过它且存在行级/单元格级样式时才逐格
// （全选卡顿修复）；超过则不做逐格一致性证明（见 getCurrentCellStyle 注释）
constexpr qint64 c_styleExactScanCellLimit = 10000;

// 逐属性一致性流式累加器：不物化选区样式列表（全选时是 窗口行数×列数 的
// QList<DATableCellStyle>，宽表下千万级堆分配直接冻结界面），只保留首项样式
// 与各属性的一致标志；判定语义与旧实现的逐属性全量扫描一致
class CellStyleConsensus
{
public:
    void add(const DATableCellStyle& s)
    {
        if (!mHaveFirst) {
            mFirst     = s;
            mHaveFirst = true;
            return;
        }
        mBgOK   = mBgOK && (s.backgroundValid() == mFirst.backgroundValid())
                && (!mFirst.backgroundValid() || s.background() == mFirst.background());
        mFgOK   = mFgOK && (s.foregroundValid() == mFirst.foregroundValid())
                && (!mFirst.foregroundValid() || s.foreground() == mFirst.foreground());
        mFontOK = mFontOK && (s.fontValid() == mFirst.fontValid())
                && (!mFirst.fontValid() || s.font() == mFirst.font());
        mAlignOK = mAlignOK && (s.alignmentValid() == mFirst.alignmentValid())
                && (!mFirst.alignmentValid() || s.alignment() == mFirst.alignment());
        mBorderOK = mBorderOK && (s.borderValid() == mFirst.borderValid())
                && (!mFirst.borderValid()
                    || (s.borderSides() == mFirst.borderSides() && s.borderColor() == mFirst.borderColor()
                        && s.borderWidth() == mFirst.borderWidth()));
        mWrapOK = mWrapOK && (s.wrapWordValid() == mFirst.wrapWordValid())
                && (!mFirst.wrapWordValid() || s.wrapWord() == mFirst.wrapWord());
    }

    DATableCellStyle take()
    {
        DATableCellStyle result;
        if (!mHaveFirst) {
            return result;
        }
        if (mBgOK && mFirst.backgroundValid()) {
            result.setBackground(mFirst.background());
        }
        if (mFgOK && mFirst.foregroundValid()) {
            result.setForeground(mFirst.foreground());
        }
        if (mFontOK && mFirst.fontValid()) {
            result.setFont(mFirst.font());
        }
        if (mAlignOK && mFirst.alignmentValid()) {
            result.setAlignment(mFirst.alignment());
        }
        if (mBorderOK && mFirst.borderValid()) {
            result.setBorder(mFirst.borderSides(), mFirst.borderColor(), mFirst.borderWidth());
        }
        if (mWrapOK && mFirst.wrapWordValid()) {
            result.setWrapWord(mFirst.wrapWord());
        }
        return result;
    }

private:
    DATableCellStyle mFirst;
    bool mHaveFirst { false };
    bool mBgOK { true };
    bool mFgOK { true };
    bool mFontOK { true };
    bool mAlignOK { true };
    bool mBorderOK { true };
    bool mWrapOK { true };
};
}  // namespace

//===================================================
// DATableStyleController
//===================================================

DATableStyleController::DATableStyleController(const DATablePageContext& ctx,
                                               const DATableSelectionService& sel,
                                               QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel)
{
}

/**
 * @brief 合并样式片段到选中区
 *
 * 自动判定层级：选中整列→列级，选中整行→行级，否则单元格级。
 * fragment 只设部分属性（如只设 background），merge 到目标已有样式，未设属性保留。
 * @param fragment 样式片段
 */
void DATableStyleController::mergeStyleToSelection(const DATableCellStyle& fragment)
{
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr) {
        return;
    }
    // 判定应用层级
    QList< int > fullCols = mSel.fullySelectedDataframeColumns(false);
    QList< int > fullRows = mSel.fullySelectedDataframeRows(false);
    const QList< QPoint > cells = mSel.selectedDataframeCells(false);
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.style.mergeSelection", [ &cells, &fullCols, &fullRows ]() {
        return QStringLiteral("cells=%1 fullCols=%2 fullRows=%3")
            .arg(cells.size())
            .arg(fullCols.size())
            .arg(fullRows.size());
    });

    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }

    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(styleMgr));

    if (!fullCols.isEmpty()) {
        // 列级：合并 fragment 到已有列样式
        for (int col : std::as_const(fullCols)) {
            DATableCellStyle oldStyle = styleMgr->getColumnStyle(col);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, newStyle, true);
        }
    } else if (!fullRows.isEmpty()) {
        // 行级：合并 fragment 到已有行样式
        for (int row : std::as_const(fullRows)) {
            const int actualRow       = row;
            DATableCellStyle oldStyle = styleMgr->getRowStyle(actualRow);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, newStyle, true);
        }
    } else {
        // 单元格级：合并 fragment 到已有单元格样式
        for (const QPoint& p : cells) {
            // QPoint(actualRow, column)
            const int actualRow       = p.x();
            const int col             = p.y();
            DATableCellStyle oldStyle = styleMgr->getCellStyle(actualRow, col);
            DATableCellStyle newStyle = oldStyle;
            newStyle.mergeFrom(fragment);
            cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, newStyle, true);
        }
    }

    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
}

/**
 * @brief 外框线：对选中区包围矩形四边应用边框（单条命令，一次撤销）
 *
 * 语义按选区形态分三档：
 * - 单元格选区：包围矩形四边，逐格只给落在包围矩形外沿的边（内部格无框线）；
 * - 整列选区：列级左右边 + 首/末数据行各补上下边（不逐行枚举，10 万行也 O(列数)）；
 * - 整行选区：行级上下边 + 首/末列各补左右边。
 * 边界只画一次由委派层的相邻边去重保证（见 DATableStyleItemDelegate::paintCellBorder）。
 */
void DATableStyleController::applyBorderOutlineToSelection()
{
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr) {
        return;
    }
    QList< int > fullCols = mSel.fullySelectedDataframeColumns(false);
    QList< int > fullRows = mSel.fullySelectedDataframeRows(false);
    const QList< QPoint > cells = mSel.selectedDataframeCells(false);
    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }
    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(styleMgr));
    // 单元格级加一条边（merge 语义，保留该格其它属性）
    auto addCellBorder = [ & ](int actualRow, int col, quint8 sides) {
        DATableCellStyle oldStyle = styleMgr->getCellStyle(actualRow, col);
        DATableCellStyle newStyle = oldStyle;
        newStyle.setBorder(sides);
        cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, newStyle, true);
    };
    if (!fullCols.isEmpty()) {
        // 整列选区：列级左右 + 首/末数据行上下
        const int lastRow = mCtx.model() ? mCtx.model()->actualRowCount() - 1 : -1;
        for (int col : std::as_const(fullCols)) {
            DATableCellStyle oldStyle = styleMgr->getColumnStyle(col);
            DATableCellStyle newStyle = oldStyle;
            newStyle.setBorder(DATableCellStyle::BorderLeft | DATableCellStyle::BorderRight);
            cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, newStyle, true);
            if (lastRow >= 0) {
                addCellBorder(0, col, DATableCellStyle::BorderTop);
                addCellBorder(lastRow, col, DATableCellStyle::BorderBottom);
            }
        }
    } else if (!fullRows.isEmpty()) {
        // 整行选区：行级上下 + 首/末列左右
        const int lastCol = mCtx.model() ? mCtx.model()->columnCount() - 1 : -1;
        for (int row : std::as_const(fullRows)) {
            DATableCellStyle oldStyle = styleMgr->getRowStyle(row);
            DATableCellStyle newStyle = oldStyle;
            newStyle.setBorder(DATableCellStyle::BorderTop | DATableCellStyle::BorderBottom);
            cmd->addChange(DACommandTableStyle::Row, row, 0, oldStyle, newStyle, true);
            if (lastCol >= 0) {
                addCellBorder(row, 0, DATableCellStyle::BorderLeft);
                addCellBorder(row, lastCol, DATableCellStyle::BorderRight);
            }
        }
    } else {
        // 单元格选区：包围矩形四边，逐格只给外沿边（QPoint(actualRow, column)）
        int minRow = std::numeric_limits< int >::max();
        int maxRow = std::numeric_limits< int >::min();
        int minCol = std::numeric_limits< int >::max();
        int maxCol = std::numeric_limits< int >::min();
        for (const QPoint& p : cells) {
            minRow = qMin(minRow, p.x());
            maxRow = qMax(maxRow, p.x());
            minCol = qMin(minCol, p.y());
            maxCol = qMax(maxCol, p.y());
        }
        for (const QPoint& p : cells) {
            quint8 sides = DATableCellStyle::BorderNone;
            if (p.y() == minCol) {
                sides |= DATableCellStyle::BorderLeft;
            }
            if (p.y() == maxCol) {
                sides |= DATableCellStyle::BorderRight;
            }
            if (p.x() == minRow) {
                sides |= DATableCellStyle::BorderTop;
            }
            if (p.x() == maxRow) {
                sides |= DATableCellStyle::BorderBottom;
            }
            if (sides != DATableCellStyle::BorderNone) {
                addCellBorder(p.x(), p.y(), sides);
            }
        }
    }
    if (cmd->isEmpty()) {
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
}

/**
 * @brief 单侧边框：只作用于选区外沿（Excel 语义）
 *
 * 选中 3x3 区域应用「上框线」时，只有最上一行 3 个单元格加上框线，选区内其它格不受
 * 影响；下/左/右同理。全框线（BorderAll）与无框线（BorderNone=0）仍作用于整个选区，
 * 直接复用 mergeStyleToSelection。
 *
 * 选区层级：
 * - 整列选区：左右落在首/末列（列级），上/下落在首/末数据行（单元格级）；
 * - 整行选区：上下落在首/末行（行级），左/右落在首/末数据列（单元格级）；
 * - 单元格选区：按包围矩形外沿，逐格只加命中外沿的那一侧。
 * 已有边框按位并集保留（应用上框线不会抹掉原有左/右边），单条命令一次撤销。
 * @param sides BorderSide 位组合（通常为单个边；BorderAll/0 走全选区语义）
 */
void DATableStyleController::applyBorderSidesToSelection(quint8 sides)
{
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr) {
        return;
    }
    // 全框线 / 无框线：所有选中目标整体处理，保持既有语义
    if (sides == DATableCellStyle::BorderNone
        || (sides & DATableCellStyle::BorderAll) == DATableCellStyle::BorderAll) {
        DATableCellStyle fragment;
        fragment.setBorder(sides);
        mergeStyleToSelection(fragment);
        return;
    }
    QList< int > fullCols = mSel.fullySelectedDataframeColumns(false);
    QList< int > fullRows = mSel.fullySelectedDataframeRows(false);
    const QList< QPoint > cells = mSel.selectedDataframeCells(false);
    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }

    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(styleMgr));
    // 在已有边框上并集追加，保留颜色/宽度及其它边
    auto unionBorder = [](const DATableCellStyle& oldStyle, quint8 addSides) {
        DATableCellStyle newStyle = oldStyle;
        const quint8 oldSides =
            oldStyle.borderValid() ? oldStyle.borderSides() : quint8(DATableCellStyle::BorderNone);
        newStyle.setBorder(oldSides | addSides, oldStyle.borderColor(), oldStyle.borderWidth());
        return newStyle;
    };
    auto addCellBorder = [ & ](int actualRow, int col, quint8 addSides) {
        DATableCellStyle oldStyle = styleMgr->getCellStyle(actualRow, col);
        cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, unionBorder(oldStyle, addSides), true);
    };
    auto addColumnBorder = [ & ](int col, quint8 addSides) {
        DATableCellStyle oldStyle = styleMgr->getColumnStyle(col);
        cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, unionBorder(oldStyle, addSides), true);
    };
    auto addRowBorder = [ & ](int actualRow, quint8 addSides) {
        DATableCellStyle oldStyle = styleMgr->getRowStyle(actualRow);
        cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, unionBorder(oldStyle, addSides), true);
    };
    // 升序去重后切成连续区间（Ctrl 多选不相邻列/行时各自保留自己的外沿）
    auto contiguousRuns = [](QList< int > lines) {
        std::sort(lines.begin(), lines.end());
        lines.erase(std::unique(lines.begin(), lines.end()), lines.end());
        QList< QPair< int, int > > runs;
        for (int line : std::as_const(lines)) {
            if (!runs.isEmpty() && line == runs.last().second + 1) {
                runs.last().second = line;
            } else {
                runs.append(qMakePair(line, line));
            }
        }
        return runs;
    };
    if (!fullCols.isEmpty()) {
        // 整列选区：左右落在首/末列，上下落在首/末数据行
        const int lastRow = mCtx.model() ? mCtx.model()->actualRowCount() - 1 : -1;
        const QList< QPair< int, int > > runs = contiguousRuns(fullCols);
        for (const QPair< int, int >& run : runs) {
            if (sides & DATableCellStyle::BorderLeft) {
                addColumnBorder(run.first, DATableCellStyle::BorderLeft);
            }
            if (sides & DATableCellStyle::BorderRight) {
                addColumnBorder(run.second, DATableCellStyle::BorderRight);
            }
        }
        if (sides & DATableCellStyle::BorderTop) {
            for (int col : std::as_const(fullCols)) {
                addCellBorder(0, col, DATableCellStyle::BorderTop);
            }
        }
        if ((sides & DATableCellStyle::BorderBottom) && lastRow >= 0) {
            for (int col : std::as_const(fullCols)) {
                addCellBorder(lastRow, col, DATableCellStyle::BorderBottom);
            }
        }
    } else if (!fullRows.isEmpty()) {
        // 整行选区：上下落在首/末行，左右落在首/末数据列
        const int lastCol = mCtx.model() ? mCtx.model()->columnCount() - 1 : -1;
        const QList< QPair< int, int > > runs = contiguousRuns(fullRows);
        for (const QPair< int, int >& run : runs) {
            if (sides & DATableCellStyle::BorderTop) {
                addRowBorder(run.first, DATableCellStyle::BorderTop);
            }
            if (sides & DATableCellStyle::BorderBottom) {
                addRowBorder(run.second, DATableCellStyle::BorderBottom);
            }
        }
        if (sides & DATableCellStyle::BorderLeft) {
            for (int row : std::as_const(fullRows)) {
                addCellBorder(row, 0, DATableCellStyle::BorderLeft);
            }
        }
        if ((sides & DATableCellStyle::BorderRight) && lastCol >= 0) {
            for (int row : std::as_const(fullRows)) {
                addCellBorder(row, lastCol, DATableCellStyle::BorderRight);
            }
        }
    } else {
        // 单元格选区：包围矩形外沿，逐格只加命中外沿的那一侧（QPoint(actualRow, column)）
        int minRow = std::numeric_limits< int >::max();
        int maxRow = std::numeric_limits< int >::min();
        int minCol = std::numeric_limits< int >::max();
        int maxCol = std::numeric_limits< int >::min();
        for (const QPoint& p : cells) {
            minRow = qMin(minRow, p.x());
            maxRow = qMax(maxRow, p.x());
            minCol = qMin(minCol, p.y());
            maxCol = qMax(maxCol, p.y());
        }
        for (const QPoint& p : cells) {
            quint8 addSides = DATableCellStyle::BorderNone;
            if ((sides & DATableCellStyle::BorderTop) && p.x() == minRow) {
                addSides |= DATableCellStyle::BorderTop;
            }
            if ((sides & DATableCellStyle::BorderBottom) && p.x() == maxRow) {
                addSides |= DATableCellStyle::BorderBottom;
            }
            if ((sides & DATableCellStyle::BorderLeft) && p.y() == minCol) {
                addSides |= DATableCellStyle::BorderLeft;
            }
            if ((sides & DATableCellStyle::BorderRight) && p.y() == maxCol) {
                addSides |= DATableCellStyle::BorderRight;
            }
            if (addSides != DATableCellStyle::BorderNone) {
                addCellBorder(p.x(), p.y(), addSides);
            }
        }
    }
    if (cmd->isEmpty()) {
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
}

/**
 * @brief 设置完整样式到选中区（整体替换，为格式刷准备）
 *
 * 自动判定层级：选中整列→列级，选中整行→行级，否则单元格级。
 * 传入什么样式就设置什么样式，完全替换目标已有样式（不做 merge）。
 * style 中 valid=false 的属性会让目标对应属性变为"未设置"。
 * @param style 完整样式
 */
void DATableStyleController::applyStyleToSelection(const DATableCellStyle& style)
{
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr) {
        return;
    }
    QList< int > fullCols = mSel.fullySelectedDataframeColumns(false);
    QList< int > fullRows = mSel.fullySelectedDataframeRows(false);
    const QList< QPoint > cells = mSel.selectedDataframeCells(false);
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.style.applySelection", [ &cells, &fullCols, &fullRows ]() {
        return QStringLiteral("cells=%1 fullCols=%2 fullRows=%3")
            .arg(cells.size())
            .arg(fullCols.size())
            .arg(fullRows.size());
    });

    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }

    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(styleMgr));

    if (!fullCols.isEmpty()) {
        // 列级：直接替换
        for (int col : std::as_const(fullCols)) {
            DATableCellStyle oldStyle = styleMgr->getColumnStyle(col);
            cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, style, false);
        }
    } else if (!fullRows.isEmpty()) {
        // 行级：直接替换
        for (int row : std::as_const(fullRows)) {
            const int actualRow       = row;
            DATableCellStyle oldStyle = styleMgr->getRowStyle(actualRow);
            cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, style, false);
        }
    } else {
        // 单元格级：直接替换
        for (const QPoint& p : cells) {
            const int actualRow       = p.x();
            const int col             = p.y();
            DATableCellStyle oldStyle = styleMgr->getCellStyle(actualRow, col);
            cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, style, false);
        }
    }

    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
}

/**
 * @brief 清除选中区样式
 *
 * 自动判定层级，仅清除有样式的目标的样式。
 */
void DATableStyleController::clearStyleSelection()
{
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr) {
        return;
    }
    QList< int > fullCols = mSel.fullySelectedDataframeColumns(false);
    QList< int > fullRows = mSel.fullySelectedDataframeRows(false);
    const QList< QPoint > cells = mSel.selectedDataframeCells(false);

    if (cells.isEmpty() && fullCols.isEmpty() && fullRows.isEmpty()) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return;
    }

    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(styleMgr));

    if (!fullCols.isEmpty()) {
        for (int col : std::as_const(fullCols)) {
            if (styleMgr->hasColumnStyle(col)) {
                DATableCellStyle oldStyle = styleMgr->getColumnStyle(col);
                cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, DATableCellStyle(), false);
            }
        }
    } else if (!fullRows.isEmpty()) {
        for (int row : std::as_const(fullRows)) {
            const int actualRow = row;
            if (styleMgr->hasRowStyle(actualRow)) {
                DATableCellStyle oldStyle = styleMgr->getRowStyle(actualRow);
                cmd->addChange(DACommandTableStyle::Row, actualRow, 0, oldStyle, DATableCellStyle(), false);
            }
        }
    } else {
        for (const QPoint& p : cells) {
            const int actualRow = p.x();
            const int col       = p.y();
            if (styleMgr->hasCellStyle(actualRow, col)) {
                DATableCellStyle oldStyle = styleMgr->getCellStyle(actualRow, col);
                cmd->addChange(DACommandTableStyle::Cell, actualRow, col, oldStyle, DATableCellStyle(), false);
            }
        }
    }

    // 避免推送空命令到 undo 栈（选中区无样式时不产生 undo 步骤）
    if (cmd->isEmpty()) {
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
}

/**
 * @brief 清除整表所有样式（可撤销）
 *
 * 记录所有现有样式的 oldStyle，构造撤销命令。
 */
void DATableStyleController::clearStyleAll()
{
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr || styleMgr->isEmpty()) {
        return;
    }
    std::unique_ptr< DACommandTableStyle > cmd(new DACommandTableStyle(styleMgr));
    // 收集列级样式
    const auto& cs = styleMgr->styledColumns();
    for (int col : cs) {
        DATableCellStyle oldStyle = styleMgr->getColumnStyle(col);
        cmd->addChange(DACommandTableStyle::Column, col, 0, oldStyle, DATableCellStyle(), false);
    }

    // 收集行级样式
    const auto& rs = styleMgr->styledRows();
    for (int row : rs) {
        DATableCellStyle oldStyle = styleMgr->getRowStyle(row);
        cmd->addChange(DACommandTableStyle::Row, row, 0, oldStyle, DATableCellStyle(), false);
    }
    // 收集单元格级样式
    const auto& cells = styleMgr->styledCells();
    for (const QPair< int, int >& key : cells) {
        DATableCellStyle oldStyle = styleMgr->getCellStyle(key.first, key.second);
        cmd->addChange(DACommandTableStyle::Cell, key.first, key.second, oldStyle, DATableCellStyle(), false);
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
}

/**
 * @brief 获取选中区代表的样式
 *
 * 对每个属性（background/foreground/font）独立判断选中区所有单元格的一致性：
 * - 所有单元格该属性值相同 → 该属性 valid，返回具体值
 * - 该属性在不同单元格有不同值 → 该属性 invalid（ribbon 不调整该控件）
 * 无选中时返回空样式。
 *
 * @note 性能（全选卡顿修复）：按三级策略计算：
 * 1) 快路径（精确）：选区内没有行级/单元格级样式 → 每格最终样式只由列级样式决定，
 *    一致性只需比较各选中列的列样式（含"无样式"档），O(选中列数)；
 * 2) 小选区（精确）：选区格数 ≤ c_styleExactScanCellLimit 且有行级/单元格级样式，
 *    走逐格流式比对（CellStyleConsensus，不物化列表）；
 * 3) 大选区（近似）：既有行级/单元格级样式、选区又超阈值时，不再逐格证明一致性，
 *    返回空样式（ribbon 控件保持原状态）。样式指示是提示性 UI，为它扫描几十万格
 *    不值得（Excel 同场景也只认活动单元格格式）。
 * 路径 1)/2) 与旧实现逐位等价（含尾部预留空白行列的遍历语义）；3) 是有意取舍。
 * @return 代表样式
 */
DATableCellStyle DATableStyleController::getCurrentCellStyle() const
{
    DATableCellStyle result;
    DATableStyleManager* styleMgr = mCtx.styleManager();
    DADataTableModel* model        = mCtx.model();
    if (!styleMgr || !model) {
        return result;
    }
    QItemSelectionModel* sm = mSel.selectionModel();
    if (!sm) {
        return result;
    }
    const QItemSelection sel = sm->selection();
    if (sel.isEmpty()) {
        return result;
    }
    // 选区格数与选中列集合（区间运算，不物化索引）
    qint64 cellCount = 0;
    QSet< int > selCols;
    for (const QItemSelectionRange& r : sel) {
        cellCount += qint64(r.width()) * qint64(r.height());
        for (int c = r.left(); c <= r.right(); ++c) {
            selCols.insert(c);
        }
    }
    const int windowStart = model->getCacheWindowStartRow();
    const bool rowStyleHit  = mSel.selectionIntersectsRowStyles(sel, windowStart);
    const bool cellStyleHit = rowStyleHit ? false : mSel.selectionIntersectsCellStyles(sel, windowStart);
    if (!rowStyleHit && !cellStyleHit) {
        // 1) 快路径：每格最终样式 = 列级样式（或无），一致性由选中列的列样式决定
        //（尾部预留空白列没有列样式，参与比较后与旧实现的逐格遍历等价）
        CellStyleConsensus acc;
        for (int c : std::as_const(selCols)) {
            acc.add(styleMgr->hasColumnStyle(c) ? styleMgr->getColumnStyle(c) : DATableCellStyle());
        }
        return acc.take();
    }
    if (cellCount > c_styleExactScanCellLimit) {
        // 3) 大选区 + 行/格级样式：放弃精确一致性（见函数注释）
        return result;
    }
    // 2) 小选区：逐格流式比对（范围含尾部预留空白行列，与旧实现一致）
    CellStyleConsensus acc;
    for (const QItemSelectionRange& r : sel) {
        for (int row = r.top(); row <= r.bottom(); ++row) {
            const int actualRow = mCtx.toActualRow(row);
            for (int col = r.left(); col <= r.right(); ++col) {
                acc.add(styleMgr->resolveCellStyle(actualRow, col));
            }
        }
    }
    return acc.take();
}

/**
 * @brief 收集当前选中区涉及的列号（用于列级显示格式操作）
 *
 * 优先用整列选中（fullySelectedDataframeColumns），否则取选中单元格涉及的列
 * （selectedDataframeColumns，去重）。两者都空返回空列表。
 * @param ensureInDataframe 是否限定在 dataframe 范围内
 * @return 列号列表
 */
QList< int > DATableStyleController::collectSelectedColumns(bool ensureInDataframe) const
{
    QList< int > cols = mSel.fullySelectedDataframeColumns(ensureInDataframe);
    if (cols.isEmpty()) {
        cols = mSel.selectedDataframeColumns(ensureInDataframe);
    }
    return cols;
}

/**
 * @brief 设置列显示格式到选中列
 *
 * 列级操作：对选中区涉及的所有列设置格式（可撤销）。
 * @param fmt 显示格式
 */
void DATableStyleController::setDisplayFormatToSelection(const DATableDisplayFormat& fmt)
{
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr) {
        return;
    }
    QList< int > cols = collectSelectedColumns(false);
    if (cols.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    std::unique_ptr< DACommandTableColumnFormat > cmd(new DACommandTableColumnFormat(styleMgr));
    for (int col : std::as_const(cols)) {
        DATableDisplayFormat oldFmt =
            styleMgr->hasColumnFormat(col) ? styleMgr->getColumnFormat(col) : DATableDisplayFormat();
        cmd->addChange(col, oldFmt, fmt);
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
}

/**
 * @brief 清除选中列的显示格式
 *
 * 仅清除显示格式，保留 bg/fg/font 样式。仅对有格式的列记录变更。
 */
void DATableStyleController::clearDisplayFormatFromSelection()
{
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr) {
        return;
    }
    QList< int > cols = collectSelectedColumns(false);
    if (cols.isEmpty()) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    std::unique_ptr< DACommandTableColumnFormat > cmd(new DACommandTableColumnFormat(styleMgr));
    for (int col : std::as_const(cols)) {
        if (styleMgr->hasColumnFormat(col)) {
            DATableDisplayFormat oldFmt = styleMgr->getColumnFormat(col);
            cmd->addChange(col, oldFmt, DATableDisplayFormat());  // invalid → 清除
        }
    }
    if (cmd->isEmpty()) {
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
}

/**
 * @brief 获取选中列代表的显示格式
 *
 * 收集选中列各自的列级格式，若全部一致（含全 invalid）返回该格式，
 * 否则返回 invalid（ribbon 格式下拉框回退到 General）。
 * @return 代表格式
 */
DATableDisplayFormat DATableStyleController::getCurrentColumnDisplayFormat() const
{
    DATableDisplayFormat result;  // invalid
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr) {
        return result;
    }
    QList< int > cols = collectSelectedColumns(false);
    if (cols.isEmpty()) {
        return result;
    }
    QList< DATableDisplayFormat > fmts;
    for (int col : std::as_const(cols)) {
        fmts.append(styleMgr->hasColumnFormat(col) ? styleMgr->getColumnFormat(col)
                                                   : DATableDisplayFormat());
    }
    const DATableDisplayFormat& first = fmts.first();
    for (int i = 1; i < fmts.size(); ++i) {
        if (!(fmts[ i ] == first)) {
            return result;  // 不一致 → invalid
        }
    }
    return first;
}
}  // end of namespace DA
