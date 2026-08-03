#include "DAFigureWidgetCommands.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChart3DWidget.h"
#include "DAChartUtil.h"
// qwt3d
#include "qwt3d_plot.h"
#include "qwt3d_plotitem.h"
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
// DAFigureWidgetCommandCreate3DChart
//===============================================================
DAFigureWidgetCommandCreate3DChart::DAFigureWidgetCommandCreate3DChart(DAFigureWidget* fig,
                                                                       qreal xPresent,
                                                                       qreal yPresent,
                                                                       qreal wPresent,
                                                                       qreal hPresent,
                                                                       QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mChart3D(nullptr)
    , mChartSize(xPresent, yPresent, wPresent, hPresent)
    , mNeedDelete(false)
{
    setText(QObject::tr("create 3D chart"));  // cn:创建3D绘图
}

DAFigureWidgetCommandCreate3DChart::DAFigureWidgetCommandCreate3DChart(DAFigureWidget* fig,
                                                                       const QRectF& versatileSize,
                                                                       QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mChart3D(nullptr)
    , mChartSize(versatileSize)
    , mNeedDelete(false)
{
    setText(QObject::tr("create 3D chart"));  // cn:创建3D绘图
}

DAFigureWidgetCommandCreate3DChart::~DAFigureWidgetCommandCreate3DChart()
{
    if (mNeedDelete) {
        if (mChart3D) {
            mChart3D->deleteLater();
        }
    }
}

void DAFigureWidgetCommandCreate3DChart::redo()
{
    mNeedDelete = false;
    if (mChart3D) {
        // 这是执行undo后执行redo会进入这个分支
        figure()->add3DChart(mChart3D, mChartSize);
    } else {
        // 第一次执行redo会进入这里
        mChart3D = figure()->create3DChart(mChartSize);
    }
}

void DAFigureWidgetCommandCreate3DChart::undo()
{
    mNeedDelete = true;
    figure()->remove3DChart(mChart3D);
}

DAChart3DWidget* DAFigureWidgetCommandCreate3DChart::getChart3DWidget()
{
    return mChart3D;
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

//===============================================================
// DAFigureWidgetCommandRemove3DChart
//===============================================================
DAFigureWidgetCommandRemove3DChart::DAFigureWidgetCommandRemove3DChart(DAFigureWidget* fig,
                                                                       DAChart3DWidget* chart3d,
                                                                       QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mChart3D(chart3d)
{
    setText(QObject::tr("remove 3D chart"));  // cn:移除3D绘图
    // 使用 widgetNormRect 获取归一化位置（3D chart 不是 QwtPlot，不能用 axesNormRect）
    mChartNormRect = fig->widgetNormRect(chart3d);
}

DAFigureWidgetCommandRemove3DChart::~DAFigureWidgetCommandRemove3DChart()
{
    if (mNeedDelete) {
        if (mChart3D) {
            mChart3D->deleteLater();
        }
    }
}

void DAFigureWidgetCommandRemove3DChart::redo()
{
    mNeedDelete = true;
    if (mChart3D) {
        figure()->remove3DChart(mChart3D);
    }
}

void DAFigureWidgetCommandRemove3DChart::undo()
{
    mNeedDelete = false;
    if (mChart3D) {
        figure()->add3DChart(mChart3D, mChartNormRect);
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
// DAFigureWidgetCommandAttach3DItem
//----------------------------------------------------
DAFigureWidgetCommandAttach3DItem::DAFigureWidgetCommandAttach3DItem(DAFigureWidget* fig,
                                                                     DAChart3DWidget* chart3d,
                                                                     Qwt3DPlotItem* item,
                                                                     bool skipFirst,
                                                                     QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mChart3D(chart3d)
    , mItem(item)
    , mSkipFirst(skipFirst)
    , mNeedDelete(false)
{
    setText(QObject::tr("add 3D item in chart"));  // cn:添加3D图元到绘图
}

DAFigureWidgetCommandAttach3DItem::~DAFigureWidgetCommandAttach3DItem()
{
    if (mNeedDelete) {
        if (mItem) {
            delete mItem;
        }
    }
}

void DAFigureWidgetCommandAttach3DItem::redo()
{
    if (!mChart3D || !mItem) {
        return;
    }
    if (mSkipFirst) {
        mSkipFirst = false;
    } else {
        mItem->attach(mChart3D);
        mNeedDelete = false;
        // 触发 GL 重绘（Qwt3DPlot 继承 QOpenGLWidget，update() 触发 paintGL）
        mChart3D->update();
    }
}

void DAFigureWidgetCommandAttach3DItem::undo()
{
    if (!mItem) {
        return;
    }
    mItem->detach();
    mNeedDelete = true;
    if (mChart3D) {
        mChart3D->update();
    }
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

//----------------------------------------------------
// DAFigureWidgetCommandMove3DItem
//----------------------------------------------------
DAFigureWidgetCommandMove3DItem::DAFigureWidgetCommandMove3DItem(DAFigureWidget* fig,
                                                                  DAChart3DWidget* sourceChart3D,
                                                                  DAChart3DWidget* targetChart3D,
                                                                  Qwt3DPlotItem* item,
                                                                  QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par)
    , mSourceChart3D(sourceChart3D)
    , mTargetChart3D(targetChart3D)
    , mItem(item)
{
    setText(QObject::tr("move 3D plot item to another chart"));  // cn:移动3D图元到另一个绘图
}

DAFigureWidgetCommandMove3DItem::~DAFigureWidgetCommandMove3DItem()
{
    // item始终attach在某个plot上，无需在此管理生命周期
}

void DAFigureWidgetCommandMove3DItem::redo()
{
    if (!mTargetChart3D || !mItem) {
        return;
    }
    mItem->attach(mTargetChart3D);
    if (mSourceChart3D) {
        mSourceChart3D->update();
    }
    mTargetChart3D->update();
}

void DAFigureWidgetCommandMove3DItem::undo()
{
    if (!mSourceChart3D || !mItem) {
        return;
    }
    mItem->attach(mSourceChart3D);
    if (mTargetChart3D) {
        mTargetChart3D->update();
    }
    mSourceChart3D->update();
}

}  // namespace DA
