#include "DAMarkdownView.h"
#include <QVBoxLayout>
#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QFont>
#include <QUrl>
#include "DALogCategory.h"

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
    // 拦截 QWebEngineView 默认右键菜单，改用自定义菜单
    mWebView->installEventFilter(this);

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

    // 构建右键菜单（仅一次，后续右键复用）
    buildContextMenu();
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

/**
 * @brief 拦截 QWebEngineView 的右键事件，改用自定义菜单
 *
 * 默认菜单的“Save page”/“View page source”针对 qrc:// HTML 壳无法工作，
 * 且菜单文本来自 Qt WebEngine 自身翻译（项目未随附，显示为英文）。
 * 这里在事件过滤器层拦截并替换为项目自定义、可翻译的菜单。
 * @param watched 被监听的对象（mWebView）
 * @param event 事件
 * @return 已处理返回 true，否则交给基类
 */
bool DAMarkdownView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == mWebView && event->type() == QEvent::ContextMenu) {
        auto* ctxEvent = static_cast< QContextMenuEvent* >(event);
        showContextMenu(ctxEvent->globalPos());
        return true;  // 阻止 QWebEngineView 弹出默认菜单
    }
    return QWidget::eventFilter(watched, event);
}

/**
 * @brief 构建右键菜单及其 action（仅在 setupUI 中调用一次）
 *
 * 菜单项：Copy / Select All / View Markdown Source / Save Markdown As... / Reload，
 * 全部使用 tr() 翻译。菜单与 action 持久存在、复用，随上下文变化的可用状态
 * 在 showContextMenu 中刷新，避免每次右键都重建。
 */
void DAMarkdownView::buildContextMenu()
{
    mContextMenu = new QMenu(this);

    // 复制
    mCopyAction = mContextMenu->addAction(tr("Copy"));  // cn:复制
    QObject::connect(mCopyAction, &QAction::triggered, this, [this]() {
        if (mPage) {
            mPage->triggerAction(QWebEnginePage::Copy);
        }
    });

    // 全选
    QAction* selectAllAct = mContextMenu->addAction(tr("Select All"));  // cn:全选
    QObject::connect(selectAllAct, &QAction::triggered, this, [this]() {
        if (mPage) {
            mPage->triggerAction(QWebEnginePage::SelectAll);
        }
    });

    mContextMenu->addSeparator();

    // 查看 Markdown 源码
    mViewMarkdownSourceAction = mContextMenu->addAction(tr("View Markdown Source"));  // cn:查看 Markdown 源码
    QObject::connect(mViewMarkdownSourceAction, &QAction::triggered, this, &DAMarkdownView::onViewMarkdownSource);

    // 保存 Markdown 为文件
    mSaveMarkdownAction = mContextMenu->addAction(tr("Save Markdown As..."));  // cn:保存 Markdown 为...
    QObject::connect(mSaveMarkdownAction, &QAction::triggered, this, &DAMarkdownView::onSaveMarkdownAs);

    mContextMenu->addSeparator();

    // 重新加载 HTML 壳并重新渲染缓存内容
    QAction* reloadAct = mContextMenu->addAction(tr("Reload"));  // cn:重新加载
    QObject::connect(reloadAct, &QAction::triggered, this, [this]() {
        if (mWebView) {
            mWebView->reload();
        }
    });
}

/**
 * @brief 弹出右键菜单
 *
 * 菜单及 action 在 buildContextMenu 中一次性构建，此处仅刷新随上下文
 * 变化的可用状态后弹出。
 * @param globalPos 菜单弹出的全局坐标
 */
void DAMarkdownView::showContextMenu(const QPoint& globalPos)
{
    if (!mContextMenu) {
        return;
    }
    // 刷新随上下文变化的可用状态
    mCopyAction->setEnabled(mPage && mPage->action(QWebEnginePage::Copy)->isEnabled());
    mViewMarkdownSourceAction->setEnabled(!mMarkdown.isEmpty());
    mSaveMarkdownAction->setEnabled(!mMarkdown.isEmpty());
    mContextMenu->exec(globalPos);
}

/**
 * @brief 在只读对话框中展示当前 markdown 源文本
 */
void DAMarkdownView::onViewMarkdownSource()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Markdown Source"));  // cn:Markdown 源码
    auto* edit = new QPlainTextEdit(&dlg);
    edit->setPlainText(mMarkdown);
    edit->setReadOnly(true);
    edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont monoFont(QStringLiteral("Consolas"), 10);
    monoFont.setStyleHint(QFont::Monospace);
    edit->setFont(monoFont);
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    QObject::connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::close);
    auto* layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(edit, 1);
    layout->addWidget(btnBox);
    dlg.resize(640, 520);
    dlg.exec();
}

/**
 * @brief 将当前 markdown 源文本保存为 UTF-8 文件
 *
 * 保存成功/失败通过 daInfo/daWarning 反馈到 UI 消息队列。
 */
void DAMarkdownView::onSaveMarkdownAs()
{
    if (mMarkdown.isEmpty()) {
        return;
    }
    const QString defaultName = QStringLiteral("markdown.md");
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Save Markdown"),  // cn:保存 Markdown
        defaultName,
        tr("Markdown Files (*.md);;Text Files (*.txt);;All Files (*)"));  // cn:Markdown 文件 (*.md);;文本文件 (*.txt);;所有文件 (*)
    if (path.isEmpty()) {
        return;  // 用户取消
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        daWarning << tr("Failed to save markdown: %1").arg(file.errorString());  // cn:保存 Markdown 失败：%1
        return;
    }
    const QByteArray data = mMarkdown.toUtf8();
    if (file.write(data) != data.size()) {
        daWarning << tr("Failed to save markdown: %1").arg(file.errorString());  // cn:保存 Markdown 失败：%1
        file.close();
        return;
    }
    file.close();
    daInfo << tr("Markdown saved to %1").arg(path);  // cn:Markdown 已保存到 %1
}

} // namespace DA
