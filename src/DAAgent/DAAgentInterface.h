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
class DAAgentPromptOps;

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
    virtual ~DAAgentInterface() override = default;

    // 注册工具供 agent 使用；工具名为空或与已注册工具重复时拒绝注册并返回 false
    virtual bool registerTool(DAAbstractAgentTool* tool) = 0;
    // 注册命名系统提示词片段
    virtual void registerSystemPrompt(const QString& name, const QString& content) = 0;

    // 显示 agent dock 窗口
    virtual void showDockWidget() = 0;
    // 隐藏 agent dock 窗口
    virtual void hideDockWidget() = 0;

    // 发送用户消息给 agent
    virtual void sendMessage(const QString& text) = 0;
    // 停止正在运行的 agent（用户主动终止，非阻塞）
    virtual void stop() = 0;
    // 停止 agent 子进程并等待退出（阻塞，仅在应用关闭时调用）
    virtual void shutdown() = 0;
    // 转发用户对 agent 提问的回答给子进程
    virtual void sendUserAnswer(const QString& answer) = 0;
    // 新建会话（UI "+" 按钮入口）—— createSession + emit sessionCreated
    virtual void newSession() = 0;
    // 检查 agent 是否正在运行
    virtual bool isRunning() const = 0;

    // 获取 LLM 配置
    virtual QJsonObject getLLMConfig() const = 0;
    // 设置 LLM 配置
    virtual void setLLMConfig(const QJsonObject& config) = 0;

    // ---- 供应商与多模型管理（多供应商多模型） ----
    // 每个供应商含 name / base_url / api_key / models(模型 id 字符串数组)。
    // 激活供应商 + 激活模型决定实际下发给子进程的 base_url/api_key/model。
    /// 获取所有供应商配置（设置页 CRUD 用；api_key 已解密为明文返回）
    virtual QJsonArray getProviders() const = 0;
    /// 保存所有供应商配置（设置页 apply 用；api_key 明文传入，内部加密存储）
    virtual void setProviders(const QJsonArray& providers) = 0;
    /// 获取当前激活供应商名称
    virtual QString getActiveProvider() const = 0;
    /// 获取所有可选模型列表（Dock 下拉用，不含 api_key）：每元素 QVariantMap{provider,model}
    virtual QVariantList getAvailableModels() const = 0;
    /// 获取当前激活模型 id
    virtual QString getActiveModel() const = 0;
    /// 设置激活供应商+模型（Dock 选择用）：同步 base_url/api_key/model，emit activeModelChanged；
    /// 若子进程正在运行则热替换 LLM 配置（reconfigureAgent，不重启子进程、不丢会话状态）
    virtual void setActiveModel(const QString& provider, const QString& model) = 0;

    // ---- 会话管理（plan-03 新增，破坏性接口变更，插件需重编译；AGENTS.md plan-06 标注） ----
    // 创建新会话，返回新 sessionId
    virtual QString createSession() = 0;
    // 切换到指定会话（懒启动→下发历史→恢复）
    virtual bool switchSession(const QString& sessionId) = 0;
    // 删除指定会话
    virtual void deleteSession(const QString& sessionId) = 0;
    // 重命名指定会话
    virtual void renameSession(const QString& sessionId, const QString& title) = 0;
    // 列出所有会话（供 UI 下拉）
    virtual QVariantList listSessions() const = 0;
    // 获取当前活跃会话 ID
    virtual QString currentSessionId() const = 0;
    // 导出当前活跃会话字节（plan-05 工程保存调用）
    virtual QHash<QString, QByteArray> exportActiveSessions() const = 0;
    // 从工程 zip 加载会话文件（plan-05 工程加载调用）
    virtual void loadSessionsFromProject(const QHash<QString, QByteArray>& files, const QString& projectPath) = 0;
    // 设置当前工程路径（CRITICAL round-3：提升为接口纯虚）
    virtual void setCurrentProjectPath(const QString& path) = 0;

    // ---- 提示词库管理（agent 提示词库内置注入 / 执行 / CRUD 回调） ----
    /// 注册内置 agent：仅当 <daAgent>/<name>.md 不存在时写入（尊重用户已有编辑）
    virtual void registerBuiltinAgent(const QString& name, const QString& content) = 0;
    /// 按标题执行 agent：查找提示词→校验 LLM 配置→显示 dock 并发送消息；未配置则警告
    virtual bool runAgent(const QString& title) = 0;
    /// 获取提示词库操作回调（由 DAAgentManager 实现），供 DAGui 对话框执行 CRUD
    /// 返回的指针所有权归 DAAgentModule，调用方不销毁
    virtual DAAgentPromptOps* agentPromptOps() = 0;

    // ---- 权限层（permission-layer P1 新增，一次性 ABI 批处理；同"会话管理"节惯例，
    //      破坏性接口变更，插件需重编译；计划二不再新增纯虚/信号，不二次破坏 ABI） ----
    /// 获取权限配置（模式/审批超时/判官/规则/危险模式/分级覆盖，设置页读）
    virtual QJsonObject getPermissionConfig() const = 0;
    /// 写入权限配置（contains 守卫；运行中的子进程经 reconfigure 同步）
    virtual void setPermissionConfig(const QJsonObject& config) = 0;
    /// 获取当前权限模式（yolo/auto/manual）
    virtual QString getPermissionMode() const = 0;
    /// 设置权限模式（写 ini + emit permissionModeChanged + 运行中经 reconfigure 同步）
    virtual void setPermissionMode(const QString& mode) = 0;
    /// 用户对审批卡的裁决（callId 配对；rememberSession 仅 file_write 生效）
    virtual void sendToolApproval(const QString& callId, bool approved, bool rememberSession) = 0;
    /// 设置脚本工作区根目录（供 ${workspace} 变量解析 + 下发 Python；
    /// 复用 setCurrentProjectPath 的 L5 注入模式，由 DAAppProject 经 core() 注入）
    virtual void setScriptWorkspaceDir(const QString& dir) = 0;

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
    /// agent 子进程开始启动时发射（预启动/懒启动/崩溃重启均触发），UI 进入"启动中"过渡态
    void agentStarting();
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
    /// 系统消息（用户可见但不作为 LLM 对话内容），如 LLM 未配置提示
    /// level: "info" / "warning" / "error"
    void systemMessage(const QString& text, const QString& level);

    // ---- 供应商与多模型管理信号 ----
    /// 可用模型列表变化（供应商变更/设置页 apply 后），Dock 据此填充下拉
    /// payload 每元素 QVariantMap{provider,model}
    void availableModelsChanged(QVariantList models);
    /// 激活模型变化（Dock 选择 / 设置页 apply 触发），Dock 据此选中下拉项 + 刷新模型标签
    void activeModelChanged(const QString& provider, const QString& model);

    // ---- 权限层信号（permission-layer P1，契约 1 一次性批处理） ----
    /// 工具调用需要用户审批时发射（ask 决策）；args 含 _tier（分级）与
    /// _rememberable（是否渲染"本会话记住"，仅 file_write，A5）
    void agentToolApprovalRequest(const QString& callId, const QString& toolName, const QJsonObject& args);
    /// 审批卡作废（子进程退出/崩溃/切换会话清理），UI 据此撤卡
    void agentToolApprovalDismissed(const QString& callId);
    /// 权限模式变化（设置/热切换/启动推送），Dock 据此刷新模式选择器
    void permissionModeChanged(const QString& mode);
};
} // namespace DA
