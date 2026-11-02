#include "DATableRenameOps.h"
#include "TableCore/DATablePageContext.h"
#include "TableViewState/DATableViewStateController.h"
#include "TableComputedColumn/DATableComputedColumnController.h"
#include "TableValidation/DATableValidationController.h"
// qt
#include <QDialog>
#include <QWidget>
// DAGui
#include "Models/DADataTableModel.h"
#include "Commands/DACommandsDataFrame.h"
#include "DACallBackInterface.h"
#include "DATableViewStateStore.h"
#include "Dialog/DARenameColumnsNameDialog.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

#include <memory>

namespace DA
{
//===================================================
// DATableRenameOps
//===================================================

DATableRenameOps::DATableRenameOps(const DATablePageContext& ctx,
                                   DATableViewStateController* viewState,
                                   DATableComputedColumnController* computedColumn,
                                   DATableValidationController* validation,
                                   QObject* parent)
    : QObject(parent), mCtx(ctx), mViewState(viewState), mComputedColumn(computedColumn), mValidation(validation)
{
}

QString DATableRenameOps::id() const
{
    return QStringLiteral("rename-ops");
}

QString DATableRenameOps::title() const
{
    return tr("Rename Columns");  // cn:重命名列
}

bool DATableRenameOps::isAvailable() const
{
    return !mCtx.df().isNone();
}

/**
 * @brief 打开全表列名对话框（唯一判断在对话框）
 *
 * 改名对（old→new，仅变化的）→ 计算列/校验规则引用影响面确认（R20）→
 * 引用迁移挂 directional 回调（与改名同一撤销单元）→ 命令入栈 →
 * 视图状态键单向迁移（宽度记录随改名同步）→ 宿主树刷新。
 */
void DATableRenameOps::renameColumns()
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return;
    }
    QList< QString > oldcols = df.columns();
    if (oldcols.size() <= 0) {
        daWarning << tr("Table has no columns");  // cn:表格没有列
        return;
    }
    DARenameColumnsNameDialog dlg(mCtx.hostWidget());
    dlg.setDataName(mCtx.data().getName());
    dlg.setColumnsName(oldcols);
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    QList< QString > cols = dlg.getColumnsName();
    // 唯一判断在DARenameColumnsNameDialog里进行
    // 改名对（old→new，仅变化的）
    QList< QPair< QString, QString > > renames;
    const int n = qMin(cols.size(), oldcols.size());
    for (int i = 0; i < n; ++i) {
        if (cols[ i ] != oldcols[ i ]) {
            renames.append(qMakePair(oldcols[ i ], cols[ i ]));
        }
    }
    // 计算列引用联动（004 T1.23 R20）：先提示影响面，确认后随改名命令同一撤销单元批量迁移
    if (mComputedColumn && !mComputedColumn->confirmRename(renames)) {
        return;
    }
    // 校验规则引用联动（005 T1.25 R20）：同款提示与迁移
    if (mValidation && !mValidation->confirmRuleRename(renames)) {
        return;
    }
    // 表头刷新走模型通知入口（命令不再持有 QHeaderView*）
    DACommandDataFrame_renameColumns* cmd = new DACommandDataFrame_renameColumns(df, cols, oldcols, mCtx.model());
    if (mComputedColumn) {
        mComputedColumn->wireRenameMigration(cmd, renames);
    }
    if (mValidation) {
        mValidation->wireRuleRenameMigration(cmd, renames);
    }
    if (!cmd->exec()) {
        delete cmd;
        return;
    }
    // 列改名迁移：视图状态按列名引用（R20），宽度记录随改名同步迁移
    if (DATableViewStateStore* vsStore = mCtx.viewStateStore()) {
        for (const auto& p : std::as_const(renames)) {
            vsStore->renameColumn(p.first, p.second);
        }
    }
    if (mCtx.notifyHostColumnRename) {
        mCtx.notifyHostColumnRename();  // 通知 datamanager（树内列子项重建）
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd);
    }
}

/**
 * @brief 重命名单列
 *
 * 构造"全表列名列表，仅修改目标列"传给 DACommandDataFrame_renameColumns，
 * 复用现有命令的 undo/redo 与 headerDataChanged 通知机制。
 * @param col 列位置索引
 * @param newName 新列名
 * @return 成功返回 true；列为空/重名/越界/exec 失败返回 false
 */
bool DATableRenameOps::renameColumn(int col, const QString& newName)
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return false;
    }
    QList< QString > oldcols = df.columns();
    if (col < 0 || col >= oldcols.size()) {
        return false;
    }
    if (newName.isEmpty()) {
        daWarning << tr("Column name cannot be empty");  // cn:列名不能为空
        return false;
    }
    if (oldcols.contains(newName)) {
        daWarning << tr("Column name \"%1\" already exists, please use another name").arg(newName);  // cn:列名"%1"已存在，请使用其他名称
        return false;
    }
    QList< QString > newcols = oldcols;
    newcols[ col ]           = newName;
    // 计算列引用联动（004 T1.23 R20）：先提示影响面，确认后随改名命令同一撤销单元批量迁移
    const QList< QPair< QString, QString > > renames { qMakePair(oldcols[ col ], newName) };
    if (mComputedColumn && !mComputedColumn->confirmRename(renames)) {
        return false;
    }
    // 校验规则引用联动（005 T1.25 R20）
    if (mValidation && !mValidation->confirmRuleRename(renames)) {
        return false;
    }
    std::unique_ptr< DACommandDataFrame_renameColumns > cmd(
        new DACommandDataFrame_renameColumns(df, newcols, oldcols, mCtx.model()));
    if (mComputedColumn) {
        mComputedColumn->wireRenameMigration(cmd.get(), renames);
    }
    if (mValidation) {
        mValidation->wireRuleRenameMigration(cmd.get(), renames);
    }
    // 视图状态迁移（列宽/列序/隐藏按列名键）：挂 directional 回调 redo/undo 双向
    // 迁移（链式包裹前两个迁移）——旧实现只在 exec 后单向迁移，undo 后 store 里
    // 还是新列名，后续投影按孤儿键跳过，列宽/隐藏/冻结状态错位
    if (DATableViewStateStore* vsStore = mCtx.viewStateStore()) {
        DACallBackInterface::DirectionalCallBack prev = cmd->getDirectionalCallBack();
        cmd->setDirectionalCallBack([ prev, vsStore, renames ](bool isUndo) {
            if (prev) {
                prev(isUndo);
            }
            for (const auto& p : std::as_const(renames)) {
                vsStore->renameColumn(isUndo ? p.second : p.first, isUndo ? p.first : p.second);
            }
        });
    }
    if (!cmd->exec()) {
        return false;
    }
    // 列改名后模型列名缓存随命令 exec/undo 的 refreshData 重建（命令内完成），
    // 这里补视图状态投影（模型 reset 触发的重投影用新键命中）
    if (mCtx.notifyHostColumnRename) {
        mCtx.notifyHostColumnRename();
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
    if (mViewState) {
        mViewState->applyFromStore();
    }
    return true;
}
}  // end of namespace DA
