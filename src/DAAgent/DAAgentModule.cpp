// DAAgentModule.cpp
#include "DAAgentModule.h"
#include "DAAgentBridge.h"
#include "DAAgentSessionStore.h"
#include "DAAbstractAgentTool.h"
#include "DAAgentInterface.h"
#include "DACoreInterface.h"
#include "DAPyInterpreter.h"
#include "DADir.h"
#include "DALogCategory.h"
// Platform built-in tools (plan-05)
#include "tools/DAAgentToolListData.h"
#include "tools/DAAgentToolDataInfo.h"
#include "tools/DAAgentToolQueryData.h"
#include "tools/DAAgentToolColumnStats.h"
#include "tools/DAAgentToolExportData.h"
#include "tools/DAAgentToolCreateChart.h"
#include "tools/DAAgentToolAddCurve.h"
#include "tools/DAAgentToolSetChartStyle.h"
#include "tools/DAAgentToolAddAnnotation.h"
#include "tools/DAAgentToolAddRegion.h"
#include "tools/DAAgentToolCreateSubplots.h"
#include "tools/DAAgentToolSaveChartImage.h"
#include "tools/DAAgentToolListFigures.h"
#include "tools/DAAgentToolReadFile.h"
#include "tools/DAAgentToolWriteFile.h"
#include "tools/DAAgentToolSaveReport.h"
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

namespace DA
{

DAAgentModule::DAAgentModule(DACoreInterface* core, QObject* parent)
    : DAAgentInterface(parent), m_core(core) {}

DAAgentModule::~DAAgentModule()
{
    // CRITICAL1：m_sessionStore 非 QObject 无 Qt 父子所有权，需手动 delete。
    // m_bridge 是 QObject 子对象，parent=this，由 Qt 自动释放，不在此 delete。
    delete m_sessionStore;
}

void DAAgentModule::initialize(DACoreInterface* core)
{
    m_core = core;

    // Module 不创建也不持有 DAAgentDockWidget——Dock 由 DAAppDockingArea::
    // buildDockingArea() 创建；Dock 信号链（接口信号↔Dock 槽/信号）由 DAAppController
    // 在 initialize() 经接口直接 connect（plan-02 决策 D3b）。本模块只负责 agent
    // 框架逻辑 + Bridge 信号转发 + 持久化 lambda，不依赖 DAGui。

    // 创建 Bridge（不依赖 Dock 存在）
    m_bridge = new DAAgentBridge(this);

    // 把 Bridge 的 agent 生命周期信号转发到 DAAgentInterface，供 APP 层
    // （DAAppController）connect 到 Dock。注意：agentUsage 不直接转发——
    // Module 内部 lambda（见 connectSignals）会补 context_window 后以
    // tokenUsageUpdated 暴露。此转发为 Bridge→this 接口信号（PMF 到 this），
    // 与 Dock 无关。
    connect(m_bridge, &DAAgentBridge::agentToken, this, &DAAgentInterface::agentToken);
    connect(m_bridge, &DAAgentBridge::agentMessageComplete, this, &DAAgentInterface::agentMessageComplete);
    connect(m_bridge, &DAAgentBridge::agentToolCall, this, &DAAgentInterface::agentToolCall);
    connect(m_bridge, &DAAgentBridge::agentToolResult, this, &DAAgentInterface::agentToolResult);
    connect(m_bridge, &DAAgentBridge::agentQuestion, this, &DAAgentInterface::agentQuestion);
    connect(m_bridge, &DAAgentBridge::agentError, this, &DAAgentInterface::agentError);
    connect(m_bridge, &DAAgentBridge::agentReady, this, &DAAgentInterface::agentReady);
    connect(m_bridge, &DAAgentBridge::agentBusy, this, &DAAgentInterface::agentBusy);
    connect(m_bridge, &DAAgentBridge::agentDone, this, &DAAgentInterface::agentDone);
    connect(m_bridge, &DAAgentBridge::agentSessionLoaded, this, &DAAgentInterface::agentSessionLoaded);

    // CRITICAL1：创建会话持久化层（非 QObject 无参构造，不传 parent）。
    // 目录就绪由 store 内部 DADir::getAppDataPath("sessions") mkpath。
    m_sessionStore = new DAAgentSessionStore();

    // 连接 Bridge→Module 的持久化/状态 lambda（connectSignals 不再连 Dock，
    // 守卫改为仅判 m_bridge；Dock 连接已由 DAAppController 经接口完成）。
    connectSignals();

    // 注册平台内置工具（plan-05；未完成时为空体）
    registerBuiltinTools();
}

void DAAgentModule::registerTool(DAAbstractAgentTool* tool)
{
    QString name = tool->getToolSpec()["name"].toString();
    m_tools[name] = tool;
    if (m_bridge) m_bridge->setTools(m_tools);
}

void DAAgentModule::registerSystemPrompt(const QString& name, const QString& content)
{
    m_systemPrompts[name] = content;
}

QString DAAgentModule::assembleSystemPrompt() const
{
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
    for (auto it = m_systemPrompts.begin(); it != m_systemPrompts.end(); ++it) {
        parts << it.value();
    }
    return parts.join("\n\n");
}

QJsonArray DAAgentModule::assembleToolSpecs() const
{
    QJsonArray specs;
    for (auto* tool : m_tools) {
        specs.append(tool->getToolSpec());
    }
    return specs;
}

void DAAgentModule::sendMessage(const QString& text)
{
    // 注意：不要在此 emit agentBusy(true)——DAAgentInterface 未声明 agentBusy 信号，
    // 此处 emit 无法编译。busy 状态改由 DAAgentBridge::sendMessage() 统一发射：
    // Bridge 的 agentBusy 信号已在 connectSignals() 中连接到 DAAgentDockWidget::onAgentBusy。
    // 契约7 + MAJOR3（round-3）：经 Module::createSession（内部 store.createSession +
    // 设 m_currentSessionId + setLastActive + emit sessionListChanged，不 emit sessionCreated
    // ——避免触发 plan-04 onSessionCreated 的 clearChat 擦除刚显示的用户消息）。
    // UI "+" 走 newSession()（有 sessionCreated）。m_currentSessionId = createSession() 为
    // 冗余赋值（createSession 内部已设），保留以契约7 的形式。
    if (m_currentSessionId.isEmpty()) {
        m_currentSessionId = createSession();
    }
    // 发送前持久化 user 消息
    m_sessionStore->appendRecord(m_currentSessionId, makeUserRecord(text));
    // 首条 user 消息定简短标题；标题变更时刷新 UI 下拉（否则 combo 一直显示 (untitled)）
    if (m_sessionStore->ensureTitle(m_currentSessionId)) {
        emit sessionListChanged(listSessionsForUI());
    }
    if (!m_bridge->isRunning()) {
        // 懒启动
        startAgentInternal();
    }
    m_bridge->sendMessage(text);
}

void DAAgentModule::stop()
{
    if (m_bridge && m_bridge->isRunning()) {
        m_bridge->requestStop();
    }
}

void DAAgentModule::sendUserAnswer(const QString& answer)
{
    // 持久化：吸收原 connectSignals 的 dock::userAnswerSelected 持久化 lambda
    // （appendToolResultRecord），plan-02 删除 Dock 持有后由本方法体承接。
    // DAAppController 经 dock::userAnswerSelected → interface::sendUserAnswer
    // 信号→方法 PMF 连接，单次调用即完成持久化 + 转发 Bridge（无双重）。
    if (!m_currentSessionId.isEmpty() && !m_pendingToolCallUuids.isEmpty()) {
        QString tcid = m_pendingToolCallUuids.dequeue();
        appendToolResultRecord(m_currentSessionId, tcid, answer);
    }
    if (m_bridge) {
        m_bridge->sendUserAnswer(answer);
    }
}

void DAAgentModule::startAgentInternal()
{
    // 获取 LLM 配置（plan-06 提供真实实现）
    QJsonObject config = getLLMConfig();

    // 通过 detect 方法解析路径（不依赖 config 是否包含这些键）
    QString pythonExe = detectPythonExePath();
    QString scriptPath = detectAgentScriptPath();

    // 路径缺失时提前返回并报错——daCritical 会路由到 UI 日志窗口
    if (pythonExe.isEmpty()) {
        daCritical << tr("无法找到 Python 解释器路径，请在设置页配置 Python 解释器");
        return;
    }
    if (scriptPath.isEmpty() || !QFile::exists(scriptPath)) {
        daCritical << tr("无法找到 agent_runner.py 路径: %1").arg(scriptPath);
        return;
    }

    // 读取可配超时(与 DAAgentSettingsWidget 共用 agent-config.ini,默认值一致)
    // 单位:秒→毫秒。ready 超时默认 60s 覆盖 langchain 冷启动导入(~17s)+余量;
    // stop 超时默认 5s 保持原有行为。
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    int readyTimeoutMs = s.value("agent/ready_timeout_sec", 60).toInt() * 1000;
    int stopTimeoutMs   = s.value("agent/stop_timeout_sec", 5).toInt() * 1000;

    // 启动
    m_bridge->startAgent(config, assembleToolSpecs(), assembleSystemPrompt(),
                         pythonExe, scriptPath, readyTimeoutMs, stopTimeoutMs);
}

bool DAAgentModule::isRunning() const
{
    // 转发 Bridge 的子进程运行状态——避免桩始终返回 false，
    // 导致外部（如 UI 忙状态判断）在子进程活跃期间误判为未运行
    return m_bridge ? m_bridge->isRunning() : false;
}

void DAAgentModule::registerBuiltinTools()
{
    // Data tools (5)
    registerTool(new DAAgentToolListData(m_core, this));
    registerTool(new DAAgentToolDataInfo(m_core, this));
    registerTool(new DAAgentToolQueryData(m_core, this));
    registerTool(new DAAgentToolColumnStats(m_core, this));
    registerTool(new DAAgentToolExportData(m_core, this));
    // Plotting tools (8)
    registerTool(new DAAgentToolCreateChart(m_core, this));
    registerTool(new DAAgentToolAddCurve(m_core, this));
    registerTool(new DAAgentToolSetChartStyle(m_core, this));
    registerTool(new DAAgentToolAddAnnotation(m_core, this));
    registerTool(new DAAgentToolAddRegion(m_core, this));
    registerTool(new DAAgentToolCreateSubplots(m_core, this));
    registerTool(new DAAgentToolSaveChartImage(m_core, this));
    registerTool(new DAAgentToolListFigures(m_core, this));
    // File/report tools (3)
    registerTool(new DAAgentToolReadFile(m_core, this));
    registerTool(new DAAgentToolWriteFile(m_core, this));
    registerTool(new DAAgentToolSaveReport(m_core, this));
}

void DAAgentModule::connectSignals()
{
    // plan-02：Dock 信号链已由 DAAppController::initialize() 经接口直接 connect 到
    // DAAgentDockWidget（13 条 interface→dock 槽 + 7 条 dock 信号→interface 方法）。
    // 本函数不再持有/连接 Dock——只保留 Bridge→Module 的持久化/状态 lambda（与 Dock 无关）。
    // Dock 不再是 Module 依赖；Bridge 仍是。
    if (!m_bridge) {
        return;
    }

    // ---- 持久化：对话事件 → JSONL ----
    // assistant 消息完成（纯文本回复）
    connect(m_bridge, &DAAgentBridge::agentMessageComplete, this, [this](const QString& fullText) {
        if (m_currentSessionId.isEmpty()) return;
        appendAssistantRecord(m_currentSessionId, fullText, /*toolCalls=*/{});
    });
    // 工具调用（含 ask_user 提问——ask_user 复用 tool_call 语义）
    connect(m_bridge, &DAAgentBridge::agentToolCall, this, [this](const QString& tool, const QJsonObject& args) {
        if (m_currentSessionId.isEmpty()) return;
        // 契约6：appendToolCallRecord 返回本条记录 uuid，入队供后续 tool_result 按 FIFO 配对 tool_call_id
        m_pendingToolCallUuids.enqueue(appendToolCallRecord(m_currentSessionId, tool, args));
    });
    // 工具结果
    connect(m_bridge, &DAAgentBridge::agentToolResult, this, [this](const QString& /*tool*/, const QJsonObject& result) {
        if (m_currentSessionId.isEmpty()) return;
        // 契约6：FIFO 出队取配对的 tool_call uuid（_rpc_call 串行保证顺序）
        // MAJOR1（round-4）：dequeue 前加 isEmpty 守卫，防空队列未定义行为/崩溃
        // （agentError/switchSession 已 clear 队列后残余 tool_result 信号可达）
        if (m_pendingToolCallUuids.isEmpty()) return;
        QString tcid = m_pendingToolCallUuids.dequeue();
        // MAJOR（round-4，from plan-04）：content 统一明文（result 序列化为 JSON 字符串，
        // 对齐映射表 content:json.dumps(result)）
        appendToolResultRecord(m_currentSessionId, tcid,
                               QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)));
    });
    // token 使用量
    connect(m_bridge, &DAAgentBridge::agentUsage, this, [this](int inT, int outT, int tot, const QString& src) {
        if (m_currentSessionId.isEmpty()) return;
        appendUsageRecord(m_currentSessionId, inT, outT, tot, src);
        // 契约2：查 context_window（从 QSettings agent/context_window 默认 1048576），emit 5 参信号供 plan-04 UI
        QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
        int window = s.value("agent/context_window", 1048576).toInt();
        emit tokenUsageUpdated(inT, outT, tot, window, src);
    });
    // agent 提问（ask_user）——记录为 tool_call，供下一条 answer 配对
    connect(m_bridge, &DAAgentBridge::agentQuestion, this, [this](const QString& text, const QStringList& options, bool multiSelect) {
        if (m_currentSessionId.isEmpty()) return;
        QJsonObject args;
        args["question"]     = text;
        args["options"]      = QJsonArray::fromStringList(options);
        args["multi_select"] = multiSelect;
        // 契约6：入队供下一条 answer 按 FIFO 配对
        m_pendingToolCallUuids.enqueue(appendToolCallRecord(m_currentSessionId, "ask_user", args));
    });
    // 注：userAnswerSelected 的持久化路径已删除——逻辑由 DAAgentModule::sendUserAnswer
    // 方法体承接（plan-01：dequeue m_pendingToolCallUuids + appendToolResultRecord + 转发
    // m_bridge->sendUserAnswer）。DAAppController 经 dock::userAnswerSelected →
    // interface::sendUserAnswer 单次调用即完成持久化+协议转发，无双重持久化。
    // （sendUserAnswer 是方法非信号，PMF 指向虚方法，Qt5+ 合法。）

    // ---- 常驻 ready/busy/done/error 槽（plan-03，替代一次性 QMetaObject::Connection，
    //      避免多次连接泄漏与 ready 永不到达时堆泄漏） ----
    connect(m_bridge, &DAAgentBridge::agentReady, this, [this](const QString&) {
        // MAJOR9：switchSession 懒启动分支缓存 pending 于此，ready 到达后下发 load_session。
        // 覆盖语义天然处理快速连续切换 A→B→C（只保留最后一次 pending）。
        if (!m_pendingLoadSessionId.isEmpty()) {
            m_bridge->sendLoadSession(m_pendingLoadSessionId, m_pendingLoadMessages);
            m_pendingLoadSessionId.clear();
            m_pendingLoadMessages = QJsonArray();
        }
    });
    connect(m_bridge, &DAAgentBridge::agentBusy, this, [this](bool busy) {
        m_agentBusy = busy;
    });
    connect(m_bridge, &DAAgentBridge::agentDone, this, [this]() {
        m_agentBusy = false;
        // 忙碌态切换排队续切（此时已非 busy）
        if (!m_pendingSwitchSessionId.isEmpty()) {
            QString sid = m_pendingSwitchSessionId;
            m_pendingSwitchSessionId.clear();
            switchSession(sid);
        }
    });
    connect(m_bridge, &DAAgentBridge::agentError, this, [this](const QString&) {
        // 兜底清理 pending，避免 ready 永不到达时泄漏
        m_pendingLoadSessionId.clear();
        m_pendingLoadMessages = QJsonArray();
        m_pendingSwitchSessionId.clear();
        // MAJOR1（round-3）：出错时清空队列，避免旧会话残留 uuid 配对新会话
        m_pendingToolCallUuids.clear();
        m_agentBusy = false;
    });
}

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

QString DAAgentModule::detectAgentScriptPath() const
{
    // agent 脚本位于 bin/PyScripts/DAWorkbench/agent/agent_runner.py（plan-02）。
    // DACoreInterface::getPythonScriptsPath() 是静态方法，返回 PyScripts 目录。
    QString scriptsDir = DACoreInterface::getPythonScriptsPath();
    return scriptsDir + "/DAWorkbench/agent/agent_runner.py";
}

QString DAAgentModule::detectSystemPromptPath() const
{
    // 系统提示词 markdown 安装到与 agent_runner.py 同目录，运行时由
    // assembleSystemPrompt() 读取；路径解析复用 getPythonScriptsPath()。
    QString scriptsDir = DACoreInterface::getPythonScriptsPath();
    return scriptsDir + "/DAWorkbench/agent/system_prompt.md";
}

void DAAgentModule::showDockWidget()
{
    // plan-02：Module 不再持有 Dock；Dock 显隐走 ADS 的 setToggleViewAction
    // （由 DAAppController 绑定 ribbon action），本方法为 no-op。
}

void DAAgentModule::hideDockWidget()
{
    // plan-02：同 showDockWidget，no-op。
}

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
    config["context_window"]            = s.value("agent/context_window", 1048576).toInt();
    config["compaction_threshold"]      = s.value("agent/compaction_threshold", 0.85).toDouble();
    config["max_recent_messages"]       = s.value("agent/max_recent_messages", 10).toInt();
    config["tool_result_max_chars"]     = s.value("agent/tool_result_max_chars", 50000).toInt();
    config["tool_result_preview_chars"] = s.value("agent/tool_result_preview_chars", 2000).toInt();
    return config;
}

void DAAgentModule::setLLMConfig(const QJsonObject& config)
{
    // 与 getLLMConfig() 对称的 key 写入 agent-config.ini（可用于运行时覆盖配置）
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    s.setValue("agent/llm_base_url", config.value("base_url").toString());
    s.setValue("agent/llm_model", config.value("model").toString());
    QString apiKey = config.value("api_key").toString();
    if (!apiKey.isEmpty()) {
        // IniFormat 原生支持 QByteArray,加密 blob 直接存储
        s.setValue("agent/llm_api_key", encryptApiKey(apiKey));
    }
}

// ===========================================================================
// 会话管理接口实现（plan-03）
// ===========================================================================
QString DAAgentModule::createSession()
{
    // MAJOR3（round-3）：不 emit sessionCreated，供 sendMessage 自动建会话用
    // （避免触发 plan-04 onSessionCreated 的 clearChat 擦除刚显示的用户消息）。
    // MAJOR2（round-3）：内部设 m_currentSessionId，所有调用方统一受益。
    QString sid = m_sessionStore->createSession(m_currentProjectPath);  // 带 projectPath
    m_currentSessionId = sid;
    m_sessionStore->setLastActive(sid, m_currentProjectPath);
    emit sessionListChanged(listSessionsForUI());  // 契约3：带 payload
    return sid;
}

void DAAgentModule::newSession()
{
    // MAJOR3（round-3）：供 UI "+" 按钮——createSession + emit sessionCreated。
    // sendMessage 自动建会话调 createSession（无 sessionCreated），
    // UI "+" 调 newSession（有 sessionCreated）。
    QString sid = createSession();
    emit sessionCreated(sid);  // 仅此路径触发 UI clearChat
}

bool DAAgentModule::switchSession(const QString& sessionId)
{
    if (sessionId == m_currentSessionId) return false;  // false = 已是当前会话
    // 0. 守忙碌态：上一轮仍在流式输出时，requestStop 并排队，agentDone 后续切，
    //    避免残余 agentToken/agentMessageComplete/agentToolCall/agentToolResult 信号被
    //    持久化 lambda 写入新会话 JSONL（旧会话尾巴污染新会话）。
    if (m_bridge->isRunning() && m_agentBusy) {
        m_pendingSwitchSessionId = sessionId;   // 排队；常驻 agentDone 槽续切
        m_bridge->requestStop();                // 非阻塞停止
        return true;                             // 异步完成，sessionSwitched 在续切时 emit
    }
    // 1. 读历史消息
    QJsonArray messages = m_sessionStore->readMessagesForLoad(sessionId);
    // MAJOR1（round-3）：实际切换前清空队列，丢弃旧会话未完成的 pending 配对
    m_pendingToolCallUuids.clear();
    m_currentSessionId = sessionId;
    m_sessionStore->setLastActive(sessionId, m_currentProjectPath);  // 带工程路径
    // 2. 确保子进程：未运行则懒启动，ready 后由常驻槽发 load_session
    if (!m_bridge->isRunning()) {
        startAgentInternal();
        // 缓存 pending load；由 connectSignals 里建立的常驻 agentReady 槽处理
        // （不再每次 new QMetaObject::Connection，避免快速连续切换时多次连接、堆积
        //  与 ready 永不到达时的堆泄漏）
        m_pendingLoadSessionId = sessionId;
        m_pendingLoadMessages = messages;
    } else {
        m_bridge->sendLoadSession(sessionId, messages);
    }
    // 3. UI 历史重放由 plan-04 的 sessionSwitched 信号触发
    emit sessionSwitched(sessionId, m_sessionStore->readAllRecords(sessionId));
    return true;
}

void DAAgentModule::deleteSession(const QString& sessionId)
{
    m_sessionStore->deleteSession(sessionId);
    if (m_currentSessionId == sessionId) {
        m_currentSessionId.clear();  // 删当前会话后回归无活跃
    }
    emit sessionListChanged(listSessionsForUI());  // 契约3：带 payload
}

void DAAgentModule::renameSession(const QString& sessionId, const QString& title)
{
    m_sessionStore->renameSession(sessionId, title);
    emit sessionListChanged(listSessionsForUI());  // 契约3：带 payload
}

QVariantList DAAgentModule::listSessions() const
{
    // 供 UI 下拉（全量元数据）
    QVariantList out;
    for (const auto& m : m_sessionStore->listSessions()) {
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

QVariantList DAAgentModule::listSessionsForUI() const
{
    // 契约3：sessionListChanged 的 payload，每元素 QVariantMap{id,title}
    // MAJOR4（round-3）：按当前工程路径过滤
    QVariantList out;
    for (const auto& m : m_sessionStore->listSessions(m_currentProjectPath)) {
        QVariantMap vm;
        vm["id"]    = m.id;
        vm["title"] = m.title;
        out.append(vm);
    }
    return out;
}

QString DAAgentModule::currentSessionId() const
{
    return m_currentSessionId;
}

QHash<QString, QByteArray> DAAgentModule::exportActiveSessions() const
{
    // 一期：导出当前活跃会话单条（总纲 D3：保存工程时复制活跃会话进 zip）
    if (m_currentSessionId.isEmpty()) return {};
    return m_sessionStore->exportSessionFiles({m_currentSessionId});
}

void DAAgentModule::loadSessionsFromProject(const QHash<QString, QByteArray>& files, const QString& projectPath)
{
    // 契约4：projectPath 由 plan-05 executeLoad 回调传入（先 setCurrentProjectPath 再调本方法），
    // 标记导入会话工程路径
    m_sessionStore->importSessionFiles(files, projectPath);
    emit sessionListChanged(listSessionsForUI());  // 契约3：带 payload
}

void DAAgentModule::setCurrentProjectPath(const QString& path)
{
    // CRITICAL（round-3）：已提升为 DAAgentInterface 纯虚，DAAppProject（L5）经
    // core()->getAgentInterface() 多态调用（onProjectLoaded / 工程关闭时注入），
    // 不依赖 qobject_cast。
    m_currentProjectPath = path;
}

void DAAgentModule::setSessionProjectPathForCurrent(const QString& path)
{
    // 契约5：包装 store.setSessionProjectPath，供 plan-05 saveAs/save 成功后更新当前会话工程路径
    if (m_currentSessionId.isEmpty()) return;
    // MAJOR2（round-4）：同步更新 m_currentProjectPath，使 saveAs 后 listSessionsForUI
    // 按新路径过滤能看到当前会话
    m_currentProjectPath = path;
    m_sessionStore->setSessionProjectPath(m_currentSessionId, path);
    // MAJOR（round-3，from plan-05）：saveAs 后更新 last_active.json 指针的 projectPath，
    // 使打开工程自动恢复生效
    m_sessionStore->setLastActive(m_currentSessionId, path);
}

void DAAgentModule::cleanupSessions()
{
    // 配置 key 由 plan-06 定义（agent/max_sessions 默认 20、agent/session_retention_days 默认 30）；
    // 读法复用 startAgentInternal/getLLMConfig 现有的 agent-config.ini QSettings 访问模式
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    int maxCount = s.value("agent/max_sessions", 20).toInt();
    int retentionDays = s.value("agent/session_retention_days", 30).toInt();
    m_sessionStore->cleanupOldSessions(maxCount, retentionDays, m_currentSessionId);  // 跳过当前活跃 + lastActive
}

void DAAgentModule::restoreLastActiveSession()
{
    QString sid = m_sessionStore->lastActiveSession(m_currentProjectPath);  // 按工程过滤
    emit sessionListChanged(listSessionsForUI());  // 契约3：启动/开工程时填充 UI 下拉
    if (!sid.isEmpty() && m_sessionStore->hasSession(sid)) {  // CRITICAL2：hasSession 读 index 判断
        // 注：listSessions() 返回 QVector<SessionMeta>，QVector::contains(QString) 类型不匹配
        // 不可编译；故用 hasSession
        switchSession(sid);
    }
}

// ===========================================================================
// 持久化私有辅助方法（plan-03）
// 构造总纲 T6 格式的 QJsonObject，追加写盘。
// uuid=QUuid::createUuid().toString(QUuid::WithoutBraces)
// timestamp=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
// parent_uuid 一期固定 null
// ===========================================================================
QString DAAgentModule::appendToolCallRecord(const QString& sid, const QString& tool, const QJsonObject& args)
{
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

    m_sessionStore->appendRecord(sid, record);
    return uuid;  // 契约6：返回本条记录 uuid，调用方 enqueue
}

void DAAgentModule::appendToolResultRecord(const QString& sid, const QString& toolCallId, const QString& content)
{
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

    m_sessionStore->appendRecord(sid, record);
}

void DAAgentModule::appendAssistantRecord(const QString& sid, const QString& text, const QJsonArray& toolCalls)
{
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

    m_sessionStore->appendRecord(sid, record);
}

void DAAgentModule::appendUsageRecord(const QString& sid, int inT, int outT, int tot, const QString& src)
{
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

    m_sessionStore->appendRecord(sid, record);
}

QJsonObject DAAgentModule::makeUserRecord(const QString& text) const
{
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QJsonObject msg;
    msg["role"]    = "human";
    msg["content"] = text;

    QJsonObject record;
    record["uuid"]        = uuid;
    record["parent_uuid"] = QJsonValue::Null;
    record["session_id"]  = m_currentSessionId;
    record["timestamp"]   = now;
    record["type"]        = "user";
    record["message"]     = msg;
    return record;
}

} // namespace DA
