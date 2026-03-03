#include "DAFigureChartEditorWidgetOverlay.h"
#include <QPoint>
#include <QMouseEvent>
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "da_qt5qt6_compat.hpp"
#include "DAAbstractChartEditor.h"
namespace DA
{
class DAFigureChartEditorWidgetOverlay::PrivateData
{
    DA_DECLARE_PUBLIC(DAFigureChartEditorWidgetOverlay)
public:
    PrivateData(DAFigureChartEditorWidgetOverlay* p);
    QPoint mapPosToPlot(const QwtPlot* plot, const QPoint& figPos);
    QPoint mapPosToPlot(const QPoint& figPos);
    QPoint mapMousePosToPlot();

public:
    DAFigureChartEditorWidgetOverlay::FpChartEditorFactory m_funFactory { nullptr };
    QPointer< QwtPlot > m_activePlot;
    QPointer< DAAbstractChartEditor > m_activeEditor;
    QPoint m_lastFigureMousePos;
};

DAFigureChartEditorWidgetOverlay::PrivateData::PrivateData(DAFigureChartEditorWidgetOverlay* p) : q_ptr(p)
{
}

QPoint DAFigureChartEditorWidgetOverlay::PrivateData::mapPosToPlot(const QwtPlot* plot, const QPoint& figPos)
{
    if (!plot) {
        return QPoint();
    }
    QwtFigure* fig = q_ptr->figure();
    if (plot->parentWidget() == fig) {
        return plot->mapFrom(fig, figPos);
    }
    // 先转换到全局坐标
    QPoint globalPos = fig->mapToGlobal(figPos);
    return plot->mapFromGlobal(globalPos);
}

QPoint DAFigureChartEditorWidgetOverlay::PrivateData::mapPosToPlot(const QPoint& figPos)
{
    return mapPosToPlot(m_activePlot.data(), figPos);
}

QPoint DAFigureChartEditorWidgetOverlay::PrivateData::mapMousePosToPlot()
{
    return mapPosToPlot(m_lastFigureMousePos);
}
//----------------------------------------------------
// DAFigureChartEditorWidgetOverlay
//----------------------------------------------------
DAFigureChartEditorWidgetOverlay::DAFigureChartEditorWidgetOverlay(QwtFigure* fig, FpChartEditorFactory funFactory)
    : DAFigureWidgetOverlay(fig), DA_PIMPL_CONSTRUCT
{
    setTransparentForMouseEvents(false);  // 截取所有鼠标事件
    setBuiltInFunctionsEnable(QwtFigureWidgetOverlay::FunResizePlot, false);
    // 先赋值plot，再设置工厂
    d_ptr->m_activePlot = fig->currentAxes();
    setChartEditorFactory(funFactory);
    connect(this, &DAFigureWidgetOverlay::activeWidgetChanged, this, &DAFigureChartEditorWidgetOverlay::onActiveWidgetChanged);
}

DAFigureChartEditorWidgetOverlay::~DAFigureChartEditorWidgetOverlay()
{
}
void DAFigureChartEditorWidgetOverlay::setChartEditorFactory(FpChartEditorFactory funFactory)
{
    DA_D(d);
    d->m_funFactory = funFactory;
    if (d->m_activePlot) {
        d->m_activeEditor = funFactory(d->m_activePlot.data());
        if (d->m_activeEditor) {
            d->m_activeEditor->setEnabled(true);
            connect(d->m_activeEditor,
                    &DAAbstractChartEditor::editorFinished,
                    this,
                    &DAFigureChartEditorWidgetOverlay::onEditorFinished);
            connect(d->m_activeEditor,
                    &DAAbstractChartEditor::editorFinished,
                    this,
                    &DAFigureChartEditorWidgetOverlay::onChartEditorFinished);
        }
    }
}

DAFigureChartEditorWidgetOverlay::FpChartEditorFactory DAFigureChartEditorWidgetOverlay::getChartEditorFactory() const
{
    return d_ptr->m_funFactory;
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
    // 激活了窗口就发射完成信号DAFigureWidget收到完成信号后应该隐藏
    hide();
    Q_EMIT finished(false);
}

void DAFigureChartEditorWidgetOverlay::onEditorFinished(bool isCancel)
{
    DA_D(d);
    if (d->m_activeEditor) {
        d->m_activeEditor->deleteLater();
        d->m_activeEditor = nullptr;
    }
    d->m_activePlot = nullptr;
    // 自身也需要完成
    Q_EMIT finished(isCancel);
}

void DAFigureChartEditorWidgetOverlay::createChartEditor(QwtPlot* plot)
{
    DA_D(d);
    if (!plot || !(d->m_funFactory) || d->m_activeEditor) {
        return;
    }
    d->m_activePlot   = plot;
    d->m_activeEditor = d->m_funFactory(plot);
    if (d->m_activeEditor) {
        d->m_activeEditor->setEnabled(true);
        connect(d->m_activeEditor,
                &DAAbstractChartEditor::editorFinished,
                this,
                &DAFigureChartEditorWidgetOverlay::onEditorFinished);
        connect(d->m_activeEditor,
                &DAAbstractChartEditor::editorFinished,
                this,
                &DAFigureChartEditorWidgetOverlay::onChartEditorFinished);
    }
}

void DAFigureChartEditorWidgetOverlay::mouseMoveEvent(QMouseEvent* me)
{
    DA_D(d);
    DAFigureWidgetOverlay::mouseMoveEvent(me);
    d->m_lastFigureMousePos = compat::eventPos(me);
    // 这里要把事件传递过去
    QPoint plotPos = d_ptr->mapMousePosToPlot();
    if (d->m_activeEditor) {
        // 把事件传递给editor
        QMouseEvent mappedEvent(QEvent::MouseButtonPress,
                                plotPos,
                                plotPos,
                                me->globalPosition().toPoint(),
                                me->button(),
                                me->buttons(),
                                me->modifiers());
        d->m_activeEditor->mouseMoveEvent(&mappedEvent);
    }
}

void DAFigureChartEditorWidgetOverlay::mouseReleaseEvent(QMouseEvent* me)
{
    DA_D(d);
    DAFigureWidgetOverlay::mouseReleaseEvent(me);
    d->m_lastFigureMousePos = compat::eventPos(me);
    // 这里要把事件传递过去
    QPoint plotPos = d_ptr->mapMousePosToPlot();
    if (d->m_activeEditor) {
        // 把事件传递给editor
        QMouseEvent mappedEvent(QEvent::MouseButtonPress,
                                plotPos,
                                plotPos,
                                me->globalPosition().toPoint(),
                                me->button(),
                                me->buttons(),
                                me->modifiers());
        d->m_activeEditor->mouseReleaseEvent(&mappedEvent);
    }
}

void DAFigureChartEditorWidgetOverlay::mousePressEvent(QMouseEvent* me)
{
    DA_D(d);
    DAFigureWidgetOverlay::mousePressEvent(me);
    d->m_lastFigureMousePos = compat::eventPos(me);
    // 这里要把第一个点击传递过去
    QPoint plotPos = d_ptr->mapMousePosToPlot();
    if (d->m_activeEditor) {
        // 把事件传递给editor
        QMouseEvent mappedEvent(QEvent::MouseButtonPress,
                                plotPos,
                                plotPos,
                                me->globalPosition().toPoint(),
                                me->button(),
                                me->buttons(),
                                me->modifiers());
        d->m_activeEditor->mousePressEvent(&mappedEvent);
    }
}

}
