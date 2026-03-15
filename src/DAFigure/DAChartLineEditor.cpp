#include "DAChartLineEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QKeyEvent>
#include <QPen>
#include <QPainterPath>
#include "DAChartWidget.h"
#include "da_qt5qt6_compat.hpp"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"

namespace DA
{
class DAChartLineEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartLineEditor)
public:
    QColor m_lineColor { Qt::black };
    qreal m_lineWidth { 1.0 };
    Qt::PenStyle m_lineStyle { Qt::SolidLine };
    QwtPlotItem* m_previewCurve { nullptr };

public:
    PrivateData(DAChartLineEditor* p) : q_ptr(p)
    {
    }

    ~PrivateData()
    {
        releasePreviewCurve();
    }

    void releasePreviewCurve()
    {
        if (m_previewCurve) {
            m_previewCurve->detach();
            delete m_previewCurve;
            m_previewCurve = nullptr;
        }
    }
};

DAChartLineEditor::DAChartLineEditor(QwtPlot* parent) : DAAbstractTwoPointEditor(parent), DA_PIMPL_CONSTRUCT
{
}

DAChartLineEditor::~DAChartLineEditor()
{
}

int DAChartLineEditor::rtti() const
{
    return DAAbstractChartEditor::RTTILineEditor;
}

void DAChartLineEditor::setLineColor(const QColor& color)
{
    if (d_ptr->m_lineColor == color) {
        return;
    }
    d_ptr->m_lineColor = color;
}

QColor DAChartLineEditor::getLineColor() const
{
    return d_ptr->m_lineColor;
}

void DAChartLineEditor::setLineWidth(qreal width)
{
    if (qFuzzyCompare(d_ptr->m_lineWidth, width)) {
        return;
    }
    d_ptr->m_lineWidth = width;
}

qreal DAChartLineEditor::getLineWidth() const
{
    return d_ptr->m_lineWidth;
}

void DAChartLineEditor::setLineStyle(Qt::PenStyle style)
{
    if (d_ptr->m_lineStyle == style) {
        return;
    }
    d_ptr->m_lineStyle = style;
}

Qt::PenStyle DAChartLineEditor::getLineStyle() const
{
    return d_ptr->m_lineStyle;
}

QwtPlotItem* DAChartLineEditor::createPlotItem(const QPointF& startPoint, const QPointF& endPoint)
{
    // 创建曲线项
    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    // 设置线条属性
    QPen pen;
    pen.setColor(d_ptr->m_lineColor);
    pen.setWidthF(d_ptr->m_lineWidth);
    pen.setStyle(d_ptr->m_lineStyle);
    curve->setPen(pen);

    // 设置数据点
    QVector< QPointF > points;
    points.append(startPoint);
    points.append(endPoint);
    curve->setSamples(points);

    // 设置标题
    QString title = QString("Line (%1, %2) -> (%3, %4)")
                        .arg(startPoint.x(), 0, 'f', 2)
                        .arg(startPoint.y(), 0, 'f', 2)
                        .arg(endPoint.x(), 0, 'f', 2)
                        .arg(endPoint.y(), 0, 'f', 2);
    curve->setTitle(title);

    // 附加到图表
    curve->attach(plot());

    return curve;
}

void DAChartLineEditor::updatePreviewItem(const QPointF& startPoint, const QPointF& currentPoint)
{
    // 清理之前的预览曲线
    d_ptr->releasePreviewCurve();

    // 创建预览曲线
    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    // 设置预览线条样式（虚线）
    QPen pen;
    pen.setColor(d_ptr->m_lineColor);
    pen.setWidthF(d_ptr->m_lineWidth);
    pen.setStyle(Qt::DashLine);
    curve->setPen(pen);

    // 设置数据点
    QVector< QPointF > points;
    points.append(startPoint);
    points.append(currentPoint);
    curve->setSamples(points);

    // 附加到图表
    curve->attach(plot());
    d_ptr->m_previewCurve = curve;
}

void DAChartLineEditor::clearPreviewItem()
{
    d_ptr->releasePreviewCurve();
}
}  // End Of Namespace DA