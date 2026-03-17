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
    qreal m_arrowSize { 8.0 };
    qreal m_arrowLineWidth { 1.0 };
    DAArrowSymbol::ArrowEndType m_startEndType { DAArrowSymbol::NoEnd };
    DAArrowSymbol::ArrowEndType m_endEndType { DAArrowSymbol::SimpleEnd };
    DAArrowSymbol::OriginPosition m_originPosition { DAArrowSymbol::OriginAtEnd };
    qreal m_arrowLength { 20.0 };

public:
    PrivateData(DAChartArrowEditor* p) : q_ptr(p)
    {
    }

    ~PrivateData()
    {
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

void DAChartArrowEditor::setStartEndType(DAArrowSymbol::ArrowEndType type)
{
    d_ptr->m_startEndType = type;
}

DAArrowSymbol::ArrowEndType DAChartArrowEditor::getStartEndType() const
{
    return d_ptr->m_startEndType;
}

void DAChartArrowEditor::setEndEndType(DAArrowSymbol::ArrowEndType type)
{
    d_ptr->m_endEndType = type;
}

DAArrowSymbol::ArrowEndType DAChartArrowEditor::getEndEndType() const
{
    return d_ptr->m_endEndType;
}

void DAChartArrowEditor::setOriginPosition(DAArrowSymbol::OriginPosition pos)
{
    d_ptr->m_originPosition = pos;
}

DAArrowSymbol::OriginPosition DAChartArrowEditor::getOriginPosition() const
{
    return d_ptr->m_originPosition;
}

QwtPlotItem* DAChartArrowEditor::createPlotItem(const QPointF& startPoint, const QPointF& endPoint)
{
    return createArrowMarker(startPoint, endPoint);
}

void DAChartArrowEditor::updatePreviewItem(const QPointF& startPoint, const QPointF& currentPoint)
{
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


    // 调用基类方法创建箭头预览
    DAAbstractTwoPointEditor::updatePreviewItem(startPoint, currentPoint);
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
    QString label = QString("Arrow (%1, %2)").arg(startPoint.x(), 0, 'f', 2).arg(startPoint.y(), 0, 'f', 2);
    marker->setLabel(QwtText(label));
    marker->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);

    // 返回标记（线条由图表管理）
    marker->attach(plot);
    return marker;
}

}  // End Of Namespace DA