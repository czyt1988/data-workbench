#pragma once
#include <QWebEnginePage>
#include "DAGuiAPI.h"

namespace DA
{

/**
 * @brief Agent 聊天视图的 QWebEnginePage 子类
 *
 * 聊天页面为单页应用（qrc:///DAAgent/chat.html），agent 回复中的普通超链接
 * （http/https 等）若在 webview 内导航会覆盖整个聊天内容且无法返回，
 * 这里统一拦截：内部导航放行，外部链接交给系统浏览器打开；
 * 新窗口导航（target=_blank/中键）经 createWindow 中转页同样外部打开。
 *
 * da-figure: 自定义协议链接由 chat.js 在 DOM 层拦截（preventDefault +
 * chatBridge.onFigureLink），不会进入导航流程，不受本类影响。
 */
class DAGUI_API DAAgentChatWebPage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit DAAgentChatWebPage(QObject* parent = nullptr);
    ~DAAgentChatWebPage();

protected:
    bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) override;
    // 新窗口导航（target=_blank/中键）中转为系统浏览器打开
    QWebEnginePage* createWindow(WebWindowType type) override;
};

}  // namespace DA
