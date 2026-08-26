#include "DAAgentToolSaveChartImage.h"
#include <QPixmap>
#include <QPrinter>
#include <QSvgGenerator>
#include <QPageSize>

namespace DA
{

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolSaveChartImage::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("save_chart_image"),
                         QStringLiteral("Save a chart as an image file (PNG, PDF, or SVG). Use figure_name to target "
                                        "a specific figure.")};
    spec.addParam({QStringLiteral("chart_id"),
                   QStringLiteral("Chart identifier. Empty or 'current' for active chart."),
                   {Type::String}});
    spec.addParam({QStringLiteral("figure_name"),
                   QStringLiteral("Figure name to target a specific figure. Empty for current active figure."),
                   {Type::String}});
    spec.addParam({QStringLiteral("file_path"), QStringLiteral("Output file path"), {Type::String}, true});
    spec.addParam({QStringLiteral("format"),
                   QStringLiteral("Image format: png, pdf, svg (default png)"),
                   {Type::String}});
    spec.addParam({QStringLiteral("width"),
                   QStringLiteral("Output width in pixels (PNG) or points (PDF/SVG)"),
                   {Type::Integer}});
    spec.addParam({QStringLiteral("height"),
                   QStringLiteral("Output height in pixels (PNG) or points (PDF/SVG)"),
                   {Type::Integer}});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolSaveChartImage::execute(const QJsonObject& params)
{
    QString chartId    = params["chart_id"].toString();
    QString figureName = params["figure_name"].toString();
    QString filePath    = params["file_path"].toString();
    QString format      = params.contains("format") ? params["format"].toString().toLower() : "png";
    int width  = params.contains("width") ? params["width"].toInt() : 0;
    int height = params.contains("height") ? params["height"].toInt() : 0;

    if (filePath.isEmpty()) {
        return errorResponse("file_path is required");
    }

    DAChartWidget* chart = findChart(chartId, figureName);
    if (!chart) {
        QString ref = figureName.isEmpty() ? (chartId.isEmpty() ? "current" : chartId) : (figureName + "/" + (chartId.isEmpty() ? "current" : chartId));
        return errorResponse(QString("Chart '%1' not found").arg(ref));
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
