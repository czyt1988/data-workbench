#include "DATableColumnCombineOps.h"
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"
#include "TableCore/DATableMutationCoordinator.h"
// stl
#include <algorithm>
// qt
#include <QDialog>
#include <QPointer>
#include <QWidget>
// DAGui
#include "Models/DADataTableModel.h"
#include "Commands/DACommandsDataFrame.h"
#include "DATableStyleManager.h"
#include "DATableNoteStore.h"
#include "DATableConditionalFormatStore.h"
#include "DATableComputedColumnStore.h"
#include "Dialog/DADialogSplitColumn.h"
#include "Dialog/DADialogMergeColumns.h"
// DAData
#include "DADataPyDataFrame.h"
// python 算子封装（源列快照）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"
// 忙等待光标（分列/合并执行）
#include "DAWaitCursorScoped.h"

#include <memory>

namespace DA
{
//===================================================
// DATableColumnCombineOps
//===================================================

DATableColumnCombineOps::DATableColumnCombineOps(const DATablePageContext& ctx,
                                                 const DATableSelectionService& sel,
                                                 DATableMutationCoordinator* mutation,
                                                 QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel), mMutation(mutation)
{
}

QString DATableColumnCombineOps::id() const
{
    return QStringLiteral("column-combine-ops");
}

QString DATableColumnCombineOps::title() const
{
    return tr("Split / Merge Columns");  // cn:分列/合并列
}

bool DATableColumnCombineOps::isAvailable() const
{
    return !mCtx.df().isNone() && mCtx.isDataSourceEditable();
}

/**
 * @brief 打开分列对话框并执行（004 T1.9）
 *
 * col<0 时取当前选区的唯一数据列（多列/无选区提示）。落位规则：结果列插在原列
 * 右侧、原列保留（算子注释同步写明）。撤销用列级记录：命令持源列快照 + 结果列名
 * 清单，undo 删结果列并写回源列快照（D4，内存代价=一列）。预览与执行同一算子
 * （对话框已用 da_head_copy + da_split_column 试算，红线5）。
 * 后效：结构刷新（notifyColumnsInserted）+ 统计保守全失效（Structure commit）；
 * 位置键元数据（样式/条件格式/批注）重映射在 directional 回调。
 * @param col 源列位置（-1=从选区解析）
 */
void DATableColumnCombineOps::showSplitColumnDialog(int col)
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone() || !mCtx.isDataSourceEditable()) {
        daWarning << tr("Split Column requires an editable table");  // cn:分列需要可编辑的表格
        return;
    }
    int sourceCol = col;
    if (sourceCol < 0) {
        const QList< int > sel = mSel.selectedDataframeColumns();
        if (sel.size() != 1) {
            daWarning << tr("Please select exactly one column to split");  // cn:请选择要拆分的单个列
            return;
        }
        sourceCol = sel.first();
    }
    DADialogSplitColumn dlg(df, sourceCol, mCtx.hostWidget());
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    const DACommandDataFrame_splitColumn::SplitParams params = dlg.params();
    DAPyScriptsDataFrame& py                                 = DAPyScripts::getDataFrame();
    // 源列快照（列级撤销记录，D4）
    const DAPySeries oldSource = py.getColumnCopy(df, sourceCol);
    if (oldSource.isNone()) {
        daWarning << tr("Failed to snapshot the source column");  // cn:源列快照失败
        return;
    }
    auto cmd = std::make_unique< DACommandDataFrame_splitColumn >(df, params, oldSource);
    QPointer< DADataTableModel > modle     = mCtx.model();
    DATableStyleManager* styleMgr          = mCtx.styleManager();
    DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore();
    DATableNoteStore* noteStore            = mCtx.noteStore();
    DACommandDataFrame_splitColumn* cmdRaw = cmd.get();
    const int srcCol                       = sourceCol;
    // 编辑后效（008 期2）：插列结构通知 + 全量统计失效 + 关浮窗 + 宿主树，收口到 commit
    DATableMutation mutation;
    mutation.kind               = DATableMutation::Structure;
    mutation.closeQuickPlot      = true;
    mutation.notifyHostValueChange = true;
    QPointer< DADataTableModel > modle2 = mCtx.model();
    mutation.modelRefresh           = [ modle2, srcCol ]() {
        if (modle2) {
            modle2->notifyColumnsInserted({ srcCol + 1 });  // 结构刷新（cacheShape + refreshWindow）
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    // 位置键元数据重映射（装饰类 R20）：结果列插入原列右侧 → >=插入点的键右移；undo 反向
    cmd->setDirectionalCallBack([ styleMgr, cfStore, noteStore, cmdRaw, srcCol ](bool isUndo) {
        const int n = cmdRaw->insertedNames().size();
        if (n <= 0) {
            return;
        }
        QList< int > inserted;
        inserted.reserve(n);
        for (int i = 1; i <= n; ++i) {
            inserted.append(srcCol + i);
        }
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onColumnsRemoved(inserted);
            } else {
                styleMgr->onColumnsInserted(inserted);
            }
        }
        if (cfStore) {
            if (isUndo) {
                cfStore->onColumnsRemoved(inserted);
            } else {
                cfStore->onColumnsInserted(inserted);
            }
        }
        // 批注列维同款重映射（005 T1.26）
        if (noteStore) {
            if (isUndo) {
                noteStore->onColumnsRemoved(inserted);
            } else {
                noteStore->onColumnsInserted(inserted);
            }
        }
    });
    {
        DAWaitCursorScoped waitCursor;  // 10 万行分列实测亚秒级，仍加忙等待（验收 §4.4）
        if (!cmd->exec()) {
            daWarning << tr("Split column failed: %1").arg(cmd->lastErrorMessage());  // cn:分列失败：%1
            return;
        }
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
}

/**
 * @brief 打开合并列对话框并执行（004 T1.9）
 *
 * 源列 = 当前选区数据列（>=2，顺序可在对话框内调整）。落位规则：结果列替换被合并
 * 列的位置（插到最左源列处，源列删除）。撤销用列级记录：命令持全部源列快照，
 * undo 删结果列并按原位置升序写回源列（D4）。预览与执行同一算子（红线5）。
 * 被合并列若被计算列依赖 → 相关计算列置失效并提示（与删列联动同款，004 T1.23）。
 * 后效：结构刷新（notifyColumnsRemoved）+ 统计保守全失效；位置键元数据重映射
 * （exec=删源列+插结果列；undo 反向）；计算列 undo 走整店快照恢复。
 */
void DATableColumnCombineOps::showMergeColumnsDialog()
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone() || !mCtx.isDataSourceEditable()) {
        daWarning << tr("Merge Columns requires an editable table");  // cn:合并列需要可编辑的表格
        return;
    }
    QList< int > sel = mSel.selectedDataframeColumns();
    if (sel.size() < 2) {
        daWarning << tr("Please select at least two columns to merge");  // cn:请选择至少两列进行合并
        return;
    }
    std::sort(sel.begin(), sel.end());
    DADialogMergeColumns dlg(df, sel, mCtx.hostWidget());
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    const DACommandDataFrame_mergeColumns::MergeParams params = dlg.params();
    DAPyScriptsDataFrame& py                                  = DAPyScripts::getDataFrame();
    // 源列快照（与 params.cols 顺序一一对应，列级撤销记录 D4）
    QList< DAPySeries > oldSources;
    oldSources.reserve(params.cols.size());
    for (int c : params.cols) {
        const DAPySeries s = py.getColumnCopy(df, c);
        if (s.isNone()) {
            daWarning << tr("Failed to snapshot the source columns");  // cn:源列快照失败
            return;
        }
        oldSources.append(s);
    }
    // 计算列联动（004 T1.23）：源列被删 → 依赖它的计算列失效；快照供 undo 恢复
    DATableComputedColumnStore* ccStore = mCtx.computedColumnStore();
    const QList< QString > colNames = df.columns();
    QStringList removedNames;
    for (int c : params.cols) {
        if (c >= 0 && c < colNames.size()) {
            removedNames.append(colNames.at(c));
        }
    }
    QList< DATableComputedColumnStore::Entry > ccSnapshot;
    QStringList invalidatedTargets;
    if (ccStore) {
        ccSnapshot = ccStore->snapshot();
        for (const QString& n : std::as_const(removedNames)) {
            const QStringList refs = ccStore->definitionsReferencing(n);
            for (const QString& t : refs) {
                if (!invalidatedTargets.contains(t) && !removedNames.contains(t)) {
                    invalidatedTargets.append(t);
                }
            }
        }
    }
    auto cmd = std::make_unique< DACommandDataFrame_mergeColumns >(df, params, oldSources);
    DATableStyleManager* styleMgr          = mCtx.styleManager();
    DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore();
    DATableNoteStore* noteStore            = mCtx.noteStore();
    const int insertPos                    = cmd->insertPosition();
    QList< int > srcCols                   = params.cols;
    std::sort(srcCols.begin(), srcCols.end());
    // 编辑后效（008 期2）：删列结构通知 + 全量统计失效 + 关浮窗 + 宿主树，收口到 commit
    DATableMutation mutation;
    mutation.kind               = DATableMutation::Structure;
    mutation.closeQuickPlot      = true;
    mutation.notifyHostValueChange = true;
    QPointer< DADataTableModel > modle2 = mCtx.model();
    mutation.modelRefresh           = [ modle2, insertPos ]() {
        if (modle2) {
            modle2->notifyColumnsRemoved({ insertPos });  // 结构刷新（cacheShape + refreshWindow）
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    cmd->setDirectionalCallBack([ styleMgr, cfStore, noteStore, ccStore, srcCols, insertPos, ccSnapshot, removedNames ](bool isUndo) {
        // 位置键元数据（装饰类 R20）：exec=删源列+插结果列；undo=删结果列+回插源列
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onColumnsRemoved({ insertPos });
                styleMgr->onColumnsInserted(srcCols);
            } else {
                styleMgr->onColumnsRemoved(srcCols);
                styleMgr->onColumnsInserted({ insertPos });
            }
        }
        if (cfStore) {
            if (isUndo) {
                cfStore->onColumnsRemoved({ insertPos });
                cfStore->onColumnsInserted(srcCols);
            } else {
                cfStore->onColumnsRemoved(srcCols);
                cfStore->onColumnsInserted({ insertPos });
            }
        }
        // 批注列维同款重映射（005 T1.26）：源列批注随删列丢弃，结果列位置右移的键回位
        if (noteStore) {
            if (isUndo) {
                noteStore->onColumnsRemoved({ insertPos });
                noteStore->onColumnsInserted(srcCols);
            } else {
                noteStore->onColumnsRemoved(srcCols);
                noteStore->onColumnsInserted({ insertPos });
            }
        }
        // 计算列（语义类，列名引用）：exec=删列联动；undo=整店快照恢复
        if (ccStore) {
            if (isUndo) {
                ccStore->restore(ccSnapshot);
            } else {
                ccStore->onColumnsRemoved(removedNames);
            }
        }
    });
    {
        DAWaitCursorScoped waitCursor;
        if (!cmd->exec()) {
            daWarning << tr("Merge columns failed: %1").arg(cmd->lastErrorMessage());  // cn:合并列失败：%1
            return;
        }
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
    // 失效提示（不静默，004 T1.23 R9）
    if (!invalidatedTargets.isEmpty()) {
        daWarning << tr("Computed column(s) %1 became invalid because their dependency column(s) were merged")
                         .arg(invalidatedTargets.join(QStringLiteral(", ")));  // cn:计算列 %1 因依赖列被合并已失效
    }
}
}  // end of namespace DA
