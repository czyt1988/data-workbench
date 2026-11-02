#include "DATableFillOps.h"
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"
#include "TableCore/DATableMutationCoordinator.h"
// stl
#include <algorithm>
// qt
#include <QDialog>
#include <QHash>
#include <QPoint>
#include <QVector>
// DAGui
#include "Commands/DACommandsDataFrame.h"
#include "Dialog/DADialogFillSeries.h"
// DAData
#include "DADataPyDataFrame.h"
// python 算子封装（填充求值）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

#include <memory>

namespace DA
{
//===================================================
// DATableFillOps
//===================================================

DATableFillOps::DATableFillOps(const DATablePageContext& ctx,
                               const DATableSelectionService& sel,
                               DATableMutationCoordinator* mutation,
                               QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel), mMutation(mutation)
{
}

QString DATableFillOps::id() const
{
    return QStringLiteral("fill-ops");
}

QString DATableFillOps::title() const
{
    return tr("Fill Operations");  // cn:填充操作
}

bool DATableFillOps::isAvailable() const
{
    return !mCtx.df().isNone() && mCtx.isDataSourceEditable();
}

/**
 * @brief 用填充清单构造一条 paste 命令并入栈（值记录，D4）
 *
 * 与 pasteFromClipboard 同款后效：模型逐格 notifyDataChanged、失效受影响列的
 * 条件格式统计、按变更列把依赖的计算列标 Stale（Values commit）。一次填充 =
 * 一条命令 = 一步撤销。
 * @param df 数据
 * @param rows 行位置（绝对）
 * @param cols 列位置（与 rows/values 一一对应）
 * @param values 新值
 * @param text 命令文案（已翻译）
 * @return 写入单元格数（0 表示未写入）
 */
int DATableFillOps::pushFillPasteCommand(const DAPyDataFrame& df,
                                         const QList< int >& rows,
                                         const QList< int >& cols,
                                         const QList< QVariant >& values,
                                         const QString& text)
{
    if (rows.isEmpty()) {
        return 0;
    }
    std::unique_ptr< DACommandDataFrame_paste > cmd(new DACommandDataFrame_paste(df, rows, cols, values));
    cmd->setText(text);
    // 编辑后效（008 期2）：模型逐格刷新 + 受影响列统计失效 + 计算列标脏，收口到 commit
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Values;
    mutation.changedRows = rows;
    mutation.changedCols = cols;
    cmd->setCallBack(mMutation->callbackFor(mutation));
    if (!cmd->exec()) {
        daWarning << tr("Fill failed: the content does not match the column data type");  // cn:填充失败：内容与列数据类型不匹配
        return 0;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
    return rows.size();
}

/**
 * @brief 选区填充（向下/向上/ffill/bfill）
 *
 * 按列独立处理：每列在自己的连续选中行区间内传递（选区服务的
 * selectedColumnRowRuns 切 run）。求值全部交 Python 算子 da_fill_selection
 * （空值 NaN/NaT/None 判定在 pandas 侧才可靠，设计稿 §4.8 硬要求），
 * 只返回变化的单元格；全部列的变化清单汇总成**一条** paste 命令入栈
 * （一次填充 = 一次撤销，D4）。只读/惰性源不生效（入口已置灰，此处二次守卫）。
 * @param mode 填充模式
 * @return 写入单元格数（0 表示无变化/无选区/只读源）
 */
int DATableFillOps::fillSelection(FillMode mode)
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return 0;
    }
    if (!mCtx.isDataSourceEditable()) {
        daWarning << tr("Cannot fill a read-only data source");  // cn:只读数据源不能填充
        return 0;
    }
    const QHash< int, QVector< QPair< int, int > > > runs = mSel.selectedColumnRowRuns();
    if (runs.isEmpty()) {
        return 0;
    }
    QString modeStr;
    QString text;
    switch (mode) {
    case FillMode::Down:
        modeStr = QStringLiteral("down");
        text    = tr("Fill down");  // cn:向下填充
        break;
    case FillMode::Up:
        modeStr = QStringLiteral("up");
        text    = tr("Fill up");  // cn:向上填充
        break;
    case FillMode::Forward:
        modeStr = QStringLiteral("ffill");
        text    = tr("Fill forward (ffill)");  // cn:向下填充空值(ffill)
        break;
    case FillMode::Backward:
        modeStr = QStringLiteral("bfill");
        text    = tr("Fill backward (bfill)");  // cn:向上填充空值(bfill)
        break;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    QList< int > allRows, allCols;
    QList< QVariant > allValues;
    // const 遍历（红线：非 const Qt 容器禁范围迭代）
    for (auto cit = runs.constBegin(); cit != runs.constEnd(); ++cit) {
        const int col = cit.key();
        const QVector< QPair< int, int > >& colRuns = cit.value();
        for (const QPair< int, int >& run : colRuns) {
            const DAPyScriptsDataFrame::FillSelectionResult r = py.fillSelection(df, col, run.first, run.second, modeStr);
            if (!r.ok) {
                continue;
            }
            for (int i = 0; i < r.rows.size(); ++i) {
                allRows.append(r.rows[ i ]);
                allCols.append(col);
                allValues.append(i < r.values.size() ? r.values[ i ] : QVariant());
            }
        }
    }
    return pushFillPasteCommand(df, allRows, allCols, allValues, text);
}

/**
 * @brief 序列填充对话框（数值等差 / 日期步进）
 *
 * 对话框收集参数并实时预览（da_fill_series，Python 求值，预览与写入同一算子，红线5）。
 * 接受后对选区**每一列**各生成一个序列（Excel 语义）：每列按自己选中行的个数生成等长
 * 序列，写入自己选中的行（选中行升序，非连续也按序填）。全部列汇总成一条 paste 命令入栈。
 * @return 写入单元格数（取消/无选区/只读源返回 0）
 */
int DATableFillOps::showFillSeriesDialog()
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return 0;
    }
    if (!mCtx.isDataSourceEditable()) {
        daWarning << tr("Cannot fill a read-only data source");  // cn:只读数据源不能填充
        return 0;
    }
    // 每列的选中行（升序去重），序列按此顺序写入
    const QList< QPoint > cells = mSel.selectedDataframeCells();
    if (cells.isEmpty()) {
        daWarning << tr("Please select cells to fill");  // cn:请先选中要填充的单元格
        return 0;
    }
    QHash< int, QList< int > > colRows;
    for (const QPoint& p : cells) {
        colRows[ p.y() ].append(p.x());
    }
    for (auto it = colRows.begin(); it != colRows.end(); ++it) {
        std::sort(it.value().begin(), it.value().end());
        it.value().erase(std::unique(it.value().begin(), it.value().end()), it.value().end());
    }
    // 预览个数取首列选中行数
    int previewCount = 5;
    for (auto it = colRows.constBegin(); it != colRows.constEnd(); ++it) {
        previewCount = it.value().size();
        break;
    }
    DADialogFillSeries dlg(previewCount, mCtx.hostWidget());
    if (QDialog::Accepted != dlg.exec()) {
        return 0;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    QList< int > allRows, allCols;
    QList< QVariant > allValues;
    for (auto it = colRows.constBegin(); it != colRows.constEnd(); ++it) {
        const int col          = it.key();
        const QList< int > rows = it.value();
        bool ok                = false;
        const QVariantList seq = py.fillSeries(rows.size(), dlg.kind(), dlg.startText(), dlg.stepText(), dlg.stopText(), dlg.dateUnit(), &ok);
        if (!ok) {
            daWarning << tr("Failed to generate the fill series");  // cn:生成填充序列失败
            continue;
        }
        const int n = qMin(rows.size(), seq.size());
        for (int i = 0; i < n; ++i) {
            allRows.append(rows[ i ]);
            allCols.append(col);
            allValues.append(seq[ i ]);
        }
    }
    return pushFillPasteCommand(df, allRows, allCols, allValues, tr("Fill series"));  // cn:序列填充
}
}  // end of namespace DA
