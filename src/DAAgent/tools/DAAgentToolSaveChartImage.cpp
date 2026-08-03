#include "DAAgentToolSaveChartImage.h"
#include <QPixmap>
#include <QPrinter>
#include <QSvgGenerator>
#include <QPageSize>

namespace DA
{
QJsonObject DAAgentToolSaveChartImage::getToolSpec() const
{
    return QJsonObject{
        {"name", "save_chart_image"},
        {"description", "Save a chart as an image file (PNG, PDF, or SVG)."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"chart_id", QJsonObject{{"type", "string"}, {"description", "Chart identifier. Empty or 'current' for active chart."}}},
                {"file_path", QJsonObject{{"type", "string"}, {"description", "Output file path"}}},
                {"format", QJsonObject{{"type", "string"}, {"description", "Image format: png, pdf, svg (default png)"}}},
                {"width", QJsonObject{{"type", "integer"}, {"description", "Output width in pixels (PNG) or points (PDF/SVG)"}}},
                {"height", QJsonObject{{"type", "integer"}, {"description", "Output height in pixels (PNG) or points (PDF/SVG)"}}}
            }},
            {"required", QJsonArray{"file_path"}}
        }}
    };
}

QJsonObject DAAgentToolSaveChartImage::execute(const QJsonObject& params)
{
    QString chartId  = params["chart_id"].toString();
    QString filePath = params["file_path"].toString();
    QString format   = params.contains("format") ? params["format"].toString().toLower() : "png";
    int width  = params.contains("width") ? params["width"].toInt() : 0;
    int height = params.contains("height") ? params["height"].toInt() : 0;

    if (filePath.isEmpty()) {
        return errorResponse("file_path is required");
    }

    DAChartWidget* chart = findChart(chartId);
    if (!chart) {
        return errorResponse(QString("Chart '%1' not found").arg(chartId.isEmpty() ? "current" : chartId));
    }

    bool ok = false;

    if (format == "pdf") {
        // PDF export via QPrinter — QWidget::render(QPaintDevice*) renders the chart
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filePath);
        if (width > 0 && height > 0) {
            printer.setPageSize(QPageSize(QSize(width, height), QPageSize::Point));
        }
        chart->render(&printer);
        ok = true;
    }
    else if (format == "svg") {
        // SVG export via QSvgGenerator
        QSvgGenerator generator;
        generator.setFileName(filePath);
        if (width > 0 && height > 0) {
            generator.setSize(QSize(width, height));
        } else {
            generator.setSize(chart->size());
        }
        chart->render(&generator);
        ok = true;
    }
    else {
        // PNG (and other raster formats) via QPixmap
        QPixmap pixmap = chart->grab();
        if (width > 0 && height > 0) {
            pixmap = pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        ok = pixmap.save(filePath, format.toUpper().toLatin1().constData());
    }

    if (!ok) {
        return errorResponse(QString("Failed to save chart image to %1").arg(filePath));
    }

    return successResponse(QString("Chart saved to %1").arg(filePath));
}
}  // namespace DA
