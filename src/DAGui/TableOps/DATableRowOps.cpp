#include "DATableRowOps.h"
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"
#include "TableCore/DATableMutationCoordinator.h"
// qt
#include <QPointer>
// DAGui
#include "Models/DADataTableModel.h"
#include "Commands/DACommandsDataFrame.h"
#include "DATableNoteStore.h"
#include "DATableStyleManager.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

#include <memory>

namespace DA
{
//===================================================
// DATableRowOps
//===================================================

DATableRowOps::DATableRowOps(const DATablePageContext& ctx,
                             const DATableSelectionService& sel,
                             DATableMutationCoordinator* mutation,
                             QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel), mMutation(mutation)
{
}

QString DATableRowOps::id() const
{
    return QStringLiteral("row-ops");
}

QString DATableRowOps::title() const
{
    return tr("Row Operations");  // cn:行操作
}

bool DATableRowOps::isAvailable() const
{
    // 插删行要求 dataframe 数据（只读的数据库惰性表不支持）
    return !mCtx.df().isNone();
}

/**
 * @brief 在 row 位置插入一个 NaN 行（可撤销）
 *
 * 插行拿不到列信息 → 后效走 Structure commit（全量统计失效 + 计算列全标脏）；
 * 样式/批注位置键重映射（>=row 的键 +1；undo 按删除处理）在 directional 回调内。
 * @param row 插入位置（数据源绝对行号）
 */
void DATableRowOps::insertRowAt(int row)
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        // 非dataframe数据（如只读的数据库惰性表）不支持插入行
        return;
    }
    std::unique_ptr< DACommandDataFrame_insertNanRow > cmd(new DACommandDataFrame_insertNanRow(df, row));
    // 编辑后效（008 期2）：插行拿不到列信息 → 全量统计失效 + 计算列全标脏，收口到 commit
    QPointer< DADataTableModel > modle = mCtx.model();
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Structure;
    mutation.modelRefresh = [ modle, row ]() {
        if (modle) {
            modle->notifyRowsInserted({ row });
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    // 行列增删样式键同步：插入行时 >=row 的键 +1；撤销时按删除处理（>=row 的键 -1）
    DATableStyleManager* styleMgr = mCtx.styleManager();
    DATableNoteStore* noteStore   = mCtx.noteStore();
    cmd->setDirectionalCallBack([ styleMgr, noteStore, row ](bool isUndo) {
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onRowsRemoved({ row });
            } else {
                styleMgr->onRowsInserted({ row });
            }
        }
        // 批注与样式同源位置键，插删行同款重映射（005 T1.26「批注跟着行走」）
        if (noteStore) {
            if (isUndo) {
                noteStore->onRowsRemoved({ row });
            } else {
                noteStore->onRowsInserted({ row });
            }
        }
    });
    if (!cmd->exec()) {
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
}

/**
 * @brief 删除选中的行（可撤销）
 *
 * 删行拿不到列信息 → 后效走 Structure commit；样式/批注位置键重映射
 * （==row 的键删除，>row 的键 -1；undo 按插入处理）在 directional 回调内。
 * 被删除行的样式/批注在删除时丢失，撤销无法恢复（已知限制，见 spec §7.3）。
 * @return 返回成功移除的行数
 */
int DATableRowOps::removeSelectedRows()
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return 0;
    }
    QList< int > rows = mSel.selectedDataframeRows();
    if (rows.size() <= 0) {
        daWarning << tr("Please select valid data cells");  // cn:请选择正确的行
        return 0;
    }
    std::unique_ptr< DACommandDataFrame_dropIRow > cmd(new DACommandDataFrame_dropIRow(df, rows));
    // 编辑后效（008 期2）：删行拿不到列信息 → 全量统计失效 + 计算列全标脏，收口到 commit
    QPointer< DADataTableModel > modle = mCtx.model();
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Structure;
    mutation.modelRefresh = [ modle, rows ]() {
        if (modle) {
            modle->notifyRowsInserted(rows);
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    // 行列增删样式键同步：删除行时 ==row 的键删除，>row 的键 -1；撤销时按插入处理
    DATableStyleManager* styleMgr = mCtx.styleManager();
    DATableNoteStore* noteStore   = mCtx.noteStore();
    cmd->setDirectionalCallBack([ styleMgr, noteStore, rows ](bool isUndo) {
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onRowsInserted(rows);
            } else {
                styleMgr->onRowsRemoved(rows);
            }
        }
        // 批注同款重映射（005 T1.26）：被删行的批注在删除时丢失（与样式同一已知限制，
        // undo 走 load 恢复 dataframe 无法恢复位置键元数据，见 spec §7.3）
        if (noteStore) {
            if (isUndo) {
                noteStore->onRowsInserted(rows);
            } else {
                noteStore->onRowsRemoved(rows);
            }
        }
    });
    if (!cmd->exec()) {
        return 0;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
    return rows.size();
}
}  // end of namespace DA
