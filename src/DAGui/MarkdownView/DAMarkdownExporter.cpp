#include "DAMarkdownExporter.h"
#include "DAMarkdownJsUtils.h"
#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#ifdef Q_OS_WIN
#include "DAAxObjectWordWrapper.h"
#endif

namespace DA
{
namespace
{
// 分片与超时参数：单次 runJavaScript 载荷过大（数 MB base64）会超 Chromium
// IPC 消息上限静默失败，DAAgentWebChannel 以 2MB/片 为先例，此处取保守值
constexpr int kInputChunkUnits  = 512 * 1024;    // 下发分片（UTF-16 单元，转义后约 ×1.05）
constexpr int kOutputChunkUnits = 1024 * 1024;   // 取回分片（UTF-16 单元）
constexpr int kPageLoadTimeoutMs  = 15000;       // HTML 壳加载超时
constexpr int kRenderTimeoutMs    = 60000;       // markdown-it/KaTeX 渲染超时（大量图片时较慢）
constexpr int kChunkTimeoutMs     = 30000;       // 单片取回超时
constexpr int kPrintTimeoutMs     = 120000;      // PDF 打印超时
constexpr int kFontsReadyPollMs   = 200;         // 字体就绪轮询间隔
constexpr int kFontsReadyMaxPolls = 15;          // 字体就绪轮询次数上限（~3s）

/**
 * @brief 发起 runJavaScript 并在局部事件循环中等待回调，超时返回 false
 * @param page 目标页面
 * @param script JS 脚本
 * @param timeoutMs 超时毫秒数
 * @param result 可选，接收回调返回值
 * @return 收到回调返回 true，超时返回 false
 */
bool runJsAndWait(QWebEnginePage* page, const QString& script, int timeoutMs, QVariant* result = nullptr)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool ok = false;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    page->runJavaScript(script, [&loop, &ok, result](const QVariant& v) {
        ok = true;
        if (result) {
            *result = v;
        }
        loop.quit();
    });
    timer.start(timeoutMs);
    loop.exec();
    return ok;
}

/**
 * @brief 加载 url 并在局部事件循环中等待 loadFinished，超时或加载失败返回 false
 */
bool waitPageLoaded(QWebEnginePage* page, const QUrl& url, int timeoutMs)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool ok = false;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(page, &QWebEnginePage::loadFinished, &loop, [&loop, &ok](bool success) {
        ok = success;
        loop.quit();
    });
    timer.start(timeoutMs);
    page->setUrl(url);
    loop.exec();
    return ok;
}

/**
 * @brief 在事件循环中等待 ms 毫秒（不阻塞事件分发）
 */
void waitMs(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

/**
 * @brief 读取 qrc 资源文件的 UTF-8 文本，失败返回空串
 */
QString readResourceText(const QString& resourcePath)
{
    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

/**
 * @brief HTML 文本转义（< > & "）
 */
QString htmlEscape(const QString& s)
{
    QString result;
    result.reserve(s.size());
    for (const QChar& ch : s) {
        switch (ch.unicode()) {
        case '<':
            result += QStringLiteral("&lt;");
            break;
        case '>':
            result += QStringLiteral("&gt;");
            break;
        case '&':
            result += QStringLiteral("&amp;");
            break;
        case '"':
            result += QStringLiteral("&quot;");
            break;
        default:
            result += ch;
        }
    }
    return result;
}
}  // namespace

DAMarkdownExporter::DAMarkdownExporter(QObject* parent) : QObject(parent)
{
}

DAMarkdownExporter::~DAMarkdownExporter()
{
}

/**
 * @brief 将 markdown 中相对图片路径解析为基于输出目录的绝对路径
 *
 * 渲染管线（markdown-it data URI 内嵌）只能加载绝对本地路径与 file: URL
 * （qrc 壳的 baseUrl 未指向源文件目录，相对路径解析必失败）；
 * http(s)/data:/file: URL 与已是绝对路径的引用保持原样。
 */
QString DAMarkdownExporter::normalizeImagePaths(const QString& markdown, const QString& baseDir)
{
    static const QRegularExpression imgRegex(
        QStringLiteral("!\\[[^\\]]*\\]\\(([^\\s)]+)(?:\\s+\"[^\"]*\")?\\)"));
    // 自左向右收集匹配，自右向左按记录偏移替换（右侧替换不影响左侧偏移）
    struct ImgReplacement
    {
        int start;  ///< 捕获组（路径）在原文中的起始偏移
        int length; ///< 捕获组（路径）长度
        QString absPath;
    };
    QList< ImgReplacement > replacements;
    QRegularExpressionMatchIterator it = imgRegex.globalMatch(markdown);
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
    QString result = markdown;
    for (int i = replacements.size() - 1; i >= 0; --i) {
        const ImgReplacement& r = replacements.at(i);
        result.replace(r.start, r.length, r.absPath);
    }
    return result;
}

/**
 * @brief 读取本地图片并转为 data URI
 *
 * png/jpg/jpeg/gif/webp 直接读原始字节（markdown-it 的链接校验仅放行
 * data:image/(gif|png|jpeg|webp) 前缀）；svg/bmp 等白名单外格式经
 * QImage 统一转码为 PNG，否则渲染时图片标记会被 markdown-it 丢弃。
 * 文件不存在或读取失败返回空串，调用方保留原始路径。
 */
QString DAMarkdownExporter::localFileToDataUri(const QString& filePath)
{
    QFileInfo fi(filePath);
    if (!fi.isFile()) {
        return QString();
    }
    static const QHash< QString, QString > kMimeByExt = {
        { QStringLiteral("png"), QStringLiteral("image/png") },
        { QStringLiteral("jpg"), QStringLiteral("image/jpeg") },
        { QStringLiteral("jpeg"), QStringLiteral("image/jpeg") },
        { QStringLiteral("gif"), QStringLiteral("image/gif") },
        { QStringLiteral("webp"), QStringLiteral("image/webp") },
    };
    QString mime;
    QByteArray bytes;
    auto it = kMimeByExt.constFind(fi.suffix().toLower());
    if (it != kMimeByExt.cend()) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) {
            return QString();
        }
        mime  = it.value();
        bytes = f.readAll();
    } else {
        // svg/bmp 等：QImage 加载（svg 经 Qt svg 插件）后统一转 PNG
        QImage img(fi.absoluteFilePath());
        if (img.isNull()) {
            return QString();
        }
        QBuffer buffer;
        if (!buffer.open(QIODevice::WriteOnly) || !img.save(&buffer, "PNG")) {
            return QString();
        }
        mime  = QStringLiteral("image/png");
        bytes = buffer.data();
    }
    if (bytes.isEmpty()) {
        return QString();
    }
    return QStringLiteral("data:%1;base64,%2").arg(mime, QString::fromLatin1(bytes.toBase64()));
}

/**
 * @brief 将 markdown 中可读取的本地图片内嵌为 data URI
 *
 * 在 markdown 源文本层面替换（渲染前），渲染后的 img src 即为 data URI，
 * 导出的 HTML/PDF 天然自包含；markdown-it 对 data:image URI 的链接校验
 * 仅放行 gif/png/jpeg/webp（svg/bmp 已在 localFileToDataUri 中转码 PNG）。
 * data:/http(s): 引用与读取失败的路径保持原样。
 */
QString DAMarkdownExporter::embedLocalImagesAsDataUri(const QString& markdown)
{
    static const QRegularExpression imgRegex(
        QStringLiteral("!\\[[^\\]]*\\]\\(([^\\s)]+)(?:\\s+\"[^\"]*\")?\\)"));
    struct ImgReplacement
    {
        int start;
        int length;
        QString dataUri;
    };
    QList< ImgReplacement > replacements;
    QRegularExpressionMatchIterator it = imgRegex.globalMatch(markdown);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString ref = m.captured(1);
        QString localPath;
        if (ref.startsWith(QStringLiteral("data:"), Qt::CaseInsensitive)
            || ref.startsWith(QStringLiteral("http:"), Qt::CaseInsensitive)
            || ref.startsWith(QStringLiteral("https:"), Qt::CaseInsensitive)) {
            continue;
        }
        if (ref.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
            QUrl url(ref);
            if (!url.isLocalFile()) {
                continue;
            }
            localPath = url.toLocalFile();
        } else {
            localPath = ref;  // normalizeImagePaths 已将相对路径转绝对
        }
        QString dataUri = localFileToDataUri(localPath);
        if (dataUri.isEmpty()) {
            continue;
        }
        ImgReplacement r;
        r.start   = static_cast< int >(m.capturedStart(1));
        r.length  = static_cast< int >(m.capturedLength(1));
        r.dataUri = dataUri;
        replacements.append(r);
    }
    QString result = markdown;
    for (int i = replacements.size() - 1; i >= 0; --i) {
        const ImgReplacement& r = replacements.at(i);
        result.replace(r.start, r.length, r.dataUri);
    }
    return result;
}

/**
 * @brief 将 katex css 中的字体引用内联为 base64 data URI
 *
 * 导出单一离线 HTML 不允许携带外部字体文件：url(fonts/X.woff2) 替换为
 * url(data:font/woff2;base64,...)（字体经 chat.qrc 的 fonts/ 别名从 qrc 读取），
 * 同时剔除 .ttf 回退源（离线文件不再需要多格式回退，可显著减小体积）。
 */
QString DAMarkdownExporter::inlineKatexFonts(const QString& katexCss)
{
    QString result = katexCss;
    // 剔除 ttf 回退源：",url(fonts/X.ttf) format("truetype")"
    static const QRegularExpression ttfRegex(
        QStringLiteral(",url\\(fonts/[A-Za-z0-9_\\-]+\\.ttf\\)\\s*format\\(\"truetype\"\\)"));
    result.replace(ttfRegex, QString());
    // woff2 内联
    static const QRegularExpression woff2Regex(QStringLiteral("url\\(fonts/([A-Za-z0-9_\\-]+\\.woff2)\\)"));
    struct FontReplacement
    {
        int start;
        int length;
        QString dataUri;
    };
    QList< FontReplacement > replacements;
    QRegularExpressionMatchIterator it = woff2Regex.globalMatch(result);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QFile f(QStringLiteral(":/DAAgent/fonts/") + m.captured(1));
        if (!f.open(QIODevice::ReadOnly)) {
            continue;
        }
        FontReplacement r;
        r.start   = static_cast< int >(m.capturedStart(0));
        r.length  = static_cast< int >(m.capturedLength(0));
        r.dataUri = QStringLiteral("url(data:font/woff2;base64,%1)").arg(
            QString::fromLatin1(f.readAll().toBase64()));
        replacements.append(r);
    }
    for (int i = replacements.size() - 1; i >= 0; --i) {
        const FontReplacement& r = replacements.at(i);
        result.replace(r.start, r.length, r.dataUri);
    }
    return result;
}

/**
 * @brief 将 HTML 中图片 src 转换为 Word 兼容形态
 *
 * Word 的 HTML 导入不支持 data URI 图片，也不稳定支持无 scheme 的本地路径：
 * - data:image/... → base64 解码写入 tempDirPath 下临时文件，src 替换为 file:/// URL
 * - 本地绝对路径（markdown-it 输出可能带百分号编码，如中文/空格路径）→
 *   file:/// URL（优先百分号解码后的路径，文件不存在回退原始字面值）
 * - http(s)/已是 file: 的 src 保持原样
 */
QString DAMarkdownExporter::htmlForWord(const QString& html, const QString& tempDirPath)
{
    static const QRegularExpression srcRegex(QStringLiteral("src=\"([^\"]*)\""));
    static const QRegularExpression localPathRegex(QStringLiteral("^[A-Za-z]:[/\\\\].*"));
    struct SrcReplacement
    {
        int start;
        int length;
        QString url;
    };
    QList< SrcReplacement > replacements;
    QRegularExpressionMatchIterator it = srcRegex.globalMatch(html);
    int imgIndex = 0;
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString src = m.captured(1);
        QString url;
        if (src.startsWith(QStringLiteral("data:image/"), Qt::CaseInsensitive)) {
            // data:image/xxx;base64,.....
            int commaPos = src.indexOf(',');
            int semiPos  = src.indexOf(';');
            if (commaPos < 0 || semiPos < 0 || semiPos > commaPos) {
                continue;
            }
            QString mime = src.left(semiPos).mid(5).toLower();
            QString ext;
            if (mime == QStringLiteral("image/png")) {
                ext = QStringLiteral("png");
            } else if (mime == QStringLiteral("image/jpeg")) {
                ext = QStringLiteral("jpg");
            } else if (mime == QStringLiteral("image/gif")) {
                ext = QStringLiteral("gif");
            } else if (mime == QStringLiteral("image/webp")) {
                ext = QStringLiteral("webp");
            }
            if (ext.isEmpty()) {
                continue;
            }
            QByteArray bytes = QByteArray::fromBase64(src.mid(commaPos + 1).toLatin1());
            if (bytes.isEmpty()) {
                continue;
            }
            QDir tempDir(tempDirPath);
            if (!tempDir.exists() && !QDir().mkpath(tempDirPath)) {
                continue;
            }
            QString filePath = tempDir.filePath(QStringLiteral("img_%1.%2").arg(imgIndex++).arg(ext));
            QFile f(filePath);
            if (!f.open(QIODevice::WriteOnly)) {
                continue;
            }
            f.write(bytes);
            f.close();
            url = QUrl::fromLocalFile(filePath).toString();
        } else if (localPathRegex.match(src).hasMatch()) {
            QString candidate = QUrl::fromPercentEncoding(src.toUtf8());
            if (!QFileInfo::exists(candidate)) {
                candidate = src;  // 路径含字面 % 时解码结果不存在，回退原始字面值
            }
            url = QUrl::fromLocalFile(candidate).toString();
        } else {
            continue;  // http(s)/file:/相对路径等保持原样
        }
        SrcReplacement r;
        r.start  = static_cast< int >(m.capturedStart(1));
        r.length = static_cast< int >(m.capturedLength(1));
        r.url    = url;
        replacements.append(r);
    }
    QString result = html;
    for (int i = replacements.size() - 1; i >= 0; --i) {
        const SrcReplacement& r = replacements.at(i);
        result.replace(r.start, r.length, r.url);
    }
    return result;
}

/**
 * @brief 离屏渲染 markdown 为 HTML 片段
 *
 * 与查看器共用 qrc:///DAMarkdown/markdown.html 壳（markdown-it +
 * highlight.js + KaTeX），保证导出结果与窗口所见一致：
 * 1. 图片路径归一化并内嵌 data URI（markdown 源文本层面）
 * 2. 离屏 QWebEnginePage 加载壳（局部事件循环等待 loadFinished）
 * 3. markdown 经 exportRenderBegin/renderMarkdownAppend 分片下发（大体积
 *    base64 载荷规避 IPC 上限，同页 FIFO 保序）
 * 4. exportRenderEnd 执行渲染并取回总长度，再分片取回渲染结果
 * @param markdown markdown 源文本
 * @param baseDir 相对图片路径解析基准目录
 * @param mathmlOutput true 时 KaTeX 以 MathML 输出（Word 原生公式），false 为默认 HTML 输出
 * @param errMsg 失败时接收错误信息
 * @return 渲染后的 HTML 片段，失败返回空串
 */
QString DAMarkdownExporter::renderBodyHtml(const QString& markdown, const QString& baseDir, bool mathmlOutput,
                                           QString* errMsg)
{
    auto setError = [errMsg](const QString& msg) {
        if (errMsg) {
            *errMsg = msg;
        }
    };

    // 渲染副本：路径归一化 + 图片 data URI 内嵌（调用方源 markdown 不受影响）
    const QString embedded = embedLocalImagesAsDataUri(normalizeImagePaths(markdown, baseDir));

    QWebEnginePage page;  // 离屏页面，无关联 view
    if (!waitPageLoaded(&page, QUrl(QStringLiteral("qrc:///DAMarkdown/markdown.html")), kPageLoadTimeoutMs)) {
        setError("failed to load markdown render shell");
        return QString();
    }

    // 分片下发渲染（renderMarkdownAppend 无需等待回调，同页 FIFO 保序）
    page.runJavaScript(
        QStringLiteral("exportRenderBegin('%1')").arg(mathmlOutput ? QStringLiteral("mathml") : QStringLiteral("html")));
    const int total = embedded.size();
    int offset      = 0;
    while (offset < total) {
        int end = qMin(offset + kInputChunkUnits, total);
        // 分片边界不能落在 UTF-16 代理对中间，否则两侧字符串各出现孤立代理项
        if (end < total && QChar::isHighSurrogate(embedded.at(end - 1).unicode())) {
            --end;
        }
        page.runJavaScript(QStringLiteral("renderMarkdownAppend(\"%1\")").arg(
            DAMarkdownJsUtils::toJsString(embedded.mid(offset, end - offset))));
        offset = end;
    }
    QVariant lenVar;
    if (!runJsAndWait(&page, QStringLiteral("exportRenderEnd()"), kRenderTimeoutMs, &lenVar)) {
        setError("markdown render timed out");
        return QString();
    }
    const int totalHtml = lenVar.toInt();
    if (totalHtml <= 0) {
        setError("markdown render produced empty result");
        return QString();
    }

    // 分片取回渲染结果（getExportHtmlChunk 内部已处理代理对边界，按实际推进）
    QString body;
    while (body.size() < totalHtml) {
        QVariant chunkVar;
        if (!runJsAndWait(&page,
                          QStringLiteral("getExportHtmlChunk(%1, %2)").arg(body.size()).arg(kOutputChunkUnits),
                          kChunkTimeoutMs,
                          &chunkVar)) {
            setError("timed out retrieving render result");
            return QString();
        }
        const QString chunk = chunkVar.toString();
        if (chunk.isEmpty()) {
            setError("unexpected empty chunk while retrieving render result");
            return QString();
        }
        body += chunk;
    }
    page.runJavaScript(QStringLiteral("clearExportHtml()"));
    return body;
}

/**
 * @brief 组装单一离线 HTML 文档
 *
 * 内联 markdown.css（查看器同款，含 hljs 主题）与 katex.min.css（字体已
 * base64 内联），无任何外部引用与脚本；追加 @media print 样式供 PDF 打印
 * （长代码行换行、表格不截断、取消 900px 居中限宽）。
 */
QString DAMarkdownExporter::buildStandaloneHtml(const QString& bodyHtml, const QString& title)
{
    const QString mdCss    = readResourceText(QStringLiteral(":/DAMarkdown/markdown.css"));
    const QString katexCss = inlineKatexFonts(readResourceText(QStringLiteral(":/DAAgent/katex/katex.min.css")));
    static const QString kPrintCss = QStringLiteral(R"(/* print (PDF export) overrides */
@media print {
    body { padding: 0; }
    #content { max-width: none; margin: 0; }
    pre, table, .math-block { overflow: visible; }
    pre { white-space: pre-wrap; word-break: break-all; }
    img { max-width: 100% !important; page-break-inside: avoid; }
    h1, h2, h3, h4, h5, h6 { page-break-after: avoid; }
    table, tr, img { page-break-inside: avoid; }
})");
    return QStringLiteral(
               "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n<meta charset=\"utf-8\">\n"
               "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
               "<title>%1</title>\n<style>\n%2\n%3\n%4\n</style>\n</head>\n<body>\n"
               "<div id=\"content\">\n%5\n</div>\n</body>\n</html>\n")
        .arg(htmlEscape(title), mdCss, katexCss, kPrintCss, bodyHtml);
}

/**
 * @brief 组装 Word 友好的简化 HTML
 *
 * Word 的 HTML 导入仅稳定支持基础 CSS（字体、表格边框、背景色），复杂的
 * GitHub 风格样式与外部资源引用会被忽略或导致排版错乱，故用极简样式；
 * 公式此时已是 MathML（Word 原生公式格式）。
 */
QString DAMarkdownExporter::buildWordHtml(const QString& bodyHtml, const QString& title)
{
    static const QString kWordCss = QStringLiteral(R"(body { font-family: "Microsoft YaHei", "Segoe UI", sans-serif; font-size: 11pt; }
pre { background-color: #f6f8fa; font-family: Consolas, monospace; font-size: 10pt; }
code { font-family: Consolas, monospace; }
table { border-collapse: collapse; }
th, td { border: 1px solid #999999; padding: 4px 6px; }
img { max-width: 600px; })");
    return QStringLiteral(
               "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n<meta charset=\"utf-8\">\n"
               "<title>%1</title>\n<style>\n%2\n</style>\n</head>\n<body>\n%3\n</body>\n</html>\n")
        .arg(htmlEscape(title), kWordCss, bodyHtml);
}

/**
 * @brief 经 QWebEnginePage::printToPdf 将自包含 HTML 打印为 A4 PDF
 *
 * 渲染结果与查看器所见一致（KaTeX 公式、hljs 代码高亮、内嵌图片）。
 * 打印前轮询 document.fonts.ready（KaTeX woff2 字体异步加载，未就绪时
 * 公式会以回退字体打印），超时则直接打印（降级而非失败）。
 */
bool DAMarkdownExporter::printPdf(const QString& standaloneHtml, const QString& pdfPath, QString* errMsg)
{
    auto setError = [errMsg](const QString& msg) {
        if (errMsg) {
            *errMsg = msg;
        }
    };

    // 自包含 HTML（图片/字体全内嵌）写临时文件，经 file:// 加载打印
    const QString tempFile = QDir::tempPath() + "/da_md_export_"
                             + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".html";
    {
        QFile f(tempFile);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setError(QString("failed to create temporary file: %1").arg(f.errorString()));
            return false;
        }
        f.write(standaloneHtml.toUtf8());
        f.close();
    }

    QWebEnginePage page;
    // 保留远程 http(s) 图片的直接加载能力（本地 file 页面默认被拦）
    page.settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    if (!waitPageLoaded(&page, QUrl::fromLocalFile(tempFile), kPageLoadTimeoutMs)) {
        QFile::remove(tempFile);
        setError("failed to load page for printing");
        return false;
    }

    // 等待 KaTeX 字体加载完成（超时降级直接打印）
    page.runJavaScript(
        QStringLiteral("document.fonts.ready.then(function(){window.__daFontsReady=true})"));
    for (int i = 0; i < kFontsReadyMaxPolls; ++i) {
        QVariant v;
        if (!runJsAndWait(&page, QStringLiteral("window.__daFontsReady === true"), 2000, &v)) {
            break;  // 脚本异常（无 fonts API 等），直接打印
        }
        if (v.toBool()) {
            break;
        }
        waitMs(kFontsReadyPollMs);
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool gotSignal = false;
    bool printOk   = false;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&page, &QWebEnginePage::pdfPrintingFinished, &loop,
                     [&loop, &gotSignal, &printOk](const QString&, bool success) {
                         gotSignal = true;
                         printOk   = success;
                         loop.quit();
                     });
    const QPageLayout layout(QPageSize(QPageSize::A4), QPageLayout::Portrait,
                             QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);
    timer.start(kPrintTimeoutMs);
    page.printToPdf(pdfPath, layout);
    loop.exec();
    QFile::remove(tempFile);

    if (!gotSignal) {
        setError("PDF printing timed out");
        return false;
    }
    if (!printOk) {
        setError("PDF printing failed");
        return false;
    }
    return true;
}

#ifdef Q_OS_WIN
/**
 * @brief 经 Word COM 将 HTML 另存为 .docx
 *
 * 图片 src 已由 htmlForWord 转为 file:/// URL（data URI 解码写入临时目录），
 * Word 打开时按 URL 抓取图片并在 saveAs 时内嵌进 docx，随后清理临时目录。
 */
bool DAMarkdownExporter::convertDocx(const QString& wordHtml, const QString& docxPath, QString* errMsg)
{
    auto setError = [errMsg](const QString& msg) {
        if (errMsg) {
            *errMsg = msg;
        }
    };

    const QString tempDir = QDir::tempPath() + "/da_md_export_"
                            + QString::number(QDateTime::currentMSecsSinceEpoch());
    const QString htmlFile = tempDir + "/report.html";
    {
        QFile f(htmlFile);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setError(QString("failed to create temporary file: %1").arg(f.errorString()));
            return false;
        }
        f.write(htmlForWord(wordHtml, tempDir).toUtf8());
        f.close();
    }

    DAAxObjectWordWrapper word;
    if (!word.open(htmlFile, false)) {
        QDir(tempDir).removeRecursively();
        setError("failed to open Word, make sure Microsoft Word is installed");
        return false;
    }
    const bool ok = word.saveAs(docxPath);
    word.quit();
    QDir(tempDir).removeRecursively();
    if (!ok) {
        setError("failed to save .docx via Word COM");
        return false;
    }
    return true;
}
#else
/**
 * @brief Word COM 仅 Windows 可用
 */
bool DAMarkdownExporter::convertDocx(const QString& wordHtml, const QString& docxPath, QString* errMsg)
{
    Q_UNUSED(wordHtml)
    Q_UNUSED(docxPath)
    if (errMsg) {
        *errMsg = "DOCX export is only available on Windows with Microsoft Word installed";
    }
    return false;
}
#endif

/**
 * @brief 提取文档标题：首个一级标题，否则回退 fallback
 */
QString DAMarkdownExporter::extractTitle(const QString& markdown, const QString& fallback)
{
    static const QRegularExpression h1Regex(QStringLiteral("(?m)^#\\s+(.+?)\\s*$"));
    QRegularExpressionMatch m = h1Regex.match(markdown);
    if (m.hasMatch() && !m.captured(1).isEmpty()) {
        return m.captured(1);
    }
    return fallback;
}

/**
 * @copydoc exportToFile
 */
bool DAMarkdownExporter::exportToFile(const QString& markdown, const QString& outputPath, Format fmt,
                                      const QString& baseDir, QString* errMsg)
{
    auto setError = [errMsg](const QString& msg) {
        if (errMsg) {
            *errMsg = msg;
        }
    };
    if (markdown.isEmpty()) {
        setError("markdown content is empty");
        return false;
    }
    if (outputPath.isEmpty()) {
        setError("output path is empty");
        return false;
    }
    QFileInfo fi(outputPath);
    if (!QDir().mkpath(fi.absolutePath())) {
        setError(QString("failed to create output directory: %1").arg(fi.absolutePath()));
        return false;
    }
    const QString title = extractTitle(markdown, fi.baseName());

    switch (fmt) {
    case Format::Html: {
        const QString body = renderBodyHtml(markdown, baseDir, false, errMsg);
        if (body.isEmpty()) {
            return false;  // errMsg 已由 renderBodyHtml 设置
        }
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setError(QString("failed to open file: %1").arg(f.errorString()));
            return false;
        }
        const QByteArray data = buildStandaloneHtml(body, title).toUtf8();
        if (f.write(data) != data.size()) {
            setError(QString("failed to write file: %1").arg(f.errorString()));
            f.close();
            return false;
        }
        f.close();
        return true;
    }
    case Format::Pdf: {
        const QString body = renderBodyHtml(markdown, baseDir, false, errMsg);
        if (body.isEmpty()) {
            return false;
        }
        return printPdf(buildStandaloneHtml(body, title), fi.absoluteFilePath(), errMsg);
    }
    case Format::Docx: {
        const QString body = renderBodyHtml(markdown, baseDir, true, errMsg);
        if (body.isEmpty()) {
            return false;
        }
        return convertDocx(buildWordHtml(body, title), fi.absoluteFilePath(), errMsg);
    }
    }
    setError("unknown export format");
    return false;
}
}  // namespace DA
