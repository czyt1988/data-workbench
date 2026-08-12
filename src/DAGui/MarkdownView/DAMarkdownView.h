#pragma once
#include <QWidget>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QUrl>
#include "DAGuiAPI.h"

namespace DA
{

// Markdown 渲染页面，拦截链接点击并通过信号转发
class DAGUI_API DAMarkdownWebPage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit DAMarkdownWebPage(QObject* parent = nullptr);
    ~DAMarkdownWebPage();

Q_SIGNALS:
    // 用户点击非内部链接时发射
    void linkClicked(const QUrl& url);

protected:
    bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) override;
};

// 通用 Markdown 渲染显示控件，基于 QWebEngineView + markdown-it + highlight.js
class DAGUI_API DAMarkdownView : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString markdown READ markdown WRITE setMarkdown NOTIFY markdownChanged)
public:
    explicit DAMarkdownView(QWidget* parent = nullptr);
    ~DAMarkdownView();

    // 设置并渲染 markdown 文本
    void setMarkdown(const QString& markdown);
    // 获取当前 markdown 源文本
    QString markdown() const;
    // 清空内容
    void clear();
    // 从文件加载并渲染，成功返回 true
    bool loadFile(const QString& filePath);
    // 滚动到顶部
    void scrollToTop();
    // 滚动到底部
    void scrollToBottom();

Q_SIGNALS:
    // markdown 内容变化
    void markdownChanged(const QString& markdown);
    // 用户点击链接（外部 URL 或自定义协议如 da-figure:）
    void linkClicked(const QUrl& url);

private:
    void setupUI();
    void renderMarkdown();

    QWebEngineView* mWebView;
    DAMarkdownWebPage* mPage;
    QString mMarkdown;
    bool mPageLoaded = false;
};

} // namespace DA
