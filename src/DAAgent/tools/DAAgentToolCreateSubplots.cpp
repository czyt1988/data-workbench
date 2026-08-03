#include "DAAgentToolCreateSubplots.h"
#include <QRectF>
#include <QStringList>

namespace DA
{
QJsonObject DAAgentToolCreateSubplots::getToolSpec() const
{
    return QJsonObject{
        {"name", "create_subplots"},
        {"description", "Create a grid of subplots in the current figure."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"layout", QJsonObject{{"type", "string"}, {"description", "Grid layout, e.g. '2x2' for 2 rows 2 columns"}}},
                {"data_name", QJsonObject{{"type", "string"}, {"description", "Optional dataset name for automatic plotting"}}}
            }},
            {"required", QJsonArray{"layout"}}
        }}
    };
}

QJsonObject DAAgentToolCreateSubplots::execute(const QJsonObject& params)
{
    QString layout = params["layout"].toString();

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

    DAFigureWidget* fig = currentFigure();
    if (!fig) {
        return errorResponse("No active figure. Please create or open a figure first.");
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
    QList< DAChartWidget* > charts = fig->getCharts();
    if (!charts.isEmpty()) {
        fig->setCurrentChart(charts.first());
    }

    return successResponse(QString("Created %1 subplots in %2x%3 grid").arg(created).arg(rowCnt).arg(colCnt));
}
}  // namespace DA
