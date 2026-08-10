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
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DAAgentBridge(QObject* parent = nullptr);
    /**
     * @brief 析构函数，若子进程仍在运行则自动停止
     */
    ~DAAgentBridge();

    /**
     * @brief 启动 agent 子进程
     * @param llmConfig LLM 配置（base_url、api_key、model）
     * @param toolSpecs 工具规格 JSON 数组（OpenAI function schema）
     * @param systemPrompt 系统提示词
     * @param pythonExePath Python 解释器路径
     * @param agentScriptPath agent 脚本路径
     * @param readyTimeoutMs 等待 ready/booting 心跳的超时（毫秒），默认 60s
     * @param stopTimeoutMs stopAgent 等待进程退出的超时（毫秒），默认 5s
     */
    void startAgent(const QJsonObject& llmConfig,
                    const QJsonArray& toolSpecs,
                    const QString& systemPrompt,
                    const QString& pythonExePath,
                    const QString& agentScriptPath,
                    int readyTimeoutMs = 60000,
                    int stopTimeoutMs = 5000);
    /**
     * @brief 停止 agent 子进程（阻塞，供析构/重启时调用）
     */
    void stopAgent();
    /**
     * @brief 请求停止 agent 子进程（非阻塞，供用户主动终止时调用）
     *
     * 与 stopAgent() 的区别：不调用 waitForFinished 阻塞 UI 线程，
     * 而是用 QTimer 在 m_stopTimeoutMs 后 kill。进程退出后由
     * onProcessFinished 发射 agentBusy(false) 恢复 UI。
     */
    void requestStop();

    /**
     * @brief 发送用户消息到 agent 子进程
     * @param text 用户消息文本
     */
    void sendMessage(const QString& text);
    /**
     * @brief 发送工具执行结果回 agent 子进程
     * @param callId 工具调用 ID
     * @param result 工具执行结果 JSON
     */
    void sendToolResult(const QString& callId, const QJsonObject& result);
    /**
     * @brief 发送用户对问题的回答回 agent 子进程
     * @param answer 用户回答文本
     */
    void sendUserAnswer(const QString& answer);
    /**
     * @brief 下发历史会话消息让 agent 子进程重建 state（不重启子进程切换会话）
     * @param sessionId 会话 ID
     * @param messages 历史消息数组（复用协议消息结构，见总纲 T6）
     * @note 不 emit agentBusy——load_session 不是一轮对话，UI 忙碌态由调用方
     *       （plan-03 switchSession）自行管理。不经 DAAgentInterface 多态，
     *       plan-03 的 switchSession 直接调 m_bridge->sendLoadSession，由其
     *       自行用 isRunning() 守卫 + 懒启动 pending 缓存兜底。
     */
    void sendLoadSession(const QString& sessionId, const QJsonArray& messages);

    /**
     * @brief 设置 C++ 侧工具映射表，供工具调用时查找执行
     * @param tools 工具名 → 工具实现指针的映射
     */
    void setTools(const QMap<QString, DAAbstractAgentTool*>& tools) { m_tools = tools; }

    /**
     * @brief 检查 agent 子进程是否正在运行
     * @return 若子进程正在运行返回 true
     */
    bool isRunning() const { return m_running; }
    /**
     * @brief 检查是否处于崩溃恢复流程中
     * @return 若正在崩溃恢复返回 true
     */
    bool isRecovering() const { return m_recovering; }
    /**
     * @brief 设置崩溃恢复标志
     * @param v 是否处于崩溃恢复
     */
    void setRecovering(bool v) { m_recovering = v; }
    /**
     * @brief 获取当前会话 ID（供崩溃恢复时 load_session 用）
     * @return 当前会话 ID
     */
    QString lastSessionId() const { return m_lastSessionId; }
    /**
     * @brief 崩溃恢复后重发最后一条用户消息
     */
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

    QProcess* m_process = nullptr;
    bool m_running = false;
    QByteArray m_stdoutBuffer;  // 累积不完整的行
    QMap<QString, DAAbstractAgentTool*> m_tools;  // tool name → tool impl
    QString m_pythonExePath;
    QString m_agentScriptPath;
    QTimer* m_readyTimer = nullptr;  // agent 启动后等待 ready 消息的超时计时器,防止子进程卡死时 UI 干等
    int m_readyTimeoutMs = 60000;      // ready/booting 等待超时(毫秒),由 startAgent 参数注入
    int m_stopTimeoutMs  = 5000;       // stopAgent 等待进程退出超时(毫秒),由 startAgent 参数注入
    bool m_userRequestedStop = false;  // 用户主动终止标志,抑制 onProcessFinished 中的异常退出错误
    QTimer* m_stopTimer = nullptr;     // requestStop 的非阻塞 kill 计时器
    // —— 无活动看门狗 ——
    QTimer* m_inactivityTimer = nullptr;  // 无活动超时计时器
    int m_inactivityTimeoutMs = 240000;    // 默认 4 分钟
    bool m_toolExecuting = false;           // 工具执行期间暂停看门狗
    bool m_turnActive = false;              // 对话进行中标志（sendMessage 置 true，done/error 置 false）
    QString m_lastUserMessage;              // 记录最后用户消息（崩溃恢复时重发）
    // —— 子进程崩溃恢复 ——
    int m_restartCount = 0;                 // 当前重启次数
    int m_maxRestarts = 3;                   // 最大重启次数
    QString m_lastSessionId;                 // 当前会话 ID（sendLoadSession 赋值，startAgent 清空，崩溃恢复时 load_session 用）
    bool m_recovering = false;               // 是否处于崩溃恢复流程中（agentReady 槽据此判断是否走恢复路径）
    QJsonObject m_savedLlmConfig;            // 启动参数缓存（崩溃恢复时复用）
    QJsonArray m_savedToolSpecs;
    QString m_savedSystemPrompt;
};
} // namespace DA
