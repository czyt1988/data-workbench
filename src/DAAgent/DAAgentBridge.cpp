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
                               const QString& agentScriptPath,
                               int readyTimeoutMs,
                               int stopTimeoutMs)
{
    m_readyTimeoutMs = readyTimeoutMs;
    m_stopTimeoutMs  = stopTimeoutMs;
    // 清理上一次的 ready 超时计时器(若存在)
    if (m_readyTimer) {
        m_readyTimer->stop();
        m_readyTimer->deleteLater();
        m_readyTimer = nullptr;
    }
    // 清理上一次的 stop kill 计时器(若存在)——避免其 lambda 误杀重启后的新进程
    if (m_stopTimer) {
        m_stopTimer->stop();
        m_stopTimer->deleteLater();
        m_stopTimer = nullptr;
    }

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

    // 记录 agent 启动信息(不打印 api_key 明文)——用于诊断启动失败/退出码异常
    daDebug << "Starting agent: python=" << pythonExePath
            << " script=" << agentScriptPath
            << " base_url=" << llmConfig.value("base_url").toString()
            << " model=" << llmConfig.value("model").toString();

    // 2. 发送 init 消息（此时 state() 为 Running，writeJson 守卫通过）
    QJsonObject initMsg;
    initMsg["type"]          = "init";
    initMsg["config"]        = llmConfig;
    initMsg["tools"]         = toolSpecs;
    initMsg["system_prompt"] = systemPrompt;
    writeJson(initMsg);
    m_running = true;

    // 启动 ready 超时检测——若子进程在 m_readyTimeoutMs 内未发送 ready 或
    // booting 心跳,视为初始化失败(常见原因:Python 导入失败、stdin 读取卡死、
    // LLM 配置错误)。booting 心跳会在 handleJsonLine 中重置本计时器,因此
    // langchain 冷启动导入(~16s)只要持续发出 booting 就不会被误杀。
    // 必须主动通知用户并杀进程，避免 UI 干等无响应。
    m_readyTimer = new QTimer(this);
    m_readyTimer->setSingleShot(true);
    connect(m_readyTimer, &QTimer::timeout, this, [this]() {
        if (m_running) {
            emit agentError(tr("Agent 子进程启动后 %1 毫秒内未就绪，初始化可能失败，请查看日志排查")
                                .arg(m_readyTimeoutMs));
            if (m_process && m_process->state() != QProcess::NotRunning) {
                m_process->kill();
            }
            m_running = false;
        }
    });
    m_readyTimer->start(m_readyTimeoutMs);
}

void DAAgentBridge::stopAgent()
{
    // 停止 ready 超时计时器(若还在等待)
    if (m_readyTimer) {
        m_readyTimer->stop();
        m_readyTimer->deleteLater();
        m_readyTimer = nullptr;
    }
    if (m_running && m_process) {
        writeJson(QJsonObject{{"type", "stop"}});
        m_process->waitForFinished(m_stopTimeoutMs);  // 可配超时(默认 5s)
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
        }
    }
    m_running = false;
}

void DAAgentBridge::requestStop()
{
    // 停止 ready 超时计时器(若还在等待)
    if (m_readyTimer) {
        m_readyTimer->stop();
        m_readyTimer->deleteLater();
        m_readyTimer = nullptr;
    }
    // 停止已有的 stop 计时器(防止重复调用)
    if (m_stopTimer) {
        m_stopTimer->stop();
        m_stopTimer->deleteLater();
        m_stopTimer = nullptr;
    }
    if (m_running && m_process) {
        // 标记为用户主动终止——onProcessFinished 据此抑制异常退出错误
        m_userRequestedStop = true;
        writeJson(QJsonObject{{"type", "stop"}});
        // 非阻塞: 不调用 waitForFinished(会冻结 UI 最多 m_stopTimeoutMs),
        // 改用 QTimer 在超时后 kill。进程退出后由 onProcessFinished
        // 发射 agentBusy(false) 恢复 UI。
        m_stopTimer = new QTimer(this);
        m_stopTimer->setSingleShot(true);
        connect(m_stopTimer, &QTimer::timeout, this, [this]() {
            if (m_process && m_process->state() != QProcess::NotRunning) {
                m_process->kill();
            }
            m_stopTimer->deleteLater();
            m_stopTimer = nullptr;
        });
        m_stopTimer->start(m_stopTimeoutMs);
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

void DAAgentBridge::sendLoadSession(const QString& sessionId, const QJsonArray& messages)
{
    // 与 sendMessage 的区别：不 emit agentBusy——load_session 不是一轮对话，
    // UI 忙碌态由调用方（plan-03 switchSession）自行管理。writeJson 内部
    // 守卫 state()==Running，进程未运行时静默返回 false 不 emit agentError；
    // 调用方须自行用 isRunning() 守卫 + 懒启动 pending 缓存兜底。
    QJsonObject obj;
    obj["type"]       = "load_session";
    obj["session_id"] = sessionId;
    obj["messages"]   = messages;
    writeJson(obj);
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
    if (type == "booting") {
        // 子进程已启动、正在导入重模块(langchain_openai 冷启动 ~16s)。
        // 收到 booting 心跳 → 重置 ready 超时计时器,避免导入期间被误杀。
        // 不 emit 任何信号(booting 非 ready,UI 无需感知)。
        if (m_readyTimer) {
            m_readyTimer->start(m_readyTimeoutMs);
        }
    } else if (type == "ready") {
        // 收到 ready 消息——停止 ready 超时计时器
        if (m_readyTimer) {
            m_readyTimer->stop();
            m_readyTimer->deleteLater();
            m_readyTimer = nullptr;
        }
        emit agentReady(msg["model"].toString());
    } else if (type == "token") {
        emit agentToken(msg["content"].toString());
    } else if (type == "message_end") {
        emit agentMessageComplete(msg["content"].toString());
        // plan-01 的 send_message_end(content, usage) 附带本轮 LLM usage_metadata，
        // 作为 token 统计的权威锚点（与独立 usage 消息互补）。
        if (msg.contains("usage")) {
            QJsonObject u = msg.value("usage").toObject();
            emit agentUsage(u.value("input_tokens").toInt(0),
                            u.value("output_tokens").toInt(0),
                            u.value("total_tokens").toInt(0),
                            "agent");
        }
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
        emit agentQuestion(msg["text"].toString(),
                           msg["options"].toVariant().toStringList(),
                           msg.value("multi_select").toBool(false));
    } else if (type == "error") {
        emit agentError(msg["message"].toString());
    } else if (type == "usage") {
        // plan-01 独立 send_usage 消息（如摘要生成的 usage_metadata）。
        // 字段缺失/类型错误时 toInt(0) 兜底，发 0 不崩溃。
        int inT  = msg.value("input_tokens").toInt(0);
        int outT = msg.value("output_tokens").toInt(0);
        int tot  = msg.value("total_tokens").toInt(0);
        QString src = msg.value("source").toString("agent");
        emit agentUsage(inT, outT, tot, src);
    } else if (type == "session_loaded") {
        // plan-01 收到 load_session 重建 state 后回传的确认消息。
        // 本计划只透传 session_id，session_id 与请求是否相符由 plan-03 switchSession 校验。
        QString sid = msg.value("session_id").toString();
        emit agentSessionLoaded(sid);
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
    // 停止 ready 超时计时器(进程已退出，无需再等)
    if (m_readyTimer) {
        m_readyTimer->stop();
        m_readyTimer->deleteLater();
        m_readyTimer = nullptr;
    }

    // 记录进程退出状态——即使 agent 静默崩溃(无 stderr 输出)，
    // 日志也有记录，便于诊断退出码含义(如 62097 等异常退出码)
    daDebug << "Agent process finished: exitCode=" << exitCode
            << " exitStatus=" << (exitStatus == QProcess::NormalExit ? "NormalExit" : "CrashExit");

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
    if (m_userRequestedStop) {
        // 用户主动终止——不报异常退出错误，仅记日志
        m_userRequestedStop = false;
        daDebug << "Agent process stopped by user request";
    } else if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        // 子进程崩溃/异常退出——转发到 UI 聊天面板显示
        emit agentError(tr("Agent 进程异常退出，代码: %1").arg(exitCode));
    }
    // 停止 stop 计时器(进程已退出)
    if (m_stopTimer) {
        m_stopTimer->stop();
        m_stopTimer->deleteLater();
        m_stopTimer = nullptr;
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
