#ifndef DATABLEOPERATIONREGISTRY_H
#define DATABLEOPERATIONREGISTRY_H
#include "DAGuiAPI.h"
#include <QHash>
#include <QList>
#include <QString>

namespace DA
{
class DATableDataOperation;
class DATablePageContext;
class DATableSelectionService;
class DATableMutationCoordinator;
class DATableViewStateController;
class DATableComputedColumnController;
class DATableValidationController;
class DATableResultOps;

/**
 * @brief 表格数据操作注册表（008 期4）
 *
 * id → 唯一算子实例。对话框类算子将来可经 runOperation("split-column") 按 id 调用
 * （Agent/插件的 id 驱动入口）；当前 Facade 的参数化入口直调具体算子的具名方法，
 * Registry 作为 id → 实例的唯一登记处（菜单启用规则、能力枚举共用 id()/isAvailable()）。
 * 生命周期：宿主页面构造并持有；算子实例的 QObject 父子归页面。
 */
class DAGUI_API DATableOperationRegistry
{
public:
    // 注册一个算子（接管所有权不发生；实例由页面以 QObject 父子持有，此处只登记指针）
    void add(DATableDataOperation* op);
    // 按 id 查找（无则 nullptr）
    DATableDataOperation* find(const QString& id) const;
    // 全部已注册算子（菜单/能力枚举用）
    QList< DATableDataOperation* > all() const;

private:
    QList< DATableDataOperation* > mOps;               ///< 登记顺序保持
    QHash< QString, DATableDataOperation* > mById;      ///< id → 实例
};
}  // end of namespace DA
#endif  // DATABLEOPERATIONREGISTRY_H
