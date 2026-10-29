#ifndef DACHARTOPERATEWIDGETCOMMANDS_H
#define DACHARTOPERATEWIDGETCOMMANDS_H
#include <QUndoCommand>
#include "DAGuiAPI.h"

namespace DA
{
class DAChartOperateWidget;
class DAFigureWidget;

/**
 * @brief 「转入绘图区」新建 Figure 的撤销命令
 *
 * 与该 figure 自己的 createChart_ / addItem_ 命令一起被包进同一个撤销宏：
 * - undo：把 dock 从停靠区摘掉，但 figure 保活（栈与图元都还在），
 *   并把该 figure 的栈重新置为激活，保证 Ctrl+Y 立刻可用；
 * - redo：把同一个 dock 挂回停靠区；
 * - skipFirstRedo：首次压栈时 figure 已经在停靠区（由 createFigure 完成），跳过第一次 redo。
 *
 * 这样一次「转入绘图区」= 一个撤销单元，一次 Ctrl+Z 撤销整个转入（实施方案 §6.4）。
 */
class DAGUI_API DAChartOperateWidgetCommandDetachFigure : public QUndoCommand
{
public:
    DAChartOperateWidgetCommandDetachFigure(DAChartOperateWidget* chartOperate,
                                            DAFigureWidget* figure,
                                            bool skipFirstRedo = true,
                                            QUndoCommand* parent = nullptr);
    ~DAChartOperateWidgetCommandDetachFigure() override;

    void undo() override;
    void redo() override;

private:
    DAChartOperateWidget* mChartOperate { nullptr };
    DAFigureWidget* mFigure { nullptr };
    bool mSkipFirstRedo { true };
};

}  // namespace DA

#endif  // DACHARTOPERATEWIDGETCOMMANDS_H
