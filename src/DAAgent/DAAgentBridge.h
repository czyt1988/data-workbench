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
     * @brief 设置 C++ 侧工具映射表，供工具调用时查找执行
     * @param tools 工具名 → 工具实现指针的映射
     */
    void setTools(const QMap<QString, DAAbstractAgentTool*>& tools) { m_tools = tools; }

    /**
     * @brief 检查 agent 子进程是否正在运行
     * @return 若子进程正在运行返回 true
     */
    bool isRunning() const { return m_running; }

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
     */
    void agentQuestion(const QString& text, const QStringList& options);
    /**
     * @brief agent 发生错误时发射
     * @param message 错误信息
     */
    void agentError(const QString& message);
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
     * @brief agent 本轮处理完成时发射
     */
    void agentDone();

private Q_SLOTS:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void handleJsonLine(const QJsonObject& msg);
    bool writeJson(const QJsonObject& msg);
    void executeTool(const QString& callId, const QString& toolName, const QJsonObject& args);

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
};
} // namespace DA
