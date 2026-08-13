#pragma once
#include <QWidget>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QUrl>
#include "DAGuiAPI.h"

class QMenu;
class QAction;

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

protected:
    // 拦截 QWebEngineView 默认右键菜单，改用自定义菜单
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUI();
    void renderMarkdown();
    // 构建右键菜单及其 action（仅构建一次，供 showContextMenu 复用）
    void buildContextMenu();
    // 弹出自定义右键菜单
    void showContextMenu(const QPoint& globalPos);
    // 在只读对话框中展示当前 markdown 源文本
    void onViewMarkdownSource();
    // 将当前 markdown 源文本保存为文件
    void onSaveMarkdownAs();

    QWebEngineView* mWebView;
    DAMarkdownWebPage* mPage;
    QString mMarkdown;
    bool mPageLoaded = false;

    // 右键菜单及其需动态刷新可用状态的 action（构建一次复用）
    QMenu* mContextMenu = nullptr;
    QAction* mCopyAction = nullptr;
    QAction* mViewMarkdownSourceAction = nullptr;
    QAction* mSaveMarkdownAction = nullptr;
};

} // namespace DA
