#pragma once
#include "DAAgentAPI.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QVariantList>
#include <QVector>
#include <QString>
#include <QStringList>
#include "DAAbstractAgentTool.h"

namespace DA
{
/**
 * @brief DAAgent 模块的公共接口
 *
 * 此接口由 DAAgentModule 实现，插件通过此接口注册工具、系统提示词、
 * 控制 agent 的 UI 显隐与消息收发，以及配置 LLM 参数（base_url、api_key、model）。
 */
class DAAgent_API DAAgentInterface : public QObject
{
    Q_OBJECT
public:
    explicit DAAgentInterface(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~DAAgentInterface() = default;

    /**
     * @brief 注册工具供 agent 使用
     * @param tool 工具实现指针
     */
    virtual void registerTool(DAAbstractAgentTool* tool) = 0;
    /**
     * @brief 注册命名系统提示词片段
     * @param name 提示词片段名称
     * @param content 提示词内容
     */
    virtual void registerSystemPrompt(const QString& name, const QString& content) = 0;

    /**
     * @brief 显示 agent dock 窗口
     */
    virtual void showDockWidget() = 0;
    /**
     * @brief 隐藏 agent dock 窗口
     */
    virtual void hideDockWidget() = 0;

    /**
     * @brief 发送用户消息给 agent
     * @param text 用户消息文本
     */
    virtual void sendMessage(const QString& text) = 0;
    /**
     * @brief 停止正在运行的 agent（用户主动终止）
     *
     * 非阻塞调用，进程退出后通过 agentBusy(false) 信号通知 UI 恢复。
     */
    virtual void stop() = 0;
    /**
     * @brief 转发用户对 agent 提问的回答给子进程
     *
     * 方法体吸收原 connectSignals 中 dock::userAnswerSelected 的持久化 lambda
     * （appendToolResultRecord），plan-02 删 lambda 后由 DAAppController 经
     * dock::userAnswerSelected → interface::sendUserAnswer 信号→方法直连，
     * 单次执行持久化 + 转发子进程（无双重）。
     * @param answer 用户回答文本
     */
    virtual void sendUserAnswer(const QString& answer) = 0;
    /**
     * @brief 新建会话（UI "+" 按钮入口）—— createSession + emit sessionCreated
     *
     * 跨计划协调（plan-02）：Dock 的 sessionCreateRequested 连此方法（而非 createSession()），
     * 因 Module 既有的 void newSession() 会 emit sessionCreated 触发 UI clearChat，
     * 而 createSession() 不 emit → 连错会导致 UI 不清空。
     * newSession 是方法不是信号。
     */
    virtual void newSession() = 0;
    /**
     * @brief 检查 agent 是否正在运行
     * @return 若 agent 正在运行返回 true
     */
    virtual bool isRunning() const = 0;

    /**
     * @brief 获取 LLM 配置
     * @return LLM 配置 JSON（base_url、api_key、model）
     */
    virtual QJsonObject getLLMConfig() const = 0;
    /**
     * @brief 设置 LLM 配置
     * @param config LLM 配置 JSON（base_url、api_key、model）
     */
    virtual void setLLMConfig(const QJsonObject& config) = 0;

    // ---- 会话管理（plan-03 新增，破坏性接口变更，插件需重编译；AGENTS.md plan-06 标注） ----
    /**
     * @brief 创建新会话，返回新 sessionId
     *
     * MAJOR3（round-3）：此方法不 emit sessionCreated（若 Module 实现如此），
     * 供 sendMessage 自动建会话用——避免触发 UI clearChat 擦除刚显示的用户消息。
     * UI "+" 按钮应调 newSession()（有 sessionCreated 信号）。
     */
    virtual QString createSession() = 0;
    /**
     * @brief 切换到指定会话（懒启动→下发历史→恢复）
     * @param sessionId 目标会话 ID
     * @return 是否启动切换流程（false=已是当前会话无操作）
     */
    virtual bool switchSession(const QString& sessionId) = 0;
    /**
     * @brief 删除指定会话
     */
    virtual void deleteSession(const QString& sessionId) = 0;
    /**
     * @brief 重命名指定会话
     */
    virtual void renameSession(const QString& sessionId, const QString& title) = 0;
    /**
     * @brief 列出所有会话（供 UI 下拉）
     * @return QVariantList，每元素 QVariantMap（id/title/createdAt/updatedAt/messageCount/projectPath）
     */
    virtual QVariantList listSessions() const = 0;
    /**
     * @brief 获取当前活跃会话 ID
     */
    virtual QString currentSessionId() const = 0;
    /**
     * @brief 导出当前活跃会话字节（plan-05 工程保存调用）
     * @return id -> jsonl 字节
     */
    virtual QHash<QString, QByteArray> exportActiveSessions() const = 0;
    /**
     * @brief 从工程 zip 加载会话文件（plan-05 工程加载调用）
     * @param files id -> jsonl 字节
     * @param projectPath 绑定工程路径（契约4：标记导入会话工程路径）
     */
    virtual void loadSessionsFromProject(const QHash<QString, QByteArray>& files, const QString& projectPath) = 0;
    /**
     * @brief 设置当前工程路径（CRITICAL round-3：提升为接口纯虚）
     *
     * 供 DAAppProject（L5）经 core()->getAgentInterface() 多态调用
     * （onProjectLoaded / 工程关闭时注入），不依赖 qobject_cast。
     * 空=自由会话模式；非空=工程绑定模式（影响会话过滤与 last_active）。
     */
    virtual void setCurrentProjectPath(const QString& path) = 0;

Q_SIGNALS:
    // ---- 以下 10 个由 DAAgentModule 从 DAAgentBridge 转发 ----
    /// agent 生成 token 时发射（流式渲染）
    void agentToken(const QString& token);
    /// agent 消息生成完成时发射
    void agentMessageComplete(const QString& fullText);
    /// agent 发起工具调用时发射
    void agentToolCall(const QString& toolName, const QJsonObject& args);
    /// 工具执行结果返回时发射
    void agentToolResult(const QString& toolName, const QJsonObject& result);
    /// agent 向用户提问时发射
    void agentQuestion(const QString& text, const QStringList& options, bool multiSelect);
    /// agent 发生错误时发射
    void agentError(const QString& message, const QString& errorType = QString(), const QString& detail = QString());
    /// agent 正在重试 LLM 调用时发射（Python 端指数退避期间每次重试发一次）
    void agentRetrying(int attempt, int maxAttempts, int delayMs,
                       const QString& errorType, const QString& errorMessage);
    /// agent 就绪时发射
    void agentReady(const QString& model);
    /// agent 忙碌状态变化时发射
    void agentBusy(bool busy);
    /// agent 本轮处理完成时发射（生命周期事件，目前无 Dock 槽对接，纳入接口备扩展）
    void agentDone();
    /// agent 完成会话历史重建时发射
    void agentSessionLoaded(const QString& sessionId);
    // ---- 以下 5 个由 DAAgentModule 自身 emit（从 Module 的 Q_SIGNALS 上移） ----
    /// token 使用量更新（agentUsage lambda 内补 context_window 后 emit；switchSession 也会从持久化 usage 记录 emit）
    void tokenUsageUpdated(int inputTokens, int outputTokens, int totalTokens, int contextWindow, const QString& source);
    /// 切换会话完成时发射，供 UI 重放历史
    void sessionSwitched(const QString& sessionId, const QVector<QJsonObject>& allRecords);
    /// 新会话创建时发射（仅 newSession 路径，触发 UI clearChat）
    void sessionCreated(const QString& sessionId);
    /// 会话列表变化时发射，带 payload（每元素 QVariantMap{id,title,updatedAt,messageCount}）
    void sessionListChanged(QVariantList sessions);
    /// 当前无活跃会话时发射（启动/打开工程后不自动恢复上次会话，始终全新开始）
    ///
    /// 触发场景：打开一个无内嵌会话的工程时，需清空残留的游离会话聊天区与 token 统计，
    /// 并清空 m_currentSessionId（之后用户发消息由 sendMessage 懒创建绑定工程的会话）。
    /// Dock 收到后应 clearChat + 复位 token 控件 + 下拉不选中。
    void sessionCleared();
};
} // namespace DA
