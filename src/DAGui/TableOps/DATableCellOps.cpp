#include "DATableCellOps.h"
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"
#include "TableCore/DATableMutationCoordinator.h"
// qt
#include <QClipboard>
#include <QGuiApplication>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QStringList>
#include <QVariant>
// DAGui
#include "Commands/DACommandsDataFrame.h"
#include "DADataTableView.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"
// 性能埋点（不进界面消息队列，内容纯英文）
#include "DASlowLog.h"

#include <memory>

namespace DA
{
//===================================================
// DATableCellOps
//===================================================

DATableCellOps::DATableCellOps(const DATablePageContext& ctx,
                               const DATableSelectionService& sel,
                               DATableMutationCoordinator* mutation,
                               QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel), mMutation(mutation)
{
}

QString DATableCellOps::id() const
{
    return QStringLiteral("cell-ops");
}

QString DATableCellOps::title() const
{
    return tr("Cell Operations");  // cn:单元格操作
}

bool DATableCellOps::isAvailable() const
{
    return !mCtx.df().isNone();
}

/**
 * @brief 设置选中单元格为 NaN（可撤销）
 *
 * 后效走 Values commit（模型逐格刷新 + 受影响列统计失效 + 计算列标脏）。
 * @return 返回设置成功的个数
 */
int DATableCellOps::removeSelectedCells()
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return 0;
    }
    const QList< QPoint > cells = mSel.selectedDataframeCells();
    if (cells.size() <= 0) {
        daWarning << tr("Please select a valid cell");  // cn:请选择正确的单元格
        return 0;
    }
    QList< int > rows, cols;
    rows.reserve(cells.size());
    cols.reserve(cells.size());
    for (const auto p : cells) {
        rows.append(p.x());
        cols.append(p.y());
    }
    std::unique_ptr< DACommandDataFrame_setnan > cmd(new DACommandDataFrame_setnan(df, rows, cols));
    // 编辑后效（008 期2）：模型逐格刷新 + 受影响列统计失效 + 计算列标脏，收口到 commit
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Values;
    mutation.changedRows = rows;
    mutation.changedCols = cols;
    cmd->setCallBack(mMutation->callbackFor(mutation));
    if (!cmd->exec()) {
        return 0;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
    return cells.size();
}

/**
 * @brief 粘贴剪贴板内容到当前选区左上角
 *
 * 解析剪贴板 TSV 文本（Tab 分列、\n 分行，与 copySelectionToClipboard 格式对称），
 * 从当前选中区左上角起填充；超出数据表边界的部分丢弃并 daWarning 提示丢弃规模（D9）。
 * 数值列遇非数值文本时按 iat 现有转换链路处理（写入失败则整条命令回滚）。
 * @return 成功写入的单元格数（0 表示未写入）
 */
int DATableCellOps::pasteFromClipboard()
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return 0;
    }
    QClipboard* cb = QGuiApplication::clipboard();
    if (!cb) {
        return 0;
    }
    QString text = cb->text();
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.paste", [ &text ]() {
        return QStringLiteral("clipboard_len=%1").arg(text.size());
    });
    if (text.isEmpty()) {
        daWarning << tr("Clipboard is empty");  // cn:剪贴板为空
        return 0;
    }
    // 定位选区左上角；无选区时用当前光标格
    QModelIndex anchor;
    if (QItemSelectionModel* sm = mSel.selectionModel()) {
        // 段列表取首个区间左上角（等价于旧 selectedIndexes().first()，不物化整个选区）
        const QItemSelection sel = sm->selection();
        if (sel.isEmpty()) {
            anchor = sm->currentIndex();
        } else {
            anchor = sel.first().topLeft();
        }
    }
    if (!anchor.isValid()) {
        daWarning << tr("Please select a cell to paste into");  // cn:请先选中要粘贴的起始单元格
        return 0;
    }
    auto shape    = df.shape();
    const int dfRows = static_cast< int >(shape.first);
    const int dfCols = static_cast< int >(shape.second);
    // 视图行号是滑动窗内逻辑行，写 dataframe 前必须换成绝对行
    const int startRow = mCtx.toActualRow(anchor.row());
    const int startCol  = anchor.column();
    if (startRow >= dfRows || startCol >= dfCols) {
        return 0;
    }
    // 解析 TSV（Tab 分列、\n 分行、\r 行尾剔除）
    const QStringList lines = text.split(QLatin1Char('\n'));
    QList< int > rows, cols;
    QList< QVariant > values;
    int droppedRows = 0, droppedCols = 0;
    int contentLineCount = 0;
    for (int r = 0; r < lines.size(); ++r) {
        QString line = lines[ r ];
        // 结尾换行产生的末尾空行跳过；中间空行保留占位（与复制格式对称）
        if (r == lines.size() - 1 && line.isEmpty()) {
            break;
        }
        if (line.endsWith(QLatin1Char('\r'))) {
            line.chop(1);
        }
        const QStringList fields = line.split(QLatin1Char('\t'));
        const bool rowOverflow = (startRow + contentLineCount >= dfRows);
        int lineDroppedCols = 0;
        for (int c = 0; c < fields.size(); ++c) {
            if (startCol + c >= dfCols) {
                ++lineDroppedCols;
                continue;
            }
            if (rowOverflow) {
                continue;
            }
            rows.append(startRow + contentLineCount);
            cols.append(startCol + c);
            values.append(fields[ c ]);
        }
        if (rowOverflow) {
            ++droppedRows;
        } else {
            droppedCols += lineDroppedCols;
        }
        ++contentLineCount;
    }
    if (rows.isEmpty()) {
        daWarning << tr("Nothing to paste: the clipboard content exceeds the table boundary");  // cn:无可粘贴内容：剪贴板内容超出表格边界
        return 0;
    }
    if (droppedRows > 0 || droppedCols > 0) {
        daWarning << tr("Paste partially dropped: %1 row(s) and %2 cell(s) outside the table were ignored")
                        .arg(droppedRows)
                        .arg(droppedCols);  // cn:粘贴部分丢弃：%1 行与 %2 个单元格超出表格范围被忽略
    }
    std::unique_ptr< DACommandDataFrame_paste > cmd(new DACommandDataFrame_paste(df, rows, cols, values));
    // 编辑后效（008 期2）：模型逐格刷新 + 受影响列统计失效 + 计算列标脏，收口到 commit
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Values;
    mutation.changedRows = rows;
    mutation.changedCols = cols;
    cmd->setCallBack(mMutation->callbackFor(mutation));
    if (!cmd->exec()) {
        // 数值列写入非数值等场景：整条命令回滚
        daWarning << tr("Paste failed: the content does not match the column data type");  // cn:粘贴失败：内容与列数据类型不匹配
        return 0;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
    return rows.size();
}

/**
 * @brief 剪切选中区
 *
 * 先按复制格式导出到剪贴板，再把选中区置 NaN（复用 removeSelectedCells 语义）
 * @return 受影响单元格数
 */
int DATableCellOps::cutSelection()
{
    DADataTableView* view = mCtx.mainView();
    // 先复制（失败说明无选中）
    if (!view || !view->copySelectionToClipboard()) {
        daWarning << tr("No cells selected to cut");  // cn:没有选中可剪切的单元格
        return 0;
    }
    // 复制成功说明有选中，置 NaN（同 removeSelectedCells 语义）
    return removeSelectedCells();
}
}  // end of namespace DA
