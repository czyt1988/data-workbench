// DAAgentBridge.h
#pragma once
#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QMap>
#include "DAAgentAPI.h"
#include "DAGlobals.h"

class QTimer;

namespace DA
{
class DAAbstractAgentTool;

/**
 * @brief QProcess 桥接器：管理 agent 子进程的启停、stdin/stdout 读写、JSON Lines 协议解析
 *
 * 子进程通过 stdin/stdout 管道与主进程通信，每条消息一行 JSON（JSON Lines 协议）。
 * 工具调用在 C++ 主进程执行，结果通过 stdin 回传子进程。
 */
class DAAgent_API DAAgentBridge : public QObject
{
    Q_OBJECT
public:
    // 构造函数
    explicit DAAgentBridge(QObject* parent = nullptr);
    // 析构函数，若子进程仍在运行则自动停止
    virtual ~DAAgentBridge() override;

    // 启动 agent 子进程
    void startAgent(const QJsonObject& llmConfig,
                    const QJsonArray& toolSpecs,
                    const QString& systemPrompt,
                    const QString& pythonExePath,
                    const QString& agentScriptPath,
                    int readyTimeoutMs = 60000,
                    int stopTimeoutMs = 5000);
    // 停止 agent 子进程（阻塞，供析构/重启时调用）
    void stopAgent();
    // 请求停止 agent 子进程（非阻塞，供用户主动终止时调用）
    void requestStop();

    // 发送用户消息到 agent 子进程
    void sendMessage(const QString& text);
    // 发送工具执行结果回 agent 子进程
    void sendToolResult(const QString& callId, const QJsonObject& result);
    // 发送用户对问题的回答回 agent 子进程
    void sendUserAnswer(const QString& answer);
    // 下发历史会话消息让 agent 子进程重建 state（不重启子进程切换会话）
    void sendLoadSession(const QString& sessionId, const QJsonArray& messages);

    // 设置 C++ 侧工具映射表，供工具调用时查找执行
    void setTools(const QMap<QString, DAAbstractAgentTool*>& tools);

    // 检查 agent 子进程是否正在运行
    bool isRunning() const;
    // 检查是否处于崩溃恢复流程中
    bool isRecovering() const;
    // 设置崩溃恢复标志
    void setRecovering(bool v);
    // 获取当前会话 ID（供崩溃恢复时 load_session 用）
    QString lastSessionId() const;
    // 崩溃恢复后重发最后一条用户消息
    void resendLastMessage();

Q_SIGNALS:
    /**
     * @brief agent 生成 token 时发射
     * @param token 生成的 token 文本
     */
    void agentToken(const QString& token);
    /**
     * @brief agent 消息生成完成时发射
     * @param fullText 完整消息文本
     */
    void agentMessageComplete(const QString& fullText);
    /**
     * @brief agent 发起工具调用时发射
     * @param toolName 工具名称
     * @param args 工具调用参数 JSON
     */
    void agentToolCall(const QString& toolName, const QJsonObject& args);
    /**
     * @brief 工具执行结果返回时发射，用于 UI 展示
     * @param toolName 工具名称
     * @param result 工具执行结果 JSON
     * @note 无 callId 参数。callId 仅 sendToolResult 回传子进程时需要，
     * UI 展示工具结果不需要 callId。plan-03 的 DAAgentDockWidget::onAgentToolResult
     * 槽签名为 (const QString& toolName, const QJsonObject& result)，Qt PMF connect
     * 要求槽参数是信号参数的类型兼容前缀，若信号带 callId 则位置 2 类型不匹配
     * （信号 QString vs 槽 QJsonObject）→ 编译错误。
     */
    void agentToolResult(const QString& toolName, const QJsonObject& result);
    /**
     * @brief agent 向用户提问时发射
     * @param text 问题文本
     * @param options 可选选项列表
     * @param multiSelect 是否允许多选
     */
    void agentQuestion(const QString& text, const QStringList& options, bool multiSelect);
    /**
     * @brief agent 发生错误时发射
     * @param message 错误信息
     * @param errorType 错误类型（quota_exhausted/auth_error/rate_limit_exhausted/...），空表示未知
     * @param detail 详细错误描述（如原始异常信息），可为空
     */
    void agentError(const QString& message, const QString& errorType = QString(), const QString& detail = QString());
    /**
     * @brief agent 正在重试 LLM 调用时发射（Python 端指数退避期间每次重试发一次）
     * @param attempt 当前重试次数（1-based）
     * @param maxAttempts 最大重试次数
     * @param delayMs 本次退避延迟毫秒数
     * @param errorType 触发重试的错误类型
     * @param errorMessage 触发重试的错误消息
     */
    void agentRetrying(int attempt, int maxAttempts, int delayMs,
                       const QString& errorType, const QString& errorMessage);
    /**
     * @brief agent 就绪时发射
     * @param model 就绪的模型名称
     */
    void agentReady(const QString& model);
    /**
     * @brief agent 忙碌状态变化时发射
     * @param busy 是否忙碌
     */
    void agentBusy(bool busy);
    /**
     * @brief agent 上报 token 使用量时发射（来自独立 usage 消息或 message_end 附带 usage）
     * @param inputTokens 输入 token 数
     * @param outputTokens 输出 token 数
     * @param totalTokens 总 token 数
     * @param source 来源标识："agent"（一轮对话）或 "summary"（摘要生成）
     */
    void agentUsage(int inputTokens, int outputTokens, int totalTokens, const QString& source);
    /**
     * @brief agent 完成会话历史重建时发射（Python 回传 session_loaded 确认）
     * @param sessionId 已加载的会话 ID
     */
    void agentSessionLoaded(const QString& sessionId);
    /**
     * @brief agent 本轮处理完成时发射
     */
    void agentDone();
    /**
     * @brief 崩溃恢复时请求 Module 从 SessionStore 读取会话历史并下发 load_session
     * @param sessionId 需要恢复的会话 ID
     */
    void sessionRestoreRequested(const QString& sessionId);

private Q_SLOTS:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onInactivityTimeout();

private:
    void handleJsonLine(const QJsonObject& msg);
    bool writeJson(const QJsonObject& msg);
    void executeTool(const QString& callId, const QString& toolName, const QJsonObject& args);
    void startInactivityTimer();
    void recoverFromCrash();
    // RAII guard for tool execution watchdog management (defined in .cpp)
    friend struct ToolExecGuard;

    DA_DECLARE_PRIVATE(DAAgentBridge)
};
} // namespace DA
