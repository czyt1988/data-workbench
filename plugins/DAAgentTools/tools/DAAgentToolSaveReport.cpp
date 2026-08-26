#include "DAAgentToolSaveReport.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTextDocument>
#include <QPrinter>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#ifdef Q_OS_WIN
#include "DAAxObjectWordWrapper.h"
#endif

namespace DA
{
// 路径安全策略已移交权限门（permission-layer P1）：工作区内放行、区外询问、
// 系统目录硬 deny，统一由 DAAgentPermissionManager::decide() 执法。

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
QJsonObject DAAgentToolSaveReport::getToolSpec() const
{
    return QJsonObject{
        {"name", "save_report"},
        {"description", "Save a markdown report as md, pdf, or docx file, then open it in the viewer."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"content", QJsonObject{{"type", "string"}, {"description", "Report content in markdown"}}},
                {"file_path", QJsonObject{{"type", "string"}, {"description", "Output file path"}}},
                {"format", QJsonObject{{"type", "string"}, {"description", "Output format: md, pdf, docx (default md)"}}},
                {"open_after_save", QJsonObject{{"type", "boolean"}, {"description", "Whether to open the report in the viewer after saving (default true)"}}}
            }},
            {"required", QJsonArray{"content", "file_path"}}
        }}
    };
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolSaveReport::execute(const QJsonObject& params)
{
    QString content  = params["content"].toString();
    QString filePath = params["file_path"].toString();
    QString format   = params.contains("format") ? params["format"].toString().toLower() : "md";
    bool openAfterSave = params.contains("open_after_save") ? params["open_after_save"].toBool(true) : true;

    if (content.isEmpty()) {
        return errorResponse("content is required");
    }
    if (filePath.isEmpty()) {
        return errorResponse("file_path is required");
    }
    // 系统目录硬 deny 已由权限门统一拦截（permission-layer P1），此处不再重复检查

    // Ensure parent directory exists
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());
    QString absPath = fi.absoluteFilePath();

    // 保存成功后打开报告：md 用应用内置 Markdown 查看器，pdf/docx 用系统默认程序
    // 返回附加消息（追加到成功提示末尾），不抛异常；打开失败不影响保存成功状态
    auto openReport = [this](const QString& path, const QString& fmt, bool open) -> QString {
        if (!open) {
            return QString();
        }
        if (fmt == "md") {
            auto* ui   = mCore ? mCore->getUiInterface() : nullptr;
            auto* dock = ui ? ui->getDockingArea() : nullptr;
            if (dock && dock->showMarkdownFile(path)) {
                return " and opened in viewer";
            }
            return " (failed to open in viewer)";
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        return " and opened";
    };

    if (format == "md") {
        // Direct write
        QFile f(filePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            return errorResponse(QString("Cannot open file: %1").arg(f.errorString()));
        }
        f.write(content.toUtf8());
        f.close();
        return successResponse(QString("Report saved as markdown to %1%2").arg(absPath, openReport(absPath, format, openAfterSave)));
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
        return successResponse(QString("Report saved as PDF to %1%2").arg(absPath, openReport(absPath, format, openAfterSave)));
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
        return successResponse(QString("Report saved as DOCX to %1%2").arg(absPath, openReport(absPath, format, openAfterSave)));
#else
        return errorResponse("DOCX export is only available on Windows with Microsoft Word installed. Use 'md' or 'pdf' format instead.");
#endif
    }
    else {
        return errorResponse(QString("Unsupported format: %1. Use md, pdf, or docx.").arg(format));
    }
}
}  // namespace DA
