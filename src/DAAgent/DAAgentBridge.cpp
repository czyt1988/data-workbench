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

// ===========================================================================
// PrivateData
// ===========================================================================
class DAAgentBridge::PrivateData
{
    DA_DECLARE_PUBLIC(DAAgentBridge)
public:
    explicit PrivateData(DAAgentBridge* p);

    QProcess* mProcess = nullptr;
    bool mRunning = false;
    bool mStopped = false;  ///< 防止 stopAgent() 重复执行（closeEvent + 析构双重调用）
    QByteArray mStdoutBuffer;  ///< 累积不完整的行
    QMap<QString, DAAbstractAgentTool*> mTools;  ///< tool name → tool impl
    QString mPythonExePath;
    QString mAgentScriptPath;
    QTimer* mReadyTimer = nullptr;  ///< agent 启动后等待 ready 消息的超时计时器
    int mReadyTimeoutMs = 60000;     ///< ready/booting 等待超时(毫秒)
    int mStopTimeoutMs  = 5000;      ///< stopAgent 等待进程退出超时(毫秒)
    bool mUserRequestedStop = false;  ///< 用户主动终止标志
    QTimer* mStopTimer = nullptr;     ///< requestStop 的非阻塞 kill 计时器
    QTimer* mInactivityTimer = nullptr;  ///< 无活动超时计时器
    int mInactivityTimeoutMs = 240000;    ///< 默认 4 分钟
    bool mToolExecuting = false;           ///< 工具执行期间暂停看门狗
    bool mTurnActive = false;              ///< 对话进行中标志
    bool mWaitingUserAnswer = false;       ///< 等待用户回答问题标志
    QString mLastUserMessage;              ///< 记录最后用户消息（崩溃恢复时重发）
    int mRestartCount = 0;                 ///< 当前重启次数
    int mMaxRestarts = 3;                   ///< 最大重启次数
    QString mLastSessionId;                 ///< 当前会话 ID
    bool mRecovering = false;               ///< 是否处于崩溃恢复流程中
    QJsonObject mSavedLlmConfig;            ///< 启动参数缓存（崩溃恢复时复用）
    QJsonArray mSavedToolSpecs;
    QString mSavedSystemPrompt;
};

DAAgentBridge::PrivateData::PrivateData(DAAgentBridge* p) : q_ptr(p)
{
}

// ===========================================================================
// ctor / dtor
// ===========================================================================

/**
 * @brief 构造函数
 * @param parent 父对象
 */
DAAgentBridge::DAAgentBridge(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->mInactivityTimer = new QTimer(this);
    d->mInactivityTimer->setSingleShot(true);
    connect(d->mInactivityTimer, &QTimer::timeout, this, &DAAgentBridge::onInactivityTimeout);
}

/**
 * @brief 析构函数，若子进程仍在运行则自动停止
 */
DAAgentBridge::~DAAgentBridge()
{
    stopAgent();
}

// ===========================================================================
// 公共方法
// ===========================================================================

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
void DAAgentBridge::startAgent(const QJsonObject& llmConfig,
                               const QJsonArray& toolSpecs,
                               const QString& systemPrompt,
                               const QString& pythonExePath,
                               const QString& agentScriptPath,
                               int readyTimeoutMs,
                               int stopTimeoutMs)
{
    DA_D(d);
    // 保存启动参数（崩溃恢复时复用）
    d->mSavedLlmConfig   = llmConfig;
    d->mSavedToolSpecs    = toolSpecs;
    d->mSavedSystemPrompt = systemPrompt;
    // 从 config 读取看门狗和重启参数（plan-05 在 getLLMConfig 中添加这些 key）
    d->mInactivityTimeoutMs = llmConfig.value("inactivity_timeout_sec").toInt(240) * 1000;
    d->mMaxRestarts         = llmConfig.value("max_subprocess_restarts").toInt(3);
    // 全新启动时重置崩溃恢复状态（recoverFromCrash 调用时 m_recovering=true，跳过重置）
    if (!d->mRecovering) {
        d->mRestartCount = 0;
        d->mLastSessionId.clear();  // 全新启动不应记住旧会话
        d->mUserRequestedStop = false;  // 清除可能残留的主动停止标志
    }

    d->mReadyTimeoutMs = readyTimeoutMs;
    d->mStopTimeoutMs  = stopTimeoutMs;
    // 清理上一次的 ready 超时计时器(若存在)
    if (d->mReadyTimer) {
        d->mReadyTimer->stop();
        d->mReadyTimer->deleteLater();
        d->mReadyTimer = nullptr;
    }
    // 清理上一次的 stop kill 计时器(若存在)——避免其 lambda 误杀重启后的新进程
    if (d->mStopTimer) {
        d->mStopTimer->stop();
        d->mStopTimer->deleteLater();
        d->mStopTimer = nullptr;
    }

    // 0. 清理上一次的进程——崩溃重启时旧 QProcess 仍持有资源，直接 new 会泄漏
    //    死进程对象。先 disconnect 防止旧进程的 pending 信号在 deleteLater 之后
    //    投递到新逻辑上造成错乱。
    if (d->mProcess) {
        d->mProcess->disconnect();
        if (d->mProcess->state() != QProcess::NotRunning) {
            d->mProcess->kill();
            d->mProcess->waitForFinished(3000);
        }
        d->mProcess->deleteLater();
        d->mProcess = nullptr;
    }

    d->mPythonExePath   = pythonExePath;
    d->mAgentScriptPath = agentScriptPath;

    // 1. 启动 QProcess
    d->mProcess = new QProcess(this);
    d->mProcess->setProgram(pythonExePath);
    d->mProcess->setArguments({agentScriptPath});

    // 连接信号
    connect(d->mProcess, &QProcess::readyReadStandardOutput, this, &DAAgentBridge::onReadyReadStandardOutput);
    connect(d->mProcess, &QProcess::readyReadStandardError, this, &DAAgentBridge::onReadyReadStandardError);
    connect(d->mProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &DAAgentBridge::onProcessFinished);

    d->mProcess->start();

    // 等待进程真正启动后再发送 init——QProcess 是异步的，start() 返回后
    // state() 仍为 Starting（非 Running），writeJson 的 Running 守卫会拒绝
    // 写入，导致 init 永不发送、子进程在 stdin 读取上阻塞挂起。
    if (!d->mProcess->waitForStarted(5000)) {
        emit agentError(tr("Agent process startup timed out"));  //cn:Agent 进程启动超时
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
    d->mRunning = true;

    // 启动 ready 超时检测——若子进程在 m_readyTimeoutMs 内未发送 ready 或
    // booting 心跳,视为初始化失败(常见原因:Python 导入失败、stdin 读取卡死、
    // LLM 配置错误)。booting 心跳会在 handleJsonLine 中重置本计时器,因此
    // langchain 冷启动导入(~16s)只要持续发出 booting 就不会被误杀。
    // 必须主动通知用户并杀进程，避免 UI 干等无响应。
    d->mReadyTimer = new QTimer(this);
    d->mReadyTimer->setSingleShot(true);
    connect(d->mReadyTimer, &QTimer::timeout, this, [this]() {
        auto* d = d_func();
        if (d->mRunning) {
            emit agentError(tr("Agent subprocess not ready within %1 ms, initialization may have failed, check logs")
                                .arg(d->mReadyTimeoutMs));  //cn:Agent 子进程启动后 %1 毫秒内未就绪，初始化可能失败，请查看日志排查
            if (d->mProcess && d->mProcess->state() != QProcess::NotRunning) {
                d->mProcess->kill();
            }
            d->mRunning = false;
        }
    });
    d->mReadyTimer->start(d->mReadyTimeoutMs);
}

/**
 * @brief 停止 agent 子进程（阻塞，供析构/重启时调用）
 */
void DAAgentBridge::stopAgent()
{
    DA_D(d);
    if (d->mStopped) {
        return;
    }
    d->mStopped = true;

    d->mInactivityTimer->stop();
    // 停止 ready 超时计时器(若还在等待)
    if (d->mReadyTimer) {
        d->mReadyTimer->stop();
        delete d->mReadyTimer;
        d->mReadyTimer = nullptr;
    }
    if (d->mRunning && d->mProcess) {
        d->mUserRequestedStop = true;  // 标记主动停止，防止 onProcessFinished 误判为崩溃
        writeJson(QJsonObject{{"type", "stop"}});
        d->mProcess->closeWriteChannel();  // 关闭 stdin 写通道，使 Python 端 read1() 收到 EOF，reader 线程退出释放 BufferedReader 锁
        d->mProcess->waitForFinished(d->mStopTimeoutMs);  // 可配超时(默认 5s)
        if (d->mProcess->state() != QProcess::NotRunning) {
            d->mProcess->kill();
        }
    }
    d->mRunning = false;
}

/**
 * @brief 请求停止 agent 子进程（非阻塞，供用户主动终止时调用）
 */
void DAAgentBridge::requestStop()
{
    DA_D(d);
    d->mInactivityTimer->stop();
    // 停止 ready 超时计时器(若还在等待)
    if (d->mReadyTimer) {
        d->mReadyTimer->stop();
        delete d->mReadyTimer;
        d->mReadyTimer = nullptr;
    }
    // 停止已有的 stop 计时器(防止重复调用)
    if (d->mStopTimer) {
        d->mStopTimer->stop();
        delete d->mStopTimer;
        d->mStopTimer = nullptr;
    }
    if (d->mRunning && d->mProcess) {
        // 标记为用户主动终止——onProcessFinished 据此抑制异常退出错误
        d->mUserRequestedStop = true;
        writeJson(QJsonObject{{"type", "stop"}});
        // 非阻塞: 不调用 waitForFinished(会冻结 UI 最多 m_stopTimeoutMs),
        // 改用 QTimer 在超时后 kill。进程退出后由 onProcessFinished
        // 发射 agentBusy(false) 恢复 UI。
        d->mStopTimer = new QTimer(this);
        d->mStopTimer->setSingleShot(true);
        connect(d->mStopTimer, &QTimer::timeout, this, [this]() {
            auto* d = d_func();
            if (d->mProcess && d->mProcess->state() != QProcess::NotRunning) {
                d->mProcess->kill();
            }
            delete d->mStopTimer;
            d->mStopTimer = nullptr;
        });
        d->mStopTimer->start(d->mStopTimeoutMs);
    }
    d->mRunning = false;
}

/**
 * @brief 设置 C++ 侧工具映射表，供工具调用时查找执行
 * @param tools 工具名 → 工具实现指针的映射
 */
void DAAgentBridge::setTools(const QMap<QString, DAAbstractAgentTool*>& tools)
{
    DA_D(d);
    d->mTools = tools;
}

/**
 * @brief 检查 agent 子进程是否正在运行
 * @return 若子进程正在运行返回 true
 */
bool DAAgentBridge::isRunning() const
{
    DA_DC(d);
    return d->mRunning;
}

/**
 * @brief 检查是否处于崩溃恢复流程中
 * @return 若正在崩溃恢复返回 true
 */
bool DAAgentBridge::isRecovering() const
{
    DA_DC(d);
    return d->mRecovering;
}

/**
 * @brief 设置崩溃恢复标志
 * @param v 是否处于崩溃恢复
 */
void DAAgentBridge::setRecovering(bool v)
{
    DA_D(d);
    d->mRecovering = v;
}

/**
 * @brief 获取当前会话 ID（供崩溃恢复时 load_session 用）
 * @return 当前会话 ID
 */
QString DAAgentBridge::lastSessionId() const
{
    DA_DC(d);
    return d->mLastSessionId;
}

/**
 * @brief 发送用户消息到 agent 子进程
 * @param text 用户消息文本
 */
void DAAgentBridge::sendMessage(const QString& text)
{
    DA_D(d);
    d->mLastUserMessage = text;   // 记录用于崩溃恢复
    d->mTurnActive = true;        // 标记对话进行中
    d->mRestartCount = 0;         // 正常发消息时重置崩溃恢复计数
    // 用户发消息后 agent 进入"思考中"状态——此处统一发射 agentBusy(true)。
    emit agentBusy(true);
    QJsonObject msg;
    msg["type"]    = "user_msg";
    msg["content"] = text;
    writeJson(msg);
    startInactivityTimer();     // 启动看门狗
}

/**
 * @brief 发送工具执行结果回 agent 子进程
 * @param callId 工具调用 ID
 * @param result 工具执行结果 JSON
 */
void DAAgentBridge::sendToolResult(const QString& callId, const QJsonObject& result)
{
    DA_D(d);
    QJsonObject msg;
    msg["type"]    = "tool_result";
    msg["call_id"] = callId;
    msg["result"]  = result;
    writeJson(msg);
}

/**
 * @brief 发送用户对问题的回答回 agent 子进程
 * @param answer 用户回答文本
 */
void DAAgentBridge::sendUserAnswer(const QString& answer)
{
    DA_D(d);
    // 用户回答后 agent 恢复工作——清除等待标志并重新启动看门狗，
    // 以便检测 agent 恢复推理后是否卡死
    d->mWaitingUserAnswer = false;
    QJsonObject msg;
    msg["type"]   = "user_answer";
    msg["answer"] = answer;
    writeJson(msg);
    startInactivityTimer();
}

/**
 * @brief 下发历史会话消息让 agent 子进程重建 state（不重启子进程切换会话）
 * @param sessionId 会话 ID
 * @param messages 历史消息数组
 */
void DAAgentBridge::sendLoadSession(const QString& sessionId, const QJsonArray& messages)
{
    DA_D(d);
    d->mLastSessionId = sessionId;  // 记录当前会话 ID，供崩溃恢复时 load_session 用
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

// ===========================================================================
// 私有方法
// ===========================================================================

/**
 * @brief 写入 JSON 消息到子进程 stdin
 * @param obj JSON 消息对象
 * @return 写入成功返回 true
 */
bool DAAgentBridge::writeJson(const QJsonObject& obj)
{
    DA_D(d);
    if (!d->mProcess || d->mProcess->state() != QProcess::Running) {
        return false;
    }
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    qint64 written  = d->mProcess->write(data);
    if (written != data.size()) {
        // stdin 写入失败——管道可能已关闭
        emit agentError(tr("Failed to write to agent subprocess stdin"));  //cn:写入 agent 子进程 stdin 失败
        return false;
    }
    return true;
}

/**
 * @brief 读取子进程 stdout 数据并按行解析 JSON Lines 协议
 */
void DAAgentBridge::onReadyReadStandardOutput()
{
    DA_D(d);
    // 累积数据到缓冲区
    d->mStdoutBuffer += d->mProcess->readAllStandardOutput();

    // 按行解析 JSON Lines
    while (true) {
        int idx = d->mStdoutBuffer.indexOf('\n');
        if (idx < 0) break;  // 不完整行，等更多数据

        QByteArray lineData = d->mStdoutBuffer.left(idx);
        d->mStdoutBuffer      = d->mStdoutBuffer.mid(idx + 1);

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

/**
 * @brief 处理一行 JSON 协议消息并分发到对应信号/逻辑
 * @param msg JSON 消息对象
 */
void DAAgentBridge::handleJsonLine(const QJsonObject& msg)
{
    DA_D(d);
    // 任意协议消息到达——重置无活动看门狗
    // 仅在对话进行中（m_turnActive）且非工具执行期间才 reset，
    // 避免 ready/booting/session_loaded 等空闲期消息误启动计时器杀死空闲 agent
    if (!d->mToolExecuting && d->mTurnActive) {
        startInactivityTimer();
    }

    QString type = msg["type"].toString();
    if (type == "booting") {
        // 子进程已启动、正在导入重模块(langchain_openai 冷启动 ~16s)。
        // 收到 booting 心跳 → 重置 ready 超时计时器,避免导入期间被误杀。
        // 不 emit 任何信号(booting 非 ready,UI 无需感知)。
        if (d->mReadyTimer) {
            d->mReadyTimer->start(d->mReadyTimeoutMs);
        }
    } else if (type == "ready") {
        // 收到 ready 消息——停止 ready 超时计时器
        if (d->mReadyTimer) {
            d->mReadyTimer->stop();
            d->mReadyTimer->deleteLater();
            d->mReadyTimer = nullptr;
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
        // agent 向用户提问后 langgraph 进入 interrupt 暂停态，等待用户回答。
        // 期间不应启动无活动看门狗——用户可能离开较长时间才回答，
        // 这不属于 agent 卡死。设置标志并停止看门狗，sendUserAnswer 时恢复。
        d->mWaitingUserAnswer = true;
        d->mInactivityTimer->stop();
        emit agentQuestion(msg["text"].toString(),
                           msg["options"].toVariant().toStringList(),
                           msg.value("multi_select").toBool(false));
    } else if (type == "retrying") {
        // Python 端指数退避重试期间每次重试发一次 retrying 消息，不触发 agentBusy
        // 状态变化——busy 状态已在 sendMessage 时设为 true，重试期间保持 true
        emit agentRetrying(
            msg["attempt"].toInt(),
            msg["max_attempts"].toInt(),
            msg["delay_ms"].toInt(),
            msg["error_type"].toString(),
            msg["error_message"].toString()
        );
    } else if (type == "error") {
        // D9: error 消息增强为携带 error_type，用于 C++ 端选择用户文案
        QString errorType = msg.value("error_type").toString();
        QString detail    = msg.value("detail").toString();
        // Gap A 修复：error 消息也重置 busy 状态（防 Python 发 error 不发 done 时 UI 卡死）
        // 但注意：Python 的 main() catch block 总是 error + done 连续发送，
        // done 分支也会 emit agentBusy(false)，所以这里 emit 是双保险
        d->mInactivityTimer->stop();
        d->mTurnActive = false;
        d->mWaitingUserAnswer = false;
        emit agentError(msg["message"].toString(), errorType, detail);
        emit agentBusy(false);
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
        d->mInactivityTimer->stop();
        d->mTurnActive = false;
        d->mWaitingUserAnswer = false;
        emit agentBusy(false);
        emit agentDone();
    }
}

// RAII guard：工具执行期间暂停无活动看门狗，析构时恢复
struct ToolExecGuard {
    DAAgentBridge* self;
    ToolExecGuard(DAAgentBridge* s) : self(s) {
        auto* d = self->d_func();
        d->mToolExecuting = true;
        d->mInactivityTimer->stop();
    }
    ~ToolExecGuard() {
        auto* d = self->d_func();
        d->mToolExecuting = false;
        self->startInactivityTimer();  // sendToolResult 后 Python 会继续工作
    }
};

/**
 * @brief 执行工具调用，查找工具并返回结果
 * @param callId 工具调用 ID
 * @param toolName 工具名称
 * @param args 工具调用参数 JSON
 */
void DAAgentBridge::executeTool(const QString& callId,
                                const QString& toolName,
                                const QJsonObject& args)
{
    DA_D(d);
    ToolExecGuard guard(this);  // RAII：暂停看门狗，覆盖所有 return 路径

    QJsonObject result;

    // 1. 查找工具（Bridge 持有 m_tools，由 DAAgentModule::registerTool → setTools 填充）
    auto it = d->mTools.find(toolName);
    if (it == d->mTools.end() || it.value() == nullptr) {
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

/**
 * @brief 子进程退出时处理：排空缓冲、判断正常/异常退出、触发崩溃恢复
 * @param exitCode 退出码
 * @param exitStatus 退出状态
 */
void DAAgentBridge::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    DA_D(d);
    // 1. 停止 ready 超时计时器(进程已退出，无需再等)
    if (d->mReadyTimer) {
        d->mReadyTimer->stop();
        d->mReadyTimer->deleteLater();
        d->mReadyTimer = nullptr;
    }

    // 记录进程退出状态——即使 agent 静默崩溃(无 stderr 输出)，
    // 日志也有记录，便于诊断退出码含义(如 62097 等异常退出码)
    daDebug << "Agent process finished: exitCode=" << exitCode
            << " exitStatus=" << (exitStatus == QProcess::NormalExit ? "NormalExit" : "CrashExit");

    // 2. 排空残留缓冲——若最后一个 stdout chunk 与 finished 信号几乎同时到达，
    // onReadyReadStandardOutput 可能留下未以 '\n' 结尾的完整行；不在此排空会丢失
    // 这部分消息（如最后的 message_end / done）。
    // 注意：排空可能调用 handleJsonLine（其中 done/error 会停止 inactivityTimer）
    if (!d->mStdoutBuffer.isEmpty()) {
        QByteArray lastLine = d->mStdoutBuffer;
        d->mStdoutBuffer.clear();
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

    // 3. 停止 inactivityTimer——必须在排空之后，避免排空消息重启计时器后又遗漏停止
    d->mInactivityTimer->stop();
    d->mToolExecuting = false;  // 重置工具执行标志，确保恢复从干净状态开始
    d->mWaitingUserAnswer = false;  // 重置等待用户回答标志，确保恢复从干净状态开始
    d->mTurnActive = false;     // 对话中断，重置对话进行中标志

    d->mRunning = false;

    bool wasUserStop = d->mUserRequestedStop;
    d->mUserRequestedStop = false;

    // 4. 停止 stop 计时器(进程已退出)
    if (d->mStopTimer) {
        d->mStopTimer->stop();
        d->mStopTimer->deleteLater();
        d->mStopTimer = nullptr;
    }

    if (wasUserStop) {
        d->mRecovering = false;
        daDebug << "Agent process stopped by user request";
        emit agentBusy(false);
        return;
    }

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        d->mRecovering = false;
        emit agentBusy(false);
        return;
    }

    // —— 异常退出：尝试自动恢复 ——

    if (d->mRestartCount < d->mMaxRestarts) {
        d->mRestartCount++;
        emit agentError(
            tr("Agent process crashed (exit code %1), recovering... (%2/%3)")
                //cn:Agent 进程异常退出（代码 %1），正在恢复... (%2/%3)
                .arg(exitCode)
                .arg(d->mRestartCount)
                .arg(d->mMaxRestarts),
            "crash_recovery", ""
        );

        // 延迟 1 秒后重启（避免崩溃循环过快）
        QTimer::singleShot(1000, this, [this]() {
            recoverFromCrash();
        });
    } else {
        d->mRecovering = false;
        emit agentError(
            tr("Agent process crashed repeatedly (%1 times), please restart the application")
                //cn:Agent 进程多次异常退出（%1 次），请重启程序
                .arg(d->mMaxRestarts),
            "crash_exhausted", ""
        );
        emit agentBusy(false);
    }
}

/**
 * @brief 读取子进程 stderr 用于调试（不转发到协议解析流）
 */
void DAAgentBridge::onReadyReadStandardError()
{
    DA_D(d);
    // 读取 stderr 用于调试——Python traceback、langchain/httpx 警告等
    // 注意：不要把 stderr 原样转发到 stdout 协议解析（会污染 JSON Lines 流）
    //
    // 不从 stderr emit agentError：httpx/openai 在 DEBUG 级别日志中打印
    // HTTP 错误（含 "Traceback"/"Error" 关键词）到 stderr，但这些错误
    // 通常已被 Python 侧 agent_node 的 force_compact 路径捕获处理，
    // agent 仍在正常运行。若从 stderr 发 agentError 会产生假警报，
    // 且触发 Module 的 agentError lambda 清空 pending 状态，干扰流程。
    // 真正的错误通过 stdout 的 {"type":"error"} 协议消息传递；
    // 进程崩溃由 onProcessFinished 处理。
    QByteArray data = d->mProcess->readAllStandardError();
    daDebug << "Agent stderr:" << QString::fromUtf8(data);
}

/**
 * @brief 无活动看门狗超时：通知用户并停止 agent 子进程
 */
void DAAgentBridge::onInactivityTimeout()
{
    DA_D(d);
    // 4 分钟无活动——通知 Python 优雅停止
    // 注意：有意先发 error 后 requestStop——让用户更快看到超时提示。
    // 功能等价：requestStop 最终触发 onProcessFinished 的 wasUserStop 分支（无重复 error）
    emit agentError(
        tr("Agent response timeout (no activity for %1 minutes)") //cn:Agent 响应超时（%1 分钟无活动）
            .arg(d->mInactivityTimeoutMs / 60000),
        "timeout", ""
    );
    // 发 stop 消息让 Python 优雅退出
    requestStop();
}

/**
 * @brief 启动无活动看门狗计时器
 */
void DAAgentBridge::startInactivityTimer()
{
    DA_D(d);
    // 等待用户回答期间不启动看门狗——用户可能离开较长时间才回答，
    // 此时 agent 处于 langgraph interrupt 暂停态，并非"卡死"
    if (d->mWaitingUserAnswer) {
        return;
    }
    if (d->mInactivityTimeoutMs > 0 && d->mRunning) {
        d->mInactivityTimer->start(d->mInactivityTimeoutMs);
    }
}

/**
 * @brief 崩溃恢复：标记恢复状态并复用 startAgent 重启子进程
 */
void DAAgentBridge::recoverFromCrash()
{
    DA_D(d);
    // 标记恢复流程——startAgent 据此跳过 m_restartCount/m_lastSessionId 重置，
    // 现有 agentReady 持久连接据此判断是否走恢复路径（见步骤 3.5）
    d->mRecovering = true;

    // 直接复用现有 startAgent——它已封装：
    //   - 旧进程清理（disconnect + kill + deleteLater）
    //   - 新 QProcess 创建 + connect 信号
    //   - init 消息发送
    //   - m_readyTimer 创建（new QTimer + setSingleShot + connect timeout + start）
    //   - m_pythonExePath / m_agentScriptPath / m_readyTimeoutMs / m_stopTimeoutMs 保存
    // 避免重复实现整套启动序列（旧版本手动重建 m_readyTimer 会空指针解引用）
    startAgent(d->mSavedLlmConfig, d->mSavedToolSpecs, d->mSavedSystemPrompt,
               d->mPythonExePath, d->mAgentScriptPath,
               d->mReadyTimeoutMs, d->mStopTimeoutMs);

    // ready 消息到达后，现有 agentReady 处理逻辑检查 m_recovering 标志，
    // 触发会话恢复 + 重发最后消息（见步骤 3.5），无需一次性连接
}

/**
 * @brief 崩溃恢复后重发最后一条用户消息
 */
void DAAgentBridge::resendLastMessage()
{
    DA_D(d);
    d->mRecovering = false;
    if (!d->mLastUserMessage.isEmpty()) {
        d->mTurnActive = true;
        emit agentBusy(true);
        writeJson(QJsonObject{{"type", "user_msg"}, {"content", d->mLastUserMessage}});
        startInactivityTimer();
    } else {
        d->mTurnActive = false;
        emit agentBusy(false);
    }
}

} // namespace DA
