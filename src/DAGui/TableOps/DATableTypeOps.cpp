#include "DATableTypeOps.h"
#include "TableCore/DATablePageContext.h"
#include "TableCore/DATableSelectionService.h"
#include "TableCore/DATableMutationCoordinator.h"
// qt
#include <QDialog>
#include <QPointer>
// DAGui
#include "Models/DADataTableModel.h"
#include "Commands/DACommandsDataFrame.h"
#include "Dialog/DADialogDataframeColumnCastToNumeric.h"
#include "Dialog/DADialogDataframeColumnCastToDatetime.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

#include <memory>

namespace DA
{
//===================================================
// DATableTypeOps
//===================================================

DATableTypeOps::DATableTypeOps(const DATablePageContext& ctx,
                               const DATableSelectionService& sel,
                               DATableMutationCoordinator* mutation,
                               QObject* parent)
    : QObject(parent), mCtx(ctx), mSel(sel), mMutation(mutation)
{
}

QString DATableTypeOps::id() const
{
    return QStringLiteral("type-ops");
}

QString DATableTypeOps::title() const
{
    return tr("Type Operations");  // cn:类型操作
}

bool DATableTypeOps::isAvailable() const
{
    return !mCtx.df().isNone();
}

/**
 * @brief 设置选择列的数据类型（astype，可撤销）
 *
 * 类型转换改整列值 → 受影响列统计失效 + 计算列按列标脏（Values commit）。
 * @param dt 目标类型
 * @return 成功改变类型返回 true
 */
bool DATableTypeOps::changeSelectColumnType(const DAPyDType& dt)
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        Q_EMIT selectTypeChanged({ }, DAPyDType());
        return false;
    }
    QList< int > selColumns = mSel.selectedDataframeColumns();
    if (selColumns.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        Q_EMIT selectTypeChanged({ }, DAPyDType());
        return false;
    }
    std::unique_ptr< DACommandDataFrame_astype > cmd(new DACommandDataFrame_astype(df, selColumns, dt));
    // 编辑后效（008 期2）：类型转换改整列值 → 受影响列统计失效 + 计算列标脏，收口到 commit
    cmd->setCallBack(mMutation->callbackFor(columnValuesMutation(selColumns)));
    if (!cmd->exec()) {
        // 说明没有设置成功
        Q_EMIT selectTypeChanged({ }, DAPyDType());
        return false;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());  // 成功，push 会执行 redo 但会跳过
    }
    Q_EMIT selectTypeChanged(selColumns, dt);
    return true;
}

/**
 * @brief 把选择的列转换为数值（对话框参数，可撤销）
 *
 * 数值转换改整列值 → 受影响列统计失效 + 计算列按列标脏；转换后类型有变化时上报。
 */
void DATableTypeOps::castSelectToNum()
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return;
    }
    QList< int > colsIndex = mSel.selectedDataframeColumns();
    if (colsIndex.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    if (mDialogCastNumArgs == nullptr) {
        mDialogCastNumArgs = new DADialogDataframeColumnCastToNumeric(mCtx.hostWidget());
    }
    if (QDialog::Accepted != mDialogCastNumArgs->exec()) {
        return;
    }
    DAPyDType dt        = df.dtypeObject(colsIndex.first());
    pybind11::dict args = mDialogCastNumArgs->getArgs();
    std::unique_ptr< DACommandDataFrame_castNum > cmd(new DACommandDataFrame_castNum(df, colsIndex, args));
    // 编辑后效（008 期2）：数值转换改整列值 → 受影响列统计失效 + 计算列标脏，收口到 commit
    cmd->setCallBack(mMutation->callbackFor(columnValuesMutation(colsIndex)));
    if (!cmd->exec()) {
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());  // 推入后不会执行 redo 逻辑部分
    }
    DAPyDType dt2 = df.dtypeObject(colsIndex.first());
    if (dt != dt2) {
        Q_EMIT selectTypeChanged(colsIndex, dt2);
    }
}

/**
 * @brief 把选择的列转换为日期（对话框参数，可撤销）
 *
 * 日期转换改整列值 → 受影响列统计失效 + 计算列按列标脏；转换后类型有变化时上报。
 */
void DATableTypeOps::castSelectToDatetime()
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return;
    }
    QList< int > colsIndex = mSel.selectedDataframeColumns();
    if (colsIndex.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return;
    }
    if (mDialogCastDatetimeArgs == nullptr) {
        mDialogCastDatetimeArgs = new DADialogDataframeColumnCastToDatetime(mCtx.hostWidget());
    }
    if (QDialog::Accepted != mDialogCastDatetimeArgs->exec()) {
        return;
    }
    DAPyDType dt        = df.dtypeObject(colsIndex.first());
    pybind11::dict args = mDialogCastDatetimeArgs->getArgs();
    std::unique_ptr< DACommandDataFrame_castDatetime > cmd(new DACommandDataFrame_castDatetime(df, colsIndex, args));
    // 编辑后效（008 期2）：日期转换改整列值 → 受影响列统计失效 + 计算列标脏，收口到 commit
    cmd->setCallBack(mMutation->callbackFor(columnValuesMutation(colsIndex)));
    if (!cmd->exec()) {
        return;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());
    }
    DAPyDType dt2 = df.dtypeObject(colsIndex.first());
    if (dt != dt2) {
        Q_EMIT selectTypeChanged(colsIndex, dt2);
    }
}

/**
 * @brief 把选择的列转换为索引（结构性整表操作，可撤销）
 *
 * 列转索引 → 模型 reset + 全量统计失效 + 计算列全标脏（FullReset commit）。
 * @return 成功返回 true
 */
bool DATableTypeOps::changeSelectColumnToIndex()
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        return false;
    }
    QList< int > colsIndex = mSel.selectedDataframeColumns();
    if (colsIndex.size() <= 0) {
        daWarning << tr("Please select a valid column");  // cn:请选择正确的列
        return false;
    }
    std::unique_ptr< DACommandDataFrame_setIndex > cmd = std::make_unique< DACommandDataFrame_setIndex >(df, colsIndex);
    // 编辑后效（008 期2）：列转索引是结构性整表操作 → 模型 reset + 全量失效 +
    // 计算列全标脏，收口到 commit（FullReset）
    DATableMutation mutation;
    mutation.kind = DATableMutation::FullReset;
    cmd->setCallBack(mMutation->callbackFor(mutation));
    if (!cmd->exec()) {
        return false;
    }
    if (mCtx.pushCommand) {
        mCtx.pushCommand(cmd.release());  // 推入后不会执行 redo 逻辑部分
    }
    return true;
}

/**
 * @brief 整列值变更的公共后效构造（Values commit + 整列通知形状）
 *
 * @param cols 受影响列
 * @return Values 变更描述（modelRefresh = 逐列 notifyColumnChanged）
 */
DATableMutation DATableTypeOps::columnValuesMutation(const QList< int >& cols) const
{
    QPointer< DADataTableModel > modle = mCtx.model();
    DATableMutation mutation;
    mutation.kind        = DATableMutation::Values;
    mutation.changedCols = cols;
    mutation.modelRefresh = [ modle, cols ]() {
        if (modle) {
            for (int c : std::as_const(cols)) {
                modle->notifyColumnChanged(c);
            }
        }
    };
    return mutation;
}
}  // end of namespace DA
