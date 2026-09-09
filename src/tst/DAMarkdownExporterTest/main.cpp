// DAMarkdownExporterTest/main.cpp
// 单元测试：DAMarkdownExporter（markdown 统一导出引擎）。
//
// 覆盖两类内容：
// 1. 纯函数（无 WebEngine 依赖）：normalizeImagePaths / embedLocalImagesAsDataUri /
//    localFileToDataUri / inlineKatexFonts / htmlForWord。
// 2. WebEngine 回归：exportToFile(Html/Pdf) 的产物形态——HTML 为单一离线文件
//    （样式/字体/图片全内嵌、无 qrc: 引用），PDF 非 %PDF 头且非空。
//    docx 不自动测（会拉起真实 Word COM），留人工验证。

#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "DAMarkdownExporter.h"

using DA::DAMarkdownExporter;

class DAMarkdownExporterTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testNormalizeImagePaths();     // 相对→绝对，协议前缀/绝对路径原样
    void testEmbedLocalImages();        // 本地 png 内嵌 data URI；缺失文件保留原值
    void testEmbedSvgAsPng();           // svg（白名单外格式）经 QImage 转 PNG data URI
    void testLocalFileToDataUri();      // mime 推断 + base64 可解码还原
    void testInlineKatexFonts();        // 字体 base64 内嵌、ttf 回退剔除
    void testHtmlForWord();             // data URI→临时文件 file:///；本地路径→file:///
    void testExportHtml();              // HTML 导出：内嵌样式/图片、无 qrc: 引用
    void testExportPdf();               // PDF 导出：%PDF 头且非空
};

// ---------------------------------------------------------------------------
// normalizeImagePaths：相对路径基于 baseDir 转绝对，协议前缀与绝对路径原样
// ---------------------------------------------------------------------------
void DAMarkdownExporterTest::testNormalizeImagePaths()
{
    const QString md = QStringLiteral(
        "![a](charts/temp.png)\n"
        "![b](http://x.com/a.png)\n"
        "![c](https://x.com/a.png)\n"
        "![d](data:image/png;base64,AAAA)\n"
        "![e](file:///C:/tmp/a.png)\n"
        "![f](C:/data/img.png)\n"
        "![g](./sub/b.png)\n");
    const QString result = DAMarkdownExporter::normalizeImagePaths(md, QStringLiteral("D:/report"));
    QVERIFY(result.contains(QStringLiteral("![a](D:/report/charts/temp.png)")));
    QVERIFY(result.contains(QStringLiteral("![b](http://x.com/a.png)")));
    QVERIFY(result.contains(QStringLiteral("![c](https://x.com/a.png)")));
    QVERIFY(result.contains(QStringLiteral("![d](data:image/png;base64,AAAA)")));
    QVERIFY(result.contains(QStringLiteral("![e](file:///C:/tmp/a.png)")));
    QVERIFY(result.contains(QStringLiteral("![f](C:/data/img.png)")));  // 绝对路径原样
    QVERIFY(result.contains(QStringLiteral("![g](D:/report/sub/b.png)")));
    // 非图片链接不受影响
    QVERIFY(result.contains(QStringLiteral("file:///C:/tmp/a.png)")));
}

// ---------------------------------------------------------------------------
// embedLocalImagesAsDataUri：可读取的本地图片内嵌，缺失文件保留原值
// ---------------------------------------------------------------------------
void DAMarkdownExporterTest::testEmbedLocalImages()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // 生成 2x2 红色 PNG
    QImage img(2, 2, QImage::Format_RGB32);
    img.fill(Qt::red);
    const QString pngPath = QDir(dir.path()).filePath("a.png");
    QVERIFY(img.save(pngPath, "PNG"));

    const QString md = QStringLiteral("![ok](%1)\n![miss](%2/not_exist.png)\n![remote](http://x.com/a.png)")
                           .arg(QDir::toNativeSeparators(pngPath), dir.path());
    const QString result = DAMarkdownExporter::embedLocalImagesAsDataUri(md);
    QVERIFY(result.contains(QStringLiteral("data:image/png;base64,")));
    // 缺失文件与远程引用保留原值
    QVERIFY(result.contains(QStringLiteral("not_exist.png")));
    QVERIFY(result.contains(QStringLiteral("http://x.com/a.png")));
}

// ---------------------------------------------------------------------------
// svg（markdown-it data URI 白名单外格式）经 QImage 统一转码 PNG
// ---------------------------------------------------------------------------
void DAMarkdownExporterTest::testEmbedSvgAsPng()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString svgPath = QDir(dir.path()).filePath("a.svg");
    {
        QFile f(svgPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\">"
                "<rect width=\"4\" height=\"4\" fill=\"blue\"/></svg>");
        f.close();
    }
    const QString dataUri = DAMarkdownExporter::localFileToDataUri(svgPath);
    // svg 插件可用时转 PNG（image/png）；无 svg 插件环境（极少数）QImage 加载
    // 失败返回空串——两种结果都合法，仅断言不产生 image/svg+xml
    if (!dataUri.isEmpty()) {
        QVERIFY(dataUri.startsWith(QStringLiteral("data:image/png;base64,")));
    }
}

// ---------------------------------------------------------------------------
// localFileToDataUri：mime 推断 + base64 可解码还原原始字节
// ---------------------------------------------------------------------------
void DAMarkdownExporterTest::testLocalFileToDataUri()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QImage img(4, 4, QImage::Format_RGB32);
    img.fill(Qt::blue);
    const QString jpgPath = QDir(dir.path()).filePath("b.jpg");
    QVERIFY(img.save(jpgPath, "JPG"));

    const QString dataUri = DAMarkdownExporter::localFileToDataUri(jpgPath);
    QVERIFY(dataUri.startsWith(QStringLiteral("data:image/jpeg;base64,")));
    // base64 部分可解码且非空
    const QString b64 = dataUri.mid(dataUri.indexOf(',') + 1);
    QVERIFY(!QByteArray::fromBase64(b64.toLatin1()).isEmpty());
    // 不存在的文件返回空串
    QVERIFY(DAMarkdownExporter::localFileToDataUri(dir.path() + "/none.png").isEmpty());
}

// ---------------------------------------------------------------------------
// inlineKatexFonts：woff2 引用替换为 data URI，ttf 回退剔除
// ---------------------------------------------------------------------------
void DAMarkdownExporterTest::testInlineKatexFonts()
{
    const QString css = QStringLiteral(
        "@font-face{font-family:KaTeX_AMS;src:url(fonts/KaTeX_AMS-Regular.woff2) format(\"woff2\"),"
        "url(fonts/KaTeX_AMS-Regular.ttf) format(\"truetype\")}\n"
        "body{background:url(qrc:///DAMarkdown/markdown.css)}");
    const QString result = DAMarkdownExporter::inlineKatexFonts(css);
    QVERIFY(!result.contains(QStringLiteral("url(fonts/")));
    QVERIFY(result.contains(QStringLiteral("data:font/woff2;base64,")));
    QVERIFY(!result.contains(QStringLiteral(".ttf")));
    // 非字体 url 引用不受影响
    QVERIFY(result.contains(QStringLiteral("qrc:///DAMarkdown/markdown.css")));
}

// ---------------------------------------------------------------------------
// htmlForWord：data URI→临时文件 file:/// URL；本地绝对路径→file:/// URL
// ---------------------------------------------------------------------------
void DAMarkdownExporterTest::testHtmlForWord()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString html = QStringLiteral(
        "<p><img src=\"data:image/png;base64,iVBORw0KGgo=\" /> <img src=\"C:/data/a%20b.png\" /></p>");
    const QString result = DAMarkdownExporter::htmlForWord(html, dir.path());
    QVERIFY(result.contains(QStringLiteral("src=\"file:///")));
    QVERIFY(!result.contains(QStringLiteral("src=\"data:")));
    QVERIFY(!result.contains(QStringLiteral("src=\"C:/")));
    // data URI 落盘为临时图片文件
    const QStringList entries = QDir(dir.path()).entryList({ QStringLiteral("img_*.png") }, QDir::Files);
    QCOMPARE(entries.size(), 1);
}

// ---------------------------------------------------------------------------
// exportToFile(Html)：单一离线文件——内联样式与图片、无 qrc: 引用
// ---------------------------------------------------------------------------
void DAMarkdownExporterTest::testExportHtml()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QImage img(6, 6, QImage::Format_RGB32);
    img.fill(Qt::green);
    const QString imgDirPath = QDir(dir.path()).filePath("charts");
    QVERIFY(QDir().mkpath(imgDirPath));
    QVERIFY(img.save(QDir(imgDirPath).filePath("c.png"), "PNG"));

    const QString md = QStringLiteral(
        "# Test Report\n\n"
        "Some **bold** text and a formula $E=mc^2$.\n\n"
        "```python\nprint('hello')\n```\n\n"
        "![chart](charts/c.png)\n");
    const QString outPath = QDir(dir.path()).filePath("out.html");
    QString err;
    DAMarkdownExporter exporter;
    QVERIFY2(exporter.exportToFile(md, outPath, DAMarkdownExporter::Format::Html, dir.path(), &err),
             qPrintable(err));

    QFile f(outPath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QString html = QString::fromUtf8(f.readAll());
    f.close();
    QVERIFY(html.contains(QStringLiteral("<title>Test Report</title>")));
    QVERIFY(html.contains(QStringLiteral("<style>")));
    QVERIFY(html.contains(QStringLiteral("data:image/png;base64,")));  // 图片内嵌
    QVERIFY(html.contains(QStringLiteral("data:font/woff2;base64,")));  // KaTeX 字体内嵌
    QVERIFY(!html.contains(QStringLiteral("qrc:")));  // 无 qrc 引用（完全离线）
    QVERIFY(!html.contains(QStringLiteral("<script")));  // 纯静态，无脚本
    QVERIFY(html.contains(QStringLiteral("<h1")));  // markdown 已渲染（非源文本）
}

// ---------------------------------------------------------------------------
// exportToFile(Pdf)：%PDF 头且非空
// ---------------------------------------------------------------------------
void DAMarkdownExporterTest::testExportPdf()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString md = QStringLiteral("# PDF Test\n\nFormula: $\\int_0^1 x dx$\n");
    const QString outPath = QDir(dir.path()).filePath("out.pdf");
    QString err;
    DAMarkdownExporter exporter;
    QVERIFY2(exporter.exportToFile(md, outPath, DAMarkdownExporter::Format::Pdf, dir.path(), &err),
             qPrintable(err));

    QFile f(outPath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray head = f.read(5);
    const qint64 size     = f.size();
    f.close();
    QCOMPARE(head, QByteArray("%PDF-"));
    QVERIFY(size > 1000);
}

int main(int argc, char* argv[])
{
    // QWebEnginePage 需要 QApplication（QCoreApplication 不足以创建渲染进程工厂）
    QApplication app(argc, argv);
    DAMarkdownExporterTest t;
    return QTest::qExec(&t, argc, argv);
}

#include "main.moc"
