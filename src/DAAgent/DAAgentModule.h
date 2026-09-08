// DAAgentModule.h
#pragma once
#include "DAAgentInterface.h"
#include "DACoreInterface.h"
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QQueue>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <functional>
#include "DAAbstractAgentTool.h"
#include "DAGlobals.h"

namespace DA
{
class DAAgentBridge;
class DAAgentSessionStore;
class DAAgentManager;
class DAAgentSubagentManager;
class DAAgentPermissionManager;

/**
 * @brief DAAgent 模块的完整实现：工具注册、系统提示词组装、懒启动生命周期管理
 *
 * DAAgentModule 作为 DAAgentInterface 的实现类，协调 DAAgentBridge（子进程管理）
 * 并转发 agent 生命周期信号到接口。Dock（聊天 UI）由 DAAppController 经接口
 * 直接 connect，本模块不持有也不感知 Dock（plan-02 决策 D3b）。插件通过
 * registerTool / registerSystemPrompt 注入领域内容。
 */
class DAAgent_API DAAgentModule : public DAAgentInterface
{
    Q_OBJECT
public:
    // 构造函数
    explicit DAAgentModule(DACoreInterface* core, QObject* parent = nullptr);
    // 析构函数
    virtual ~DAAgentModule() override;

    // 使用核心接口初始化模块
    void initialize(DACoreInterface* core);

    /// @copydoc DAAgentInterface::registerTool
    bool registerTool(DAAbstractAgentTool* tool) override;
    /// @copydoc DAAgentInterface::registerSystemPrompt
    void registerSystemPrompt(const QString& name, const QString& content) override;
    void registerSystemPrompt(const QString& name, const QString& content, QObject* provider) override;

    // ---- 插件热插拔（plugin-hotswap，override DAAgentInterface 2 个新纯虚） ----
    /// @copydoc DAAgentInterface::unregisterToolsByProvider
    int unregisterToolsByProvider(QObject* provider) override;
    /// @copydoc DAAgentInterface::unregisterSystemPromptsByProvider
    int unregisterSystemPromptsByProvider(QObject* provider) override;
    /// @copydoc DAAgentInterface::showDockWidget
    void showDockWidget() override;
    /// @copydoc DAAgentInterface::hideDockWidget
    void hideDockWidget() override;
    /// @copydoc DAAgentInterface::sendMessage
    void sendMessage(const QString& text) override;
    /// @copydoc DAAgentInterface::stop
    void stop() override;
    /// @copydoc DAAgentInterface::shutdown
    void shutdown() override;
    /// @copydoc DAAgentInterface::sendUserAnswer
    void sendUserAnswer(const QString& answer) override;
    /// @copydoc DAAgentInterface::isRunning
    bool isRunning() const override;
    /// @copydoc DAAgentInterface::getLLMConfig
    DAAgentLLMConfig getLLMConfig() const override;
    /// @copydoc DAAgentInterface::setLLMConfig
    void setLLMConfig(const DAAgentLLMConfig& config) override;

    // ---- 供应商与多模型管理（override DAAgentInterface 6 个新纯虚） ----
    /// @copydoc DAAgentInterface::getProviders
    QList< DAAgentProvider > getProviders() const override;
    /// @copydoc DAAgentInterface::setProviders
    void setProviders(const QList< DAAgentProvider >& providers) override;
    /// @copydoc DAAgentInterface::getActiveProvider
    QString getActiveProvider() const override;
    /// @copydoc DAAgentInterface::getAvailableModels
    QList< DAAgentModelRef > getAvailableModels() const override;
    /// @copydoc DAAgentInterface::getActiveModel
    QString getActiveModel() const override;
    /// @copydoc DAAgentInterface::setActiveModel
    void setActiveModel(const QString& provider, const QString& model) override;

    // ---- 会话管理接口（plan-03，override DAAgentInterface 9 个新纯虚） ----
    /// @copydoc DAAgentInterface::createSession
    QString createSession() override;
    /// @copydoc DAAgentInterface::switchSession
    bool switchSession(const QString& sessionId) override;
    /// @copydoc DAAgentInterface::deleteSession
    void deleteSession(const QString& sessionId) override;
    /// @copydoc DAAgentInterface::renameSession
    void renameSession(const QString& sessionId, const QString& title) override;
    /// @copydoc DAAgentInterface::listSessions
    QVariantList listSessions() const override;
    /// @copydoc DAAgentInterface::currentSessionId
    QString currentSessionId() const override;
    /// @copydoc DAAgentInterface::exportActiveSessions
    QHash<QString, QByteArray> exportActiveSessions() const override;
    /// @copydoc DAAgentInterface::loadSessionsFromProject
    void loadSessionsFromProject(const QHash<QString, QByteArray>& files, const QString& projectPath) override;
    /// @copydoc DAAgentInterface::setCurrentProjectPath
    void setCurrentProjectPath(const QString& path) override;

    // ---- 提示词库管理（override DAAgentInterface 3 个新纯虚） ----
    /// @copydoc DAAgentInterface::registerBuiltinAgent
    void registerBuiltinAgent(const QString& name, const QString& content) override;
    /// @copydoc DAAgentInterface::runAgent
    bool runAgent(const QString& title) override;
    /// @copydoc DAAgentInterface::agentPromptOps
    DAAgentPromptOps* agentPromptOps() override;

    // ---- 权限层（override DAAgentInterface 契约 1 一次性 ABI 批处理 6 个纯虚） ----
    /// @copydoc DAAgentInterface::getPermissionConfig
    DAAgentPermissionConfig getPermissionConfig() const override;
    /// @copydoc DAAgentInterface::setPermissionConfig
    void setPermissionConfig(const DAAgentPermissionConfig& config) override;
    /// @copydoc DAAgentInterface::getPermissionMode
    QString getPermissionMode() const override;
    /// @copydoc DAAgentInterface::setPermissionMode
    void setPermissionMode(const QString& mode) override;
    /// @copydoc DAAgentInterface::sendToolApproval
    void sendToolApproval(const QString& callId, bool approved, bool rememberSession) override;
    /// @copydoc DAAgentInterface::setScriptWorkspaceDir
    void setScriptWorkspaceDir(const QString& dir) override;

    // ---- 子 agent 管理（subagent-phase1，override DAAgentInterface 5 个新纯虚） ----
    /// @copydoc DAAgentInterface::registerBuiltinSubagent
    void registerBuiltinSubagent(const QString& name, const QString& content) override;
    /// @copydoc DAAgentInterface::subagentDefinitions
    QJsonArray subagentDefinitions() const override;
    /// @copydoc DAAgentInterface::saveSubagent
    bool saveSubagent(const QJsonObject& def, const QString& oldName = QString()) override;
    /// @copydoc DAAgentInterface::deleteSubagent
    bool deleteSubagent(const QString& name) override;
    /// @copydoc DAAgentInterface::registeredToolNames
    QStringList registeredToolNames() const override;

    // ---- 会话管理辅助方法（非接口，plan-03 声明归属本计划） ----
    // 新建会话（UI "+" 按钮用）—— createSession + emit sessionCreated
    void newSession() override;
    // 清理旧会话（配置 key 由 plan-06 定义，调用点由 plan-05 接入）
    void cleanupSessions();
    // 包装 store.setSessionProjectPath，供 plan-05 saveAs/save 成功后更新当前会话工程路径
    void setSessionProjectPathForCurrent(const QString& path);
    // 启动/打开工程后初始化会话 UI——填充下拉列表但不自动恢复上次会话
    void restoreLastActiveSession();
    // 预启动 agent 子进程（程序启动时调用，受 auto_prestart 配置开关 + LLM 配置就绪控制）
    void prestartAgent();
    // 推送当前供应商/模型选择到 Dock（emit availableModelsChanged + activeModelChanged）
    // 由 DAAppController 在接口↔Dock 信号链 connect 完成后调用
    void pushModelSelection();
    // 推送当前权限模式到 Dock（emit permissionModeChanged + permissionModeExplicitChanged），
    // 与 pushModelSelection 同处，由 DAAppController 在接口↔Dock 信号链 connect 完成后调用；
    // A13 启动 yolo 确认由 Dock 侧触发（仅显式设置的 yolo 弹卡，默认值静默进入全自动）
    void pushPermissionMode();
    // 会话运行态（供 UI 角标）："starting" / "running" / "waiting_input" / "error" / ""（空闲）
    // 公开供测试断言状态机（DAAgentModuleTest）；UI 常规消费走 sessionListChanged payload
    QString sessionRuntimeState(const QString& sessionId) const;

private:
    // Helper methods
    QString assembleSystemPrompt() const;
    QJsonArray assembleToolSpecs() const;
    // 组装子 agent 定义协议数组（随 init/update_subagents 下发，协议载荷四字段）
    QJsonArray assembleSubagentDefs() const;
    QString detectPythonExePath() const;
    QString detectAgentScriptPath() const;
    QString detectSystemPromptPath() const;
    QString appendToolCallRecord(const QString& sid, const QString& tool, const QJsonObject& args);
    void appendToolResultRecord(const QString& sid, const QString& toolCallId, const QString& content);
    void appendAssistantRecord(const QString& sid, const QString& text, const QJsonArray& toolCalls);
    void appendUsageRecord(const QString& sid, int inT, int outT, int tot, const QString& src);
    // 构造 error 记录并追加写盘（决策点 4：错误落盘 JSONL，审计问题 3）
    void appendErrorRecord(const QString& sid, const QString& message, const QString& errorType, const QString& detail);
    QJsonObject makeUserRecord(const QString& text) const;
    QVariantList listSessionsForUI() const;
    int readContextWindow() const;
    // "回合疑似未完成"提醒文案（实时转发与问题 8 切回重发共用）
    QString turnIncompleteMessage(int toolRounds) const;
    void emitTokenUsageForSession(const QString& sid);
    // 会话累计 token 清零（新建/删除当前/恢复时调用）
    void resetCumulativeTokens();

    // ---- 会话桥管理（concurrent-sessions：多子进程并发会话） ----
    // 为会话冷启动新桥（含 LLM/路径配置校验，失败返回 nullptr 并报错）
    DAAgentBridge* createBridgeForSession(const QString& sessionId);
    // 绑定会话：注册映射 + 连接全部信号路由（持久化写桥所属会话、UI 仅活跃会话）
    void attachBridge(DAAgentBridge* bridge, const QString& sessionId);
    // 确保会话有桥：优先接管预热空闲桥，否则冷启动；历史非空时管道序下发 load_session
    //（excludeTrailingUser：快照剔除末尾待重发 user 记录，问题10 统一约定）
    DAAgentBridge* adoptOrStartBridge(const QString& sessionId, bool excludeTrailingUser = false);
    // 读取 load_session 历史快照（统一约定：永不含将被重发的末尾 user 记录）
    QJsonArray readSessionSnapshotForLoad(const QString& sessionId, bool excludeTrailingUser) const;
    // 优雅退役：断开路由、清缓存、requestStop + processExited 后 deleteLater
    void retireBridge(const QString& sessionId);
    // 查会话桥（无返回 nullptr）
    DAAgentBridge* bridgeForSession(const QString& sessionId) const;
    // 重断言活跃会话 UI 运行态（switchSession step4 / newSession 共用，问题14）
    void reassertActiveSessionState();
    // 遍历全部存活桥（会话桥 + 预热桥），fn 内不得增删桥
    void forEachLiveBridge(const std::function<void(DAAgentBridge*)>& fn);

    DA_DECLARE_PRIVATE(DAAgentModule)
};
} // namespace DA
