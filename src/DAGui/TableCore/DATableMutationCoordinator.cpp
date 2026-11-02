#include "DATableMutationCoordinator.h"
#include "DATablePageContext.h"
// stl
#include <algorithm>
// qt
#include <QPointer>
// DAGui
#include "Models/DADataTableModel.h"
#include "DATableConditionalFormatStore.h"
#include "DATableComputedColumnStore.h"

namespace DA
{
namespace
{
// 列集合去重排序（变更清单的列收敛为唯一列集合）
QList< int > toUniqueSorted(const QList< int >& in)
{
    QList< int > out = in;
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}
}  // namespace

//===================================================
// DATableMutationCoordinator
//===================================================

DATableMutationCoordinator::DATableMutationCoordinator(const DATablePageContext& ctx, QObject* parent)
    : QObject(parent), mCtx(ctx)
{
}

/**
 * @brief 编辑后效唯一入口
 *
 * 后效顺序钉死（同旧手抄实现的顺序）：
 * 1. 模型刷新（值变更逐格 / 结构变更经 m.modelRefresh）；
 * 2. 条件格式列统计失效；
 * 3. 计算列标待重算；
 * 4. 浮窗关闭 / 宿主树刷新。
 * @param m 变更描述
 */
void DATableMutationCoordinator::commit(const DATableMutation& m)
{
    switch (m.kind) {
    case DATableMutation::Values:
        commitValues(m);
        break;
    case DATableMutation::Structure:
    case DATableMutation::FullReset:
        commitStructure(m);
        break;
    }
    if (m.closeQuickPlot && mCtx.closeFloatingWindows) {
        mCtx.closeFloatingWindows();
    }
    if (m.notifyHostValueChange && mCtx.notifyHostValueChange) {
        mCtx.notifyHostValueChange();
    }
}

/**
 * @brief 给命令 setCallBack 用的后效回调
 *
 * exec 与 undo/redo 共用一次 commit；QPointer 防悬挂（宿主页面销毁后
 * 命令仍在撤销栈时静默丢弃）。
 * @param m 变更描述（值捕获）
 * @return 可直接传给 DACallBackInterface::setCallBack 的回调
 */
std::function< void() > DATableMutationCoordinator::callbackFor(const DATableMutation& m) const
{
    QPointer< DATableMutationCoordinator > self(const_cast< DATableMutationCoordinator* >(this));
    return [ self, m ]() {
        if (self) {
            self->commit(m);
        }
    };
}

/**
 * @brief 值变更后效（收口自 removeSelectCell/pasteFromClipboard/pushFillPasteCommand 的手抄回调）
 *
 * 顺序与旧实现逐行一致：模型通知（modelRefresh 给出更高效形状时优先，否则逐格
 * notifyDataChanged）→ 受影响列统计失效 → 计算列标脏。
 * @param m 变更描述（changedRows/changedCols 一一对应）
 */
void DATableMutationCoordinator::commitValues(const DATableMutation& m)
{
    if (m.modelRefresh) {
        // 命令侧给出整列级通知形状（如类型转换的 notifyColumnChanged 循环）
        m.modelRefresh();
    } else if (DADataTableModel* model = mCtx.model()) {
        const auto size = qMin(m.changedRows.size(), m.changedCols.size());
        for (int i = 0; i < size; ++i) {
            model->notifyDataChanged(m.changedRows[ i ], m.changedCols[ i ]);
        }
    }
    if (DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore()) {
        const QList< int > uniqueCols = toUniqueSorted(m.changedCols);
        for (int c : uniqueCols) {
            cfStore->invalidateColumnStats(c);
        }
    }
    markComputedColumnsStale(m.changedCols);
}

/**
 * @brief 结构/整表变更后效（收口自 insertRowAt/removeSelectRow/removeColumns/changeSelectColumnToIndex）
 *
 * 模型结构通知形状由命令侧经 m.modelRefresh 给出（notifyRowsInserted/notifyColumnsRemoved/refreshData），
 * 之后全量统计失效 + 计算列全标脏（行/列增删拿不到精确的变更列信息，保守全失效）。
 * @param m 变更描述
 */
void DATableMutationCoordinator::commitStructure(const DATableMutation& m)
{
    if (m.modelRefresh) {
        m.modelRefresh();
    }
    if (m.kind == DATableMutation::FullReset) {
        if (DADataTableModel* model = mCtx.model()) {
            model->refreshData();
        }
    }
    if (DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore()) {
        cfStore->invalidateAllStats();
    }
    markAllComputedColumnsStale();
}

/**
 * @brief 数据变更列 → 计算列待重算标记（收口自 Facade 的同名私有助手）
 *
 * 只标状态不求值（R8 手动重算）；排序路径不走这里（R13 行顺序变化不影响列值）。
 * @param actualCols 变更列位置（空列表 = 无操作）
 */
void DATableMutationCoordinator::markComputedColumnsStale(const QList< int >& actualCols)
{
    DATableComputedColumnStore* ccStore = mCtx.computedColumnStore();
    if (!ccStore || ccStore->isEmpty() || actualCols.isEmpty()) {
        return;
    }
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return;
    }
    const QList< QString > cols = df.columns();
    QStringList names;
    names.reserve(actualCols.size());
    for (int c : actualCols) {
        if (c >= 0 && c < cols.size()) {
            names.append(cols.at(c));
        }
    }
    ccStore->markStaleForChangedColumns(names);
}

/**
 * @brief 整表操作保守把全部计算列标待重算（拿不到变更列信息时用，R8）
 */
void DATableMutationCoordinator::markAllComputedColumnsStale()
{
    DATableComputedColumnStore* ccStore = mCtx.computedColumnStore();
    if (!ccStore || ccStore->isEmpty()) {
        return;
    }
    ccStore->markAllStale();
}
}  // end of namespace DA
