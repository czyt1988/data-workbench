#ifndef DATABLEMUTATIONCOORDINATOR_H
#define DATABLEMUTATIONCOORDINATOR_H
#include "DAGuiAPI.h"
#include <QList>
#include <QObject>
#include <functional>

namespace DA
{
class DATablePageContext;

/**
 * @brief 编辑后效描述（008 期2）
 *
 * 描述「一次数据编辑改变了什么」；后效集合（模型刷新/统计失效/计算列标脏/
 * 浮窗关闭/宿主通知）由 DATableMutationCoordinator::commit 按变更类型统一执行，
 * 各编辑入口不再手抄。
 */
struct DATableMutation
{
    /**
     * @brief 变更类型（决定后效策略，见计划 §6.2 后效策略表）
     */
    enum Kind
    {
        Values,    ///< 值变更：changedRows/changedCols 一一对应，模型逐格通知 + 受影响列统计失效 + 计算列按列标脏
        Structure,  ///< 结构变更（插删行/列）：全量统计失效 + 计算列全标脏；键重映射由命令的 directional 回调完成
        FullReset    ///< 整表替换（转索引等）：模型 reset + 全量失效 + 计算列全标脏
    };
    Kind kind { Values };
    QList< int > changedRows;  ///< 受影响行（数据源绝对行号；与 changedCols 一一对应）
    QList< int > changedCols;  ///< 受影响列（位置索引）
    // 结构变更的模型通知形状（命令侧给出，如 notifyColumnsRemoved(cols)）；
    // 值变更无需提供（commit 按行列逐格通知）。空 = 不做结构通知
    std::function< void() > modelRefresh;
    bool closeQuickPlot { false };         ///< 关闭快速绘图浮窗（列被删/整表重排后列身份失效）
    bool notifyHostValueChange { false };   ///< DADataManager 树刷新（ChangeValue：删列后重建列子项）
};

/**
 * @brief 编辑后效唯一实现点（008 期2）
 *
 * 收口此前散落 ~20 处的「编辑后」扇出手抄（模型刷新 → 条件格式统计失效 →
 * 计算列标脏 → 浮窗/宿主通知）。后效顺序钉死且只有这一处：
 * 1. 模型刷新（值变更逐格 / 结构变更经 modelRefresh 给出通知形状）；
 * 2. 条件格式列统计失效（受影响列；结构/整表保守全失效）；
 * 3. 计算列标待重算（受影响列按列名依赖比对；结构/整表保守全标）；
 * 4. 浮窗关闭 / 宿主树刷新（按变更描述）。
 *
 * 边界：不接管命令的 exec/undo 时序（plain 回调与 directional 回调的先后
 * 由各命令自身决定，见 DACommandsDataFrame.cpp），只提供 callbackFor()
 * 给命令的 setCallBack 装配；键重映射（样式/条件格式/批注的位移）属于命令的
 * directional 回调，不在本类职责内。
 * 生命周期：宿主页面构造并持有（QObject 父子），构造注入 DATablePageContext。
 */
class DAGUI_API DATableMutationCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit DATableMutationCoordinator(const DATablePageContext& ctx, QObject* parent = nullptr);

public:
    // 编辑后效唯一入口（按变更类型执行后效策略表）
    void commit(const DATableMutation& m);
    // 给 QUndoCommand 的 setCallBack 用：exec 与 undo/redo 共用一次后效；
    // 捕获 QPointer 防悬挂（宿主页面销毁后命令仍在撤销栈时静默丢弃）
    std::function< void() > callbackFor(const DATableMutation& m) const;
    // 数据变更列（位置索引）→ 计算列待重算标记（按列名依赖比对，空列表无操作）。
    // 公开给计算列专项路径（重算/编辑）用；一般编辑入口经 commit() 间接触达
    void markComputedColumnsStale(const QList< int >& actualCols);
    // 整表操作保守把全部计算列标待重算（同上）
    void markAllComputedColumnsStale();

private:
    // 值变更后效：模型逐格通知 + 受影响列统计失效 + 计算列按列标脏
    void commitValues(const DATableMutation& m);
    // 结构/整表变更后效：模型结构通知/复位 + 全量统计失效 + 计算列全标脏
    void commitStructure(const DATableMutation& m);

private:
    const DATablePageContext& mCtx;
};
}  // end of namespace DA
#endif  // DATABLEMUTATIONCOORDINATOR_H
