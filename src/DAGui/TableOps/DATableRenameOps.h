#ifndef DATABLERENAMEOPS_H
#define DATABLERENAMEOPS_H
#include "TableOps/DATableDataOperation.h"
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>

namespace DA
{
class DATablePageContext;
class DATableViewStateController;
class DATableComputedColumnController;
class DATableValidationController;

/**
 * @brief 列改名算子（008 期4）
 *
 * renameColumns：全表列名对话框（唯一判断在对话框）；renameColumn：单列重命名
 * （列头菜单/列视图对话框用）。两者共用 DACommandDataFrame_renameColumns。
 * 改名影响面确认与引用迁移经计算列/校验控制器（Mediator-light，与全表改名
 * 同款）；视图状态键迁移挂 directional 回调（undo 双向回迁）。
 * 生命周期：宿主页面构造并持有（QObject 父子持有对话框）。
 */
class DAGUI_API DATableRenameOps : public QObject, public DATableDataOperation
{
    Q_OBJECT
public:
    DATableRenameOps(const DATablePageContext& ctx,
                     DATableViewStateController* viewState,
                     DATableComputedColumnController* computedColumn,
                     DATableValidationController* validation,
                     QObject* parent = nullptr);

public:
    QString id() const override;
    QString title() const override;
    bool isAvailable() const override;

public:
    // 打开全表列名对话框（改名对确认 → 计算列/校验规则引用迁移 → 命令入栈）
    void renameColumns();
    // 重命名单列；成功返回 true（空名/重名/越界/exec 失败返回 false）
    bool renameColumn(int col, const QString& newName);

private:
    const DATablePageContext& mCtx;              ///< 依赖注入的上下文（构造注入，非拥有）
    DATableViewStateController* mViewState { nullptr };  ///< 视图状态控制器（改名后重投影）
    DATableComputedColumnController* mComputedColumn { nullptr };  ///< 计算列控制器（影响面确认/迁移）
    DATableValidationController* mValidation { nullptr };          ///< 校验控制器（影响面确认/迁移）
};
}  // end of namespace DA
#endif  // DATABLERENAMEOPS_H
