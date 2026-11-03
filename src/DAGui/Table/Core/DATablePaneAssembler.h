#ifndef DATABLEPANEASSEMBLER_H
#define DATABLEPANEASSEMBLER_H
#include "DAGuiAPI.h"

class QHeaderView;

namespace DA
{
class DADataHeaderView;
class DADataTableView;
class DATableStyleItemDelegate;
class DATablePageContext;
class DATableViewStateController;

/**
 * @brief 窗格装配器（008 期5）
 *
 * configurePaneView 的装配段（表头替换 → setModel/selectionModel 共享 → 委托 →
 * 批注/计算列 store 注入 → 表头信号接线到视图状态控制器/窗格容器/store）。
 * 主窗格与懒建的冻结窗格都走同一条；表意信号（快捷键/右键菜单入口）的连接
 * 归页面（消费方是页面自身的槽，留在 Facade）。
 * 纯装配工具：构造注入上下文、视图状态控制器与样式委托，无状态、非 QObject。
 */
class DAGUI_API DATablePaneAssembler
{
public:
    DATablePaneAssembler(const DATablePageContext& ctx,
                         DATableViewStateController* viewState,
                         DATableStyleItemDelegate* styleDelegate);

    /**
     * @brief 装配一个窗格（表头替换/setModel/委托/store 注入/表头接线）
     *
     * 表头替换必须在 setModel 之前（QTableView::setModel 会把模型接到"当前"表头上）。
     * @param pane 目标窗格
     */
    void assemble(DADataTableView* pane) const;

private:
    // 水平表头接线（计算列 store/冻结查询/列宽采集/列序拖动/表头高）
    void wireHorizontalHeader(DADataHeaderView* hh, DADataTableView* pane) const;
    // 垂直表头接线（行头宽拖拽/统一行高）
    void wireVerticalHeader(DADataHeaderView* vh) const;

private:
    const DATablePageContext& mCtx;          ///< 依赖注入的上下文（构造注入，非拥有）
    DATableViewStateController* mViewState { nullptr };  ///< 视图状态控制器（列宽采集/行高表头高落库）
    DATableStyleItemDelegate* mStyleDelegate { nullptr };  ///< 样式委托（同一实例注入各窗格，属页面）
};
}  // end of namespace DA
#endif  // DATABLEPANEASSEMBLER_H
