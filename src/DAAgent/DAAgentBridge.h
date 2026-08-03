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
    explicit DAAgentBridge(QObject* parent = nullptr);
    ~DAAgentBridge();

    // 启动 agent 子进程
    void startAgent(const QJsonObject& llmConfig,
                    const QJsonArray& toolSpecs,
                    const QString& systemPrompt,
                    const QString& pythonExePath,
                    const QString& agentScriptPath);
    void stopAgent();

    // 发送消息
    void sendMessage(const QString& text);
    void sendToolResult(const QString& callId, const QJsonObject& result);
    void sendUserAnswer(const QString& answer);

    // 注册工具（供 DAAgentModule 调用）
    void setTools(const QMap<QString, DAAbstractAgentTool*>& tools) { m_tools = tools; }

    bool isRunning() const { return m_running; }

signals:
    void agentToken(const QString& token);
    void agentMessageComplete(const QString& fullText);
    void agentToolCall(const QString& toolName, const QJsonObject& args);
    // 注意：无 callId 参数。callId 仅 sendToolResult 回传子进程时需要，
    // UI 展示工具结果不需要 callId。plan-03 的 DAAgentDockWidget::onAgentToolResult
    // 槽签名为 (const QString& toolName, const QJsonObject& result)，Qt PMF connect
    // 要求槽参数是信号参数的类型兼容前缀，若信号带 callId 则位置 2 类型不匹配
    // （信号 QString vs 槽 QJsonObject）→ 编译错误。
    void agentToolResult(const QString& toolName, const QJsonObject& result);
    void agentQuestion(const QString& text, const QStringList& options);
    void agentError(const QString& message);
    void agentReady(const QString& model);
    void agentBusy(bool busy);
    void agentDone();

private slots:
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
};
} // namespace DA
