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
    DAFigureChartEditorWidgetOverlay::FpChartEditorFactory m_funFactory { nullptr };
    QPointer< QwtPlot > m_activePlot;
    QPointer< DAAbstractChartEditor > m_activeEditor;
    QPoint m_lastFigureMousePos;
    bool m_autoStart { false };
    FpActiveChartCanvasPainter m_activeChartCanvasPainter { nullptr };
};

DAFigureChartEditorWidgetOverlay::PrivateData::PrivateData(DAFigureChartEditorWidgetOverlay* p) : q_ptr(p)
{
}

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

QPoint DAFigureChartEditorWidgetOverlay::PrivateData::mapPosToPlotCanvas(const QPoint& figPos)
{
    return mapPosToPlotCanvas(m_activePlot.data(), figPos);
}

QPoint DAFigureChartEditorWidgetOverlay::PrivateData::mapMousePosToPlotCanvas()
{
    return mapPosToPlotCanvas(m_lastFigureMousePos);
}
//----------------------------------------------------
// DAFigureChartEditorWidgetOverlay
//----------------------------------------------------
DAFigureChartEditorWidgetOverlay::DAFigureChartEditorWidgetOverlay(QwtFigure* fig, FpChartEditorFactory funFactory)
    : DAFigureWidgetOverlay(fig), DA_PIMPL_CONSTRUCT
{
    setBuiltInFunctionsEnable(QwtFigureWidgetOverlay::FunResizePlot, false);
    setChartEditorFactory(funFactory);
    d_ptr->m_activePlot = fig->currentAxes();
    // 这里先不创建editor，有可能会切换gca
    connect(this, &DAFigureWidgetOverlay::activeWidgetChanged, this, &DAFigureChartEditorWidgetOverlay::onActiveWidgetChanged);
}

DAFigureChartEditorWidgetOverlay::~DAFigureChartEditorWidgetOverlay()
{
}
void DAFigureChartEditorWidgetOverlay::setChartEditorFactory(FpChartEditorFactory funFactory)
{
    DA_D(d);
    d->m_funFactory = funFactory;
}

DAFigureChartEditorWidgetOverlay::FpChartEditorFactory DAFigureChartEditorWidgetOverlay::getChartEditorFactory() const
{
    return d_ptr->m_funFactory;
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
    d->m_autoStart = autoStart;
}

/**
 * @brief 是否自动启动
 * @return 是否自动启动
 */
bool DAFigureChartEditorWidgetOverlay::isAutoStart() const
{
    return d_ptr->m_autoStart;
}

/**
 * @brief 是否激活编辑器
 * @return 是否激活编辑器
 */
bool DAFigureChartEditorWidgetOverlay::isChartEditorActive() const
{
    return d_ptr->m_activeEditor != nullptr;
}

void DAFigureChartEditorWidgetOverlay::setActiveChartCanvasPainter(FpActiveChartCanvasPainter painterFp)
{
    d_ptr->m_activeChartCanvasPainter = painterFp;
}

DAFigureChartEditorWidgetOverlay::FpActiveChartCanvasPainter DAFigureChartEditorWidgetOverlay::getActiveChartCanvasPainter() const
{
    return d_ptr->m_activeChartCanvasPainter;
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

void DAFigureChartEditorWidgetOverlay::onActiveWidgetChanged(QWidget* oldActive, QWidget* newActive)
{
    DA_D(d);
    Q_UNUSED(oldActive);

    if (!(d->m_funFactory) || !newActive || !(d->m_activeEditor.isNull())) {
        return;
    }
    QwtPlot* plot = qobject_cast< QwtPlot* >(newActive);
    if (!plot) {
        return;
    }
    createChartEditor(plot);
}

void DAFigureChartEditorWidgetOverlay::onEditorFinished(bool isCancel)
{
    DA_D(d);
    if (d->m_activeEditor) {
        d->m_activeEditor->setEnabled(false);
        d->m_activeEditor->deleteLater();
        d->m_activeEditor = nullptr;
    }
    // 自身也需要完成
    Q_EMIT finished(isCancel);
}

void DAFigureChartEditorWidgetOverlay::onEditorBegin()
{
    // 开始编辑DAFigureChartEditorWidgetOverlay可以隐藏取消
    // hide();
}

void DAFigureChartEditorWidgetOverlay::createChartEditor(QwtPlot* plot)
{
    DA_D(d);
    if (!plot || !(d->m_funFactory)) {
        return;
    }
    if (d->m_activeEditor) {
        d->m_activeEditor->setEnabled(false);
        d->m_activeEditor->deleteLater();
    }
    if (d->m_activePlot != plot) {
        d->m_activePlot = plot;
    }
    d->m_activeEditor = d->m_funFactory(d->m_activePlot.data());
    if (d->m_activeEditor) {
        d->m_activeEditor->setEnabled(true);
        connect(d->m_activeEditor, &DAAbstractChartEditor::finishedEdit, this, &DAFigureChartEditorWidgetOverlay::onEditorFinished);
        connect(d->m_activeEditor, &DAAbstractChartEditor::beginEdit, this, &DAFigureChartEditorWidgetOverlay::onEditorBegin);
    }
}

void DAFigureChartEditorWidgetOverlay::mouseMoveEvent(QMouseEvent* me)
{
    DA_D(d);
#if DAFigureChartEditorWidgetOverlay_DebugPrint
    qDebug() << "DAFigureChartEditorWidgetOverlay::mouseMoveEvent";
#endif
    d->m_lastFigureMousePos = compat::eventPos(me);
    if (d->m_autoStart) {
        if (!d->m_activeEditor) {
            // 检查当前鼠标所在绘图是否在当前激活的绘图上
            QwtPlot* activePlot = figure()->plotUnderPos(d->m_lastFigureMousePos);
            if (activePlot && (activePlot == currentActivePlot())) {
                createChartEditor(activePlot);
            }
        }
    }
    if (d->m_activeEditor) {
        // 这里要把事件传递过去
        QPoint plotPos = d_ptr->mapMousePosToPlotCanvas();
        // 把事件传递给editor
        QMouseEvent mappedEvent(
            QEvent::MouseButtonPress, plotPos, plotPos, me->globalPosition().toPoint(), me->button(), me->buttons(), me->modifiers()
        );
        d->m_activeEditor->mouseMoveEvent(&mappedEvent);
    } else {
        DAFigureWidgetOverlay::mouseMoveEvent(me);
    }
}

void DAFigureChartEditorWidgetOverlay::mouseReleaseEvent(QMouseEvent* me)
{
    DA_D(d);
    DAFigureWidgetOverlay::mouseReleaseEvent(me);
    d->m_lastFigureMousePos = compat::eventPos(me);
    if (d->m_activeEditor) {
        // 这里要把事件传递过去
        QPoint plotPos = d_ptr->mapMousePosToPlotCanvas();
        // 把事件传递给editor
        QMouseEvent mappedEvent(
            QEvent::MouseButtonPress, plotPos, plotPos, me->globalPosition().toPoint(), me->button(), me->buttons(), me->modifiers()
        );
        d->m_activeEditor->mouseReleaseEvent(&mappedEvent);
        releaseMouse();
        me->accept();
    } else {
        DAFigureWidgetOverlay::mouseReleaseEvent(me);
    }
}

void DAFigureChartEditorWidgetOverlay::mousePressEvent(QMouseEvent* me)
{
    DA_D(d);
    d->m_lastFigureMousePos = compat::eventPos(me);
#if DAFigureChartEditorWidgetOverlay_DebugPrint
    qDebug() << "DAFigureChartEditorWidgetOverlay::mousePressEvent(" << d->m_lastFigureMousePos
             << "),m_activeEditor=" << d->m_activeEditor;
#endif
    DAFigureWidgetOverlay::mousePressEvent(me);
    if (me->isAccepted()) {
        return;
    }
    if (!d->m_activeEditor) {
        if (QwtPlot* activePlot = currentActivePlot()) {
            createChartEditor(activePlot);
        }
    }
    if (d->m_activeEditor) {
        // 这里要把第一个点击传递过去
        QPoint plotPos = d_ptr->mapMousePosToPlotCanvas();
        // 把事件传递给editor
        QMouseEvent mappedEvent(
            QEvent::MouseButtonPress, plotPos, plotPos, me->globalPosition().toPoint(), me->button(), me->buttons(), me->modifiers()
        );
        d->m_activeEditor->mousePressEvent(&mappedEvent);
        grabMouse();
        me->accept();
    }
}


void DAFigureChartEditorWidgetOverlay::keyPressEvent(QKeyEvent* ke)
{
    DA_D(d);
    if (d->m_activeEditor) {
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
        if (d->m_activeEditor->keyPressEvent(&nke)) {
            ke->accept();
            return;
        }
    }
    DAFigureWidgetOverlay::keyPressEvent(ke);
}
void DAFigureChartEditorWidgetOverlay::paintEvent(QPaintEvent* pe)
{
    DA_D(d);
    DAFigureWidgetOverlay::paintEvent(pe);
    if (d->m_activeChartCanvasPainter) {
        QwtPlot* activePlot = currentActivePlot();
        if (activePlot) {
            QPainter painter(this);
            QWidget* canvas = activePlot->canvas();
            if (!canvas) {
                return;
            }
            QRect canvasRect = mapRectTo(activePlot, figure(), canvas->geometry());
            d->m_activeChartCanvasPainter(&painter, d->m_lastFigureMousePos, canvasRect);
        }
    }
}
}
