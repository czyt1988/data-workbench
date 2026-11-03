#include "Table/Core/DATableMutationCoordinator.h"
#include "Table/Core/DATablePageContext.h"
// stl
#include <algorithm>
// qt
#include <QPointer>
// DAGui
#include "Table/Models/DADataTableModel.h"
#include "Table/Views/DADataTableView.h"
#include "Table/Metadata/DATableConditionalFormatRule.h"
#include "Table/Metadata/DATableConditionalFormatStore.h"
#include "Table/Metadata/DATableComputedColumnStore.h"
#include "Table/Views/DATableHighlightOverlay.h"
#include "Table/Metadata/DATableNoteStore.h"
#include "Table/Metadata/DATableStyleManager.h"
#include "Table/Metadata/DATableValidationStore.h"
// DAPyScripts 算子封装（条件格式统计/重复键提供者，色阶/数据条端点）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"

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
 * @brief 一次性订阅：各子存储/模型的「变更→失效/刷新」接线
 *
 * 收口此前散在页面构造里的 store→model 连接（Observer，构造时一次性订阅）。
 * 全部只依赖上下文（模型/子存储/窗格），不持窗口指针；会话级子存储
 * （styleManager/validationStore 等借自注册表，比页面长寿）以本对象为
 * 连接上下文，页面析构时随子对象销毁自动断开。
 */
void DATableMutationCoordinator::attach()
{
    DADataTableModel* model = mCtx.model();
    // —— 批注变更只影响角标（不经模型 dataChanged 链路），重绘主窗格视口即可 ——
    if (DATableNoteStore* noteStore = mCtx.noteStore()) {
        if (DADataTableView* view = mCtx.mainView()) {
            connect(noteStore, &DATableNoteStore::notesChanged, this, [ view ]() {
                view->viewport()->update();
            });
        }
    }
    // —— 瞬态高亮层变更 → 全部窗格视口重绘（瞬态装饰不经模型 dataChanged 链路）——
    if (DATableHighlightOverlay* overlay = mCtx.highlightOverlay()) {
        connect(overlay, &DATableHighlightOverlay::changed, this, [ this ]() {
            for (DADataTableView* pane : mCtx.panes()) {
                pane->viewport()->update();
            }
        });
    }
    // —— 样式变更 → view 刷新（actualRow 转回 logical row 通知 model）——
    if (DATableStyleManager* styleMgr = mCtx.styleManager()) {
        connect(styleMgr, &DATableStyleManager::styleChanged, this, [ model ](int actualRow, int actualCol) {
            if (model) {
                model->notifyDataChanged(actualRow, actualCol);
            }
        });
        connect(styleMgr, &DATableStyleManager::styleRangeChanged, this, [ model ](int rowStart, int colStart, int rowEnd, int colEnd) {
            if (!model) {
                return;
            }
            // -1 表示整个维度；行号统一用数据源绝对行，由 model 换算到窗口内
            const int rStart = (rowStart < 0) ? 0 : rowStart;
            const int rEnd   = (rowEnd < 0) ? qMax(0, model->actualRowCount() - 1) : rowEnd;
            const int cStart = (colStart < 0) ? 0 : colStart;
            const int cEnd   = (colEnd < 0) ? model->columnCount() - 1 : colEnd;
            model->notifyDataChanged(rStart, cStart, rEnd, cEnd);
        });
        connect(styleMgr, &DATableStyleManager::styleReset, this, [ model ]() {
            if (model) {
                // 全表刷新
                model->notifyDataChanged(0, 0, qMax(0, model->actualRowCount() - 1), model->columnCount() - 1);
            }
        });
    }
    // —— 条件格式（003 T1.5）：规则变更 → 重绘对应列（-1 整表）；统计/重复键提供者注入 ——
    if (DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore()) {
        connect(cfStore, &DATableConditionalFormatStore::rulesChanged, this, [ model ](int column) {
            if (!model) {
                return;
            }
            const int rEnd = qMax(0, model->actualRowCount() - 1);
            if (column < 0) {
                model->notifyDataChanged(0, 0, rEnd, qMax(0, model->columnCount() - 1));
            } else {
                model->notifyDataChanged(0, column, rEnd, column);
            }
        });
        connect(cfStore, &DATableConditionalFormatStore::ruleReset, this, [ model ]() {
            if (model) {
                model->notifyDataChanged(0, 0, qMax(0, model->actualRowCount() - 1), qMax(0, model->columnCount() - 1));
            }
        });
        // 列统计提供者（色阶/数据条端点）：store 是会话级（比页面长寿），
        // lambda 用 QPointer 防悬挂，页面析构时另行清空提供者
        QPointer< DATableMutationCoordinator > self(this);
        cfStore->setStatsProvider([ self ](int column, DATableConditionalFormatStore::ColumnStats& out) {
            if (!self) {
                return false;
            }
            DAPyDataFrame df = self->mCtx.df();
            if (df.isNone()) {
                return false;
            }
            DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
            const DAPyScriptsDataFrame::ColumnStatsResult r = py.columnStats(df, column);
            if (!r.ok) {
                return false;
            }
            out.minValue    = r.minValue;
            out.maxValue    = r.maxValue;
            out.medianValue = r.medianValue;
            return true;
        });
        // 重复键提供者（重复/唯一条件）：与统计同款 QPointer 防悬挂 + 惰性缓存语义
        cfStore->setDuplicatesProvider([ self ](int column, QSet< QString >& out) {
            if (!self) {
                return false;
            }
            DAPyDataFrame df = self->mCtx.df();
            if (df.isNone()) {
                return false;
            }
            DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
            QList< QVariant > values;
            if (!py.columnDuplicateValues(df, column, values)) {
                return false;
            }
            for (const QVariant& v : std::as_const(values)) {
                const QString key = DATableConditionalFormatRule::duplicateKey(v);
                if (!key.isEmpty()) {
                    out.insert(key);
                }
            }
            return true;
        });
        // 统计失效挂点（widget 侧，计划 WP2 步骤 5）：单元格编辑命令在模型内部创建，
        // 不走命令回调，用模型的 cellValueChanged 精确信号失效对应列。
        // 不用 dataChanged：滑窗滚动（setCacheWindowStartRow）与样式刷新也发整窗
        // dataChanged，会在滚动时每步误失效统计（破坏"滚动千行无卡顿"验收）；
        // 排序不发本信号（不改列值集合，不失效——性能决策）
        connect(model, &DADataTableModel::cellValueChanged, this, [ cfStore ](int, int actualColumn) {
            cfStore->invalidateColumnStats(actualColumn);
        });
    }
    // 计算列待重算标记（004 T1.23 R8 手动重算）：单元格编辑走 cellValueChanged 精确信号，
    // 只把依赖该列（或目标列自身被改）的计算列标 Stale；排序不发本信号（R13 不触发）
    connect(model, &DADataTableModel::cellValueChanged, this, [ this ](int, int actualColumn) {
        markComputedColumnsStale({ actualColumn });
    });
    // —— 唯一性违规缓存失效挂点（005 T1.25 §4.9）：数据/结构变更后重复集合可能变化，
    //    保守全失效（缓存只为「同一份数据重复点校验」省算，正确性优先）——
    if (DATableValidationStore* vaStore = mCtx.validationStore()) {
        connect(model, &QAbstractItemModel::modelReset, this, [ vaStore ]() {
            vaStore->invalidateUniqueCache();
        });
        connect(model, &QAbstractItemModel::rowsInserted, this, [ vaStore ]() {
            vaStore->invalidateUniqueCache();
        });
        connect(model, &QAbstractItemModel::rowsRemoved, this, [ vaStore ]() {
            vaStore->invalidateUniqueCache();
        });
        connect(model, &QAbstractItemModel::columnsInserted, this, [ vaStore ]() {
            vaStore->invalidateUniqueCache();
        });
        connect(model, &QAbstractItemModel::columnsRemoved, this, [ vaStore ]() {
            vaStore->invalidateUniqueCache();
        });
        connect(model, &DADataTableModel::cellValueChanged, this, [ vaStore ](int, int) {
            vaStore->invalidateUniqueCache();
        });
    }
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
