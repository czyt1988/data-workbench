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

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param par 父命令
 */
DAFigureWidgetCommandBase::DAFigureWidgetCommandBase(DAFigureWidget* fig, QUndoCommand* par)
    : QUndoCommand(par), figureWidget(fig)
{
}

/**
 * @brief 获取关联的绘图窗口
 * @return 绘图窗口指针
 */
DAFigureWidget* DAFigureWidgetCommandBase::figure()
{
    return figureWidget;
}

//----------------------------------------------------
// DAFigureWidgetCommandCreateChart
//----------------------------------------------------

/**
 * @brief 构造函数，通过百分比坐标创建图表
 * @param fig 关联的绘图窗口
 * @param xPresent x坐标百分比
 * @param yPresent y坐标百分比
 * @param wPresent 宽度百分比
 * @param hPresent 高度百分比
 * @param par 父命令
 */
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

/**
 * @brief 构造函数，通过归一化矩形创建图表
 * @param fig 关联的绘图窗口
 * @param versatileSize 归一化矩形
 * @param par 父命令
 */
DAFigureWidgetCommandCreateChart::DAFigureWidgetCommandCreateChart(DAFigureWidget* fig,
                                                                   const QRectF& versatileSize,
                                                                   QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(nullptr), mChartSize(versatileSize), mNeedDelete(false)
{
    setText(QObject::tr("create chart"));  // cn:创建绘图
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandCreateChart::~DAFigureWidgetCommandCreateChart()
{
    if (mNeedDelete) {
        if (mChart) {
            mChart->deleteLater();
        }
    }
}

/**
 * @brief 重做操作：创建或重新添加图表
 */
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

/**
 * @brief 撤销操作：移除图表
 */
void DAFigureWidgetCommandCreateChart::undo()
{
    mNeedDelete = true;
    figure()->removeChart(mChart);
}

/**
 * @brief 获取创建的图表窗口
 * @return 图表窗口指针
 */
DAChartWidget* DAFigureWidgetCommandCreateChart::getChartWidget()
{
    return mChart;
}

//===============================================================
// DAFigureWidgetCommandCreate3DChart
//===============================================================

/**
 * @brief 构造函数，通过百分比坐标创建3D图表
 * @param fig 关联的绘图窗口
 * @param xPresent x坐标百分比
 * @param yPresent y坐标百分比
 * @param wPresent 宽度百分比
 * @param hPresent 高度百分比
 * @param par 父命令
 */
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

/**
 * @brief 构造函数，通过归一化矩形创建3D图表
 * @param fig 关联的绘图窗口
 * @param versatileSize 归一化矩形
 * @param par 父命令
 */
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

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandCreate3DChart::~DAFigureWidgetCommandCreate3DChart()
{
    if (mNeedDelete) {
        if (mChart3D) {
            mChart3D->deleteLater();
        }
    }
}

/**
 * @brief 重做操作：创建或重新添加3D图表
 */
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

/**
 * @brief 撤销操作：移除3D图表
 */
void DAFigureWidgetCommandCreate3DChart::undo()
{
    mNeedDelete = true;
    figure()->remove3DChart(mChart3D);
}

/**
 * @brief 获取创建的3D图表窗口
 * @return 3D图表窗口指针
 */
DAChart3DWidget* DAFigureWidgetCommandCreate3DChart::getChart3DWidget()
{
    return mChart3D;
}

//===============================================================
// DAFigureWidgetCommandRemoveChart
//===============================================================

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param chart 要移除的图表
 * @param par 父命令
 */
DAFigureWidgetCommandRemoveChart::DAFigureWidgetCommandRemoveChart(DAFigureWidget* fig, DAChartWidget* chart, QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(chart)
{
    setText(QObject::tr("remove chart"));  // cn:移除绘图
                                           // 先要获取尺寸
    mChartNormRect = fig->axesNormRect(chart);
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandRemoveChart::~DAFigureWidgetCommandRemoveChart()
{
    if (mNeedDelete) {
        if (mChart) {
            mChart->deleteLater();
        }
    }
}

/**
 * @brief 重做操作：移除图表
 */
void DAFigureWidgetCommandRemoveChart::redo()
{
    mNeedDelete = true;
    if (mChart) {
        figure()->removeChart(mChart);
    }
}

/**
 * @brief 撤销操作：重新添加图表
 */
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

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param chart3d 要移除的3D图表
 * @param par 父命令
 */
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

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandRemove3DChart::~DAFigureWidgetCommandRemove3DChart()
{
    if (mNeedDelete) {
        if (mChart3D) {
            mChart3D->deleteLater();
        }
    }
}

/**
 * @brief 重做操作：移除3D图表
 */
void DAFigureWidgetCommandRemove3DChart::redo()
{
    mNeedDelete = true;
    if (mChart3D) {
        figure()->remove3DChart(mChart3D);
    }
}

/**
 * @brief 撤销操作：重新添加3D图表
 */
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

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param w 要调整尺寸的窗口
 * @param oldNormRect 原归一化矩形
 * @param newNormRect 新归一化矩形
 * @param par 父命令
 */
DAFigureWidgetCommandResizeWidget::DAFigureWidgetCommandResizeWidget(DAFigureWidget* fig,
                                                                     QWidget* w,
                                                                     const QRectF& oldNormRect,
                                                                     const QRectF& newNormRect,
                                                                     QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mWidget(w), mOldNormRect(oldNormRect), mNewNormRect(newNormRect)
{
    setText(QObject::tr("set figure widget size"));  // cn:设置绘图中窗体的尺寸
}

/**
 * @brief 重做操作：设置窗口新尺寸
 */
void DAFigureWidgetCommandResizeWidget::redo()
{
    if (!mWidget) {
        return;
    }
    //= 给qwt_figure增加可以添加任意窗口的方法
    figure()->setWidgetNormPos(mWidget, mNewNormRect);
}

/**
 * @brief 撤销操作：恢复窗口原尺寸
 */
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

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param chart 目标图表
 * @param item 要添加的图元
 * @param skipFirst 是否跳过第一次redo
 * @param par 父命令
 */
DAFigureWidgetCommandAttachItem::DAFigureWidgetCommandAttachItem(DAFigureWidget* fig,
                                                                 DAChartWidget* chart,
                                                                 QwtPlotItem* item,
                                                                 bool skipFirst,
                                                                 QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mChart(chart), mItem(item), mSkipFirst(skipFirst), mNeedDelete(false)
{
    setText(QObject::tr("add item in chart"));  // cn:添加图元到绘图
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandAttachItem::~DAFigureWidgetCommandAttachItem()
{
    if (mNeedDelete) {
        if (mItem) {
            delete mItem;
        }
    }
}

/**
 * @brief 重做操作：将图元附加到图表
 */
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

/**
 * @brief 撤销操作：将图元从图表分离
 */
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

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param chart3d 目标3D图表
 * @param item 要添加的3D图元
 * @param skipFirst 是否跳过第一次redo
 * @param par 父命令
 */
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

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandAttach3DItem::~DAFigureWidgetCommandAttach3DItem()
{
    if (mNeedDelete) {
        if (mItem) {
            delete mItem;
        }
    }
}

/**
 * @brief 重做操作：将3D图元附加到3D图表
 */
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

/**
 * @brief 撤销操作：将3D图元从3D图表分离
 */
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

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param sourceChart 源图表
 * @param targetChart 目标图表
 * @param item 要移动的图元
 * @param par 父命令
 */
DAFigureWidgetCommandMoveItem::DAFigureWidgetCommandMoveItem(DAFigureWidget* fig,
                                                             DAChartWidget* sourceChart,
                                                             DAChartWidget* targetChart,
                                                             QwtPlotItem* item,
                                                             QUndoCommand* par)
    : DAFigureWidgetCommandBase(fig, par), mSourceChart(sourceChart), mTargetChart(targetChart), mItem(item)
{
    setText(QObject::tr("move plot item to another chart"));  // cn:移动图元到另一个绘图
}

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandMoveItem::~DAFigureWidgetCommandMoveItem()
{
    // item始终attach在某个plot上，无需在此管理生命周期
}

/**
 * @brief 重做操作：将图元移动到目标图表
 */
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

/**
 * @brief 撤销操作：将图元移回源图表
 */
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

/**
 * @brief 构造函数
 * @param fig 关联的绘图窗口
 * @param sourceChart3D 源3D图表
 * @param targetChart3D 目标3D图表
 * @param item 要移动的3D图元
 * @param par 父命令
 */
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

/**
 * @brief 析构函数
 */
DAFigureWidgetCommandMove3DItem::~DAFigureWidgetCommandMove3DItem()
{
    // item始终attach在某个plot上，无需在此管理生命周期
}

/**
 * @brief 重做操作：将3D图元移动到目标3D图表
 */
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

/**
 * @brief 撤销操作：将3D图元移回源3D图表
 */
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
