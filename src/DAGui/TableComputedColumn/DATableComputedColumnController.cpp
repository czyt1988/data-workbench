#include "DATableComputedColumnController.h"
#include "TableCore/DATablePageContext.h"
// qt
#include <QDialog>
#include <QMessageBox>
#include <QPointer>
#include <QUndoCommand>
#include <QWidget>
// DAGui
#include "DAComputedColumn.h"
#include "DADataHeaderView.h"
#include "DADataTableView.h"
#include "Models/DADataTableModel.h"
#include "Commands/DACommandsDataFrame.h"
#include "Commands/DACommandsTableColumnView.h"
#include "DATableComputedColumnStore.h"
#include "DATableConditionalFormatStore.h"
#include "Dialog/DADialogComputedColumn.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"
// 忙等待光标（求值可能长）
#include "DAWaitCursorScoped.h"

namespace DA
{
//===================================================
// DATableComputedColumnController
//===================================================

DATableComputedColumnController::DATableComputedColumnController(const DATablePageContext& ctx, QObject* parent)
    : QObject(parent), mCtx(ctx)
{
    // 定义/状态变更 → 表头重绘（底色经模型 headerDataChanged，角标经视口 update）；
    // 期2 attach 时的两条连接随本控制器迁入（Observer：构造时一次性订阅）
    if (DATableComputedColumnStore* ccStore = mCtx.computedColumnStore()) {
        connect(ccStore, &DATableComputedColumnStore::definitionsChanged, this, &DATableComputedColumnController::refreshHeaders);
        connect(ccStore, &DATableComputedColumnStore::stateChanged, this,
                [ this ](const QString&, DATableComputedColumnStore::State) { refreshHeaders(); });
    }
}

/**
 * @brief store 变更后刷新表头（底色经模型 headerDataChanged，ƒx 角标经视口 update）
 */
void DATableComputedColumnController::refreshHeaders()
{
    if (DADataTableModel* model = mCtx.model()) {
        model->notifyHeaderDataChanged(Qt::Horizontal, 0, qMax(0, model->columnCount() - 1));
    }
    // ƒx 角标经视口 update 重绘——覆盖全部窗格（006e 冻结窗格同刷）
    for (DADataTableView* pane : mCtx.panes()) {
        if (DADataHeaderView* hh = qobject_cast< DADataHeaderView* >(pane->horizontalHeader())) {
            hh->viewport()->update();
        }
    }
}

/**
 * @brief 列位置索引 → 列名（越界返回空串）
 */
QString DATableComputedColumnController::columnNameAt(int actualCol) const
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone() || actualCol < 0) {
        return QString();
    }
    const QList< QString > cols = df.columns();
    return (actualCol < cols.size()) ? cols.at(actualCol) : QString();
}

/**
 * @brief 求值定义并入栈重算命令（创建/编辑/重算共用管线，D4：重算必须入栈）
 *
 * 流程：构建 DACommandDataFrame_evalColumn → 接线回调 → cmd->exec()（首次求值 +
 * 捕获旧列快照；失败 → 标失效 + 提示，不写数据 R10）→ push（首次 redo 跳过）。
 * 定义/状态联动经 directional 回调（与数据同一撤销单元）：
 * - 创建：undo 移除定义，redo 恢复定义（Valid）；
 * - 重算/编辑：undo 恢复原定义与原状态，redo 写入新定义并置 Valid。
 * @param def 定义（targetColumnName/expr/dependsOn 已就绪）
 * @param isNewColumn true=创建路径（目标列求值前不存在）
 * @return 成功入栈返回 true
 */
bool DATableComputedColumnController::applyEval(const DAComputedColumn& def, bool isNewColumn)
{
    DAPyDataFrame df = mCtx.df();
    DATableComputedColumnStore* ccStore = mCtx.computedColumnStore();
    if (df.isNone() || !ccStore) {
        return false;
    }
    auto cmd = std::make_unique< DACommandDataFrame_evalColumn >(df, def.targetColumnName, def.expr);
    QPointer< DADataTableModel > modle        = mCtx.model();
    DATableConditionalFormatStore* cfStore    = mCtx.conditionalFormatStore();
    QPointer< DATableComputedColumnController > self(this);
    const QString target = def.targetColumnName;
    // 编辑/重算场景：捕获原条目（定义+状态），undo 整体还原
    DATableComputedColumnStore::Entry prevEntry;
    const bool hasPrev = ccStore->entryOf(target, prevEntry);
    cmd->setCallBack([ modle, cfStore, self, target, isNewColumn ]() {
        if (modle) {
            if (isNewColumn) {
                // 创建/撤销创建都改变列结构 → 结构刷新（cacheShape + refreshWindow）
                modle->notifyColumnsInserted({ 0 });
            } else {
                // 重算/撤销重算只改列值
                const DAPyDataFrame mdf = modle->getData().toDataFrame();
                if (!mdf.isNone()) {
                    const QList< QString > cols = mdf.columns();
                    const int pos               = cols.indexOf(target);
                    if (pos >= 0) {
                        modle->notifyColumnChanged(pos);
                    }
                    if (cfStore) {
                        cfStore->invalidateColumnStats(pos);
                    }
                }
            }
        }
        if (self && self->mCtx.notifyDataModified) {
            self->mCtx.notifyDataModified();  // 重算改数据 → 标脏（dataModified 同源）
        }
    });
    cmd->setDirectionalCallBack([ ccStore, def, isNewColumn, hasPrev, prevEntry ](bool isUndo) {
        if (!ccStore) {
            return;
        }
        if (isNewColumn) {
            if (isUndo) {
                ccStore->removeDefinition(def.targetColumnName);
            } else {
                ccStore->addDefinition(def, DATableComputedColumnStore::State::Valid);
            }
        } else {
            if (isUndo) {
                if (hasPrev) {
                    ccStore->addDefinition(prevEntry.def, prevEntry.state);
                }
            } else {
                ccStore->addDefinition(def, DATableComputedColumnStore::State::Valid);
            }
        }
    });
    {
        DAWaitCursorScoped waitCursor;  // 求值可能长（大表整列计算），忙等待光标
        if (!cmd->exec()) {
            // 求值失败：标失效 + 提示，不静默清空列数据（R9/R10）
            if (!isNewColumn) {
                ccStore->setState(def.targetColumnName, DATableComputedColumnStore::State::Invalid);
            }
            daWarning << tr("Failed to evaluate the formula of computed column \"%1\": %2")
                             .arg(def.targetColumnName, cmd->lastErrorMessage());  // cn:计算列“%1”公式求值失败：%2
            return false;
        }
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
    return true;
}

/**
 * @brief 打开计算列对话框（col<0 新建；col>=0 编辑该列既有公式）
 *
 * 只读/惰性源不可写数据，直接拦截。接受后求值入栈（创建 = 一个撤销单元，
 * Ctrl+Z 删列 + 移除定义）。
 * @param col 列位置索引（编辑模式）
 */
void DATableComputedColumnController::showDialog(int col)
{
    DAPyDataFrame df = mCtx.df();
    DATableComputedColumnStore* ccStore = mCtx.computedColumnStore();
    if (df.isNone() || !ccStore) {
        // 开发诊断：非 dataframe（只读惰性表）或无元数据注册表
        qWarning() << "DATableComputedColumnController::showDialog: dataframe or metadata registry unavailable";
        return;
    }
    if (!mCtx.isDataSourceEditable()) {
        daWarning << tr("The table is read-only and does not support computed columns");  // cn:只读表格不支持计算列
        return;
    }
    // 编辑模式：解析目标列既有定义（非计算列 → 走新建流程，不阻塞用户）
    DAComputedColumn editDef;
    if (col >= 0) {
        const QString name = columnNameAt(col);
        DATableComputedColumnStore::Entry entry;
        if (!name.isEmpty() && ccStore->entryOf(name, entry)) {
            editDef = entry.def;
        }
    }
    DADialogComputedColumn dlg(df, mCtx.hostWidget());
    if (editDef.isValid()) {
        dlg.setEditMode(editDef);
    }
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    DAComputedColumn def(dlg.targetColumnName(), dlg.expressionBody());
    def.dependsOn = dlg.dependencies();
    if (def.isSelfReferencing()) {
        // 对话框已拦截，双保险（存储层校验，计划 §3.3-4）
        daWarning << tr("The formula must not reference the target column itself");  // cn:公式不得引用目标列自身
        return;
    }
    const bool isNew = !ccStore->hasDefinition(def.targetColumnName);
    applyEval(def, isNew);
}

/**
 * @brief 重算单个计算列（列头右键「重新计算」，R8 手动重算，入栈 D4）
 * @param col 列位置索引
 * @return 成功入栈返回 true
 */
bool DATableComputedColumnController::recompute(int col)
{
    DATableComputedColumnStore* ccStore = mCtx.computedColumnStore();
    if (!ccStore) {
        return false;
    }
    const QString name = columnNameAt(col);
    if (name.isEmpty()) {
        return false;
    }
    const DAComputedColumn def = ccStore->definition(name);
    if (!def.isValid()) {
        daWarning << tr("Column \"%1\" has no computed formula").arg(name);  // cn:列“%1”没有计算公式
        return false;
    }
    return applyEval(def, false);
}

/**
 * @brief 重算全部待重算/失效的计算列（Ribbon「全部重算」）
 *
 * 每列一个独立命令、合并进一个宏命令 = 一个撤销单元（Ctrl+Z 一次回退全部重算）。
 * 求值顺序按定义插入顺序（创建顺序天然满足「先算被依赖列」；编辑造成的逆序依赖
 * v1 不做拓扑排序，用户可再点一次全部重算收敛）。
 * @return 成功重算的列数
 */
int DATableComputedColumnController::recomputeAllStale()
{
    DATableComputedColumnStore* ccStore = mCtx.computedColumnStore();
    if (!ccStore || ccStore->isEmpty()) {
        return 0;
    }
    DAPyDataFrame df = mCtx.df();
    if (df.isNone() || !mCtx.isDataSourceEditable()) {
        return 0;
    }
    const QList< DATableComputedColumnStore::Entry > entries = ccStore->entries();
    QUndoCommand* macro = new QUndoCommand(tr("Recompute All Computed Columns"));  // cn:重算全部计算列
    int done            = 0;
    DAWaitCursorScoped waitCursor;  // 多列求值可能长，忙等待光标
    for (const DATableComputedColumnStore::Entry& e : entries) {
        if (!e.def.enabled || e.state == DATableComputedColumnStore::State::Valid) {
            continue;  // 只重算 Stale / Invalid
        }
        auto* cmd = new DACommandDataFrame_evalColumn(df, e.def.targetColumnName, e.def.expr, macro);
        QPointer< DADataTableModel > modle     = mCtx.model();
        DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore();
        QPointer< DATableComputedColumnController > self(this);
        const QString target                   = e.def.targetColumnName;
        const DATableComputedColumnStore::Entry prevEntry = e;
        cmd->setCallBack([ modle, cfStore, self, target ]() {
            if (modle) {
                const DAPyDataFrame mdf = modle->getData().toDataFrame();
                if (!mdf.isNone()) {
                    const QList< QString > cols = mdf.columns();
                    const int pos               = cols.indexOf(target);
                    if (pos >= 0) {
                        modle->notifyColumnChanged(pos);
                    }
                    if (cfStore) {
                        cfStore->invalidateColumnStats(pos);
                    }
                }
            }
            if (self && self->mCtx.notifyDataModified) {
                self->mCtx.notifyDataModified();  // 重算改数据 → 标脏
            }
        });
        cmd->setDirectionalCallBack([ ccStore, prevEntry ](bool isUndo) {
            if (ccStore) {
                ccStore->addDefinition(prevEntry.def, isUndo ? prevEntry.state : DATableComputedColumnStore::State::Valid);
            }
        });
        if (!cmd->exec()) {
            // 求值失败：标失效 + 提示，命令从宏中移除（析构即脱离父命令）
            ccStore->setState(e.def.targetColumnName, DATableComputedColumnStore::State::Invalid);
            daWarning << tr("Failed to evaluate the formula of computed column \"%1\": %2")
                             .arg(e.def.targetColumnName, cmd->lastErrorMessage());  // cn:计算列“%1”公式求值失败：%2
            delete cmd;
            continue;
        }
        ++done;
    }
    if (done == 0) {
        delete macro;
        return 0;
    }
    // 数据已在上面逐列 exec 完成；子命令均为 DACommandWithRedoCount（首次 redo 跳过），
    // push 宏命令不会重复求值
    if (mCtx.pushCommand) {
        mCtx.pushCommand(macro);
    }
    return done;
}

/**
 * @brief 移除某列的公式定义（保留列数据，列头右键「移除公式」；可撤销）
 * @param col 列位置索引
 * @return 成功入栈返回 true
 */
bool DATableComputedColumnController::removeFormula(int col)
{
    DATableComputedColumnStore* ccStore = mCtx.computedColumnStore();
    if (!ccStore) {
        return false;
    }
    const QString name = columnNameAt(col);
    if (name.isEmpty() || !ccStore->hasDefinition(name)) {
        return false;
    }
    auto cmd = std::make_unique< DACommandComputedColumn_removeDefinition >(ccStore, name);
    if (!cmd->isCaptured()) {
        return false;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());  // 普通 QUndoCommand：push 即执行 redo（移除定义）
    }
    return true;
}

/**
 * @brief 列改名前的计算列影响面确认（004 T1.23 R20）
 *
 * 统计引用了将被改名列（作依赖或自身是计算列目标）的定义数；>0 时提示
 * 「有 N 个计算列引用它，将随改名批量更新」，用户确认后继续。
 * @param renames 改名对（old→new）
 * @return 继续返回 true（无引用恒 true）
 */
bool DATableComputedColumnController::confirmRename(const QList< QPair< QString, QString > >& renames) const
{
    DATableComputedColumnStore* ccStore = mCtx.computedColumnStore();
    if (!ccStore || ccStore->isEmpty() || renames.isEmpty()) {
        return true;
    }
    QStringList affected;
    for (const auto& p : renames) {
        const QStringList refs = ccStore->definitionsReferencing(p.first);
        for (const QString& t : refs) {
            if (!affected.contains(t)) {
                affected.append(t);
            }
        }
        if (ccStore->hasDefinition(p.first) && !affected.contains(p.first)) {
            affected.append(p.first);
        }
    }
    if (affected.isEmpty()) {
        return true;
    }
    const auto answer = QMessageBox::question(
        mCtx.hostWidget(),
        tr("Rename Column"),  // cn:重命名列
        tr("%1 computed column(s) reference the renamed column(s): %2. Their references will be updated automatically. Continue?")
            .arg(affected.size())
            .arg(affected.join(QStringLiteral(", "))),  // cn:有 %1 个计算列引用被改名的列：%2。其引用将自动批量更新，是否继续？
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);
    return answer == QMessageBox::Yes;
}

/**
 * @brief 把计算列定义迁移挂进改名命令的 directional 回调（与改名同一撤销单元）
 * @param cmd 改名命令（exec 前接线）
 * @param renames 改名对（old→new）
 */
void DATableComputedColumnController::wireRenameMigration(DACommandDataFrame_renameColumns* cmd,
                                                         const QList< QPair< QString, QString > >& renames) const
{
    DATableComputedColumnStore* ccStore = mCtx.computedColumnStore();
    if (!cmd || !ccStore || renames.isEmpty()) {
        return;
    }
    cmd->setDirectionalCallBack([ ccStore, renames ](bool isUndo) {
        for (const auto& p : renames) {
            if (isUndo) {
                ccStore->renameColumnReferences(p.second, p.first);
            } else {
                ccStore->renameColumnReferences(p.first, p.second);
            }
        }
    });
}
}  // end of namespace DA
