#pragma once
#include <QObject>
#include <QString>
#include "DAGuiAPI.h"

namespace DA
{
/**
 * @brief Markdown 统一导出引擎：HTML（单一离线文件）/ PDF / Word(docx)
 *
 * 与 DAMarkdownView 共用同一渲染管线（markdown-it + highlight.js + KaTeX，
 * qrc:///DAMarkdown/markdown.html 壳），保证导出结果与查看器所见一致。
 * 旧管线（QTextDocument::setMarkdown + QPrinter）无法渲染数学公式与代码
 * 高亮，导出的 PDF 呈现为未渲染源文本，本引擎即为其替代。
 *
 * 本地图片在渲染前内嵌为 data URI（HTML/PDF），Word 路径转换为 file:///
 * URL（Word HTML 导入不支持 data URI）；exportToFile 为同步接口，内部经
 * 局部事件循环等待 WebEngine 异步回调，必须在 GUI 线程调用。
 */
class DAGUI_API DAMarkdownExporter : public QObject
{
    Q_OBJECT
public:
    // 导出格式
    enum class Format
    {
        Html,  // 单一离线 HTML（CSS/字体/图片全部内嵌）
        Pdf,   // 渲染后页面经 QWebEnginePage::printToPdf 输出
        Docx,  // Word HTML 导入 + COM 另存（仅 Windows）
    };

    explicit DAMarkdownExporter(QObject* parent = nullptr);
    ~DAMarkdownExporter();

    // 同步导出 markdown 到指定文件；baseDir 为相对图片路径的解析基准目录
    bool exportToFile(const QString& markdown, const QString& outputPath, Format fmt, const QString& baseDir, QString* errMsg = nullptr);

    // 将 markdown 中相对图片路径解析为基于 baseDir 的绝对路径（http/data/file 与绝对路径原样）
    static QString normalizeImagePaths(const QString& markdown, const QString& baseDir);
    // 将 markdown 中可读取的本地图片内嵌为 data URI（读取失败保留原路径；svg/bmp 经 QImage 转 PNG）
    static QString embedLocalImagesAsDataUri(const QString& markdown);
    // 读取本地图片文件并转为 data:image/...;base64,... 形式，失败返回空串
    static QString localFileToDataUri(const QString& filePath);
    // 将 katex css 中 url(fonts/*.woff2) 引用替换为 base64 内嵌（导出单一 HTML 用）
    static QString inlineKatexFonts(const QString& katexCss);
    // 将 HTML 中图片 src 转换为 Word 兼容形态：data URI → 临时文件 file:/// URL；本地绝对路径 → file:/// URL
    static QString htmlForWord(const QString& html, const QString& tempDirPath);

private:
    // 离屏渲染：markdown 经 qrc 壳内 markdown-it 渲染为 HTML 片段（mathmlOutput 切换 KaTeX 公式输出形态）
    static QString renderBodyHtml(const QString& markdown, const QString& baseDir, bool mathmlOutput, QString* errMsg);
    // 组装单一离线 HTML（内联 markdown.css + katex css/字体 + 打印样式）
    static QString buildStandaloneHtml(const QString& bodyHtml, const QString& title);
    // 组装 Word 友好的简化 HTML（极简 CSS，无外部依赖）
    static QString buildWordHtml(const QString& bodyHtml, const QString& title);
    // 经临时 HTML 文件 + QWebEnginePage::printToPdf 输出 A4 PDF
    static bool printPdf(const QString& standaloneHtml, const QString& pdfPath, QString* errMsg);
    // 经 Word COM 将 HTML 另存为 .docx（仅 Windows）
    static bool convertDocx(const QString& wordHtml, const QString& docxPath, QString* errMsg);
    // 提取首个一级标题作为文档标题，无则回退 fallback
    static QString extractTitle(const QString& markdown, const QString& fallback);
};
}  // namespace DA
