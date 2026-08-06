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
#include "DAAbstractAgentTool.h"

namespace DA
{
class DAAgentBridge;
class DAAgentSessionStore;

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
    /**
     * @brief 构造函数
     * @param core 核心接口指针
     * @param parent 父对象
     */
    explicit DAAgentModule(DACoreInterface* core, QObject* parent = nullptr);
    /**
     * @brief 析构函数
     */
    ~DAAgentModule();

    /**
     * @brief 使用核心接口初始化模块
     * @param core 核心接口指针
     */
    void initialize(DACoreInterface* core);

    /// @copydoc DAAgentInterface::registerTool
    void registerTool(DAAbstractAgentTool* tool) override;
    /// @copydoc DAAgentInterface::registerSystemPrompt
    void registerSystemPrompt(const QString& name, const QString& content) override;
    /// @copydoc DAAgentInterface::showDockWidget
    void showDockWidget() override;
    /// @copydoc DAAgentInterface::hideDockWidget
    void hideDockWidget() override;
    /// @copydoc DAAgentInterface::sendMessage
    void sendMessage(const QString& text) override;
    /// @copydoc DAAgentInterface::stop
    void stop() override;
    /// @copydoc DAAgentInterface::sendUserAnswer
    void sendUserAnswer(const QString& answer) override;
    /// @copydoc DAAgentInterface::isRunning
    bool isRunning() const override;
    /// @copydoc DAAgentInterface::getLLMConfig
    QJsonObject getLLMConfig() const override;
    /// @copydoc DAAgentInterface::setLLMConfig
    void setLLMConfig(const QJsonObject& config) override;

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

    // ---- 会话管理辅助方法（非接口，plan-03 声明归属本计划） ----
    /**
     * @brief 新建会话（UI "+" 按钮用）—— createSession + emit sessionCreated
     *
     * MAJOR3（round-3）：与 createSession 区分——
     * sendMessage 自动建会话调 createSession（不 emit sessionCreated，避免 clearChat 擦除用户消息）；
     * UI "+" 调 newSession（emit sessionCreated 触发 plan-04 onSessionCreated → clearChat）。
     */
    void newSession() override;
    /**
     * @brief 清理旧会话（配置 key 由 plan-06 定义，调用点由 plan-05 接入）
     */
    void cleanupSessions();
    /**
     * @brief 包装 store.setSessionProjectPath，供 plan-05 saveAs/save 成功后更新当前会话工程路径
     *
     * 契约5：同步更新 m_currentProjectPath + store.setSessionProjectPath + store.setLastActive。
     * 保持 Module 方法（不上接口）——由 DAAppController qobject_cast 调用。
     */
    void setSessionProjectPathForCurrent(const QString& path);
    /**
     * @brief 恢复上次活跃会话（plan-05 在启动/打开工程后调用）
     */
    void restoreLastActiveSession();

private:
    DACoreInterface* m_core;
    DAAgentBridge* m_bridge = nullptr;
    QMap<QString, DAAbstractAgentTool*> m_tools;        // tool name → impl
    QHash<QString, QString> m_systemPrompts;            // prompt name → content
    // ---- 会话持久化成员（plan-03） ----
    DAAgentSessionStore* m_sessionStore = nullptr;  // 非 QObject 无参构造；initialize() 内 new、析构显式 delete（CRITICAL1）
    QString m_currentSessionId;                     // 当前活跃会话
    QString m_currentProjectPath;                   // 由 DAAppController::setCurrentProjectPath 注入（plan-05）；空=自由会话
    QQueue<QString> m_pendingToolCallUuids;         // 契约6：待配对的 tool_call/question 记录 uuid 队列（FIFO），供后续 tool_result/answer 配对 tool_call_id
    bool m_agentBusy = false;                       // 由 agentBusy(bool)/agentDone 信号维护，switchSession 入口守忙碌态
    // 懒启动→ready 串联 load_session 的缓存（替代一次性 QMetaObject::Connection，避免多次连接泄漏）
    QString m_pendingLoadSessionId;
    QJsonArray m_pendingLoadMessages;
    // 忙碌态切换排队：switchSession 遇 busy 时 requestStop 并缓存，agentDone 后续切
    QString m_pendingSwitchSessionId;
    // Helper methods（plan-04 填充实现）
    void connectSignals();
    void registerBuiltinTools();      // plan-05 填充真实工具注册
    void startAgentInternal();
    QString assembleSystemPrompt() const;
    QJsonArray assembleToolSpecs() const;
    // Python 解释器与 agent 脚本路径解析（plan-04 §4）
    QString detectPythonExePath() const;
    QString detectAgentScriptPath() const;
    QString detectSystemPromptPath() const;
    // ---- 会话持久化私有辅助方法（plan-03） ----
    /// 构造 tool_call 记录并追加写盘，返回本条记录 uuid（供 m_pendingToolCallUuids 入队）
    QString appendToolCallRecord(const QString& sid, const QString& tool, const QJsonObject& args);
    /// 构造 tool_result 记录并追加写盘（content 为明文 QString）
    void appendToolResultRecord(const QString& sid, const QString& toolCallId, const QString& content);
    /// 构造 assistant 记录并追加写盘
    void appendAssistantRecord(const QString& sid, const QString& text, const QJsonArray& toolCalls);
    /// 构造 usage 记录并追加写盘
    void appendUsageRecord(const QString& sid, int inT, int outT, int tot, const QString& src);
    /// 构造 user 记录对象（不写盘，供调用方 appendRecord）
    QJsonObject makeUserRecord(const QString& text) const;
    /// 生成 sessionListChanged 的 payload：按 m_currentProjectPath 过滤，每元素 QVariantMap{id,title}
    QVariantList listSessionsForUI() const;
};
} // namespace DA
