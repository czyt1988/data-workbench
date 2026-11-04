#include "Table/Controllers/DATableSortController.h"
#include "Table/Core/DATablePageContext.h"
// stl
#include <algorithm>
#include <memory>
#include <utility>
// qt
#include <QAbstractItemModel>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QMap>
#include <QPointer>
// DAGui
#include "Table/Views/DADataHeaderView.h"
#include "Table/Views/DADataTableView.h"
#include "Table/Models/DADataTableModel.h"
#include "Table/Ops/DACommandsDataFrame.h"
#include "Table/Metadata/DATableKeyRemap.h"
#include "Table/Metadata/DATableNoteStore.h"
#include "Table/Metadata/DATableStyleManager.h"
#include "Table/Metadata/DATableValidationStore.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"
// 忙等待光标（排序是整表操作）
#include "DAWaitCursorScoped.h"
// 性能埋点（不进界面消息队列，内容纯英文）
#include "DASlowLog.h"

namespace DA
{
namespace
{
}

//===================================================
// DATableSortController
//===================================================

DATableSortController::DATableSortController(const DATablePageContext& ctx, QObject* parent)
    : QObject(parent), mCtx(ctx)
{
}

/**
 * @brief 当前多列排序规格
 * @return 列逻辑索引+方向列表（顺序即优先级；不持久化 R13）
 */
QVector< QPair< int, Qt::SortOrder > > DATableSortController::sortSpec() const
{
    return mSortSpec;
}

/**
 * @brief 按列排序（重置排序规格为「仅该列」）
 *
 * 排序入口 = 列头右键菜单 / Ribbon「排序」面板。只读/惰性源 daWarning 提示一次；
 * 拾取模式下不执行。
 * @param logicalIndex 列逻辑索引（c_indexSortColumn 表示按索引排序）
 * @param order 排序方向
 */
void DATableSortController::sortColumn(int logicalIndex, Qt::SortOrder order)
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.sort.column", [ logicalIndex ]() {
        return QStringLiteral("column=%1").arg(logicalIndex);
    });
    applySortAction(logicalIndex, order, false);
}

/**
 * @brief 追加为次级排序列（保留既有规格；该列已在规格中则切换其方向）
 * @param logicalIndex 列逻辑索引
 * @param order 排序方向
 */
void DATableSortController::addSecondarySortColumn(int logicalIndex, Qt::SortOrder order)
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.sort.secondaryColumn", [ logicalIndex ]() {
        return QStringLiteral("column=%1").arg(logicalIndex);
    });
    applySortAction(logicalIndex, order, true);
}

/**
 * @brief 按索引升序恢复原始行顺序（Ribbon「清除排序」入口）
 *
 * 行头显示的是 DataFrame 的 index 标签，排序后标签随行移动、看上去变成乱序；
 * 按索引升序把行按原始行号还原（默认 RangeIndex 即排序前的顺序）。
 * 与其它排序一样是一条可撤销命令；规格记为「仅索引列升序」（表头无箭头可画）。
 */
void DATableSortController::restoreOriginalOrder()
{
    DA_SLOW_LOG_SCOPE("dagui.table.sort.restoreOriginalOrder");
    applySortAction(c_indexSortColumn, Qt::AscendingOrder, false);
}

/**
 * @brief 应用完整排序规格（自定义排序对话框入口）
 *
 * 规格清洗：剔除越界列号（索引哨兵除外）、同列只保留首次出现（列名组合框天然去重，
 * 这里再兜一层）；与当前规格完全相同则不执行，避免推入一条数据无变化的撤销命令。
 * 空规格不执行——「清除排序」是独立入口（restoreOriginalOrder），对话框确定按钮
 * 在无有效条件时置灰。
 * @param spec 列逻辑索引（或 c_indexSortColumn）+ 方向，顺序即优先级
 * @return 成功 push 返回 true
 */
bool DATableSortController::applySpec(const QVector< QPair< int, Qt::SortOrder > >& spec)
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.sort.applySpecFromDialog", [ &spec ]() {
        return QStringLiteral("keys=%1").arg(spec.size());
    });
    QVector< QPair< int, Qt::SortOrder > > cleaned;
    cleaned.reserve(spec.size());
    for (const auto& k : spec) {
        if (k.first != c_indexSortColumn && (k.first < 0 || k.first >= mCtx.dataColumnCount())) {
            continue;  // 尾部预留空白列/越界列不入规格
        }
        bool duplicated = false;
        for (const auto& c : std::as_const(cleaned)) {
            if (c.first == k.first) {
                duplicated = true;
                break;
            }
        }
        if (!duplicated) {
            cleaned.append(k);
        }
    }
    if (cleaned.isEmpty()) {
        return false;
    }
    if (cleaned == mSortSpec) {
        return false;  // 规格未变：不重复推命令（数据顺序已是该规格）
    }
    return applySortSpec(cleaned);
}

/**
 * @brief 清除排序规格并刷新全部窗格角标（插删列/换数据集后列位置失效时用）
 */
void DATableSortController::clearSpec()
{
    mSortSpec.clear();
    refreshHeaders();
}

/**
 * @brief 刷新全部窗格表头的排序角标
 */
void DATableSortController::refreshHeaders()
{
    for (DADataTableView* pane : mCtx.panes()) {
        if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(pane->horizontalHeader())) {
            hh->setSortSpec(mSortSpec);
        }
    }
}

/**
 * @brief 排序动作统一入口（菜单驱动）：守卫 → 构造规格 → applySortSpec
 *
 * 规格构造规则（与原点击交互一致，入口改为菜单）：
 * - addKey=false：重置为「仅该列，指定方向」；
 * - addKey=true：该列不在规格中 → 追加（指定方向）；已在 → 切换其方向；
 * - 每次动作 = 一个撤销单元。
 * @param logicalIndex 列逻辑索引
 * @param order 排序方向
 * @param addKey 是否保留既有规格追加
 * @return 成功 push 返回 true
 */
bool DATableSortController::applySortAction(int logicalIndex, Qt::SortOrder order, bool addKey)
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.sort.action", [ this ]() {
        return QStringLiteral("keys=%1").arg(mSortSpec.size());
    });
    if (mCtx.isHeaderPickMode && mCtx.isHeaderPickMode()) {
        return false;  // 拾取会话中不排序（与点击互斥语义一致）
    }
    if (logicalIndex != c_indexSortColumn && (logicalIndex < 0 || logicalIndex >= mCtx.dataColumnCount())) {
        return false;  // 尾部预留空白列不排序（索引哨兵 -1 例外：它不对应任何列）
    }
    if (!mCtx.isDataSourceEditable()) {
        daWarning << tr("This data source is read-only and cannot be sorted.");  // cn:该数据源为只读，无法排序
        return false;
    }
    DAPyDataFrame df = mCtx.df();
    if (df.isNone() || mCtx.dataRowCount() <= 1) {
        return false;  // 非 dataframe 或无可排序的行
    }
    QVector< QPair< int, Qt::SortOrder > > newSpec = mSortSpec;
    if (addKey) {
        int idx = -1;
        for (int i = 0; i < newSpec.size(); ++i) {
            if (newSpec.at(i).first == logicalIndex) {
                idx = i;
                break;
            }
        }
        if (idx >= 0) {
            newSpec[ idx ].second = (newSpec.at(idx).second == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        } else {
            newSpec.append(qMakePair(logicalIndex, order));
        }
    } else {
        newSpec.clear();
        newSpec.append(qMakePair(logicalIndex, order));
    }
    return applySortSpec(newSpec);
}

/**
 * @brief 应用排序规格（构造命令 + 回调接线 + push）
 *
 * 命令生效后按设计稿 §4.1 顺序联动：先样式键重映射（directionalCallback 里
 * styleManager()->onRowsReordered(最近应用置换)）→ 再模型刷新（notifyDataChanged
 * 清块缓存 + 行头 index 变更通知）→ 最后按置换修正选区、视口回顶。
 * 排序路径计时走慢日志嵌套作用域（超阈值才写 slow.log，验收口径：10 万行 ≤300 ms）。
 * @param newSpec 新排序规格
 * @return 成功 push 返回 true；算子失败（如混合类型列）提示并保持原规格返回 false
 */
bool DATableSortController::applySortSpec(const QVector< QPair< int, Qt::SortOrder > >& newSpec)
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.sort.applySpec", [ this, &newSpec ]() {
        return QStringLiteral("rows=%1 keys=%2").arg(mCtx.dataRowCount()).arg(newSpec.size());
    });
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return false;
    }
    QVector< QPair< int, bool > > keys;
    keys.reserve(newSpec.size());
    for (const auto& k : newSpec) {
        keys.append(qMakePair(k.first, k.second == Qt::AscendingOrder));
    }
    std::unique_ptr< DACommandDataFrame_sortRows > cmd(new DACommandDataFrame_sortRows(df, keys));
    QPointer< DADataTableModel > modle          = mCtx.model();
    QPointer< DADataTableView > view             = mCtx.mainView();
    DACommandDataFrame_sortRows* cmdRaw          = cmd.get();
    DATableStyleManager* styleMgr               = mCtx.styleManager();
    DATableValidationStore* vaStore              = mCtx.validationStore();
    DATableNoteStore* noteStore                 = mCtx.noteStore();
    QPointer< DATableSortController > self(this);
    // ① 样式键重映射（先于模型刷新；exec/undo 均传最近应用的置换，undo 传逆置换即等价按逆置换重映射）
    cmd->setDirectionalCallBack([ styleMgr, vaStore, noteStore, cmdRaw ](bool isUndo) {
        Q_UNUSED(isUndo);
        if (styleMgr) {
            styleMgr->onRowsReordered(cmdRaw->lastAppliedPermutation());
        }
        // 批注与样式同源（005 T1.26）：同一置换重映射，「批注跟着行走」
        if (noteStore) {
            noteStore->onRowsReordered(cmdRaw->lastAppliedPermutation());
        }
        // 排序移动行位置 → 唯一性违规缓存的行号失效（005 T1.25），保守全失效
        if (vaStore) {
            vaStore->invalidateUniqueCache();
        }
    });
    // ② 模型刷新 → ③ 选区跟随 → 视口回顶（回调范式照 paste 的逐格 notifyDataChanged 写法）
    cmd->setCallBack([ modle, view, cmdRaw, self ]() {
        // 排序不改变列值集合 → 条件格式列统计不失效（003 性能决策）：
        // 统计失效挂点走 cellValueChanged 精确信号，排序路径不发该信号，天然不失效
        // 各阶段用嵌套慢日志作用域计时：只在超过各自阈值时写入 slow.log
        if (modle) {
            DA_SLOW_LOG_SCOPE("dagui.table.sort.notify");
            const int rows = modle->actualRowCount();
            const int cols = modle->columnCount();
            modle->notifyDataChanged(0, 0, qMax(0, rows - 1), qMax(0, cols - 1));
            // 行头显示 index 值，排序后 index 随行移动，需通知垂直表头重取
            modle->notifyHeaderDataChanged(Qt::Vertical, 0, qMax(0, rows - 1));
        }
        if (view) {
            {
                DA_SLOW_LOG_SCOPE("dagui.table.sort.remap");
                self->remapSelectionByPermutation(cmdRaw->lastAppliedPermutation());
            }
            DA_SLOW_LOG_SCOPE("dagui.table.sort.showRow");
            view->showActualRow(0);  // 视口回顶（行位置整体变了，含 undo/redo）
        }
        // 排序是破坏式重排：置工程未保存标记（exec/undo 都发，与 004 dataModified 口径一致；
        // 修复 002 遗留缺口——此前排序从不标脏，排序后关工程不提示保存）
        if (self && self->mCtx.notifyDataModified) {
            self->mCtx.notifyDataModified();
        }
    });
    // 排序是整表操作（大表算子+刷新可达秒级）：全程忙等待光标（RAII 恢复），避免 UI 假死观感
    DAWaitCursorScoped waitCursor;
    bool execOk = false;
    {
        DA_SLOW_LOG_SCOPE("dagui.table.sort.exec");
        execOk = cmd->exec();
    }
    if (!execOk) {
        // 算子报错（如 object 列混合类型不可比较）：排序规格回滚到变更前
        daWarning << tr("Cannot sort: the column contains values of incomparable mixed types.");  // cn:无法排序：该列包含不可比较的混合类型
        return false;
    }
    {
        DA_SLOW_LOG_SCOPE("dagui.table.sort.push");
        if (mCtx.pushCommand) {
            mCtx.pushCommand(cmd.release());
        }
    }
    mSortSpec = newSpec;
    refreshHeaders();
    return true;
}

/**
 * @brief 按最近应用的置换修正选区（选中行随排序跟随）
 *
 * appliedPerm 为新→旧置换（new[i]=old[appliedPerm[i]]），旧行号 old 的新位置 =
 * inv[old]（逆置换）。行数不变、滑动窗未平移，回调时视图选中的位置号即重排前的
 * 数据行号，直接按逆置换映射到新位置重建选区；映射出窗口的行跳过。
 *
 * **性能约束（2026-10-18 修复）**：禁止逐格重建——Qt 的「整列选择」本身就会产生
 * 每行一个 range 的选区（2 万行窗口 = 2 万个 range），逐格映射再 ClearAndSelect
 * 会让视图对上万散落 range 逐个做脏区合并，实测 3.2 万行表耗时 6~19 秒。
 * 这里的算法：先按列跨度 (left,right) 分组归并行区间，再分两条路径——
 * 1. 整窗快速路径：归并后覆盖全部模型行（典型=整列选择）→ 保持整列选中。
 *    置换是双射，「列仍被选中」与 Excel 排序后的视觉语义一致（滚出窗口的新位置
 *    本就不可见；换入窗口的行随列视为选中）；
 * 2. 一般路径：逐行映射 → 排序 → 游程压缩，每个连续段一个 range；总段数超过
 *    c_maxRemapSelRuns 时降级为只保留当前格映射（响应性优先，qDebug 说明）。
 * @param appliedPerm 最近应用的置换（exec=正置换 / undo=逆置换）
 */
void DATableSortController::remapSelectionByPermutation(const QVector< int >& appliedPerm)
{
    // 游程压缩后的 range 数上限：超过则视为「巨大散落选区」，降级为只映射当前格
    static constexpr int c_maxRemapSelRuns = 2048;
    DADataTableView* view          = mCtx.mainView();
    QItemSelectionModel* sm        = view ? view->selectionModel() : nullptr;
    QAbstractItemModel* m          = view ? view->model() : nullptr;
    if (!sm || !m || appliedPerm.isEmpty() || !sm->hasSelection()) {
        return;
    }
    const QVector< int > inv = DATableKeyRemap::inversePermutation(appliedPerm);
    const int modelRows      = m->rowCount();
    auto mapRow              = [ this, view, &inv ](int viewRow) -> int {
        const int oldActual = mCtx.toActualRow(viewRow);
        if (oldActual < 0 || oldActual >= inv.size() || inv.at(oldActual) < 0) {
            return -1;
        }
        return view->toLogicalRow(inv.at(oldActual));  // 目标行不在当前滑动窗内时返回 -1
    };
    // 按列跨度分组收集行区间（const 引用迭代，不触发 COW）
    const QItemSelection& oldSel = sm->selection();
    QMap< QPair< int, int >, QVector< QPair< int, int > > > spanRows;
    for (const QItemSelectionRange& r : oldSel) {
        spanRows[ qMakePair(r.left(), r.right()) ].append(qMakePair(r.top(), r.bottom()));
    }
    QItemSelection newSel;
    int totalRuns     = 0;
    bool fallback     = false;
    for (auto it = spanRows.constBegin(); it != spanRows.constEnd(); ++it) {
        const int left  = it.key().first;
        const int right = it.key().second;
        // 行区间排序 + 归并
        QVector< QPair< int, int > > iv = it.value();
        std::sort(iv.begin(), iv.end());
        QVector< QPair< int, int > > merged;
        merged.reserve(iv.size());
        for (int k = 0; k < iv.size(); ++k) {
            const QPair< int, int >& p = iv.at(k);
            if (!merged.isEmpty() && p.first <= merged.last().second + 1) {
                merged.last().second = qMax(merged.last().second, p.second);
            } else {
                merged.append(p);
            }
        }
        // 整窗快速路径：选区覆盖全部模型行（典型=整列选择）→ 保持整列
        if (modelRows > 0 && merged.size() == 1 && merged.at(0).first == 0 && merged.at(0).second == modelRows - 1) {
            newSel.select(m->index(0, left), m->index(modelRows - 1, right));
            continue;
        }
        // 一般路径：逐行映射（出窗跳过）→ 排序 → 游程压缩
        QVector< int > mapped;
        for (int k = 0; k < merged.size(); ++k) {
            for (int row = merged.at(k).first; row <= merged.at(k).second; ++row) {
                const int nr = mapRow(row);
                if (nr >= 0) {
                    mapped.append(nr);
                }
            }
        }
        std::sort(mapped.begin(), mapped.end());
        int i = 0;
        while (i < mapped.size()) {
            int j = i;
            while (j + 1 < mapped.size() && mapped.at(j + 1) == mapped.at(j) + 1) {
                ++j;
            }
            newSel.select(m->index(mapped.at(i), left), m->index(mapped.at(j), right));
            ++totalRuns;
            i = j + 1;
            if (totalRuns > c_maxRemapSelRuns) {
                fallback = true;
                break;
            }
        }
        if (fallback) {
            break;
        }
    }
    if (fallback) {
        // 巨大散落选区降级：放弃细粒度保留（重建上万散落段会让视图卡死数秒），只保留当前格映射
        newSel.clear();
        qDebug().noquote() << QStringLiteral("DATableSortController::remapSelectionByPermutation: "
                                             "selection too scattered (> %1 runs), only current index remapped")
                                  .arg(c_maxRemapSelRuns);
    }
    QModelIndex newCur;
    const QModelIndex cur = sm->currentIndex();
    if (cur.isValid()) {
        const int newLogical = mapRow(cur.row());
        if (newLogical >= 0) {
            newCur = m->index(newLogical, cur.column());
        }
    }
    sm->select(newSel, QItemSelectionModel::ClearAndSelect);
    if (newCur.isValid()) {
        sm->setCurrentIndex(newCur, QItemSelectionModel::NoUpdate);
    }
}
}  // end of namespace DA
