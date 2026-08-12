#include "DAAgentToolAddAnnotation.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_arrowmarker.h"
#include "qwt_text.h"
#include "qwt_symbol.h"
#include "qwt_scale_div.h"
#include <QColor>
#include <QJsonArray>
#include <QPointF>

namespace DA
{
QJsonObject DAAgentToolAddAnnotation::getToolSpec() const
{
    return QJsonObject{
        {"name", "add_annotation"},
        {"description", "Add a text, arrow, or point annotation to a chart. For text/point use 'position' [x,y]; for arrow use 'start' [x,y] and 'end' [x,y]. Use figure_name to target a specific figure."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"chart_id", QJsonObject{{"type", "string"}, {"description", "Chart identifier. Empty or 'current' for active chart."}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name to target a specific figure. Empty for current active figure."}}},
                {"type", QJsonObject{{"type", "string"}, {"description", "Annotation type: text, arrow, point. Arrow uses start/end instead of position."}}},
                {"position", QJsonObject{{"type", "array"}, {"description", "Position [x, y] in data coordinates (for text/point)"}, {"items", QJsonObject{{"type", "number"}}}}},
                {"start", QJsonObject{{"type", "array"}, {"description", "Arrow start point [x, y] in data coordinates (for arrow)"}, {"items", QJsonObject{{"type", "number"}}}}},
                {"end", QJsonObject{{"type", "array"}, {"description", "Arrow end point [x, y] in data coordinates (for arrow)"}, {"items", QJsonObject{{"type", "number"}}}}},
                {"text", QJsonObject{{"type", "string"}, {"description", "Annotation text (for text type, or label at arrow tip)"}}},
                {"color", QJsonObject{{"type", "string"}, {"description", "Annotation color (hex or name)"}}}
            }},
            {"required", QJsonArray{"type"}}
        }}
    };
}

QJsonObject DAAgentToolAddAnnotation::execute(const QJsonObject& params)
{
    QString chartId    = params["chart_id"].toString();
    QString figureName = params["figure_name"].toString();
    QString type        = params["type"].toString().toLower();
    QString text        = params["text"].toString();
    QString colorStr    = params["color"].toString();

    if (type.isEmpty()) {
        return errorResponse("type is required");
    }
    if (!colorStr.isEmpty() && !QColor(colorStr).isValid()) {
        return errorResponse(QString("Invalid color '%1'").arg(colorStr));
    }

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId) : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
    }

    QColor color = colorStr.isEmpty() ? Qt::red : QColor(colorStr);

    // Whether a point lies outside the current axis range (after replot).
    // Used to warn the caller that the annotation was added but is off-screen.
    auto pointOffScreen = [chart](const QPointF& p) -> bool {
        const QwtScaleDiv& xDiv = chart->axisScaleDiv(QwtPlot::xBottom);
        const QwtScaleDiv& yDiv = chart->axisScaleDiv(QwtPlot::yLeft);
        return p.x() < xDiv.lowerBound() || p.x() > xDiv.upperBound()
            || p.y() < yDiv.lowerBound() || p.y() > yDiv.upperBound();
    };

    if (type == "text") {
        // Text annotation: QwtPlotMarker with text label
        QJsonArray posArr = params["position"].toArray();
        if (posArr.size() < 2) {
            return errorResponse("position must be [x, y] array for text annotation");
        }
        double x = posArr[ 0 ].toDouble();
        double y = posArr[ 1 ].toDouble();

        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setLabel(QwtText(text.isEmpty() ? QString::number(x) : text));
        marker->setValue(x, y);
        marker->setLabelAlignment(Qt::AlignTop | Qt::AlignRight);
        marker->attach(chart);
        enableAutoScale(chart);
        chart->replot();
        QString warning;
        if (pointOffScreen(QPointF(x, y))) {
            warning = " (warning: position is outside the visible axis range)";
        }
        return successResponse(QString("Added text annotation at (%1, %2)%3").arg(x).arg(y).arg(warning));
    }
    else if (type == "point") {
        // Point annotation: QwtPlotMarker with a symbol
        QJsonArray posArr = params["position"].toArray();
        if (posArr.size() < 2) {
            return errorResponse("position must be [x, y] array for point annotation");
        }
        double x = posArr[ 0 ].toDouble();
        double y = posArr[ 1 ].toDouble();

        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setValue(x, y);
        marker->setSymbol(new QwtSymbol(QwtSymbol::Ellipse, color, QPen(color, 1), QSize(10, 10)));
        marker->attach(chart);
        enableAutoScale(chart);
        chart->replot();
        QString warning;
        if (pointOffScreen(QPointF(x, y))) {
            warning = " (warning: position is outside the visible axis range)";
        }
        return successResponse(QString("Added point annotation at (%1, %2)%3").arg(x).arg(y).arg(warning));
    }
    else if (type == "arrow") {
        // Arrow annotation: QwtPlotArrowMarker draws in canvas pixels
        // (zoom-stable size), unlike the old QPainterPath-in-data-units
        // approach which rendered as a sub-pixel speck on real charts.
        QJsonArray startArr = params["start"].toArray();
        QJsonArray endArr   = params["end"].toArray();
        if (startArr.size() < 2 || endArr.size() < 2) {
            return errorResponse("arrow requires start [x, y] and end [x, y] arrays");
        }
        QPointF start(startArr[ 0 ].toDouble(), startArr[ 1 ].toDouble());
        QPointF end(endArr[ 0 ].toDouble(), endArr[ 1 ].toDouble());

        QwtPlotArrowMarker* arrow = new QwtPlotArrowMarker(text);
        arrow->setPoints(start, end);
        arrow->setLinePen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        arrow->setHeadStyle(QwtPlotArrowMarker::Triangle);
        arrow->setTailStyle(QwtPlotArrowMarker::NoEndpoint);
        arrow->setHeadSize(8.0);  // pixels, matches DAChartArrowEditor default
        arrow->attach(chart);

        // Optional text label at the arrow tip
        if (!text.isEmpty()) {
            QwtPlotMarker* labelMarker = new QwtPlotMarker();
            labelMarker->setLabel(QwtText(text));
            labelMarker->setValue(end);
            labelMarker->setLabelAlignment(Qt::AlignTop | Qt::AlignLeft);
            labelMarker->attach(chart);
        }

        enableAutoScale(chart);
        chart->replot();
        QString warning;
        if (pointOffScreen(start) || pointOffScreen(end)) {
            warning = " (warning: arrow endpoints are outside the visible axis range)";
        }
        return successResponse(QString("Added arrow annotation from (%1, %2) to (%3, %4)%5")
            .arg(start.x()).arg(start.y()).arg(end.x()).arg(end.y()).arg(warning));
    }
    else {
        return errorResponse(QString("Unsupported annotation type: %1").arg(type));
    }
}
}  // namespace DA
