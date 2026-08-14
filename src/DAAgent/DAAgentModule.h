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
#include "DAGlobals.h"

namespace DA
{
class DAAgentBridge;
class DAAgentSessionStore;
class DAAgentManager;

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
    /// @copydoc DAAgentInterface::shutdown
    void shutdown() override;
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

    // ---- 提示词库管理（override DAAgentInterface 3 个新纯虚） ----
    /// @copydoc DAAgentInterface::registerBuiltinAgent
    void registerBuiltinAgent(const QString& name, const QString& content) override;
    /// @copydoc DAAgentInterface::runAgent
    bool runAgent(const QString& title) override;
    /// @copydoc DAAgentInterface::agentPromptOps
    DAAgentPromptOps* agentPromptOps() override;

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

private:
    // Helper methods
    void connectSignals();
    void startAgentInternal();
    QString assembleSystemPrompt() const;
    QJsonArray assembleToolSpecs() const;
    QString detectPythonExePath() const;
    QString detectAgentScriptPath() const;
    QString detectSystemPromptPath() const;
    QString appendToolCallRecord(const QString& sid, const QString& tool, const QJsonObject& args);
    void appendToolResultRecord(const QString& sid, const QString& toolCallId, const QString& content);
    void appendAssistantRecord(const QString& sid, const QString& text, const QJsonArray& toolCalls);
    void appendUsageRecord(const QString& sid, int inT, int outT, int tot, const QString& src);
    QJsonObject makeUserRecord(const QString& text) const;
    QVariantList listSessionsForUI() const;
    int readContextWindow() const;
    void emitTokenUsageForSession(const QString& sid);
    // 会话累计 token 清零（新建/删除当前/恢复时调用）
    void resetCumulativeTokens();

    DA_DECLARE_PRIVATE(DAAgentModule)
};
} // namespace DA
