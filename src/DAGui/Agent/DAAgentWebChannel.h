#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QVector>
#include <QVariantMap>
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
     * @brief JS 调用：用户点击了绘图引用超链接（da-figure: 协议）
     * @param href 超链接 href，形如 da-figure:&lt;figure_name&gt; 或 da-figure:id=&lt;uuid&gt;
     */
    Q_INVOKABLE void onFigureLink(const QString& href);

    /**
     * @brief JS 调用：web 侧初始化完成（chat.js init() 建立 QWebChannel 后回调）
     *
     * 握手信号：通知 C++ web 已就绪可接收状态推送。C++ 收到后回推
     * setI18nLabels/setBusy/setModel/setTokenStats，缓解 webview 异步加载期间的
     * JS-ready 竞态（agent 信号若在 chat.html 加载完成前触发会丢失）。
     */
    Q_INVOKABLE void onReady();

    /**
     * @brief JS 调用：用户在 web 输入区点击 Stop 按钮（忙碌态）
     *
     * web 输入区按钮 Send/Stop 切换后，Stop 走此通道直达 C++ 终止流程，
     * 替代旧原生 m_sendButton 分流。
     */
    Q_INVOKABLE void onStopRequested();

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
     * @param multiSelect 是否允许多选
     */
    void appendQuestion(const QString& text, const QStringList& options, bool multiSelect);

    /**
     * @brief 显示重试状态条（LLM 调用重试期间）
     * @param attempt 当前重试次数（1-based）
     * @param maxAttempts 最大重试次数
     * @param delayMs 本次退避延迟毫秒数
     * @param errorType 触发重试的错误类型
     * @param errorMessage 触发重试的错误消息
     */
    void showRetryStatus(int attempt, int maxAttempts, int delayMs,
                         const QString& errorType, const QString& errorMessage);

    /**
     * @brief 追加错误信息到聊天界面（独立错误卡片）
     * @param message 错误消息（经 mapErrorMessage 映射后的用户文案）
     * @param errorType 错误类型（用于 JS 端样式/图标选择）
     */
    void appendError(const QString& message, const QString& errorType);

    /**
     * @brief 清空聊天界面
     */
    void clearChat();

    /**
     * @brief 批量重放历史会话记录到聊天界面（plan-04）
     *
     * 遍历 plan-03 原始 JSONL 记录，把连续的 `assistant(tool_call)` + 紧随的 `tool_result`
     * 合并为单个 UI 事件（type:user/assistant/tool/question/usage），序列化为 JSON 数组
     * 调 `callJS("loadHistory(" + jsonArrayStr + ")")`。JS 端 loadHistory(events)
     * 据合并后 type 分发渲染。MAJOR2: 配对在 C++ 完成，JS 不再读 _toolName/_toolArgs。
     *
     * @param records plan-03 原始 JSONL 记录（QVector<QJsonObject>）
     */
    void loadHistory(const QVector<QJsonObject>& records);

    /**
     * @brief 设置忙碌状态（busy 打包：JS 解释按钮 Send/Stop 切换+输入禁用+状态文案）
     * @param busy 是否忙碌
     */
    void setBusy(bool busy);

    /**
     * @brief 设置停止过渡态（onStopClicked 后、onAgentBusy(false)/Ready 前）
     *
     * JS 侧禁用按钮防重复点击 + 状态文案置 Stopping...。
     */
    void setStopping();

    /**
     * @brief 设置当前模型名标签（中）
     * @param label 已由 C++ 格式化为 "Model: &lt;name&gt;" 的翻译串，JS 仅显示（CSS ellipsis 截断）
     */
    void setModel(const QString& label);

    /**
     * @brief 设置 token 计量（右）+ 缓存明细供 popover
     * @param label 已由 C++ 格式化的 "tokens: N / window" 串（streaming_estimate 带 ~ 前缀）
     * @param inputTokens 输入 token
     * @param outputTokens 输出 token
     * @param totalTokens 总 token
     * @param contextWindow 上下文窗口大小
     * @param source 来源（tiktoken / usage_metadata / streaming_estimate）
     */
    void setTokenStats(const QString& label, int inputTokens, int outputTokens,
                       int totalTokens, int contextWindow, const QString& source);

    /**
     * @brief 复位 token 计量到无活跃会话初始态（新会话/清空时）
     */
    void resetTokenStats();

    /**
     * @brief 注入静态 UI 标签（握手时 C++ 一次性推送，C++ 仍是唯一 i18n 拥有者）
     * @param labels QVariantMap，键见 chat.js setI18nLabels 注释
     */
    void setI18nLabels(const QVariantMap& labels);

    /**
     * @brief 聚焦 web 输入框（新会话/切换会话后）
     */
    void focusInput();

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

    /**
     * @brief 用户点击绘图引用超链接信号
     * @param href 超链接 href，形如 da-figure:&lt;figure_name&gt; 或 da-figure:id=&lt;uuid&gt;
     */
    void figureLinkRequested(const QString& href);

    /**
     * @brief web 侧就绪信号（chat.js init() 握手，C++ 收到后 flush 当前态）
     */
    void webReady();

    /**
     * @brief 用户在 web 输入区点 Stop 按钮信号（直达 C++ 终止流程）
     */
    void stopRequested();

private:
    /**
     * @brief 调用 JS 函数
     * @param funcCall JS 函数调用字符串
     */
    void callJS(const QString& funcCall);

    QWebEngineView* m_view;
};
} // namespace DA
