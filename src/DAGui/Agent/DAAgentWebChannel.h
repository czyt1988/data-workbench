#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QWebEngineView>
#include "DAGuiAPI.h"

namespace DA
{
/**
 * @brief WebChannel 桥接对象，由 DAAgentDockWidget 通过 QWebChannel::registerObject 暴露给 JS
 *
 * 本类是 QObject（不是 QWebChannel）。QWebChannel 实例本身由 setupWebChannel() 中
 * `new QWebChannel(this)` 单独创建，并通过 registerObject("chatBridge", m_channel) 注册本对象。
 * 继承 QObject 即可拥有 Q_OBJECT/槽，供 JS 调用。
 */
class DAGUI_API DAAgentWebChannel : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param view 关联的 WebEngineView
     * @param parent 父对象
     */
    explicit DAAgentWebChannel(QWebEngineView* view, QObject* parent = nullptr);

    /**
     * @brief JS 调用：用户选择了答案
     * @param answer 用户选择的答案
     */
    Q_INVOKABLE void onUserSelect(const QString& answer);

    /**
     * @brief JS 调用：用户发送了消息
     * @param text 用户输入的消息文本
     */
    Q_INVOKABLE void onUserMessage(const QString& text);

    /**
     * @brief 追加用户消息到聊天界面
     * @param text 消息文本
     */
    void appendUserMessage(const QString& text);

    /**
     * @brief 追加 Agent 流式 token 到聊天界面
     * @param token 当前 token 文本
     */
    void appendToken(const QString& token);

    /**
     * @brief 完成 Agent 消息（标记消息结束）
     * @param fullText 完整消息文本
     */
    void finalizeAgentMessage(const QString& fullText);

    /**
     * @brief 追加工具调用信息到聊天界面
     * @param toolName 工具名称
     * @param args 工具参数
     */
    void appendToolCall(const QString& toolName, const QJsonObject& args);

    /**
     * @brief 追加工具执行结果到聊天界面
     * @param toolName 工具名称
     * @param result 工具执行结果
     */
    void appendToolResult(const QString& toolName, const QJsonObject& result);

    /**
     * @brief 追加提问信息到聊天界面
     * @param text 问题文本
     * @param options 选项列表
     */
    void appendQuestion(const QString& text, const QStringList& options);

    /**
     * @brief 清空聊天界面
     */
    void clearChat();

    /**
     * @brief 设置忙碌状态
     * @param busy 是否忙碌
     */
    void setBusy(bool busy);

    /**
     * @brief 终止时定稿当前流式消息 + 关闭工具分组
     *
     * 调用 JS flushAgentMessage()（保留已累积的 token 文本）+ closeToolGroup()
     *（将未完成的工具卡片标记为 incomplete），避免半截消息悬挂。
     */
    void onAgentStopped();

Q_SIGNALS:
    /**
     * @brief 用户选择答案信号
     * @param answer 用户选择的答案
     */
    void userAnswerSelected(const QString& answer);

    /**
     * @brief 用户发送消息信号
     * @param text 用户输入的消息文本
     */
    void userMessageSent(const QString& text);

private:
    /**
     * @brief 调用 JS 函数
     * @param funcCall JS 函数调用字符串
     */
    void callJS(const QString& funcCall);

    QWebEngineView* m_view;
};
} // namespace DA
