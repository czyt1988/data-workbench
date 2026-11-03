#include "Table/Ops/DATableReshapeOps.h"
#include "Table/Core/DATablePageContext.h"
#include "Table/Ops/DATableResultOps.h"
// qt
#include <QDialog>
#include <QWidget>
// DAGui
#include "Table/Dialogs/DADialogGroupBy.h"
#include "Table/Dialogs/DADialogReshape.h"
// DAData
#include "DADataPyDataFrame.h"
// python 算子封装（转置/melt）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"
// 忙等待光标（全表变换）
#include "DAWaitCursorScoped.h"

namespace DA
{
//===================================================
// DATableReshapeOps
//===================================================

DATableReshapeOps::DATableReshapeOps(const DATablePageContext& ctx, DATableResultOps* resultOps, QObject* parent)
    : QObject(parent), mCtx(ctx), mResultOps(resultOps)
{
}

QString DATableReshapeOps::id() const
{
    return QStringLiteral("reshape-ops");
}

QString DATableReshapeOps::title() const
{
    return tr("Reshape Operations");  // cn:形状变换
}

bool DATableReshapeOps::isAvailable() const
{
    return !mCtx.df().isNone();
}

/**
 * @brief 打开分组聚合对话框并执行（004 T1.22）
 *
 * 产出长格式聚合结果表注册为**新数据集**（非破坏，R14：原表与其元数据完全不动，
 * 不做折叠视图 R15）。产出流程照透视表的真实流程：命名 xxx_GroupBy（重名由
 * addData_ 的既有唯一化策略处理）→ addData_ 可撤销注册 → 发 newDataCreated
 * 交 APP 抬起数据区并激活新页面（全部经 ResultOps 落库）。
 */
void DATableReshapeOps::showGroupByDialog()
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        // 开发诊断：非 dataframe（只读惰性表无 python 数据体）
        qWarning() << "DATableReshapeOps::showGroupByDialog: dataframe unavailable";
        return;
    }
    DADialogGroupBy dlg(df, mCtx.hostWidget());
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    DAPyDataFrame result = dlg.result();
    if (result.isNone()) {
        return;
    }
    // 注册新数据集（addData_ 可撤销 + newDataCreated 交 APP 激活新页面，与转置/melt 共用管线）
    if (mResultOps) {
        mResultOps->registerNewDataset(result,
                                       tr("%1_GroupBy").arg(mCtx.data().getName()),                  // cn:%1_分组聚合
                                       tr("Group-by aggregation of %1").arg(mCtx.data().getName()));  // cn:生成%1的分组聚合结果
    }
}

/**
 * @brief 打开转置/宽转长对话框并执行（004 T1.27）
 *
 * 默认产出新数据集（R14），命名 xxx_Transposed / xxx_Melted；勾选「就地替换」走
 * ResultOps 的就地替换路径（元数据确认 + 整束快照 + 超大表降级不可撤销 / Swap 命令）。
 * @param meltMode true=宽转长（melt）初始模式，false=转置初始模式（对话框内可切换）
 */
void DATableReshapeOps::showReshapeDialog(bool meltMode)
{
    DAPyDataFrame df = mCtx.df();
    if (df.isNone()) {
        // 开发诊断：非 dataframe（只读惰性表无 python 数据体）
        qWarning() << "DATableReshapeOps::showReshapeDialog: dataframe unavailable";
        return;
    }
    DADialogReshape dlg(df, meltMode, mCtx.hostWidget());
    if (QDialog::Accepted != dlg.exec()) {
        return;
    }
    DAPyScriptsDataFrame& py = DAPyScripts::getDataFrame();
    QString err;
    DAPyDataFrame result;
    {
        DAWaitCursorScoped waitCursor;  // 全表变换忙等待
        if (dlg.isMeltMode()) {
            result = py.melt(df, dlg.idVars(), dlg.valueVars(), dlg.varName(), dlg.valueName(), &err);
        } else {
            result = py.transpose(df, &err);
        }
    }
    if (result.isNone()) {
        daWarning << tr("Reshape failed: %1").arg(err);  // cn:形状变换失败：%1
        return;
    }
    if (!mResultOps) {
        return;
    }
    if (dlg.isInplace()) {
        mResultOps->requestInplaceReplace(result,
                                          dlg.isMeltMode() ? tr("Melt table in place")     // cn:就地宽转长
                                                           : tr("Transpose table in place"));  // cn:就地转置
        return;
    }
    // 默认产出新数据集（R14），命名 xxx_Transposed / xxx_Melted（与 _PivotTable/_GroupBy 对称）
    if (dlg.isMeltMode()) {
        mResultOps->registerNewDataset(result, tr("%1_Melted").arg(mCtx.data().getName()), tr("Melt (wide to long) of %1").arg(mCtx.data().getName()));  // cn:%1_宽转长 / 生成%1的宽转长结果
    } else {
        mResultOps->registerNewDataset(result, tr("%1_Transposed").arg(mCtx.data().getName()), tr("Transpose of %1").arg(mCtx.data().getName()));  // cn:%1_转置 / 生成%1的转置结果
    }
}
}  // end of namespace DA
