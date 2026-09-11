#include "DAAgentLinkDispatcher.h"
// Qt 自身日志只写文件不进 UI 队列（注册冲突属开发期诊断）
#include <QDebug>

namespace DA
{

/**
 * @brief 校验 scheme 是否为合法的 da-<kind> 格式
 *
 * kind 以字母开头，后接字母/数字/连字符（与 chat.js 端 /^da-[a-z][a-z0-9-]*:/i 对齐），
 * 如 da-figure、da-data、da-workflow-node。
 * @param scheme 不带冒号的协议名，如 "da-figure"
 * @return 合法返回 true
 */
static bool isValidDaScheme(const QString& scheme)
{
    if (!scheme.startsWith(QStringLiteral("da-"), Qt::CaseInsensitive)) {
        return false;
    }
    const QString kind = scheme.mid(3);
    if (kind.isEmpty()) {
        return false;
    }
    for (int i = 0; i < kind.length(); ++i) {
        const QChar c = kind.at(i);
        const bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const bool digit = (c >= '0' && c <= '9');
        if (i == 0) {
            if (!alpha) {
                return false;
            }
        } else if (!alpha && !digit && c != '-') {
            return false;
        }
    }
    return true;
}

/**
 * @brief 注册协议处理器
 * @param scheme 协议名，形如 "da-figure"（不带冒号），大小写不敏感（内部统一小写存储）
 * @param handler 处理函数，收到的是原始 href（含百分号编码），解码与 payload 解析由各协议自行处理
 */
void DAAgentLinkDispatcher::registerHandler(const QString& scheme, Handler handler)
{
    if (!isValidDaScheme(scheme)) {
        qWarning() << "DAAgentLinkDispatcher::registerHandler: invalid scheme:" << scheme;
        return;
    }
    const QString key = scheme.toLower();
    if (mHandlers.contains(key)) {
        qWarning() << "DAAgentLinkDispatcher::registerHandler: scheme already registered, overriding:" << key;
    }
    mHandlers[ key ] = std::move(handler);
}

/**
 * @brief 注销协议处理器
 * @param scheme 协议名（不带冒号，大小写不敏感）
 */
void DAAgentLinkDispatcher::unregisterHandler(const QString& scheme)
{
    mHandlers.remove(scheme.toLower());
}

/**
 * @brief 获取所有已注册的 scheme 列表
 * @return 已注册的 scheme 列表（小写形式）
 */
QStringList DAAgentLinkDispatcher::registeredSchemes() const
{
    return mHandlers.keys();
}

/**
 * @brief 按 href 前缀分发到对应 handler
 *
 * 提取 href 中首个 ':' 之前的部分作为 scheme，校验 da-<kind> 合法性后查表调用。
 * handler 收到原始 href；未注册或非 da- 前缀返回 false，由调用方提示用户。
 * @param href 超链接 href，形如 da-figure:<name>、da-data:id=<id>
 * @return 命中已注册 handler 返回 true
 */
bool DAAgentLinkDispatcher::dispatch(const QString& href) const
{
    const int colon = href.indexOf(':');
    if (colon <= 0) {
        return false;
    }
    const QString scheme = href.left(colon);
    if (!isValidDaScheme(scheme)) {
        return false;
    }
    auto it = mHandlers.constFind(scheme.toLower());
    if (it == mHandlers.constEnd() || !it.value()) {
        return false;
    }
    it.value()(href);
    return true;
}

}  // end namespace DA
