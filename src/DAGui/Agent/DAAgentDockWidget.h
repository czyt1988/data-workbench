#pragma once
#include <QWidget>
#include <QJsonObject>
#include <QVector>
#include <QStringList>
#include <QVariantList>
#include "DAGuiAPI.h"
#include "DAGlobals.h"

namespace ads
{
class CDockManager;
class CDockWidget;
class CDockAreaWidget;
}  // namespace ads

namespace DA
{
class DAAgentSessionChatWidget;

/**
 * @brief Agent 助手停靠区宿主（session-tabs：每会话一个 dock 标签页），继承 QWidget
 *
 * 本项目使用 Qt-Advanced-Docking-System (ADS)，平台 DAAppDockingArea 的
 * createDockWidget(QWidget*, ...) 会把本 QWidget 包装进 ads::CDockWidget。
 * 若继承 QDockWidget 再交给 createDockWidget()，会出现双标题栏、float/拖拽冲突。
 *
 * 内部结构仿 DAChartOperateWidget 的嵌套停靠模式（chart-dock-nesting.md）：
 * QStackedWidget{ 占位页(无会话时) + 嵌套 ads::CDockManager }。每个会话一个
 * DAAgentSessionChatWidget（纯 QWidget）由 ads::CDockWidget 包装，objectName =
 * 会话 ID；另至多一个 unbound 视图（未绑定会话的"新会话"草稿区，懒创建：
 * 首条消息发出后才落盘建会话并绑定）。嵌套管理器内的 dock 只能在本区域内
 * 分屏/并栏/拖拽，无法逃逸（独立停靠宇宙），且全局禁止浮动为独立窗口。
 *
 * 标题栏（DAAgentDockAreaTitleBar，经组件工厂注入）在标签页右侧内置按钮组前
 * 提供「+ 新建会话」「会话管理」两个入口；标签右键菜单（DAAgentSessionDockWidgetTab）
 * 提供重命名/删除/停止。
 *
 * 会话级接口信号（首参 sessionId）在本类路由到对应视图，后台会话的视图实时
 * 渲染流式输出；全局信号（模型列表/激活模型/权限模式/系统消息）广播到全部视图。
 * 用户交互（发消息/回答）遵循"交互即激活"：宿主先确保该视图会话为模块当前会话
 * 再转发，保持 Module 下行调用（sendMessage/sendUserAnswer）的"当前会话"语义。
 *
 * @note 本类继承 QWidget，而非 QDockWidget
 */
class DAGUI_API DAAgentDockWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAgentDockWidget)
public:
    explicit DAAgentDockWidget(QWidget* parent = nullptr);
    ~DAAgentDockWidget();

    /// 「+」入口：已有 unbound 视图则 raise 它，否则创建 unbound 视图（懒创建，不落盘）。
    /// 标题栏「+」按钮与 Ribbon「新建会话」共用本入口
    void requestNewSession();
    /// 弹出会话管理对话框（标题栏按钮与 Ribbon「会话管理」共用本入口）
    void requestShowSessionManager();

    // ---- 标签右键菜单入口（DAAgentSessionDockWidgetTab 调用） ----
    /// 重命名会话：输入框确认后发射 sessionRenameRequested
    void requestRenameSession(const QString& sessionId);
    /// 删除会话：确认框后发射 sessionDeleteRequested
    void requestDeleteSession(const QString& sessionId);
    /// 停止会话生成：发射 stopSessionRequested
    void requestStopSession(const QString& sessionId);
    /// 会话是否处于启动/运行/等待输入态（右键菜单"停止会话"项的显隐与关闭弹窗判定）
    bool isSessionActive(const QString& sessionId) const;

public Q_SLOTS:
    // ---- 会话级信号槽（首参 sessionId，路由到对应视图；视图不存在则忽略） ----
    void onAgentToken(const QString& sessionId, const QString& token);
    void onAgentMessageComplete(const QString& sessionId, const QString& fullText);
    void onAgentToolCall(const QString& sessionId, const QString& toolName, const QJsonObject& args);
    /// 工具排队状态（决策点 2 ③）：position>0=排队中第 N 位，0=开始执行
    void onAgentToolQueued(const QString& sessionId, const QString& toolName, int position);
    void onAgentToolResult(const QString& sessionId, const QString& toolName, const QJsonObject& result);
    void onAgentQuestion(const QString& sessionId, const QString& text, const QStringList& options, bool multiSelect);
    /// 挂起问题卡作废（子进程退出/崩溃/用户 Stop/桥退役），通知 web 移除未回答问题卡
    void onQuestionDismissed(const QString& sessionId);
    void onAgentError(const QString& sessionId, const QString& message, const QString& errorType = QString(),
                      const QString& detail = QString());
    void onAgentRetrying(const QString& sessionId, int attempt, int maxAttempts, int delayMs,
                         const QString& errorType, const QString& errorMessage);
    void onAgentStarting(const QString& sessionId);
    void onAgentReady(const QString& sessionId, const QString& model);
    void onAgentBusy(const QString& sessionId, bool busy);
    void onAgentUsage(const QString& sessionId, int inputTokens, int outputTokens, int totalTokens,
                      int contextWindow, const QString& source);
    void onAgentSessionLoaded(const QString& sessionId);
    void onToolApprovalRequest(const QString& sessionId, const QString& callId, const QString& toolName,
                               const QJsonObject& args);
    void onToolApprovalDismissed(const QString& sessionId, const QString& callId);
    void onAgentSubagentProgress(const QString& sessionId, const QJsonObject& progress);

    // ---- 会话生命周期信号槽 ----
    /// 会话切换完成：无视图时创建视图并重放历史；有视图（标签切换快速路径）仅同步缓存
    void onSessionSwitched(const QString& sessionId, const QVector<QJsonObject>& allRecords);
    /// 会话列表变化：同步标签标题/状态徽标 + 对账移除已删除会话的视图
    void onSessionListChanged(QVariantList sessions);
    /// 新会话创建（"+"或首条消息懒建）：把 unbound 视图绑定到该会话
    void onSessionCreated(const QString& sessionId);
    /// 当前无活跃会话（启动/打开工程）：关闭全部绑定视图，保留一个 unbound 视图
    void onSessionCleared();
    /// 跨工程存活会话变化（决策点 5 方案 c，审计问题 18）：缓存列表 +
    /// 各视图聊天区顶部提示条（"N 个上一工程的会话仍在后台运行"，点击打开
    /// 会话管理对话框的"全部工程"视图）；空列表隐藏提示条
    void onForeignAgentSessionsRunning(QVariantList sessions);

    // ---- 全局信号槽（广播到全部视图） ----
    void onSystemMessage(const QString& text, const QString& level = QStringLiteral("info"));
    void onAvailableModelsChanged(QVariantList models);
    void onActiveModelChanged(const QString& provider, const QString& model);
    void onPermissionModeChanged(const QString& mode);
    void onPermissionModeExplicitChanged(bool explicitSet);

Q_SIGNALS:
    // ---- 用户操作 → 接口方法（DAAppController 桥接） ----
    /// 用户发送消息（宿主已确保来源视图会话为模块当前会话）
    void sendMessageRequested(const QString& text);
    /// 用户请求终止（unbound 视图，理论不触发；保留契约）
    void stopRequested();
    /// 用户选择答案（宿主已确保来源视图会话为模块当前会话）
    void userAnswerSelected(const QString& answer);
    /// 用户点击绘图引用超链接（da-figure: 协议），转发给 DAAppController 处理
    void figureLinkRequested(const QString& href);
    /// 用户激活某会话标签（≠ 模块当前会话时发射）
    void sessionSwitchRequested(const QString& sessionId);
    /// 用户删除会话（标签右键菜单，带确认后发射）
    void sessionDeleteRequested(const QString& sessionId);
    /// 用户重命名会话（标签右键菜单/管理对话框）
    void sessionRenameRequested(const QString& sessionId, const QString& newTitle);
    /// 用户在 web 两级选择器选定供应商+模型，请求设置激活（→ DAAgentInterface::setActiveModel）
    void activeModelChangeRequested(const QString& provider, const QString& model);
    /// 用户在 web 模式选择器选定模式
    void permissionModeChangeRequested(const QString& mode);
    /// 用户对审批卡的裁决（callId 路由，无需激活来源会话）
    void toolApprovalDecision(const QString& callId, bool approved, bool rememberSession);
    /// 启动 yolo 确认卡（A13）响应
    void startupModeConfirmResponse(bool keepYolo);

    // ---- 会话视图生命周期 → 接口（session-tabs 新增） ----
    /// 会话视图 attach/detach 通知（→ DAAgentInterface::setSessionViewAttached：
    /// attach 时模块重发挂起问题卡/审批卡，detach 影响桥退役判定）
    void sessionViewAttachedChanged(const QString& sessionId, bool attached);
    /// unbound 视图激活（→ DAAgentInterface::clearCurrentSession：清当前会话指针）
    void currentSessionClearedRequested();
    /// 停止指定会话（关闭运行中标签"停止会话并关闭"路径 / 标签右键"停止会话"）
    void stopSessionRequested(const QString& sessionId);

private:
    void setupUI();
    void setupViewConnections(DAAgentSessionChatWidget* view);
    /// 创建会话视图 dock（objectName=sessionId），加载历史并 raise
    void createSessionView(const QString& sessionId, const QVector<QJsonObject>& records);
    /// 创建 unbound 视图 dock（懒创建草稿区）
    void createUnboundView();
    /// 移除视图 dock（notifyDetach=true 时发射 sessionViewAttachedChanged）
    void removeViewDock(ads::CDockWidget* dock, bool notifyDetach);
    /// 当前会话视图变化处理（聚焦路由）：绑定会话→切换请求；unbound→清当前
    void handleCurrentViewChanged(ads::CDockWidget* dock);

private Q_SLOTS:
    void onFocusedDockChanged(ads::CDockWidget* oldDock, ads::CDockWidget* nowDock);
    void onDockCloseRequested(ads::CDockWidget* dock);
    /// A13 确认卡已在某视图弹出：置全局 shown 标志，其余视图不再弹
    void onViewStartupYoloConfirmShown();
    /// 占位页「新建会话」按钮
    void onPlaceholderNewSessionClicked();
};
}  // namespace DA
