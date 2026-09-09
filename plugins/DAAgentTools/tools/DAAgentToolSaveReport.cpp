#include "DAAgentToolSaveReport.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DAMarkdownExporter.h"

namespace DA
{
// 路径安全策略已移交权限门（permission-layer P1）：工作区内放行、区外询问、
// 系统目录硬 deny，统一由 DAAgentPermissionManager::decide() 执法。

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolSaveReport::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("save_report"),
                         QStringLiteral("Save a markdown report as md, html, pdf, or docx file, then open it in the viewer.")};
    spec.addParam({QStringLiteral("content"), QStringLiteral("Report content in markdown"), {Type::String}, true});
    spec.addParam({QStringLiteral("file_path"), QStringLiteral("Output file path"), {Type::String}, true});
    spec.addParam({QStringLiteral("format"),
                   QStringLiteral("Output format: md, html, pdf, docx (default md)"),
                   {Type::String}});
    spec.addParam({QStringLiteral("open_after_save"),
                   QStringLiteral("Whether to open the report in the viewer after saving (default true)"),
                   {Type::Boolean}});
    return spec;
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

    // 保存成功后打开报告：md 用应用内置 Markdown 查看器，其余用系统默认程序
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
    else if (format == "html" || format == "pdf" || format == "docx") {
        // html/pdf/docx 经 DAMarkdownExporter：与内置 Markdown 查看器同一渲染
        // 管线（markdown-it + highlight.js + KaTeX），公式/高亮/图片正常渲染；
        // 旧 QTextDocument + QPrinter 管线无法渲染公式，已废弃
        DAMarkdownExporter::Format fmt;
        QString fmtLabel;
        if (format == "html") {
            fmt      = DAMarkdownExporter::Format::Html;
            fmtLabel = "HTML";
        } else if (format == "pdf") {
            fmt      = DAMarkdownExporter::Format::Pdf;
            fmtLabel = "PDF";
        } else {
            fmt      = DAMarkdownExporter::Format::Docx;
            fmtLabel = "DOCX";
        }
        DAMarkdownExporter exporter;
        QString err;
        if (!exporter.exportToFile(content, absPath, fmt, fi.absolutePath(), &err)) {
            return errorResponse(QString("Failed to export %1 report: %2").arg(format, err));
        }
        return successResponse(QString("Report saved as %1 to %2%3").arg(fmtLabel, absPath, openReport(absPath, format, openAfterSave)));
    }
    else {
        return errorResponse(QString("Unsupported format: %1. Use md, html, pdf, or docx.").arg(format));
    }
}
}  // namespace DA
