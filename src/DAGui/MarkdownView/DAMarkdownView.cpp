#include "DAMarkdownView.h"
#include <QVBoxLayout>
#include <QFile>
#include <QUrl>

namespace DA
{

/**
 * @brief 把 QString 转义为可安全嵌入 JS 字符串字面量的形式
 *
 * 处理双引号、反斜杠、换行、制表符及控制字符；CJK 等 BMP 字符直通。
 * 复用 DAAgentWebChannel 的同名逻辑。
 * @param str 原始字符串
 * @return 转义后的字符串（外层双引号由调用方提供）
 */
static QString toJsString(const QString& str)
{
    QString result;
    result.reserve(str.size() + 8);
    for (const QChar& ch : str) {
        ushort code = ch.unicode();
        switch (code) {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            if (code < 0x20) {
                // 控制字符用 \uXXXX 表示
                result += QString("\\u%1").arg(code, 4, 16, QChar('0'));
            } else {
                // 含 CJK 在内的 BMP 字符直接保留
                result += ch;
            }
        }
    }
    return result;
}

// ================================================================
// DAMarkdownWebPage
// ================================================================

DAMarkdownWebPage::DAMarkdownWebPage(QObject* parent) : QWebEnginePage(parent)
{
}

DAMarkdownWebPage::~DAMarkdownWebPage()
{
}

/**
 * @brief 拦截页面导航请求
 *
 * - qrc:/data: 协议 → 放行（内部资源加载）
 * - 无 scheme 的相对 URL（含锚点 #section）→ 放行（页内导航）
 * - 其余 scheme（http/https/da-figure:/mailto: 等）→ 主框架时发射 linkClicked 并拒绝导航
 *
 * @param url 目标 URL
 * @param type 导航类型
 * @param isMainFrame 是否为主框架
 * @return true 允许导航，false 拒绝导航
 */
bool DAMarkdownWebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame)
{
    Q_UNUSED(type)
    // 内部资源加载放行
    if (url.scheme() == "qrc" || url.scheme() == "data" || url.scheme() == "about") {
        return true;
    }
    // 相对 URL（含纯锚点 #section）放行，允许页内跳转
    if (url.scheme().isEmpty()) {
        return true;
    }
    // 外部/自定义协议链接：主框架拦截并通知
    if (isMainFrame) {
        emit linkClicked(url);
        return false;
    }
    return true;
}

// ================================================================
// DAMarkdownView
// ================================================================

DAMarkdownView::DAMarkdownView(QWidget* parent)
    : QWidget(parent), mWebView(nullptr), mPage(nullptr)
{
    setupUI();
}

DAMarkdownView::~DAMarkdownView()
{
    // 页面的父对象是 mWebView，随 mWebView 析构自动释放
}

/**
 * @brief 初始化 UI：创建 QWebEngineView + 自定义 Page，加载 HTML 壳
 */
void DAMarkdownView::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    mWebView = new QWebEngineView(this);
    mWebView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 自定义 Page，父对象设为 mWebView，随 mWebView 析构释放
    mPage = new DAMarkdownWebPage(mWebView);
    mWebView->setPage(mPage);

    // 页面加载完成后渲染缓存的 markdown
    connect(mWebView, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok) {
            mPageLoaded = true;
            if (!mMarkdown.isEmpty()) {
                renderMarkdown();
            }
        }
    });

    // 链接点击信号转发
    connect(mPage, &DAMarkdownWebPage::linkClicked, this, &DAMarkdownView::linkClicked);

    layout->addWidget(mWebView);

    // 加载 HTML 壳（markdown-it / highlight.js 由 HTML 内部从 DAAgent qrc 引用）
    mWebView->setUrl(QUrl(QStringLiteral("qrc:///DAMarkdown/markdown.html")));
}

/**
 * @brief 设置并渲染 markdown 文本
 *
 * 若 HTML 壳尚未加载完成，缓存文本等待 loadFinished 后自动渲染。
 * @param markdown markdown 源文本
 */
void DAMarkdownView::setMarkdown(const QString& markdown)
{
    if (mMarkdown == markdown) {
        return;
    }
    mMarkdown = markdown;
    if (mPageLoaded) {
        renderMarkdown();
    }
    emit markdownChanged(mMarkdown);
}

/**
 * @brief 获取当前 markdown 源文本
 * @return markdown 源文本
 */
QString DAMarkdownView::markdown() const
{
    return mMarkdown;
}

/**
 * @brief 清空内容
 */
void DAMarkdownView::clear()
{
    if (mMarkdown.isEmpty()) {
        return;
    }
    mMarkdown.clear();
    if (mPageLoaded && mWebView && mWebView->page()) {
        mWebView->page()->runJavaScript(QStringLiteral("clearContent()"));
    }
    emit markdownChanged(QString());
}

/**
 * @brief 从文件加载 markdown 并渲染
 * @param filePath 文件路径（UTF-8 编码）
 * @return 成功返回 true
 */
bool DAMarkdownView::loadFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QByteArray data = file.readAll();
    file.close();
    setMarkdown(QString::fromUtf8(data));
    return true;
}

/**
 * @brief 滚动到顶部
 */
void DAMarkdownView::scrollToTop()
{
    if (mPageLoaded && mWebView && mWebView->page()) {
        mWebView->page()->runJavaScript(QStringLiteral("window.scrollTo(0, 0)"));
    }
}

/**
 * @brief 滚动到底部
 */
void DAMarkdownView::scrollToBottom()
{
    if (mPageLoaded && mWebView && mWebView->page()) {
        mWebView->page()->runJavaScript(QStringLiteral("window.scrollTo(0, document.body.scrollHeight)"));
    }
}

/**
 * @brief 调用 JS renderMarkdown 函数渲染当前缓存的 markdown
 */
void DAMarkdownView::renderMarkdown()
{
    if (!mWebView || !mWebView->page()) {
        return;
    }
    QString js = QStringLiteral("renderMarkdown(\"%1\")").arg(toJsString(mMarkdown));
    mWebView->page()->runJavaScript(js);
}

} // namespace DA
