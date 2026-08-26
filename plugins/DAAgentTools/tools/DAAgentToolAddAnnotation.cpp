#include "DAAgentToolAddAnnotation.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_arrowmarker.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_text.h"
#include "qwt_symbol.h"
#include "qwt_scale_div.h"
#include "qwt_date.h"
#include <QColor>
#include <QJsonArray>
#include <QPointF>
#include <QPainterPath>
#include <QBrush>
#include <QPen>
#include <QDateTime>
#include <optional>

namespace DA
{

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolAddAnnotation::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    // X 坐标在时间轴下的输入约定：秒 / 毫秒 / ISO 字符串均可，工具自动归一化
    static const QString xCoordDesc = QStringLiteral(
        "X coordinate. On a datetime x-axis, accepts Unix seconds, Unix milliseconds, "
        "or an ISO datetime string (e.g. \"2026-05-15T11:29:14\", \"2026-05-15 11:29:14\", "
        "\"2026-05-15\"); the tool auto-normalizes to axis units. On a linear x-axis, pass a number.");
    static const QString yCoordDesc =
        QStringLiteral("Y coordinate as a number (always numeric, never a datetime).");

    DAAgentToolSpec spec{QStringLiteral("add_annotation"),
                         QStringLiteral("Add a text, arrow, point, or region annotation to a chart. For text/point "
                                        "use 'position' [x,y]; for arrow use 'start' [x,y] and 'end' [x,y]; for "
                                        "region use 'start_x' and 'end_x' to highlight a vertical band. On a datetime "
                                        "x-axis, x coordinates accept Unix seconds, Unix milliseconds, or ISO datetime "
                                        "strings (auto-normalized). Use figure_name to target a specific figure.")};
    spec.addParam({QStringLiteral("chart_id"),
                   QStringLiteral("Chart identifier. Empty or 'current' for active chart."),
                   {Type::String}});
    spec.addParam({QStringLiteral("figure_name"),
                   QStringLiteral("Figure name to target a specific figure. Empty for current active figure."),
                   {Type::String}});
    spec.addParam({QStringLiteral("type"),
                   QStringLiteral("Annotation type: text, arrow, point, region. Arrow uses start/end; region uses "
                                  "start_x/end_x."),
                   {Type::String},
                   true});
    // position/start/end：数组元素允许 number | string（datetime 轴输入约定）
    const QList< Type > coordItemTypes{Type::Number, Type::String};
    DAAgentToolParam position{QStringLiteral("position"),
                              QStringLiteral("Position [x, y] in data coordinates (for text/point). ") + xCoordDesc
                                  + QStringLiteral(" ") + yCoordDesc,
                              {Type::Array}};
    position.itemTypes = coordItemTypes;
    spec.addParam(position);
    DAAgentToolParam start{QStringLiteral("start"),
                           QStringLiteral("Arrow start point [x, y] in data coordinates (for arrow). ") + xCoordDesc
                               + QStringLiteral(" ") + yCoordDesc,
                           {Type::Array}};
    start.itemTypes = coordItemTypes;
    spec.addParam(start);
    DAAgentToolParam end{QStringLiteral("end"),
                         QStringLiteral("Arrow end point [x, y] in data coordinates (for arrow). ") + xCoordDesc
                             + QStringLiteral(" ") + yCoordDesc,
                         {Type::Array}};
    end.itemTypes = coordItemTypes;
    spec.addParam(end);
    // start_x/end_x：主类型即联合类型 ["number", "string"]
    spec.addParam({QStringLiteral("start_x"),
                   QStringLiteral("Start x value of the region (for region type). ") + xCoordDesc,
                   {Type::Number, Type::String}});
    spec.addParam({QStringLiteral("end_x"),
                   QStringLiteral("End x value of the region (for region type). ") + xCoordDesc,
                   {Type::Number, Type::String}});
    spec.addParam({QStringLiteral("text"),
                   QStringLiteral("Annotation text (for text type, label at arrow tip, or region label)"),
                   {Type::String}});
    spec.addParam({QStringLiteral("color"), QStringLiteral("Annotation color (hex or name)"), {Type::String}});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
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

    // 当前图 X 轴是否为时间轴（QwtDateScaleDraw）。时间轴下 Qwt 把坐标 double
    // 解释为「自 1970-01-01 UTC 起的毫秒数」，与 DAPySeries::castTo 产出的曲线
    // 数据（datetime64 → 毫秒）单位一致。
    const bool xIsDateTime = chart->isDateTimeAxis(QwtPlot::xBottom);

    // 把传入的 x 坐标归一化到当前图 X 轴所期望的单位；解析失败返回 nullopt。
    // - 非时间轴：number 原样；string 宽松接受数字字符串。
    // - 时间轴：number 按量级判断秒(<1e11)→×1000、毫秒原样；string 走 ISO 解析
    //   后用 QwtDate::toDouble 转毫秒（与轴/曲线同单位）。
    auto normalizeX = [xIsDateTime](const QJsonValue& v) -> std::optional<double> {
        if (v.isDouble()) {
            double val = v.toDouble();
            if (xIsDateTime) {
                // 1e11 毫秒 = 1973-03-03。真实业务毫秒戳远大于此；
                // 秒级戳(1973-2033) = 1e8~2e9 < 1e11，故小于阈值视为秒并放大到毫秒。
                if (qAbs(val) < 1e11) {
                    val *= 1000.0;
                }
            }
            return val;
        }
        if (v.isString()) {
            const QString s = v.toString().trimmed();
            if (xIsDateTime) {
                // ISO 时间字符串 → 毫秒（与 QwtDateScaleDraw 对齐）
                QDateTime dt = QDateTime::fromString(s, Qt::ISODateWithMs);
                if (!dt.isValid()) dt = QDateTime::fromString(s, Qt::ISODate);
                if (!dt.isValid()) dt = QDateTime::fromString(s, QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
                if (!dt.isValid()) dt = QDateTime::fromString(s, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
                if (!dt.isValid()) dt = QDateTime::fromString(s, QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
                if (!dt.isValid()) dt = QDateTime::fromString(s, QStringLiteral("yyyy-MM-dd"));
                if (!dt.isValid()) {
                    return std::nullopt;
                }
                return QwtDate::toDouble(dt);
            }
            // 线性轴：宽容接受数字字符串
            bool ok = false;
            double val = s.toDouble(&ok);
            if (!ok) {
                return std::nullopt;
            }
            return val;
        }
        return std::nullopt;
    };

    // 把归一化后的 x 值格式化为可读字符串：时间轴显示日期，否则显示数值。
    auto formatX = [xIsDateTime](double val) -> QString {
        if (xIsDateTime) {
            QDateTime dt = QwtDate::toDateTime(val, Qt::LocalTime);
            if (dt.isValid()) {
                return dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
            }
        }
        return QString::number(val);
    };

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
        auto xOpt = normalizeX(posArr[ 0 ]);
        if (!xOpt) {
            return errorResponse("position[0] must be a number or ISO datetime string");
        }
        double x = *xOpt;
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
        return successResponse(QString("Added text annotation at (x=%1, y=%2)%3").arg(formatX(x)).arg(y).arg(warning));
    }
    else if (type == "point") {
        // Point annotation: QwtPlotMarker with a symbol
        QJsonArray posArr = params["position"].toArray();
        if (posArr.size() < 2) {
            return errorResponse("position must be [x, y] array for point annotation");
        }
        auto xOpt = normalizeX(posArr[ 0 ]);
        if (!xOpt) {
            return errorResponse("position[0] must be a number or ISO datetime string");
        }
        double x = *xOpt;
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
        return successResponse(QString("Added point annotation at (x=%1, y=%2)%3").arg(formatX(x)).arg(y).arg(warning));
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
        auto startXOpt = normalizeX(startArr[ 0 ]);
        auto endXOpt   = normalizeX(endArr[ 0 ]);
        if (!startXOpt || !endXOpt) {
            return errorResponse("start[0]/end[0] must be a number or ISO datetime string");
        }
        QPointF start(*startXOpt, startArr[ 1 ].toDouble());
        QPointF end(*endXOpt, endArr[ 1 ].toDouble());

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
        return successResponse(QString("Added arrow annotation from (x=%1, y=%2) to (x=%3, y=%4)%5")
            .arg(formatX(start.x())).arg(start.y()).arg(formatX(end.x())).arg(end.y()).arg(warning));
    }
    else if (type == "region") {
        // Region annotation: highlighted vertical band between start_x and end_x.
        // Ported from the former add_region tool; uses QwtPlotShapeItem with
        // a semi-transparent fill and dashed border.
        if (!params.contains("start_x") || !params.contains("end_x")) {
            return errorResponse("region requires start_x and end_x parameters");
        }
        auto startXOpt = normalizeX(params["start_x"]);
        auto endXOpt   = normalizeX(params["end_x"]);
        if (!startXOpt) {
            return errorResponse("start_x must be a number or ISO datetime string");
        }
        if (!endXOpt) {
            return errorResponse("end_x must be a number or ISO datetime string");
        }
        double startX = *startXOpt;
        double endX   = *endXOpt;
        if (qFuzzyCompare(startX, endX)) {
            return errorResponse("start_x and end_x must be different");
        }
        if (startX > endX) {
            std::swap(startX, endX);
        }

        // Re-enable auto-scale and replot so the y-axis reflects the real data
        // range. createChart locks axes to [0,800]x[0,500] via setAxisScale, which
        // disables Qwt auto-scaling; without this, axisScaleDiv(yLeft) below
        // returns the stale locked range and the region spans an empty y area.
        enableAutoScale(chart);
        chart->replot();

        // Get y-axis range to cover the full vertical extent (now the data range)
        const QwtScaleDiv& yDiv = chart->axisScaleDiv(QwtPlot::yLeft);
        double yMin = yDiv.lowerBound();
        double yMax = yDiv.upperBound();
        if (qFuzzyCompare(yMin, yMax)) {
            yMin -= 1.0;
            yMax += 1.0;
        }

        // Build rectangle path
        QPainterPath path;
        path.addRect(QRectF(startX, yMin, endX - startX, yMax - yMin));

        QwtPlotShapeItem* item = chart->addShapeItem(path, text);
        if (item) {
            QColor fill = colorStr.isEmpty() ? QColor(255, 200, 0) : color;
            fill.setAlpha(100);  // semi-transparent
            item->setBrush(QBrush(fill));
            QPen pen(fill.darker(150));
            pen.setStyle(Qt::DashLine);
            pen.setWidthF(1.5);
            item->setPen(pen);
        }

        chart->replot();

        // Off-screen x-range hint: warn if the region falls entirely outside the
        // visible axis range (so the caller knows it was added but not visible).
        QString warning;
        const QwtScaleDiv& xDiv = chart->axisScaleDiv(QwtPlot::xBottom);
        if (endX < xDiv.lowerBound() || startX > xDiv.upperBound()) {
            warning = " (warning: region x-range is outside the visible axis range)";
        }
        return successResponse(QString("Added region [%1 ~ %2]%3").arg(formatX(startX)).arg(formatX(endX)).arg(warning));
    }
    else {
        return errorResponse(QString("Unsupported annotation type: %1").arg(type));
    }
}
}  // namespace DA
