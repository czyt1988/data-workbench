// DAAgentBridge.cpp
#include "DAAgentBridge.h"
#include "DAAbstractAgentTool.h"
#include "DAAgentPermissionManager.h"
#include "DAAgentToolExecutor.h"
#include "DAPyScriptRunner.h"
#include <QTimer>
#include <QHash>
#include <QSet>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include "DALogCategory.h"

namespace DA
{

/// 挂起的审批请求（executeTool 前置门 ask 路径登记，onToolApproval 消费）
struct PendingApproval {
    QString toolName;   ///< 工具名
    QJsonObject args;   ///< 工具参数（批准后原样执行，不含 _subagent 卡上下文）
    QString tier;       ///< 工具分级（用户拒绝时合成脱敏结果用）
    QString subagentId; ///< 子 agent 任务 id（子 agent 一期；主 agent 调用为空，Q18 撤卡依据）
    QString contentHash; ///< 判定时内容哈希（审计问题 26；批准后派发时注入执行参数校验）
};

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
    QTimer* mRecoveryTimer = nullptr; ///< 崩溃自愈延迟重启计时器（持句柄：用户 Stop 可取消，L4）
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
    bool mReadyReceived = false;            ///< 本次进程生命周期内是否收到过 ready（startAgent 重置；区分 init 阶段/运行期错误）
    QJsonObject mSavedLlmConfig;            ///< 启动参数缓存（崩溃恢复时复用）
    QJsonArray mSavedToolSpecs;
    QString mSavedSystemPrompt;
    QJsonArray mSavedSubagents;             ///< 子 agent 定义缓存（随 init 下发，崩溃恢复复用）

    // ---- 权限层（permission-layer P1） ----
    DAAgentPermissionManager* mPermissionManager = nullptr;  ///< 权限引擎（Module 持有，非拥有）
    QHash< QString, PendingApproval > mPendingApprovals;     ///< callId → 挂起审批
    QString mSessionId;  ///< 所属会话（attachBridge 注入；权限记忆按会话隔离查询键，可空=预热桥）

    // ---- 全局工具执行队列（决策点 2 方案 c，审计问题 12） ----
    DAAgentToolExecutor* mToolExecutor = nullptr;  ///< Module 持有，非拥有；空=退化直执行
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
 * @param subagents 子 agent 定义数组（随 init 下发，子 agent 一期）
 * @param pythonExePath Python 解释器路径
 * @param agentScriptPath agent 脚本路径
 * @param readyTimeoutMs 等待 ready/booting 心跳的超时（毫秒），默认 60s
 * @param stopTimeoutMs stopAgent 等待进程退出的超时（毫秒），默认 5s
 */
void DAAgentBridge::startAgent(const QJsonObject& llmConfig,
                               const QJsonArray& toolSpecs,
                               const QString& systemPrompt,
                               const QJsonArray& subagents,
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
    d->mSavedSubagents    = subagents;
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
    d->mReadyReceived  = false;  // 新进程生命周期开始，重置 ready 接收标志
    d->mStopped        = false;  // 审计 L7①：复位停止标志——桥对象经 stopAgent 后再
                                 // startAgent 复用时，优雅停止逻辑不得被旧标志短路
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
        // FailedToStart：Qt 只发 errorOccurred 不发 finished，onProcessFinished
        // 不会执行——必须在此补齐终止语义（审计问题 21），否则恢复路径
        // （recoverFromCrash → startAgent）成为状态机黑洞：无 processExited、
        // 无 agentBusy(false)，Module 侧 mSessionBusy/mSessionStarting 永不复位，
        // Dock 发送守卫拦截输入，用户连触发防御重建的消息都发不出。
        emit agentError(tr("Agent process startup timed out"));  //cn:Agent 进程启动超时
        d->mRecovering = false;
        d->mRunning = false;
        emit agentBusy(false);
        emit processExited();  // Module 据此记账清理（会话记忆/挂起状态）
        return;
    }

    // 记录 agent 启动信息(不打印 api_key 明文)——用于诊断启动失败/退出码异常
    // 子 agent 一期：附 init 载荷中的子 agent 定义名单（验收/排查 init 下发用）
    QStringList subagentNames;
    for (const QJsonValue& sv : subagents) {
        subagentNames.append(sv.toObject().value("name").toString());
    }
    daDebug << "Starting agent: python=" << pythonExePath
            << " script=" << agentScriptPath
            << " base_url=" << llmConfig.value("base_url").toString()
            << " model=" << llmConfig.value("model").toString()
            << " subagents=[" << subagentNames.join(QStringLiteral(", ")) << "]";

    // 2. 发送 init 消息（此时 state() 为 Running，writeJson 守卫通过）
    //    config 内合并权限层字段（母文档 §8 契约 3：全量下发；Python 侧 P1 仅存储，
    //    计划二实现消费）。合并进 config 而非顶层，与 reconfigure 单一来源对齐。
    QJsonObject initConfig = llmConfig;
    const QJsonObject permFields = buildPermissionConfig();
    for (auto it = permFields.constBegin(); it != permFields.constEnd(); ++it) {
        initConfig[it.key()] = it.value();
    }
    QJsonObject initMsg;
    initMsg["type"]          = "init";
    initMsg["config"]        = initConfig;
    initMsg["tools"]         = toolSpecs;
    initMsg["system_prompt"] = systemPrompt;
    // 子 agent 一期：init 附子 agent 定义数组（母文档 §7 契约；为空时下发空数组，
    // Python 侧据此不注入 dispatch_subagents 工具）
    initMsg["subagents"]     = subagents;
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
            // ready 超时的典型原因是环境性失败（Python 依赖损坏/langchain 导入
            // 失败），重启必然再次超时——置用户停止标志使 onProcessFinished 走
            // wasUserStop 分支（审计问题 20）：不进 3 轮崩溃自愈循环（最长
            // 4×readyTimeout 无意义等待 + 5 条错误轰炸），直接终态报错。
            d->mUserRequestedStop = true;
            if (d->mProcess && d->mProcess->state() != QProcess::NotRunning) {
                d->mProcess->kill();
            }
            d->mRunning = false;
        }
    });
    d->mReadyTimer->start(d->mReadyTimeoutMs);

    // 通知 UI 进入"启动中"过渡态——与 agentBusy(thinking) 区分，
    // 避免冷启动期间（~16s langchain 导入）被误显示为"思考中"。
    // ready/ready 超时/进程退出后由 agentReady/agentBusy(false) 清除。
    emit agentStarting();
}

/**
 * @brief 停止 agent 子进程（阻塞，供析构/重启时调用）
 */
void DAAgentBridge::stopAgent()
{
    beginStopAgent();
    awaitStopAgent();
}

/**
 * @brief 两阶段停止·第一阶段：写 stop + 关写通道（非阻塞）
 *
 * 审计 L7②：Module::shutdown 对 N 桥先全部执行本阶段（Python 端并行收到
 * stop/EOF 开始优雅退出），再逐桥 awaitStopAgent——避免串行
 * "写 stop → 各自等满 stopTimeout" 造成最坏 N×5s 的应用关闭冻结。
 */
void DAAgentBridge::beginStopAgent()
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
    // 取消待执行的崩溃自愈重启（析构/关闭期间不得再复活子进程，L4 同族）
    if (d->mRecoveryTimer) {
        d->mRecoveryTimer->stop();
        d->mRecoveryTimer->deleteLater();
        d->mRecoveryTimer = nullptr;
        d->mRecovering = false;
    }
    if (d->mRunning && d->mProcess) {
        d->mUserRequestedStop = true;  // 标记主动停止，防止 onProcessFinished 误判为崩溃
        writeJson(QJsonObject{{"type", "stop"}});
        d->mProcess->closeWriteChannel();  // 关闭 stdin 写通道，使 Python 端 read1() 收到 EOF，reader 线程退出释放 BufferedReader 锁
    }
}

/**
 * @brief 两阶段停止·第二阶段：等待退出 + kill 兜底（阻塞至多 stopTimeoutMs）
 *
 * 注意（审计 L7③）：waitForFinished 可能在调用栈内同步触发 onProcessFinished
 * → Module 的持久化/记账 lambda 重入（后台会话可能当场 retireBridge 改桥
 * 映射）——调用方遍历桥集合时必须持快照，不得引用活映射。
 */
void DAAgentBridge::awaitStopAgent()
{
    DA_D(d);
    if (d->mRunning && d->mProcess) {
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
        d->mStopTimer->deleteLater();
        d->mStopTimer = nullptr;
    }
    // 审计 L4：崩溃恢复窗口（1s 延迟重启）内用户 Stop——取消重启、终结恢复
    // 流程。旧实现 singleShot 无句柄不可取消，Stop 被静默忽略后照常重启重放。
    if (d->mRecoveryTimer) {
        d->mRecoveryTimer->stop();
        d->mRecoveryTimer->deleteLater();
        d->mRecoveryTimer = nullptr;
        d->mRecovering = false;
        d->mLastUserMessage.clear();  // 用户已终止，不再重发
        emit agentBusy(false);        // 回合随 Stop 终结，解除 UI 忙碌/Stopping 态
    }
    // 审计 L3：kill 兜底按进程实际状态判断（而非 mRunning）——二次 requestStop
    // 时 mRunning 已为 false，若因此跳过定时器重建，则上方刚取消的 kill 兜底
    // 无人重建：Python 忽略 stop 时进程永不退出。
    if (d->mProcess && d->mProcess->state() != QProcess::NotRunning) {
        // 标记为用户主动终止——onProcessFinished 据此抑制异常退出错误
        d->mUserRequestedStop = true;
        writeJson(QJsonObject{{"type", "stop"}});  // 重复写入无害（Python 忽略第二条 stop）
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
            // 审计 L3：自身 timeout 槽内不得裸 delete 发送者（QTimer 在
            // timeout 发射栈内被销毁属未定义行为边界），改 deleteLater
            d->mStopTimer->deleteLater();
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
    if (!writeJson(msg)) {
        // 写入失败（进程未运行/管道已关闭）：消息从未到达 Python——回滚本轮
        // 状态，避免 busy 挂到 4 分钟看门狗超时才报错（审计问题 23）。
        // mLastUserMessage 一并清除：崩溃恢复不应重发从未送达的消息。
        d->mTurnActive = false;
        d->mLastUserMessage.clear();
        d->mInactivityTimer->stop();
        emit agentError(tr("Message not sent: agent subprocess is not running"));  //cn:消息未发送：agent 子进程未在运行
        emit agentBusy(false);
        return;
    }
    startInactivityTimer();     // 启动看门狗
}

/**
 * @brief 发送工具执行结果回 agent 子进程
 * @param callId 工具调用 ID
 * @param result 工具执行结果 JSON
 * @return 写入是否成功——失败=进程已死/管道已关，调用方据此不 emit
 * agentToolResult（Module 持久化 lambda 挂该信号），孤儿结果不落盘 JSONL
 *（审计问题 12 决策 ⑤"迟到结果不落盘"的核心闭环）
 */
bool DAAgentBridge::sendToolResult(const QString& callId, const QJsonObject& result)
{
    QJsonObject msg;
    msg["type"]    = "tool_result";
    msg["call_id"] = callId;
    msg["result"]  = result;
    return writeJson(msg);
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
    if (!writeJson(msg)) {
        // 写入失败：答案从未到达 Python（死桥场景，审计问题 17/23）——
        // 回滚忙碌态并发明确错误，避免用户以为已回答而 UI 挂到看门狗超时
        d->mTurnActive = false;
        d->mInactivityTimer->stop();
        emit agentError(tr("Answer not sent: agent subprocess is not running"));  //cn:回答未发送：agent 子进程未在运行
        emit agentBusy(false);
        return;
    }
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

/**
 * @brief 热替换 LLM 配置（不重启子进程、不丢 MemorySaver 会话状态）
 * @param config 新的 LLM 配置（base_url/api_key/model/max_output_tokens 等）
 *
 * 下发 reconfigure 消息给运行中的子进程，Python 端 AgentRunner.reconfigure() 热替换
 * ChatOpenAI 实例及相关组件（compactor/token_estimator 等），图与 MemorySaver 状态不动。
 * reconfigure 消息在 stdin 缓冲区排队，当前轮 run()/resume() 返回后主循环才处理，
 * 因此天然在两轮之间应用——当前轮用旧模型跑完，下一轮用新模型。Python 回 ready 确认。
 * 与 sendLoadSession 同构：不 emit agentBusy（非一轮对话），writeJson 守卫 state()==Running。
 *
 * 权限层（母文档 §8）：模式切换/设置页保存后经此热更新 permission_mode、workspace_root、
 * code_patterns、judge 等字段——复用既有 reconfigure 管道，不重建图，Python 侧仅存储。
 */
void DAAgentBridge::reconfigureAgent(const QJsonObject& config)
{
    DA_D(d);
    // 同步启动参数缓存（审计问题 22）：崩溃恢复 recoverFromCrash 用
    // mSavedLlmConfig 重启 init——不同步则用户换模型/密钥后复活的进程仍跑
    // 旧配置，旧 key 已失效时恢复必然再失败进 ready 超时循环。
    // 与 sendUpdateSubagents 同步 mSavedSubagents 的既定意图对齐。
    // 权限字段无需缓存：buildPermissionConfig() 在恢复 init 时实时读取
    // PermissionManager 当前状态。
    d->mSavedLlmConfig = config;
    QJsonObject merged = config;
    const QJsonObject permFields = buildPermissionConfig();
    for (auto it = permFields.constBegin(); it != permFields.constEnd(); ++it) {
        merged[it.key()] = it.value();
    }
    QJsonObject obj;
    obj["type"]   = "reconfigure";
    obj["config"] = merged;
    writeJson(obj);
}

/**
 * @brief 热更新子 agent 定义（不重启子进程、不重建图、不动会话状态，Q17）
 * @param subagents 当前全量子 agent 定义数组（协议载荷格式，母文档 §7）
 *
 * 定义增删改后由 DAAgentModule 调用。Python 侧分发器常驻，收到后即时替换
 * 定义集与 dispatch_subagents schema；运行中任务仍用派发时快照。
 * 与 sendLoadSession 同构：不 emit agentBusy，writeJson 守卫 state()==Running。
 */
void DAAgentBridge::sendUpdateSubagents(const QJsonArray& subagents)
{
    DA_D(d);
    d->mSavedSubagents = subagents;  // 同步缓存（崩溃恢复时 init 复用）
    QJsonObject obj;
    obj["type"]      = "update_subagents";
    obj["subagents"] = subagents;
    writeJson(obj);
}

/**
 * @brief 热更新工具规格（审计问题 19，镜像 sendUpdateSubagents）
 * @param toolSpecs 当前全量工具规格数组（OpenAI function schema）
 *
 * setTools 只同步 C++ 执行表——Python/LLM 看到的工具列表停留在 init 时刻：
 * 插件热插拔（57c90f8 官方特性）后，禁用插件的存活桥 LLM 仍调用已移除工具
 * （C++ 表已删 → Unknown tool 浪费一轮推理）；启用插件/新注册工具的存活桥
 * LLM 永远看不到新工具。长寿命会话桥（活跃会话跑完不退役）使窗口无限延长。
 * Python 侧收到后替换 tool_specs 并重绑 llm_with_tools（call-time 读取即生效）。
 */
void DAAgentBridge::sendUpdateTools(const QJsonArray& toolSpecs)
{
    DA_D(d);
    d->mSavedToolSpecs = toolSpecs;  // 同步缓存（崩溃恢复时 init 复用，对齐 sendUpdateSubagents）
    QJsonObject obj;
    obj["type"]  = "update_tools";
    obj["tools"] = toolSpecs;
    writeJson(obj);
}

/**
 * @brief 组装权限层下发字段（母文档 §8）
 * @return JSON 对象，含 permission_mode/workspace_root/gated_tools/
 *         tool_approval_timeout_sec/code_patterns/judge；未设置权限引擎时为空对象
 */
QJsonObject DAAgentBridge::buildPermissionConfig() const
{
    DA_DC(d);
    QJsonObject p;
    if (!d->mPermissionManager) {
        return p;
    }
    const DAAgentPermissionManager* mgr = d->mPermissionManager;
    p["permission_mode"]            = mgr->mode();
    // 按会话上下文下发（审计问题 25）：Python 判官（permission_judge）的
    // run_script 相对路径解析用该会话绑定的工作区——工程切换后 reconfigure
    // 广播不再把后台会话的 workspace_root 改写成新工程
    p["workspace_root"]             = mgr->workspaceRootForSession(d->mSessionId);
    p["gated_tools"]                = QJsonArray::fromStringList(DAAgentPermissionManager::gatedTools());
    p["tool_approval_timeout_sec"]  = mgr->toolApprovalTimeoutSec();
    p["code_patterns"]              = mgr->codePatterns().toJson();
    p["judge"]                      = QJsonObject{
        {QStringLiteral("model"), mgr->judgeModel()},
        {QStringLiteral("timeout_sec"), mgr->judgeTimeoutSec()},
    };
    return p;
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
        // stdin 写入失败——管道可能已关闭。不在此处 emit 用户错误：writeJson
        // 服务于多种消息（init/stop/load_session/tool_result/...），"失败意味着
        // 什么"因调用方而异——由关键调用方（sendMessage/sendUserAnswer 等）
        // 检查返回值、回滚本轮状态并发明确错误（审计问题 23）。此处仅记诊断日志。
        qWarning() << "DAAgentBridge::writeJson: stdin write failed, written"
                   << written << "of" << data.size() << "bytes, type="
                   << obj.value("type").toString();
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

    // 缓冲上限（审计 L6）：异常超长无换行输出（如子进程崩溃倾泻二进制垃圾）
    // 可无限吃内存。合法协议单行远小于此上限（最大的 message_end/
    // subagent_progress 聚合通常数百 KB 级），超限视为协议流损坏，整段丢弃。
    static constexpr int kMaxStdoutBufferBytes = 10 * 1024 * 1024;  // 10MB
    if (d->mStdoutBuffer.size() > kMaxStdoutBufferBytes) {
        qWarning() << "DAAgentBridge: stdout buffer exceeded" << kMaxStdoutBufferBytes
                   << "bytes without a complete line, dropping buffer content";
        d->mStdoutBuffer.clear();
    }

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
            // 不可解析的行不能静默丢弃——记录到日志便于排查协议问题。
            // 审计 L6：开发诊断日志用 qWarning 纯英文（da* 宏会把原始协议垃圾
            // 刷进 UI 消息队列，违反 AGENTS.md"开发诊断禁用 da* 宏"规约）
            qWarning() << "DAAgentBridge: failed to parse JSON line from agent stdout:"
                       << lineData << "error:" << parseError.errorString();
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
        d->mReadyReceived = true;  // 此后 error 视为运行期错误（进程设计为存活）
        emit agentReady(msg["model"].toString());
        // 审计问题 15（Bridge 侧重断言）：冷启动时序下 sendMessage 的 busy(true)
        // 先于 ready 到达、被 Dock 的 starting 守卫吞掉 web 推送；ready 在 ~16s 后
        // 到达时本轮对话才真正开始。若回合仍在进行则重发 busy(true)，避免整轮
        // 纯文本回复期间 UI 显示 Ready、无 Stop 按钮、发送守卫放行第二条消息。
        // reconfigure 确认 ready 不受影响：Python 在两轮之间处理 reconfigure，
        // done（清 mTurnActive）必先于该 ready 到达。
        if (d->mTurnActive) {
            emit agentBusy(true);
        }
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
        QString callId   = msg["call_id"].toString();
        QString toolName = msg["tool"].toString();
        QJsonObject args = msg["arguments"].toObject();
        // 权限层（母文档 §8）：Python 侧安全裁决（仅 auto 模式 + code_exec 产出，
        // 计划二上线；P1 恒为空对象，C++ 解析与消费逻辑已就绪并经测试锁定，契约 2）
        QJsonObject safety = msg.value("safety").toObject();
        // 子 agent 一期：子图发起的调用带可选 subagent_id（与 safety 并存，母文档 §7）。
        // 带 subagent_id 的调用不 emit agentToolCall（子转录不进主聊天流；
        // Module 持久化 lambda 因此天然跳过），执行与权限门照常（同门执法，Q5）。
        QString subagentId = msg.value("subagent_id").toString();
        if (subagentId.isEmpty()) {
            emit agentToolCall(toolName, args);
        }
        // 通过 QTimer::singleShot(0) 把工具执行投递回主线程事件循环，
        // 让当前 onReadyReadStandardOutput 尽快返回，避免在读取回调里
        // 长时间阻塞 stdout 管道（管道阻塞会导致子进程 write 阻塞/死锁）。
        QTimer::singleShot(0, this, [this, callId, toolName, args, safety, subagentId]() {
            executeTool(callId, toolName, args, safety, subagentId);
        });
    } else if (type == "subagent_progress") {
        // 子 agent 一期（母文档 §7）：任务进度消息 {call_id, task_id?, subagent?,
        // state: spawned|running|done|error|timeout|stopped, message?, results?}。
        // 心跳为无 task_id 的 running 态——handleJsonLine 开头的看门狗重置天然保活。
        // Q18 dismissal 由本分支承担：任务进入终态或派发聚合结束时，按任务 id
        // 撤销对应挂起审批卡，防"任务已死而用户事后批准"的身后执行。
        const QString state = msg.value("state").toString();
        static const QSet<QString> kTerminalStates = {
            QStringLiteral("done"), QStringLiteral("error"),
            QStringLiteral("timeout"), QStringLiteral("stopped"),
        };
        if (kTerminalStates.contains(state)) {
            // 单任务终态：按 task_id 撤卡（心跳等无 task_id 消息跳过）
            const QString taskId = msg.value("task_id").toString();
            if (!taskId.isEmpty()) {
                dismissSubagentApprovals({taskId});
            }
        }
        if (msg.contains("results")) {
            // 派发聚合结束：按 results 中全部 task_id 撤销残留挂起审批
            // （覆盖未逐条上报终态的任务；主 agent 审批无 subagentId 不受影响）
            // results 兼容两种形态：任务对象数组，或 {tasks:[...]}（B 文件 dispatch 返回形态）
            QStringList taskIds;
            QJsonArray results = msg.value("results").toArray();
            if (results.isEmpty() && msg.value("results").isObject()) {
                results = msg.value("results").toObject().value("tasks").toArray();
            }
            for (const QJsonValue& rv : results) {
                const QString tid = rv.toObject().value("task_id").toString();
                if (!tid.isEmpty()) {
                    taskIds.append(tid);
                }
            }
            dismissSubagentApprovals(taskIds);
        }
        emit agentSubagentProgress(msg);
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
        // Python 端线性退避重试期间每次重试发一次 retrying 消息，不触发 agentBusy
        // 状态变化——busy 状态已在 sendMessage 时设为 true，重试期间保持 true
        emit agentRetrying(
            msg["attempt"].toInt(),
            msg["max_attempts"].toInt(),
            msg["delay_ms"].toInt(),
            msg["error_type"].toString(),
            msg["error_message"].toString()
        );
    } else if (type == "error") {
        // error 消息按阶段区分处置（审计问题 9）：
        // - init 阶段错误（未收到过 ready，如 "config missing"）：Python main() 发完
        //   error 即提前 return 退出。停止 ready 超时计时器避免无谓等待；关闭 stdin
        //   写通道使 Python 端 stdin reader daemon 线程的 read1() 收到 EOF 解除阻塞，
        //   进程能正常退出而非被 TerminateProcess kill（exitCode=62097 CrashExit）。
        // - 运行期错误（已收到过 ready，如 quota_exhausted/auth_error/recursion_limit/
        //   session_load_failed）：Python 发完 error+done 后主循环继续、进程设计为存活。
        //   此时绝不能关闭写通道——否则 Python reader 收到 EOF → 主循环 break →
        //   进程以 exit 0 静默终止，下一条消息被迫再付一次冷启动，且退役路径写 stop
        //   到已关闭通道会触发二次假错误。
        if (d->mReadyTimer) {
            d->mReadyTimer->stop();
            d->mReadyTimer->deleteLater();
            d->mReadyTimer = nullptr;
        }
        if (!d->mReadyReceived && d->mProcess && d->mProcess->state() != QProcess::NotRunning) {
            d->mProcess->closeWriteChannel();
        }
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
    } else if (type == "tool_result_rejected") {
        // 迟到结果拒绝反馈（审计问题 12 ⑤ 观测增强）：Python 侧等待槽已弹出
        //（回合超时/停止后终结），结果被丢弃——记日志便于排查孤儿记录。
        // 超时两段式（本体超时从 exec_start 起算）落地后此场景已极罕见
        qWarning() << "DAAgentBridge: tool result rejected by python side (late or mismatched), callId="
                   << msg.value("call_id").toString();
    } else if (type == "done") {
        d->mInactivityTimer->stop();
        d->mTurnActive = false;
        d->mWaitingUserAnswer = false;
        // 回合正常完成——清除崩溃恢复重发缓存（审计问题 11）：活跃会话的桥
        // 跑完不退役，若进程在空闲期异常崩溃，自愈链 ready→load_session→
        // resendLastMessage 会把已回答过的用户消息重新注入：无人操作时 UI
        // 自发进入"思考中"、LLM 对同一问题再生成一遍答案写进会话 JSONL
        // （持久化污染）、白白消耗一轮 token。
        d->mLastUserMessage.clear();
        emit agentBusy(false);
        // turn_summary（Python send_done 附带）：回合完成度统计。
        // possibly_incomplete=true 表示模型"话说一半就停"（执行过工具但
        // 最终回复是意图性短句、无产出物）——提示用户任务可能未完成。
        QJsonObject ts = msg.value("turn_summary").toObject();
        if (ts.value("possibly_incomplete").toBool(false)) {
            emit agentTurnPossiblyIncomplete(ts.value("tool_rounds").toInt(0));
        }
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
 * @brief 设置权限引擎（Module 初始化时注入，非拥有指针）
 * @param manager 权限引擎指针（nullptr=不设门，保持旧行为）
 */
void DAAgentBridge::setPermissionManager(DAAgentPermissionManager* manager)
{
    DA_D(d);
    d->mPermissionManager = manager;
}

/**
 * @brief 设置所属会话标识（Module attachBridge 注入）
 * @param sessionId 会话 ID（权限记忆按会话隔离的查询键，决策点 1 方案 b）
 */
void DAAgentBridge::setSessionId(const QString& sessionId)
{
    DA_D(d);
    d->mSessionId = sessionId;
}

/**
 * @brief 所属会话 ID（可空：预热桥未被接管时无会话归属）
 * @return 会话 ID
 */
QString DAAgentBridge::sessionId() const
{
    DA_DC(d);
    return d->mSessionId;
}

/**
 * @brief 设置全局工具执行队列（Module attachBridge 注入，非拥有）
 * @param executor 执行器指针（nullptr=退化直执行，独立使用/测试场景）
 */
void DAAgentBridge::setToolExecutor(DAAgentToolExecutor* executor)
{
    DA_D(d);
    d->mToolExecutor = executor;
}

/**
 * @brief 用户/系统是否已请求停止（执行队列出队存活检查用）
 * @return requestStop/stopAgent/ready 超时置位后为 true（进程退出时复位）
 */
bool DAAgentBridge::isStopRequested() const
{
    DA_DC(d);
    return d->mUserRequestedStop;
}

/**
 * @brief 执行队列出队后的真实执行入口（DAAgentToolExecutor 泵调用）
 * @param callId 工具调用 ID
 * @param toolName 工具名
 * @param args 工具参数
 * @param subagentId 子 agent 任务 id
 *
 * executeToolNow 已含存活守卫（12b），此处直接委托——执行器出队检查与
 * 本守卫双保险（队列等待窗口内桥状态可能变化）
 */
void DAAgentBridge::runQueuedTool(const QString& callId, const QString& toolName,
                                  const QJsonObject& args, const QString& subagentId)
{
    executeToolNow(callId, toolName, args, subagentId);
}

/**
 * @brief 执行工具调用（前置权限门，两阶段，母文档 §4，继承 v1 暂停-恢复范式）
 *
 * decide() 产出 Allow → executeToolNow 真实执行；Deny → 合成拒绝结果回传；
 * Ask → 登记 mPendingApprovals、停看门狗、发 approval_pending 挂起 Python 侧
 * 计时、emit agentToolApprovalRequest 并 return（镜像 ask_user question 暂停态；
 * 裁决经 onToolApproval 恢复）。
 * @param callId 工具调用 ID
 * @param toolName 工具名称
 * @param args 工具调用参数 JSON
 * @param safety Python 侧安全裁决（可空；计划二生产）
 * @param subagentId 子 agent 任务 id（子 agent 一期；主 agent 调用为空串）。
 * 子 agent 调用经同一门执法、天然继承父当前激活模式与分级（Q5）；
 * Ask 路径登记 subagentId 供 Q18 终态撤卡，审批卡经 args._subagent 携带上下文。
 */
void DAAgentBridge::executeTool(const QString& callId,
                                const QString& toolName,
                                const QJsonObject& args,
                                const QJsonObject& safety,
                                const QString& subagentId)
{
    DA_D(d);

    // 判定时内容哈希（审计问题 26）：Python permission_judge 对 run_script
    // 判定读文件产出 sha256，随 safety 透传——注入执行参数供工具执行前校验
    //（TOCTOU 闭环），其它工具/无判定时为空不注入
    const QString contentHash = safety.value(QStringLiteral("content_hash")).toString();

    // ---- 权限门（C++ 唯一执法点，A1） ----
    if (d->mPermissionManager) {
        // decide 携带桥所属会话（决策点 1 方案 b）：会话记忆按会话查询
        const DAAgentPermissionManager::Decision dec =
            d->mPermissionManager->decide(d->mSessionId, toolName, args, safety);
        if (dec.action == DAAgentPermissionManager::Deny) {
            // 合成拒绝结果（A11 按分级脱敏：reason 已由 decide 产出）；
            // 写入失败（进程已死）不 emit——孤儿结果不落盘（问题 12 决策 ⑤）
            QJsonObject result;
            result["success"] = false;
            result["error"]   = dec.reason;
            if (sendToolResult(callId, result)) {
                emit agentToolResult(toolName, result, subagentId);
            }
            return;
        }
        if (dec.action == DAAgentPermissionManager::Ask) {
            // 挂起等待用户裁决：登记 pending、停看门狗（用户思考时间不计无活动）
            PendingApproval pa;
            pa.toolName    = toolName;
            pa.args        = args;
            pa.tier        = dec.tier;
            pa.subagentId  = subagentId;
            pa.contentHash = contentHash;  // 批准后派发校验用（问题 26）
            d->mPendingApprovals.insert(callId, pa);
            d->mInactivityTimer->stop();
            // 通知 Python 侧暂停工具 RPC 计时——用户审批等待不设时限，
            // 批准后由 executeToolNow 发 tool_exec_start 作为计时起点
            QJsonObject pendingMsg;
            pendingMsg["type"]    = "approval_pending";
            pendingMsg["call_id"] = callId;
            writeJson(pendingMsg);
            // 审批卡上下文：子 agent 来源以 _subagent 键写入 args（同 _tier/
            // _rememberable 先例，不改 agentToolApprovalRequest 签名，Q18）；
            // pa.args 保持干净，批准后执行不带该键
            QJsonObject cardArgs = args;
            if (!subagentId.isEmpty()) {
                cardArgs[QStringLiteral("_subagent")] = subagentId;
            }
            emit agentToolApprovalRequest(callId, toolName, cardArgs);
            return;
        }
    }

    // 放行 → 经全局执行队列派发（决策点 2 方案 c；无执行器退化直执行）
    dispatchToolExecution(callId, toolName, args, subagentId, contentHash);
}

/**
 * @brief 派发执行（决策点 2 方案 c）：入全局队列或退化直执行
 * @param callId 工具调用 ID
 * @param toolName 工具名
 * @param args 工具参数
 * @param subagentId 子 agent 任务 id
 * @param expectedContentHash 判定时内容哈希（审计问题 26，可空）
 *
 * executeTool 放行路径与 onToolApproval 批准路径共用。有执行器时入队并
 * 上报排队位置（Python tool_exec_queued + UI agentToolQueued），出队时
 * 执行器做存活/停止检查（12b/12c 取消语义）；无执行器（独立 Bridge/
 * 协议级测试）保持旧直执行行为。
 */
void DAAgentBridge::dispatchToolExecution(const QString& callId, const QString& toolName,
                                          const QJsonObject& args, const QString& subagentId,
                                          const QString& expectedContentHash)
{
    DA_D(d);
    // 判定时内容哈希注入执行参数（审计问题 26）：内部键 _expected_content_hash
    // 同 _tier/_subagent 先例——只进执行副本，不进 tool_call 持久化/UI 卡
    //（agentToolCall 已先以原始 args 发射）。run_script 工具执行前重读文件
    // 校验哈希，不一致拒绝执行（TOCTOU：判定→执行窗口含全局队列排队段，
    // 共享工作区脚本可能被 write_file/其它会话改写）
    QJsonObject execArgs = args;
    if (!expectedContentHash.isEmpty()) {
        execArgs[QStringLiteral("_expected_content_hash")] = expectedContentHash;
    }
    if (d->mToolExecutor) {
        const int position = d->mToolExecutor->enqueue(this, callId, toolName, execArgs, subagentId);
        notifyToolQueued(callId, toolName, position);
        return;
    }
    executeToolNow(callId, toolName, execArgs, subagentId);
}

/**
 * @brief 排队态上报：tool_exec_queued 协议消息 + agentToolQueued 信号
 * @param callId 工具调用 ID
 * @param toolName 工具名
 * @param position 队列位置（1-based）
 */
void DAAgentBridge::notifyToolQueued(const QString& callId, const QString& toolName, int position)
{
    QJsonObject msg;
    msg["type"]     = "tool_exec_queued";
    msg["call_id"]  = callId;
    msg["position"] = position;
    writeJson(msg);  // 排队上报失败无害（Python 侧排队段预算本就宽松）
    emit agentToolQueued(toolName, position);
}

/**
 * @brief 用户对审批卡的裁决（镜像 sendUserAnswer 恢复范式）
 *
 * approved=true → （file_write 可选写会话记忆，A5）+ executeToolNow 真实执行；
 * approved=false → 合成用户拒绝结果回传（code_exec 脱敏，不教 LLM 绕过）。
 * callId 无配对（已作废/重复点击）时静默忽略。
 * @param callId 工具调用 ID
 * @param approved 是否批准
 * @param rememberSession 是否本会话记住（仅 file_write 生效）
 */
void DAAgentBridge::onToolApproval(const QString& callId, bool approved, bool rememberSession)
{
    DA_D(d);
    auto it = d->mPendingApprovals.find(callId);
    if (it == d->mPendingApprovals.end()) {
        return;
    }
    const PendingApproval pa = it.value();
    d->mPendingApprovals.erase(it);

    if (approved) {
        // A5 [v2.1]：会话记忆仅 file_write；code_exec 一律不记忆。
        // 按桥所属会话分桶写入（决策点 1 方案 b）——"本会话记住"仅本会话可见
        if (rememberSession && d->mPermissionManager && pa.tier == DAAgentPermissionManager::tierFileWrite()) {
            const QString key = d->mPermissionManager->sessionScopeKey(d->mSessionId, pa.toolName, pa.args);
            if (!key.isEmpty()) {
                d->mPermissionManager->rememberSession(d->mSessionId, pa.toolName, key);
            }
        }
        // 批准后同样经全局执行队列派发（决策点 2 方案 c）：审批窗口内桥可能
        // 已死/被 Stop，出队存活检查取消"为将死进程执行"（审计 12b）；
        // 判定时内容哈希随挂起条目保留，派发时注入校验（问题 26）
        dispatchToolExecution(callId, pa.toolName, pa.args, pa.subagentId, pa.contentHash);
    } else {
        QJsonObject result;
        result["success"] = false;
        if (pa.tier == DAAgentPermissionManager::tierCodeExec()) {
            // 脱敏：与策略拒绝同文案，避免向 LLM 泄露"是用户拒绝"之外的信息差异
            result["error"] = DAAgentPermissionManager::codeDenyMessage();
        } else {
            result["error"] = QStringLiteral("Access denied: user rejected the operation");
        }
        // 写入失败（进程已死）不 emit——孤儿结果不落盘（问题 12 决策 ⑤）
        if (sendToolResult(callId, result)) {
            emit agentToolResult(pa.toolName, result, pa.subagentId);
        }
    }

    // 恢复看门狗（仍有其它挂起审批时由 startInactivityTimer 内部守卫拦截）
    startInactivityTimer();
}

/**
 * @brief 权限门放行后的真实执行（原 executeTool 主体，铁律 T5 try/catch 兜底）
 *
 * 入口处先发 tool_exec_start——Python 侧以此为工具 RPC 计时起点（审批等待
 * 期间已由 approval_pending 挂起计时，批准/直接放行后从执行开始重新计时）。
 * @param callId 工具调用 ID
 * @param toolName 工具名称
 * @param args 工具调用参数 JSON
 * @param subagentId 子 agent 任务 id（主 agent 调用为空串；随结果信号透传供过滤）
 */
void DAAgentBridge::executeToolNow(const QString& callId,
                                   const QString& toolName,
                                   const QJsonObject& args,
                                   const QString& subagentId)
{
    DA_D(d);
    ToolExecGuard guard(this);  // RAII：暂停看门狗，覆盖所有 return 路径
    // 会话命名空间上下文（决策点 3 方案 c，审计问题 13）：run_code/run_script
    // 经 DAPyScriptRunner 按会话查专属变量表——并发会话不再共享同一 Jupyter
    // 式命名空间（会话 B 的 df 静默改写会话 A 正在使用的 df，产出错误分析
    // 结果且无报错，是数据分析工作台最坏失败模式）。守卫覆盖整个执行期，
    // 不改 DAAbstractAgentTool::execute 公开 API；其它工具不消费该上下文。
    // mSessionId 为空（预热桥，理论上不执行工具）回退默认表
    DAPyScriptSessionContext pySessionCtx(d->mSessionId);

    // 存活/停止守卫（审计 12b/12c）：tool_call 投递后进程可能立刻崩溃，或
    // 用户在排队窗口内 Stop——不再真实执行（副作用不为死进程/已终止回合
    // 发生）。执行器出队检查与本守卫双保险（直执行退化路径同样受保护）
    if (!d->mRunning || d->mUserRequestedStop
        || !d->mProcess || d->mProcess->state() != QProcess::Running) {
        qInfo() << "DAAgentBridge::executeToolNow: skipped, subprocess not running or stop requested, tool="
                << toolName << "callId=" << callId;
        return;
    }

    // 出队开始执行（决策点 2 ③）：position=0 通知 UI 由"排队中"恢复"运行中"；
    // tool_exec_start 是 Python 侧工具本体超时的计时起点（两段式第二段）
    emit agentToolQueued(toolName, 0);
    QJsonObject execStart;
    execStart["type"]    = "tool_exec_start";
    execStart["call_id"] = callId;
    writeJson(execStart);

    QJsonObject result;

    // 1. 查找工具（Bridge 持有 m_tools，由 DAAgentModule::registerTool → setTools 填充）
    auto it = d->mTools.find(toolName);
    if (it == d->mTools.end() || it.value() == nullptr) {
        result["error"]  = QString("Unknown tool: %1").arg(toolName);
        result["success"] = false;
        // 写入成功才 emit（同步推送到 UI 显示 + Module 持久化）——孤儿不落盘
        if (sendToolResult(callId, result)) {
            emit agentToolResult(toolName, result, subagentId);
        }
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

    // 3. 把结果回传子进程（让 agent_runner 继续推理）；4. 写入成功才发射
    // 信号（聊天 UI 展示 + Module 持久化挂该信号）——工具执行期间进程死亡
    // （崩溃/被杀）时结果无处送达，emit 会落盘孤儿 tool_result（Python 永远
    // 没收到，重放/恢复配对错乱），审计问题 12 决策 ⑤"迟到结果不落盘"
    const bool delivered = sendToolResult(callId, result);
    if (delivered) {
        //（带 subagentId 的子转录结果由 Module 过滤，不进主聊天流/不落盘，母文档 §7）
        emit agentToolResult(toolName, result, subagentId);
    } else {
        qWarning() << "DAAgentBridge::executeToolNow: result not delivered (subprocess gone), dropped, tool="
                   << toolName << "callId=" << callId;
    }
}

/**
 * @brief Q18 dismissal：按子 agent 任务 id 撤销对应挂起审批卡（母文档 §3/§7）
 * @param subagentIds 需要撤卡的子 agent 任务 id 列表（空串忽略）
 *
 * 仅撤 PendingApproval.subagentId 非空且命中列表的条目——主 agent 审批
 * （subagentId 为空）绝不受影响（风险表：Q18 dismissal 误撤主 agent 审批）。
 * 逐条 emit agentToolApprovalDismissed 让 UI 撤卡；不回传合成结果——
 * 任务终态后 Python 侧 Future 已取消/严格 call_id 匹配丢弃迟到结果，无害。
 */
void DAAgentBridge::dismissSubagentApprovals(const QStringList& subagentIds)
{
    DA_D(d);
    if (d->mPendingApprovals.isEmpty() || subagentIds.isEmpty()) {
        return;
    }
    const QSet<QString> idSet(subagentIds.cbegin(), subagentIds.cend());
    QStringList toDismiss;
    for (auto it = d->mPendingApprovals.constBegin(); it != d->mPendingApprovals.constEnd(); ++it) {
        if (!it.value().subagentId.isEmpty() && idSet.contains(it.value().subagentId)) {
            toDismiss.append(it.key());
        }
    }
    for (const QString& callId : std::as_const(toDismiss)) {
        d->mPendingApprovals.remove(callId);
        emit agentToolApprovalDismissed(callId);
    }
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
            // 审计 L6：开发诊断日志用 qWarning 纯英文（不进 UI 消息队列）
            qWarning() << "DAAgentBridge: failed to parse trailing JSON line from agent stdout:"
                       << lastLine << "error:" << parseError.errorString();
        }
    }

    // 3. 停止 inactivityTimer——必须在排空之后，避免排空消息重启计时器后又遗漏停止
    d->mInactivityTimer->stop();
    d->mToolExecuting = false;  // 重置工具执行标志，确保恢复从干净状态开始
    d->mWaitingUserAnswer = false;  // 重置等待用户回答标志，确保恢复从干净状态开始
    d->mTurnActive = false;     // 对话中断，重置对话进行中标志

    // 权限层（继承 v1 P5）：子进程退出使所有挂起审批作废——逐条 emit dismissed
    // 让 UI 撤卡，不跨重启存活。正常停止/请求停止/崩溃退出均走此路径。
    if (!d->mPendingApprovals.isEmpty()) {
        const QStringList pendingIds = d->mPendingApprovals.keys();
        d->mPendingApprovals.clear();
        for (const QString& id : pendingIds) {
            emit agentToolApprovalDismissed(id);
        }
    }
    // 进程退出钩子：Module 据此清空权限会话记忆（A5）。非 QObject manager
    // 无法自收信号，由 Module 在 connectSignals 中显式调用。
    emit processExited();

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
        // 调度时即置恢复标志（审计 L4）：1s 恢复窗口内 isRunning()==false，
        // Module stop() 与 UI 需经 isRecovering() 识别"正在自愈"的桥
        d->mRecovering = true;
        emit agentError(
            tr("Agent process crashed (exit code %1), recovering... (%2/%3)")
                //cn:Agent 进程异常退出（代码 %1），正在恢复... (%2/%3)
                .arg(exitCode)
                .arg(d->mRestartCount)
                .arg(d->mMaxRestarts),
            "crash_recovery", ""
        );

        // 延迟 1 秒后重启（避免崩溃循环过快）——持句柄定时器（审计 L4）：
        // 恢复窗口内用户 Stop 可经 requestStop 取消，不再"静默忽略、1s 后
        // 照常重启重放"违背用户终止意图（旧 singleShot 无句柄不可取消）
        if (d->mRecoveryTimer) {
            d->mRecoveryTimer->stop();
            d->mRecoveryTimer->deleteLater();
        }
        d->mRecoveryTimer = new QTimer(this);
        d->mRecoveryTimer->setSingleShot(true);
        connect(d->mRecoveryTimer, &QTimer::timeout, this, [this]() {
            recoverFromCrash();
        });
        d->mRecoveryTimer->start(1000);
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
    // 用 qInfo（系统 logger，无 category）而非 daDebug：
    //   1. 系统 logger 无 UI sink，不会把 httpx/openai 的 stderr 警告刷进 UI 消息队列；
    //   2. qInfo 为 info 级别，即使用户把日志级别调到 Info（daDebug 是 debug 级会被滤掉），
    //      Python 侧的 stderr 日志/traceback 仍能落 da_log.log，保证后续调试 agent 可见。
    //   （默认 Trace 级别下 daDebug 也能落盘，此处升级为 qInfo 是为应对用户调高级别的场景。）
    // 会话归属短码（审计 L18）：多子进程并发时区分 traceback 属于哪个会话
    //（预热桥无会话归属显示 [idle]）
    qInfo() << "Agent stderr:"
            << (d->mSessionId.isEmpty() ? QStringLiteral("[idle]") : d->mSessionId.left(8))
            << QString::fromUtf8(data);
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
    // 权限层：存在挂起审批时不启动看门狗——等待用户裁决可能超过无活动阈值
    //（继承 v1 风险项：ToolExecGuard 析构路径也经此函数恢复，统一在此守卫）
    if (!d->mPendingApprovals.isEmpty()) {
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
    startAgent(d->mSavedLlmConfig, d->mSavedToolSpecs, d->mSavedSystemPrompt, d->mSavedSubagents,
               d->mPythonExePath, d->mAgentScriptPath,
               d->mReadyTimeoutMs, d->mStopTimeoutMs);

    // 恢复链路由 Module 侧驱动（审计 L5 注释纠偏，Bridge 无 agentReady 消费逻辑）：
    // ready 到达 → Bridge emit agentReady → Module attachBridge 挂接的 agentReady
    // lambda 检查 isRecovering() → sendLoadSession（JSONL 历史重建）→
    // session_loaded → Module 调 resendLastMessage() 重发末条用户消息。
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
        if (!writeJson(QJsonObject{{"type", "user_msg"}, {"content", d->mLastUserMessage}})) {
            // 重发失败（进程恢复后又死亡）：回滚忙碌态（审计问题 23）。
            // mLastUserMessage 保留——消息从未送达，若后续再走恢复链应继续重试
            d->mTurnActive = false;
            d->mInactivityTimer->stop();
            emit agentError(tr("Message not sent: agent subprocess is not running"));  //cn:消息未发送：agent 子进程未在运行
            emit agentBusy(false);
            return;
        }
        startInactivityTimer();
    } else {
        d->mTurnActive = false;
        emit agentBusy(false);
    }
}

} // namespace DA
