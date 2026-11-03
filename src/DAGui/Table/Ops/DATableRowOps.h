#ifndef DATABLEROWOPS_H
#define DATABLEROWOPS_H
#include "Table/Ops/DATableDataOperation.h"
#include <QList>
#include <QObject>
#include <QString>

namespace DA
{
class DATablePageContext;
class DATableSelectionService;
class DATableMutationCoordinator;

/**
 * @brief 行操作算子（008 期4：插行/删行）
 *
 * 参数化入口（菜单/右键携带行号或选区解析）：insertRowAt/removeSelectedRows。
 * 插删行是拿不到列信息的整表操作 → 后效走 Structure commit（全量统计失效 +
 * 计算列全标脏）；样式/批注位置键重映射在命令的 directional 回调内
 * （与数据同一撤销单元）。
 * 生命周期：宿主页面构造并持有（QObject 父子）。
 */
class DAGUI_API DATableRowOps : public QObject, public DATableDataOperation
{
    Q_OBJECT
public:
    DATableRowOps(const DATablePageContext& ctx,
                  const DATableSelectionService& sel,
                  DATableMutationCoordinator* mutation,
                  QObject* parent = nullptr);

public:
    QString id() const override;
    QString title() const override;
    bool isAvailable() const override;

public:
    // 在 row 位置插入一个 NaN 行（可撤销；非 dataframe 数据静默忽略）
    void insertRowAt(int row);
    // 删除选中的行（数据源绝对行号；可撤销）；返回成功移除的行数
    int removeSelectedRows();

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    DATableMutationCoordinator* mMutation { nullptr };  ///< 编辑后效协调器（宿主持有，非拥有）
};
}  // end of namespace DA
#endif  // DATABLEROWOPS_H
