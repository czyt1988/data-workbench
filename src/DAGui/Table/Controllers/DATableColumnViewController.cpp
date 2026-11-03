#include "Table/Controllers/DATableColumnViewController.h"
#include "Table/Core/DATablePageContext.h"
#include "Table/Core/DATableSelectionService.h"
#include "Table/Controllers/DATableViewStateController.h"
#include "Table/Controllers/DATableComputedColumnController.h"
#include "Table/Controllers/DATableValidationController.h"
// qt
#include <QDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QWidget>
// DAGui
#include "Table/Views/DADataHeaderView.h"
#include "Table/Views/DADataTableView.h"
#include "Table/Models/DADataTableModel.h"
#include "Table/Ops/DACommandsDataFrame.h"
#include "Table/Commands/DACommandsTableColumnView.h"
#include "DACallBackInterface.h"
#include "Table/Views/DATablePaneContainer.h"
#include "Table/Metadata/DATableViewStateStore.h"
#include "Table/Dialogs/DADialogTableColumnView.h"
// DAData
#include "DADataPyDataFrame.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

#include <memory>

namespace DA
{
namespace
{
// 身份列表 → 身份集合（Qt5.14 无 QSet 迭代器区间构造，循环插入）
QSet< QPair< QString, int > > toIdentSet(const QList< QPair< QString, int > >& idents)
{
    QSet< QPair< QString, int > > res;
    for (const QPair< QString, int >& ident : idents) {
        res.insert(ident);
    }
    return res;
}

// 冻结集合合并（唯一插入规则，单列冻结 / 冻结前 N 列 / 列视图对话框共用）：
// 保留旧冻结列表中仍被选中的列及其相对顺序（用户可能在冻结区拖过顺序），
// 新增列按全局列序插入到「它在全局列序中前一个已选列」之后；前面没有已选列
// 时插到最前。
QList< QPair< QString, int > > mergeFrozenOrder(const QList< QPair< QString, int > >& oldFrozen,
                                                const QSet< QPair< QString, int > >& checked,
                                                const QList< QPair< QString, int > >& globalOrder)
{
    QList< QPair< QString, int > > res;
    res.reserve(checked.size());
    for (const QPair< QString, int >& ident : oldFrozen) {
        if (checked.contains(ident) && !res.contains(ident)) {
            res.append(ident);  // 旧顺序保留（冻结区自定义顺序不丢）
        }
    }
    for (int g = 0; g < globalOrder.size(); ++g) {
        const QPair< QString, int >& ident = globalOrder.at(g);
        if (!checked.contains(ident) || res.contains(ident)) {
            continue;
        }
        int pos = 0;  // 前驱插入：前一个已选列之后
        for (int i = g - 1; i >= 0; --i) {
            const int idx = res.indexOf(globalOrder.at(i));
            if (idx >= 0) {
                pos = idx + 1;
                break;
            }
        }
        res.insert(pos, ident);
    }
    return res;
}
}  // namespace

//===================================================
// DATableColumnViewController
//===================================================

DATableColumnViewController::DATableColumnViewController(const DATablePageContext& ctx,
                                                         const DATableSelectionService& sel,
                                                         DATableViewStateController* viewState,
                                                         DATableComputedColumnController* computedColumn,
                                                         DATableValidationController* validation,
                                                         QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel), mViewState(viewState), mComputedColumn(computedColumn),
      mValidation(validation)
{
}

/**
 * @brief 列视图直接入口：只写 store + 重投影，不入撤销栈
 *
 * 007 定位的对接面（恢复隐藏列后跳转）。唯一投影路径（视图状态控制器 applyFromStore），
 * 不另写第二条，也不偷偷 push 命令。
 * @param name 列名
 * @param occurrence 重名消歧位
 * @param hidden 是否隐藏
 */
void DATableColumnViewController::setColumnHiddenDirectly(const QString& name, int occurrence, bool hidden)
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    if (!vsStore || name.isEmpty()) {
        return;
    }
    vsStore->setColumnHidden(name, occurrence, hidden);
    if (mViewState) {
        mViewState->applyFromStore();
    }
}

/**
 * @brief 生效冻结列数（= 冻结集合中当前可见的列数，转发容器）
 */
int DATableColumnViewController::effectiveFrozenColumnCount() const
{
    DATablePaneContainer* pc = mCtx.paneContainer();
    return pc ? pc->effectiveFrozenColumnCount() : 0;
}

/**
 * @brief 全局列序（含隐藏列）的身份表
 *
 * 口径 = store 的 columnOrder 按 (名, occurrence) 精确匹配实际列身份表，
 * 每条只匹配一次，未匹配的实际列按 logical 升序追加末尾；store 为空 = logical 升序。
 * 任何地方要"全局列序身份表"都调这里（006d 列视图对话框的输入），不要在调用点重写匹配。
 * @return 全局列序身份表
 */
QList< QPair< QString, int > > DATableColumnViewController::orderedColumnIdentities() const
{
    const QList< QPair< QString, int > > actual = mSel.actualColumnIdentities();
    QList< QPair< QString, int > > res;
    res.reserve(actual.size());
    if (DATableViewStateStore* vsStore = mCtx.viewStateStore()) {
        QSet< QPair< QString, int > > used;
        const QList< QPair< QString, int > > storedOrder = vsStore->columnOrder();
        for (const QPair< QString, int >& ident : storedOrder) {
            if (actual.contains(ident) && !used.contains(ident)) {  // 精确匹配，每条只匹配一次
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
}

/**
 * @brief 隐藏指定列（列头菜单「隐藏此列」）
 *
 * 构造隐藏集合变更命令入栈；已隐藏（集合无差异）不推空命令。
 * @param logicalColumn 逻辑列号
 */
void DATableColumnViewController::hideColumnAt(int logicalColumn)
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    DADataTableModel* model        = mCtx.model();
    if (!vsStore || !model) {
        return;
    }
    if (logicalColumn < 0 || logicalColumn >= mCtx.dataColumnCount()) {
        return;
    }
    const QString name = model->headerData(logicalColumn, Qt::Horizontal).toString();
    if (name.isEmpty()) {
        return;  // 尾部预留空白列不参与列视图
    }
    int occ = 0;
    for (int c = 0; c < logicalColumn; ++c) {
        if (model->headerData(c, Qt::Horizontal).toString() == name) {
            ++occ;
        }
    }
    const QSet< QPair< QString, int > > oldHidden = toIdentSet(vsStore->hiddenColumns());
    QSet< QPair< QString, int > > newHidden       = oldHidden;
    newHidden.insert(qMakePair(name, occ));
    if (newHidden == oldHidden) {
        return;  // 已隐藏，不推空命令
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandTableColumnVisibility(vsStore, oldHidden, newHidden,
                                                             [ this ]() { mViewState->applyFromStore(); }));
    }
}

/**
 * @brief 显示所有列（列头菜单「显示所有列」）
 *
 * 隐藏集合清空入栈；空集合（无差异）不推空命令。
 */
void DATableColumnViewController::showAllColumns()
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    if (!vsStore) {
        return;
    }
    const QSet< QPair< QString, int > > oldHidden = toIdentSet(vsStore->hiddenColumns());
    if (oldHidden.isEmpty()) {
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandTableColumnVisibility(vsStore, oldHidden, QSet< QPair< QString, int > >(),
                                                            [ this ]() { mViewState->applyFromStore(); }));
    }
}

/**
 * @brief 冻结 / 取消冻结指定列（表头右键「冻结此列/取消冻结此列」）
 *
 * 冻结列可以来自列序任意位置：冻结不改变全局列序（解冻后列回到原位），只把该列
 * 切进左侧固定区。入撤销栈；集合无差异不推空命令。
 * @param logicalColumn 逻辑列号
 * @param frozen true = 冻结，false = 取消冻结
 */
void DATableColumnViewController::setColumnFrozen(int logicalColumn, bool frozen)
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    DADataTableModel* model        = mCtx.model();
    if (!vsStore || !model) {
        return;
    }
    if (logicalColumn < 0 || logicalColumn >= mCtx.dataColumnCount()) {
        return;
    }
    const QString name = model->headerData(logicalColumn, Qt::Horizontal).toString();
    if (name.isEmpty()) {
        return;  // 尾部预留空白列不参与列视图
    }
    int occ = 0;
    for (int c = 0; c < logicalColumn; ++c) {
        if (model->headerData(c, Qt::Horizontal).toString() == name) {
            ++occ;
        }
    }
    const QPair< QString, int > ident               = qMakePair(name, occ);
    const QList< QPair< QString, int > > oldFrozen = vsStore->frozenColumns();
    const QSet< QPair< QString, int > > oldSet     = toIdentSet(oldFrozen);
    QSet< QPair< QString, int > > checked          = oldSet;
    if (frozen) {
        checked.insert(ident);
    } else {
        checked.remove(ident);
    }
    if (checked == oldSet) {
        return;  // 无差异不推空命令
    }
    const QList< QPair< QString, int > > newFrozen = mergeFrozenOrder(oldFrozen, checked, orderedColumnIdentities());
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandTableFreezeColumns(vsStore, oldFrozen, newFrozen,
                                                          [ this ]() { mViewState->applyFromStore(); }));
    }
}

/**
 * @brief 冻结全局列序第一个可见列（Ribbon「冻结首列」）
 */
void DATableColumnViewController::freezeFirstColumn()
{
    DATablePaneContainer* pc = mCtx.paneContainer();
    const QVector< int > visible = pc ? pc->visibleColumnOrderLogical() : QVector< int >();
    if (visible.isEmpty()) {
        return;
    }
    setColumnFrozen(visible.first(), true);
}

/**
 * @brief 取消全部冻结（Ribbon「取消全部冻结」）
 */
void DATableColumnViewController::unfreezeAllColumns()
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    if (!vsStore) {
        return;
    }
    const QList< QPair< QString, int > > oldFrozen = vsStore->frozenColumns();
    if (oldFrozen.isEmpty()) {
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandTableFreezeColumns(vsStore, oldFrozen, QList< QPair< QString, int > >(),
                                                          [ this ]() { mViewState->applyFromStore(); }));
    }
}

/**
 * @brief 弹「冻结前 N 列」输入框（Ribbon 冻结列主体入口）
 *
 * 语义是「追加」：把全局列序前 N 列并入已有冻结集合，不影响其它已冻结列；
 * 0 = 不做修改（取消全部冻结走独立菜单项）。
 */
void DATableColumnViewController::promptFreezeColumns()
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    if (!vsStore) {
        return;
    }
    const QList< QPair< QString, int > > ordered = orderedColumnIdentities();
    if (ordered.isEmpty()) {
        return;
    }
    bool ok    = false;
    const int n = QInputDialog::getInt(
        mCtx.hostWidget(),
        tr("Freeze Columns"),  // cn:冻结列
        tr("Freeze the first N columns (added to already frozen columns, 0 = no change):"),
        // cn:冻结前 N 列（追加到已冻结的列，0 = 不做修改）：
        1,
        0,
        ordered.size(),
        1,
        &ok);
    if (!ok || n <= 0) {
        return;
    }
    const QList< QPair< QString, int > > oldFrozen = vsStore->frozenColumns();
    const QSet< QPair< QString, int > > oldSet     = toIdentSet(oldFrozen);
    QSet< QPair< QString, int > > checked          = oldSet;
    for (int i = 0; i < ordered.size() && i < n; ++i) {
        checked.insert(ordered.at(i));
    }
    if (checked == oldSet) {
        return;  // 无差异不推空命令
    }
    const QList< QPair< QString, int > > newFrozen = mergeFrozenOrder(oldFrozen, checked, ordered);
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandTableFreezeColumns(vsStore, oldFrozen, newFrozen,
                                                          [ this ]() { mViewState->applyFromStore(); }));
    }
}

/**
 * @brief 容器 columnOrderChanged 驱动：拖动落库入栈
 *
 * 收到的是容器**合并后的全局列序**（含隐藏列）。logical → (列名, occurrence)
 * 转换按数据列序算 occurrence（不按传入顺序）；无差异不推命令。
 * @param newOrderLogical 新全局列序（logical，含隐藏列）
 */
void DATableColumnViewController::applyColumnOrderFromView(const QVector< int >& newOrderLogical)
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    DADataTableModel* model        = mCtx.model();
    if (!vsStore || !model) {
        return;
    }
    QList< QPair< QString, int > > newOrder;
    newOrder.reserve(newOrderLogical.size());
    for (int logical : newOrderLogical) {
        if (logical < 0 || logical >= mCtx.dataColumnCount()) {
            continue;
        }
        const QString name = model->headerData(logical, Qt::Horizontal).toString();
        if (name.isEmpty()) {
            continue;  // 尾部预留空白列不进列序
        }
        int occ = 0;
        for (int c = 0; c < logical; ++c) {
            if (model->headerData(c, Qt::Horizontal).toString() == name) {
                ++occ;
            }
        }
        newOrder.append(qMakePair(name, occ));
    }
    QList< QPair< QString, int > > oldOrder = vsStore->columnOrder();
    if (oldOrder.isEmpty()) {
        oldOrder = mSel.actualColumnIdentities();  // store 为空 → 旧值视作"数据列序"
    }
    if (oldOrder == newOrder) {
        return;  // 无差异不推命令
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandTableColumnOrder(vsStore, oldOrder, newOrder,
                                                       [ this ]() { mViewState->applyFromStore(); }));
    }
}

/**
 * @brief 容器 frozenOrderChanged 驱动：冻结区内部拖动落库入栈
 *
 * 只改冻结列之间的先后顺序，不动全局列序（解冻后列仍回原位）。logical → 身份
 * 换算按数据列序算 occurrence；顺序无变化不推命令。
 * @param newFrozenOrderLogical 新冻结区顺序（logical，按冻结窗格 visualIndex）
 */
void DATableColumnViewController::applyFrozenOrderFromView(const QVector< int >& newFrozenOrderLogical)
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    DADataTableModel* model        = mCtx.model();
    if (!vsStore || !model) {
        return;
    }
    QList< QPair< QString, int > > newFrozen;
    newFrozen.reserve(newFrozenOrderLogical.size());
    for (int logical : newFrozenOrderLogical) {
        if (logical < 0 || logical >= mCtx.dataColumnCount()) {
            continue;
        }
        const QString name = model->headerData(logical, Qt::Horizontal).toString();
        if (name.isEmpty()) {
            continue;
        }
        int occ = 0;
        for (int c = 0; c < logical; ++c) {
            if (model->headerData(c, Qt::Horizontal).toString() == name) {
                ++occ;
            }
        }
        const QPair< QString, int > ident = qMakePair(name, occ);
        if (!newFrozen.contains(ident)) {
            newFrozen.append(ident);
        }
    }
    if (newFrozen == vsStore->frozenColumns()) {
        return;  // 顺序无变化不推命令
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(new DACommandTableFreezeColumns(vsStore, vsStore->frozenColumns(), newFrozen,
                                                          [ this ]() { mViewState->applyFromStore(); }));
    }
}

/**
 * @brief 「列」子菜单动作分发（006d）
 *
 * 冻结动作按当前列状态切换：未冻结 → 冻结此列；已冻结 → 取消冻结此列。
 * 「取消全部冻结」清空冻结集合。
 * @param logicalColumn 触发菜单的列（逻辑索引）
 * @param action 子菜单动作（DADataHeaderView::ColumnViewAction 整数值）
 */
void DATableColumnViewController::onHeaderActionRequested(int logicalColumn, int action)
{
    switch (static_cast< DADataHeaderView::ColumnViewAction >(action)) {
    case DADataHeaderView::ColumnViewAction::HideColumn:
        hideColumnAt(logicalColumn);
        break;
    case DADataHeaderView::ColumnViewAction::ShowAllColumns:
        showAllColumns();
        break;
    case DADataHeaderView::ColumnViewAction::ToggleFreezeColumn:
        setColumnFrozen(logicalColumn,
                        !(mViewState && mViewState->isColumnInFrozenArea(logicalColumn)));
        break;
    case DADataHeaderView::ColumnViewAction::UnfreezeAll:
        unfreezeAllColumns();
        break;
    case DADataHeaderView::ColumnViewAction::OpenColumnViewDialog:
        showDialog();
        break;
    }
}

/**
 * @brief 打开列视图对话框（006d；表格化改版：列名编辑/隐藏/冻结三列表格）
 *
 * 输入 = 全局列序（含隐藏列）的身份表（orderedColumnIdentities 唯一实现）；
 * 确定后按差异构造一个批量父命令（改名 + 隐藏集合变更 + 冻结数变更 =
 * 一个撤销单元），取消不改动；冻结数比较基准用 store 真源旧值。
 * 改名对经 DACommandDataFrame_renameColumns 入批（列名缓存重建 + 视图状态
 * 键迁移 + 数据管理器通知均在命令/控制器接线里完成）。
 */
void DATableColumnViewController::showDialog()
{
    DATableViewStateStore* vsStore = mCtx.viewStateStore();
    DADataTableModel* model        = mCtx.model();
    if (!vsStore || !model) {
        return;
    }
    if (!mDialogColumnView) {
        mDialogColumnView = new DADialogTableColumnView(mCtx.hostWidget());
    }
    const QList< QPair< QString, int > > ordered = orderedColumnIdentities();
    if (ordered.isEmpty()) {
        return;
    }
    QSet< QPair< QString, int > > oldHidden = toIdentSet(vsStore->hiddenColumns());
    const QList< QPair< QString, int > > oldFrozen = vsStore->frozenColumns();
    mDialogColumnView->setup(ordered, oldHidden, toIdentSet(oldFrozen));
    if (mDialogColumnView->exec() != QDialog::Accepted) {
        return;  // 取消不改动任何东西
    }
    const QSet< QPair< QString, int > > newHidden       = mDialogColumnView->hiddenColumns();
    const QSet< QPair< QString, int > > checkedFrozen   = mDialogColumnView->frozenColumns();
    // 对话框只给勾选集合；顺序按统一插入规则合并（保住冻结区自定义顺序，新增列按全局列序落位）
    const QList< QPair< QString, int > > newFrozen = mergeFrozenOrder(oldFrozen, checkedFrozen, ordered);
    // 改名对（对话框按行序给出 (旧身份 → 新名)）
    const QList< QPair< QPair< QString, int >, QString > > renames = mDialogColumnView->renamedColumns();
    // 改名后身份映射：改名命令先于隐藏/冻结子命令执行，视图状态键已迁移到新列名，
    // 子命令的 old/new 身份必须一并换成新键（否则写入孤儿身份，隐藏/冻结丢失）
    QHash< QPair< QString, int >, QPair< QString, int > > identAfterRename;
    if (newHidden == oldHidden && newFrozen == oldFrozen && renames.isEmpty()) {
        return;  // 无差异不推空命令
    }
    // 批量父命令先行（renameCmd 要以它为 parent 构造）
    DACommandTableColumnViewBatch* batch = new DACommandTableColumnViewBatch(tr("Column View..."));  // cn:列视图
    if (!renames.isEmpty()) {
        DAPyDataFrame df = mCtx.df();
        if (df.isNone()) {
            delete batch;
            return;
        }
        const QList< QString > oldcols = df.columns();
        const QList< QString > newcols = [ &oldcols, &renames ]() {
            // 身份 (名, occurrence) → 数据列序中的位置（occurrence 按数据列序计）
            QList< QString > res = oldcols;
            QHash< QString, int > seen;
            QHash< QPair< QString, int >, int > posOfIdent;
            for (int c = 0; c < res.size(); ++c) {
                const int occ = seen.value(res.at(c), 0);
                seen[ res.at(c) ] = occ + 1;
                posOfIdent.insert(qMakePair(res.at(c), occ), c);
            }
            for (const auto& p : renames) {
                const auto it = posOfIdent.constFind(p.first);
                if (it != posOfIdent.constEnd() && it.value() >= 0 && it.value() < res.size()) {
                    res[ it.value() ] = p.second;
                }
            }
            return res;
        }();
        // 重名校验（全表口径兜底；对话框已校验行间不重）
        {
            QSet< QString > uniq;
            bool dup = false;
            for (const QString& n : newcols) {
                if (uniq.contains(n)) {
                    dup = true;
                    break;
                }
                uniq.insert(n);
            }
            if (dup) {
                daWarning << tr("Column names must be unique");  // cn:列名不能重复
                delete batch;
                return;
            }
        }
        QList< QPair< QString, QString > > renamePairs;  // 计算列/校验规则/视图状态迁移用
        for (const auto& p : renames) {
            renamePairs.append(qMakePair(p.first.first, p.second));
        }
        // 改名后身份映射（旧身份 → 新身份；occurrence 按新列名表重算）
        {
            QHash< QPair< QString, int >, int > posOfOld;
            QHash< QString, int > seenOld;
            for (int c = 0; c < oldcols.size(); ++c) {
                const int occ = seenOld.value(oldcols.at(c), 0);
                seenOld[ oldcols.at(c) ] = occ + 1;
                posOfOld.insert(qMakePair(oldcols.at(c), occ), c);
            }
            QHash< int, QPair< QString, int > > identOfNewPos;
            QHash< QString, int > seenNew;
            for (int c = 0; c < newcols.size(); ++c) {
                const int occ = seenNew.value(newcols.at(c), 0);
                seenNew[ newcols.at(c) ] = occ + 1;
                identOfNewPos.insert(c, qMakePair(newcols.at(c), occ));
            }
            for (auto it = posOfOld.constBegin(); it != posOfOld.constEnd(); ++it) {
                identAfterRename.insert(it.key(), identOfNewPos.value(it.value(), it.key()));
            }
        }
        // 计算列/校验规则引用联动确认（与全表改名同款）
        if (mComputedColumn && !mComputedColumn->confirmRename(renamePairs)) {
            delete batch;
            return;
        }
        if (mValidation && !mValidation->confirmRuleRename(renamePairs)) {
            delete batch;
            return;
        }
        // 构造即挂 batch（QUndoCommand 的 parent 只能在构造时给）；DACommandWithRedoCount
        // 首次 redo 跳过 exec——与"页面 exec 后 push 首次 redo 不重放"的批量语义吻合
        DACommandDataFrame_renameColumns* renameCmd = new DACommandDataFrame_renameColumns(df, newcols, oldcols, model, batch);
        if (mComputedColumn) {
            mComputedColumn->wireRenameMigration(renameCmd, renamePairs);
        }
        if (mValidation) {
            mValidation->wireRuleRenameMigration(renameCmd, renamePairs);
        }
        // 视图状态键迁移挂 directional（undo 反向回迁），与 renameColumn 同款
        DATableViewStateStore* vsStore2 = vsStore;
        DACallBackInterface::DirectionalCallBack prev = renameCmd->getDirectionalCallBack();
        renameCmd->setDirectionalCallBack([ prev, vsStore2, renamePairs ](bool isUndo) {
            if (prev) {
                prev(isUndo);
            }
            for (const auto& p : std::as_const(renamePairs)) {
                vsStore2->renameColumn(isUndo ? p.second : p.first, isUndo ? p.first : p.second);
            }
        });
        if (!renameCmd->exec()) {
            delete batch;  // 连同子命令一起销毁
            return;
        }
        batch->setText(tr("Rename and update column view"));  // cn:重命名并更新列视图
    }
    // 隐藏/冻结子命令的身份统一换到改名后的键（无改名时映射为空，原样透传）
    QSet< QPair< QString, int > > finalOldHidden;
    QSet< QPair< QString, int > > finalNewHidden;
    QList< QPair< QString, int > > finalOldFrozen;
    QList< QPair< QString, int > > finalNewFrozen;
    for (const QPair< QString, int >& id : oldHidden) {
        finalOldHidden.insert(identAfterRename.value(id, id));
    }
    for (const QPair< QString, int >& id : newHidden) {
        finalNewHidden.insert(identAfterRename.value(id, id));
    }
    for (const QPair< QString, int >& id : oldFrozen) {
        finalOldFrozen.append(identAfterRename.value(id, id));
    }
    for (const QPair< QString, int >& id : newFrozen) {
        finalNewFrozen.append(identAfterRename.value(id, id));
    }
    if (finalNewHidden != finalOldHidden) {
        new DACommandTableColumnVisibility(vsStore, finalOldHidden, finalNewHidden,
                                           [ this ]() { mViewState->applyFromStore(); }, QString(), batch);
    }
    if (finalNewFrozen != finalOldFrozen) {
        new DACommandTableFreezeColumns(vsStore, finalOldFrozen, finalNewFrozen,
                                         [ this ]() { mViewState->applyFromStore(); }, QString(), batch);
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(batch);
    }
    // 数据管理器树重建（改名改变树内列子项显示）
    if (!renames.isEmpty()) {
        if (mCtx.notifyHostColumnRename) {
            mCtx.notifyHostColumnRename();
        }
        if (mViewState) {
            mViewState->applyFromStore();
        }
    }
}
}  // end of namespace DA
