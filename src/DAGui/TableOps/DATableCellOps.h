#ifndef DATABLECELLOPS_H
#define DATABLECELLOPS_H
#include "TableOps/DATableDataOperation.h"
#include <QList>
#include <QObject>
#include <QString>

namespace DA
{
class DATablePageContext;
class DATableSelectionService;
class DATableMutationCoordinator;

/**
 * @brief 单元格值操作算子（008 期4：置空/粘贴/剪切）
 *
 * removeSelectedCells：选中区置 NaN（DACommandDataFrame_setnan，可撤销）；
 * pasteFromClipboard：剪贴板 TSV 解析 → 从选区左上角起填充（越界丢弃提示 D9）；
 * cutSelection：复制 + 置 NaN。后效走 Values commit（模型逐格刷新 +
 * 受影响列统计失效 + 计算列标脏）。
 * 生命周期：宿主页面构造并持有（QObject 父子）。
 */
class DAGUI_API DATableCellOps : public QObject, public DATableDataOperation
{
    Q_OBJECT
public:
    DATableCellOps(const DATablePageContext& ctx,
                   const DATableSelectionService& sel,
                   DATableMutationCoordinator* mutation,
                   QObject* parent = nullptr);

public:
    QString id() const override;
    QString title() const override;
    bool isAvailable() const override;

public:
    // 选中单元格置 NaN（可撤销）；返回设置成功的个数
    int removeSelectedCells();
    // 粘贴剪贴板内容到当前选区左上角（越界丢弃并提示）；返回写入单元格数
    int pasteFromClipboard();
    // 剪切选中区（复制 + 置 NaN）；返回受影响单元格数
    int cutSelection();

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    DATableMutationCoordinator* mMutation { nullptr };  ///< 编辑后效协调器（宿主持有，非拥有）
};
}  // end of namespace DA
#endif  // DATABLECELLOPS_H
