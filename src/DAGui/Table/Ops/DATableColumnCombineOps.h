#ifndef DATABLECOLUMNCOMBINEOPS_H
#define DATABLECOLUMNCOMBINEOPS_H
#include "Table/Ops/DATableDataOperation.h"
#include <QObject>
#include <QString>

namespace DA
{
class DATablePageContext;
class DATableSelectionService;
class DATableMutationCoordinator;

/**
 * @brief 分列/合并列算子（008 期4，004 T1.9）
 *
 * showSplitColumnDialog：源列拆成多列，结果列插原列右侧、原列保留；
 * 撤销用列级记录（源列快照 + 结果列名清单，undo 删结果列写回快照，D4）。
 * showMergeColumnsDialog：多源列合并成一列，结果列替换被合并列位置；
 * 撤销持全部源列快照；被合并列被计算列依赖 → 失效并提示（R9）。
 * 后效：Structure commit（列位置位移 → 统计保守全失效）；位置键元数据
 * （样式/条件格式/批注）重映射在 directional 回调；计算列 undo 走整店快照恢复。
 * 生命周期：宿主页面构造并持有（QObject 父子）。
 */
class DAGUI_API DATableColumnCombineOps : public QObject, public DATableDataOperation
{
    Q_OBJECT
public:
    DATableColumnCombineOps(const DATablePageContext& ctx,
                            const DATableSelectionService& sel,
                            DATableMutationCoordinator* mutation,
                            QObject* parent = nullptr);

public:
    QString id() const override;
    QString title() const override;
    bool isAvailable() const override;

public:
    // 打开分列对话框并执行；col<0 时取当前选区的唯一数据列
    void showSplitColumnDialog(int col);
    // 打开合并列对话框并执行；源列 = 当前选区数据列（>=2）
    void showMergeColumnsDialog();

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    DATableMutationCoordinator* mMutation { nullptr };  ///< 编辑后效协调器（宿主持有，非拥有）
};
}  // end of namespace DA
#endif  // DATABLECOLUMNCOMBINEOPS_H
