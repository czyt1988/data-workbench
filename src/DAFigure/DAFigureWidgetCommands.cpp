#include "DAFigureWidgetCommands.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
namespace DA
{

//----------------------------------------------------
// DAFigureWidgetCommand_base
//----------------------------------------------------

DAFigureWidgetCommand_base::DAFigureWidgetCommand_base(DAFigureWidget* fig, QUndoCommand* par)
    : QUndoCommand(par), figureWidget(fig)
{
}

DAFigureWidget* DAFigureWidgetCommand_base::figure()
{
    return figureWidget;
}

//----------------------------------------------------
// DAFigureWidgetCommandCreateChart
//----------------------------------------------------
DAFigureWidgetCommandCreateChart::DAFigureWidgetCommandCreateChart(DAFigureWidget* fig,
                                                                   float xPresent,
                                                                   float yPresent,
                                                                   float wPresent,
                                                                   float hPresent,
                                                                   QUndoCommand* par)
    : DAFigureWidgetCommand_base(fig, par)
    , mChart(nullptr)
    , mXPresent(xPresent)
    , mYPresent(yPresent)
    , mWPresent(wPresent)
    , mHPresent(hPresent)
    , mNeedDelete(false)
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
        figure()->addChart(mChart, mXPresent, mYPresent, mWPresent, mHPresent);
    } else {
        mChart = figure()->createChart(mXPresent, mYPresent, mWPresent, mHPresent);
        mChart->setXLabel("x");
        mChart->setYLabel("y");
    }
}

void DAFigureWidgetCommandCreateChart::undo()
{
    mNeedDelete = true;
    figure()->removeChart(mChart);
}

//----------------------------------------------------
// DAFigureWidgetCommandResizeWidget
//----------------------------------------------------
DAFigureWidgetCommandResizeWidget::DAFigureWidgetCommandResizeWidget(DAFigureWidget* fig,
                                                                     QWidget* w,
                                                                     const QRectF& oldPresent,
                                                                     const QRectF& newPresent,
                                                                     QUndoCommand* par)
    : DAFigureWidgetCommand_base(fig, par), mWidget(w), mOldPresent(oldPresent), mNewPresent(newPresent)
{
    setText(QObject::tr("set figure widget size"));  // cn:设置绘图中窗体的尺寸
}

void DAFigureWidgetCommandResizeWidget::redo()
{
    figure()->setWidgetPosPercent(mWidget, mNewPresent);
}

void DAFigureWidgetCommandResizeWidget::undo()
{
    figure()->setWidgetPosPercent(mWidget, mOldPresent);
}

//----------------------------------------------------
// DAFigureWidgetCommandAttachItem
//----------------------------------------------------
DAFigureWidgetCommandAttachItem::DAFigureWidgetCommandAttachItem(DAFigureWidget* fig,
                                                                 DAChartWidget* chart,
                                                                 QwtPlotItem* item,
                                                                 bool skipFirst,
                                                                 QUndoCommand* par)
    : DAFigureWidgetCommand_base(fig, par), mChart(chart), mItem(item), mSkipFirst(skipFirst), mNeedDelete(false)
{
    setText(QObject::tr("add item in chart"));  // cn:设置绘图中窗体的尺寸
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
    if (mSkipFirst) {
        mSkipFirst = false;
    } else {
        mItem->attach(mChart);
        mNeedDelete = false;
    }
}

void DAFigureWidgetCommandAttachItem::undo()
{
    mItem->detach();
    mNeedDelete = true;
}

}
