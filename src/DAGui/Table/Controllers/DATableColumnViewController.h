#ifndef DATABLECOLUMNVIEWCONTROLLER_H
#define DATABLECOLUMNVIEWCONTROLLER_H
#include "DAGuiAPI.h"
#include <QList>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QVector>

class QUndoCommand;

namespace DA
{
class DACommandDataFrame_renameColumns;
class DATablePageContext;
class DATableSelectionService;
class DATableViewStateController;
class DATableComputedColumnController;
class DATableValidationController;

/**
 * @brief 列视图控制器（008 期3，最后一拆）
 *
 * 隐藏/显示全部/冻结/列序/列视图对话框；「列视图真源操作」（006c/006d）：
 * 全部构造命令入栈、由回调重投影（经视图状态控制器）。构造注入上下文、
 * 选区服务、视图状态/计算列/校验控制器（Mediator-light：对话框的改名
 * 需要计算列/校验规则的影响面确认与引用迁移，与全表改名同款），
 * 不持窗口指针。
 * 生命周期：宿主页面构造并持有（QObject 父子持有列视图对话框）。
 */
class DAGUI_API DATableColumnViewController : public QObject
{
    Q_OBJECT
public:
    explicit DATableColumnViewController(const DATablePageContext& ctx,
                                         const DATableSelectionService& sel,
                                         DATableViewStateController* viewState,
                                         DATableComputedColumnController* computedColumn,
                                         DATableValidationController* validation,
                                         QObject* parent = nullptr);

public:
    // 列视图直接入口：只写 store + 重投影，不入撤销栈（007 定位的对接面）
    void setColumnHiddenDirectly(const QString& name, int occurrence, bool hidden);
    // 生效冻结列数（= 冻结集合中当前可见的列数，转发容器）
    int effectiveFrozenColumnCount() const;
    // 隐藏指定列（列头菜单「隐藏此列」，集合无差异不推空命令）
    void hideColumnAt(int logicalColumn);
    // 显示所有列（隐藏集合清空入栈）
    void showAllColumns();
    // 冻结/取消冻结指定列（集合无差异不推空命令）
    void setColumnFrozen(int logicalColumn, bool frozen);
    // 冻结全局列序第一个可见列（Ribbon「冻结首列」）
    void freezeFirstColumn();
    // 取消全部冻结（Ribbon「取消全部冻结」）
    void unfreezeAllColumns();
    // 弹「冻结前 N 列」输入框（追加语义；0 = 不做修改）
    void promptFreezeColumns();
    // 容器 columnOrderChanged 驱动：拖动落库入栈（无差异不推命令）
    void applyColumnOrderFromView(const QVector< int >& newOrderLogical);
    // 容器 frozenOrderChanged 驱动：冻结区内部拖动落库入栈
    void applyFrozenOrderFromView(const QVector< int >& newFrozenOrderLogical);
    // 打开列管理对话框（列名/顺序/隐藏/冻结）：确定按差异构造一个批量父命令
    void showManageColumnsDialog();
    // 「列」子菜单动作分发（006d）；action 为 DADataHeaderView::ColumnViewAction 整数值
    void onHeaderActionRequested(int logicalColumn, int action);

private:
    // 全局列序（含隐藏列）的身份表（store columnOrder 精确匹配实际列；任何地方要都调这里）
    QList< QPair< QString, int > > orderedColumnIdentities() const;

private:
    const DATablePageContext& mCtx;      ///< 依赖注入的上下文（构造注入，非拥有）
    const DATableSelectionService& mSel;  ///< 选区/坐标查询服务（构造注入，非拥有）
    DATableViewStateController* mViewState { nullptr };              ///< 视图状态控制器（重投影）
    DATableComputedColumnController* mComputedColumn { nullptr };     ///< 计算列控制器（改名影响面确认/迁移）
    DATableValidationController* mValidation { nullptr };             ///< 校验控制器（改名影响面确认/迁移）
    class DADialogManageColumns* mDialogManageColumns { nullptr };  ///< 列管理对话框（懒建复用）
};
}  // end of namespace DA
#endif  // DATABLECOLUMNVIEWCONTROLLER_H
