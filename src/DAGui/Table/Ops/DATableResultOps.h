#ifndef DATABLERESULTOPS_H
#define DATABLERESULTOPS_H
#include "Table/Ops/DATableDataOperation.h"
#include <QObject>
#include <QString>

namespace DA
{
class DAPyDataFrame;
class DAData;
class DATablePageContext;

/**
 * @brief 计算结果落库算子（008 期4：新数据集注册 / 就地替换）
 *
 * 对话框算子（分组聚合/转置/melt）产出新 dataframe 后的公共落库段：
 * registerNewDataset 照透视表流程注册为新数据集（可撤销）并发 newDataCreated；
 * requestInplaceReplace 勾选「就地替换」路径（确认框列元数据条目数 →
 * 整束快照 → 超大表降级不可撤销 / 常规发 inplaceDataReplaceRequested 交 APP
 * 建 Swap 命令）。isInplaceUndoTooLarge 为静态纯函数（单测覆盖）。
 * 生命周期：宿主页面构造并持有（QObject 父子）；信号经页面转发（APP 连接点不变）。
 */
class DAGUI_API DATableResultOps : public QObject, public DATableDataOperation
{
    Q_OBJECT
public:
    explicit DATableResultOps(const DATablePageContext& ctx, QObject* parent = nullptr);

public:
    QString id() const override;
    QString title() const override;
    bool isAvailable() const override;

    // 就地替换撤销内存判定（静态纯函数）：单元格数超阈值建议降级不可撤销
    static bool isInplaceUndoTooLarge(qint64 rows, qint64 cols);

public:
    // 注册新数据集（命名 → addData_ 可撤销注册 → 发 newDataCreated 交 APP 激活新页面）
    void registerNewDataset(const DAPyDataFrame& resultDf, const QString& name, const QString& describe);
    // 就地替换（确认框 → 快照 → 超大表降级 / 发 inplaceDataReplaceRequested 交 APP 建 Swap 命令）
    void requestInplaceReplace(const DAPyDataFrame& newDf, const QString& text);

Q_SIGNALS:
    // 新数据集已注册（页面转发 newDataCreated → APP 置脏/抬区/激活新页面）
    void newDataCreated(const DA::DAData& newData);
    // 就地替换请求（页面转发 inplaceDataReplaceRequested → APP 建 Swap 命令）
    void inplaceDataReplaceRequested(const DA::DAData& target, const DA::DAData& source, const QString& text, const QString& metadataSnapshot);
    // 数据已修改（超大表降级路径直改数据后标脏；页面转发 dataModified）
    void dataModified();

private:
    const DATablePageContext& mCtx;  ///< 依赖注入的上下文（构造注入，非拥有）
};
}  // end of namespace DA
#endif  // DATABLERESULTOPS_H
