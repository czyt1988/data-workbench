#include "Table/Controllers/DATableViewStateController.h"
#include "Table/Core/DATablePageContext.h"
#include "Table/Core/DATableSelectionService.h"
// qt
#include <QGuiApplication>
#include <QHash>
#include <QHeaderView>
#include <QSet>
// DAGui
#include "Table/Views/DADataTableView.h"
#include "Table/Models/DADataTableModel.h"
#include "Table/Views/DATablePaneContainer.h"
#include "Table/Metadata/DATableViewStateStore.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

namespace DA
{
//===================================================
// DATableViewStateController
//===================================================

DATableViewStateController::DATableViewStateController(const DATablePageContext& ctx,
                                                       const DATableSelectionService& sel,
                                                       QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel)
{
    // 逐行行高覆盖的任何变更（单行拖动/插删行/排序/清空）→ 差异重投影。
    // store 是会话级对象、比页面长寿，连接随本对象析构自动断开。
    if (DATableViewStateStore* vsStore = mCtx.viewStateStore()) {
        connect(vsStore,
                &DATableViewStateStore::rowHeightsChanged,
                this,
                &DATableViewStateController::applyRowHeightOverridesToPanes);
    }
}

/**
 * @brief 从视图状态子存储投影到窗格（唯一投影路径，006b/006e）
 *
 * 列宽按「列名 + 第几次出现（occurrence）」匹配逻辑列号后 resizeSection——
 * 按列名匹配保证插列/删列后宽度不错位（R6/R20）。应用期间置守卫标志，
 * 屏蔽 sectionResized 采集，避免把「应用」当「用户改动」写回 store。
 * 行头宽用 setFixedWidth 落地（Qt 纵向表头宽度=clamp(qMax(min,sizeHint),max)，
 * min=max=w 恒为 w，不会被视图布局覆盖）。
 * 首次调用先做一次加载收敛（normalizeToColumns：孤儿剪除 + 新列追加列序末尾），
 * 有丢弃时发一条用户可见提示；列序/隐藏/冻结的投影由容器落地。
 */
void DATableViewStateController::applyFromStore()
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    DADataTableModel* model        = mCtx.model();
    if (!vsStore || !model) {
        return;
    }
    // 加载收敛（只做一次）：把已被删除的列从视图状态里剪掉、把新列追加到列序末尾。
    // 释放后不再收敛（E6：不在列插删时收敛，避免与"删除列"的 Ctrl+Z 互踩）
    if (!vsStore->isColumnSetNormalized() && mCtx.dataColumnCount() > 0) {
        if (vsStore->normalizeToColumns(mSel.actualColumnIdentities())) {
            daWarning << tr("Some column view settings were dropped because the table columns have changed.");
            // cn:表格的列已变化，部分列视图设置已被丢弃
        }
    }
    mApplyingViewState = true;
    // 1) 列宽：对每个窗格按列名（+occurrence）套用（Qt 按段存宽度，隐藏段写进
    //    hiddenSectionSize、显示时恢复，安全）
    for (DADataTableView* pane : mCtx.panes()) {
        applyColumnWidthsToPane(pane);
        // store 无记录的列 → 内容自适应（冻结区宽随列宽改造：默认 100px 的冻结列
        // 曾让冻结区固定偏宽；自适应只补空白，不动已记录列）
        autoFitUnrecordedColumnWidths(pane);
    }
    // 2) 行头宽：设给行头归属方（冻结时归属冻结窗格，否则主窗格）
    applyRowHeaderWidthToOwner();
    // 2.5) 默认行高 / 逐行覆盖 / 水平表头高：默认值套到各窗格后，
    //      再把「绝对行号 → 高度」的覆盖落到当前窗口内的段（搬窗时由窗格信号重投影）
    if (const int rowH = vsStore->rowHeight(); rowH > 0) {
        applyRowHeightToPanes(rowH);
    } else {
        applyRowHeightOverridesToPanes();
    }
    if (const int headH = vsStore->horizontalHeaderHeight(); headH > 0) {
        applyHeaderHeightToPanes(headH);
    }
    // 3) 列序 / 隐藏 / 冻结 → 容器投影（冻结列可来自列序任意位置；冻结不改变列序）
    const QList< QPair< QString, int > > actual       = mSel.actualColumnIdentities();
    // 全局列序身份表（store 的 columnOrder 精确匹配实际列，未匹配的按数据列序追加末尾）
    const QList< QPair< QString, int > > storedOrder  = [ this, &actual ]() {
        QList< QPair< QString, int > > res;
        res.reserve(actual.size());
        if (DATableViewStateStore* vs = mCtx.viewStateStore()) {
            QSet< QPair< QString, int > > used;
            const QList< QPair< QString, int > > order = vs->columnOrder();
            for (const QPair< QString, int >& ident : order) {
                if (actual.contains(ident) && !used.contains(ident)) {
                    res.append(ident);
                    used.insert(ident);
                }
            }
            for (const QPair< QString, int >& ident : actual) {
                if (!used.contains(ident)) {
                    res.append(ident);
                }
            }
            return res;
        }
        return actual;
    }();
    const QList< QPair< QString, int > > storedHidden = vsStore->hiddenColumns();
    const QList< QPair< QString, int > > storedFrozen = vsStore->frozenColumns();  // 冻结区顺序
    QList< int > actualLogical;                            // 身份 → logical：与 actualColumnIdentities() 同口径
    QHash< QPair< QString, int >, int > logicalOfIdent;    // 重扫列头（跳过空列名），与 actual 逐条对齐
    {
        QHash< QString, int > seen;
        const int cols = mCtx.dataColumnCount();
        actualLogical.reserve(cols);
        for (int c = 0; c < cols; ++c) {
            const QString name = model->headerData(c, Qt::Horizontal).toString();
            if (name.isEmpty()) {
                continue;
            }
            const int occ = seen.value(name, 0);
            seen[ name ]  = occ + 1;
            actualLogical.append(c);
            logicalOfIdent.insert(qMakePair(name, occ), c);
        }
        Q_ASSERT(actualLogical.size() == actual.size());
    }
    QVector< int > ordered;  // store 列序里能匹配的列 ++ 未匹配的实际列（数据列序追加末尾）
    QSet< int > used;
    ordered.reserve(actualLogical.size());
    for (const QPair< QString, int >& ident : storedOrder) {
        const auto it = logicalOfIdent.constFind(ident);
        if (it == logicalOfIdent.constEnd() || used.contains(it.value())) {
            continue;  // 孤儿由加载收敛剪掉，这里只跳过（防御）
        }
        ordered.append(it.value());
        used.insert(it.value());
    }
    for (const int logical : actualLogical) {
        if (!used.contains(logical)) {
            ordered.append(logical);
        }
    }
    QSet< int > hidden;  // store 隐藏集合按身份匹配成 logical 集合（孤儿由加载收敛剪掉）
    for (const QPair< QString, int >& ident : storedHidden) {
        const auto it = logicalOfIdent.constFind(ident);
        if (it != logicalOfIdent.constEnd()) {
            hidden.insert(it.value());
        }
    }
    // 冻结列按身份匹配成 logical 有序列表（顺序 = 冻结区显示顺序；孤儿由加载收敛剪掉）；
    // 集合缓存供 isColumnInFrozenArea 纯查表，唯一写入时机在这里
    QVector< int > frozenOrder;
    frozenOrder.reserve(storedFrozen.size());
    mFrozenColumnsLogical.clear();
    for (const QPair< QString, int >& ident : storedFrozen) {
        const auto it = logicalOfIdent.constFind(ident);
        if (it != logicalOfIdent.constEnd() && !mFrozenColumnsLogical.contains(it.value())) {
            frozenOrder.append(it.value());
            mFrozenColumnsLogical.insert(it.value());
        }
    }
    if (DATablePaneContainer* pc = mCtx.paneContainer()) {
        pc->applyColumnState(ordered, hidden, frozenOrder);
    }
    mApplyingViewState = false;
    // 4) 投影落地后再补一轮内容自适应：懒建的冻结窗格在 frozenViewCreated 时刻
    //    视口往往还没行（列刚被投影隐藏/显示），此时自适应量不到单元格；这里
    //    窗格已布局，采样才有效。已自适应过的列此轮宽非默认值但仍无 store 记录，
    //    重复自适应幂等（同一批可见文本宽度不变）
    for (DADataTableView* pane : mCtx.panes()) {
        autoFitUnrecordedColumnWidths(pane);
    }
}

/**
 * @brief 目标列是否落在冻结区
 *
 * 纯查表（mFrozenColumnsLogical 在 applyFromStore 尾部与投影同一次写入）；
 * 007 不得自己重算这份语义（真源唯一）。
 * @param logicalColumn 逻辑列号
 * @return 是否在冻结区
 */
bool DATableViewStateController::isColumnInFrozenArea(int logicalColumn) const
{
    return mFrozenColumnsLogical.contains(logicalColumn);
}

/**
 * @brief 采集列宽改动写入视图状态子存储
 *
 * 只记录用户拖拽：左键按住且鼠标在表头上；应用守卫期间、程序化/布局触发的
 * sectionResized（模型重置、窗口缩放拉伸末列等）一律不记录。尾部预留空白列
 * 与空列名不记录。occurrence = 该列之前同名列的个数（重名列消歧，R20）。
 * source = 触发信号的那个窗格（006b 起不写死主窗格）。
 * @param source 触发窗格
 * @param logicalIndex 逻辑列号
 * @param newSize 新宽度（像素）
 */
void DATableViewStateController::onHeaderSectionResized(DADataTableView* source, int logicalIndex, int newSize)
{
    if (mApplyingViewState || !source) {
        return;
    }
    QHeaderView* hh = source->horizontalHeader();
    if (!hh || !(QGuiApplication::mouseButtons() & Qt::LeftButton) || !hh->underMouse()) {
        return;  // 非用户拖拽
    }
    storeColumnWidthAt(logicalIndex, newSize);
    // 冻结列宽变化 → 重算冻结窗格宽度（006e；非冻结列宽只影响主区，重算幂等无害）
    if (DATablePaneContainer* pc = mCtx.paneContainer()) {
        pc->updateFrozenGeometry();
    }
}

/**
 * @brief 按 (名, occurrence) 写一条列宽进 store
 *
 * 分界条拖动/双击自适应的公共落库段：与 onHeaderSectionResized 同键口径
 * （列名 + 数据列序 occurrence），列号越界/空名静默忽略。
 * @param logicalColumn 逻辑列号
 * @param width 宽度（像素）
 */
void DATableViewStateController::storeColumnWidthAt(int logicalColumn, int width)
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    DADataTableModel* model        = mCtx.model();
    if (!vsStore || !model || logicalColumn < 0 || logicalColumn >= mCtx.dataColumnCount() || width <= 0) {
        return;
    }
    const QString name = model->headerData(logicalColumn, Qt::Horizontal).toString();
    if (name.isEmpty()) {
        return;
    }
    int occ = 0;
    for (int c = 0; c < logicalColumn; ++c) {
        if (model->headerData(c, Qt::Horizontal).toString() == name) {
            ++occ;
        }
    }
    vsStore->setColumnWidth(name, occ, width);
}

/**
 * @brief 指定列的内容自适应宽度
 *
 * = max(表头文本宽 + 表头边距, 视口可见单元格文本宽) + 两侧边距，钳制
 * [min, max]：min = 视觉可接受的最小数据列宽（略宽于表头最小段宽 25px），
 * max = 防长文本把列撑爆的上限（Excel 同款做法：自适应有上限，超长靠截断省略）。
 * 单元格文本只取当前视口可见行（visibleRegion 采样），首屏足够代表性且不卡大表。
 * @param pane 目标窗格（取视口与水平表头）
 * @param logicalColumn 逻辑列号
 * @return 自适应宽度（像素）；列号无效返回 -1
 */
int DATableViewStateController::contentFittedColumnWidth(DADataTableView* pane, int logicalColumn) const
{
    DADataTableModel* model = mCtx.model();
    if (!pane || !model || !pane->horizontalHeader()) {
        return -1;
    }
    QHeaderView* hh = pane->horizontalHeader();
    if (logicalColumn < 0 || logicalColumn >= model->columnCount()) {
        return -1;
    }
    constexpr int c_minAutoWidth  = 40;  // 最小数据列宽（px）
    constexpr int c_maxAutoWidth  = 400;  // 自适应上限（px，长文本截断显示）
    constexpr int c_cellHMargin   = 14;  // 单元格文本两侧边距（含 selection 高亮内边距）
    constexpr int c_headerHMargin = 12;  // 表头文本两侧边距（排序角标预留）
    int textWidth = hh->fontMetrics().horizontalAdvance(
        model->headerData(logicalColumn, Qt::Horizontal, Qt::DisplayRole).toString())
        + c_headerHMargin;
    // 只采样视口可见行：先由视口矩形换算出首/末可见行，再逐行取样。
    // 曾按"全模型遍历 + visualRect 过滤"实现，20000 行 × 200 列 = 400 万次
    // index()+visualRect()，打开宽表会冻结数十秒到数分钟（testSelectAllLargeSelectionPerf
    // 就是被这一步拖过 Qt Test 的 300s 看门狗；dev 6be9b15b 同款修复随迁）。
    if (pane->viewport()) {
        const QRect viewportRect   = pane->viewport()->rect();
        const int firstVisibleRow = pane->rowAt(viewportRect.top());
        const int lastVisibleRow  = pane->rowAt(viewportRect.bottom());
        for (int r = qMax(0, firstVisibleRow); r <= lastVisibleRow; ++r) {
            textWidth = qMax(textWidth,
                            hh->fontMetrics().horizontalAdvance(
                                model->data(model->index(r, logicalColumn), Qt::DisplayRole).toString())
                                + c_cellHMargin);
        }
    }
    return qBound(c_minAutoWidth, textWidth, c_maxAutoWidth);
}

/**
 * @brief 把 store 里的列宽套到指定窗格（列宽段唯一实现）
 *
 * 按「列名 + occurrence」匹配逻辑列号后 resizeSection；尾部预留空白列跳过。
 * 主窗格与冻结窗格共用（006e）。
 * @param pane 目标窗格
 */
void DATableViewStateController::applyColumnWidthsToPane(DADataTableView* pane)
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    DADataTableModel* model         = mCtx.model();
    if (!pane || !vsStore || !model) {
        return;
    }
    QHeaderView* hh = pane->horizontalHeader();
    if (!hh) {
        return;
    }
    const int totalCols = model->columnCount();
    QHash< QString, int > seen;  // 列名 -> 已出现次数（即下一列的 occurrence）
    for (int c = 0; c < totalCols; ++c) {
        const QString name = model->headerData(c, Qt::Horizontal).toString();
        const int occ      = seen.value(name, 0);
        seen[ name ]       = occ + 1;
        if (name.isEmpty()) {
            continue;  // 尾部预留空白列无列名，不应用
        }
        const int w = vsStore->columnWidth(name, occ);
        if (w > 0) {
            hh->resizeSection(c, w);
        }
    }
}

/**
 * @brief store 无记录的列做内容自适应列宽
 *
 * 「冻结区过宽」根因修复：从未动过列宽的列保持 Qt 默认 100px，冻结窗格宽 =
 * Σ(冻结列宽) + 行头宽 + 2×frame + 2px 余量，一列冻结也显得极宽。这里对 store
 * 无记录的数据列取内容自适应宽（表头文本 + 视口采样单元格文本），冻结与主区
 * 两窗格都套（同一列两边宽度一致，冻结列宽直接决定冻结区宽）。
 * 视口首屏采样（visibleRegion 内的行），不遍历全模型——滑动窗默认 20000 行，
 * QHeaderView::resizeContentsPrecision 全遍历会卡大表。尾部预留空白列跳过。
 * @param pane 目标窗格
 */
void DATableViewStateController::autoFitUnrecordedColumnWidths(DADataTableView* pane)
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    DADataTableModel* model         = mCtx.model();
    if (!pane || !vsStore || !model) {
        return;
    }
    QHeaderView* hh = pane->horizontalHeader();
    if (!hh) {
        return;
    }
    const int totalCols = model->columnCount();
    QHash< QString, int > seen;  // 列名 -> 已出现次数（即下一列的 occurrence）
    for (int c = 0; c < totalCols; ++c) {
        const QString name = model->headerData(c, Qt::Horizontal).toString();
        const int occ      = seen.value(name, 0);
        seen[ name ]       = occ + 1;
        if (name.isEmpty()) {
            continue;  // 尾部预留空白列不参与
        }
        if (vsStore->hasColumnWidth(name, occ)) {
            continue;  // 已有记录（用户拖过/工程加载），不动
        }
        const int w = contentFittedColumnWidth(pane, c);
        if (w > 0) {
            hh->resizeSection(c, w);
        }
    }
}

/**
 * @brief 行头宽只设给归属方
 *
 * 冻结时行头归属冻结窗格（[行号][冻结列][主区]），否则归属主窗格。
 * 只对归属方 setFixedWidth，不给非归属方的隐藏行头写宽度（切回时留脏值）。
 */
void DATableViewStateController::applyRowHeaderWidthToOwner()
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    if (!vsStore) {
        return;
    }
    const int rhw = vsStore->rowHeaderWidth();
    if (rhw <= 0) {
        return;
    }
    QHeaderView* owner = mCtx.mainView() ? mCtx.mainView()->verticalHeader() : nullptr;
    if (DATablePaneContainer* pc = mCtx.paneContainer()) {
        if (DADataTableView* frozen = pc->frozenView()) {
            owner = frozen->verticalHeader();
        }
    }
    if (owner) {
        owner->setFixedWidth(rhw);
    }
}

/**
 * @brief 默认行高套到全部窗格垂直表头
 *
 * 整表统一行高（对话框/右键/紧凑/普通）：默认行高 + 全部可见段一并设齐，
 * 滚动换入的新段随默认行高保持一致。段被统一成默认值后，之前的逐行覆盖记录
 * 全部失效，末尾按 store 重新套用仍然存在的覆盖（加载回填路径）。
 * @param height 行高（像素，<=0 忽略）
 */
void DATableViewStateController::applyRowHeightToPanes(int height)
{
    if (height <= 0) {
        return;
    }
    for (DADataTableView* pane : mCtx.panes()) {
        if (!pane) {
            continue;
        }
        if (QHeaderView* vh = pane->verticalHeader()) {
            vh->setDefaultSectionSize(height);
            const int n = vh->count();
            for (int i = 0; i < n; ++i) {
                if (!vh->isSectionHidden(i)) {
                    vh->resizeSection(i, height);
                }
            }
        }
    }
    mAppliedRowHeights.clear();
    applyRowHeightOverridesToPanes();
}

/**
 * @brief 逐行行高覆盖套到全部窗格
 *
 * 差异增量实现：只遍历「已应用段」与「store 覆盖」两张小表，
 * 不遍历滑动窗 20000 段。窗口平移/结构变化后由窗格信号单窗格调用。
 */
void DATableViewStateController::applyRowHeightOverridesToPanes()
{
    const QList< DADataTableView* > panes = mCtx.panes();
    // 清理已销毁窗格的残留键（指针值不 dereference，但避免地址复用误判）
    for (auto it = mAppliedRowHeights.begin(); it != mAppliedRowHeights.end();) {
        if (!panes.contains(it.key())) {
            it = mAppliedRowHeights.erase(it);
        } else {
            ++it;
        }
    }
    for (DADataTableView* pane : panes) {
        applyRowHeightOverridesToPane(pane);
    }
}

/**
 * @brief 逐行行高覆盖套到指定窗格（差异增量）
 *
 * 段号是「窗口内行号」，覆盖按「数据源绝对行号」存：窗口平移后同一段代表的行变了，
 * 所以先按 store 校验已应用记录（不再匹配的恢复默认），再把落在当前窗口内的覆盖
 * 重新设到对应段。复杂度 O(覆盖数)，与窗口大小无关。
 * @param pane 目标窗格
 */
void DATableViewStateController::applyRowHeightOverridesToPane(DADataTableView* pane)
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    if (!pane || !vsStore) {
        return;
    }
    QHeaderView* vh = pane->verticalHeader();
    if (!vh) {
        return;
    }
    const int sectionCount = vh->count();
    // 恢复默认值：优先 store 记录的默认行高，否则用表头当前默认
    const int storedDefault = vsStore->rowHeight();
    const int fallback      = (storedDefault > 0) ? storedDefault : vh->defaultSectionSize();
    if (storedDefault > 0 && vh->defaultSectionSize() != storedDefault) {
        vh->setDefaultSectionSize(storedDefault);
    }
    QHash< int, int >& applied = mAppliedRowHeights[ pane ];
    // 1) 已应用段：对应绝对行不再有覆盖（或高度变了）→ 恢复默认
    for (auto it = applied.begin(); it != applied.end();) {
        const int logical = it.key();
        const int actual  = pane->toActualRow(logical);
        const int want    = vsStore->rowHeightOverride(actual);
        if (logical < 0 || logical >= sectionCount || want != it.value()) {
            if (logical >= 0 && logical < sectionCount && !vh->isSectionHidden(logical)) {
                vh->resizeSection(logical, fallback);
            }
            it = applied.erase(it);
        } else {
            ++it;
        }
    }
    // 2) store 覆盖落到当前窗口内的段
    const QHash< int, int > overrides = vsStore->rowHeightOverrides();
    for (auto it = overrides.constBegin(); it != overrides.constEnd(); ++it) {
        const int logical = pane->toLogicalRow(it.key());
        if (logical < 0 || logical >= sectionCount || vh->isSectionHidden(logical)) {
            continue;
        }
        if (vh->sectionSize(logical) != it.value()) {
            vh->resizeSection(logical, it.value());
        }
        applied.insert(logical, it.value());
    }
}

/**
 * @brief 水平表头高套到全部窗格（setFixedHeight 落地，视图布局不覆盖）
 * @param height 表头高（像素，<=0 忽略）
 */
void DATableViewStateController::applyHeaderHeightToPanes(int height)
{
    if (height <= 0) {
        return;
    }
    for (DADataTableView* pane : mCtx.panes()) {
        if (pane && pane->horizontalHeader()) {
            pane->horizontalHeader()->setFixedHeight(height);
        }
    }
}
}  // end of namespace DA
