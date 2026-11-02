#include "DATableFindController.h"
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"
#include "TableCore/DATableMutationCoordinator.h"
// qt
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QWidget>
// DAGui
#include "Models/DADataTableModel.h"
#include "Commands/DACommandsDataFrame.h"
#include "DATableHighlightOverlay.h"
#include "DATableConditionalFormatStore.h"
// python 算子封装（查找/替换）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "DAPybind11QtCaster.hpp"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"
// 忙等待光标（扫描/替换）
#include "DAWaitCursorScoped.h"
// 性能埋点（不进界面消息队列，内容纯英文）
#include "DASlowLog.h"

#include <climits>
#include <memory>

namespace DA
{
namespace
{
// 替换撤销阈值（格）：变更格数超过此值降级为「不可撤销 + 确认提示」（设计稿 §4.11 策略表，默认 100 万格）
constexpr int c_replaceUndoCellLimit = 1000000;
}

//===================================================
// DATableFindController
//===================================================

DATableFindController::DATableFindController(const DATablePageContext& ctx,
                                             const DATableSelectionService& sel,
                                             DATableMutationCoordinator* mutation,
                                             QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel), mMutation(mutation)
{
}

/**
 * @brief 打开查找/替换浮动条（懒建，非模态，页级持有）
 * @param expandReplace true 展开替换行（Ctrl+H），false 仅查找（Ctrl+F）
 */
void DATableFindController::showFindBar(bool expandReplace, bool prefillFromCurrentCell)
{
    if (!mFindDialog) {
        mFindDialog = new DADialogFindReplace(mCtx.hostWidget());
        connect(mFindDialog, &DADialogFindReplace::searchRequested, this, &DATableFindController::onSearchRequested);
        connect(mFindDialog, &DADialogFindReplace::nextRequested, this, &DATableFindController::onNextRequested);
        connect(mFindDialog, &DADialogFindReplace::previousRequested, this, &DATableFindController::onPrevRequested);
        connect(mFindDialog, &DADialogFindReplace::replaceRequested, this, &DATableFindController::onReplaceRequested);
        connect(mFindDialog, &DADialogFindReplace::replaceAllRequested, this, &DATableFindController::onReplaceAllRequested);
        connect(mFindDialog, &DADialogFindReplace::closed, this, &DATableFindController::onDialogClosed);
    }
    // 右键菜单入口：查找框预填当前单格显示文本（省得用户把选中内容再敲一遍，D9）；
    // 快捷键入口保持空框/旧词。预填经 textChanged 走去抖扫描，activate 里 selectAll 便于直接改写
    if (prefillFromCurrentCell) {
        const QString cellText = mSel.currentSingleCellDisplayText();
        if (!cellText.isEmpty()) {
            mFindDialog->setFindText(cellText);
        }
    }
    mFindDialog->activate(expandReplace);
    updateCountLabel();
}

/**
 * @brief 关闭查找/替换浮动条（清高亮经 closed → onDialogClosed）
 */
void DATableFindController::closeFindBar()
{
    if (mFindDialog && mFindDialog->isVisible()) {
        mFindDialog->close();
    }
}

int DATableFindController::hitCount() const
{
    return mHits.size();
}

int DATableFindController::currentHitIndex() const
{
    return mHitIndex;
}

/**
 * @brief 选区变化时重启去抖重扫（页面选区变化信号接线用）
 *
 * 查找条处于「当前选区」范围模式且可见时，选区变化经去抖触发重扫
 * （范围基准随选区更新）；其它情况无操作。
 */
void DATableFindController::restartDebounceIfVisible()
{
    if (mFindDialog && mFindDialog->isVisible() && mFindDialog->options().selectionOnly) {
        mFindDialog->restartDebounce();
    }
}

void DATableFindController::onSearchRequested()
{
    runScan();
}

void DATableFindController::onPrevRequested()
{
    if (mHits.isEmpty()) {
        // 无命中（如刚打开未扫描）：先扫一次再导航
        runScan();
        if (mHits.isEmpty()) {
            return;
        }
        gotoHit(mHits.size() - 1);
        return;
    }
    gotoHit(mHitIndex - 1);
}

void DATableFindController::onNextRequested()
{
    if (mHits.isEmpty()) {
        runScan();
        if (mHits.isEmpty()) {
            return;
        }
        gotoHit(0);
        return;
    }
    gotoHit(mHitIndex + 1);
}

void DATableFindController::onReplaceRequested()
{
    applyReplace(false);
}

void DATableFindController::onReplaceAllRequested()
{
    applyReplace(true);
}

void DATableFindController::onDialogClosed()
{
    // 清查找高亮（按来源清 Find，不影响 Validation 层）与命中状态；
    // 序号自增使任何在途扫描结果作废（关闭即弃）
    ++mRequestSerial;
    mHits.clear();
    mHitIndex  = -1;
    mTotalHits = 0;
    if (DATableHighlightOverlay* overlay = mCtx.highlightOverlay()) {
        overlay->clear(DATableHighlightOverlay::Source::Find);
    }
}

/**
 * @brief 执行一次查找扫描（硬约束①②③的页面侧落点）
 *
 * ①去抖在对话框侧（QTimer 350ms）；②请求序号：进入即自增，结果返回时序号非最新即丢弃
 * （不排队、过期即弃）；③上限截断在算子侧（max_hits 默认 10 万），totalHits 带真实总数。
 * 扫描同步执行 + DAWaitCursorScoped 忙等待光标（RAII 保证异常/提前返回也恢复）。
 * 命中灌入 overlay(Source::Find, Level::Hit)，当前命中单独 Level::CurrentHit。
 */
void DATableFindController::runScan()
{
    DA_SLOW_LOG_SCOPE_DETAIL("dagui.table.findScan", [ this ]() {
        return QStringLiteral("text_len=%1").arg(mLastOptions.keyword.size());
    });
    if (!mFindDialog) {
        return;
    }
    const int serial                            = ++mRequestSerial;  // 硬约束②：本次请求序号
    mLastOptions                                = mFindDialog->options();
    const DADialogFindReplace::FindOptions& opt = mLastOptions;
    DATableHighlightOverlay* overlay             = mCtx.highlightOverlay();
    // 清旧命中与高亮
    mHits.clear();
    mHitIndex  = -1;
    mTotalHits = 0;
    if (overlay) {
        overlay->clear(DATableHighlightOverlay::Source::Find);
    }
    if (opt.keyword.isEmpty()) {
        updateCountLabel();
        return;
    }
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        updateCountLabel();
        return;
    }
    DAWaitCursorScoped waitCursor;  // 忙等待光标（§4.2 进度反馈），RAII 恢复
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    const DAPyScriptsDataFrame::SearchDataResult res =
        py.searchData(df,
                      opt.keyword,
                      opt.caseSensitive,
                      opt.useRegex,
                      opt.wholeCell,
                      scanRowRange(opt.selectionOnly),
                      scanColRange(opt.selectionOnly));
    if (serial != mRequestSerial) {
        return;  // 过期结果丢弃（期间又发起了新请求/关闭）
    }
    if (!res.ok) {
        mFindDialog->setSearchFailed(true);  // 无效正则等
        updateCountLabel();
        return;
    }
    mTotalHits = res.totalHits;
    const int n = qMin(res.rows.size(), res.cols.size());
    mHits.reserve(n);
    for (int i = 0; i < n; ++i) {
        mHits.append(qMakePair(res.rows.at(i), res.cols.at(i)));
    }
    if (overlay) {
        overlay->set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::Hit, mHits);
    }
    if (!mHits.isEmpty()) {
        gotoHit(0);  // 首条设为当前命中并定位（内部会更新计数标签）
    } else {
        updateCountLabel();
    }
}

/**
 * @brief 导航到第 idx 条命中（循环），更新当前命中高亮并滚动定位
 * @param idx 目标序号（越界自动回绕）
 */
void DATableFindController::gotoHit(int idx)
{
    const int n = mHits.size();
    DATableHighlightOverlay* overlay = mCtx.highlightOverlay();
    if (n == 0) {
        mHitIndex = -1;
        if (overlay) {
            overlay->clear(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::CurrentHit);
        }
        updateCountLabel();
        return;
    }
    mHitIndex               = ((idx % n) + n) % n;  // 循环导航
    const QPair< int, int > hit = mHits.at(mHitIndex);
    // 当前命中单独一条 Level::CurrentHit（覆盖该格的 Hit 级别）
    if (overlay) {
        overlay->set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::CurrentHit, { hit });
    }
    if (mCtx.locateActualCell) {
        if (mLastOptions.selectionOnly) {
            // 选区范围模式：只滚动定位、不选中命中格（选区是扫描/替换的范围基准，
            // 不能被导航收缩，002 回归点）；当前命中靠 CurrentHit 高亮辨识
            mCtx.locateActualCell(hit.first, hit.second, /*select=*/false);
        } else {
            // 滚动定位 + 选中（命中来自全表扫描，可能落在隐藏列——单入口负责取消隐藏）
            mCtx.locateActualCell(hit.first, hit.second, /*select=*/true);
        }
    }
    updateCountLabel();
}

/**
 * @brief 扫描行范围：全表 → nullopt；选区 → 选中行包围盒（数据源绝对行号）
 * @param selectionOnly 范围选项是否为「当前选区」
 * @return 行闭区间或 nullopt
 */
std::optional< QPair< int, int > > DATableFindController::scanRowRange(bool selectionOnly) const
{
    if (!selectionOnly) {
        return std::nullopt;
    }
    QItemSelectionModel* sm = mSel.selectionModel();
    if (!sm || !sm->hasSelection()) {
        return std::nullopt;  // 无选区按全表处理
    }
    int minR = INT_MAX;
    int maxR = INT_MIN;
    // 按选区区间取包围盒（全选卡顿修复：不再 selectedIndexes 物化整个选区；
    // toActualRow 随视图行单调，区间顶/底的映射即最小/最大）
    const QItemSelection sel = sm->selection();
    for (const QItemSelectionRange& r : sel) {
        minR = qMin(minR, mCtx.toActualRow(r.top()));
        maxR = qMax(maxR, mCtx.toActualRow(r.bottom()));
    }
    if (maxR < minR) {
        return std::nullopt;
    }
    return qMakePair(minR, maxR);
}

/**
 * @brief 扫描列范围：全表 → nullopt；选区 → 选中列包围盒
 * @param selectionOnly 范围选项是否为「当前选区」
 * @return 列闭区间或 nullopt
 */
std::optional< QPair< int, int > > DATableFindController::scanColRange(bool selectionOnly) const
{
    if (!selectionOnly) {
        return std::nullopt;
    }
    QItemSelectionModel* sm = mSel.selectionModel();
    if (!sm || !sm->hasSelection()) {
        return std::nullopt;
    }
    int minC = INT_MAX;
    int maxC = INT_MIN;
    // 按选区区间取包围盒（全选卡顿修复：不再 selectedIndexes 物化整个选区）
    const QItemSelection sel = sm->selection();
    for (const QItemSelectionRange& r : sel) {
        minC = qMin(minC, r.left());
        maxC = qMax(maxC, r.right());
    }
    if (maxC < minC) {
        return std::nullopt;
    }
    return qMakePair(minC, maxC);
}

/**
 * @brief 更新计数标签（含上限截断提示，硬约束③）
 */
void DATableFindController::updateCountLabel()
{
    if (!mFindDialog) {
        return;
    }
    mFindDialog->setHitCount(mHitIndex + 1, mHits.size(), mTotalHits, mTotalHits > mHits.size());
}

/**
 * @brief 应用替换（all=false 替换当前命中这一格 / true 替换扫描范围全部）
 *
 * 流程：先用同一匹配语义预扫拿变更格数（真实命中总数=将变更格数）→ 超阈值
 * （c_replaceUndoCellLimit）弹「不可撤销」确认，确认后执行但不入栈（§4.11 降级路径，
 * 不违反 D4）→ 否则 findReplace 就地替换 + 构造 DACommandDataFrame_replace push
 * （一次全部替换 = 一个撤销单元；后效经协调器 commit：受影响列统计失效+计算列标脏）
 * → 重新扫描刷新命中。
 * @param all 是否全部替换
 */
void DATableFindController::applyReplace(bool all)
{
    if (!mFindDialog) {
        return;
    }
    const DADialogFindReplace::FindOptions opt = mFindDialog->options();
    if (opt.keyword.isEmpty()) {
        return;
    }
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return;
    }
    if (!mCtx.isDataSourceEditable()) {
        daWarning << tr("This data source is read-only and cannot be modified.");  // cn:该数据源为只读，无法修改
        return;
    }
    // 解析替换范围：全部替换 → 扫描范围；替换当前 → 当前命中单格
    std::optional< QPair< int, int > > rowRange;
    std::optional< QPair< int, int > > colRange;
    if (all) {
        rowRange = scanRowRange(opt.selectionOnly);
        colRange = scanColRange(opt.selectionOnly);
    } else {
        if (mHitIndex < 0 || mHitIndex >= mHits.size()) {
            // 无当前命中：先扫一次（如刚打开直接点替换）
            runScan();
            if (mHits.isEmpty()) {
                daInfo << tr("No matches found");  // cn:未找到匹配项
                return;
            }
        }
        const QPair< int, int > hit = mHits.at(mHitIndex);
        rowRange                    = qMakePair(hit.first, hit.first);
        colRange                    = qMakePair(hit.second, hit.second);
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    // 预扫变更格数（与替换同一匹配语义，命中总数=将变更格数）
    int pendingCount = 0;
    {
        DAWaitCursorScoped waitCursor;
        const DAPyScriptsDataFrame::SearchDataResult probe =
            py.searchData(df, opt.keyword, opt.caseSensitive, opt.useRegex, opt.wholeCell, rowRange, colRange);
        if (!probe.ok) {
            mFindDialog->setSearchFailed(true);  // 无效正则等
            return;
        }
        pendingCount = probe.totalHits;
    }
    if (pendingCount <= 0) {
        daInfo << tr("No matches found");  // cn:未找到匹配项
        return;
    }
    bool undoable = true;
    if (pendingCount > c_replaceUndoCellLimit) {
        const auto btn = QMessageBox::question(mCtx.hostWidget(),
                                               tr("Replace All"),  // cn:全部替换
                                               tr("Too many cells will be changed (%1). This operation cannot be undone. Continue?")
                                                   .arg(pendingCount),  // cn:改动量过大（%1 格），本次替换将不可撤销，是否继续？
                                               QMessageBox::Yes | QMessageBox::No,
                                               QMessageBox::No);
        if (btn != QMessageBox::Yes) {
            return;
        }
        undoable = false;
    }
    QVector< int > rows;
    QVector< int > cols;
    pybind11::list oldValues;
    pybind11::list newValues;
    bool okReplace = false;
    {
        DAWaitCursorScoped waitCursor;
        if (undoable) {
            okReplace = py.findReplace(df, opt.keyword, opt.replaceText, opt.caseSensitive, opt.useRegex, opt.wholeCell, rowRange, colRange, &rows, &cols, &oldValues, &newValues);
        } else {
            // 降级路径：只执行替换，不记录清单（不可撤销、不入栈）
            okReplace = py.findReplace(df, opt.keyword, opt.replaceText, opt.caseSensitive, opt.useRegex, opt.wholeCell, rowRange, colRange);
        }
    }
    if (!okReplace) {
        mFindDialog->setSearchFailed(true);
        return;
    }
    if (!undoable) {
        // 统计失效挂点（003）：降级路径未记录清单，保守失效全部列统计
        if (DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore()) {
            cfStore->invalidateAllStats();
        }
        // 未记录清单 → 全表模型刷新 + 全部计算列标脏（拿不到变更列信息的保守路径，R8）
        if (DADataTableModel* model = mCtx.model()) {
            model->notifyDataChanged(0, 0, qMax(0, model->actualRowCount() - 1), qMax(0, model->columnCount() - 1));
        }
        if (mMutation) {
            mMutation->markAllComputedColumnsStale();
        }
        runScan();  // 替换后重新扫描刷新命中（§WP2 步骤 6）
        Q_EMIT replaceApplied(pendingCount);
        return;
    }
    if (rows.isEmpty()) {
        daInfo << tr("No matches found");  // cn:未找到匹配项
        return;
    }
    // 正常路径：数据已被 findReplace 就地替换，构造命令持有变更清单后 push
    //（DACommandWithRedoCount 首次 redo 跳过，不重复替换；undo/redo 经 da_write_cells 批量写回）
    std::unique_ptr< DACommandDataFrame_replace > cmd(new DACommandDataFrame_replace(df, rows, cols, oldValues, newValues));
    QPointer< DADataTableModel > modle = mCtx.model();
    DACommandDataFrame_replace* cmdRaw = cmd.get();
    // 编辑后效（008 期2 收口）：受影响列统计失效 + 变更范围包围盒模型通知 + 计算列标脏，
    // exec 与 undo/redo 共用一次 commit
    QList< int > colsList;
    colsList.reserve(cols.size());
    for (int c : cols) {
        colsList.append(c);
    }
    if (mMutation) {
        DATableMutation mutation;
        mutation.kind        = DATableMutation::Values;
        mutation.changedCols = colsList;
        mutation.modelRefresh = [ modle, cmdRaw ]() {
            const QVector< int >& rs = cmdRaw->changedRows();
            const QVector< int >& cs = cmdRaw->changedCols();
            if (!modle) {
                return;
            }
            if (rs.isEmpty()) {
                return;
            }
            int r0 = rs.at(0), r1 = rs.at(0), c0 = cs.at(0), c1 = cs.at(0);
            for (int r : rs) {
                r0 = qMin(r0, r);
                r1 = qMax(r1, r);
            }
            for (int c : cs) {
                c0 = qMin(c0, c);
                c1 = qMax(c1, c);
            }
            modle->notifyDataChanged(r0, c0, r1, c1);
        };
        cmd->setCallBack(mMutation->callbackFor(mutation));
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
    runScan();  // 替换后重新扫描刷新命中（§WP2 步骤 6）
    Q_EMIT replaceApplied(rows.size());
}
}  // end of namespace DA
