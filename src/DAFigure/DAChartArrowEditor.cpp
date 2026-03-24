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
#include "qwt_text.h"

#include <cmath>

namespace DA
{
class DAChartArrowEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartArrowEditor)
public:
    QPen m_arrowPen { Qt::black };
    qreal m_arrowSize { 8.0 };
    qreal m_arrowLineWidth { 1.0 };
    QwtPlotArrowMarker::EndpointStyle m_startEndType { QwtPlotArrowMarker::NoEndpoint };
    QwtPlotArrowMarker::EndpointStyle m_endEndType { QwtPlotArrowMarker::Triangle };

    qreal m_arrowLength { 20.0 };
    QwtPlotArrowMarker* m_marker { nullptr };

public:
    PrivateData(DAChartArrowEditor* p) : q_ptr(p)
    {
    }

    ~PrivateData()
    {
        if (m_marker) {
            m_marker->detach();
            delete m_marker;
            m_marker = nullptr;
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

void DAChartArrowEditor::setArrowLinePen(const QPen& pen)
{
    d_ptr->m_arrowPen = pen;
}

QPen DAChartArrowEditor::getArrowLinePen() const
{
    return d_ptr->m_arrowPen;
}

void DAChartArrowEditor::setArrowSize(qreal size)
{
    d_ptr->m_arrowSize = size;
}

qreal DAChartArrowEditor::getArrowSize() const
{
    return d_ptr->m_arrowSize;
}

void DAChartArrowEditor::setStartEndType(QwtPlotArrowMarker::EndpointStyle type)
{
    d_ptr->m_startEndType = type;
}

QwtPlotArrowMarker::EndpointStyle DAChartArrowEditor::getStartEndType() const
{
    return d_ptr->m_startEndType;
}

void DAChartArrowEditor::setEndEndType(QwtPlotArrowMarker::EndpointStyle type)
{
    d_ptr->m_endEndType = type;
}

QwtPlotArrowMarker::EndpointStyle DAChartArrowEditor::getEndEndType() const
{
    return d_ptr->m_endEndType;
}

QwtPlotItem* DAChartArrowEditor::takeItem()
{
    QwtPlotItem* item = d_ptr->m_marker;
    d_ptr->m_marker   = nullptr;
    return item;
}

QwtPlotItem* DAChartArrowEditor::createPlotItem(const QPointF& startPoint, const QPointF& endPoint)
{
    return createArrowMarker(startPoint, endPoint);
}

void DAChartArrowEditor::updatePreview(const QVector< QPointF >& points)
{
    DA_D(d);
    if (points.size() < 2) {
        return;
    }
    QPointF startPoint = points[ 0 ];
    QPointF endPoint   = points[ 1 ];
    if (!d->m_marker) {
        d->m_marker = createArrowMarker(startPoint, endPoint);
        d->m_marker->attach(plot());
    } else {
        // 更新标记位置
        d->m_marker->setPoints(startPoint, endPoint);
        if(QwtPlot* p = d->m_marker->plot()){
            p->replot();
        }
    }
}


QwtPlotArrowMarker* DAChartArrowEditor::createArrowMarker(const QPointF& startPoint, const QPointF& endPoint)
{
    // 创建标记
    QwtPlotArrowMarker* marker = new QwtPlotArrowMarker();
    marker->setLinePen(getArrowLinePen());
    marker->setHeadStyle(getStartEndType());
    marker->setTailStyle(getEndEndType());
    marker->setTailSize(getArrowSize());
    marker->setPoints(startPoint, endPoint);

    // 返回标记（线条由图表管理）
    return marker;
}


}  // End Of Namespace DA
