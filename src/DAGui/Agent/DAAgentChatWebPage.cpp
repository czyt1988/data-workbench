// DAAgentChatWebPage.cpp
#include "DAAgentChatWebPage.h"
#include "DAWebExternalOpenPage.h"
#include <QUrl>
#include <QDesktopServices>

namespace DA
{

DAAgentChatWebPage::DAAgentChatWebPage(QObject* parent) : QWebEnginePage(parent)
{
}

DAAgentChatWebPage::~DAAgentChatWebPage()
{
}

/**
 * @brief 拦截页面导航请求
 *
 * - 内部导航（qrc:/data:/about: 与相对锚点）→ 放行（详见 daIsInternalWebNavigation）
 * - 其余链接 → 主框架时拒绝导航（防止外部页面覆盖聊天内容且无法返回），
 *   标准 web 协议（http/https/mailto: 等）交给系统浏览器打开
 *
 * @param url 目标 URL
 * @param type 导航类型
 * @param isMainFrame 是否为主框架
 * @return true 允许导航，false 拒绝导航
 */
bool DAAgentChatWebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame)
{
    Q_UNUSED(type)
    // 内部资源/页内锚点导航放行
    if (daIsInternalWebNavigation(url)) {
        return true;
    }
    // 外部链接：主框架交给系统浏览器，禁止覆盖聊天页面
    if (isMainFrame) {
        daOpenUrlExternally(url);
        return false;
    }
    return true;
}

/**
 * @brief 新窗口导航（target=_blank / 中键点击）中转
 *
 * 返回中转页把目标 URL 交给系统浏览器打开后自毁；返回 nullptr 会导致
 * 此类导航被静默丢弃（点击无反应）。
 * @param type 窗口类型
 * @return 一次性中转页
 */
QWebEnginePage* DAAgentChatWebPage::createWindow(WebWindowType type)
{
    Q_UNUSED(type)
    return new DAWebExternalOpenPage(this);
}

}  // namespace DA
