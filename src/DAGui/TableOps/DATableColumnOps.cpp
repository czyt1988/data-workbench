#include "DATableColumnOps.h"
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"
#include "TableCore/DATableMutationCoordinator.h"
#include "TableViewState/DATableViewStateController.h"
// qt
#include <QDialog>
#include <QDomDocument>
#include <QDomElement>
#include <QMessageBox>
#include <QPointer>
#include <QStringList>
#include <QWidget>
// DAGui
#include "Models/DADataTableModel.h"
#include "Commands/DACommandsDataFrame.h"
#include "DATableStyleManager.h"
#include "DATableNoteStore.h"
#include "DATableConditionalFormatStore.h"
#include "DATableComputedColumnStore.h"
#include "DATableViewStateStore.h"
#include "Dialog/DADialogInsertNewColumn.h"
// DAData
#include "DADataPyDataFrame.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

#include <memory>

namespace DA
{
//===================================================
// DATableColumnOps
//===================================================

DATableColumnOps::DATableColumnOps(const DATablePageContext& ctx,
                                 const DATableSelectionService& sel,
                                 DATableMutationCoordinator* mutation,
                                 DATableViewStateController* viewState,
                                 QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel), mMutation(mutation), mViewState(viewState)
{
}

QString DATableColumnOps::id() const
{
    return QStringLiteral("column-ops");
}

QString DATableColumnOps::title() const
{
    return tr("Column Operations");  // cn:列操作
}

bool DATableColumnOps::isAvailable() const
{
    return !mCtx.df().isNone() && mCtx.isDataSourceEditable();
}

/**
 * @brief 在 col 位置插入新列（插列对话框：范围/默认值两模式，可撤销）
 *
 * 后效走 Structure commit（插列使列位置整体后移 → 全量统计失效 + 计算列全标脏）；
 * 样式/条件格式/批注位置键重映射（>=col 的键 +1；undo 按删除处理）在 directional 回调。
 * @param col 插入位置
 */
void DATableColumnOps::insertColumnAt(int col)
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        // 非dataframe数据（如只读的数据库惰性表）不支持插入列
        return;
    }
    DADialogInsertNewColumn dlg(mCtx.hostWidget());
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    std::unique_ptr< DACommandDataFrame_insertColumn > cmd;
    QString name = dlg.getName();
    if (name.isEmpty()) {
        QMessageBox::warning(mCtx.hostWidget(),
                            tr("Warning"),                                                    // cn:警告
                            tr("The name of the new column to be inserted must be specified")  // cn:必须指定列的名字
        );
        return;
    }
    DAPyDType dt = dlg.getDType();
    if (dlg.isRangeMode()) {
        cmd.reset(new DACommandDataFrame_insertColumn(df, col, name, dlg.getStartValue(), dlg.getStopValue()));
    } else {
        cmd.reset(new DACommandDataFrame_insertColumn(df, col, name, dlg.getDefaultValue()));
    }
    QPointer< DADataTableModel > modle = mCtx.model();
    // 编辑后效（008 期2）：插列使列位置整体后移 → 全量统计失效 + 计算列全标脏（策略表：列增删）
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Structure;
    mutation.modelRefresh = [ modle, col ]() {
        if (modle) {
            // 此操作会删除一列，添加一列，整个 modelreflash（沿用旧通知形状）
            modle->notifyColumnsRemoved({ col });
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    // 行列增删样式键同步：插入列时 >=col 的键 +1；撤销时按删除处理
    // 条件格式规则同为装饰类位置键（R20），随列插删同款重映射（003）
    DATableStyleManager* styleMgr          = mCtx.styleManager();
    DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore();
    DATableNoteStore* noteStore            = mCtx.noteStore();
    cmd->setDirectionalCallBack([ styleMgr, cfStore, noteStore, col ](bool isUndo) {
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onColumnsRemoved({ col });
            } else {
                styleMgr->onColumnsInserted({ col });
            }
        }
        if (cfStore) {
            if (isUndo) {
                cfStore->onColumnsRemoved({ col });
            } else {
                cfStore->onColumnsInserted({ col });
            }
        }
        // 批注列维同款重映射（005 T1.26）
        if (noteStore) {
            if (isUndo) {
                noteStore->onColumnsRemoved({ col });
            } else {
                noteStore->onColumnsInserted({ col });
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
 * @brief 选中列右边插入新列
 */
void DATableColumnOps::insertColumnRightBySelect()
{
    int c = mSel.oneSelectedDataframeColumn();
    if (c < 0) {
        return;
    }
    insertColumnAt(c + 1);
}

/**
 * @brief 选中列左边插入新列
 */
void DATableColumnOps::insertColumnLeftBySelect()
{
    int c = mSel.oneSelectedDataframeColumn();
    if (c < 0) {
        return;
    }
    insertColumnAt(c);
}

/**
 * @brief 移除选中的列
 */
int DATableColumnOps::removeSelectColumn()
{
    QList< int > columns = mSel.selectedDataframeColumns();
    if (columns.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return 0;
    }
    return removeColumns(columns);
}

/**
 * @brief 移除指定列（表头右键「删除列」直接入口）
 *
 * 与 removeSelectColumn 的差异：不依赖选区——右键菜单弹出瞬间焦点被菜单窗口
 * 抢走，APP 侧 getCurrentDataFrameOperateWidget 的焦点检查会失败，选区也不一定
 * 在目标列上；本入口直接按列号操作本页。
 * 冻结列被删除时冻结集合同步剔除（主区列不补位），全部冻结列删光后
 * 冻结窗格随之隐藏（removeColumns 内投影收敛）。
 * @param col 列位置索引
 * @return 成功移除的列数（0 = 失败/列号无效）
 */
int DATableColumnOps::removeColumnAt(int col)
{
    if (col < 0 || col >= mCtx.dataColumnCount()) {
        return 0;
    }
    return removeColumns(QList< int >({ col }));
}

/**
 * @brief 按列号列表删除列（removeSelectColumn / removeColumnAt 共用命令管线）
 *
 * 命令：DACommandDataFrame_dropIColumn（临时 pickle 快照，undo 整表还原）。
 * 联动（对照重命名路径补全）：
 * 1) 视图状态（列宽/列序/隐藏/冻结）：删除前整店 XML 快照，redo 清被删列条目
 *    （冻结集合随 removeColumn 一并剔除），undo 从快照整店还原（同一撤销单元）；
 * 2) 数据管理器树：notifyHostValueChange 重建列子项；
 * 3) 样式/条件格式/批注/计算列重映射与失效提示：与旧 removeSelectColumn 一致。
 * @param columns 列位置索引列表
 * @return 成功移除的列数
 */
int DATableColumnOps::removeColumns(const QList< int >& columns)
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return 0;
    }
    if (columns.isEmpty()) {
        return 0;
    }
    // —— 删除前解析：被删列名（视图状态按名字清条目，冻结集合随之剔除） ——
    QStringList removedNames;
    const QList< QString > colNames = df.columns();
    for (int c : columns) {
        if (c >= 0 && c < colNames.size()) {
            removedNames.append(colNames.at(c));
        }
    }
    // 视图状态整店快照（undo 还原；QDomDocument 按引用持有，lambda 里 shared_ptr 保活）
    auto snapDoc = std::make_shared< QDomDocument >();
    QDomElement snapRoot = snapDoc->createElement(QStringLiteral("snapshot"));
    snapDoc->appendChild(snapRoot);  // 必须挂进文档，否则 documentElement() 读不到
    if (DATableViewStateStore* vsStore = mCtx.viewStateStore()) {
        vsStore->toXml(*snapDoc, snapRoot);
    }
    std::unique_ptr< DACommandDataFrame_dropIColumn > cmd(new DACommandDataFrame_dropIColumn(df, columns));
    // 编辑后效（008 期2）：结构通知 + 全量统计失效 + 计算列全标脏 + 关浮窗 + 宿主树刷新，
    // 收口到 commit（undo/redo 同走此回调；宿主树刷新随 undo 同步——列子项随 undo 一致重建）
    DATableMutation mutation;
    mutation.kind                  = DATableMutation::Structure;
    mutation.closeQuickPlot        = true;   // 删列后浮窗绑定的列身份失效，直接关闭（实施方案 §5.3）
    mutation.notifyHostValueChange = true;  // 数据管理器树重建列子项
    QPointer< DADataTableModel > modle = mCtx.model();
    mutation.modelRefresh           = [ modle, columns ]() {
        if (modle) {
            modle->notifyColumnsRemoved(columns);
        }
    };
    cmd->setCallBack(mMutation->callbackFor(mutation));
    // 行列增删样式键同步：删除列时 ==col 的键删除，>col 的键 -1；撤销时按插入处理
    // 注意：被删除列的样式在删除时丢失，撤销(load恢复dataframe)无法恢复样式（已知限制，见 spec §7.3）
    // 条件格式规则同为装饰类位置键（R20），随列删除同款重映射（被删列的规则一并丢弃，003）
    DATableStyleManager* styleMgr          = mCtx.styleManager();
    DATableConditionalFormatStore* cfStore = mCtx.conditionalFormatStore();
    DATableNoteStore* noteStore            = mCtx.noteStore();
    // 计算列联动（004 T1.23 R20）：被删列名在 exec 前解析；定义集合先快照——
    // undo 整店恢复（含被移除的定义与原状态），redo 重放删列联动
    //（目标列被删 → 定义移除；依赖列被删 → 置 Invalid，不静默清空列数据）
    DATableComputedColumnStore* ccStore    = mCtx.computedColumnStore();
    DATableViewStateStore* viewStore       = mCtx.viewStateStore();
    QList< DATableComputedColumnStore::Entry > ccSnapshot;
    if (ccStore) {
        ccSnapshot = ccStore->snapshot();
    }
    auto invalidatedTargets = std::make_shared< QStringList >();
    DATableViewStateController* viewState = mViewState;
    cmd->setDirectionalCallBack([ styleMgr, cfStore, noteStore, ccStore, viewStore, snapDoc, columns,
                                   removedNames, ccSnapshot, invalidatedTargets, viewState ](bool isUndo) {
        if (styleMgr) {
            if (isUndo) {
                styleMgr->onColumnsInserted(columns);
            } else {
                styleMgr->onColumnsRemoved(columns);
            }
        }
        if (cfStore) {
            if (isUndo) {
                cfStore->onColumnsInserted(columns);
            } else {
                cfStore->onColumnsRemoved(columns);
            }
        }
        // 批注列维同款重映射（005 T1.26，被删列的批注一并丢弃）
        if (noteStore) {
            if (isUndo) {
                noteStore->onColumnsInserted(columns);
            } else {
                noteStore->onColumnsRemoved(columns);
            }
        }
        if (ccStore) {
            if (isUndo) {
                ccStore->restore(ccSnapshot);
            } else {
                invalidatedTargets->clear();
                ccStore->onColumnsRemoved(removedNames, nullptr, invalidatedTargets.get());
            }
        }
        // 视图状态随命令往返：redo 清被删列条目（含冻结集合）；undo 整店快照还原
        if (viewStore) {
            if (isUndo) {
                const QDomElement vs = snapDoc->documentElement().firstChildElement(QStringLiteral("view-state"));
                if (!vs.isNull()) {
                    viewStore->fromXml(vs);
                } else {
                    viewStore->clear();
                }
            } else {
                for (const QString& n : removedNames) {
                    viewStore->removeColumn(n);
                }
            }
        }
        if (viewState) {
            viewState->applyFromStore();  // 冻结投影收敛（冻结列删光 → 窗格隐藏）
        }
    });
    if (!cmd->exec()) {
        return 0;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
    // 浮窗关闭与宿主树刷新已随 commit（exec 内）执行；此处只剩用户提示
    // 删列后提示失效的计算列（exec 内的 directional 回调已填充；不静默，R9）
    if (!invalidatedTargets->isEmpty()) {
        daWarning << tr("Computed column(s) %1 became invalid because their dependency column(s) were removed")
                         .arg(invalidatedTargets->join(QStringLiteral(", ")));  // cn:计算列 %1 因依赖列被删除已失效
    }
    return columns.size();
}
}  // end of namespace DA
