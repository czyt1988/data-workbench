#ifndef DAAGENTLINKDISPATCHER_H
#define DAAGENTLINKDISPATCHER_H
#include "DAGuiAPI.h"
#include <functional>
#include <QHash>
#include <QString>
#include <QStringList>

namespace DA
{

/**
 * @brief Agent 聊天窗口自定义超链接处理器注册表
 *
 * 统一管理 da-<kind>: 协议超链接的点击分发：持有 scheme → handler 映射，
 * dispatch() 按 href 前缀查表调用。新增协议（如 da-data:）只需注册一个
 * handler，传输链路（chat.js 拦截、WebChannel、信号转发）无需改动。
 *
 * 非 QObject 普通类（无信号槽），非线程安全，仅在主线程使用。
 *
 * @code
 * DAAgentLinkDispatcher dispatcher;
 * dispatcher.registerHandler("da-figure", [this](const QString& href){ handleFigureLink(href); });
 * if (!dispatcher.dispatch(href)) {
 *     // 未注册的协议，提示用户
 * }
 * @endcode
 *
 * @since 2026-09-10
 */
class DAGUI_API DAAgentLinkDispatcher
{
public:
    using Handler = std::function< void(const QString& href) >;

    // 注册协议处理器（scheme 形如 "da-figure"，不带冒号；重名覆盖并告警）
    void registerHandler(const QString& scheme, Handler handler);
    // 注销协议处理器
    void unregisterHandler(const QString& scheme);
    // 获取所有已注册的 scheme 列表
    QStringList registeredSchemes() const;
    // 按 href 的 da-<kind>: 前缀分发到对应 handler，返回是否命中
    bool dispatch(const QString& href) const;

private:
    QHash< QString, Handler > mHandlers;  ///< scheme（小写）→ 处理函数映射
};

}  // end namespace DA
#endif  // DAAGENTLINKDISPATCHER_H
