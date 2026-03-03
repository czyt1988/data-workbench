#include "DAFigureChartEditorWidgetOverlay.h"
namespace DA
{
class DAFigureChartEditorWidgetOverlay::PrivateData
{
    DA_DECLARE_PUBLIC(DAFigureChartEditorWidgetOverlay)
public:
    PrivateData(DAFigureChartEditorWidgetOverlay* p);
    DAFigureChartEditorWidgetOverlay::FpChartEditorFactory m_funFactory { nullptr };
    QPointer< QwtPlot > m_activePlot;
    QPointer< DAAbstractChartEditor > m_activeEditor;
};

DAFigureChartEditorWidgetOverlay::PrivateData::PrivateData(DAFigureChartEditorWidgetOverlay* p) : q_ptr(p)
{
}
//----------------------------------------------------
// DAFigureChartEditorWidgetOverlay
//----------------------------------------------------
DAFigureChartEditorWidgetOverlay::DAFigureChartEditorWidgetOverlay(QwtFigure* fig, FpChartEditorFactory funFactory)
    : DAFigureWidgetOverlay(fig), DA_PIMPL_CONSTRUCT
{
    setTransparentForMouseEvents(false);  // 截取所有鼠标事件
    setBuiltInFunctionsEnable(QwtFigureWidgetOverlay::FunResizePlot, false);
    setChartEditorFactory(funFactory);
    connect(this, &DAFigureWidgetOverlay::activeWidgetChanged, this, &DAFigureChartEditorWidgetOverlay::onActiveWidgetChanged);
}

DAFigureChartEditorWidgetOverlay::~DAFigureChartEditorWidgetOverlay()
{
}
void DAFigureChartEditorWidgetOverlay::setChartEditorFactory(FpChartEditorFactory funFactory)
{
    d_ptr->m_funFactory = funFactory;
}
DAFigureChartEditorWidgetOverlay::FpChartEditorFactory DAFigureChartEditorWidgetOverlay::getChartEditorFactory() const
{
    return d_ptr->m_funFactory;
}

void DAFigureChartEditorWidgetOverlay::onActiveWidgetChanged(QWidget* oldActive, QWidget* newActive)
{
    DA_D(d);
    Q_UNUSED(oldActive);

    if (!(d->m_funFactory) || !newActive) {
        return;
    }
    QwtPlot* plot = qobject_cast< QwtPlot* >(newActive);
    if (!plot) {
        return;
    }
    d->m_activePlot               = plot;
    DAAbstractChartEditor* editor = d->m_funFactory(plot);
    editor->setEnabled(true);
    // 连接编辑器的完成信号
    connect(editor, &DAAbstractChartEditor::editorFinished, this, &DAFigureChartEditorWidgetOverlay::onEditorFinished);
    // 这里要把第一个点击传递过去
}

void DAFigureChartEditorWidgetOverlay::onEditorFinished(bool isCancel)
{
}

void DAFigureChartEditorWidgetOverlay::mouseMoveEvent(QMouseEvent* me)
{
}

void DAFigureChartEditorWidgetOverlay::mouseReleaseEvent(QMouseEvent* me)
{
}

void DAFigureChartEditorWidgetOverlay::mousePressEvent(QMouseEvent* me)
{
}

}
