#include "DAAgentToolUpdateCurveStyle.h"
#include "DAChartUtil.h"
#include "qwt_plot_curve.h"
#include "qwt_symbol.h"
#include "qwt_text.h"
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QSize>

namespace DA
{
static Qt::PenStyle parsePenStyle(const QString& s)
{
    QString l = s.toLower();
    if (l == "dash") return Qt::DashLine;
    if (l == "dot") return Qt::DotLine;
    if (l == "dashdot") return Qt::DashDotLine;
    if (l == "dashdotdot") return Qt::DashDotDotLine;
    return Qt::SolidLine;
}

static QwtSymbol::Style parseSymbolStyle(const QString& s)
{
    QString l = s.toLower();
    if (l == "ellipse") return QwtSymbol::Ellipse;
    if (l == "rect") return QwtSymbol::Rect;
    if (l == "diamond") return QwtSymbol::Diamond;
    if (l == "triangle") return QwtSymbol::Triangle;
    if (l == "dtriangle") return QwtSymbol::DTriangle;
    if (l == "utriangle") return QwtSymbol::UTriangle;
    if (l == "ltriangle") return QwtSymbol::LTriangle;
    if (l == "rtriangle") return QwtSymbol::RTriangle;
    if (l == "cross") return QwtSymbol::Cross;
    if (l == "xcross") return QwtSymbol::XCross;
    if (l == "star1") return QwtSymbol::Star1;
    if (l == "star2") return QwtSymbol::Star2;
    if (l == "hexagon") return QwtSymbol::Hexagon;
    if (l == "hline") return QwtSymbol::HLine;
    if (l == "vline") return QwtSymbol::VLine;
    return QwtSymbol::NoSymbol;
}

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
QJsonObject DAAgentToolUpdateCurveStyle::getToolSpec() const
{
    return QJsonObject{
        {"name", "update_curve_style"},
        {"description", "Modify the appearance of an existing curve (color, line style, width, symbol, fill). "
         "Identify the curve by its title or index. Use list_figures to discover curve names."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"chart_id", QJsonObject{{"type", "string"}, {"description", "Chart identifier (title or index). Empty or 'current' for active chart."}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name to target a specific figure. Empty for current active figure."}}},
                {"curve_name", QJsonObject{{"type", "string"}, {"description", "Curve title or index (0-based). Use list_figures to find curve names."}}},
                {"color", QJsonObject{{"type", "string"}, {"description", "Line color (hex or name, e.g. '#FF0000' or 'red')"}}},
                {"width", QJsonObject{{"type", "number"}, {"description", "Line width"}}},
                {"style", QJsonObject{{"type", "string"}, {"description", "Line style: solid, dash, dot, dashdot, dashdotdot"}}},
                {"symbol", QJsonObject{{"type", "string"}, {"description", "Marker symbol: none, ellipse, rect, diamond, triangle, dtriangle, utriangle, cross, xcross, star1"}}},
                {"symbol_size", QJsonObject{{"type", "integer"}, {"description", "Marker size in pixels (default 8)"}}},
                {"fill_color", QJsonObject{{"type", "string"}, {"description", "Fill color under curve (hex or name, e.g. '#0000FF80' for semi-transparent blue)"}}}
            }},
            {"required", QJsonArray{"curve_name"}}
        }}
    };
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolUpdateCurveStyle::execute(const QJsonObject& params)
{
    QString chartId    = params["chart_id"].toString();
    QString figureName = params["figure_name"].toString();
    QString curveName  = params["curve_name"].toString();
    QString colorStr   = params["color"].toString();
    double  width      = params["width"].toDouble(0.0);
    QString styleStr   = params["style"].toString();
    QString symbolStr  = params["symbol"].toString();
    int     symbolSize = params["symbol_size"].toInt(8);
    QString fillColor  = params["fill_color"].toString();

    if (curveName.isEmpty()) {
        return errorResponse("curve_name is required");
    }

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId) : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
    }

    // Find the curve by title or index
    const QList< QwtPlotCurve* > curves = chart->getCurves();
    QwtPlotCurve* target = nullptr;

    if (curves.isEmpty()) {
        return errorResponse("No curves found in this chart");
    }

    // Try integer index first
    bool ok = false;
    int idx = curveName.toInt(&ok);
    if (ok && idx >= 0 && idx < curves.size()) {
        target = curves[ idx ];
    } else {
        // Match by title
        for (QwtPlotCurve* c : curves) {
            if (c && c->title().text() == curveName) {
                target = c;
                break;
            }
        }
    }

    if (!target) {
        return errorResponse(QString("Curve '%1' not found. Use list_figures to see available curves.").arg(curveName));
    }

    bool changed = false;

    // Pen (color / width / style)
    if (!colorStr.isEmpty() || width > 0 || !styleStr.isEmpty()) {
        QPen pen = target->pen();
        if (!colorStr.isEmpty()) {
            pen.setColor(QColor(colorStr));
        }
        if (width > 0) {
            pen.setWidthF(width);
        }
        if (!styleStr.isEmpty()) {
            pen.setStyle(parsePenStyle(styleStr));
        }
        target->setPen(pen);
        changed = true;
    }

    // Symbol
    if (!symbolStr.isEmpty()) {
        QwtSymbol::Style symStyle = parseSymbolStyle(symbolStr);
        if (symStyle == QwtSymbol::NoSymbol) {
            target->setSymbol(nullptr);
        } else {
            DAChartUtil::setCurveSymbol(target, symStyle, QSize(symbolSize, symbolSize));
        }
        changed = true;
    }

    // Fill under curve
    if (!fillColor.isEmpty()) {
        target->setBrush(QBrush(QColor(fillColor)));
        changed = true;
    }

    if (changed) {
        chart->replot();
    }

    return successResponse(QString("Curve '%1' style updated").arg(target->title().text()));
}
}  // namespace DA
