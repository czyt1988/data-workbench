// DAAgentModule.cpp
#include "DAAgentModule.h"
#include "DAAgentBridge.h"
#include "DAAgentSessionStore.h"
#include "DAAgentManager.h"
#include "DAAgentPromptOps.h"
#include "DAAbstractAgentTool.h"
#include "DAAgentInterface.h"
#include "DACoreInterface.h"
#include "DAPyInterpreter.h"
#include "DADir.h"
#include "DALogCategory.h"
// Platform built-in tools moved to plugins/DAAgentTools plugin (plan-03)
#include <QFile>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include <QVariantList>
#include <QVariantMap>

// DPAPI（CryptProtectData/CryptUnprotectData）——DAAgentModule 内化的 API Key 加解密。
// 从 DAGui/DAAgentSettingsWidget.cpp 搬运而来（plan-01 加解密内化），解除对 DAGui 的依赖。
#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#endif

namespace {
// DPAPI 加解密（从 DAGui/DAAgentSettingsWidget.cpp 内化而来，
// 供 DAAgentModule::getLLMConfig/setLLMConfig 使用，解除对 DAGui 的依赖）。
// Windows 走 DPAPI，非 Windows 走 base64 fallback（与 DAGui 原版逐字一致，
// 已存的 agent-config.ini 加密 blob 可互解，两份并存期间无数据不兼容）。
QByteArray encryptApiKey(const QString& apiKey)
{
#ifdef Q_OS_WIN
    if (apiKey.isEmpty()) return {};
    QByteArray utf8 = apiKey.toUtf8();
    DATA_BLOB inBlob;
    inBlob.pbData = reinterpret_cast<BYTE*>(utf8.data());
    inBlob.cbData = static_cast<DWORD>(utf8.size());
    DATA_BLOB outBlob;
    if (!CryptProtectData(&inBlob, L"AgentApiKey", nullptr, nullptr, nullptr, 0, &outBlob)) {
        return {};
    }
    QByteArray enc(reinterpret_cast<const char*>(outBlob.pbData), static_cast<int>(outBlob.cbData));
    LocalFree(outBlob.pbData);
    return enc.toBase64();
#else
    return apiKey.toUtf8().toBase64();
#endif
}

QString decryptApiKey(const QByteArray& encrypted)
{
    if (encrypted.isEmpty()) return {};
#ifdef Q_OS_WIN
    QByteArray raw = QByteArray::fromBase64(encrypted);
    DATA_BLOB inBlob;
    inBlob.pbData = reinterpret_cast<BYTE*>(raw.data());
    inBlob.cbData = static_cast<DWORD>(raw.size());
    DATA_BLOB outBlob;
    if (!CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob)) {
        qWarning("decryptApiKey: CryptUnprotectData failed, GetLastError=%lu", GetLastError());
        return {};
    }
    QString result = QString::fromUtf8(reinterpret_cast<const char*>(outBlob.pbData), static_cast<int>(outBlob.cbData));
    LocalFree(outBlob.pbData);
    return result;
#else
    return QString::fromUtf8(QByteArray::fromBase64(encrypted));
#endif
}
} // namespace

// 模型条目既可能是对象 {id,context_window,max_output_tokens}，也可能是旧格式字符串。
// 以下 helper 兼容两种读法，保证旧 providers JSON 也能解析。
namespace {
/// 取模型 id（兼容字符串与对象格式）
QString modelIdOf(const QJsonValue& mv)
{
    if (mv.isString()) return mv.toString();
    if (mv.isObject()) return mv.toObject().value("id").toString();
    return {};
}
/// 取模型上下文窗口大小（对象格式，缺省/非正则回退 def）
int modelContextWindowOf(const QJsonValue& mv, int def)
{
    if (mv.isObject()) {
        int v = mv.toObject().value("context_window").toInt(def);
        return v > 0 ? v : def;
    }
    return def;
}
/// 取模型最大输出 token（对象格式，缺省/非正则回退 def）
int modelMaxOutputOf(const QJsonValue& mv, int def)
{
    if (mv.isObject()) {
        int v = mv.toObject().value("max_output_tokens").toInt(def);
        return v > 0 ? v : def;
    }
    return def;
}
} // namespace

namespace DA
{

// ===========================================================================
// PrivateData
// ===========================================================================
class DAAgentModule::PrivateData
{
    DA_DECLARE_PUBLIC(DAAgentModule)
public:
    explicit PrivateData(DAAgentModule* p);

    DACoreInterface* mCore = nullptr;
    DAAgentBridge* mBridge = nullptr;
    DAAgentManager* mAgentManager = nullptr;          ///< 提示词库管理器（QObject，parent=this）
    QMap<QString, DAAbstractAgentTool*> mTools;        ///< tool name → impl
    QHash<QString, QString> mSystemPrompts;            ///< prompt name → content
    DAAgentSessionStore* mSessionStore = nullptr;  ///< 非 QObject 无参构造；initialize() 内 new、析构显式 delete
    QString mCurrentSessionId;                     ///< 当前活跃会话
    QString mCurrentProjectPath;                   ///< 由 DAAppController::setCurrentProjectPath 注入
    QQueue<QString> mPendingToolCallUuids;         ///< 待配对的 tool_call/question 记录 uuid 队列（FIFO）
    bool mAgentBusy = false;                       ///< 由 agentBusy(bool)/agentDone 信号维护
    QString mPendingLoadSessionId;                  ///< 懒启动→ready 串联 load_session 的缓存
    QJsonArray mPendingLoadMessages;
    QString mPendingSwitchSessionId;               ///< 忙碌态切换排队：switchSession 遇 busy 时缓存
    int mCumulativeInTokens = 0;                   ///< 会话累计输入 token（跨轮次累加，压缩不重置）
    int mCumulativeOutTokens = 0;                  ///< 会话累计输出 token
    int mCumulativeTotalTokens = 0;                ///< 会话累计总 token
};

DAAgentModule::PrivateData::PrivateData(DAAgentModule* p) : q_ptr(p)
{
}

// ===========================================================================
// ctor / dtor
// ===========================================================================

/**
 * @brief 构造函数
 * @param core 核心接口指针
 * @param parent 父对象
 */
DAAgentModule::DAAgentModule(DACoreInterface* core, QObject* parent)
    : DAAgentInterface(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->mCore = core;
}

/**
 * @brief 析构函数——手动释放非 QObject 的 SessionStore
 */
DAAgentModule::~DAAgentModule()
{
    DA_D(d);
    // CRITICAL1：m_sessionStore 非 QObject 无 Qt 父子所有权，需手动 delete。
    // m_bridge 是 QObject 子对象，parent=this，由 Qt 自动释放，不在此 delete。
    delete d->mSessionStore;
}

/**
 * @brief 使用核心接口初始化模块
 * @param core 核心接口指针
 */
void DAAgentModule::initialize(DACoreInterface* core)
{
    DA_D(d);
    d->mCore = core;

    // Module 不创建也不持有 DAAgentDockWidget——Dock 由 DAAppDockingArea::
    // buildDockingArea() 创建；Dock 信号链（接口信号↔Dock 槽/信号）由 DAAppController
    // 在 initialize() 经接口直接 connect（plan-02 决策 D3b）。本模块只负责 agent
    // 框架逻辑 + Bridge 信号转发 + 持久化 lambda，不依赖 DAGui。

    // 创建 Bridge（不依赖 Dock 存在）
    d->mBridge = new DAAgentBridge(this);

    // 把 Bridge 的 agent 生命周期信号转发到 DAAgentInterface，供 APP 层
    // （DAAppController）connect 到 Dock。注意：agentUsage 不直接转发——
    // Module 内部 lambda（见 connectSignals）会补 context_window 后以
    // tokenUsageUpdated 暴露。此转发为 Bridge→this 接口信号（PMF 到 this），
    // 与 Dock 无关。
    connect(d->mBridge, &DAAgentBridge::agentToken, this, &DAAgentInterface::agentToken);
    connect(d->mBridge, &DAAgentBridge::agentMessageComplete, this, &DAAgentInterface::agentMessageComplete);
    connect(d->mBridge, &DAAgentBridge::agentToolCall, this, &DAAgentInterface::agentToolCall);
    connect(d->mBridge, &DAAgentBridge::agentToolResult, this, &DAAgentInterface::agentToolResult);
    connect(d->mBridge, &DAAgentBridge::agentQuestion, this, &DAAgentInterface::agentQuestion);
    connect(d->mBridge, &DAAgentBridge::agentError, this, &DAAgentInterface::agentError);
    connect(d->mBridge, &DAAgentBridge::agentReady, this, &DAAgentInterface::agentReady);
    connect(d->mBridge, &DAAgentBridge::agentStarting, this, &DAAgentInterface::agentStarting);
    connect(d->mBridge, &DAAgentBridge::agentBusy, this, &DAAgentInterface::agentBusy);
    connect(d->mBridge, &DAAgentBridge::agentDone, this, &DAAgentInterface::agentDone);
    connect(d->mBridge, &DAAgentBridge::agentSessionLoaded, this, &DAAgentInterface::agentSessionLoaded);

    // CRITICAL1：创建会话持久化层（非 QObject 无参构造，不传 parent）。
    // 目录就绪由 store 内部 DADir::getAppDataPath("sessions") mkpath。
    d->mSessionStore = new DAAgentSessionStore();

    // 提示词库管理器：播种通用默认 agent、加载用户已有提示词。
    // m_agentManager 为 QObject，parent=this，随 Module 释放。
    d->mAgentManager = new DAAgentManager(this);
    d->mAgentManager->ensureDefaultAgent();
    d->mAgentManager->loadAgents();

    // 连接 Bridge→Module 的持久化/状态 lambda（connectSignals 不再连 Dock，
    // 守卫改为仅判 m_bridge；Dock 连接已由 DAAppController 经接口完成）。
    connectSignals();

    // 平台内置工具由 plugins/DAAgentTools 插件在 initialize() 经
    // agent->registerTool 注册（plan-03 搬迁、plan-04 删除本模块的内置工具
    // 注册方法），本模块不再注册工具。
}

/**
 * @brief 注册工具供 agent 使用
 * @param tool 工具实现指针
 */
void DAAgentModule::registerTool(DAAbstractAgentTool* tool)
{
    DA_D(d);
    QString name = tool->getToolSpec()["name"].toString();
    d->mTools[name] = tool;
    if (d->mBridge) d->mBridge->setTools(d->mTools);
}

/**
 * @brief 注册命名系统提示词片段
 * @param name 提示词片段名称
 * @param content 提示词内容
 */
void DAAgentModule::registerSystemPrompt(const QString& name, const QString& content)
{
    DA_D(d);
    d->mSystemPrompts[name] = content;
}

/**
 * @brief 组装系统提示词（基础 markdown + 插件注入片段）
 * @return 完整的系统提示词字符串
 */
QString DAAgentModule::assembleSystemPrompt() const
{
    DA_DC(d);
    // 平台基础提示词：优先从外部 markdown 文件读取（src/DAAgent/system_prompt.md
    // 安装到 bin/PyScripts/DAWorkbench/agent/system_prompt.md），缺失时回退到内置默认，
    // 保证开发构建未执行 install 或文件被误删时 agent 仍可用。
    static const QString kDefaultPrompt = R"(你是 data-workbench 的 AI 数据分析助手。
你可以使用提供的工具来查询数据、绘制图表、分析数据。
请使用 markdown 格式输出你的回复。
当需要用户提供信息时，使用 ask_user 工具提问。)";

    QString base = kDefaultPrompt;
    QFile promptFile(detectSystemPromptPath());
    if (promptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray content = promptFile.readAll();
        promptFile.close();
        if (!content.trimmed().isEmpty()) {
            base = QString::fromUtf8(content);
        } else {
            daWarning << tr("Agent system prompt file is empty, fallback to built-in default: %1")
                         .arg(promptFile.fileName());
        }
    } else {
        daWarning << tr("Failed to read Agent system prompt file, fallback to built-in default: %1")
                     .arg(promptFile.fileName());
    }

    // 拼接插件注入的提示词
    QStringList parts;
    parts << base;
    for (auto it = d->mSystemPrompts.cbegin(); it != d->mSystemPrompts.cend(); ++it) {
        parts << it.value();
    }
    return parts.join("\n\n");
}

/**
 * @brief 组装工具规格 JSON 数组
 * @return 工具规格 JSON 数组
 */
QJsonArray DAAgentModule::assembleToolSpecs() const
{
    DA_DC(d);
    QJsonArray specs;
    for (auto* tool : d->mTools) {
        specs.append(tool->getToolSpec());
    }
    return specs;
}

/**
 * @brief 发送用户消息给 agent（含懒启动 + 会话持久化）
 * @param text 用户消息文本
 */
void DAAgentModule::sendMessage(const QString& text)
{
    DA_D(d);
    // 注意：不要在此 emit agentBusy(true)——DAAgentInterface 未声明 agentBusy 信号，
    // 此处 emit 无法编译。busy 状态改由 DAAgentBridge::sendMessage() 统一发射：
    // Bridge 的 agentBusy 信号已在 connectSignals() 中连接到 DAAgentDockWidget::onAgentBusy。
    // 契约7 + MAJOR3（round-3）：经 Module::createSession（内部 store.createSession +
    // 设 m_currentSessionId + setLastActive + emit sessionListChanged，不 emit sessionCreated
    // ——避免触发 plan-04 onSessionCreated 的 clearChat 擦除刚显示的用户消息）。
    // UI "+" 走 newSession()（有 sessionCreated）。m_currentSessionId = createSession() 为
    // 冗余赋值（createSession 内部已设），保留以契约7 的形式。
    if (d->mCurrentSessionId.isEmpty()) {
        d->mCurrentSessionId = createSession();
    }
    // 发送前持久化 user 消息
    d->mSessionStore->appendRecord(d->mCurrentSessionId, makeUserRecord(text));
    // 首条 user 消息定简短标题；标题变更时刷新 UI 下拉（否则 combo 一直显示 (untitled)）
    if (d->mSessionStore->ensureTitle(d->mCurrentSessionId)) {
        emit sessionListChanged(listSessionsForUI());
    }
    if (!d->mBridge->isRunning()) {
        // 懒启动
        startAgentInternal();
    }
    d->mBridge->sendMessage(text);
}

/**
 * @brief 停止正在运行的 agent（用户主动终止，非阻塞）
 */
void DAAgentModule::stop()
{
    DA_D(d);
    if (d->mBridge && d->mBridge->isRunning()) {
        d->mBridge->requestStop();
    }
}

/**
 * @brief 停止 agent 子进程并等待退出（阻塞，仅在应用关闭时调用）
 */
void DAAgentModule::shutdown()
{
    DA_D(d);
    // 阻塞停止 agent 子进程，确保在 Python 解释器关闭前子进程已干净退出。
    // 仅在 AppMainWindow::closeEvent 中调用（QApplication 事件循环尚在运行）。
    if (d->mBridge) {
        d->mBridge->stopAgent();
    }
}

/**
 * @brief 转发用户对 agent 提问的回答给子进程
 * @param answer 用户回答文本
 */
void DAAgentModule::sendUserAnswer(const QString& answer)
{
    DA_D(d);
    // 持久化：吸收原 connectSignals 的 dock::userAnswerSelected 持久化 lambda
    // （appendToolResultRecord），plan-02 删除 Dock 持有后由本方法体承接。
    // DAAppController 经 dock::userAnswerSelected → interface::sendUserAnswer
    // 信号→方法 PMF 连接，单次调用即完成持久化 + 转发 Bridge（无双重）。
    if (!d->mCurrentSessionId.isEmpty() && !d->mPendingToolCallUuids.isEmpty()) {
        QString tcid = d->mPendingToolCallUuids.dequeue();
        appendToolResultRecord(d->mCurrentSessionId, tcid, answer);
    }
    if (d->mBridge) {
        d->mBridge->sendUserAnswer(answer);
    }
}

/**
 * @brief 懒启动 agent 子进程（读取配置 + 探测路径 + 启动 Bridge）
 */
void DAAgentModule::startAgentInternal()
{
    DA_D(d);
    // 获取 LLM 配置（plan-06 提供真实实现）
    QJsonObject config = getLLMConfig();

    // 通过 detect 方法解析路径（不依赖 config 是否包含这些键）
    QString pythonExe = detectPythonExePath();
    QString scriptPath = detectAgentScriptPath();

    // 路径缺失时提前返回并报错——daCritical 会路由到 UI 日志窗口
    if (pythonExe.isEmpty()) {
        daCritical << tr("Cannot find Python interpreter path, please configure it in settings");  //cn:无法找到 Python 解释器路径，请在设置页配置 Python 解释器
        return;
    }
    if (scriptPath.isEmpty() || !QFile::exists(scriptPath)) {
        daCritical << tr("Cannot find agent_runner.py path: %1").arg(scriptPath);  //cn:无法找到 agent_runner.py 路径: %1
        return;
    }

    // 读取可配超时(与 DAAgentSettingsWidget 共用 agent-config.ini,默认值一致)
    // 单位:秒→毫秒。ready 超时默认 60s 覆盖 langchain 冷启动导入(~17s)+余量;
    // stop 超时默认 5s 保持原有行为。
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    int readyTimeoutMs = s.value("agent/ready_timeout_sec", 60).toInt() * 1000;
    int stopTimeoutMs   = s.value("agent/stop_timeout_sec", 5).toInt() * 1000;

    // 启动
    d->mBridge->startAgent(config, assembleToolSpecs(), assembleSystemPrompt(),
                           pythonExe, scriptPath, readyTimeoutMs, stopTimeoutMs);
}

/**
 * @brief 预启动 agent 子进程（程序启动时调用）
 *
 * 受 agent/auto_prestart 配置开关（默认 true）+ LLM 配置就绪（base_url/api_key/model 非空）
 * 双重控制。未配置或关闭开关时不预启动，用户发消息时走 sendMessage 的懒启动 fallback。
 * 子进程已在运行时不重复启动。
 */
void DAAgentModule::prestartAgent()
{
    DA_D(d);
    if (d->mBridge && d->mBridge->isRunning()) {
        return;  // 已在运行，不重复启动
    }
    QJsonObject config = getLLMConfig();
    // 检查 auto_prestart 开关（默认 true）
    if (!config.value("auto_prestart").toBool(true)) {
        return;  // 用户关闭了自动预热
    }
    // 检查 LLM 必填项是否就绪
    QString baseUrl = config.value("base_url").toString().trimmed();
    QString apiKey  = config.value("api_key").toString().trimmed();
    QString model   = config.value("model").toString().trimmed();
    if (baseUrl.isEmpty() || apiKey.isEmpty() || model.isEmpty()) {
        return;  // 未配置 LLM，不预启动（发消息时走懒启动 fallback 报错提示）
    }
    startAgentInternal();
}

/**
 * @brief 检查 agent 是否正在运行
 * @return 若 agent 正在运行返回 true
 */
bool DAAgentModule::isRunning() const
{
    DA_DC(d);
    // 转发 Bridge 的子进程运行状态——避免桩始终返回 false，
    // 导致外部（如 UI 忙状态判断）在子进程活跃期间误判为未运行
    return d->mBridge ? d->mBridge->isRunning() : false;
}

/**
 * @brief 连接 Bridge→Module 的持久化/状态 lambda（不连 Dock）
 */
void DAAgentModule::connectSignals()
{
    DA_D(d);
    // plan-02：Dock 信号链已由 DAAppController::initialize() 经接口直接 connect 到
    // DAAgentDockWidget（13 条 interface→dock 槽 + 7 条 dock 信号→interface 方法）。
    // 本函数不再持有/连接 Dock——只保留 Bridge→Module 的持久化/状态 lambda（与 Dock 无关）。
    // Dock 不再是 Module 依赖；Bridge 仍是。
    if (!d->mBridge) {
        return;
    }

    // ---- 持久化：对话事件 → JSONL ----
    // assistant 消息完成（纯文本回复）
    connect(d->mBridge, &DAAgentBridge::agentMessageComplete, this, [this](const QString& fullText) {
        auto* d = d_func();
        if (d->mCurrentSessionId.isEmpty()) return;
        appendAssistantRecord(d->mCurrentSessionId, fullText, /*toolCalls=*/{});
    });
    // 工具调用（含 ask_user 提问——ask_user 复用 tool_call 语义）
    connect(d->mBridge, &DAAgentBridge::agentToolCall, this, [this](const QString& tool, const QJsonObject& args) {
        auto* d = d_func();
        if (d->mCurrentSessionId.isEmpty()) return;
        // 契约6：appendToolCallRecord 返回本条记录 uuid，入队供后续 tool_result 按 FIFO 配对 tool_call_id
        d->mPendingToolCallUuids.enqueue(appendToolCallRecord(d->mCurrentSessionId, tool, args));
    });
    // 工具结果
    connect(d->mBridge, &DAAgentBridge::agentToolResult, this, [this](const QString& /*tool*/, const QJsonObject& result) {
        auto* d = d_func();
        if (d->mCurrentSessionId.isEmpty()) return;
        // 契约6：FIFO 出队取配对的 tool_call uuid（_rpc_call 串行保证顺序）
        // MAJOR1（round-4）：dequeue 前加 isEmpty 守卫，防空队列未定义行为/崩溃
        // （agentError/switchSession 已 clear 队列后残余 tool_result 信号可达）
        if (d->mPendingToolCallUuids.isEmpty()) return;
        QString tcid = d->mPendingToolCallUuids.dequeue();
        // MAJOR（round-4，from plan-04）：content 统一明文（result 序列化为 JSON 字符串，
        // 对齐映射表 content:json.dumps(result)）
        appendToolResultRecord(d->mCurrentSessionId, tcid,
                               QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)));
    });
    // token 使用量
    connect(d->mBridge, &DAAgentBridge::agentUsage, this, [this](int inT, int outT, int tot, const QString& src) {
        auto* d = d_func();
        if (d->mCurrentSessionId.isEmpty()) return;
        // streaming_estimate 是流式过程中的临时估算值，不持久化到 JSONL、不进累计——
        // 仅用于 UI 进度条实时刷新，真实 usage 由后续 message_end/usage 消息回传并持久化。
        if (src != "streaming_estimate") {
            appendUsageRecord(d->mCurrentSessionId, inT, outT, tot, src);
            // 真实 usage（agent/summary）累加到会话累计：跨轮次单调增长，压缩不重置
            d->mCumulativeInTokens += inT;
            d->mCumulativeOutTokens += outT;
            d->mCumulativeTotalTokens += tot;
            // 契约2：emit 5 参信号（context_window 经 readContextWindow 复用），累计值供 UI 显示
            emit tokenUsageUpdated(d->mCumulativeInTokens, d->mCumulativeOutTokens,
                                   d->mCumulativeTotalTokens, readContextWindow(), src);
        } else {
            // 流式估算不累加：emit“累计 + 本轮估算”的临时值，~ 前缀由 UI 侧 formatTokenLabel 添加
            emit tokenUsageUpdated(d->mCumulativeInTokens + inT, d->mCumulativeOutTokens + outT,
                                   d->mCumulativeTotalTokens + tot, readContextWindow(), src);
        }
    });
    // agent 提问（ask_user）——记录为 tool_call，供下一条 answer 配对
    connect(d->mBridge, &DAAgentBridge::agentQuestion, this, [this](const QString& text, const QStringList& options, bool multiSelect) {
        auto* d = d_func();
        if (d->mCurrentSessionId.isEmpty()) return;
        QJsonObject args;
        args["question"]     = text;
        args["options"]      = QJsonArray::fromStringList(options);
        args["multi_select"] = multiSelect;
        // 契约6：入队供下一条 answer 按 FIFO 配对
        d->mPendingToolCallUuids.enqueue(appendToolCallRecord(d->mCurrentSessionId, "ask_user", args));
    });
    // 注：userAnswerSelected 的持久化路径已删除——逻辑由 DAAgentModule::sendUserAnswer
    // 方法体承接（plan-01：dequeue m_pendingToolCallUuids + appendToolResultRecord + 转发
    // m_bridge->sendUserAnswer）。DAAppController 经 dock::userAnswerSelected →
    // interface::sendUserAnswer 单次调用即完成持久化+协议转发，无双重持久化。
    // （sendUserAnswer 是方法非信号，PMF 指向虚方法，Qt5+ 合法。）

    // ---- 崩溃恢复：sessionRestoreRequested → 从 SessionStore 读取并下发 load_session ----
    connect(d->mBridge, &DAAgentBridge::sessionRestoreRequested, this, [this](const QString& sessionId) {
        auto* d = d_func();
        // 从 SessionStore 读取历史消息
        if (d->mSessionStore) {
            QJsonArray messages = d->mSessionStore->readMessagesForLoad(sessionId);
            d->mBridge->sendLoadSession(sessionId, messages);
            // load_session 后 Python 回 session_loaded，经 agentSessionLoaded 信号
            // 在 agentSessionLoaded 的恢复 lambda 中重发最后消息
        }
    });

    // ---- 常驻 ready/busy/done/error 槽（plan-03，替代一次性 QMetaObject::Connection，
    //      避免多次连接泄漏与 ready 永不到达时堆泄漏） ----

    // 崩溃恢复：agentReady 恢复路径（必须在现有 pending lambda 之前 connect，
    // 以便在 pending lambda 清空 m_pendingLoadSessionId 之前检测到它）
    connect(d->mBridge, &DAAgentBridge::agentReady, this, [this](const QString&) {
        auto* d = d_func();
        if (!d->mBridge->isRecovering()) {
            return;  // 非恢复路径，交给现有逻辑
        }
        // switchSession 懒启动期间崩溃：m_pendingLoadSessionId 非空表示有待处理的会话切换，
        // 现有 pending lambda 会处理 load_session。恢复 lambda 不介入，避免：
        //   1. double load_session（pending lambda + 恢复 lambda 各发一次）
        //   2. 向错误会话重发上一会话的用户消息（agentSessionLoaded lambda 的 resendLastMessage）
        // 清除 m_recovering 使后续 agentSessionLoaded lambda 不触发 resendLastMessage
        if (!d->mPendingLoadSessionId.isEmpty()) {
            d->mBridge->setRecovering(false);
            return;
        }
        // 恢复路径：恢复会话历史或直接重发
        if (!d->mBridge->lastSessionId().isEmpty()) {
            // 有会话——触发 Module 侧 load_session（经 sessionRestoreRequested → sendLoadSession）
            emit d->mBridge->sessionRestoreRequested(d->mBridge->lastSessionId());
        } else {
            // 无会话——直接重发最后消息
            d->mBridge->resendLastMessage();
        }
    });

    // switchSession 懒启动 pending lambda（ready 到达后下发 load_session）
    connect(d->mBridge, &DAAgentBridge::agentReady, this, [this](const QString&) {
        auto* d = d_func();
        // MAJOR9：switchSession 懒启动分支缓存 pending 于此，ready 到达后下发 load_session。
        // 覆盖语义天然处理快速连续切换 A→B→C（只保留最后一次 pending）。
        if (!d->mPendingLoadSessionId.isEmpty()) {
            d->mBridge->sendLoadSession(d->mPendingLoadSessionId, d->mPendingLoadMessages);
            d->mPendingLoadSessionId.clear();
            d->mPendingLoadMessages = QJsonArray();
        }
    });
    connect(d->mBridge, &DAAgentBridge::agentBusy, this, [this](bool busy) {
        auto* d = d_func();
        d->mAgentBusy = busy;
    });
    connect(d->mBridge, &DAAgentBridge::agentDone, this, [this]() {
        auto* d = d_func();
        d->mAgentBusy = false;
        // 忙碌态切换排队续切（此时已非 busy）
        if (!d->mPendingSwitchSessionId.isEmpty()) {
            QString sid = d->mPendingSwitchSessionId;
            d->mPendingSwitchSessionId.clear();
            switchSession(sid);
        }
    });
    connect(d->mBridge, &DAAgentBridge::agentError, this, [this](const QString&) {
        auto* d = d_func();
        // 兜底清理 pending，避免 ready 永不到达时泄漏
        d->mPendingLoadSessionId.clear();
        d->mPendingLoadMessages = QJsonArray();
        d->mPendingSwitchSessionId.clear();
        // MAJOR1（round-3）：出错时清空队列，避免旧会话残留 uuid 配对新会话
        d->mPendingToolCallUuids.clear();
        d->mAgentBusy = false;
    });
    // 转发 agentRetrying 信号到接口（plan-03 step6）
    connect(d->mBridge, &DAAgentBridge::agentRetrying, this, [this](int attempt, int maxAttempts, int delayMs, const QString& errorType, const QString& errorMessage) {
        emit agentRetrying(attempt, maxAttempts, delayMs, errorType, errorMessage);
    });

    // ---- 崩溃恢复：agentSessionLoaded 时重发最后消息 ----
    // 如果是崩溃恢复路径，session_loaded 后重发最后一条用户消息。
    // isRecovering() 必须在 resendLastMessage 重置 m_recovering 之前判断。
    connect(d->mBridge, &DAAgentBridge::agentSessionLoaded, this, [this](const QString&) {
        auto* d = d_func();
        if (d->mBridge->isRecovering()) {
            d->mBridge->resendLastMessage();
        }
    });
}

/**
 * @brief 探测 Python 解释器路径
 * @return Python 解释器路径，空表示未找到
 */
QString DAAgentModule::detectPythonExePath() const
{
    // 平台已有规范的解释器解析器：DA::DAPyInterpreter::getPythonInterpreterPath()
    // （静态方法，DAPyInterpreter.h:46 / DAPyInterpreter.cpp:327-341）。
    // 它内部按两步优先级解析：
    //   1. python-config.json 中的 interpreter 路径（wherePythonFromConfig()）
    //   2. 系统 PATH 中的 python（wherePython()）
    QString path = DA::DAPyInterpreter::getPythonInterpreterPath();
    if (!path.isEmpty() && QFile::exists(path)) {
        return path;
    }
    // 兜底：在系统 PATH 中查找 python / python3
    return QStandardPaths::findExecutable("python");
}

/**
 * @brief 探测 agent_runner.py 脚本路径
 * @return agent 脚本路径
 */
QString DAAgentModule::detectAgentScriptPath() const
{
    // agent 脚本位于 bin/PyScripts/DAWorkbench/agent/agent_runner.py（plan-02）。
    // DACoreInterface::getPythonScriptsPath() 是静态方法，返回 PyScripts 目录。
    QString scriptsDir = DACoreInterface::getPythonScriptsPath();
    return scriptsDir + "/DAWorkbench/agent/agent_runner.py";
}

/**
 * @brief 探测系统提示词 markdown 文件路径
 * @return 系统提示词文件路径
 */
QString DAAgentModule::detectSystemPromptPath() const
{
    // 系统提示词 markdown 安装到与 agent_runner.py 同目录，运行时由
    // assembleSystemPrompt() 读取；路径解析复用 getPythonScriptsPath()。
    QString scriptsDir = DACoreInterface::getPythonScriptsPath();
    return scriptsDir + "/DAWorkbench/agent/system_prompt.md";
}

/**
 * @brief 显示 agent dock 窗口（no-op，Dock 显隐走 ADS）
 */
void DAAgentModule::showDockWidget()
{
    // plan-02：Module 不再持有 Dock；Dock 显隐走 ADS 的 setToggleViewAction
    // （由 DAAppController 绑定 ribbon action），本方法为 no-op。
}

/**
 * @brief 隐藏 agent dock 窗口（no-op）
 */
void DAAgentModule::hideDockWidget()
{
    // plan-02：同 showDockWidget，no-op。
}

/**
 * @brief 获取 LLM 配置
 * @return LLM 配置 JSON
 */
QJsonObject DAAgentModule::getLLMConfig() const
{
    // 从 agent-config.ini 读取（与设置页 DAAgentSettingsWidget 同一存储源，保持一致）
    // DAAgent 库不持有 DAAppConfig*（库无法链接 APP 可执行文件中的 DAAppConfig）
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    QJsonObject config;
    config["base_url"] = s.value("agent/llm_base_url").toString();
    config["model"]    = s.value("agent/llm_model").toString();
    // IniFormat 原生支持 QByteArray(@ByteArray 注解),api_key 直接读取
    QByteArray encKey  = s.value("agent/llm_api_key").toByteArray();
    if (!encKey.isEmpty()) {
        config["api_key"] = decryptApiKey(encKey);
    }
    // 上下文管理配置（带默认值兜底，随 init 消息 config 字段下发给 Python）
    // context_window / max_output_tokens 由激活模型派生（setActiveModel/syncActiveConnection 写入），
    // 默认 262144(256K) / 8192。
    config["context_window"]            = s.value("agent/context_window", 262144).toInt();
    config["max_output_tokens"]         = s.value("agent/max_output_tokens", 8192).toInt();
    config["compaction_threshold"]      = s.value("agent/compaction_threshold", 0.85).toDouble();
    config["max_recent_messages"]       = s.value("agent/max_recent_messages", 10).toInt();
    config["tool_result_max_chars"]     = s.value("agent/tool_result_max_chars", 20000).toInt();
    config["tool_result_preview_chars"] = s.value("agent/tool_result_preview_chars", 2000).toInt();
    // 启动/停止超时（默认 ready=60s 覆盖 langchain 冷启动导入、stop=5s），与 startAgentInternal 读法一致
    config["ready_timeout_sec"]         = s.value("agent/ready_timeout_sec", 60).toInt();
    config["stop_timeout_sec"]          = s.value("agent/stop_timeout_sec", 5).toInt();
    // 会话清理配置（默认 20/30，须与 cleanupSessions 读这两个 key 的默认值一致）
    config["max_sessions"]             = s.value("agent/max_sessions", 20).toInt();
    config["session_retention_days"]    = s.value("agent/session_retention_days", 30).toInt();
    // 重连与容错配置（plan-05：随 init 消息 config 字段下发给 Python/C++ 消费方）
    config["max_retries"]              = s.value("agent/llm_max_retries", 7).toInt();
    config["request_timeout_sec"]      = s.value("agent/llm_request_timeout_sec", 120).toInt();
    config["inactivity_timeout_sec"]   = s.value("agent/inactivity_timeout_sec", 240).toInt();
    config["max_subprocess_restarts"]  = s.value("agent/max_subprocess_restarts", 3).toInt();
    // recursion_limit：LangGraph 图最大迭代步数（compact→agent→tools 循环），
    // 防止 agent 陷入死循环时跑数千步。默认 150 步约支持 50 轮工具调用，
    // 满足数据分析频繁查数据的场景；用户可在设置页调整。
    config["recursion_limit"]          = s.value("agent/recursion_limit", 150).toInt();
    // 预启动开关：程序启动时是否自动预热 agent 子进程（默认 true）
    config["auto_prestart"]            = s.value("agent/auto_prestart", true).toBool();
    return config;
}

/**
 * @brief 设置 LLM 配置
 * @param config LLM 配置 JSON
 */
void DAAgentModule::setLLMConfig(const QJsonObject& config)
{
    // 必须用显式 ini 路径（与 getLLMConfig/startAgentInternal/cleanupSessions 一致），
    // 不可用默认构造 QSettings()——Windows 上后者写注册表，会与读 ini 的 getLLMConfig 错位致全部 key 丢失。
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    // base_url / model：原已写，补 contains 守卫保持一致
    if (config.contains("base_url"))
        s.setValue("agent/llm_base_url", config.value("base_url").toString());
    if (config.contains("model"))
        s.setValue("agent/llm_model", config.value("model").toString());
    // api_key：无条件写（移除原 if(!apiKey.isEmpty()) 守卫）。
    //   空字符串 → encryptApiKey("") 返回空 QByteArray → 清空存储 blob，用户可清空 api_key。
    //   getLLMConfig 侧的 if(!encKey.isEmpty()) 守卫保留（空时不 set api_key，页 loadConfig 得空，一致）。
    if (config.contains("api_key"))
        s.setValue("agent/llm_api_key", encryptApiKey(config.value("api_key").toString()));
    // 5 个已读未写的 context-management key（CRITICAL #1 补写）：
    // 注：context_window / max_output_tokens 通常由激活模型派生（setActiveModel/syncActiveConnection
    // 写入），设置页不再直接编辑这两个 key；此处保留守卫以兼容外部直接 setLLMConfig 的场景。
    if (config.contains("context_window"))
        s.setValue("agent/context_window",            config.value("context_window").toInt());
    if (config.contains("max_output_tokens"))
        s.setValue("agent/max_output_tokens",         config.value("max_output_tokens").toInt());
    if (config.contains("compaction_threshold"))
        s.setValue("agent/compaction_threshold",      config.value("compaction_threshold").toDouble());
    if (config.contains("max_recent_messages"))
        s.setValue("agent/max_recent_messages",       config.value("max_recent_messages").toInt());
    if (config.contains("tool_result_max_chars"))
        s.setValue("agent/tool_result_max_chars",     config.value("tool_result_max_chars").toInt());
    if (config.contains("tool_result_preview_chars"))
        s.setValue("agent/tool_result_preview_chars", config.value("tool_result_preview_chars").toInt());
    // 4 个新 key：
    if (config.contains("ready_timeout_sec"))
        s.setValue("agent/ready_timeout_sec",         config.value("ready_timeout_sec").toInt());
    if (config.contains("stop_timeout_sec"))
        s.setValue("agent/stop_timeout_sec",          config.value("stop_timeout_sec").toInt());
    if (config.contains("max_sessions"))
        s.setValue("agent/max_sessions",              config.value("max_sessions").toInt());
    if (config.contains("session_retention_days"))
        s.setValue("agent/session_retention_days",    config.value("session_retention_days").toInt());
    // 重连与容错配置（plan-05）
    if (config.contains("max_retries"))
        s.setValue("agent/llm_max_retries",           config.value("max_retries").toInt());
    if (config.contains("request_timeout_sec"))
        s.setValue("agent/llm_request_timeout_sec",   config.value("request_timeout_sec").toInt());
    if (config.contains("inactivity_timeout_sec"))
        s.setValue("agent/inactivity_timeout_sec",    config.value("inactivity_timeout_sec").toInt());
    if (config.contains("max_subprocess_restarts"))
        s.setValue("agent/max_subprocess_restarts",   config.value("max_subprocess_restarts").toInt());
    if (config.contains("recursion_limit"))
        s.setValue("agent/recursion_limit",           config.value("recursion_limit").toInt());
    if (config.contains("auto_prestart"))
        s.setValue("agent/auto_prestart",              config.value("auto_prestart").toBool());
}

// ===========================================================================
// 供应商与多模型管理实现
// ===========================================================================

/**
 * @brief 获取所有供应商配置（api_key 已解密为明文返回）
 * @return 供应商 JSON 数组，每元素 {name, base_url, api_key, models:[{id,context_window,max_output_tokens}]}
 *
 * 若 agent/providers 未配置（旧版本仅有 flat key），自动迁移：以 llm_base_url /
 * llm_api_key(解密) / llm_model 合成单个 "Default" 供应商（模型带默认
 * context_window=262144 / max_output_tokens=8192），保证旧配置平滑升级。
 * 模型条目若为旧格式字符串也按对象规范化返回。
 */
QJsonArray DAAgentModule::getProviders() const
{
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    QString raw = s.value("agent/providers").toString();
    if (!raw.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
        const QJsonArray arr = doc.array();
        QJsonArray out;
        for (const QJsonValue& pv : arr) {
            QJsonObject p = pv.toObject();
            QString enc = p.value("api_key").toString();
            p["api_key"] = enc.isEmpty() ? QString() : decryptApiKey(QByteArray::fromBase64(enc.toUtf8()));
            // 规范化 models：旧格式字符串 → 对象 {id,context_window,max_output_tokens}
            QJsonArray normModels;
            const QJsonArray models = p.value("models").toArray();
            for (const QJsonValue& mv : models) {
                if (mv.isObject()) {
                    normModels.append(mv.toObject());
                } else if (mv.isString()) {
                    QJsonObject mo;
                    mo["id"] = mv.toString();
                    mo["context_window"] = 262144;
                    mo["max_output_tokens"] = 8192;
                    normModels.append(mo);
                }
            }
            p["models"] = normModels;
            out.append(p);
        }
        return out;
    }
    // 迁移：旧版本仅有 flat key，合成单个 "Default" 供应商
    QJsonArray out;
    QJsonObject p;
    p["name"]     = QStringLiteral("Default");
    p["base_url"] = s.value("agent/llm_base_url").toString();
    QByteArray encKey = s.value("agent/llm_api_key").toByteArray();
    p["api_key"] = encKey.isEmpty() ? QString() : decryptApiKey(encKey);
    QString model = s.value("agent/llm_model").toString();
    QJsonArray models;
    if (!model.isEmpty()) {
        QJsonObject mo;
        mo["id"] = model;
        mo["context_window"] = s.value("agent/context_window", 262144).toInt();
        mo["max_output_tokens"] = 8192;
        models.append(mo);
    }
    p["models"] = models;
    out.append(p);
    return out;
}

/**
 * @brief 保存所有供应商配置（api_key 明文传入，内部加密存储）
 * @param providers 供应商 JSON 数组，每元素 {name, base_url, api_key, models:[id,...]}
 *
 * 存储为 agent/providers 单条 JSON 字符串（Compact），api_key 经 encryptApiKey
 * 加密为 base64。保存后重新同步激活连接（base_url/api_key/model）并刷新 Dock。
 */
void DAAgentModule::setProviders(const QJsonArray& providers)
{
    QJsonArray stored;
    for (const QJsonValue& pv : providers) {
        QJsonObject p = pv.toObject();
        QString key = p.value("api_key").toString();
        p["api_key"] = QString::fromUtf8(encryptApiKey(key));  // 加密 base64 字符串
        stored.append(p);
    }
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    s.setValue("agent/providers", QString::fromUtf8(QJsonDocument(stored).toJson(QJsonDocument::Compact)));
    s.sync();  // 确保 providers JSON 落盘，供下方 syncActiveConnection 的 getProviders 读到最新值
    // 重新同步激活连接（激活供应商的 base_url/api_key/model 写入 flat key）
    syncActiveConnection();
    emit availableModelsChanged(getAvailableModels());
    emit activeModelChanged(getActiveProvider(), getActiveModel());
}

/**
 * @brief 获取当前激活供应商名称
 * @return 激活供应商名称；未配置返回空
 */
QString DAAgentModule::getActiveProvider() const
{
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    return s.value("agent/active_provider").toString();
}

/**
 * @brief 获取所有可选模型列表（Dock 下拉用，不含 api_key）
 * @return QVariantList，每元素 QVariantMap{provider,model,context_window,max_output_tokens}
 */
QVariantList DAAgentModule::getAvailableModels() const
{
    QVariantList out;
    const QJsonArray providers = getProviders();
    for (const QJsonValue& pv : providers) {
        QJsonObject p = pv.toObject();
        QString pname = p.value("name").toString();
        const QJsonArray models = p.value("models").toArray();
        for (const QJsonValue& mv : models) {
            QVariantMap item;
            item["provider"] = pname;
            item["model"]    = modelIdOf(mv);
            item["context_window"]    = modelContextWindowOf(mv, 262144);
            item["max_output_tokens"] = modelMaxOutputOf(mv, 8192);
            out.append(item);
        }
    }
    return out;
}

/**
 * @brief 获取当前激活模型 id
 * @return 激活模型 id（即 agent/llm_model）
 */
QString DAAgentModule::getActiveModel() const
{
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    return s.value("agent/llm_model").toString();
}

/**
 * @brief 设置激活供应商+模型（Dock 选择用）
 * @param provider 供应商名称
 * @param model 模型 id
 *
 * 校验 supplier+model 存在后，写入 agent/active_provider / llm_model，并从该供应商
 * 同步 base_url/api_key 到 flat key、从该模型同步 context_window/max_output_tokens
 * （供 getLLMConfig/startAgentInternal 读取）。emit activeModelChanged 通知 Dock 刷新。
 * 若子进程正在运行则热替换 LLM 配置（reconfigureAgent，不重启子进程、不丢
 * MemorySaver 会话状态）；未运行时仅写 ini，下次懒启动用新配置。
 */
void DAAgentModule::setActiveModel(const QString& provider, const QString& model)
{
    DA_D(d);
    const QJsonArray providers = getProviders();
    QString baseUrl, apiKey;
    int ctxWin = 262144, maxOut = 8192;
    bool found = false;
    for (const QJsonValue& pv : providers) {
        QJsonObject p = pv.toObject();
        if (p.value("name").toString() != provider) continue;
        const QJsonArray models = p.value("models").toArray();
        for (const QJsonValue& mv : models) {
            if (modelIdOf(mv) == model) {
                baseUrl = p.value("base_url").toString();
                apiKey  = p.value("api_key").toString();
                ctxWin  = modelContextWindowOf(mv, 262144);
                maxOut  = modelMaxOutputOf(mv, 8192);
                found   = true;
                break;
            }
        }
        break;
    }
    if (!found) return;
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    s.setValue("agent/active_provider", provider);
    s.setValue("agent/llm_model",        model);
    s.setValue("agent/llm_base_url",     baseUrl);
    // DPAPI 解密失败时 apiKey 为空，不覆盖 flat key（保留可能有效的旧值）
    if (!apiKey.isEmpty()) {
        s.setValue("agent/llm_api_key", encryptApiKey(apiKey));
    }
    s.setValue("agent/context_window",  ctxWin);
    s.setValue("agent/max_output_tokens", maxOut);
    s.sync();  // 确保 6 个 flat key 落盘，供下方 getLLMConfig() 读到最新配置
    emit activeModelChanged(provider, model);
    // 子进程运行中则热替换 LLM 配置（不重启子进程、不丢 MemorySaver 会话状态）；
    // reconfigure 在 stdin 排队，当前轮跑完后 Python 主循环处理，下一轮用新模型。
    // 未运行时仅写 ini，下次懒启动用新 config。getLLMConfig() 在写完 6 个 flat
    // key 后调用，读到的是最新配置（含 base_url/api_key/model/context_window 等）。
    if (d->mBridge && d->mBridge->isRunning()) {
        d->mBridge->reconfigureAgent(getLLMConfig());
    }
}

/**
 * @brief 从激活供应商同步 base_url/api_key/model/context_window/max_output_tokens 到 flat ini key
 *
 * setProviders 后调用：保存的供应商可能改了激活供应商的 base_url/api_key，需同步到
 * flat key 供 getLLMConfig 读取。激活模型保留原 llm_model（若仍属于激活供应商则保留，
 * 否则改用激活供应商第一个模型），并同步该模型的 context_window/max_output_tokens。
 * 激活供应商为空或已不存在（被删除）时兜底取第一个供应商，使配置始终可启动。
 */
void DAAgentModule::syncActiveConnection()
{
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    const QJsonArray providers = getProviders();
    if (providers.isEmpty()) {
        return;  // 无供应商，无可同步
    }
    QString active = s.value("agent/active_provider").toString();
    // 校验 active 是否仍存在于 providers；空或不存在则兜底取第一个
    bool activeExists = false;
    for (const QJsonValue& pv : providers) {
        if (pv.toObject().value("name").toString() == active) {
            activeExists = true;
            break;
        }
    }
    if (!activeExists) {
        active = providers.first().toObject().value("name").toString();
        s.setValue("agent/active_provider", active);
    }
    for (const QJsonValue& pv : providers) {
        QJsonObject p = pv.toObject();
        if (p.value("name").toString() != active) continue;
        s.setValue("agent/llm_base_url", p.value("base_url").toString());
        // DPAPI 解密失败时 getProviders 返回空 api_key，此时不覆盖 flat key
        // （保留可能有效的旧值），避免每次启动 pushModelSelection→syncActiveConnection
        // 把 flat key 清空导致 getLLMConfig 读不到 api_key（报 config missing）
        QString decryptedKey = p.value("api_key").toString();
        if (!decryptedKey.isEmpty()) {
            s.setValue("agent/llm_api_key", encryptApiKey(decryptedKey));
        } else {
            qWarning("syncActiveConnection: provider api_key DPAPI decryption failed, "
                     "keeping existing flat key to avoid destroying it");
        }
        // 激活模型：保留原 llm_model（若属于本供应商），否则取本供应商第一个模型
        QString curModel = s.value("agent/llm_model").toString();
        const QJsonArray models = p.value("models").toArray();
        int matchedIdx = -1;
        for (int i = 0; i < models.size(); ++i) {
            if (modelIdOf(models.at(i)) == curModel) { matchedIdx = i; break; }
        }
        if (matchedIdx < 0 && !models.isEmpty()) {
            matchedIdx = 0;
            s.setValue("agent/llm_model", modelIdOf(models.at(0)));
        } else if (models.isEmpty()) {
            s.setValue("agent/llm_model", QString());  // 无模型则清空
        }
        // 同步激活模型的 context_window / max_output_tokens
        if (matchedIdx >= 0) {
            const QJsonValue mv = models.at(matchedIdx);
            s.setValue("agent/context_window", modelContextWindowOf(mv, 262144));
            s.setValue("agent/max_output_tokens", modelMaxOutputOf(mv, 8192));
        }
        break;
    }
}

/**
 * @brief 推送当前供应商/模型选择到 Dock
 *
 * 由 DAAppController 在接口↔Dock 信号链 connect 完成后调用（与 restoreLastActiveSession
 * 同处）。首次运行/旧配置迁移时若 active_provider 为空，先 syncActiveConnection 兜底
 * 取首个供应商并持久化，再 emit availableModelsChanged + activeModelChanged。
 */
void DAAgentModule::pushModelSelection()
{
    if (getActiveProvider().isEmpty()) {
        syncActiveConnection();  // 兜底：取首个供应商为激活并同步 flat key
    }
    emit availableModelsChanged(getAvailableModels());
    emit activeModelChanged(getActiveProvider(), getActiveModel());
}

// ===========================================================================
// 会话管理接口实现（plan-03）
// ===========================================================================

/**
 * @brief 创建新会话，返回新 sessionId
 * @return 新会话 ID（UUID4）
 */
QString DAAgentModule::createSession()
{
    DA_D(d);
    // MAJOR3（round-3）：不 emit sessionCreated，供 sendMessage 自动建会话用
    // （避免触发 plan-04 onSessionCreated 的 clearChat 擦除刚显示的用户消息）。
    // MAJOR2（round-3）：内部设 m_currentSessionId，所有调用方统一受益。
    QString sid = d->mSessionStore->createSession(d->mCurrentProjectPath);  // 带 projectPath
    d->mCurrentSessionId = sid;
    d->mSessionStore->setLastActive(sid, d->mCurrentProjectPath);
    resetCumulativeTokens();  // 新会话 0 消耗，清零累计成员
    emit sessionListChanged(listSessionsForUI());  // 契约3：带 payload
    return sid;
}

/**
 * @brief 新建会话（UI "+" 按钮入口）—— createSession + emit sessionCreated
 */
void DAAgentModule::newSession()
{
    DA_D(d);
    // MAJOR3（round-3）：供 UI "+" 按钮——createSession + emit sessionCreated。
    // sendMessage 自动建会话调 createSession（无 sessionCreated），
    // UI "+" 调 newSession（有 sessionCreated）。
    // 若当前已有活跃会话且仍为全新（无任何消息记录、agent 未启动），
    // 直接复用该会话——避免用户连续点击"+"堆积无用空会话。
    if (!d->mCurrentSessionId.isEmpty()
        && !isRunning()
        && d->mSessionStore->messageCount(d->mCurrentSessionId) == 0) {
        emit sessionCreated(d->mCurrentSessionId);  // 复用：仅触发 UI 幂等刷新（clearChat 对已空聊天为 no-op），不落盘新会话
        return;
    }
    QString sid = createSession();
    emit sessionCreated(sid);  // 仅此路径触发 UI clearChat
}

/**
 * @brief 切换到指定会话（懒启动→下发历史→恢复）
 * @param sessionId 目标会话 ID
 * @return 是否启动切换流程（false=已是当前会话无操作）
 */
bool DAAgentModule::switchSession(const QString& sessionId)
{
    DA_D(d);
    if (sessionId == d->mCurrentSessionId) return false;  // false = 已是当前会话
    // 0. 守忙碌态：上一轮仍在流式输出时，requestStop 并排队，agentDone 后续切，
    //    避免残余 agentToken/agentMessageComplete/agentToolCall/agentToolResult 信号被
    //    持久化 lambda 写入新会话 JSONL（旧会话尾巴污染新会话）。
    if (d->mBridge->isRunning() && d->mAgentBusy) {
        d->mPendingSwitchSessionId = sessionId;   // 排队；常驻 agentDone 槽续切
        d->mBridge->requestStop();                // 非阻塞停止
        return true;                             // 异步完成，sessionSwitched 在续切时 emit
    }
    // 1. 读历史消息
    QJsonArray messages = d->mSessionStore->readMessagesForLoad(sessionId);
    // MAJOR1（round-3）：实际切换前清空队列，丢弃旧会话未完成的 pending 配对
    d->mPendingToolCallUuids.clear();
    d->mCurrentSessionId = sessionId;
    d->mSessionStore->setLastActive(sessionId, d->mCurrentProjectPath);  // 带工程路径
    // 2. 确保子进程：未运行则懒启动，ready 后由常驻槽发 load_session
    if (!d->mBridge->isRunning()) {
        startAgentInternal();
        // 缓存 pending load；由 connectSignals 里建立的常驻 agentReady 槽处理
        // （不再每次 new QMetaObject::Connection，避免快速连续切换时多次连接、堆积
        //  与 ready 永不到达时的堆泄漏）
        d->mPendingLoadSessionId = sessionId;
        d->mPendingLoadMessages = messages;
    } else {
        d->mBridge->sendLoadSession(sessionId, messages);
    }
    // 3. UI 历史重放由 plan-04 的 sessionSwitched 信号触发
    emit sessionSwitched(sessionId, d->mSessionStore->readAllRecords(sessionId));
    // 4. 切换后回放 token 统计：从持久化 usage 记录求和重算会话累计值（无则全 0），
    //    避免 UI 拘留上一会话的 token 数值与进度条（Bug2 修复）
    emitTokenUsageForSession(sessionId);
    return true;
}

/**
 * @brief 删除指定会话
 * @param sessionId 会话 ID
 */
void DAAgentModule::deleteSession(const QString& sessionId)
{
    DA_D(d);
    d->mSessionStore->deleteSession(sessionId);
    if (d->mCurrentSessionId == sessionId) {
        d->mCurrentSessionId.clear();  // 删当前会话后回归无活跃
        resetCumulativeTokens();       // 清零累计，避免残留被下一会话误用
    }
    emit sessionListChanged(listSessionsForUI());  // 契约3：带 payload
}

/**
 * @brief 重命名指定会话
 * @param sessionId 会话 ID
 * @param title 新标题
 */
void DAAgentModule::renameSession(const QString& sessionId, const QString& title)
{
    DA_D(d);
    d->mSessionStore->renameSession(sessionId, title);
    emit sessionListChanged(listSessionsForUI());  // 契约3：带 payload
}

/**
 * @brief 列出所有会话（供 UI 下拉）
 * @return QVariantList，每元素 QVariantMap
 */
QVariantList DAAgentModule::listSessions() const
{
    DA_DC(d);
    // 供 UI 下拉（全量元数据）
    QVariantList out;
    for (const auto& m : d->mSessionStore->listSessions()) {
        QVariantMap vm;
        vm["id"]          = m.id;
        vm["title"]       = m.title;
        vm["createdAt"]   = m.createdAt;
        vm["updatedAt"]   = m.updatedAt;
        vm["messageCount"] = m.messageCount;
        vm["projectPath"] = m.projectPath;
        out.append(vm);
    }
    return out;
}

/**
 * @brief 生成 sessionListChanged 的 payload（按工程路径过滤）
 * @return QVariantList，每元素 QVariantMap{id,title,updatedAt,messageCount}
 */
QVariantList DAAgentModule::listSessionsForUI() const
{
    DA_DC(d);
    // 契约3：sessionListChanged 的 payload，每元素 QVariantMap{id,title,updatedAt,messageCount}
    // MAJOR4（round-3）：按当前工程路径过滤
    // 会话管理对话框需要 updatedAt/messageCount 展示更多会话信息。
    QVariantList out;
    for (const auto& m : d->mSessionStore->listSessions(d->mCurrentProjectPath)) {
        QVariantMap vm;
        vm["id"]           = m.id;
        vm["title"]        = m.title;
        vm["updatedAt"]    = m.updatedAt;       // ISO8601WithMs, UTC
        vm["messageCount"] = m.messageCount;
        out.append(vm);
    }
    return out;
}

/**
 * @brief 获取当前活跃会话 ID
 * @return 当前会话 ID
 */
QString DAAgentModule::currentSessionId() const
{
    DA_DC(d);
    return d->mCurrentSessionId;
}

/**
 * @brief 导出当前活跃会话字节（plan-05 工程保存调用）
 * @return id -> jsonl 字节
 */
QHash<QString, QByteArray> DAAgentModule::exportActiveSessions() const
{
    DA_DC(d);
    // 一期：导出当前活跃会话单条（总纲 D3：保存工程时复制活跃会话进 zip）
    if (d->mCurrentSessionId.isEmpty()) return {};
    return d->mSessionStore->exportSessionFiles({d->mCurrentSessionId});
}

/**
 * @brief 从工程 zip 加载会话文件（plan-05 工程加载调用）
 * @param files id -> jsonl 字节
 * @param projectPath 绑定工程路径
 */
void DAAgentModule::loadSessionsFromProject(const QHash<QString, QByteArray>& files, const QString& projectPath)
{
    DA_D(d);
    // 契约4：projectPath 由 plan-05 executeLoad 回调传入（先 setCurrentProjectPath 再调本方法），
    // 标记导入会话工程路径
    d->mSessionStore->importSessionFiles(files, projectPath);
    // Bug1 加固：导入工程会话后，若全局 last_active 指针未指向本工程的会话
    // （常因打开工程前游离会话活动覆盖了指针），把它指向导入会话中最新者，
    // 使后续 restoreLastActiveSession(P) 的指针命中分支生效。
    // listSessions(projectPath) 已按 updatedAt 倒序，取首个即最新。
    if (!files.isEmpty()) {
        QString currentPtr = d->mSessionStore->lastActiveSession(projectPath);
        if (currentPtr.isEmpty()) {
            QVector<DAAgentSessionStore::SessionMeta> bound =
                d->mSessionStore->listSessions(projectPath);
            if (!bound.isEmpty()) {
                d->mSessionStore->setLastActive(bound.first().id, projectPath);
            }
        }
    }
    emit sessionListChanged(listSessionsForUI());  // 契约3：带 payload
}

/**
 * @brief 设置当前工程路径
 * @param path 工程路径
 */
void DAAgentModule::setCurrentProjectPath(const QString& path)
{
    DA_D(d);
    // CRITICAL（round-3）：已提升为 DAAgentInterface 纯虚，DAAppProject（L5）经
    // core()->getAgentInterface() 多态调用（onProjectLoaded / 工程关闭时注入），
    // 不依赖 qobject_cast。
    d->mCurrentProjectPath = path;
}

/**
 * @brief 注册内置 agent 提示词（委托 DAAgentManager）
 * @param name 提示词标题（文件名）
 * @param content 提示词正文
 */
void DAAgentModule::registerBuiltinAgent(const QString& name, const QString& content)
{
    DA_D(d);
    if (d->mAgentManager) {
        d->mAgentManager->registerBuiltin(name, content);
        d->mAgentManager->loadAgents();
    }
}

/**
 * @brief 按标题执行 agent：查提示词→校验 LLM 配置→显示 dock 并发送消息
 * @param title agent 标题
 * @return 成功触发返回 true，未找到提示词或 LLM 未配置返回 false
 */
bool DAAgentModule::runAgent(const QString& title)
{
    DA_D(d);
    if (!d->mAgentManager) {
        return false;
    }
    const DAAgentPrompt* a = d->mAgentManager->findAgent(title);
    if (!a || a->content.isEmpty()) {
        return false;
    }
    QJsonObject config = getLLMConfig();
    if (config.value("base_url").toString().isEmpty() ||
        config.value("api_key").toString().isEmpty() ||
        config.value("model").toString().isEmpty()) {
        daWarning << tr("LLM is not configured, skip agent analysis. "
                        "Please configure LLM in settings first.");  //cn:LLM 未配置，跳过 Agent 分析，请先在设置中配置 LLM
        return false;
    }
    showDockWidget();
    sendMessage(a->content);
    return true;
}

/**
 * @brief 获取提示词库操作回调
 * @return DAAgentPromptOps 指针（所有权归 DAAgentModule，调用方不销毁）
 */
DAAgentPromptOps* DAAgentModule::agentPromptOps()
{
    DA_D(d);
    return d->mAgentManager;
}

/**
 * @brief 包装 store.setSessionProjectPath，供 plan-05 saveAs/save 成功后更新当前会话工程路径
 * @param path 工程路径
 */
void DAAgentModule::setSessionProjectPathForCurrent(const QString& path)
{
    DA_D(d);
    // 契约5：包装 store.setSessionProjectPath，供 plan-05 saveAs/save 成功后更新当前会话工程路径
    if (d->mCurrentSessionId.isEmpty()) return;
    // MAJOR2（round-4）：同步更新 m_currentProjectPath，使 saveAs 后 listSessionsForUI
    // 按新路径过滤能看到当前会话
    d->mCurrentProjectPath = path;
    d->mSessionStore->setSessionProjectPath(d->mCurrentSessionId, path);
    // MAJOR（round-3，from plan-05）：saveAs 后更新 last_active.json 指针的 projectPath，
    // 使打开工程自动恢复生效
    d->mSessionStore->setLastActive(d->mCurrentSessionId, path);
}

/**
 * @brief 清理旧会话（读 ini 配置的 max_sessions/session_retention_days）
 */
void DAAgentModule::cleanupSessions()
{
    DA_D(d);
    // 配置 key 由 plan-06 定义（agent/max_sessions 默认 20、agent/session_retention_days 默认 30）；
    // 读法复用 startAgentInternal/getLLMConfig 现有的 agent-config.ini QSettings 访问模式
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    int maxCount = s.value("agent/max_sessions", 20).toInt();
    int retentionDays = s.value("agent/session_retention_days", 30).toInt();
    d->mSessionStore->cleanupOldSessions(maxCount, retentionDays, d->mCurrentSessionId);  // 跳过当前活跃 + lastActive
}

/**
 * @brief 启动/打开工程后初始化会话 UI——填充下拉列表但不自动恢复上次会话
 */
void DAAgentModule::restoreLastActiveSession()
{
    DA_D(d);
    // 始终以全新对话开始，不自动恢复上次会话。
    // 历史会话仍填充到 UI 下拉列表，用户可通过下拉或会话管理器手动切换。
    // 用户首次发消息时由 sendMessage 懒创建绑定 m_currentProjectPath 的新会话。
    emit sessionListChanged(listSessionsForUI());  // 契约3：启动/开工程时填充 UI 下拉
    d->mCurrentSessionId.clear();
    resetCumulativeTokens();  // 清零累计，始终以全新对话开始
    emit sessionCleared();  // 清空聊天区、复位 token 统计、清空标题
}

// ===========================================================================
// 持久化私有辅助方法（plan-03）
// 构造总纲 T6 格式的 QJsonObject，追加写盘。
// uuid=QUuid::createUuid().toString(QUuid::WithoutBraces)
// timestamp=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
// parent_uuid 一期固定 null
// ===========================================================================

/**
 * @brief 构造 tool_call 记录并追加写盘
 * @param sid 会话 ID
 * @param tool 工具名称
 * @param args 工具调用参数
 * @return 本条记录 uuid
 */
QString DAAgentModule::appendToolCallRecord(const QString& sid, const QString& tool, const QJsonObject& args)
{
    DA_D(d);
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    // tool_calls 数组元素：{name, args, id}
    QJsonObject tc;
    tc["name"] = tool;
    tc["args"] = args;
    tc["id"]   = uuid;

    QJsonObject msg;
    msg["role"]       = "ai";
    msg["content"]    = "";
    msg["tool_calls"] = QJsonArray{tc};

    QJsonObject record;
    record["uuid"]        = uuid;
    record["parent_uuid"] = QJsonValue::Null;
    record["session_id"]  = sid;
    record["timestamp"]   = now;
    record["type"]        = "assistant";
    record["message"]     = msg;

    d->mSessionStore->appendRecord(sid, record);
    return uuid;  // 契约6：返回本条记录 uuid，调用方 enqueue
}

/**
 * @brief 构造 tool_result 记录并追加写盘
 * @param sid 会话 ID
 * @param toolCallId 工具调用 ID
 * @param content 记录内容（明文 QString）
 */
void DAAgentModule::appendToolResultRecord(const QString& sid, const QString& toolCallId, const QString& content)
{
    DA_D(d);
    // MAJOR（round-4，from plan-04）：content 统一明文 QString
    // （agentToolResult 传 result 的 JSON 字符串，userAnswerSelected 传 answer 明文）
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QJsonObject msg;
    msg["role"]          = "tool";
    msg["content"]       = content;
    msg["tool_call_id"]  = toolCallId;

    QJsonObject record;
    record["uuid"]        = uuid;
    record["parent_uuid"] = QJsonValue::Null;
    record["session_id"]  = sid;
    record["timestamp"]   = now;
    record["type"]        = "tool_result";
    record["message"]     = msg;

    d->mSessionStore->appendRecord(sid, record);
}

/**
 * @brief 构造 assistant 记录并追加写盘
 * @param sid 会话 ID
 * @param text assistant 消息文本
 * @param toolCalls 工具调用数组
 */
void DAAgentModule::appendAssistantRecord(const QString& sid, const QString& text, const QJsonArray& toolCalls)
{
    DA_D(d);
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QJsonObject msg;
    msg["role"]    = "ai";
    msg["content"] = text;
    if (!toolCalls.isEmpty()) {
        msg["tool_calls"] = toolCalls;
    }

    QJsonObject record;
    record["uuid"]        = uuid;
    record["parent_uuid"] = QJsonValue::Null;
    record["session_id"]  = sid;
    record["timestamp"]   = now;
    record["type"]        = "assistant";
    record["message"]     = msg;

    d->mSessionStore->appendRecord(sid, record);
}

/**
 * @brief 构造 usage 记录并追加写盘
 * @param sid 会话 ID
 * @param inT 输入 token 数
 * @param outT 输出 token 数
 * @param tot 总 token 数
 * @param src 来源标识
 */
void DAAgentModule::appendUsageRecord(const QString& sid, int inT, int outT, int tot, const QString& src)
{
    DA_D(d);
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QJsonObject usageMeta;
    usageMeta["input_tokens"]  = inT;
    usageMeta["output_tokens"] = outT;
    usageMeta["total_tokens"]  = tot;
    usageMeta["source"]        = src;

    QJsonObject record;
    record["uuid"]         = uuid;
    record["parent_uuid"]  = QJsonValue::Null;
    record["session_id"]   = sid;
    record["timestamp"]    = now;
    record["type"]         = "usage";
    record["message"]      = QJsonObject{};  // usage 记录 message 为空
    record["usage_metadata"] = usageMeta;

    d->mSessionStore->appendRecord(sid, record);
}

/**
 * @brief 构造 user 记录对象（不写盘，供调用方 appendRecord）
 * @param text 用户消息文本
 * @return 完整记录对象
 */
QJsonObject DAAgentModule::makeUserRecord(const QString& text) const
{
    DA_DC(d);
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QJsonObject msg;
    msg["role"]    = "human";
    msg["content"] = text;

    QJsonObject record;
    record["uuid"]        = uuid;
    record["parent_uuid"] = QJsonValue::Null;
    record["session_id"]  = d->mCurrentSessionId;
    record["timestamp"]   = now;
    record["type"]        = "user";
    record["message"]     = msg;
    return record;
}

/**
 * @brief 从 agent-config.ini 读 context_window
 * @return context_window 值（默认 128000）
 */
int DAAgentModule::readContextWindow() const
{
    // 从 agent-config.ini 读 context_window（默认 128000，与 getLLMConfig 一致），
    // 供 agentUsage lambda 与 emitTokenUsageForSession 复用，
    // 避免重复 QSettings 构造与魔法数字散落
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    return s.value("agent/context_window", 262144).toInt();
}

/**
 * @brief 扫描会话持久化 usage 记录求和，emit tokenUsageUpdated（会话累计值）
 * @param sid 会话 ID
 *
 * 切换会话时从 JSONL 重算会话累计 token（所有真实 usage 记录之和，含 summary），
 * 同步刷新累计成员，使 UI 显示该会话的总消耗。streaming_estimate 不持久化故不参与。
 * 无记录则全 0（仍带真实 context_window，UI 显示 tokens: 0 / 窗口、进度条 0%）。
 */
void DAAgentModule::emitTokenUsageForSession(const QString& sid)
{
    DA_D(d);
    // 先清零累计成员，再从持久化 usage 记录求和重算（switchSession 由此恢复累计态）
    d->mCumulativeInTokens = 0;
    d->mCumulativeOutTokens = 0;
    d->mCumulativeTotalTokens = 0;
    QString src;
    if (!sid.isEmpty()) {
        QVector<QJsonObject> records = d->mSessionStore->readAllRecords(sid);
        for (const QJsonObject& obj : std::as_const(records)) {
            if (obj.value("type").toString() != "usage") continue;
            QJsonObject meta = obj.value("usage_metadata").toObject();
            d->mCumulativeInTokens   += meta.value("input_tokens").toInt(0);
            d->mCumulativeOutTokens  += meta.value("output_tokens").toInt(0);
            d->mCumulativeTotalTokens += meta.value("total_tokens").toInt(0);
            src = meta.value("source").toString();  // 取最后一条 source 作展示
        }
    }
    emit tokenUsageUpdated(d->mCumulativeInTokens, d->mCumulativeOutTokens,
                           d->mCumulativeTotalTokens, readContextWindow(), src);
}

/**
 * @brief 会话累计 token 清零（新建/删除当前/恢复时调用）
 *
 * 配合 mCurrentSessionId 的变更点：新建会话（0 消耗）、删除当前会话、
 * 启动/开工程恢复（始终以全新对话开始）。switchSession 不调用本方法——
 * 它经 emitTokenUsageForSession 先清零再从 JSONL 重算恢复累计态。
 */
void DAAgentModule::resetCumulativeTokens()
{
    DA_D(d);
    d->mCumulativeInTokens = 0;
    d->mCumulativeOutTokens = 0;
    d->mCumulativeTotalTokens = 0;
}

} // namespace DA
