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
#include <QList>
#include "DAAbstractAgentTool.h"
#include "DAAgentProvider.h"
#include "DAAgentConfig.h"
#include "DAAgentPermissionConfig.h"

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
    // 注册时记录工具的 QObject parent 作为 provider（DAAbstractAgentTool 非 QObject，
    // 实现类如 DAAgentToolBase 多继承 QObject），供插件热卸载时按 provider 注销
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

    // ---- LLM 配置与供应商管理（配置结构体化，破坏性接口变更，插件需重编译；
    //      为"子 agent 管理"之后的再一次 ABI 批处理：原 QJsonObject/QJsonArray/
    //      QVariantList 载荷改为强类型结构体，JSON 仅保留在协议边界） ----
    // 获取 LLM 配置（稀疏结构体：engaged 字段=显式设置过，getter 兜底默认值）
    virtual DAAgentLLMConfig getLLMConfig() const = 0;
    // 设置 LLM 配置（仅 engaged 字段生效，等价原 contains 守卫语义）
    virtual void setLLMConfig(const DAAgentLLMConfig& config) = 0;

    // ---- 供应商与多模型管理（多供应商多模型） ----
    // 每个供应商含 name / base_url / api_key / models。
    // 激活供应商 + 激活模型决定实际下发给子进程的 base_url/api_key/model。
    /// 获取所有供应商配置（设置页 CRUD 用；api_key 为内存态明文）
    virtual QList< DAAgentProvider > getProviders() const = 0;
    /// 保存所有供应商配置（设置页 apply 用；api_key 明文传入，持久化时内部加密）
    virtual void setProviders(const QList< DAAgentProvider >& providers) = 0;
    /// 获取当前激活供应商名称
    virtual QString getActiveProvider() const = 0;
    /// 获取所有可选模型列表（Dock 下拉用，不含 api_key），每元素 DAAgentModelRef
    virtual QList< DAAgentModelRef > getAvailableModels() const = 0;
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
    //      破坏性接口变更，插件需重编译；权限层计划二未再破坏 ABI；
    //      其后的第二次破坏性变更见下方"子 agent 管理"节，顺序经
    //      permission-layer.md §13 衔接契约确认；
    //      配置结构体化时 get/setPermissionConfig 载荷改为 DAAgentPermissionConfig） ----
    /// 获取权限配置（模式/审批超时/判官/规则/危险模式/分级覆盖，设置页读）
    virtual DAAgentPermissionConfig getPermissionConfig() const = 0;
    /// 写入权限配置（标量稀疏守卫：未 engage 的标量不受影响；运行中的子进程经 reconfigure 同步）
    virtual void setPermissionConfig(const DAAgentPermissionConfig& config) = 0;
    /// 获取当前权限模式（yolo/auto/manual，未配置默认 yolo 全自动）
    virtual QString getPermissionMode() const = 0;
    /// 设置权限模式（写 ini + emit permissionModeChanged + 运行中经 reconfigure 同步）
    virtual void setPermissionMode(const QString& mode) = 0;
    /// 用户对审批卡的裁决（callId 配对；rememberSession 仅 file_write 生效）
    virtual void sendToolApproval(const QString& callId, bool approved, bool rememberSession) = 0;
    /// 设置脚本工作区根目录（供 ${workspace} 变量解析 + 下发 Python；
    /// 复用 setCurrentProjectPath 的 L5 注入模式，由 DAAppProject 经 core() 注入）
    virtual void setScriptWorkspaceDir(const QString& dir) = 0;

    // ---- 子 agent 管理（subagent-phase1 新增，破坏性接口变更，插件需重编译；
    //      同"会话管理"节惯例，为权限层之后的第二次破坏性变更） ----
    /// 注册内置子 agent 定义：仅当 <daAgent>/subagents/<name>.md 不存在时写入（尊重用户编辑）
    virtual void registerBuiltinSubagent(const QString& name, const QString& content) = 0;
    /// 获取所有子 agent 定义（管理 UI 数据源；每元素 {name, description, tools, system_prompt, permissions?}）
    virtual QJsonArray subagentDefinitions() const = 0;
    /// 保存子 agent 定义（def 含 name/description/tools/system_prompt/permissions?；
    /// oldName 非空且与 name 不同表示重命名）；成功后运行中的子进程经 update_subagents 热更新
    virtual bool saveSubagent(const QJsonObject& def, const QString& oldName = QString()) = 0;
    /// 删除指定名称的子 agent 定义；成功后运行中的子进程经 update_subagents 热更新
    virtual bool deleteSubagent(const QString& name) = 0;
    /// 已注册工具名列表（子 agent 编辑器工具白名单复选框数据源）
    virtual QStringList registeredToolNames() const = 0;

    // ---- 插件热插拔（plugin-hotswap 新增；⚠️ 为保持对外部已编译插件的 vtable 兼容，
    //      本节虚函数必须追加在既有虚函数列表末尾，禁止在其后插入其他虚函数，
    //      也禁止调整上方既有虚函数的声明顺序） ----
    /// 注册命名系统提示词片段（带 provider 的重载，provider 为注册方插件对象，
    /// 热卸载时经 unregisterSystemPromptsByProvider 按 provider 注销）
    virtual void registerSystemPrompt(const QString& name, const QString& content, QObject* provider) = 0;
    /// 注销指定 provider（插件对象）注册的全部工具，插件卸载前由 APP 层调用，
    /// 防止插件实例销毁后工具注册表留下悬空指针；返回注销的工具数量。
    /// 工具对象所有权仍归插件（parent 关系），本方法只移除宿主注册表指针，不 delete 工具
    virtual int unregisterToolsByProvider(QObject* provider) = 0;
    /// 注销指定 provider（插件对象）注册的全部系统提示词片段，返回注销数量。
    /// 注意：已启动的 agent 子进程持有旧系统提示词，注销后对新会话/重启的子进程生效
    virtual int unregisterSystemPromptsByProvider(QObject* provider) = 0;

    // ---- 会话视图管理（session-tabs 新增，破坏性接口变更，插件需重编译；
    //      ⚠️ 追加在既有虚函数列表（含插件热插拔节）末尾，不改变任何既有虚函数的
    //      vtable 槽位，已编译插件经接口指针调用既有方法不受影响） ----
    /// 会话视图 attach/detach 通知：宿主（Agent 会话标签页）视图创建/销毁时调用，
    /// 模块记录「有视图的会话」集合；attach 时向该视图重发挂起的 ask_user 问题卡
    /// 与工具审批卡（缓存来自后台运行或视图关闭期间）
    virtual void setSessionViewAttached(const QString& sessionId, bool attached) = 0;
    /// 清除当前会话指针（unbound 视图激活时调用）：空闲会话桥优雅退役，
    /// 忙碌桥留后台继续；不 emit 任何信号（区别于 switchSession）
    virtual void clearCurrentSession() = 0;
    /// 停止指定会话正在进行的生成（关闭运行中会话视图时"停止会话并关闭"路径；
    /// 无参 stop() 仅停止当前会话）
    virtual void stopSession(const QString& sessionId) = 0;

Q_SIGNALS:
    // ---- 以下会话级事件由 DAAgentModule 从 DAAgentBridge 转发（session-tabs：
    //      信号首参为 sessionId，对所有运行中会话发射，宿主按会话路由到对应视图） ----
    /// agent 生成 token 时发射（流式渲染）
    void agentToken(const QString& sessionId, const QString& token);
    /// agent 消息生成完成时发射
    void agentMessageComplete(const QString& sessionId, const QString& fullText);
    /// agent 发起工具调用时发射
    void agentToolCall(const QString& sessionId, const QString& toolName, const QJsonObject& args);
    /// 工具执行结果返回时发射
    void agentToolResult(const QString& sessionId, const QString& toolName, const QJsonObject& result);
    /// agent 向用户提问时发射
    void agentQuestion(const QString& sessionId, const QString& text, const QStringList& options, bool multiSelect);
    /// agent 发生错误时发射
    void agentError(const QString& sessionId, const QString& message, const QString& errorType = QString(),
                    const QString& detail = QString());
    /// agent 正在重试 LLM 调用时发射（Python 端指数退避期间每次重试发一次）
    void agentRetrying(const QString& sessionId, int attempt, int maxAttempts, int delayMs,
                       const QString& errorType, const QString& errorMessage);
    /// agent 就绪时发射
    void agentReady(const QString& sessionId, const QString& model);
    /// agent 子进程开始启动时发射（预启动/懒启动/崩溃重启均触发），UI 进入"启动中"过渡态
    void agentStarting(const QString& sessionId);
    /// agent 忙碌状态变化时发射
    void agentBusy(const QString& sessionId, bool busy);
    /// agent 本轮处理完成时发射（生命周期事件，目前无 Dock 槽对接，纳入接口备扩展）
    void agentDone();
    /// 回合疑似未完成时发射（模型"话说一半就停"，UI 提示用户可继续）
    void agentTurnPossiblyIncomplete(int toolRounds);
    /// agent 完成会话历史重建时发射
    void agentSessionLoaded(const QString& sessionId);
    // ---- 以下 5 个由 DAAgentModule 自身 emit（从 Module 的 Q_SIGNALS 上移） ----
    /// token 使用量更新（agentUsage lambda 内补 context_window 后 emit；switchSession 也会从持久化 usage 记录 emit）
    void tokenUsageUpdated(const QString& sessionId, int inputTokens, int outputTokens, int totalTokens,
                           int contextWindow, const QString& source);
    /// 切换会话完成时发射，供 UI 重放历史
    void sessionSwitched(const QString& sessionId, const QVector<QJsonObject>& allRecords);
    /// 新会话创建时发射（仅 newSession 路径，触发 UI clearChat）
    void sessionCreated(const QString& sessionId);
    /// 会话列表变化时发射，带 payload
    /// （每元素 QVariantMap{id,title,createdAt,updatedAt,messageCount,state,inputTokens,outputTokens,totalTokens}；
    /// token 为会话累计值，-1=旧数据未统计）
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
    /// payload 每元素 DAAgentModelRef{provider,model,contextWindow,maxOutputTokens}
    void availableModelsChanged(const QList< DAAgentModelRef >& models);
    /// 激活模型变化（Dock 选择 / 设置页 apply 触发），Dock 据此选中下拉项 + 刷新模型标签
    void activeModelChanged(const QString& provider, const QString& model);

    // ---- 权限层信号（permission-layer P1，契约 1 一次性批处理；session-tabs：
    //      审批信号加 sessionId 首参，按会话广播） ----
    /// 工具调用需要用户审批时发射（ask 决策）；args 含 _tier（分级）与
    /// _rememberable（是否渲染"本会话记住"，仅 file_write，A5）
    void agentToolApprovalRequest(const QString& sessionId, const QString& callId, const QString& toolName,
                                  const QJsonObject& args);
    /// 审批卡作废（子进程退出/崩溃/切换会话清理），UI 据此撤卡
    void agentToolApprovalDismissed(const QString& sessionId, const QString& callId);
    /// 权限模式变化（设置/热切换/启动推送），Dock 据此刷新模式选择器
    void permissionModeChanged(const QString& mode);
    /// 权限模式"显式设置"状态（启动推送）：true=用户曾显式写入模式（ini 有键），
    /// false=当前模式为默认值；Dock 据此决定 A13 启动 yolo 确认卡是否弹出
    /// （默认全自动静默生效，仅显式 yolo 跨重启时二次确认）
    void permissionModeExplicitChanged(bool explicitSet);

    // ---- 子 agent 信号（subagent-phase1） ----
    /// 子 agent 任务进度（subagent_progress 协议消息原样转发；载荷含
    /// call_id/task_id?/subagent?/state/message?/results?，见母文档 §7）
    void agentSubagentProgress(const QString& sessionId, const QJsonObject& progress);
    /// 子 agent 定义列表变化（加载/保存/删除/插件注入后），供管理 UI 刷新
    void subagentListChanged();
    /// 提示词库列表变化（保存/删除/插件注入内置 agent 后），供 Ribbon gallery 刷新
    void agentListChanged();
};
} // namespace DA
