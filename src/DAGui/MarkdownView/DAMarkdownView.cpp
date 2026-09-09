#include "DAMarkdownView.h"
#include "DAMarkdownJsUtils.h"
#include "DAMarkdownExporter.h"
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
 * 注入任意文本（非文件来源），源文件路径上下文被清除；
 * 从文件加载请使用 loadFile。
 * 若 HTML 壳尚未加载完成，缓存文本等待 loadFinished 后自动渲染。
 * @param markdown markdown 源文本
 */
void DAMarkdownView::setMarkdown(const QString& markdown)
{
    if (mMarkdown == markdown) {
        return;
    }
    mMarkdown       = markdown;
    mSourceFilePath = QString();
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
    mSourceFilePath.clear();
    if (mPageLoaded && mWebView && mWebView->page()) {
        mWebView->page()->runJavaScript(QStringLiteral("clearContent()"));
    }
    emit markdownChanged(QString());
}

/**
 * @brief 从文件加载 markdown 并渲染
 *
 * 记录源文件路径作为相对图片路径的解析基准（图片在渲染时内嵌为
 * data URI，源文本本身不变）。
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
    const QString text = QString::fromUtf8(data);
    mSourceFilePath    = QFileInfo(filePath).absoluteFilePath();
    if (mMarkdown != text) {
        mMarkdown = text;
        if (mPageLoaded) {
            renderMarkdown();
        }
        emit markdownChanged(mMarkdown);
    }
    return true;
}

/**
 * @brief 当前加载的源文件路径
 * @return 绝对路径；非文件来源（setMarkdown 注入）返回空串
 */
QString DAMarkdownView::sourceFilePath() const
{
    return mSourceFilePath;
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
 * @brief 构建渲染副本：相对图片路径转绝对并内嵌 data URI
 *
 * qrc 壳页面无法加载本地路径图片（本地路径相对未设置的 baseUrl 解析
 * 必失败），渲染副本在源文本层面把可读取的本地图片替换为 data URI，
 * 使查看器直接显示图片；mMarkdown 源文本保持不变（查看源码/另存为
 * .md 仍是原始路径）。
 * @return 用于下发渲染的文本副本
 */
QString DAMarkdownView::buildRenderCopy() const
{
    if (mSourceFilePath.isEmpty()) {
        // 无文件上下文：仅绝对路径与 file: URL 图片可解析
        return DAMarkdownExporter::embedLocalImagesAsDataUri(mMarkdown);
    }
    const QString baseDir = QFileInfo(mSourceFilePath).absolutePath();
    return DAMarkdownExporter::embedLocalImagesAsDataUri(
        DAMarkdownExporter::normalizeImagePaths(mMarkdown, baseDir));
}

/**
 * @brief 分片下发渲染文本
 *
 * 含内嵌 base64 图片的文本可达数 MB，单次 runJavaScript 会超 Chromium
 * IPC 消息上限静默失败（页面空白无报错），按 512KB/片经
 * renderMarkdownBegin/Append/End 累积后整体渲染；同页 runJavaScript
 * FIFO 保序，无需等待中间片回调（同 DAAgentWebChannel 分片先例）。
 * @param text 待渲染文本（渲染副本）
 */
void DAMarkdownView::dispatchRender(const QString& text)
{
    if (!mWebView || !mWebView->page()) {
        return;
    }
    constexpr int kChunkUnits = 512 * 1024;
    mWebView->page()->runJavaScript(QStringLiteral("renderMarkdownBegin()"));
    const int total = text.size();
    int offset      = 0;
    while (offset < total) {
        int end = qMin(offset + kChunkUnits, total);
        // 分片边界避开 UTF-16 代理对（切断会产生孤立代理项，字符损坏）
        if (end < total && QChar::isHighSurrogate(text.at(end - 1).unicode())) {
            --end;
        }
        mWebView->page()->runJavaScript(QStringLiteral("renderMarkdownAppend(\"%1\")").arg(
            DAMarkdownJsUtils::toJsString(text.mid(offset, end - offset))));
        offset = end;
    }
    mWebView->page()->runJavaScript(QStringLiteral("renderMarkdownEnd()"));
}

/**
 * @brief 渲染当前缓存的 markdown（图片内嵌副本分片下发）
 */
void DAMarkdownView::renderMarkdown()
{
    if (!mWebView || !mWebView->page()) {
        return;
    }
    dispatchRender(buildRenderCopy());
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
 * 菜单项：Copy / Select All / View Markdown Source / Save Markdown As... /
 * Export as HTML/PDF/Word... / Reload，全部使用 tr() 翻译。
 * 菜单与 action 持久存在、复用，随上下文变化的可用状态
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

    // 导出为 HTML（单一离线文件，图片内嵌）
    mExportHtmlAction = mContextMenu->addAction(tr("Export as HTML..."));  // cn:导出为 HTML...
    QObject::connect(mExportHtmlAction, &QAction::triggered, this, [this]() {
        onExportAs(DAMarkdownExporter::Format::Html);
    });

    // 导出为 PDF（与查看器渲染一致：公式/高亮/图片）
    mExportPdfAction = mContextMenu->addAction(tr("Export as PDF..."));  // cn:导出为 PDF...
    QObject::connect(mExportPdfAction, &QAction::triggered, this, [this]() {
        onExportAs(DAMarkdownExporter::Format::Pdf);
    });
#ifdef Q_OS_WIN
    // 导出为 Word 文档（Word COM，仅 Windows）
    mExportDocxAction = mContextMenu->addAction(tr("Export as Word Document..."));  // cn:导出为 Word 文档...
    QObject::connect(mExportDocxAction, &QAction::triggered, this, [this]() {
        onExportAs(DAMarkdownExporter::Format::Docx);
    });
#endif

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
    mExportHtmlAction->setEnabled(!mMarkdown.isEmpty() && !mExporting);
    mExportPdfAction->setEnabled(!mMarkdown.isEmpty() && !mExporting);
    if (mExportDocxAction) {
        mExportDocxAction->setEnabled(!mMarkdown.isEmpty() && !mExporting);
    }
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

/**
 * @brief 导出当前文档为 HTML/PDF/Word
 *
 * 经 DAMarkdownExporter（与查看器同一渲染管线）导出，弹文件对话框选择
 * 输出路径；导出为同步操作（内部局部事件循环等待 WebEngine），期间禁用
 * 重复触发。成功/失败通过 daInfo/daWarning 反馈到 UI 消息队列。
 * @param fmt 导出格式
 */
void DAMarkdownView::onExportAs(DAMarkdownExporter::Format fmt)
{
    if (mMarkdown.isEmpty() || mExporting) {
        return;
    }
    QString filter;
    QString defaultExt;
    switch (fmt) {
    case DAMarkdownExporter::Format::Html:
        filter     = tr("HTML Files (*.html);;All Files (*)");  // cn:HTML 文件 (*.html);;所有文件 (*)
        defaultExt = QStringLiteral(".html");
        break;
    case DAMarkdownExporter::Format::Pdf:
        filter     = tr("PDF Files (*.pdf);;All Files (*)");  // cn:PDF 文件 (*.pdf);;所有文件 (*)
        defaultExt = QStringLiteral(".pdf");
        break;
    case DAMarkdownExporter::Format::Docx:
        filter     = tr("Word Documents (*.docx);;All Files (*)");  // cn:Word 文档 (*.docx);;所有文件 (*)
        defaultExt = QStringLiteral(".docx");
        break;
    }
    const QString baseName = mSourceFilePath.isEmpty() ? QStringLiteral("markdown")
                                                       : QFileInfo(mSourceFilePath).completeBaseName();
    const QString path = QFileDialog::getSaveFileName(this,
                                                      tr("Export Markdown"),  // cn:导出 Markdown
                                                      baseName + defaultExt,
                                                      filter);
    if (path.isEmpty()) {
        return;  // 用户取消
    }

    mExporting = true;
    DAMarkdownExporter exporter;
    const QString baseDir = mSourceFilePath.isEmpty() ? QString() : QFileInfo(mSourceFilePath).absolutePath();
    QString err;
    const bool ok = exporter.exportToFile(mMarkdown, path, fmt, baseDir, &err);
    mExporting = false;

    if (ok) {
        daInfo << tr("Markdown exported to %1").arg(path);  // cn:Markdown 已导出到 %1
    } else {
        daWarning << tr("Failed to export markdown: %1").arg(err);  // cn:导出 Markdown 失败：%1
    }
}

} // namespace DA
