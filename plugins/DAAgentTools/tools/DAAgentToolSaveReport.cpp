#include "DAAgentToolSaveReport.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTextDocument>
#include <QPrinter>
#include <QDateTime>
#ifdef Q_OS_WIN
#include "DAAxObjectWordWrapper.h"
#endif

namespace DA
{
// Path safety check (same logic as DAAgentToolReadFile/WriteFile)
static bool isPathSafeReport(const QString& path)
{
    QString normalized = QDir::cleanPath(path).toLower();
    static const QStringList blocked = {
        "c:/windows",
        "c:/windows/system32",
        "c:/program files",
        "c:/program files (x86)",
        "c:/programdata",
    };
    for (const QString& b : blocked) {
        if (normalized.startsWith(b)) {
            return false;
        }
    }
    return true;
}

QJsonObject DAAgentToolSaveReport::getToolSpec() const
{
    return QJsonObject{
        {"name", "save_report"},
        {"description", "Save a markdown report as md, pdf, or docx file."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"content", QJsonObject{{"type", "string"}, {"description", "Report content in markdown"}}},
                {"file_path", QJsonObject{{"type", "string"}, {"description", "Output file path"}}},
                {"format", QJsonObject{{"type", "string"}, {"description", "Output format: md, pdf, docx (default md)"}}}
            }},
            {"required", QJsonArray{"content", "file_path"}}
        }}
    };
}

QJsonObject DAAgentToolSaveReport::execute(const QJsonObject& params)
{
    QString content  = params["content"].toString();
    QString filePath = params["file_path"].toString();
    QString format   = params.contains("format") ? params["format"].toString().toLower() : "md";

    if (content.isEmpty()) {
        return errorResponse("content is required");
    }
    if (filePath.isEmpty()) {
        return errorResponse("file_path is required");
    }
    if (!isPathSafeReport(filePath)) {
        return errorResponse("Access denied: path is in a system-protected directory");
    }

    // Ensure parent directory exists
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());

    if (format == "md") {
        // Direct write
        QFile f(filePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            return errorResponse(QString("Cannot open file: %1").arg(f.errorString()));
        }
        f.write(content.toUtf8());
        f.close();
        return successResponse(QString("Report saved as markdown to %1").arg(filePath));
    }
    else if (format == "pdf") {
        // Render markdown to PDF via QTextDocument + QPrinter
        QTextDocument doc;
        doc.setMarkdown(content);
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filePath);
        QRectF pageRect = printer.pageRect(QPrinter::DevicePixel);
        doc.setPageSize(pageRect.size());
        doc.print(&printer);
        return successResponse(QString("Report saved as PDF to %1").arg(filePath));
    }
    else if (format == "docx") {
#ifdef Q_OS_WIN
        // Convert markdown to HTML, open with Word COM, save as .docx
        QTextDocument doc;
        doc.setMarkdown(content);
        QString html = doc.toHtml();

        // Write HTML to a temporary file
        QString tempFile = QDir::tempPath() + "/agent_report_"
                         + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".html";
        {
            QFile f(tempFile);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                return errorResponse("Failed to create temporary HTML file");
            }
            f.write(html.toUtf8());
            f.close();
        }

        // Use Word COM to open the HTML and save as .docx
        DAAxObjectWordWrapper word;
        if (!word.open(tempFile, false)) {
            QFile::remove(tempFile);
            return errorResponse("Failed to open Word. Make sure Microsoft Word is installed.");
        }
        bool ok = word.saveAs(filePath);
        word.quit();

        // Clean up temp file
        QFile::remove(tempFile);

        if (!ok) {
            return errorResponse("Failed to save .docx file via Word COM");
        }
        return successResponse(QString("Report saved as DOCX to %1").arg(filePath));
#else
        return errorResponse("DOCX export is only available on Windows with Microsoft Word installed. Use 'md' or 'pdf' format instead.");
#endif
    }
    else {
        return errorResponse(QString("Unsupported format: %1. Use md, pdf, or docx.").arg(format));
    }
}
}  // namespace DA
