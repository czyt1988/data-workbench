#ifndef DATABLECOMPUTEDCOLUMNCONTROLLER_H
#define DATABLECOMPUTEDCOLUMNCONTROLLER_H
#include "DAGuiAPI.h"
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <memory>

class QUndoCommand;

namespace DA
{
class DACommandDataFrame_renameColumns;
class DAComputedColumn;
class DATablePageContext;

/**
 * @brief 计算列控制器（008 期3）
 *
 * 公式求值管线（创建/编辑/重算共用）、三态（Valid/Stale/Invalid）维护、
 * 全部重算（宏命令 = 一个撤销单元）、移除公式、列改名确认与定义迁移、
 * 表头角标刷新。构造注入上下文（计算列子存储/数据/模型/撤销栈/脏标桥），
 * 不持窗口指针。
 * 重算改数据经上下文 notifyDataModified 标脏（与页面 dataModified 信号同源）。
 * 生命周期：宿主页面构造并持有（QObject 父子）。
 */
class DAGUI_API DATableComputedColumnController : public QObject
{
    Q_OBJECT
public:
    explicit DATableComputedColumnController(const DATablePageContext& ctx, QObject* parent = nullptr);

public:
    // store 变更后刷新表头（底色经模型 headerDataChanged，ƒx 角标经视口 update）
    void refreshHeaders();
    // 打开计算列对话框：col<0 为新建，col>=0 为编辑该列既有公式（列头右键「编辑公式」）
    void showDialog(int col);
    // 重算单个计算列（列头右键「重新计算」），col 为列位置索引；成功入栈返回 true
    bool recompute(int col);
    // 重算全部待重算/失效的计算列（Ribbon「全部重算」），返回成功重算的列数
    int recomputeAllStale();
    // 移除某列的公式定义（保留列数据，表头右键「移除公式」），可撤销；成功返回 true
    bool removeFormula(int col);
    // 列改名前的计算列影响面确认（R20）：提示「有 N 个计算列引用它」
    bool confirmRename(const QList< QPair< QString, QString > >& renames) const;
    // 把定义迁移挂进改名命令的 directional 回调（与改名同一撤销单元，R20）
    void wireRenameMigration(DACommandDataFrame_renameColumns* cmd,
                            const QList< QPair< QString, QString > >& renames) const;

private:
    // 列位置索引 → 列名（越界返回空串）
    QString columnNameAt(int actualCol) const;
    // 求值定义并入栈重算命令：isNewColumn=true 走创建路径（undo 删列），
    // false 走重算路径（undo 还原旧列）；成功返回 true（失败标失效并提示）
    bool applyEval(const DAComputedColumn& def, bool isNewColumn);

private:
    const DATablePageContext& mCtx;  ///< 依赖注入的上下文（构造注入，非拥有）
};
}  // end of namespace DA
#endif  // DATABLECOMPUTEDCOLUMNCONTROLLER_H
