#pragma once
#include <QWebEnginePage>
#include <QUrl>
#include <QDesktopServices>

namespace DA
{

/**
 * @brief 判断 URL 是否为 webview 内部导航（允许在当前页面内完成的加载）
 *
 * 内部导航包括：qrc:/data:/about: 资源加载，以及无 scheme 的相对 URL
 * （含纯锚点 #section 的页内跳转）。除此之外的 http/https/mailto: 及
 * 自定义协议均属外部链接，应交由系统浏览器打开而非覆盖 webview 内容。
 * @param url 待判断的 URL
 * @return true 表示内部导航
 */
inline bool daIsInternalWebNavigation(const QUrl& url)
{
    const QString scheme = url.scheme();
    return scheme == "qrc" || scheme == "data" || scheme == "about" || scheme.isEmpty();
}

/**
 * @brief 把外部链接交给系统默认程序打开（标准 web 协议白名单）
 *
 * 仅 http/https/ftp/mailto/file 等标准协议走系统浏览器/邮件客户端；
 * 自定义协议（da-figure: 等）交给系统会弹"选择打开方式"对话框，返回
 * false 交由调用方走信号处理层（如 linkClicked）。
 * @param url 外部链接
 * @return true 表示已交给系统打开，false 表示未处理（自定义协议）
 */
inline bool daOpenUrlExternally(const QUrl& url)
{
    const QString scheme = url.scheme();
    if (scheme == "http" || scheme == "https" || scheme == "ftp" || scheme == "mailto"
        || scheme == "file") {
        return QDesktopServices::openUrl(url);
    }
    return false;
}

/**
 * @brief 超链接外部打开中转页（QWebEnginePage::createWindow 专用）
 *
 * WebEngine 对 target=_blank、中键点击等"新窗口"导航会先调用
 * QWebEnginePage::createWindow() 请求一个新 page 承接导航；返回 nullptr 时
 * 该导航被静默丢弃（点击无反应）。本类作为一次性中转页：收到导航请求时
 * 交给系统默认浏览器打开并自毁，使 webview 内所有超链接一律跳出而非
 * 覆盖当前页面内容。
 *
 * header-only 实现：无 Q_OBJECT（仅虚函数重写，不需要信号槽），可被多个
 * cpp 直接 include 实例化，无链接符号问题。
 */
class DAWebExternalOpenPage : public QWebEnginePage
{
public:
    explicit DAWebExternalOpenPage(QObject* parent = nullptr) : QWebEnginePage(parent)
    {
    }

protected:
    /**
     * 拦截新窗口导航：标准协议经系统浏览器打开后拒绝导航并自毁，
     * 自定义协议（da-figure: 等）不弹系统"选择打开方式"对话框
     */
    bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) override
    {
        Q_UNUSED(type);
        Q_UNUSED(isMainFrame);
        daOpenUrlExternally(url);
        deleteLater();  // 一次性中转：导航被拒后自毁（parent 兜底防泄漏）
        return false;
    }
};

}  // namespace DA
