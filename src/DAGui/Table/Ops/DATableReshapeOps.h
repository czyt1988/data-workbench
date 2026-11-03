#ifndef DATABLERESHAPEOPS_H
#define DATABLERESHAPEOPS_H
#include "Table/Ops/DATableDataOperation.h"
#include <QObject>
#include <QString>

namespace DA
{
class DAPyDataFrame;
class DATablePageContext;
class DATableResultOps;

/**
 * @brief 形状变换算子（008 期4：分组聚合/转置/宽转长）
 *
 * showGroupByDialog：聚合结果注册为新数据集（非破坏，R14）；
 * showReshapeDialog：转置/melt，默认产出新数据集，勾选「就地替换」走
 * ResultOps 的就地替换路径（元数据确认 + 快照 + 超大表降级）。
 * 求值全交 Python 算子（da_groupby_aggregate/da_transpose/da_melt）。
 * 生命周期：宿主页面构造并持有（QObject 父子）；落库经 ResultOps。
 */
class DAGUI_API DATableReshapeOps : public QObject, public DATableDataOperation
{
    Q_OBJECT
public:
    DATableReshapeOps(const DATablePageContext& ctx, DATableResultOps* resultOps, QObject* parent = nullptr);

public:
    QString id() const override;
    QString title() const override;
    bool isAvailable() const override;

public:
    // 打开分组聚合对话框：产出长格式聚合结果表注册为新数据集（xxx_GroupBy）
    void showGroupByDialog();
    // 打开转置/宽转长对话框：默认产出新数据集；勾选「就地替换」走 ResultOps 就地路径
    void showReshapeDialog(bool meltMode);

private:
    const DATablePageContext& mCtx;              ///< 依赖注入的上下文（构造注入，非拥有）
    DATableResultOps* mResultOps { nullptr };    ///< 结果落库算子（宿主持有，非拥有）
};
}  // end of namespace DA
#endif  // DATABLERESHAPEOPS_H
