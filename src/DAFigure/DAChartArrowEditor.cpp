#include "DAChartArrowEditor.h"
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QKeyEvent>
#include <QPainterPath>
#include <QPen>
#include "DAChartWidget.h"
#include "da_qt5qt6_compat.hpp"
#include "qwt_plot.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_curve.h"
#include "qwt_symbol.h"
#include "qwt_text.h"
#include <cmath>

namespace DA
{
class DAChartArrowEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartArrowEditor)
public:
    QColor m_arrowColor { Qt::red };
    qreal m_arrowSize { 10.0 };
    DAArrowSymbol::ArrowStyle m_arrowStyle { DAArrowSymbol::FilledArrow };
    qreal m_arrowLineWidth { 1.0 };
    QwtPlotItem* m_previewCurve { nullptr };

public:
    PrivateData(DAChartArrowEditor* p) : q_ptr(p)
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

DAChartArrowEditor::DAChartArrowEditor(QwtPlot* parent) : DAAbstractTwoPointEditor(parent), DA_PIMPL_CONSTRUCT
{
}

DAChartArrowEditor::~DAChartArrowEditor()
{
}

int DAChartArrowEditor::rtti() const
{
    return DAAbstractChartEditor::RTTIArrowEditor;
}

void DAChartArrowEditor::setArrowLineWidth(qreal width)
{
    d_ptr->m_arrowLineWidth = width;
}

qreal DAChartArrowEditor::getArrowLineWidth() const
{
    return d_ptr->m_arrowLineWidth;
}

void DAChartArrowEditor::setArrowColor(const QColor& color)
{
    d_ptr->m_arrowColor = color;
}

QColor DAChartArrowEditor::getArrowColor() const
{
    return d_ptr->m_arrowColor;
}

void DAChartArrowEditor::setArrowSize(qreal size)
{
    d_ptr->m_arrowSize = size;
}

qreal DAChartArrowEditor::getArrowSize() const
{
    return d_ptr->m_arrowSize;
}

void DAChartArrowEditor::setArrowStyle(DAArrowSymbol::ArrowStyle style)
{
    d_ptr->m_arrowStyle = style;
}

DAArrowSymbol::ArrowStyle DAChartArrowEditor::getArrowStyle() const
{
    return d_ptr->m_arrowStyle;
}

QwtPlotItem* DAChartArrowEditor::createPlotItem(const QPointF& startPoint, const QPointF& endPoint)
{
    return createArrowMarker(startPoint, endPoint);
}

void DAChartArrowEditor::updatePreviewItem(const QPointF& startPoint, const QPointF& currentPoint)
{
    // 清理之前的预览曲线
    d_ptr->releasePreviewCurve();

    // 创建预览曲线
    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    // 设置线条样式
    QPen pen(Qt::gray, 1, Qt::DashLine);
    curve->setPen(pen);

    // 设置数据点
    QVector< QPointF > points;
    points.append(startPoint);
    points.append(currentPoint);
    curve->setSamples(points);

    // 附加到图表
    curve->attach(plot());
    d_ptr->m_previewCurve = curve;

    // 调用基类方法创建箭头预览
    DAAbstractTwoPointEditor::updatePreviewItem(startPoint, currentPoint);
}

void DAChartArrowEditor::clearPreviewItem()
{
    d_ptr->releasePreviewCurve();
    DAAbstractTwoPointEditor::clearPreviewItem();
}

QwtPlotItem* DAChartArrowEditor::createArrowMarker(const QPointF& startPoint, const QPointF& endPoint)
{
    // 计算箭头角度
    qreal angle = calculateArrowAngle(startPoint, endPoint);

    // 创建箭头符号（不使用拷贝构造函数）
    DAArrowSymbol* arrowSymbol = new DAArrowSymbol(getArrowColor(), getArrowSize(), getArrowStyle());
    arrowSymbol->setArrowAngle(angle);
    arrowSymbol->setArrowLineWidth(getArrowLineWidth());

    // 创建标记
    QwtPlotMarker* marker = new QwtPlotMarker();
    marker->setSymbol(arrowSymbol);
    marker->setValue(endPoint);  // 箭头尖端在终点位置

    // 设置标签（可选）
    QString label = QString("Arrow (%1, %2) -> (%3, %4)")
                        .arg(startPoint.x(), 0, 'f', 2)
                        .arg(startPoint.y(), 0, 'f', 2)
                        .arg(endPoint.x(), 0, 'f', 2)
                        .arg(endPoint.y(), 0, 'f', 2);
    marker->setLabel(QwtText(label));
    marker->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);

    // 创建线条（从起点到终点）
    QwtPlotCurve* line = new QwtPlotCurve();
    line->setRenderHint(QwtPlotItem::RenderAntialiased);

    QPen linePen(getArrowColor(), getArrowLineWidth());
    line->setPen(linePen);

    QVector< QPointF > points;
    points.append(startPoint);
    points.append(endPoint);
    line->setSamples(points);

    // 将线条附加到图表
    line->attach(plot());

    // 返回标记（线条由图表管理）
    return marker;
}

qreal DAChartArrowEditor::calculateArrowAngle(const QPointF& startPoint, const QPointF& endPoint) const
{
    // 计算角度（以度为单位）
    // QLineF的角度是从水平向右开始，逆时针为正
    QLineF line(startPoint, endPoint);
    qreal angle = line.angle();  // 返回0-360度

    // QwtSymbol的0度是向右，我们需要调整
    // 默认箭头是向右的（0度），我们需要旋转到线条方向
    return angle;
}


QwtPlotItem* createArrowMarkerPlotItem(QwtPlot* plot, const QPointF& startPoint, const QPointF& endPoint)
{
    // 计算箭头角度
    QLineF line(startPoint, endPoint);
    qreal angle = line.angle();  // 返回0-360度

    // 创建箭头符号
    DAArrowSymbol* arrowSymbol = new DAArrowSymbol(Qt::red, 10.0, DAArrowSymbol::FilledArrow);
    arrowSymbol->setArrowAngle(angle);

    // 创建标记
    QwtPlotMarker* marker = new QwtPlotMarker();
    marker->setSymbol(arrowSymbol);
    marker->setValue(endPoint);  // 箭头尖端在终点位置

    // 设置标签
    QString label = QString("Arrow (%1, %2) -> (%3, %4)")
                        .arg(startPoint.x(), 0, 'f', 2)
                        .arg(startPoint.y(), 0, 'f', 2)
                        .arg(endPoint.x(), 0, 'f', 2)
                        .arg(endPoint.y(), 0, 'f', 2);
    marker->setLabel(QwtText(label));
    marker->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);

    // 创建线条（从起点到终点）
    QwtPlotCurve* lineCurve = new QwtPlotCurve();
    lineCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

    QPen linePen(Qt::red, 1.0);
    lineCurve->setPen(linePen);

    QVector< QPointF > points;
    points.append(startPoint);
    points.append(endPoint);
    lineCurve->setSamples(points);

    // 将线条附加到图表
    lineCurve->attach(plot);

    // 返回标记（线条由图表管理）
    marker->attach(plot);
    return marker;
}

QwtPlotItem* createLinePlotItem(QwtPlot* plot, const QPointF& startPoint, const QPointF& endPoint)
{
    // 创建曲线项
    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    // 设置线条属性
    QPen pen(Qt::blue, 1.0, Qt::SolidLine);
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
    curve->attach(plot);

    return curve;
}
}  // End Of Namespace DA