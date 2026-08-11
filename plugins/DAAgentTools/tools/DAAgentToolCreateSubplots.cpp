#include "DAAgentToolCreateSubplots.h"
#include <QRectF>
#include <QStringList>

namespace DA
{
QJsonObject DAAgentToolCreateSubplots::getToolSpec() const
{
    return QJsonObject{
        {"name", "create_subplots"},
        {"description", "Create a new figure with a grid of subplots. Each call creates a NEW figure. "
         "The returned figure_id/figure_name can be used in da-figure: hyperlinks so the user can open this figure from your reply."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"layout", QJsonObject{{"type", "string"}, {"description", "Grid layout, e.g. '2x2' for 2 rows 2 columns"}}},
                {"figure_name", QJsonObject{{"type", "string"}, {"description", "Figure name shown as tab title. If empty, auto-generates."}}}
            }},
            {"required", QJsonArray{"layout"}}
        }}
    };
}

QJsonObject DAAgentToolCreateSubplots::execute(const QJsonObject& params)
{
    QString layout = params["layout"].toString();
    QString figureName = params["figure_name"].toString();

    if (layout.isEmpty()) {
        return errorResponse("layout is required");
    }

    // Parse "RxC" (case-insensitive 'x')
    QString sep = layout.contains('x') ? "x" : "X";
    QStringList parts = layout.split(sep, Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        return errorResponse("Invalid layout format. Use 'RxC' e.g. '2x2'");
    }
    bool ok1 = false, ok2 = false;
    int rowCnt = parts[ 0 ].toInt(&ok1);
    int colCnt = parts[ 1 ].toInt(&ok2);
    if (!ok1 || !ok2 || rowCnt <= 0 || colCnt <= 0) {
        return errorResponse("Invalid layout dimensions. Use positive integers like '2x3'");
    }

    // Determine figure name
    if (figureName.isEmpty()) {
        figureName = QString("Subplots %1x%2").arg(rowCnt).arg(colCnt);
    }

    // Create a new figure and set it as current
    DAFigureWidget* fig = createFigure(figureName);
    if (!fig) {
        return errorResponse("Failed to create figure");
    }

    // Use createChart(QRectF) to add each chart exactly once with the correct geometry.
    // createChart internally calls addChart with the given QRectF, so calling
    // addWidget separately would add the same chart twice.
    float cellW = 1.0f / colCnt;
    float cellH = 1.0f / rowCnt;
    int created = 0;

    for (int r = 0; r < rowCnt; ++r) {
        for (int c = 0; c < colCnt; ++c) {
            float left = cellW * c;
            float top  = cellH * r;
            QRectF cellRect(left, top, cellW, cellH);
            DAChartWidget* chart = fig->createChart(cellRect);
            if (chart) {
                fig->setCurrentChart(chart);
                ++created;
            }
        }
    }

    // Set the first chart as current
    const QList< DAChartWidget* > charts = fig->getCharts();
    if (!charts.isEmpty()) {
        fig->setCurrentChart(charts.first());
    }

    // Return structured data
    QJsonObject respData;
    respData["figure_id"] = fig->getFigureId();   // 用于 da-figure:id=<figure_id> 精确引用
    respData["figure_name"] = figureName;
    respData["subplot_count"] = created;
    respData["layout"] = QString("%1x%2").arg(rowCnt).arg(colCnt);
    return successResponse(respData);
}
}  // namespace DA
