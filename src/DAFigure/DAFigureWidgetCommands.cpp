#include "DAFigureWidgetCommands.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChartUtil.h"
namespace DA
{

//----------------------------------------------------
// DAFigureWidgetCommand_base
//----------------------------------------------------

DAFigureWidgetCommandBase::DAFigureWidgetCommandBase(DAFigureWidget* fig, QUndoCommand* par)
    : QUndoCommand(par), figureWidget(fig)
{
}

DAFigureWidget* DAFigureWidgetCommandBase::figure()
{
    return figureWidget;
}

//----------------------------------------------------
// DAFigureWidgetCommandCreateChart
//----------------------------------------------------
DAFigureWidgetCommandCreateChart::DAFigureWidgetCommandCreateChart(DAFigureWidget* fig,
                                                                   qreal xPresent,
                                                                   qreal yPresent,
                                                                   qreal wPresent,
                                                                   qreal hPresent,
                                                                   QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mChart(nullptr)
    , mChartSize(xPresent, yPresent, wPresent, hPresent)
    , mNeedDelete(false)
{
    setText(QObject::tr("create chart"));  // cn:创建绘图
}

DAFigureWidgetCommandCreateChart::DAFigureWidgetCommandCreateChart(DAFigureWidget* fig,
                                                                   const QRectF& versatileSize,
                                                                   QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(nullptr), mChartSize(versatileSize), mNeedDelete(false)
{
    setText(QObject::tr("create chart"));  // cn:创建绘图
}

DAFigureWidgetCommandCreateChart::~DAFigureWidgetCommandCreateChart()
{
    if (mNeedDelete) {
        if (mChart) {
            mChart->deleteLater();
        }
    }
}

void DAFigureWidgetCommandCreateChart::redo()
{
    mNeedDelete = false;
    if (mChart) {
        // 这是执行undo后执行redo会进入这个分支
        figure()->addChart(mChart, mChartSize);
    } else {
        // 第一次执行redo会进入这里
        mChart = figure()->createChart(mChartSize);
        mChart->setAxisLabel(QwtAxis::XBottom, "x");
        mChart->setAxisLabel(QwtAxis::YLeft, "y");
    }
}

void DAFigureWidgetCommandCreateChart::undo()
{
    mNeedDelete = true;
    figure()->removeChart(mChart);
}

DAChartWidget* DAFigureWidgetCommandCreateChart::getChartWidget()
{
    return mChart;
}

//===============================================================
// DAFigureWidgetCommandRemoveChart
//===============================================================
DAFigureWidgetCommandRemoveChart::DAFigureWidgetCommandRemoveChart(DAFigureWidget* fig, DAChartWidget* chart, QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(chart)
{
    setText(QObject::tr("remove chart"));  // cn:移除绘图
                                           // 先要获取尺寸
    mChartNormRect = fig->axesNormRect(chart);
}

DAFigureWidgetCommandRemoveChart::~DAFigureWidgetCommandRemoveChart()
{
    if (mNeedDelete) {
        if (mChart) {
            mChart->deleteLater();
        }
    }
}

void DAFigureWidgetCommandRemoveChart::redo()
{
    mNeedDelete = true;
    if (mChart) {
        figure()->removeChart(mChart);
    }
}

void DAFigureWidgetCommandRemoveChart::undo()
{
    mNeedDelete = false;
    if (mChart) {
        figure()->addChart(mChart, mChartNormRect);
    }
}

//----------------------------------------------------
// DAFigureWidgetCommandResizeWidget
//----------------------------------------------------
DAFigureWidgetCommandResizeWidget::DAFigureWidgetCommandResizeWidget(DAFigureWidget* fig,
                                                                     QWidget* w,
                                                                     const QRectF& oldNormRect,
                                                                     const QRectF& newNormRect,
                                                                     QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mWidget(w), mOldNormRect(oldNormRect), mNewNormRect(newNormRect)
{
    setText(QObject::tr("set figure widget size"));  // cn:设置绘图中窗体的尺寸
}

void DAFigureWidgetCommandResizeWidget::redo()
{
    if (!mWidget) {
        return;
    }
    //= 给qwt_figure增加可以添加任意窗口的方法
    figure()->setWidgetNormPos(mWidget, mNewNormRect);
}

void DAFigureWidgetCommandResizeWidget::undo()
{
    if (!mWidget) {
        return;
    }
    figure()->setWidgetNormPos(mWidget, mOldNormRect);
}

//----------------------------------------------------
// DAFigureWidgetCommandAttachItem
//----------------------------------------------------
DAFigureWidgetCommandAttachItem::DAFigureWidgetCommandAttachItem(DAFigureWidget* fig,
                                                                 DAChartWidget* chart,
                                                                 QwtPlotItem* item,
                                                                 bool skipFirst,
                                                                 QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(chart), mItem(item), mSkipFirst(skipFirst), mNeedDelete(false)
{
    setText(QObject::tr("add item in chart"));  // cn:添加图元到绘图
}

DAFigureWidgetCommandAttachItem::~DAFigureWidgetCommandAttachItem()
{
    if (mNeedDelete) {
        if (mItem) {
            delete mItem;
        }
    }
}

void DAFigureWidgetCommandAttachItem::redo()
{
    if (!mChart || !mItem) {
        return;
    }
    if (mSkipFirst) {
        mSkipFirst = false;
    } else {
        mItem->attach(mChart);
        mNeedDelete = false;
    }
}

void DAFigureWidgetCommandAttachItem::undo()
{
    if (!mItem) {
        return;
    }
    mItem->detach();
    mNeedDelete = true;
}

//----------------------------------------------------
// DAFigureWidgetCommandMoveItem
//----------------------------------------------------
DAFigureWidgetCommandMoveItem::DAFigureWidgetCommandMoveItem(DAFigureWidget* fig,
                                                             DAChartWidget* sourceChart,
                                                             DAChartWidget* targetChart,
                                                             QwtPlotItem* item,
                                                             QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mSourceChart(sourceChart), mTargetChart(targetChart), mItem(item)
{
    setText(QObject::tr("move plot item to another chart"));  // cn:移动图元到另一个绘图
}

DAFigureWidgetCommandMoveItem::~DAFigureWidgetCommandMoveItem()
{
    // item始终attach在某个plot上，无需在此管理生命周期
}

void DAFigureWidgetCommandMoveItem::redo()
{
    if (!mTargetChart || !mItem) {
        return;
    }
    mItem->attach(mTargetChart);
    if (mSourceChart) {
        DAChartUtil::replot(mSourceChart);
    }
    DAChartUtil::replot(mTargetChart);
}

void DAFigureWidgetCommandMoveItem::undo()
{
    if (!mSourceChart || !mItem) {
        return;
    }
    mItem->attach(mSourceChart);
    if (mTargetChart) {
        DAChartUtil::replot(mTargetChart);
    }
    DAChartUtil::replot(mSourceChart);
}

}
