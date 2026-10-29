#include "DAChartOperateWidgetCommands.h"
#include "DAChartOperateWidget.h"
#include "DAFigureWidget.h"
#include <QUndoStack>

namespace DA
{

/**
 * @brief 构造函数
 * @param chartOperate 绘图区容器
 * @param figure 已创建并挂好的 figure
 * @param skipFirstRedo 首次压栈时 figure 已在停靠区，跳过第一次 redo
 * @param parent 父命令
 */
DAChartOperateWidgetCommandDetachFigure::DAChartOperateWidgetCommandDetachFigure(DAChartOperateWidget* chartOperate,
                                                                                 DAFigureWidget* figure,
                                                                                 bool skipFirstRedo,
                                                                                 QUndoCommand* parent)
    : QUndoCommand(parent)
    , mChartOperate(chartOperate)
    , mFigure(figure)
    , mSkipFirstRedo(skipFirstRedo)
{
    setText(QObject::tr("create figure"));  // cn:创建绘图
}

DAChartOperateWidgetCommandDetachFigure::~DAChartOperateWidgetCommandDetachFigure()
{
}

/**
 * @brief 撤销：摘掉 dock 但保活 figure，并把该 figure 的栈重新置为激活
 */
void DAChartOperateWidgetCommandDetachFigure::undo()
{
    if (!mChartOperate || !mFigure) {
        return;
    }
    mChartOperate->detachFigureDock(mFigure);
    // 若 ADS 因 dock 移除自动激活了别的 figure，命令里把本图的栈显式置回激活，
    // 保证紧接着的 Ctrl+Y 立刻可用（实施方案 §6.4）。
    if (QUndoStack* un = mFigure->getUndoStack()) {
        un->setActive(true);
    }
}

/**
 * @brief 重做：把同一个 dock 挂回停靠区
 */
void DAChartOperateWidgetCommandDetachFigure::redo()
{
    if (!mChartOperate || !mFigure) {
        return;
    }
    if (mSkipFirstRedo) {
        // 首次压栈时 figure 由 createFigure 创建并已挂好，这里只做记录
        mSkipFirstRedo = false;
        return;
    }
    mChartOperate->reattachFigureDock(mFigure);
}

}  // namespace DA
