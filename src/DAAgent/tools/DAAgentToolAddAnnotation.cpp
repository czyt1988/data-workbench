#include "DAAgentToolAddAnnotation.h"
#include "qwt_plot_marker.h"
#include "qwt_text.h"
#include "qwt_symbol.h"
#include <QPainterPath>
#include <QColor>
#include <QJsonArray>

namespace DA
{
QJsonObject DAAgentToolAddAnnotation::getToolSpec() const
{
    return QJsonObject{
        {"name", "add_annotation"},
        {"description", "Add a text, arrow, or point annotation to a chart. Use figure_name to target a specific figure."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"chart_id", QJsonObject{{"type", "string"}, {"description", "Chart identifier. Empty or 'current' for active chart."}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name to target a specific figure. Empty for current active figure."}}},
                {"type", QJsonObject{{"type", "string"}, {"description", "Annotation type: text, arrow, point"}}},
                {"position", QJsonObject{{"type", "array"}, {"description", "Position [x, y] in data coordinates"}, {"items", QJsonObject{{"type", "number"}}}}},
                {"text", QJsonObject{{"type", "string"}, {"description", "Annotation text (for text type)"}}},
                {"color", QJsonObject{{"type", "string"}, {"description", "Annotation color (hex or name)"}}}
            }},
            {"required", QJsonArray{"type", "position"}}
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
    QJsonArray posArr = params["position"].toArray();
    if (posArr.size() < 2) {
        return errorResponse("position must be [x, y] array");
    }
    double x = posArr[ 0 ].toDouble();
    double y = posArr[ 1 ].toDouble();

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId) : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
    }

    QColor color = colorStr.isEmpty() ? Qt::red : QColor(colorStr);

    if (type == "text") {
        // Text annotation: QwtPlotMarker with text label
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setLabel(QwtText(text.isEmpty() ? QString::number(x) : text));
        marker->setValue(x, y);
        marker->setLabelAlignment(Qt::AlignTop | Qt::AlignRight);
        marker->attach(chart);
        chart->replot();
        return successResponse(QString("Added text annotation at (%1, %2)").arg(x).arg(y));
    }
    else if (type == "point") {
        // Point annotation: QwtPlotMarker with a symbol
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setValue(x, y);
        marker->setSymbol(new QwtSymbol(QwtSymbol::Ellipse, color, QPen(color, 1), QSize(10, 10)));
        marker->attach(chart);
        chart->replot();
        return successResponse(QString("Added point annotation at (%1, %2)").arg(x).arg(y));
    }
    else if (type == "arrow") {
        // Arrow annotation: shape item with arrow path
        // Draw a simple arrow from a computed start point to the target position
        QPainterPath path;
        // Compute a start point offset from the target (diagonally up-left)
        QPointF start(x - 1.0, y + 1.0);
        path.moveTo(start);
        path.lineTo(x, y);
        // Arrowhead (two short lines)
        double arrowSize = 0.05;
        path.lineTo(x - arrowSize, y + arrowSize);
        path.moveTo(x, y);
        path.lineTo(x + arrowSize, y + arrowSize);

        QwtPlotShapeItem* item = chart->addShapeItem(path, text);
        if (item) {
            QPen pen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            item->setPen(pen);
            item->setBrush(Qt::NoBrush);
        }
        chart->replot();
        return successResponse(QString("Added arrow annotation at (%1, %2)").arg(x).arg(y));
    }
    else {
        return errorResponse(QString("Unsupported annotation type: %1").arg(type));
    }
}
}  // namespace DA
