#include "DAAgentToolSaveReport.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTextDocument>
#include <QPrinter>
#include <QDateTime>
#include <QDesktopServices>
#include <QRegularExpression>
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
 * @brief 将 markdown 中的相对图片路径解析为基于输出目录的绝对路径
 *
 * pdf 分支：QTextDocument 默认 loadResource 只能加载绝对本地路径
 * （相对路径基于未设置的 baseUrl 解析必失败）；
 * docx 分支：toHtml() 的 img src 原样携带 markdown 路径，而临时 HTML
 * 位于系统 temp 目录，相对路径相对 temp 解析必然失效。
 * http(s)/data:/file: URL 与已是绝对路径的引用保持原样。
 */
static QString normalizeImagePaths(const QString& content, const QString& baseDir)
{
    static const QRegularExpression imgRegex(
        QStringLiteral("!\\[[^\\]]*\\]\\(([^\\s)]+)(?:\\s+\"[^\"]*\")?\\)"));
    // 自左向右收集匹配，自右向左按记录偏移替换（右侧替换不影响左侧偏移）
    struct ImgReplacement
    {
        int start;      ///< 捕获组（路径）在原文中的起始偏移
        int length;     ///< 捕获组（路径）长度
        QString absPath;
    };
    QList< ImgReplacement > replacements;
    QRegularExpressionMatchIterator it = imgRegex.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString path = m.captured(1);
        if (path.startsWith(QStringLiteral("http:"), Qt::CaseInsensitive)
            || path.startsWith(QStringLiteral("https:"), Qt::CaseInsensitive)
            || path.startsWith(QStringLiteral("data:"), Qt::CaseInsensitive)
            || path.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)
            || QFileInfo(path).isAbsolute()) {
            continue;
        }
        ImgReplacement r;
        r.start   = static_cast< int >(m.capturedStart(1));
        r.length  = static_cast< int >(m.capturedLength(1));
        r.absPath = QFileInfo(QDir(baseDir), path).absoluteFilePath();
        replacements.append(r);
    }
    QString result = content;
    for (int i = replacements.size() - 1; i >= 0; --i) {
        const ImgReplacement& r = replacements.at(i);
        result.replace(r.start, r.length, r.absPath);
    }
    return result;
}

#ifdef Q_OS_WIN
/**
 * @brief 将 HTML 中 img src 的绝对本地路径转换为 file:/// URL
 *
 * toHtml() 对绝对路径图片输出 src="C:/..."，Word 打开临时 HTML 时对
 * 无 scheme 的路径解析不稳定；file:/// URL 是 Word HTML 导入的确定形态。
 */
static QString imgSrcToFileUrl(const QString& html)
{
    static const QRegularExpression srcRegex(QStringLiteral("src=\"([A-Za-z]:[/\\\\][^\"]*)\""));
    struct SrcReplacement
    {
        int start;
        int length;
        QString url;
    };
    QList< SrcReplacement > replacements;
    QRegularExpressionMatchIterator it = srcRegex.globalMatch(html);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        SrcReplacement r;
        r.start  = static_cast< int >(m.capturedStart(1));
        r.length = static_cast< int >(m.capturedLength(1));
        r.url    = QUrl::fromLocalFile(m.captured(1)).toString();
        replacements.append(r);
    }
    QString result = html;
    for (int i = replacements.size() - 1; i >= 0; --i) {
        const SrcReplacement& r = replacements.at(i);
        result.replace(r.start, r.length, r.url);
    }
    return result;
}
#endif

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolSaveReport::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("save_report"),
                         QStringLiteral("Save a markdown report as md, pdf, or docx file, then open it in the viewer.")};
    spec.addParam({QStringLiteral("content"), QStringLiteral("Report content in markdown"), {Type::String}, true});
    spec.addParam({QStringLiteral("file_path"), QStringLiteral("Output file path"), {Type::String}, true});
    spec.addParam({QStringLiteral("format"),
                   QStringLiteral("Output format: md, pdf, docx (default md)"),
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
        // 相对图片路径先解析为基于输出目录的绝对路径（QTextDocument 默认
        // loadResource 仅能加载绝对本地路径）
        QTextDocument doc;
        doc.setMarkdown(normalizeImagePaths(content, fi.absolutePath()));
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
        // 相对图片路径先解析为绝对路径，导出的 img src 再转 file:/// URL：
        // 临时 HTML 位于系统 temp 目录，相对/无 scheme 路径在 Word 中无法解析
        QTextDocument doc;
        doc.setMarkdown(normalizeImagePaths(content, fi.absolutePath()));
        QString html = imgSrcToFileUrl(doc.toHtml());

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
