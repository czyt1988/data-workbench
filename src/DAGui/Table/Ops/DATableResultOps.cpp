#include "Table/Ops/DATableResultOps.h"
#include "Table/Core/DATablePageContext.h"
// qt
#include <QMessageBox>
#include <QStringList>
#include <QWidget>
// DAGui
#include "Table/Models/DADataTableModel.h"
#include "Table/Metadata/DATableMetadata.h"
// DAData
#include "DAData.h"
#include "DADataPyDataFrame.h"
#include "DADataManager.h"
// 日志（用户可见提示必须走 da* 宏并翻译）
#include "DALogCategory.h"

namespace DA
{
namespace
{
// 就地替换撤销阈值（格）：Swap 命令旧对象驻留内存 ≈ 1× 表内存（§4.11 策略表），
// 单元格数超阈值时建议降级为「本次操作不可撤销」
constexpr qint64 c_inplaceUndoCellLimit = 50000000;
}

//===================================================
// DATableResultOps
//===================================================

DATableResultOps::DATableResultOps(const DATablePageContext& ctx, QObject* parent)
    : QObject(parent), mCtx(ctx)
{
}

QString DATableResultOps::id() const
{
    return QStringLiteral("result-ops");
}

QString DATableResultOps::title() const
{
    return tr("Result Operations");  // cn:结果落库
}

bool DATableResultOps::isAvailable() const
{
    return !mCtx.df().isNone();
}

/**
 * @brief 就地替换撤销内存判定（静态纯函数）
 *
 * Swap 命令旧对象驻留内存 ≈ 1× 表内存（§4.11 策略表）；单元格数超阈值时
 * 建议降级为「本次操作不可撤销」（二次确认后直接替换）。
 */
bool DATableResultOps::isInplaceUndoTooLarge(qint64 rows, qint64 cols)
{
    if (rows <= 0 || cols <= 0) {
        return false;
    }
    return rows * cols > c_inplaceUndoCellLimit;
}

/**
 * @brief 注册新数据集（分组聚合/转置/melt 默认路径共用）
 *
 * 照透视表 createPivotTable 的真实产出流程：命名（重名由 addData_ 的既有唯一化
 * 策略处理）→ DADataManager::addData_（可撤销注册，命令进管理器自己的撤销栈，
 * 已由 APP 接入统一撤销组）→ 发 newDataCreated 交 APP 置脏/抬区/激活新页面。
 */
void DATableResultOps::registerNewDataset(const DAPyDataFrame& resultDf, const QString& name, const QString& describe)
{
    if (resultDf.isNone()) {
        return;
    }
    DAData newData(resultDf);
    newData.setName(name);
    newData.setDescribe(describe);
    DADataManager* mgr = mCtx.data().getDataManager();
    if (!mgr) {
        // 开发诊断：无数据管理器（正常 APP 链路数据都来自管理器）
        qWarning() << "DATableResultOps::registerNewDataset: no data manager, result not registered";
        return;
    }
    mgr->addData_(newData);  // 可撤销注册（别用不带下划线的 addData，计划 §5.3-2/§6.3-1）
    Q_EMIT newDataCreated(newData);
}

/**
 * @brief 就地替换（004 T1.27 勾选「就地替换当前表」路径）
 *
 * 流程：① 确认框**列出将被清除的元数据类别与条目数**（§6.3-2 强制）→
 * ② 整束元数据 XML 快照 → ③ 超大表（单元格数 > c_inplaceUndoCellLimit）提示降级
 * 「本次操作不可撤销」，确认后直接替换 + 清束（§4.11 策略表）→ ④ 常规路径发
 * inplaceDataReplaceRequested 交 APP 经 DAAppCommand::beginDataOperateCommand
 * (isObjectPersist=false) 建 Swap 命令（统一撤销链，不在 DAGui new），元数据清理
 * 与数据替换经命令 directional 回调构成同一撤销单元。
 */
void DATableResultOps::requestInplaceReplace(const DAPyDataFrame& newDf, const QString& text)
{
    if (newDf.isNone()) {
        return;
    }
    DATableMetadata* md = mCtx.metadata();
    DAData data = mCtx.data();
    // ① 确认框：列出将被清除的元数据类别与条目数（不能只说一句"将清除样式"）
    QStringList lines;
    if (md) {
        const QList< QPair< QString, int > > counts = md->entryCountsByCategory();
        for (const auto& p : counts) {
            lines << QStringLiteral("%1: %2").arg(p.first).arg(p.second);
        }
    }
    const QString detail = lines.isEmpty() ? tr("(no table metadata to clear)")  // cn:（无需要清除的表元数据）
                                           : lines.join(QLatin1Char('\n'));
    const auto answer    = QMessageBox::question(
        mCtx.hostWidget(),
        tr("Replace In Place"),  // cn:就地替换
        tr("Replacing in place will clear the following table metadata:\n%1\n\nData and metadata share one undo unit (Ctrl+Z restores both). Continue?")
            .arg(detail),  // cn:就地替换将清除本表的以下元数据：\n%1\n\n数据与元数据共用一个撤销单元（Ctrl+Z 一并还原）。是否继续？
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    // ② 整束快照（undo 还原用）
    const QString snapshot = md ? md->snapshotToString() : QString();
    // ③ 超大表降级：旧对象驻留内存 ≈ 1× 表内存，超阈值提示「本次不可撤销」
    const DAPyDataFrame curDf = mCtx.df();
    qint64 rows               = 0;
    qint64 cols               = 0;
    if (!curDf.isNone()) {
        const auto s = curDf.shape();
        rows         = static_cast< qint64 >(s.first);
        cols         = static_cast< qint64 >(s.second);
    }
    if (isInplaceUndoTooLarge(rows, cols)) {
        const auto answer2 = QMessageBox::question(
            mCtx.hostWidget(),
            tr("Replace In Place"),  // cn:就地替换
            tr("The table is very large (%1 cells). Keeping the old data for undo would double memory usage, so this in-place replacement will NOT be undoable. Continue?")
                .arg(rows * cols),  // cn:表格过大（%1 单元格）。保留旧数据用于撤销将使内存翻倍，本次就地替换将**不可撤销**。是否继续？
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer2 != QMessageBox::Yes) {
            return;
        }
        data.setPyObject(newDf.object());
        if (md) {
            md->clearAll();
        }
        if (mCtx.refreshAfterInplaceReplace) {
            mCtx.refreshAfterInplaceReplace();  // 整页刷新（模型 reset + 清排序规格，宿主桥）
        }
        Q_EMIT dataModified();
        return;
    }
    // ④ 可撤销路径：交 APP 建 Swap 命令（统一撤销链）
    DAData source(newDf);
    Q_EMIT inplaceDataReplaceRequested(data, source, text, snapshot);
}
}  // end of namespace DA
