// DAAgentModule.cpp
#include "DAAgentModule.h"
#include "DAAgentBridge.h"
#include "DAAgentSessionStore.h"
#include "DAAgentPermissionManager.h"
#include "DAAgentManager.h"
#include "DAAgentSubagentManager.h"
#include "DAAgentSubagentDef.h"
#include "DAAgentPromptOps.h"
#include "DAAbstractAgentTool.h"
#include "DAAgentToolSpecJson.h"
#include "DAAgentInterface.h"
#include "DAAgentConfig.h"
#include "DACoreInterface.h"
#include "DAPyInterpreter.h"
#include "DADir.h"
#include "DALogCategory.h"
// Platform built-in tools moved to plugins/DAAgentTools plugin (plan-03)
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStringList>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include <QVariantList>
#include <QVariantMap>

// availableModelsChanged 信号载荷跨线程安全（queued connection 时需要 metatype）
DA_AUTO_REGISTER_META_TYPE(DA::DAAgentModelRef)
DA_AUTO_REGISTER_META_TYPE(QList< DA::DAAgentModelRef >)

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
    DAAgentManager* mAgentManager = nullptr;          ///< 提示词库管理器（QObject，parent=this）
    DAAgentSubagentManager* mSubagentManager = nullptr; ///< 子 agent 定义库（QObject，parent=this）
    QMap<QString, DAAbstractAgentTool*> mTools;        ///< tool name → impl
    QHash<QString, QString> mSystemPrompts;            ///< prompt name → content
    // ---- 插件热插拔：注册方追踪（provider → 注销索引） ----
    QHash<QString, QObject*> mToolProviders;           ///< tool name → 注册方（注册时记录 tool->parent()）
    QHash<QString, QObject*> mSystemPromptProviders;   ///< prompt name → 注册方（registerSystemPrompt 传入，可为 nullptr）
    DAAgentSessionStore* mSessionStore = nullptr;  ///< 非 QObject 无参构造；initialize() 内 new、析构显式 delete
    QString mCurrentSessionId;                     ///< 当前活跃会话（UI 归属，唯一）
    QString mCurrentProjectPath;                   ///< 由 DAAppController::setCurrentProjectPath 注入

    // ---- 会话桥管理（concurrent-sessions：多子进程并发会话） ----
    QHash<QString, DAAgentBridge*> mSessionBridges;    ///< 有子进程的会话 → 桥
    DAAgentBridge* mIdleBridge = nullptr;              ///< 预热未绑定桥（auto_prestart，0..1 个）
    bool mIdleBridgeReady = false;                     ///< 预热桥是否已收到 ready
    QHash<QString, QQueue<QString>> mPendingToolCallUuids;  ///< 会话 → tool_call/question 配对 FIFO
    QHash<QString, bool> mSessionBusy;                 ///< 会话 → 本轮是否进行中
    QHash<QString, bool> mSessionStarting;             ///< 会话 → 子进程启动中
    QHash<QString, bool> mSessionError;                ///< 会话 → 最近一次 agentError 未消化
    QHash<QString, int> mCumulativeInTokens;           ///< 会话累计输入 token（压缩不重置）
    QHash<QString, int> mCumulativeOutTokens;          ///< 会话累计输出 token
    QHash<QString, int> mCumulativeTotalTokens;        ///< 会话累计总 token
    // ---- 挂起交互缓存（后台会话切回时重发） ----
    struct PendingQuestion {
        QString text;
        QStringList options;
        bool multiSelect = false;
    };
    QHash<QString, PendingQuestion> mPendingQuestions;           ///< 会话 → 待回答 ask_user
    QHash<QString, QString> mApprovalSessionByCallId;            ///< callId → 会话（审批路由）
    QHash<QString, QJsonArray> mPendingApprovalRequests;         ///< 会话 → 待审批载荷[{callId,toolName,args}]

    // ---- 权限层（permission-layer P1） ----
    DAAgentPermissionManager* mPermissionManager = nullptr;  ///< 权限引擎（非 QObject，析构显式 delete）
    QString mScriptWorkspaceDir;                   ///< 脚本工作区根（${workspace}），由 L5 注入

    // ---- 配置（agent-config.json 领域模型） ----
    // initialize() 最先 load()（含旧 ini→json 迁移），运行期所有配置读写均经此
    // 内存模型（不再逐调用重读文件）；变更经接口方法同步并 save() 落盘。
    DAAgentConfig mConfig;
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
    // 权限引擎同为非 QObject，手动 delete（镜像 SessionStore 惯例）
    delete d->mPermissionManager;
}

/**
 * @brief 使用核心接口初始化模块
 * @param core 核心接口指针
 */
void DAAgentModule::initialize(DACoreInterface* core)
{
    DA_D(d);
    d->mCore = core;

    // 配置领域模型最先加载（agent-config.json + 旧 agent-config.ini 一次性迁移），
    // 后续 Bridge/PermissionManager 均消费此内存模型
    d->mConfig.load();

    // Module 不创建也不持有 DAAgentDockWidget——Dock 由 DAAppDockingArea::
    // buildDockingArea() 创建；Dock 信号链（接口信号↔Dock 槽/信号）由 DAAppController
    // 在 initialize() 经接口直接 connect（plan-02 决策 D3b）。本模块只负责 agent
    // 框架逻辑 + Bridge 信号转发 + 持久化 lambda，不依赖 DAGui。
    //
    // concurrent-sessions：Module 不再持有单一 Bridge——每运行中会话一个桥
    //（attachBridge 按会话路由：持久化写桥所属会话、UI 接口信号仅活跃会话转发），
    // 另有至多一个预热未绑定桥（prestartAgent）。桥在 createBridgeForSession /
    // adoptOrStartBridge 时创建并注入权限引擎。

    // ---- 权限层（permission-layer P1） ----
    // 创建权限引擎（非 QObject 无参构造，镜像 SessionStore）并加载/播种配置。
    // 注入共享的配置模型（权限 5 标量存于 agent-config.json permission 分组）；
    // 引擎在桥创建时经 setPermissionManager 注入各 Bridge（executeTool 前置门，
    // C++ 唯一执法点，A1）
    d->mPermissionManager = new DAAgentPermissionManager(&d->mConfig);
    d->mPermissionManager->load();

    // CRITICAL1：创建会话持久化层（非 QObject 无参构造，不传 parent）。
    // 目录就绪由 store 内部 DADir::getAppDataPath("sessions") mkpath。
    d->mSessionStore = new DAAgentSessionStore();

    // 提示词库管理器：播种通用默认 agent、加载用户已有提示词。
    // m_agentManager 为 QObject，parent=this，随 Module 释放。
    d->mAgentManager = new DAAgentManager(this);
    d->mAgentManager->ensureDefaultAgent();
    d->mAgentManager->loadAgents();
    // 定义列表变化透传给接口，供插件 initialize() 注入内置 agent 后
    // Ribbon gallery 兜底刷新（gallery 首次构建早于插件加载）
    connect(d->mAgentManager, &DAAgentManager::agentListChanged,
            this, &DAAgentInterface::agentListChanged);

    // 子 agent 定义库（子 agent 一期）：播种内置 explore（仅文件缺失时写入）、
    // 加载用户已有定义。mSubagentManager 为 QObject，parent=this，随 Module 释放。
    d->mSubagentManager = new DAAgentSubagentManager(this);
    d->mSubagentManager->ensureDefaultSubagents();
    d->mSubagentManager->loadSubagents();
    // 定义列表变化透传给接口，供管理 UI 刷新
    connect(d->mSubagentManager, &DAAgentSubagentManager::subagentListChanged,
            this, &DAAgentInterface::subagentListChanged);

    // concurrent-sessions：Bridge 信号路由已迁移至 attachBridge（每桥按会话连接），
    // 旧的 connectSignals（单桥全局连接）删除。

    // 平台内置工具由 plugins/DAAgentTools 插件在 initialize() 经
    // agent->registerTool 注册（plan-03 搬迁、plan-04 删除本模块的内置工具
    // 注册方法），本模块不再注册工具。
}

/**
 * @brief 注册工具供 agent 使用
 *
 * 注册期校验：工具名为空或与已注册工具重复时拒绝注册并返回 false
 * （此前重复注册会静默覆盖）；工具名不符合 snake_case 约定时仅告警。
 * @param tool 工具实现指针
 * @return 注册成功返回 true，校验失败返回 false
 */
bool DAAgentModule::registerTool(DAAbstractAgentTool* tool)
{
    DA_D(d);
    if (nullptr == tool) {
        qWarning("DAAgentModule::registerTool: null tool pointer, registration rejected");
        return false;
    }
    const QString name = tool->getToolSpec().name;
    if (name.trimmed().isEmpty()) {
        qWarning("DAAgentModule::registerTool: tool spec has an empty name, registration rejected");
        return false;
    }
    if (d->mTools.contains(name)) {
        qWarning("DAAgentModule::registerTool: tool '%s' is already registered, registration rejected",
                 qPrintable(name));
        return false;
    }
    static const QRegularExpression reSnakeCase(QStringLiteral("^[a-z][a-z0-9_]*$"));
    if (!reSnakeCase.match(name).hasMatch()) {
        qWarning("DAAgentModule::registerTool: tool name '%s' is not lower snake_case (convention only, registered)",
                 qPrintable(name));
    }
    d->mTools[name] = tool;
    // 插件热插拔：记录注册方 provider。DAAbstractAgentTool 本身不是 QObject，
    // 需 dynamic_cast 横转到 QObject 面（实现类如 DAAgentToolBase 多继承两者）再取 parent。
    // 约定：插件注册的工具必须以插件对象为 parent，否则无法按 provider 注销，
    // 插件卸载后会留下悬空指针
    QObject* toolObject = dynamic_cast< QObject* >(tool);
    QObject* provider   = toolObject ? toolObject->parent() : nullptr;
    if (nullptr == provider) {
        qWarning("DAAgentModule::registerTool: tool '%s' has no QObject parent (provider), "
                 "it cannot be unregistered by provider when its plugin is unloaded",
                 qPrintable(name));
    }
    d->mToolProviders[name] = provider;
    // concurrent-sessions：同步到全部存活桥（后创建的桥在 attachBridge 时注入最新工具表）
    forEachLiveBridge([this](DAAgentBridge* b) { b->setTools(d_func()->mTools); });
    return true;
}

/**
 * @brief 注册命名系统提示词片段（无provider版本，委托到带provider重载）
 * @param name 提示词片段名称
 * @param content 提示词内容
 */
void DAAgentModule::registerSystemPrompt(const QString& name, const QString& content)
{
    registerSystemPrompt(name, content, nullptr);
}

/**
 * @brief 注册命名系统提示词片段
 * @param name 提示词片段名称
 * @param content 提示词内容
 * @param provider 注册方对象（插件热卸载时按 provider 注销），可为 nullptr
 */
void DAAgentModule::registerSystemPrompt(const QString& name, const QString& content, QObject* provider)
{
    DA_D(d);
    d->mSystemPrompts[name] = content;
    d->mSystemPromptProviders[name] = provider;
}

/**
 * @brief 注销指定 provider（插件对象）注册的全部工具
 *
 * 插件热卸载前由 APP 层调用，防止插件实例销毁后 mTools 留下悬空指针。
 * 工具对象所有权归插件（parent 关系），此处只移除宿主注册表指针，不 delete 工具。
 * 注销后同步到全部存活桥（与 registerTool 相同的 setTools 热更新路径）
 * @param provider 注册方对象指针
 * @return 注销的工具数量
 */
int DAAgentModule::unregisterToolsByProvider(QObject* provider)
{
    DA_D(d);
    if (nullptr == provider) {
        return 0;
    }
    QStringList removedNames;
    for (auto it = d->mToolProviders.begin(); it != d->mToolProviders.end();) {
        if (it.value() == provider) {
            removedNames.append(it.key());
            it = d->mToolProviders.erase(it);
        } else {
            ++it;
        }
    }
    for (const QString& name : std::as_const(removedNames)) {
        d->mTools.remove(name);
    }
    if (!removedNames.isEmpty()) {
        qInfo("DAAgentModule::unregisterToolsByProvider: %d tool(s) unregistered for provider %p",
              removedNames.size(),
              static_cast<void*>(provider));
        forEachLiveBridge([this](DAAgentBridge* b) { b->setTools(d_func()->mTools); });
    }
    return removedNames.size();
}

/**
 * @brief 注销指定 provider 注册的全部系统提示词片段
 *
 * 已启动的 agent 子进程持有旧系统提示词，注销后对新会话/重启的子进程生效
 * @param provider 注册方对象指针
 * @return 注销的提示词片段数量
 */
int DAAgentModule::unregisterSystemPromptsByProvider(QObject* provider)
{
    DA_D(d);
    if (nullptr == provider) {
        return 0;
    }
    int removedCount = 0;
    for (auto it = d->mSystemPromptProviders.begin(); it != d->mSystemPromptProviders.end();) {
        if (it.value() == provider) {
            d->mSystemPrompts.remove(it.key());
            it = d->mSystemPromptProviders.erase(it);
            ++removedCount;
        } else {
            ++it;
        }
    }
    if (removedCount > 0) {
        qInfo("DAAgentModule::unregisterSystemPromptsByProvider: %d prompt(s) unregistered for provider %p",
              removedCount,
              static_cast<void*>(provider));
    }
    return removedCount;
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
            daWarning << tr("Agent system prompt file is empty, fallback to built-in default: %1")  //cn:Agent 系统提示词文件为空，回退到内置默认提示词：%1
                         .arg(promptFile.fileName());
        }
    } else {
        daWarning << tr("Failed to read Agent system prompt file, fallback to built-in default: %1")  //cn:读取 Agent 系统提示词文件失败，回退到内置默认提示词：%1
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
 *
 * 逐个把结构化工具规格经 DA::toJson 序列化为 OpenAI function schema。
 * @return 工具规格 JSON 数组
 */
QJsonArray DAAgentModule::assembleToolSpecs() const
{
    DA_DC(d);
    QJsonArray specs;
    for (auto* tool : d->mTools) {
        specs.append(toJson(tool->getToolSpec()));
    }
    return specs;
}

/**
 * @brief 组装子 agent 定义协议数组（子 agent 一期）
 * @return 协议载荷 JSON 数组，每元素 {name, description, tools, system_prompt}
 *（母文档 §7 契约逐字一致；permissions 预留字段不下发）
 *
 * 随 init 一次性下发，定义增删改后经 DAAgentBridge::sendUpdateSubagents 热更新。
 */
QJsonArray DAAgentModule::assembleSubagentDefs() const
{
    DA_DC(d);
    QJsonArray defs;
    if (!d->mSubagentManager) {
        return defs;
    }
    const QList<DAAgentSubagentDef> list = d->mSubagentManager->subagents();
    for (const DAAgentSubagentDef& s : list) {
        defs.append(s.toProtocolJson());
    }
    return defs;
}

/**
 * @brief 发送用户消息给 agent（会话桥懒启动 + 持久化）
 * @param text 用户消息文本
 *
 * concurrent-sessions：消息路由到当前会话的桥——桥已存在（活跃/后台运行/等待
 * 输入）则直接复用；否则 adoptOrStartBridge 接管预热桥或冷启动新桥（历史非空
 * 时 init → load_session → user_msg 按 stdin 管道序下发，Python 主循环顺序消费）。
 * 修复既有缺陷：旧实现用户 Stop 后继续发消息不重建 state（上下文丢失），
 * 新实现冷启动路径经 load_session 恢复完整上下文。
 */
void DAAgentModule::sendMessage(const QString& text)
{
    DA_D(d);
    // 契约7 + MAJOR3（round-3）：经 Module::createSession（内部 store.createSession +
    // 设 mCurrentSessionId + setLastActive + emit sessionListChanged，不 emit
    // sessionCreated——避免触发 onSessionCreated 的 clearChat 擦除刚显示的用户消息）。
    // UI "+" 走 newSession()（有 sessionCreated）。
    if (d->mCurrentSessionId.isEmpty()) {
        d->mCurrentSessionId = createSession();
    }
    const QString sid = d->mCurrentSessionId;
    // 发送前持久化 user 消息（桥启动失败时记录保留，会话不丢，下次重试）
    d->mSessionStore->appendRecord(sid, makeUserRecord(text));
    // 首条 user 消息定简短标题；标题变更时刷新 UI 下拉（否则 combo 一直显示 (untitled)）
    if (d->mSessionStore->ensureTitle(sid)) {
        emit sessionListChanged(listSessionsForUI());
    }
    d->mSessionError.remove(sid);
    // 确保会话桥：已有则复用；死亡且不再自愈（用户 Stop/异常退出耗尽）则防御性重建
    DAAgentBridge* bridge = d->mSessionBridges.value(sid);
    if (bridge && !bridge->isRunning()) {
        // 崩溃自愈中的桥 isRunning 短暂为 false 但 1s 后 recoverFromCrash 重启——
        // 此处退役它并换新桥同样正确（历史经 load_session 重建，仅多一次冷启动）
        retireBridge(sid);
        bridge = nullptr;
    }
    if (!bridge) {
        bridge = adoptOrStartBridge(sid);  // 失败时已报错并返回 nullptr
        if (!bridge) return;
    }
    bridge->sendMessage(text);
}

/**
 * @brief 停止正在运行的 agent（用户主动终止，非阻塞）
 *
 * concurrent-sessions：仅停止活跃会话的桥——后台会话不受影响
 *（Stop 按钮作用于当前聊天区）。
 */
void DAAgentModule::stop()
{
    DA_D(d);
    DAAgentBridge* bridge = d->mSessionBridges.value(d->mCurrentSessionId);
    // isRecovering 放行（审计 L4）：崩溃自愈 1s 恢复窗口内 isRunning()==false，
    // 旧守卫使 Stop 被静默 no-op，恢复计时器照常重启重放，违背用户终止意图。
    // requestStop 内取消恢复定时器并回发 busy(false)。
    if (bridge && (bridge->isRunning() || bridge->isRecovering())) {
        // 审计问题 17：Stop 必然中断挂起的 ask_user——同步清缓存 + 撤卡，
        // 否则用户对着幽灵卡作答，sendUserAnswer 落盘孤儿 tool_result 且
        // 答案蒸发（死桥 writeJson 静默失败）
        if (d->mPendingQuestions.remove(d->mCurrentSessionId)) {
            emit agentQuestionDismissed();
        }
        // 审计问题 1：Stop 必然中断在途工具配对——清会话 FIFO。并发版 Stop
        // 只作用活跃会话，按会话清理无副作用；不清则 ask_user 待答时按 Stop
        // 再发"继续"，残留 uuid 使答案/结果配对错位落盘
        d->mPendingToolCallUuids.remove(d->mCurrentSessionId);
        bridge->requestStop();
    }
}

/**
 * @brief 停止全部 agent 子进程并等待退出（阻塞，仅在应用关闭时调用）
 */
void DAAgentModule::shutdown()
{
    DA_D(d);
    // concurrent-sessions：停止全部桥（会话桥 + 预热桥），确保在 Python 解释器
    // 关闭前子进程已干净退出。仅在 AppMainWindow::closeEvent 中调用
    //（QApplication 事件循环尚在运行）。
    //
    // 两阶段停止（审计 L7②）：先对全部桥写 stop + 关写通道（非阻塞，各 Python
    // 端并行优雅退出），再逐个等待——避免此前串行 stopAgent 各自
    // waitForFinished(stopTimeout) 造成最坏 N×5s 的关闭冻结。
    //
    // 重入防御（审计 L7③）：awaitStopAgent 的 waitForFinished 可能在栈内同步
    // 触发 onProcessFinished → 本 Module 的持久化/记账 lambda 重入（后台会话
    // done → retireBridge 当场改 mSessionBridges）。故先取桥指针快照，两阶段
    // 均遍历快照而非活映射；retireBridge 的 deleteLater 在事件循环恢复前不会
    // 销毁对象，快照指针在本函数栈内保持有效。
    QList<DAAgentBridge*> bridges;
    bridges.reserve(d->mSessionBridges.size() + 1);
    for (auto it = d->mSessionBridges.constBegin(); it != d->mSessionBridges.constEnd(); ++it) {
        if (it.value()) {
            bridges.append(it.value());
        }
    }
    if (d->mIdleBridge) {
        bridges.append(d->mIdleBridge);
    }
    for (DAAgentBridge* b : std::as_const(bridges)) {
        b->beginStopAgent();
    }
    for (DAAgentBridge* b : std::as_const(bridges)) {
        b->awaitStopAgent();
    }
}

/**
 * @brief 转发用户对 agent 提问的回答给子进程
 * @param answer 用户回答文本
 *
 * concurrent-sessions：交互卡只在活跃会话显示 → 回答路由到活跃会话的桥；
 * 配对出队与落盘均按该会话的 FIFO。
 */
void DAAgentModule::sendUserAnswer(const QString& answer)
{
    DA_D(d);
    const QString sid = d->mCurrentSessionId;
    if (!sid.isEmpty()) {
        QQueue<QString>& q = d->mPendingToolCallUuids[sid];
        if (!q.isEmpty()) {
            appendToolResultRecord(sid, q.dequeue(), answer);
        }
        // 已作答：清除挂起问题缓存（角标消失）
        d->mPendingQuestions.remove(sid);
    }
    DAAgentBridge* bridge = d->mSessionBridges.value(sid);
    if (bridge) {
        bridge->sendUserAnswer(answer);
    }
    emit sessionListChanged(listSessionsForUI());
}

/**
 * @brief 为会话冷启动新桥（原 startAgentInternal 会话化改造）
 *
 * 保留原配置校验与错误提示语义（LLM 必填项 / Python / 脚本路径）。
 * 失败返回 nullptr（调用方 sendMessage 已持久化 user 记录，会话保留待重试）。
 * @param sessionId 目标会话 ID
 * @return 已启动的桥；启动失败返回 nullptr
 */
DAAgentBridge* DAAgentModule::createBridgeForSession(const QString& sessionId)
{
    DA_D(d);
    // 获取 LLM 配置协议投影（扁平 key，Python init 消息 config 字段）
    QJsonObject config = d->mConfig.toRunnerConfigJson();

    // 检查 LLM 必填项是否就绪——防止空配置启动 agent 导致
    // agent_runner.py 报 "config missing" 后进程无法正常退出、
    // 60s ready 超时被 kill（exitCode=62097 CrashExit）
    QString baseUrl = config.value("base_url").toString().trimmed();
    QString apiKey  = config.value("api_key").toString().trimmed();
    QString model   = config.value("model").toString().trimmed();
    if (baseUrl.isEmpty() || apiKey.isEmpty() || model.isEmpty()) {
        daWarning << tr("LLM is not configured, cannot start agent. "
                        "Please configure LLM in settings first.");  //cn:LLM 未配置，无法启动 Agent，请先在设置中配置 LLM
        emit systemMessage(tr("LLM is not configured. Please configure LLM in settings first."),  //cn:LLM 未配置，请先在设置中配置 LLM
                            QStringLiteral("warning"));
        return nullptr;
    }

    // 通过 detect 方法解析路径（不依赖 config 是否包含这些键）
    QString pythonExe = detectPythonExePath();
    QString scriptPath = detectAgentScriptPath();

    // 路径缺失时提前返回并报错——daCritical 会路由到 UI 日志窗口
    if (pythonExe.isEmpty()) {
        daCritical << tr("Cannot find Python interpreter path, please configure it in settings");  //cn:无法找到 Python 解释器路径，请在设置页配置 Python 解释器
        return nullptr;
    }
    if (scriptPath.isEmpty() || !QFile::exists(scriptPath)) {
        daCritical << tr("Cannot find agent_runner.py path: %1").arg(scriptPath);  //cn:无法找到 agent_runner.py 路径: %1
        return nullptr;
    }

    // 读取可配超时(内存配置模型，默认值由 DAAgentLLMConfig 兜底)
    // 单位:秒→毫秒。ready 超时默认 60s 覆盖 langchain 冷启动导入(~17s)+余量;
    // stop 超时默认 5s 保持原有行为。
    const DAAgentLLMConfig& c = d->mConfig.llm();
    int readyTimeoutMs = c.readyTimeoutSec() * 1000;
    int stopTimeoutMs   = c.stopTimeoutSec() * 1000;

    // concurrent-sessions：每会话一个桥；权限引擎注入（executeTool 前置门唯一执法依据）
    DAAgentBridge* bridge = new DAAgentBridge(this);
    bridge->setPermissionManager(d->mPermissionManager);
    attachBridge(bridge, sessionId);
    // 启动（子 agent 一期：init 附子 agent 定义数组，assembleSubagentDefs 协议载荷）；
    // agentStarting 由 startAgent 内部发射，经 attachBridge 路由（活跃会话 UI 进入"启动中"）
    bridge->startAgent(config, assembleToolSpecs(), assembleSystemPrompt(),
                       assembleSubagentDefs(),
                       pythonExe, scriptPath, readyTimeoutMs, stopTimeoutMs);
    // 启动失败（如 waitForStarted 超时，startAgent 内部已 emit agentError）：
    // 退役半死桥，返回 nullptr 走失败路径
    if (!bridge->isRunning()) {
        retireBridge(sessionId);
        return nullptr;
    }
    return bridge;
}

/**
 * @brief 确保会话有桥：优先接管预热空闲桥，否则冷启动新桥；历史非空时管道序下发 load_session
 *
 * stdin 管道序保证 init → load_session → user_msg 依序被 Python 主循环消费
 * （agent_runner.py 主循环 await 逐条处理，结构性满足铁律 T15 时序）。
 * @param sessionId 目标会话 ID
 * @return 会话桥；启动失败返回 nullptr（已报错）
 */
DAAgentBridge* DAAgentModule::adoptOrStartBridge(const QString& sessionId)
{
    DA_D(d);
    if (DAAgentBridge* existing = d->mSessionBridges.value(sessionId)) {
        return existing;
    }
    DAAgentBridge* bridge = nullptr;
    if (d->mIdleBridge) {
        bridge = d->mIdleBridge;
        d->mIdleBridge = nullptr;
        const bool wasReady = d->mIdleBridgeReady;
        d->mIdleBridgeReady = false;
        if (!bridge->isRunning()) {
            // 预热桥已死（异常退出未被清理）——丢弃并走冷启动
            bridge->deleteLater();
            bridge = nullptr;
        } else {
            // 断开预热期的记账连接（agentReady 记账 / processExited 清理），
            // 避免接管后误置 mIdleBridgeReady / 误清 mIdleBridge
            disconnect(bridge, &DAAgentBridge::agentReady, this, nullptr);
            disconnect(bridge, &DAAgentBridge::processExited, this, nullptr);
            attachBridge(bridge, sessionId);
            if (!wasReady) {
                // 预热桥仍在冷启动中：agentStarting 未路由过（预热线未连），
                // 手动补会话启动态（后续 ready 到达时经路由 lambda 清除）
                d->mSessionStarting[sessionId] = true;
                if (sessionId == d->mCurrentSessionId) emit agentStarting();
            }
        }
    }
    if (!bridge) {
        bridge = createBridgeForSession(sessionId);
        if (!bridge) return nullptr;
    }
    // 历史非空（本轮 user 记录已计入 messageCount > 1）→ 先下发历史重建 state，
    // 随后调用方的 user_msg 在 stdin 管道中排在 load_session 之后
    if (d->mSessionStore->messageCount(sessionId) > 1) {
        bridge->sendLoadSession(sessionId, d->mSessionStore->readMessagesForLoad(sessionId));
    }
    return bridge;
}

/**
 * @brief 预启动 agent 子进程（程序启动时调用）
 *
 * concurrent-sessions：预热桥为"未绑定会话"的独立子进程（至多 1 个），
 * 首次 sendMessage 时被 adoptOrStartBridge 接管（免冷启动）；之后的并发
 * 新会话按用户决策接受 ~16s 冷启动。受 auto_prestart 配置开关（默认 true）
 * + LLM 配置就绪双重控制；工具未注册时跳过（退回懒启动）。
 */
void DAAgentModule::prestartAgent()
{
    DA_D(d);
    if (d->mIdleBridge && d->mIdleBridge->isRunning()) {
        return;  // 已有预热桥
    }
    if (d->mIdleBridge) {
        d->mIdleBridge->deleteLater();
        d->mIdleBridge = nullptr;
    }
    const DAAgentLLMConfig c = d->mConfig.llm();
    // 检查 auto_prestart 开关（默认 true）
    if (!c.autoPrestart()) {
        return;  // 用户关闭了自动预热
    }
    // 检查 LLM 必填项是否就绪
    if (c.baseUrl().trimmed().isEmpty() || c.apiKey().trimmed().isEmpty() || c.model().trimmed().isEmpty()) {
        return;  // 未配置 LLM，不预启动（发消息时走懒启动 fallback 报错提示）
    }
    // 工具未注册时跳过预启动——init 消息会携带空工具列表发给 Python，
    // 导致 LLM 无 list_data 等工具可调用。跳过后退回懒启动：
    // 首次 sendMessage() 时 createBridgeForSession() 会携带全部已注册工具的 init。
    if (d->mTools.isEmpty()) {
        daDebug << "Skip agent prestart: no tools registered yet, "
                   "will lazily start on first message";
        return;
    }
    // 配置/路径探测（与 createBridgeForSession 同源校验，失败静默跳过预热）
    QJsonObject config = d->mConfig.toRunnerConfigJson();
    QString pythonExe = detectPythonExePath();
    QString scriptPath = detectAgentScriptPath();
    if (pythonExe.isEmpty() || scriptPath.isEmpty() || !QFile::exists(scriptPath)) {
        return;
    }
    d->mIdleBridge = new DAAgentBridge(this);
    d->mIdleBridge->setPermissionManager(d->mPermissionManager);
    d->mIdleBridgeReady = false;
    // 仅连接 ready 记账与退出清理；其余信号待接管时 attachBridge 按会话连接。
    // 预热期 agentStarting/agentReady 不经会话路由——启动预热期间 UI 不闪"启动中"
    connect(d->mIdleBridge, &DAAgentBridge::agentReady, this, [this](const QString&) {
        d_func()->mIdleBridgeReady = true;
    });
    connect(d->mIdleBridge, &DAAgentBridge::processExited, this, [this]() {
        auto* d = d_func();
        if (d->mIdleBridge) {
            d->mIdleBridge->deleteLater();
            d->mIdleBridge = nullptr;
            d->mIdleBridgeReady = false;
        }
    });
    d->mIdleBridge->startAgent(config, assembleToolSpecs(), assembleSystemPrompt(),
                               assembleSubagentDefs(),
                               pythonExe, scriptPath,
                               c.readyTimeoutSec() * 1000, c.stopTimeoutSec() * 1000);
}

/**
 * @brief 检查是否有任何 agent 子进程正在运行（会话桥或预热桥）
 * @return 任一桥存活返回 true
 */
bool DAAgentModule::isRunning() const
{
    DA_DC(d);
    // concurrent-sessions：任一存活桥即视为运行（prestart 守卫等外部判断用）；
    // 会话级忙碌判断走 sessionRuntimeState / mSessionBusy
    if (d->mIdleBridge && d->mIdleBridge->isRunning()) return true;
    for (DAAgentBridge* b : d->mSessionBridges) {
        if (b && b->isRunning()) return true;
    }
    return false;
}

/**
 * @brief 绑定会话：注册映射 + 连接全部信号路由
 *
 * 路由规则（concurrent-sessions 核心）：
 * - 持久化 lambda 捕获 sessionId，永远写桥所属会话（与 mCurrentSessionId 解耦，
 *   后台会话输出不再污染当前会话——根治"旧会话尾巴污染新会话"）；
 * - UI 接口信号仅当 sessionId == mCurrentSessionId 时 emit（Dock 只见活跃会话）；
 * - 后台会话的 ask_user/审批请求缓存于 Module（切回时重发），并刷新会话列表角标。
 */
void DAAgentModule::attachBridge(DAAgentBridge* bridge, const QString& sessionId)
{
    DA_D(d);
    if (!bridge || sessionId.isEmpty()) return;
    d->mSessionBridges.insert(sessionId, bridge);

    // ---- 工具实现表注入（冷启动新桥与接管预热桥的公共收口） ----
    // 桥的创建时机晚于插件 registerTool，注册期的热更新（forEachLiveBridge）
    // 覆盖不到"后出生"的桥；漏注入会导致 executeToolNow 对全部插件工具
    // 返回 Unknown tool（工具规格已随 startAgent 下发 Python，LLM 可见可调用，
    // 但 C++ 侧执行表为空——规格与实现两张表必须同步）
    bridge->setTools(d->mTools);

    // ---- 持久化（写桥所属会话，无条件执行） ----
    // assistant 消息完成（纯文本回复，含伴随 tool_calls 的中间思考文本）
    connect(bridge, &DAAgentBridge::agentMessageComplete, this,
            [this, sessionId](const QString& fullText) {
        appendAssistantRecord(sessionId, fullText, /*toolCalls=*/{});
    });
    // 工具调用（含 ask_user 提问——ask_user 复用 tool_call 语义）
    connect(bridge, &DAAgentBridge::agentToolCall, this,
            [this, sessionId](const QString& tool, const QJsonObject& args) {
        auto* d = d_func();
        // 契约6：appendToolCallRecord 返回本条记录 uuid，入会话 FIFO 供 tool_result 配对
        d->mPendingToolCallUuids[sessionId].enqueue(appendToolCallRecord(sessionId, tool, args));
    });
    // 工具结果
    // 子 agent 一期过滤（母文档 §7）：带 subagent_id 的结果不写会话 JSONL
    //（子转录不落盘；对应的 tool_call 本就未 emit/未入队，出队配对天然一致）
    connect(bridge, &DAAgentBridge::agentToolResult, this,
            [this, sessionId](const QString& /*tool*/, const QJsonObject& result, const QString& subagentId) {
        auto* d = d_func();
        if (!subagentId.isEmpty()) return;
        // 契约6：FIFO 出队取配对的 tool_call uuid（_rpc_call 串行保证顺序）
        // MAJOR1（round-4）：dequeue 前加 isEmpty 守卫，防空队列未定义行为
        QQueue<QString>& q = d->mPendingToolCallUuids[sessionId];
        if (q.isEmpty()) return;
        // MAJOR（round-4，from plan-04）：content 统一明文（result 序列化为 JSON 字符串）
        appendToolResultRecord(sessionId, q.dequeue(),
                               QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)));
    });
    // token 使用量
    connect(bridge, &DAAgentBridge::agentUsage, this,
            [this, sessionId](int inT, int outT, int tot, const QString& src) {
        auto* d = d_func();
        // streaming_estimate 是流式过程中的临时估算值，不持久化、不进累计——
        // 真实 usage 由 message_end/usage 消息回传并持久化。
        if (src != "streaming_estimate") {
            appendUsageRecord(sessionId, inT, outT, tot, src);
            // 真实 usage（agent/summary）累加到会话累计：跨轮次单调增长，压缩不重置
            d->mCumulativeInTokens[sessionId] += inT;
            d->mCumulativeOutTokens[sessionId] += outT;
            d->mCumulativeTotalTokens[sessionId] += tot;
            if (sessionId == d->mCurrentSessionId) {
                // 契约2：emit 5 参信号（context_window 经 readContextWindow 复用）
                emit tokenUsageUpdated(d->mCumulativeInTokens[sessionId], d->mCumulativeOutTokens[sessionId],
                                       d->mCumulativeTotalTokens[sessionId], readContextWindow(), src);
            }
        } else if (sessionId == d->mCurrentSessionId) {
            // 流式估算不累加：emit"累计 + 本轮估算"的临时值（仅活跃会话刷新 UI）
            emit tokenUsageUpdated(d->mCumulativeInTokens[sessionId] + inT,
                                   d->mCumulativeOutTokens[sessionId] + outT,
                                   d->mCumulativeTotalTokens[sessionId] + tot, readContextWindow(), src);
        }
    });
    // agent 提问（ask_user）——落盘 + 缓存（切回重发交互卡）
    connect(bridge, &DAAgentBridge::agentQuestion, this,
            [this, sessionId](const QString& text, const QStringList& options, bool multiSelect) {
        auto* d = d_func();
        QJsonObject args;
        args["question"]     = text;
        args["options"]      = QJsonArray::fromStringList(options);
        args["multi_select"] = multiSelect;
        // 契约6：入会话 FIFO 供下一条 answer 按 FIFO 配对
        d->mPendingToolCallUuids[sessionId].enqueue(appendToolCallRecord(sessionId, "ask_user", args));
        // concurrent-sessions：缓存问题载荷，切回该会话时重发可交互问题卡
        PrivateData::PendingQuestion pq;
        pq.text       = text;
        pq.options    = options;
        pq.multiSelect = multiSelect;
        d->mPendingQuestions[sessionId] = pq;
        if (sessionId == d->mCurrentSessionId) {
            emit agentQuestion(text, options, multiSelect);
        } else {
            emit sessionListChanged(listSessionsForUI());  // 后台等待输入 → 角标
        }
    });

    // ---- UI 信号（仅活跃会话转发） ----
    connect(bridge, &DAAgentBridge::agentToken, this, [this, sessionId](const QString& t) {
        if (sessionId == d_func()->mCurrentSessionId) emit agentToken(t);
    });
    // assistant 消息完成：Dock 据此 finalizeAgentMessage 定稿流式气泡。
    // e1053d1 会话化重构时本转发遗漏（只保留了持久化 lambda），导致 JS 侧
    // currentAgentMsg 永不闭合——整轮回复（含多次工具调用间的叙述）全部
    // 堆积进同一个气泡。
    connect(bridge, &DAAgentBridge::agentMessageComplete, this, [this, sessionId](const QString& fullText) {
        if (sessionId == d_func()->mCurrentSessionId) emit agentMessageComplete(fullText);
    });
    // 工具调用：Dock 据此渲染工具卡片。Bridge 已过滤子 agent 调用
    //（带 subagent_id 的不 emit，子转录不进主聊天流，母文档 §7）。
    // 同为 e1053d1 遗漏——缺失时聊天界面看不到任何工具调用卡片。
    connect(bridge, &DAAgentBridge::agentToolCall, this,
            [this, sessionId](const QString& toolName, const QJsonObject& args) {
        if (sessionId == d_func()->mCurrentSessionId) emit agentToolCall(toolName, args);
    });
    // 工具结果：子 agent 一期过滤规则（母文档 §7）——带 subagent_id 的结果
    // 不转发接口信号（不进主聊天流），执行照常（权限门同门执法）。
    connect(bridge, &DAAgentBridge::agentToolResult, this,
            [this, sessionId](const QString& toolName, const QJsonObject& result, const QString& subagentId) {
        if (subagentId.isEmpty() && sessionId == d_func()->mCurrentSessionId) {
            emit agentToolResult(toolName, result);
        }
    });
    // 转发 agentRetrying 信号到接口（plan-03 step6）
    connect(bridge, &DAAgentBridge::agentRetrying, this,
            [this, sessionId](int attempt, int maxAttempts, int delayMs, const QString& errorType, const QString& errorMessage) {
        if (sessionId == d_func()->mCurrentSessionId) {
            emit agentRetrying(attempt, maxAttempts, delayMs, errorType, errorMessage);
        }
    });
    // 子 agent 任务进度：原样转发（终态撤卡语义由 Bridge 消化）
    connect(bridge, &DAAgentBridge::agentSubagentProgress, this, [this, sessionId](const QJsonObject& p) {
        if (sessionId == d_func()->mCurrentSessionId) emit agentSubagentProgress(p);
    });
    // 回合疑似未完成（模型"话说一半就停"）：转发 + systemMessage 提醒卡
    connect(bridge, &DAAgentBridge::agentTurnPossiblyIncomplete, this, [this, sessionId](int toolRounds) {
        if (sessionId != d_func()->mCurrentSessionId) return;
        emit agentTurnPossiblyIncomplete(toolRounds);
        emit systemMessage(
            tr("The agent ended this turn after %1 tool calls, but its last "
               "message looks like an unfinished plan (e.g. announcing a next "
               "step without executing it). Send a message such as "
               "\"continue\" to let it finish.")  //cn:Agent 在执行 %1 轮工具调用后结束了本轮，但最后的回复疑似未完成的计划（如宣称下一步却未执行）。可发送"继续"等消息让它完成剩余工作。
                .arg(toolRounds),
            QStringLiteral("warning"));
    });

    // ---- 状态信号：内部记账 + 活跃转发 ----
    connect(bridge, &DAAgentBridge::agentStarting, this, [this, sessionId]() {
        auto* d = d_func();
        d->mSessionStarting[sessionId] = true;
        if (sessionId == d->mCurrentSessionId) emit agentStarting();
    });
    // 崩溃恢复：ready 恢复路径重发会话历史或最后消息（原 connectSignals 逻辑会话化）
    connect(bridge, &DAAgentBridge::agentReady, this, [this, bridge, sessionId](const QString& model) {
        auto* d = d_func();
        d->mSessionStarting[sessionId] = false;
        if (sessionId == d->mCurrentSessionId) emit agentReady(model);
        if (bridge->isRecovering()) {
            const QString lastSid = bridge->lastSessionId();
            if (!lastSid.isEmpty()) {
                // 有会话——下发 load_session 重建 state（session_loaded 到达后
                // agentSessionLoaded lambda 判 isRecovering 重发最后消息）
                bridge->sendLoadSession(lastSid, d->mSessionStore->readMessagesForLoad(lastSid));
            } else {
                // 无会话——直接重发最后消息（内部复位 m_recovering）
                bridge->resendLastMessage();
            }
        }
    });
    connect(bridge, &DAAgentBridge::agentBusy, this, [this, sessionId](bool busy) {
        auto* d = d_func();
        d->mSessionBusy[sessionId] = busy;
        if (!busy) {
            // busy(false) 兜底清除启动态（审计问题 2）——启动失败/崩溃耗尽等终态
            // 只发 error+busy(false) 不发 ready（crash_exhausted 见 Bridge
            // onProcessFinished），mSessionStarting 无人清除则永久残留：角标卡
            // "starting"、switchSession 切离守卫拒绝退役死桥、切入该会话时
            // agentStarting 重断言使 Dock 输入永久冻结（发不出消息也就触发不了
            // sendMessage 防御重建，死锁闭环）。与 Dock 侧 onAgentBusy 的
            // busy(false) 兜底完全同构；崩溃重试窗口内 recoverFromCrash →
            // startAgent → agentStarting 会重新置位，语义不受影响。
            d->mSessionStarting.remove(sessionId);
        }
        if (sessionId == d->mCurrentSessionId) emit agentBusy(busy);
        // 角标刷新双向覆盖（审计问题 6）：Bridge 的错误路径（crash_exhausted/
        // ready 超时/写失败回滚）只发 error+busy(false) 不发 agentDone——
        // 若仅 busy(true) 刷新，"running"→"error" 的角标变化要等下一个事件
        // 才更新，会话管理器显示过期状态
        emit sessionListChanged(listSessionsForUI());
    });
    connect(bridge, &DAAgentBridge::agentDone, this, [this, sessionId]() {
        auto* d = d_func();
        d->mSessionBusy[sessionId] = false;
        if (sessionId == d->mCurrentSessionId) emit agentDone();
        // concurrent-sessions：后台会话跑完且无需等待输入 → 优雅退役
        //（对话状态已全量落盘 JSONL，下次发消息时经 load_session 重建）
        if (sessionId != d->mCurrentSessionId && !d->mPendingQuestions.contains(sessionId)
            && d->mPendingApprovalRequests.value(sessionId).isEmpty()) {
            retireBridge(sessionId);
        }
        emit sessionListChanged(listSessionsForUI());
    });
    connect(bridge, &DAAgentBridge::agentError, this,
            [this, sessionId](const QString& message, const QString& errorType, const QString& detail) {
        auto* d = d_func();
        d->mSessionError[sessionId] = true;
        // 审计问题 1（恢复旧版语义，按会话作用域）：错误使在途工具调用配对
        // 作废——不清 FIFO 则残留 uuid 与该会话下一轮的 tool_result 错配
        // （JSONL 中 tool_result.tool_call_id 挂错，历史重放时旧工具卡挂新
        // 结果、新工具卡因无结果被跳过，落盘数据永久污染）
        d->mPendingToolCallUuids.remove(sessionId);
        // 问题 1×17 联动：挂起问题随回合作用死——同批清缓存+撤卡。若只清
        // FIFO 不清问题卡，用户对着幽灵卡作答会因 FIFO 已空跳过持久化，
        // 产生"答案消失"的新症状（FIFO/问题缓存/UI 卡三处必须同批）
        const bool hadQuestion = d->mPendingQuestions.remove(sessionId);
        if (sessionId == d->mCurrentSessionId) {
            if (hadQuestion) {
                emit agentQuestionDismissed();
            }
            emit agentError(message, errorType, detail);
        } else {
            emit sessionListChanged(listSessionsForUI());  // 后台出错 → 角标（含 waiting_input 解除）
        }
    });
    // 崩溃恢复：session_loaded 后重发最后消息（isRecovering 由 resendLastMessage 内部复位）
    connect(bridge, &DAAgentBridge::agentSessionLoaded, this, [this, bridge, sessionId](const QString& sid) {
        if (bridge->isRecovering()) {
            bridge->resendLastMessage();
        }
        if (sessionId == d_func()->mCurrentSessionId) emit agentSessionLoaded(sid);
    });

    // ---- 审批：_tier/_rememberable 补齐 + callId→会话路由表 + 挂起缓存 ----
    // Bridge 侧仅知工具名/参数，分级信息由 Module 补齐后转发给 UI，
    // 使审批卡能据 _tier 决定是否渲染"本会话记住"（仅 file_write，A5）
    connect(bridge, &DAAgentBridge::agentToolApprovalRequest, this,
            [this, sessionId](const QString& callId, const QString& toolName, const QJsonObject& args) {
        auto* d = d_func();
        QJsonObject payload = args;
        QString tier = DAAgentPermissionManager::tierUnknown();
        if (d->mPermissionManager) {
            tier = d->mPermissionManager->tierOf(toolName, args);
        }
        payload[QStringLiteral("_tier")]         = tier;
        payload[QStringLiteral("_rememberable")] = (tier == DAAgentPermissionManager::tierFileWrite());
        // 碰撞防御（审计问题 24）：callId 是 LLM 生成的 tool_call id 而非本
        // 项目 UUID——部分 OpenAI 兼容网关/本地模型用低熵或索引式 id（如
        // call_0），并发多会话下碰撞概率不可忽略。后写覆盖前者会使会话 A 的
        // "批准"被路由到会话 B 的桥执行 B 的挂起工具（批错会话）。未雨绸缪级
        // 防御：检测告警（Python 侧 _pending_rpcs 有对称检测）。
        const QString prevSid = d->mApprovalSessionByCallId.value(callId);
        if (!prevSid.isEmpty() && prevSid != sessionId) {
            qWarning() << "DAAgentModule: tool approval callId collision, callId=" << callId
                       << "previously routed to session" << prevSid
                       << "now rerouted to" << sessionId;
        }
        d->mApprovalSessionByCallId[callId] = sessionId;
        QJsonObject cached;
        cached[QStringLiteral("callId")]   = callId;
        cached[QStringLiteral("toolName")] = toolName;
        cached[QStringLiteral("args")]     = payload;
        d->mPendingApprovalRequests[sessionId].append(cached);
        if (sessionId == d->mCurrentSessionId) {
            emit agentToolApprovalRequest(callId, toolName, payload);
        } else {
            emit sessionListChanged(listSessionsForUI());  // 后台等待审批 → 角标
        }
    });
    // 审批作废（子进程退出/崩溃/子 agent 终态撤卡）：清路由表与缓存
    connect(bridge, &DAAgentBridge::agentToolApprovalDismissed, this,
            [this, sessionId](const QString& callId) {
        auto* d = d_func();
        d->mApprovalSessionByCallId.remove(callId);
        QJsonArray& arr = d->mPendingApprovalRequests[sessionId];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr.at(i).toObject().value("callId").toString() == callId) {
                arr.removeAt(i);
                break;
            }
        }
        if (sessionId == d->mCurrentSessionId) emit agentToolApprovalDismissed(callId);
    });

    // ---- 桥退出：清权限会话记忆（T16 A5：不跨重启存活；V1 保持全局语义，
    //      并发下可能过度清除——安全方向，宁可多问一次不漏清） ----
    // 注意：不在此处移除会话映射——崩溃自愈路径 processExited（DAAgentBridge.cpp:1021）
    // 先于 recoverFromCrash（:1062 1s 延迟）发射，此处移除会孤儿化正在自愈的桥；
    // 死亡且不再自愈的桥由 sendMessage 的 isRunning 防御分支 / 切离退役路径惰性清理。
    connect(bridge, &DAAgentBridge::processExited, this, [this, sessionId]() {
        auto* d = d_func();
        if (d->mPermissionManager) {
            d->mPermissionManager->clearSessionMemory();
        }
        // 审计问题 17：进程死亡（用户 Stop/崩溃/错误终止）使挂起的 ask_user
        // 作废——清缓存 + 撤卡（镜像审批 dismissed 契约）。不清则：角标永久
        // waiting_input、切离/退役守卫拒绝处理死桥、切回重发幽灵问题卡、
        // 用户对幽灵卡作答经 sendUserAnswer 落盘孤儿 tool_result（答案蒸发）。
        if (d->mPendingQuestions.remove(sessionId)) {
            if (sessionId == d->mCurrentSessionId) {
                emit agentQuestionDismissed();
            } else {
                emit sessionListChanged(listSessionsForUI());  // 后台角标解除
            }
        }
    });
}

/**
 * @brief 优雅退役会话桥：断开路由、清缓存、非阻塞停止子进程
 *
 * 使用场景：后台会话跑完（agentDone 非活跃）、切离空闲会话、删除会话、
 * sendMessage 防御性重建。requestStop 为用户请求停止语义（不触发崩溃恢复）；
 * processExited 后 deleteLater 回收对象，主线程无阻塞。
 */
void DAAgentModule::retireBridge(const QString& sessionId)
{
    DA_D(d);
    DAAgentBridge* bridge = d->mSessionBridges.take(sessionId);
    if (!bridge) return;
    d->mSessionBusy.remove(sessionId);
    d->mSessionStarting.remove(sessionId);
    d->mSessionError.remove(sessionId);
    d->mCumulativeInTokens.remove(sessionId);
    d->mCumulativeOutTokens.remove(sessionId);
    d->mCumulativeTotalTokens.remove(sessionId);
    d->mPendingToolCallUuids.remove(sessionId);
    // 审计问题 17：任何清问题缓存的退役路径（删除会话/sendMessage 防御重建等）
    // 都须同步撤活跃会话屏幕上的问题卡——契约完整性兜底（常规路径进程退出时
    // processExited lambda 已先清缓存并撤卡，此处缓存多已为空）
    if (sessionId == d->mCurrentSessionId && d->mPendingQuestions.remove(sessionId)) {
        emit agentQuestionDismissed();
    } else {
        d->mPendingQuestions.remove(sessionId);
    }
    d->mPendingApprovalRequests.remove(sessionId);
    for (auto it = d->mApprovalSessionByCallId.begin(); it != d->mApprovalSessionByCallId.end();) {
        if (it.value() == sessionId) it = d->mApprovalSessionByCallId.erase(it);
        else ++it;
    }
    // 断开全部 Module 路由（退役后退出期间的信号不再持久化/转发），
    // 再挂 processExited → deleteLater（requestStop 后进程退出即回收）
    disconnect(bridge, nullptr, this, nullptr);
    connect(bridge, &DAAgentBridge::processExited, bridge, &QObject::deleteLater);
    if (bridge->isRunning()) {
        bridge->requestStop();  // 非阻塞；Python 优雅退出后 deleteLater
    } else {
        bridge->deleteLater();  // 已死进程直接回收（析构 stopAgent 为 no-op）
    }
}

/**
 * @brief 查会话桥（无返回 nullptr）
 */
DAAgentBridge* DAAgentModule::bridgeForSession(const QString& sessionId) const
{
    DA_DC(d);
    return d->mSessionBridges.value(sessionId, nullptr);
}

/**
 * @brief 遍历全部存活桥（会话桥 + 预热桥），fn 内不得增删桥
 */
void DAAgentModule::forEachLiveBridge(const std::function<void(DAAgentBridge*)>& fn)
{
    DA_D(d);
    for (auto it = d->mSessionBridges.begin(); it != d->mSessionBridges.end(); ++it) {
        if (it.value() && it.value()->isRunning()) fn(it.value());
    }
    if (d->mIdleBridge && d->mIdleBridge->isRunning()) fn(d->mIdleBridge);
}

/**
 * @brief 会话运行态（供 UI 角标）
 * @return "starting" / "running" / "waiting_input" / "error" / ""（空闲）
 */
QString DAAgentModule::sessionRuntimeState(const QString& sessionId) const
{
    DA_DC(d);
    if (d->mSessionStarting.value(sessionId, false)) return QStringLiteral("starting");
    if (d->mSessionBusy.value(sessionId, false)) return QStringLiteral("running");
    if (d->mPendingQuestions.contains(sessionId)
        || !d->mPendingApprovalRequests.value(sessionId).isEmpty()) {
        return QStringLiteral("waiting_input");
    }
    if (d->mSessionError.value(sessionId, false)) return QStringLiteral("error");
    return QString();  // 空闲
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
 * @return LLM 配置结构体（稀疏：engaged 字段=显式设置过，getter 兜底默认值）
 */
DAAgentLLMConfig DAAgentModule::getLLMConfig() const
{
    DA_DC(d);
    return d->mConfig.llm();
}

/**
 * @brief 设置 LLM 配置（仅 engaged 字段生效，等价原 contains 守卫语义）
 * @param config LLM 配置结构体增量
 *
 * v2 起派生字段（base_url/model/api_key/context_window/max_output_tokens）
 * 不持久化：增量携带这些字段仅影响内存态，落盘以 providers+active_provider
 * 重算为准（设置页增量不含这些键，无实际影响）。
 *
 * 子进程运行中则经 reconfigure 热同步（与 setActiveModel 同管道）：reconfigure
 * 消息在 stdin 排队，当前轮 run()/resume() 结束后 Python 主循环才处理——
 * 即设置页保存的 recursion_limit/max_retries 等运行参数从下一轮对话生效，
 * 无需重启程序或子进程。未运行时仅写配置，下次懒启动用新配置。
 */
void DAAgentModule::setLLMConfig(const DAAgentLLMConfig& config)
{
    DA_D(d);
    d->mConfig.mergeLLM(config);
    d->mConfig.save();
    // concurrent-sessions：热同步到全部存活桥（reconfigure 在各桥 stdin 排队，
    // 当前轮跑完后 Python 主循环处理，下一轮对话生效）
    forEachLiveBridge([this](DAAgentBridge* b) {
        b->reconfigureAgent(d_func()->mConfig.toRunnerConfigJson());
    });
}

// ===========================================================================
// 供应商与多模型管理实现
// ===========================================================================

/**
 * @brief 获取所有供应商配置（api_key 为内存态明文）
 * @return 供应商结构体列表
 *
 * 未显式配置 providers 而仅配置了 flat 连接键时（旧版本配置），由
 * DAAgentConfig::providers() 读时合成单个 "Default" 供应商，保证旧配置平滑升级。
 */
QList< DAAgentProvider > DAAgentModule::getProviders() const
{
    DA_DC(d);
    return d->mConfig.providers();
}

/**
 * @brief 保存所有供应商配置（api_key 明文传入，持久化时由 DAAgentConfig 加密）
 * @param providers 供应商结构体列表
 *
 * 保存后重新同步激活连接（含重命名跟进：激活供应商改名时 active_provider 随之
 * 更新）并刷新 Dock。
 */
void DAAgentModule::setProviders(const QList< DAAgentProvider >& providers)
{
    DA_D(d);
    d->mConfig.setProviders(providers);
    // 重新同步激活连接（激活供应商的 base_url/api_key/model 重算内存派生值）
    d->mConfig.syncActiveConnection();
    d->mConfig.save();
    emit availableModelsChanged(getAvailableModels());
    emit activeModelChanged(getActiveProvider(), getActiveModel());
}

/**
 * @brief 获取当前激活供应商名称
 * @return 激活供应商名称；未配置返回空
 */
QString DAAgentModule::getActiveProvider() const
{
    DA_DC(d);
    return d->mConfig.activeProvider();
}

/**
 * @brief 获取所有可选模型列表（Dock 下拉用，不含 api_key）
 * @return 每元素 DAAgentModelRef{provider,model,contextWindow,maxOutputTokens}
 */
QList< DAAgentModelRef > DAAgentModule::getAvailableModels() const
{
    DA_DC(d);
    QList< DAAgentModelRef > out;
    const QList< DAAgentProvider > providers = d->mConfig.providers();
    for (const DAAgentProvider& p : providers) {
        for (const DAAgentModel& m : p.models) {
            DAAgentModelRef ref;
            ref.provider        = p.name;
            ref.model           = m.id;
            ref.contextWindow   = m.contextWindow;
            ref.maxOutputTokens = m.maxOutputTokens;
            out.append(ref);
        }
    }
    return out;
}

/**
 * @brief 获取当前激活模型 id
 * @return 激活模型 id
 */
QString DAAgentModule::getActiveModel() const
{
    DA_DC(d);
    return d->mConfig.llm().model();
}

/**
 * @brief 设置激活供应商+模型（Dock 选择用）
 * @param provider 供应商名称
 * @param model 模型 id
 *
 * 校验 supplier+model 存在后，同步 base_url/api_key/model/context_window/
 * max_output_tokens（供 toRunnerConfigJson 读取）。emit activeModelChanged 通知
 * Dock 刷新。若子进程正在运行则热替换 LLM 配置（reconfigureAgent，不重启
 * 子进程、不丢 MemorySaver 会话状态）；未运行时仅写配置，下次懒启动用新配置。
 */
void DAAgentModule::setActiveModel(const QString& provider, const QString& model)
{
    DA_D(d);
    if (!d->mConfig.applyActiveModel(provider, model)) {
        return;
    }
    d->mConfig.save();
    emit activeModelChanged(provider, model);
    // concurrent-sessions：热替换到全部存活桥（不重启子进程、不丢各会话
    // MemorySaver 状态）；reconfigure 在各桥 stdin 排队，当前轮跑完后
    // Python 主循环处理，下一轮用新模型。未运行时仅写配置，下次懒启动用新 config。
    forEachLiveBridge([this](DAAgentBridge* b) {
        b->reconfigureAgent(d_func()->mConfig.toRunnerConfigJson());
    });
}

/**
 * @brief 推送当前供应商/模型选择到 Dock
 *
 * 由 DAAppController 在接口↔Dock 信号链 connect 完成后调用（与 restoreLastActiveSession
 * 同处）。防御性兜底：load() 已做 normalizeAfterLoad（派生连接重算），此处仅在
 * active_provider 为空或 api_key 为空（运行期遗留）时再 syncActiveConnection
 * 恢复并持久化，然后 emit availableModelsChanged + activeModelChanged。
 */
void DAAgentModule::pushModelSelection()
{
    DA_D(d);
    if (d->mConfig.activeProvider().isEmpty() || d->mConfig.llm().apiKey().isEmpty()) {
        // 兜底：取首个供应商为激活并重算派生连接（api_key 为空时从
        // providers 重新恢复，否则 agent 报 config missing 崩溃）
        d->mConfig.syncActiveConnection();
        d->mConfig.save();
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
    // 若当前已有活跃会话且仍为全新（无消息记录、该会话无进行中回合/启动中/挂起交互），
    // 直接复用该会话——避免用户连续点击"+"堆积无用空会话。
    //（旧条件 !isRunning() 是全局子进程判断，并发下会误伤：后台会话运行中
    // 也会阻止空会话复用；concurrent-sessions 改为会话级状态判断。）
    if (!d->mCurrentSessionId.isEmpty()
        && !d->mSessionBusy.value(d->mCurrentSessionId, false)
        && !d->mSessionStarting.value(d->mCurrentSessionId, false)
        && !d->mPendingQuestions.contains(d->mCurrentSessionId)
        && d->mSessionStore->messageCount(d->mCurrentSessionId) == 0) {
        emit sessionCreated(d->mCurrentSessionId);  // 复用：仅触发 UI 幂等刷新（clearChat 对已空聊天为 no-op），不落盘新会话
        reassertActiveSessionState();
        return;
    }
    QString sid = createSession();
    emit sessionCreated(sid);  // 仅此路径触发 UI clearChat
    // 状态重断言（审计问题 14）：后台会话 A 运行中点「+」→ B 成为活跃会话，
    // A 跑完时的 busy(false) 被活跃会话过滤挡掉（A≠B），Dock 的 busy 守卫
    // 若不复位则新空会话永久显示 thinking、输入禁用、Stop 空转——无任何自愈
    // 路径。复用 switchSession step4 语义：新会话无运行态 → busy(false)。
    reassertActiveSessionState();
}

/**
 * @brief 重断言活跃会话的 UI 运行态（switchSession step4 / newSession 共用）
 *
 * 先 busy(false) 清残留，再按活跃会话实际状态置 starting/busy——
 * Dock 的 busy/starting 守卫与 web 状态由此与会话真实状态对齐。
 */
void DAAgentModule::reassertActiveSessionState()
{
    DA_D(d);
    emit agentBusy(false);
    if (d->mSessionStarting.value(d->mCurrentSessionId, false)) {
        emit agentStarting();
    } else if (d->mSessionBusy.value(d->mCurrentSessionId, false)) {
        emit agentBusy(true);
    }
}

/**
 * @brief 切换到指定会话（纯 UI 重放，不触碰任何子进程）
 * @param sessionId 目标会话 ID
 * @return 是否执行切换（false=已是当前会话或会话不存在）
 *
 * concurrent-sessions 核心变化：旧会话忙碌时桥留后台继续；不再 eager 下发
 * load_session（state 重建推迟到该会话下次 sendMessage，stdin 管道序）；
 * 切回运行中会话时重放历史 + 恢复运行态 + 重发挂起交互卡。
 */
bool DAAgentModule::switchSession(const QString& sessionId)
{
    DA_D(d);
    if (sessionId == d->mCurrentSessionId) return false;  // false = 已是当前会话
    if (!d->mSessionStore->hasSession(sessionId)) return false;
    // 1. 切离旧会话（concurrent-sessions 核心变化）：忙碌/启动中/等待输入 → 桥留在
    //    后台继续执行（不再 requestStop 终止，旧会话尾巴由 attachBridge 的会话化
    //    持久化 lambda 写回原会话，无污染）；空闲且无挂起交互 → 优雅退役其桥
    //    （内存收窄，状态已全量落盘 JSONL）。
    const QString oldSid = d->mCurrentSessionId;
    if (!oldSid.isEmpty()) {
        DAAgentBridge* oldBridge = d->mSessionBridges.value(oldSid);
        if (oldBridge && !d->mSessionBusy.value(oldSid, false)
            && !d->mSessionStarting.value(oldSid, false)
            && !d->mPendingQuestions.contains(oldSid)
            && d->mPendingApprovalRequests.value(oldSid).isEmpty()) {
            retireBridge(oldSid);
        }
    }
    // 2. 切换 UI 归属 + last_active 指针（无任何子进程操作，切换耗时 = UI 重放）
    d->mCurrentSessionId = sessionId;
    d->mSessionStore->setLastActive(sessionId, d->mCurrentProjectPath);  // 带工程路径
    // 3. UI 历史重放由 sessionSwitched 信号触发（clearChat + loadHistory 在 Dock 处理；
    //    未配对的末尾 ask_user 在 C++ 合并器中被跳过，不渲染为已答静态问题——
    //    随后由第 5 步重发可交互卡片）
    emit sessionSwitched(sessionId, d->mSessionStore->readAllRecords(sessionId));
    // 4. 切换后回放 token 统计：从持久化 usage 记录求和重算会话累计值（无则全 0），
    //    避免 UI 拘留上一会话的 token 数值与进度条（Bug2 修复）；
    //    随后恢复目标会话 UI 运行态（顺序：先 busy(false) 清残留，再按需置 starting/busy）
    emitTokenUsageForSession(sessionId);
    reassertActiveSessionState();
    // 5. 挂起交互重放：切回时重新弹可交互卡片（缓存来自后台期间的 ask_user/审批）
    const auto qIt = d->mPendingQuestions.constFind(sessionId);
    if (qIt != d->mPendingQuestions.constEnd()) {
        // 审计问题 17：重发前校验桥存活——挂起问题蕴含进程存活（Python 处于
        // interrupt 等待），竞态残留的死桥缓存不重发幽灵卡，直接作废
        DAAgentBridge* qb = d->mSessionBridges.value(sessionId);
        if (qb && qb->isRunning()) {
            emit agentQuestion(qIt->text, qIt->options, qIt->multiSelect);
        } else {
            d->mPendingQuestions.remove(sessionId);
        }
    }
    for (const QJsonValue& v : d->mPendingApprovalRequests.value(sessionId)) {
        const QJsonObject o = v.toObject();
        emit agentToolApprovalRequest(o.value("callId").toString(),
                                      o.value("toolName").toString(),
                                      o.value("args").toObject());
    }
    // 6. 温暖化：目标会话无桥且有预热空闲桥 → 接管并后台 load_session
    //    （下次发消息免冷启动；sendMessage 的 user_msg 在 stdin 管道中排在
    //     load_session 之后，时序安全）
    if (!d->mSessionBridges.contains(sessionId) && d->mIdleBridge) {
        adoptOrStartBridge(sessionId);
    }
    d->mSessionError.remove(sessionId);
    emit sessionListChanged(listSessionsForUI());
    return true;
}

/**
 * @brief 删除指定会话
 * @param sessionId 会话 ID
 *
 * concurrent-sessions：后台运行中的会话也可删除（用户明确意图）——
 * 先退役其桥（断开路由，退出期间信号不再落盘/转发）再删存储。
 */
void DAAgentModule::deleteSession(const QString& sessionId)
{
    DA_D(d);
    if (d->mSessionBridges.contains(sessionId)) {
        retireBridge(sessionId);
    }
    d->mSessionStore->deleteSession(sessionId);
    if (d->mCurrentSessionId == sessionId) {
        d->mCurrentSessionId.clear();  // 删当前会话后回归无活跃
        resetCumulativeTokens();       // 清零累计，避免残留被下一会话误用
        // 审计问题 16：补发 sessionCleared——修复前删除当前会话只发
        // sessionListChanged（Dock 仅缓存 payload+刷标题），聊天区保留已删
        // 会话全部气泡；下一条消息经 createSession 新建（有意不发
        // sessionCreated，MAJOR3 约定）→ 新会话气泡渲染在已删会话转写下方，
        // 两个会话内容视觉混合，token 标签残留旧值。Dock 已有完整
        // onSessionCleared 处理槽（清聊天区+复位 token+解除守卫），补发即恢复。
        emit sessionCleared();
        emitTokenUsageForSession(QString());  // 复位 token UI（全 0）
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
 * @return QVariantList，每元素 QVariantMap{id,title,createdAt,updatedAt,messageCount,state,inputTokens,outputTokens,totalTokens}
 */
QVariantList DAAgentModule::listSessionsForUI() const
{
    DA_DC(d);
    // 契约3：sessionListChanged 的 payload，每元素 QVariantMap{id,title,updatedAt,messageCount}
    // MAJOR4（round-3）：按当前工程路径过滤
    // 会话管理对话框需要 updatedAt/messageCount 展示更多会话信息。
    // concurrent-sessions：附 state 运行态（starting/running/waiting_input/error/"")，
    // 供会话管理对话框渲染后台会话角标。
    // 会话管理详情面板：附 createdAt 与 token 累计（-1=旧数据未统计，store 懒迁移后为真实值）。
    QVariantList out;
    for (const auto& m : d->mSessionStore->listSessions(d->mCurrentProjectPath)) {
        QVariantMap vm;
        vm["id"]           = m.id;
        vm["title"]        = m.title;
        vm["createdAt"]    = m.createdAt;       // ISO8601WithMs, UTC
        vm["updatedAt"]    = m.updatedAt;       // ISO8601WithMs, UTC
        vm["messageCount"] = m.messageCount;
        vm["state"]        = sessionRuntimeState(m.id);
        vm["inputTokens"]  = m.inputTokens;
        vm["outputTokens"] = m.outputTokens;
        vm["totalTokens"]  = m.totalTokens;
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
 * @brief 导出活跃会话字节（plan-05 工程保存调用）
 * @return id -> jsonl 字节
 *
 * concurrent-sessions：导出当前会话 + 后台运行中的会话（对话内容实时落盘
 * JSONL，此处取字节即最新态；总纲 D3：保存工程时复制活跃会话进 zip）。
 */
QHash<QString, QByteArray> DAAgentModule::exportActiveSessions() const
{
    DA_DC(d);
    QStringList ids;
    if (!d->mCurrentSessionId.isEmpty()) {
        ids.append(d->mCurrentSessionId);
    }
    for (auto it = d->mSessionBridges.constBegin(); it != d->mSessionBridges.constEnd(); ++it) {
        if (it.key() != d->mCurrentSessionId && it.value() && it.value()->isRunning()) {
            ids.append(it.key());
        }
    }
    if (ids.isEmpty()) return {};
    return d->mSessionStore->exportSessionFiles(ids);
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
    // 权限层：同步 ${project} 变量（工程文件所在目录），供路径规则解析
    if (d->mPermissionManager) {
        d->mPermissionManager->setProjectDir(path.isEmpty() ? QString() : QFileInfo(path).absolutePath());
    }
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
    showDockWidget();
    const DAAgentLLMConfig c = d->mConfig.llm();
    if (c.baseUrl().isEmpty() || c.apiKey().isEmpty() || c.model().isEmpty()) {
        daWarning << tr("LLM is not configured, skip agent analysis. "
                        "Please configure LLM in settings first.");  //cn:LLM 未配置，跳过 Agent 分析，请先在设置中配置 LLM
        emit systemMessage(tr("LLM is not configured. Please configure LLM in settings first."),  //cn:LLM 未配置，请先在设置中配置 LLM
                            QStringLiteral("warning"));
        return false;
    }
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

// ===========================================================================
// 权限层接口实现（permission-layer P1，契约 1 一次性 ABI 批处理）
// ===========================================================================

/**
 * @brief 获取权限配置（模式/审批超时/判官/规则/危险模式/分级覆盖）
 * @return 权限配置结构体（权限引擎未就绪时返回默认值）
 */
DAAgentPermissionConfig DAAgentModule::getPermissionConfig() const
{
    DA_DC(d);
    DAAgentPermissionConfig c;
    if (!d->mPermissionManager) {
        return c;
    }
    return d->mPermissionManager->getConfig();
}

/**
 * @brief 写入权限配置（标量稀疏守卫；模式变更经信号 + reconfigure 同步）
 * @param config 权限配置结构体
 */
void DAAgentModule::setPermissionConfig(const DAAgentPermissionConfig& config)
{
    DA_D(d);
    if (!d->mPermissionManager) {
        return;
    }
    const QString oldMode = d->mPermissionManager->mode();
    d->mPermissionManager->setConfig(config);
    // 模式变更 → 通知 UI + 同步全部存活桥（Python 据模式决定是否判定）；
    // 非模式字段（判官/超时/危险模式等）变更同样广播
    if (d->mPermissionManager->mode() != oldMode) {
        emit permissionModeChanged(d->mPermissionManager->mode());
    }
    forEachLiveBridge([this](DAAgentBridge* b) {
        b->reconfigureAgent(d_func()->mConfig.toRunnerConfigJson());
    });
}

/**
 * @brief 获取当前权限模式
 * @return yolo / auto / manual（默认 yolo 全自动）
 */
QString DAAgentModule::getPermissionMode() const
{
    DA_DC(d);
    return d->mPermissionManager ? d->mPermissionManager->mode() : DAAgentPermissionManager::modeYolo();
}

/**
 * @brief 设置权限模式（写配置 + emit + 运行中经 reconfigure 热同步，A2 即时生效）
 * @param mode yolo / auto / manual（非法值忽略）
 */
void DAAgentModule::setPermissionMode(const QString& mode)
{
    DA_D(d);
    if (!d->mPermissionManager) {
        return;
    }
    if (mode != DAAgentPermissionManager::modeYolo() && mode != DAAgentPermissionManager::modeAuto()
        && mode != DAAgentPermissionManager::modeManual()) {
        qWarning() << "DAAgentModule::setPermissionMode: invalid mode ignored:" << mode;
        return;
    }
    if (d->mPermissionManager->mode() == mode) {
        return;  // 幂等：避免重复 emit/无谓 reconfigure
    }
    d->mPermissionManager->setMode(mode);
    emit permissionModeChanged(mode);
    // concurrent-sessions：模式即时生效需同步全部存活桥（A2）
    forEachLiveBridge([this](DAAgentBridge* b) {
        b->reconfigureAgent(d_func()->mConfig.toRunnerConfigJson());
    });
}

/**
 * @brief 用户对审批卡的裁决，按 callId 路由到对应会话的桥
 * @param callId 工具调用 ID
 * @param approved 是否批准
 * @param rememberSession 是否本会话记住（仅 file_write 生效，A5）
 *
 * concurrent-sessions：交互卡只在活跃会话渲染，但缓存中的 callId 可能属于
 * 刚切离的会话（用户切走后再切回裁决）——按 mApprovalSessionByCallId
 * 映射路由到正确的桥，并同步清理挂起缓存与角标。
 */
void DAAgentModule::sendToolApproval(const QString& callId, bool approved, bool rememberSession)
{
    DA_D(d);
    const QString sid = d->mApprovalSessionByCallId.value(callId);
    DAAgentBridge* bridge = d->mSessionBridges.value(sid);
    if (bridge) {
        bridge->onToolApproval(callId, approved, rememberSession);
    }
    // 已裁决：清路由表与挂起缓存（角标消失）
    d->mApprovalSessionByCallId.remove(callId);
    QJsonArray& arr = d->mPendingApprovalRequests[sid];
    for (int i = 0; i < arr.size(); ++i) {
        if (arr.at(i).toObject().value("callId").toString() == callId) {
            arr.removeAt(i);
            break;
        }
    }
    emit sessionListChanged(listSessionsForUI());
}

/**
 * @brief 设置脚本工作区根目录（${workspace} 变量 + 下发 Python，契约见母文档 §6.1）
 *
 * 复用 setCurrentProjectPath 的 L5 注入模式：由 DAAppProject 经
 * core()->getAgentInterface() 多态调用。存成员并转发权限引擎；运行中的
 * 子进程经 reconfigure 同步 workspace_root（供 run_script 判定解析）。
 * @param dir 工作区根目录（空=未保存工程/启动无工程）
 */
void DAAgentModule::setScriptWorkspaceDir(const QString& dir)
{
    DA_D(d);
    d->mScriptWorkspaceDir = dir;
    if (d->mPermissionManager) {
        d->mPermissionManager->setWorkspaceRoot(dir);
    }
    // concurrent-sessions：workspace_root 变更同步全部存活桥
    forEachLiveBridge([this](DAAgentBridge* b) {
        b->reconfigureAgent(d_func()->mConfig.toRunnerConfigJson());
    });
}

/**
 * @brief 推送当前权限模式到 Dock（与 pushModelSelection 同处，启动初始化点调用）
 *
 * A13：若启动读到用户显式设置的 yolo，Dock 侧据本信号弹一次确认卡
 * （"上次留在全自动模式…"），用户拒绝则降级为 auto；yolo 来自默认值
 * （未显式设置）时不弹卡，静默进入全自动。本方法推送当前态与显式标志。
 */
void DAAgentModule::pushPermissionMode()
{
    DA_D(d);
    emit permissionModeChanged(getPermissionMode());
    emit permissionModeExplicitChanged(d->mPermissionManager ? d->mPermissionManager->modeExplicitlySet() : false);
}

// ===========================================================================
// 子 agent 管理接口实现（subagent-phase1）
// ===========================================================================

/**
 * @brief 注册内置子 agent 定义（委托 DAAgentSubagentManager，镜像 registerBuiltinAgent）
 * @param name 子 agent 名称（文件名）
 * @param content md + frontmatter 全文
 */
void DAAgentModule::registerBuiltinSubagent(const QString& name, const QString& content)
{
    DA_D(d);
    if (d->mSubagentManager) {
        d->mSubagentManager->registerBuiltin(name, content);
        d->mSubagentManager->loadSubagents();
    }
}

/**
 * @brief 获取所有子 agent 定义（管理 UI 数据源）
 * @return JSON 数组，每元素 {name, description, tools, system_prompt, permissions?}
 */
QJsonArray DAAgentModule::subagentDefinitions() const
{
    DA_DC(d);
    QJsonArray arr;
    if (!d->mSubagentManager) {
        return arr;
    }
    const QList<DAAgentSubagentDef> list = d->mSubagentManager->subagents();
    for (const DAAgentSubagentDef& s : list) {
        arr.append(s.toJsonObject());
    }
    return arr;
}

/**
 * @brief 保存子 agent 定义（新增或更新；oldName 非空表示重命名）
 * @param def 定义 JSON（name/description/tools/system_prompt/permissions?）
 * @param oldName 旧名称（重命名场景，定位旧文件删除）
 * @return 保存成功返回 true
 *
 * 保存时校验工具白名单：引用未注册工具仅告警不拒绝（Python 侧求交剔除为
 * 双保险，母文档 §9 风险表）。成功后若子进程在跑则经桥下发 update_subagents
 * 热更新（Q17）；定义列表变化信号由 manager loadSubagents 透传。
 */
bool DAAgentModule::saveSubagent(const QJsonObject& def, const QString& oldName)
{
    DA_D(d);
    if (!d->mSubagentManager) {
        return false;
    }
    const DAAgentSubagentDef subDef = DAAgentSubagentDef::fromJsonObject(def);
    if (!subDef.isValid()) {
        qWarning("DAAgentModule::saveSubagent: definition has an empty name, save rejected");
        return false;
    }
    // 白名单校验：引用未注册工具告警（Python 侧求交剔除兜底）
    for (const QString& t : subDef.tools) {
        if (!d->mTools.contains(t)) {
            qWarning("DAAgentModule::saveSubagent: subagent '%s' references unregistered tool '%s'",
                     qPrintable(subDef.name), qPrintable(t));
        }
    }
    if (!d->mSubagentManager->saveSubagent(subDef, oldName)) {
        return false;
    }
    // concurrent-sessions：定义热更新广播到全部存活桥（Q17）
    forEachLiveBridge([this](DAAgentBridge* b) {
        b->sendUpdateSubagents(assembleSubagentDefs());
    });
    return true;
}

/**
 * @brief 删除指定名称的子 agent 定义
 * @param name 子 agent 名称
 * @return 删除成功返回 true
 *
 * 成功后若子进程在跑则经桥下发 update_subagents 热更新（Q17）；
 * 定义全部删除后 Python 侧不再注入 dispatch_subagents 工具。
 */
bool DAAgentModule::deleteSubagent(const QString& name)
{
    DA_D(d);
    if (!d->mSubagentManager) {
        return false;
    }
    if (!d->mSubagentManager->deleteSubagent(name)) {
        return false;
    }
    // concurrent-sessions：定义热更新广播到全部存活桥（Q17）
    forEachLiveBridge([this](DAAgentBridge* b) {
        b->sendUpdateSubagents(assembleSubagentDefs());
    });
    return true;
}

/**
 * @brief 已注册工具名列表（子 agent 编辑器工具白名单复选框数据源）
 * @return 工具名列表（注册表键序）
 */
QStringList DAAgentModule::registeredToolNames() const
{
    DA_DC(d);
    return d->mTools.keys();
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
 * @brief 清理旧会话（读配置的 maxSessions/sessionRetentionDays）
 */
void DAAgentModule::cleanupSessions()
{
    DA_D(d);
    // 默认值由 DAAgentLLMConfig 兜底（20/30，与旧 agent-config.ini 时代一致）
    const DAAgentLLMConfig c = d->mConfig.llm();
    d->mSessionStore->cleanupOldSessions(c.maxSessions(), c.sessionRetentionDays(), d->mCurrentSessionId);  // 跳过当前活跃 + lastActive
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
 * @brief 读取当前激活模型的上下文窗口
 * @return context_window 值（默认 262144，与 DAAgentLLMConfig 一致）
 */
int DAAgentModule::readContextWindow() const
{
    DA_DC(d);
    // 读内存配置模型（默认值由 DAAgentLLMConfig 兜底），供 agentUsage lambda
    // 与 emitTokenUsageForSession 复用，避免魔法数字散落
    return d->mConfig.llm().contextWindow();
}

/**
 * @brief 扫描会话持久化 usage 记录求和，emit tokenUsageUpdated（会话累计值）
 * @param sid 会话 ID
 *
 * 切换会话时从 JSONL 重算会话累计 token（所有真实 usage 记录之和，含 summary），
 * 同步刷新该会话的累计缓存，使 UI 显示该会话的总消耗。streaming_estimate
 * 不持久化故不参与。无记录则全 0（仍带真实 context_window，
 * UI 显示 tokens: 0 / 窗口、进度条 0%）。
 */
void DAAgentModule::emitTokenUsageForSession(const QString& sid)
{
    DA_D(d);
    // concurrent-sessions：先清该会话累计缓存，再从持久化 usage 记录求和重算
    //（switchSession 由此恢复累计态；后台会话的累计缓存不受影响）
    d->mCumulativeInTokens[sid] = 0;
    d->mCumulativeOutTokens[sid] = 0;
    d->mCumulativeTotalTokens[sid] = 0;
    QString src;
    if (!sid.isEmpty()) {
        QVector<QJsonObject> records = d->mSessionStore->readAllRecords(sid);
        for (const QJsonObject& obj : std::as_const(records)) {
            if (obj.value("type").toString() != "usage") continue;
            QJsonObject meta = obj.value("usage_metadata").toObject();
            d->mCumulativeInTokens[sid]    += meta.value("input_tokens").toInt(0);
            d->mCumulativeOutTokens[sid]   += meta.value("output_tokens").toInt(0);
            d->mCumulativeTotalTokens[sid] += meta.value("total_tokens").toInt(0);
            src = meta.value("source").toString();  // 取最后一条 source 作展示
        }
    }
    emit tokenUsageUpdated(d->mCumulativeInTokens[sid], d->mCumulativeOutTokens[sid],
                           d->mCumulativeTotalTokens[sid], readContextWindow(), src);
}

/**
 * @brief 会话累计 token 清零（新建/删除当前/恢复时调用）
 *
 * 配合 mCurrentSessionId 的变更点：新建会话（0 消耗）、删除当前会话、
 * 启动/开工程恢复（始终以全新对话开始）。switchSession 不调用本方法——
 * 它经 emitTokenUsageForSession 先清零再从 JSONL 重算恢复累计态。
 * concurrent-sessions：仅清当前会话项；retireBridge 清理桥所属会话项。
 */
void DAAgentModule::resetCumulativeTokens()
{
    DA_D(d);
    if (!d->mCurrentSessionId.isEmpty()) {
        d->mCumulativeInTokens.remove(d->mCurrentSessionId);
        d->mCumulativeOutTokens.remove(d->mCurrentSessionId);
        d->mCumulativeTotalTokens.remove(d->mCurrentSessionId);
    }
}

} // namespace DA
