#include "DAFigureChartEditorWidgetOverlay.h"
#include <QPoint>
#include <QMouseEvent>
#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "da_qt5qt6_compat.hpp"
#include "DAAbstractChartEditor.h"
#ifndef DAFigureChartEditorWidgetOverlay_DebugPrint
#define DAFigureChartEditorWidgetOverlay_DebugPrint 0
#endif
namespace DA
{
class DAFigureChartEditorWidgetOverlay::PrivateData
{
    DA_DECLARE_PUBLIC(DAFigureChartEditorWidgetOverlay)
public:
    PrivateData(DAFigureChartEditorWidgetOverlay* p);
    QPoint mapPosToPlotCanvas(const QwtPlot* plot, const QPoint& figPos);
    QPoint mapPosToPlotCanvas(const QPoint& figPos);
    QPoint mapMousePosToPlotCanvas();

public:
    DAFigureChartEditorWidgetOverlay::FpChartEditorFactory mFunFactory { nullptr };
    QPointer< QwtPlot > mActivePlot;
    QPointer< DAAbstractChartEditor > mActiveEditor;
    QPoint mLastFigureMousePos;
    bool mAutoStart { false };
    FpActiveChartCanvasPainter mActiveChartCanvasPainter { nullptr };
};

/**
 * @brief 构造函数
 * @param p 父对象指针
 */
DAFigureChartEditorWidgetOverlay::PrivateData::PrivateData(DAFigureChartEditorWidgetOverlay* p) : q_ptr(p)
{
}

/**
 * @brief 将Figure坐标系的位置映射到指定Plot的canvas坐标系
 * @param plot 目标QwtPlot
 * @param figPos Figure坐标系下的位置
 * @return 映射到Plot canvas坐标系下的位置
 */
QPoint DAFigureChartEditorWidgetOverlay::PrivateData::mapPosToPlotCanvas(const QwtPlot* plot, const QPoint& figPos)
{
    if (!plot) {
        return QPoint();
    }
    // 先转换到全局坐标
    QPoint globalPos      = q_ptr->mapToGlobal(figPos);
    const QWidget* canvas = plot->canvas();
    if (!canvas) {
        return QPoint();
    }
    return canvas->mapFromGlobal(globalPos);
}

/**
 * @brief 将Figure坐标系的位置映射到当前激活Plot的canvas坐标系
 * @param figPos Figure坐标系下的位置
 * @return 映射到当前激活Plot canvas坐标系下的位置
 */
QPoint DAFigureChartEditorWidgetOverlay::PrivateData::mapPosToPlotCanvas(const QPoint& figPos)
{
    return mapPosToPlotCanvas(mActivePlot.data(), figPos);
}

/**
 * @brief 将最后记录的鼠标位置映射到当前激活Plot的canvas坐标系
 * @return 映射到canvas坐标系下的位置
 */
QPoint DAFigureChartEditorWidgetOverlay::PrivateData::mapMousePosToPlotCanvas()
{
    return mapPosToPlotCanvas(mLastFigureMousePos);
}
//----------------------------------------------------
// DAFigureChartEditorWidgetOverlay
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param fig 关联的QwtFigure
 * @param funFactory 图表编辑器工厂函数
 */
DAFigureChartEditorWidgetOverlay::DAFigureChartEditorWidgetOverlay(QwtFigure* fig, FpChartEditorFactory funFactory)
    : DAFigureWidgetOverlay(fig), DA_PIMPL_CONSTRUCT
{
    setBuiltInFunctionsEnable(QwtFigureWidgetOverlay::FunResizePlot, false);
    setChartEditorFactory(funFactory);
    d_ptr->mActivePlot = fig->currentAxes();
    // 这里先不创建editor，有可能会切换gca
    connect(this, &DAFigureWidgetOverlay::activeWidgetChanged, this, &DAFigureChartEditorWidgetOverlay::onActiveWidgetChanged);
}

/**
 * @brief 析构函数
 */
DAFigureChartEditorWidgetOverlay::~DAFigureChartEditorWidgetOverlay()
{
}

/**
 * @brief 设置图表编辑器工厂函数
 * @param funFactory 图表编辑器工厂函数
 */
void DAFigureChartEditorWidgetOverlay::setChartEditorFactory(FpChartEditorFactory funFactory)
{
    DA_D(d);
    d->mFunFactory = funFactory;
}

/**
 * @brief 获取图表编辑器工厂函数
 * @return 图表编辑器工厂函数
 */
DAFigureChartEditorWidgetOverlay::FpChartEditorFactory DAFigureChartEditorWidgetOverlay::getChartEditorFactory() const
{
    return d_ptr->mFunFactory;
}

/**
 * @brief 自动启动
 *
 * 自动启动时针对不需要第一次左键点击的编辑功能，例如给绘图添加一个水平线竖直线，
 * 这种只需要鼠标移动到激活的绘图就会激活editor，而不是在点击绘图时激活editor
 * @param autoStart
 */
void DA::DAFigureChartEditorWidgetOverlay::setAutoStart(bool autoStart)
{
    DA_D(d);
    d->mAutoStart = autoStart;
}

/**
 * @brief 是否自动启动
 * @return 是否自动启动
 */
bool DAFigureChartEditorWidgetOverlay::isAutoStart() const
{
    return d_ptr->mAutoStart;
}

/**
 * @brief 是否激活编辑器
 * @return 是否激活编辑器
 */
bool DAFigureChartEditorWidgetOverlay::isChartEditorActive() const
{
    return d_ptr->mActiveEditor != nullptr;
}

/**
 * @brief 可以设置一个函数，用于在激活canvas后绘制
 *
 * 这种情况是在激活canvas后，需要在canvas上绘制一些内容，例如我要做个添加垂直线的编辑器，鼠标在激活的canvas上移动时，
 * 我需要在canvas上绘制一条垂直线，用于表示鼠标的位置。则可以通过此函数指针进行非侵入式的绘制。
 * @param painterFp
 */
void DAFigureChartEditorWidgetOverlay::setActiveChartCanvasPainter(FpActiveChartCanvasPainter painterFp)
{
    d_ptr->mActiveChartCanvasPainter = painterFp;
}

/**
 * @brief 获取激活的图表canvas绘制函数
 * @return 激活的图表canvas绘制函数
 */
DAFigureChartEditorWidgetOverlay::FpActiveChartCanvasPainter DAFigureChartEditorWidgetOverlay::getActiveChartCanvasPainter() const
{
    return d_ptr->mActiveChartCanvasPainter;
}

/**
 * @brief 将源Widget的rect映射到目标Widget的坐标系
 * @param sourceWidget 源Widget
 * @param targetWidget 目标Widget
 * @param sourceRect 源Widget的rect
 * @return 目标Widget的rect
 */
QRect DAFigureChartEditorWidgetOverlay::mapRectTo(const QWidget* sourceWidget, const QWidget* targetWidget, const QRect& sourceRect)
{
    if (!sourceWidget || !targetWidget || sourceRect.isNull()) {
        return QRect();
    }

    // 获取源Widget的左上角和右下角在屏幕上的全局坐标
    QPoint topLeftGlobal     = sourceWidget->mapToGlobal(sourceRect.topLeft());
    QPoint bottomRightGlobal = sourceWidget->mapToGlobal(sourceRect.bottomRight());

    // 将全局坐标映射到目标Widget的坐标系
    QPoint topLeftTarget     = targetWidget->mapFromGlobal(topLeftGlobal);
    QPoint bottomRightTarget = targetWidget->mapFromGlobal(bottomRightGlobal);

    return QRect(topLeftTarget, bottomRightTarget);
}

/**
 * @brief 获取图表编辑器
 * @return 图表编辑器
 */
DAAbstractChartEditor* DA::DAFigureChartEditorWidgetOverlay::getChartEditor() const
{
    return d_ptr->mActiveEditor;
}

/**
 * @brief 活跃Widget切换时触发，用于在新激活的绘图上创建编辑器
 * @param oldActive 之前活跃的Widget
 * @param newActive 新激活的Widget
 */
void DAFigureChartEditorWidgetOverlay::onActiveWidgetChanged(QWidget* oldActive, QWidget* newActive)
{
    DA_D(d);
    Q_UNUSED(oldActive);

    if (!(d->mFunFactory) || !newActive || !(d->mActiveEditor.isNull())) {
        return;
    }
    QwtPlot* plot = qobject_cast< QwtPlot* >(newActive);
    if (!plot) {
        return;
    }
    createChartEditor(plot);
}

/**
 * @brief 编辑器完成编辑时触发，清理编辑器并发出finished信号
 * @param isCancel 是否取消编辑
 */
void DAFigureChartEditorWidgetOverlay::onEditorFinished(bool isCancel)
{
    DA_D(d);
    if (d->mActiveEditor) {
        d->mActiveEditor->setEnabled(false);
        d->mActiveEditor->deleteLater();
        d->mActiveEditor = nullptr;
    }
    // 自身也需要完成
    Q_EMIT finished(isCancel);
}

/**
 * @brief 编辑器开始编辑时触发
 */
void DAFigureChartEditorWidgetOverlay::onEditorBegin()
{
    // 开始编辑DAFigureChartEditorWidgetOverlay可以隐藏取消
    // hide();
}

/**
 * @brief 创建图表编辑器并关联到指定绘图
 * @param plot 目标QwtPlot
 */
void DAFigureChartEditorWidgetOverlay::createChartEditor(QwtPlot* plot)
{
    DA_D(d);
    if (!plot || !(d->mFunFactory)) {
        return;
    }
    if (d->mActiveEditor) {
        d->mActiveEditor->setEnabled(false);
        d->mActiveEditor->deleteLater();
    }
    if (d->mActivePlot != plot) {
        d->mActivePlot = plot;
    }
    d->mActiveEditor = d->mFunFactory(d->mActivePlot.data());
    if (d->mActiveEditor) {
        d->mActiveEditor->setEnabled(true);
        connect(d->mActiveEditor, &DAAbstractChartEditor::finishedEdit, this, &DAFigureChartEditorWidgetOverlay::onEditorFinished);
        connect(d->mActiveEditor, &DAAbstractChartEditor::beginEdit, this, &DAFigureChartEditorWidgetOverlay::onEditorBegin);
    }
}

/**
 * @brief 鼠标移动事件处理
 * @param me 鼠标事件
 */
void DAFigureChartEditorWidgetOverlay::mouseMoveEvent(QMouseEvent* me)
{
    DA_D(d);
#if DAFigureChartEditorWidgetOverlay_DebugPrint
    qDebug() << "DAFigureChartEditorWidgetOverlay::mouseMoveEvent";
#endif
    d->mLastFigureMousePos = compat::eventPos(me);
    if (d->mAutoStart) {
        if (!d->mActiveEditor) {
            // 检查当前鼠标所在绘图是否在当前激活的绘图上
            QwtPlot* activePlot = figure()->plotUnderPos(d->mLastFigureMousePos);
            if (activePlot && (activePlot == currentActivePlot())) {
                createChartEditor(activePlot);
            }
        }
    }
    if (d->mActiveEditor) {
        // 这里要把事件传递过去
        QPoint plotPos = d_ptr->mapMousePosToPlotCanvas();
        // 把事件传递给editor
        QMouseEvent mappedEvent(
            QEvent::MouseButtonPress, plotPos, plotPos, compat::eventGlobalPos(me), me->button(), me->buttons(), me->modifiers()
        );
        d->mActiveEditor->mouseMoveEvent(&mappedEvent);
    } else {
        DAFigureWidgetOverlay::mouseMoveEvent(me);
    }
}

/**
 * @brief 鼠标释放事件处理
 * @param me 鼠标事件
 */
void DAFigureChartEditorWidgetOverlay::mouseReleaseEvent(QMouseEvent* me)
{
    DA_D(d);
    d->mLastFigureMousePos = compat::eventPos(me);
    if (d->mActiveEditor) {
        // 这里要把事件传递过去
        QPoint plotPos = d_ptr->mapMousePosToPlotCanvas();
        // 把事件传递给editor
        QMouseEvent mappedEvent(
            QEvent::MouseButtonPress, plotPos, plotPos, compat::eventGlobalPos(me), me->button(), me->buttons(), me->modifiers()
        );
        d->mActiveEditor->mouseReleaseEvent(&mappedEvent);
        releaseMouse();
        me->accept();
    } else {
        DAFigureWidgetOverlay::mouseReleaseEvent(me);
    }
}

/**
 * @brief 鼠标按下事件处理
 * @param me 鼠标事件
 */
void DAFigureChartEditorWidgetOverlay::mousePressEvent(QMouseEvent* me)
{
    DA_D(d);
    d->mLastFigureMousePos = compat::eventPos(me);
#if DAFigureChartEditorWidgetOverlay_DebugPrint
    qDebug() << "DAFigureChartEditorWidgetOverlay::mousePressEvent(" << d->mLastFigureMousePos
             << "),mActiveEditor=" << d->mActiveEditor;
#endif
    DAFigureWidgetOverlay::mousePressEvent(me);
    if (me->isAccepted()) {
        return;
    }
    if (!d->mActiveEditor) {
        if (QwtPlot* activePlot = currentActivePlot()) {
            createChartEditor(activePlot);
        }
    }
    if (d->mActiveEditor) {
        // 这里要把第一个点击传递过去
        QPoint canvasPos = d_ptr->mapMousePosToPlotCanvas();
        // 把事件传递给editor
        QMouseEvent mappedEvent(
            QEvent::MouseButtonPress, canvasPos, canvasPos, compat::eventGlobalPos(me), me->button(), me->buttons(), me->modifiers()
        );
        d->mActiveEditor->mousePressEvent(&mappedEvent);
        grabMouse();
        me->accept();
    }
}


/**
 * @brief 键盘按键事件处理
 * @param ke 键盘事件
 */
void DAFigureChartEditorWidgetOverlay::keyPressEvent(QKeyEvent* ke)
{
    DA_D(d);
    if (d->mActiveEditor) {
        QKeyEvent nke(
            ke->type(),
            ke->key(),
            ke->modifiers(),
            ke->nativeScanCode(),
            ke->nativeVirtualKey(),
            ke->nativeModifiers(),
            ke->text(),
            ke->isAutoRepeat(),
            ke->count()
        );
        if (d->mActiveEditor->keyPressEvent(&nke)) {
            ke->accept();
            return;
        }
    }
    DAFigureWidgetOverlay::keyPressEvent(ke);
}

/**
 * @brief 绘制事件处理，在激活的canvas上绘制自定义内容
 * @param pe 绘制事件
 */
void DAFigureChartEditorWidgetOverlay::paintEvent(QPaintEvent* pe)
{
    DA_D(d);
    DAFigureWidgetOverlay::paintEvent(pe);
    if (d->mActiveChartCanvasPainter) {
        QwtPlot* activePlot = currentActivePlot();
        if (activePlot) {
            QPainter painter(this);
            QWidget* canvas = activePlot->canvas();
            if (!canvas) {
                return;
            }
            QRect canvasRect = mapRectTo(activePlot, figure(), canvas->geometry());
            d->mActiveChartCanvasPainter(&painter, d->mLastFigureMousePos, canvasRect);
        }
    }
}
}
