#include "Table/Core/DATableSelectionService.h"
// stl
#include <algorithm>
#include <climits>
// qt
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QSet>
// DAData
#include "DADataPyDataFrame.h"
// DAGui
#include "Table/Models/DADataTableModel.h"
#include "Table/Views/DADataTableView.h"
#include "Table/Views/DATablePaneContainer.h"
#include "Table/Metadata/DATableStyleManager.h"
#include "Table/Core/DATablePageContext.h"

namespace DA
{
namespace
{
// 一维闭区间的合并覆盖长度（「整列/整行是否全选」判定用）：排序后归并相邻/重叠段再累加，
// 对重叠选区段也不会重复计数。按值传入以便就地排序（段数 = 触及该列/行的选区段数，很小）
int mergedSpanCoverage(QVector< QPair< int, int > > spans)
{
    if (spans.isEmpty()) {
        return 0;
    }
    std::sort(spans.begin(), spans.end());
    int covered  = 0;
    int curStart = spans.first().first;
    int curEnd   = spans.first().second;
    for (int i = 1; i < spans.size(); ++i) {
        const QPair< int, int >& s = spans.at(i);
        if (s.first > curEnd + 1) {
            covered += curEnd - curStart + 1;
            curStart  = s.first;
            curEnd    = s.second;
        } else {
            curEnd = qMax(curEnd, s.second);
        }
    }
    covered += curEnd - curStart + 1;
    return covered;
}
}  // namespace

//===================================================
// DATableSelectionService
//===================================================

DATableSelectionService::DATableSelectionService(const DATablePageContext& ctx) : mCtx(ctx)
{
}

/**
 * @brief 表格是否有选中项
 * @return 有选区返回 true
 */
bool DATableSelectionService::haveSelection() const
{
    const QItemSelectionModel* sm = selectionModel();
    if (!sm) {
        return false;
    }
    return sm->hasSelection();
}

/**
 * @brief 返回选中的数据列（去重）
 *
 * 按选区区间展开列号（全选卡顿修复：不再 selectedIndexes 物化整个选区，
 * 全选时开销从 窗口行数×列数 降为 选中列数）。
 * @param ensureInDataframe true 时限定返回的列都在 dataframe 范围内
 * @return 列号列表
 */
QList< int > DATableSelectionService::selectedDataframeColumns(bool ensureInDataframe) const
{
    QList< int > res;
    QItemSelectionModel* selModel = selectionModel();
    if (!selModel) {
        return res;
    }
    int dfCols = -1;
    if (ensureInDataframe) {
        // 确保返回的列数都在dataframe里
        DAPyDataFrame df = mCtx.df();
        if (df.isNone()) {
            return res;
        }
        dfCols = static_cast< int >(df.shape().second);
    }
    QSet< int > colSet;
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        for (int c = r.left(); c <= r.right(); ++c) {
            if (dfCols >= 0 && c >= dfCols) {
                continue;
            }
            colSet.insert(c);
        }
    }
    return colSet.values();
}

/**
 * @brief 返回选中的数据行（数据源绝对行号，去重）
 *
 * 按选区区间展开行号（全选卡顿修复：不再 selectedIndexes 物化整个选区）。
 * 视图行号是滑动窗内的逻辑行，数据操作一律换算为数据源绝对行。
 * @param ensureInDataframe true 时限定返回的行都在 dataframe 范围内
 * @return 数据源绝对行号列表
 */
QList< int > DATableSelectionService::selectedDataframeRows(bool ensureInDataframe) const
{
    QList< int > res;
    QItemSelectionModel* selModel = selectionModel();
    if (!selModel) {
        return res;
    }
    const int rowCount = mCtx.dataRowCount();
    QSet< int > rowSet;
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        for (int vr = r.top(); vr <= r.bottom(); ++vr) {
            const int actualRow = mCtx.toActualRow(vr);
            if (ensureInDataframe && actualRow >= rowCount) {
                continue;
            }
            rowSet.insert(actualRow);
        }
    }
    return rowSet.values();
}

/**
 * @brief 返回完全选中的一整列
 *
 * 按选区区间做「逐列行区间合并」判定（全选卡顿修复：不用
 * QItemSelectionModel::selectedColumns——它对每列逐行判定，复杂度 O(列×行)，
 * 200 列×2 万行实测 55ms，宽表下线性放大到秒级；本实现是 O(段数×选中列数)）。
 * @param ensureInDataframe true 时限定返回的列都在 dataframe 范围内
 * @return 列号列表
 */
QList< int > DATableSelectionService::fullySelectedDataframeColumns(bool ensureInDataframe) const
{
    QList< int > res;
    QItemSelectionModel* selModel = selectionModel();
    if (!selModel) {
        return res;
    }
    QAbstractItemModel* m = mCtx.model();
    if (!m) {
        return res;
    }
    const int rowCount = m->rowCount();
    const int colCount = m->columnCount();
    if (rowCount <= 0 || colCount <= 0) {
        return res;
    }
    int colLimit = colCount;
    if (ensureInDataframe) {
        // 确保返回的列数都在dataframe里
        DAPyDataFrame df = mCtx.df();
        if (df.isNone()) {
            return res;
        }
        colLimit = qMin(colCount, static_cast< int >(df.shape().second));
    }
    // 逐列收集选中的行闭区间（只收 colLimit 内的列）
    QHash< int, QVector< QPair< int, int > > > colSpans;
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        for (int c = qMax(0, r.left()); c <= r.right() && c < colLimit; ++c) {
            colSpans[ c ].append(qMakePair(r.top(), r.bottom()));
        }
    }
    // 合并区间后覆盖满全部模型行 → 整列选中
    for (auto it = colSpans.constBegin(); it != colSpans.constEnd(); ++it) {
        if (mergedSpanCoverage(it.value()) >= rowCount) {
            res.append(it.key());
        }
    }
    return res;
}

/**
 * @brief 返回完全选中的一整行（数据源绝对行号）
 *
 * 同 fullySelectedDataframeColumns：按选区区间做「逐行列区间合并」判定，
 * 不用 QItemSelectionModel::selectedRows（它对每行逐列判定，复杂度 O(行×列)）。
 * @param ensureInDataframe true 时限定返回的行都在 dataframe 范围内
 * @return 数据源绝对行号列表
 */
QList< int > DATableSelectionService::fullySelectedDataframeRows(bool ensureInDataframe) const
{
    QList< int > res;
    QItemSelectionModel* selModel = selectionModel();
    if (!selModel) {
        return res;
    }
    QAbstractItemModel* m = mCtx.model();
    if (!m) {
        return res;
    }
    const int colCount = m->columnCount();
    if (colCount <= 0) {
        return res;
    }
    // 逐行（窗口内逻辑行）收集选中的列闭区间
    QHash< int, QVector< QPair< int, int > > > rowSpans;
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        for (int vr = r.top(); vr <= r.bottom(); ++vr) {
            rowSpans[ vr ].append(qMakePair(r.left(), r.right()));
        }
    }
    const int dataRows = mCtx.dataRowCount();
    for (auto it = rowSpans.constBegin(); it != rowSpans.constEnd(); ++it) {
        if (mergedSpanCoverage(it.value()) < colCount) {
            continue;
        }
        // 视图行号是滑动窗内的逻辑行，返回值一律用数据源绝对行
        const int actualRow = mCtx.toActualRow(it.key());
        if (ensureInDataframe && actualRow >= dataRows) {
            continue;
        }
        res.append(actualRow);
    }
    return res;
}

/**
 * @brief 选区首个数据行（数据源绝对行号）
 *
 * 区间遍历（全选卡顿修复：不再 selectedIndexes 物化整个选区）。
 * 旧实现按 selectedIndexes 的行主序取第一个满足过滤的行：区间内行号自顶向下
 * 递增、toActualRow 单调，故区间首行不满足过滤时整个区间都不满足，直接跳下一段，
 * 结果与逐格遍历逐位一致。
 * @param ensureInDataframe true 时限定返回的行都在 dataframe 范围内
 * @return 无选区返回 -1
 */
int DATableSelectionService::oneSelectedDataframeRow(bool ensureInDataframe) const
{
    QItemSelectionModel* selModel = selectionModel();
    if (!selModel) {
        return -1;
    }
    const int rowCount = ensureInDataframe ? mCtx.dataRowCount() : 0;
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        const int actualRow = mCtx.toActualRow(r.top());
        if (!ensureInDataframe || actualRow < rowCount) {
            return actualRow;
        }
    }
    return -1;
}

/**
 * @brief 选区首个数据列
 *
 * 区间遍历（全选卡顿修复：不再 selectedIndexes 物化整个选区）。
 * 旧实现按 selectedIndexes 的行主序取第一个满足过滤的列：区间内列号自左向右
 * 递增，故区间首列不满足过滤时整个区间都不满足，直接跳下一段，结果逐位一致。
 * @param ensureInDataframe true 时限定返回的列都在 dataframe 范围内
 * @return 无选区返回 -1
 */
int DATableSelectionService::oneSelectedDataframeColumn(bool ensureInDataframe) const
{
    QItemSelectionModel* selModel = selectionModel();
    if (!selModel) {
        return -1;
    }
    int dfCols = -1;
    if (ensureInDataframe) {
        // 确保返回的列数都在dataframe里
        DAPyDataFrame df = mCtx.df();
        if (df.isNone()) {
            return -1;
        }
        dfCols = static_cast< int >(df.shape().second);
    }
    const QItemSelection sel = selModel->selection();
    for (const QItemSelectionRange& r : sel) {
        if (dfCols < 0 || r.left() < dfCols) {
            return r.left();
        }
    }
    return -1;
}

/**
 * @brief 返回选中的单元格
 *
 * @param ensureInDataframe true 时限定返回的行列都在 dataframe 范围内
 * @return QPoint(数据源绝对行号, 列号) 列表
 */
QList< QPoint > DATableSelectionService::selectedDataframeCells(bool ensureInDataframe) const
{
    QList< QPoint > res;
    QItemSelectionModel* selModel = selectionModel();
    if (!selModel) {
        return res;
    }
    const int rowCount    = mCtx.dataRowCount();
    const int columnCount = mCtx.dataColumnCount();
    const QModelIndexList selindexs = selModel->selectedIndexes();
    for (const QModelIndex& index : selindexs) {
        const int actualRow = mCtx.toActualRow(index.row());
        if (ensureInDataframe && (actualRow >= rowCount || index.column() >= columnCount)) {
            continue;
        }
        res.append(QPoint(actualRow, index.column()));
    }
    return res;
}

/**
 * @brief 获取选中的序列数据
 *
 * 如果用户选中了一列，返回那一列的 pd.Series 作为数据；选中多列时每列作为
 * 一个 DAData 组成列表返回。选中单元格视作选中其所在列；无选区返回空列表。
 * @return DAData 列表
 */
QList< DAData > DATableSelectionService::selectedSeries() const
{
    QList< DAData > res;
    const QList< int > indexs = selectedDataframeColumns(true);
    if (indexs.isEmpty()) {
        return res;
    }
    DAPyDataFrame df = mCtx.df();
    auto shape       = df.shape();
    for (int i : indexs) {
        if (i < shape.second) {
            auto series = df[ i ];
            DAData d(series);
            res.append(d);
        }
    }
    return res;
}

/**
 * @brief 实际数据列的身份表
 *
 * 数据列序升序收集 (列名, occurrence)；尾部预留的空白列（列名为空）跳过。
 * occurrence 只在数据列区间内计（occurrence = 该列之前同名列的个数）。
 * @return 身份表（列视图键的统一口径）
 */
QList< QPair< QString, int > > DATableSelectionService::actualColumnIdentities() const
{
    QList< QPair< QString, int > > res;
    DADataTableModel* m = mCtx.model();
    if (!m) {
        return res;
    }
    QHash< QString, int > seen;  // 列名 -> 已出现次数（= 下一列的 occurrence）
    const int cols = mCtx.dataColumnCount();
    res.reserve(cols);
    for (int c = 0; c < cols; ++c) {
        const QString name = m->headerData(c, Qt::Horizontal).toString();
        if (name.isEmpty()) {
            continue;  // 空列名先判掉：不要让空名进 seen/occurrence 计数
        }
        const int occ = seen.value(name, 0);
        seen[ name ]  = occ + 1;
        res.append(qMakePair(name, occ));
    }
    return res;
}

/**
 * @brief 选择模型（选区快照 API，007 状态栏统计）
 *
 * 只取主窗格（冻结窗格共享同一个 selectionModel）。
 * @return 选择模型指针（无视图时 nullptr）
 */
QItemSelectionModel* DATableSelectionService::selectionModel() const
{
    DADataTableView* view = mCtx.mainView();
    return view ? view->selectionModel() : nullptr;
}

/**
 * @brief 选区段数
 *
 * 选区按连续段组织（Ctrl+多点形成多段）；0=无选区，>1=多段（状态栏只给几何计数）。
 * @return 段数
 */
int DATableSelectionService::selectionSegmentCount() const
{
    QItemSelectionModel* sm = selectionModel();
    return sm ? sm->selection().size() : 0;
}

/**
 * @brief 选区包围盒（闭区间，数据源绝对行号）
 *
 * 走 selection() 段列表（不要 selectedIndexes——会物化）。列维度没有滑动窗，
 * column() 即数据列序号。
 * @return 无选区返回 false
 */
bool DATableSelectionService::selectionBoundingBox(int* rowBegin, int* rowEnd, int* colBegin, int* colEnd) const
{
    QItemSelectionModel* sm = selectionModel();
    const QItemSelection sel = sm ? sm->selection() : QItemSelection();
    if (sel.isEmpty()) {
        return false;
    }
    int top = INT_MAX, bottom = -1, left = INT_MAX, right = -1;
    for (const QItemSelectionRange& r : sel) {
        top    = qMin(top, r.top());
        bottom = qMax(bottom, r.bottom());
        left   = qMin(left, r.left());
        right  = qMax(right, r.right());
    }
    if (rowBegin) {
        *rowBegin = mCtx.toActualRow(top);
    }
    if (rowEnd) {
        *rowEnd = mCtx.toActualRow(bottom);
    }
    if (colBegin) {
        *colBegin = left;
    }
    if (colEnd) {
        *colEnd = right;
    }
    return bottom >= 0;
}

/**
 * @brief 选中单元数（几何去重）
 *
 * 按选区区间累加宽×高（selection() 的区间互不重叠，无重复计数；
 * 全选卡顿修复：不再物化 selectedIndexes）。
 * @return 单元数
 */
int DATableSelectionService::selectedCellCount() const
{
    QItemSelectionModel* sm = selectionModel();
    if (!sm) {
        return 0;
    }
    qint64 count = 0;
    const QItemSelection sel = sm->selection();
    for (const QItemSelectionRange& r : sel) {
        count += qint64(r.width()) * qint64(r.height());
    }
    return static_cast< int >(qMin< qint64 >(count, INT_MAX));
}

/**
 * @brief 当前单元格（数据源绝对坐标）
 *
 * 视图行经 toActualRow 翻译成绝对行；越出数据范围（尾部预留空白行列）返回 false。
 * @param actualRow 非空时输出绝对行
 * @param actualCol 非空时输出列位置
 * @return 有有效当前格返回 true
 */
bool DATableSelectionService::currentActualCell(int* actualRow, int* actualCol) const
{
    QItemSelectionModel* sm = selectionModel();
    if (!sm) {
        return false;
    }
    const QModelIndex cur = sm->currentIndex();
    if (!cur.isValid()) {
        return false;
    }
    const int aRow = mCtx.toActualRow(cur.row());
    const int aCol = cur.column();
    if (aRow < 0 || aRow >= mCtx.dataRowCount() || aCol < 0 || aCol >= mCtx.dataColumnCount()) {
        return false;
    }
    if (actualRow) {
        *actualRow = aRow;
    }
    if (actualCol) {
        *actualCol = aCol;
    }
    return true;
}

/**
 * @brief 当前单元格模型索引
 *
 * selectionModel()->currentIndex()，无效时取选区左上角；都没有返回无效索引。
 * @return 模型索引
 */
QModelIndex DATableSelectionService::currentOrSelectionTopLeftIndex() const
{
    QItemSelectionModel* sm = selectionModel();
    if (!sm) {
        return QModelIndex();
    }
    const QModelIndex cur = sm->currentIndex();
    if (cur.isValid()) {
        return cur;
    }
    const QItemSelection sel = sm->selection();  // 段列表（不要 selectedIndexes 物化）
    if (sel.isEmpty()) {
        return QModelIndex();
    }
    return sel.first().topLeft();
}

/**
 * @brief 当前选区为单格时取其显示文本（D9 预填用）
 * @return 单格显示文本（Qt::DisplayRole）；选区非单格/无选区返回空串
 */
QString DATableSelectionService::currentSingleCellDisplayText() const
{
    QItemSelectionModel* sm = selectionModel();
    DADataTableModel* m    = mCtx.model();
    if (!sm || !m) {
        return QString();
    }
    const QItemSelection sel = sm->selection();
    if (sel.size() != 1) {
        return QString();
    }
    const QItemSelectionRange& r = sel.first();
    if (r.height() != 1 || r.width() != 1) {
        return QString();
    }
    const QModelIndex idx = m->index(r.top(), r.left());
    return idx.isValid() ? idx.data(Qt::DisplayRole).toString() : QString();
}

/**
 * @brief 有序选区：按视图顺序返回选区涉及的数据列（数据列号）
 *
 * selectedDataframeColumns 走 QSet::values()，顺序不保证；快速绘图等需要
 * 「第 1 列当 X、第 2 列当 Y」这类稳定语义的入口按窗格容器的可见列顺序重排，
 * 隐藏列按视图状态跳过（实施方案 §8.1）。
 * @param ensureInDataframe 是否要求列号落在 dataframe 范围内（惰性源传 false）
 */
QList< int > DATableSelectionService::selectedDataColumnsInViewOrder(bool ensureInDataframe) const
{
    QList< int > selected = selectedDataframeColumns(ensureInDataframe);
    if (selected.isEmpty() && ensureInDataframe) {
        // 非 pandas 数据源（惰性表）在 ensureInDataframe=true 时取不到，回退一次
        selected = selectedDataframeColumns(false);
    }
    if (selected.isEmpty()) {
        return {};
    }
    QList< int > res;
    if (DATablePaneContainer* pc = mCtx.paneContainer()) {
        const QVector< int > order = pc->visibleColumnOrderLogical();
        const QSet< int > selectedSet(selected.begin(), selected.end());
        res.reserve(order.size());
        for (int col : order) {
            if (selectedSet.contains(col)) {
                res.append(col);
            }
        }
        return res;
    }
    // 没有窗格容器（极端场景）：退回数据列升序，至少保证顺序稳定
    std::sort(selected.begin(), selected.end());
    return selected;
}

/**
 * @brief 按选中列复制/导出的列范围解析
 *
 * 有选中列 → 视图顺序返回（所见即所得：导出列序与用户看到的列序一致）；
 * 无选中 → 回退 fallbackColumn（菜单项文案虽写「选中列」，右键在某列表头上
 * 本身就是明确的指向，Excel 同款语义）；两者皆无有效列返回空列表（调用方提示）。
 * @param fallbackColumn 无选中时回退的列位置
 */
QList< int > DATableSelectionService::selectedColumnsForDataAction(int fallbackColumn) const
{
    QList< int > columns = selectedDataColumnsInViewOrder(false);
    if (columns.isEmpty() && fallbackColumn >= 0 && fallbackColumn < mCtx.dataColumnCount()) {
        columns.append(fallbackColumn);
    }
    return columns;
}

/**
 * @brief 选区按列分组的连续行区间
 *
 * 取选中单元格（selectedDataframeCells 返回 QPoint(绝对行, 列)，已确保在 dataframe 内），
 * 按列聚合后各列的选中行升序，再按连续性切成若干 [rowStart,rowEnd] 闭区间（run）。
 * 不规则多选（Ctrl 点选的非连续行）会切成多个 run，只填选中的行、不碰未选中的行——
 * 与 Excel「填充作用于每个连续区域」一致。尾部预留空白列/行不计（ensureInDataframe）。
 * @return col -> 连续行区间列表（升序）；无选区返回空
 */
QHash< int, QVector< QPair< int, int > > > DATableSelectionService::selectedColumnRowRuns() const
{
    QHash< int, QVector< QPair< int, int > > > res;
    const QList< QPoint > cells = selectedDataframeCells();
    if (cells.isEmpty()) {
        return res;
    }
    // col -> 升序去重行
    QHash< int, QList< int > > colRows;
    for (const QPoint& p : cells) {
        colRows[ p.y() ].append(p.x());
    }
    for (auto it = colRows.constBegin(); it != colRows.constEnd(); ++it) {
        QList< int > rows = it.value();
        std::sort(rows.begin(), rows.end());
        rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
        QVector< QPair< int, int > > runs;
        int runStart = rows.first();
        int prev     = rows.first();
        for (int i = 1; i < rows.size(); ++i) {
            if (rows[ i ] == prev + 1) {
                prev = rows[ i ];
                continue;
            }
            runs.append(qMakePair(runStart, prev));
            runStart = rows[ i ];
            prev     = rows[ i ];
        }
        runs.append(qMakePair(runStart, prev));
        res.insert(it.key(), runs);
    }
    return res;
}

/**
 * @brief 选区是否与行级样式相交（稀疏键表判定，不展开选区）
 *
 * 样式键是数据源绝对行号，减窗口起始行换算回窗口内逻辑行后与区间比对；
 * 全 invalid 的空样式不参与叠加（mergeFrom 只覆盖 valid 属性），视同不相交。
 * @param sel 选区区间列表
 * @param windowStart 当前窗口起始行
 * @return 相交返回 true
 */
bool DATableSelectionService::selectionIntersectsRowStyles(const QItemSelection& sel, int windowStart) const
{
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr) {
        return false;
    }
    const QList< int > styledRows = styleMgr->styledRows();
    for (int actualRow : styledRows) {
        if (styleMgr->getRowStyle(actualRow).isNull()) {
            continue;
        }
        const int viewRow = actualRow - windowStart;
        for (const QItemSelectionRange& r : sel) {
            if (viewRow >= r.top() && viewRow <= r.bottom()) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief 选区是否与单元格级样式相交（稀疏键表判定，不展开选区）
 * @param sel 选区区间列表
 * @param windowStart 当前窗口起始行
 * @return 相交返回 true
 */
bool DATableSelectionService::selectionIntersectsCellStyles(const QItemSelection& sel, int windowStart) const
{
    DATableStyleManager* styleMgr = mCtx.styleManager();
    if (!styleMgr) {
        return false;
    }
    const QList< QPair< int, int > > styledCells = styleMgr->styledCells();
    for (const QPair< int, int >& key : styledCells) {
        if (styleMgr->getCellStyle(key.first, key.second).isNull()) {
            continue;
        }
        const int viewRow = key.first - windowStart;
        for (const QItemSelectionRange& r : sel) {
            if (viewRow >= r.top() && viewRow <= r.bottom() && key.second >= r.left() && key.second <= r.right()) {
                return true;
            }
        }
    }
    return false;
}
}  // end of namespace DA
