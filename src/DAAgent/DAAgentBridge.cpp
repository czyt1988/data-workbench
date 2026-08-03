// DAAgentBridge.cpp
#include "DAAgentBridge.h"
#include "DAAbstractAgentTool.h"
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include "DALogCategory.h"

namespace DA
{

DAAgentBridge::DAAgentBridge(QObject* parent) : QObject(parent) {}

DAAgentBridge::~DAAgentBridge()
{
    stopAgent();
}

void DAAgentBridge::startAgent(const QJsonObject& llmConfig,
                               const QJsonArray& toolSpecs,
                               const QString& systemPrompt,
                               const QString& pythonExePath,
                               const QString& agentScriptPath)
{
    // 0. 清理上一次的进程——崩溃重启时旧 QProcess 仍持有资源，直接 new 会泄漏
    //    死进程对象。先 disconnect 防止旧进程的 pending 信号在 deleteLater 之后
    //    投递到新逻辑上造成错乱。
    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_pythonExePath   = pythonExePath;
    m_agentScriptPath = agentScriptPath;

    // 1. 启动 QProcess
    m_process = new QProcess(this);
    m_process->setProgram(pythonExePath);
    m_process->setArguments({agentScriptPath});

    // 连接信号
    connect(m_process, &QProcess::readyReadStandardOutput, this, &DAAgentBridge::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &DAAgentBridge::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &DAAgentBridge::onProcessFinished);

    m_process->start();

    // 等待进程真正启动后再发送 init——QProcess 是异步的，start() 返回后
    // state() 仍为 Starting（非 Running），writeJson 的 Running 守卫会拒绝
    // 写入，导致 init 永不发送、子进程在 stdin 读取上阻塞挂起。
    if (!m_process->waitForStarted(5000)) {
        emit agentError(tr("Agent 进程启动超时"));
        return;
    }

    // 2. 发送 init 消息（此时 state() 为 Running，writeJson 守卫通过）
    QJsonObject initMsg;
    initMsg["type"]          = "init";
    initMsg["config"]        = llmConfig;
    initMsg["tools"]         = toolSpecs;
    initMsg["system_prompt"] = systemPrompt;
    writeJson(initMsg);
    m_running = true;
}

void DAAgentBridge::stopAgent()
{
    if (m_running && m_process) {
        writeJson(QJsonObject{{"type", "stop"}});
        m_process->waitForFinished(5000);  // 5秒超时
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
        }
    }
    m_running = false;
}

bool DAAgentBridge::writeJson(const QJsonObject& obj)
{
    if (!m_process || m_process->state() != QProcess::Running) {
        return false;
    }
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    qint64 written  = m_process->write(data);
    if (written != data.size()) {
        // stdin 写入失败——管道可能已关闭
        emit agentError(tr("写入 agent 子进程 stdin 失败"));
        return false;
    }
    return true;
}

void DAAgentBridge::sendMessage(const QString& text)
{
    // 用户发消息后 agent 进入"思考中"状态——此处统一发射 agentBusy(true)。
    emit agentBusy(true);
    QJsonObject msg;
    msg["type"]    = "user_msg";
    msg["content"] = text;
    writeJson(msg);
}

void DAAgentBridge::sendToolResult(const QString& callId, const QJsonObject& result)
{
    QJsonObject msg;
    msg["type"]    = "tool_result";
    msg["call_id"] = callId;
    msg["result"]  = result;
    writeJson(msg);
}

void DAAgentBridge::sendUserAnswer(const QString& answer)
{
    QJsonObject msg;
    msg["type"]   = "user_answer";
    msg["answer"] = answer;
    writeJson(msg);
}

void DAAgentBridge::onReadyReadStandardOutput()
{
    // 累积数据到缓冲区
    m_stdoutBuffer += m_process->readAllStandardOutput();

    // 按行解析 JSON Lines
    while (true) {
        int idx = m_stdoutBuffer.indexOf('\n');
        if (idx < 0) break;  // 不完整行，等更多数据

        QByteArray lineData = m_stdoutBuffer.left(idx);
        m_stdoutBuffer      = m_stdoutBuffer.mid(idx + 1);

        // plan-02 的 agent_runner.py 在 Windows 下以文本模式 sys.stdout.write(line + "\n")
        // 输出，实际字节为 "\r\n"。indexOf('\n') 会留下结尾的 '\r'，需裁掉，否则
        // QJsonDocument::fromJson 解析失败（尾部非法空白/控制字符）。
        if (lineData.endsWith('\r')) {
            lineData.chop(1);
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(lineData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            // 不可解析的行不能静默丢弃——记录到日志便于排查协议问题
            daWarning << tr("Failed to parse JSON line from agent stdout: %1, error: %2")
                             .arg(QString::fromUtf8(lineData), parseError.errorString());
            continue;  // 跳过此行，继续处理后续
        }
        if (doc.isObject()) {
            handleJsonLine(doc.object());
        }
    }
}

void DAAgentBridge::handleJsonLine(const QJsonObject& msg)
{
    QString type = msg["type"].toString();
    if (type == "ready") {
        emit agentReady(msg["model"].toString());
    } else if (type == "token") {
        emit agentToken(msg["content"].toString());
    } else if (type == "message_end") {
        emit agentMessageComplete(msg["content"].toString());
    } else if (type == "tool_call") {
        emit agentBusy(true);
        emit agentToolCall(msg["tool"].toString(), msg["arguments"].toObject());
        // 通过 QTimer::singleShot(0) 把工具执行投递回主线程事件循环，
        // 让当前 onReadyReadStandardOutput 尽快返回，避免在读取回调里
        // 长时间阻塞 stdout 管道（管道阻塞会导致子进程 write 阻塞/死锁）。
        QString callId   = msg["call_id"].toString();
        QString toolName = msg["tool"].toString();
        QJsonObject args = msg["arguments"].toObject();
        QTimer::singleShot(0, this, [this, callId, toolName, args]() {
            executeTool(callId, toolName, args);
        });
    } else if (type == "question") {
        emit agentQuestion(msg["text"].toString(), msg["options"].toVariant().toStringList());
    } else if (type == "error") {
        emit agentError(msg["message"].toString());
    } else if (type == "done") {
        emit agentBusy(false);
        emit agentDone();
    }
}

void DAAgentBridge::executeTool(const QString& callId,
                                const QString& toolName,
                                const QJsonObject& args)
{
    QJsonObject result;

    // 1. 查找工具（Bridge 持有 m_tools，由 DAAgentModule::registerTool → setTools 填充）
    auto it = m_tools.find(toolName);
    if (it == m_tools.end() || it.value() == nullptr) {
        result["error"]  = QString("Unknown tool: %1").arg(toolName);
        result["success"] = false;
        sendToolResult(callId, result);
        emit agentToolResult(toolName, result);  // 同步推送到 UI 显示
        return;
    }

    // 2. 执行工具——必须用 try/catch 兜底，避免工具抛异常导致 Bridge 崩溃，
    //    进而使子进程因收不到 tool_result 而永久挂起
    try {
        result = it.value()->execute(args);
        // Don't overwrite error responses — tools return {success:false, error:"..."}
        // via errorResponse(). Only set success=true if the tool didn't set it.
        if (!result.contains("success")) {
            result["success"] = true;
        }
    } catch (const std::exception& e) {
        result["success"] = false;
        result["error"]   = QString("Tool execution failed: %1").arg(e.what());
    } catch (...) {
        result["success"] = false;
        result["error"]   = "Tool execution failed: unknown error";
    }

    // 3. 把结果回传子进程（让 agent_runner 继续推理）
    sendToolResult(callId, result);

    // 4. 同时发射信号，让聊天 UI 在对话流中展示工具调用结果
    emit agentToolResult(toolName, result);
}

void DAAgentBridge::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // 排空残留缓冲——若最后一个 stdout chunk 与 finished 信号几乎同时到达，
    // onReadyReadStandardOutput 可能留下未以 '\n' 结尾的完整行；不在此排空会丢失
    // 这部分消息（如最后的 message_end / done）。
    if (!m_stdoutBuffer.isEmpty()) {
        QByteArray lastLine = m_stdoutBuffer;
        m_stdoutBuffer.clear();
        if (lastLine.endsWith('\r')) {
            lastLine.chop(1);
        }
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(lastLine, &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            handleJsonLine(doc.object());
        } else {
            daWarning << tr("Failed to parse trailing JSON line from agent stdout: %1, error: %2")
                             .arg(QString::fromUtf8(lastLine), parseError.errorString());
        }
    }

    m_running = false;
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        // 子进程崩溃/异常退出——转发到 UI 聊天面板显示
        emit agentError(tr("Agent 进程异常退出，代码: %1").arg(exitCode));
    }
    // 恢复 UI 为可输入状态
    emit agentBusy(false);
}

void DAAgentBridge::onReadyReadStandardError()
{
    // 读取 stderr 用于调试——Python traceback、langchain 警告等
    // 注意：不要把 stderr 原样转发到 stdout 协议解析（会污染 JSON Lines 流）
    QByteArray data = m_process->readAllStandardError();
    daDebug << "Agent stderr:" << QString::fromUtf8(data);
    // 仅把关键错误转发到 UI，避免刷屏
    if (data.contains("Traceback") || data.contains("Error")) {
        emit agentError(QString::fromUtf8(data).trimmed());
    }
}

} // namespace DA
