#pragma once
#include <QWidget>
#include <QJsonObject>
#include <QVector>
#include <QStringList>
#include <QVariantList>
#include "DAGuiAPI.h"
#include "DAGlobals.h"

namespace DA
{
class DAAgentWebChannel;

/**
 * @brief Agent 对话窗口部件，继承 QWidget（非 QDockWidget，平台使用 ADS 管理停靠）
 *
 * 本项目使用 Qt-Advanced-Docking-System (ADS)，平台 DAAppDockingArea 的
 * createDockWidget(QWidget*, ...) 会把本 QWidget 包装进 ads::CDockWidget。
 * 若继承 QDockWidget 再交给 createDockWidget()，会出现双标题栏、float/拖拽冲突。
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
    DAAgentWebChannel* webChannel() const;

private Q_SLOTS:
    void onStopClicked();
    void onUserAnswer(const QString& answer);
    void onFigureLink(const QString& href);
    void onNewSessionClicked();
    void onSessionManagerClicked();
    void onUserMessageReceived(const QString& text);
    void onWebReady();
    /// web 两级模型选择器选定供应商+模型（用户手动切换）：定稿当前流式 + emit activeModelChangeRequested
    void onModelSelect(const QString& provider, const QString& model);
    // ---- 权限层（permission-layer P1） ----
    /// web 权限模式选择器选定模式：透传 permissionModeChangeRequested
    void onPermissionModeSelect(const QString& mode);
    /// web 审批卡裁决：透传 toolApprovalDecision
    void onToolApprovalDecision(const QString& callId, bool approved, bool rememberSession);
    /// web 启动 yolo 确认卡响应（A13）：透传 startupModeConfirmResponse
    void onStartupModeConfirmResponse(bool keepYolo);

public Q_SLOTS:
    void onAgentToken(const QString& token);
    void onAgentMessageComplete(const QString& fullText);
    void onAgentToolCall(const QString& toolName, const QJsonObject& args);
    /// 工具排队状态（决策点 2 ③）：position>0=排队中第 N 位，0=开始执行
    void onAgentToolQueued(const QString& toolName, int position);
    void onAgentToolResult(const QString& toolName, const QJsonObject& result);
    void onAgentQuestion(const QString& text, const QStringList& options, bool multiSelect);
    /// 挂起问题卡作废（子进程退出/崩溃/用户 Stop/桥退役），通知 web 移除未回答问题卡
    void onQuestionDismissed();
    void onAgentError(const QString& message, const QString& errorType = QString(), const QString& detail = QString());
    void onAgentRetrying(int attempt, int maxAttempts, int delayMs, const QString& errorType, const QString& errorMessage);
    void onAgentStarting();
    void onAgentReady(const QString& model);
    void onAgentBusy(bool busy);
    void onAgentUsage(int inputTokens, int outputTokens, int totalTokens, int contextWindow, const QString& source);
    void onAgentSessionLoaded(const QString& sessionId);
    void onSessionSwitched(const QString& sessionId, const QVector<QJsonObject>& allRecords);
    void onSessionListChanged(QVariantList sessions);
    void onSessionCreated(const QString& sessionId);
    void onSessionCleared();
    void onSystemMessage(const QString& text, const QString& level = QStringLiteral("info"));

    // ---- 供应商/多模型选择（web 两级选择器） ----
    /// 可用模型列表变化（供应商变更/设置页 apply），推送列表到 web 选择器
    void onAvailableModelsChanged(QVariantList models);
    /// 激活模型变化（web 选择/设置页 apply），推送激活供应商+模型到 web
    void onActiveModelChanged(const QString& provider, const QString& model);

    // ---- 权限层（permission-layer P1） ----
    /// 权限模式变化（启动推送/热切换），推送到 web 模式选择器；显式 yolo 启动时弹确认卡（A13）
    void onPermissionModeChanged(const QString& mode);
    /// 权限模式"显式设置"状态（启动推送）：缓存供 onWebReady 判定 A13 确认卡
    /// （默认全自动不弹卡，仅用户曾显式写入的 yolo 跨重启时二次确认）
    void onPermissionModeExplicitChanged(bool explicitSet);
    /// 工具调用需审批（ask 决策），推送审批卡到 web（args 含 _tier/_rememberable/_subagent）
    void onToolApprovalRequest(const QString& callId, const QString& toolName, const QJsonObject& args);
    /// 审批作废（子进程退出/崩溃/切换会话），通知 web 撤卡
    void onToolApprovalDismissed(const QString& callId);

    // ---- 子 agent（subagent-phase1 C） ----
    /// 子 agent 任务进度（subagent_progress 协议消息原文），推送到 web 渲染进度卡片
    void onAgentSubagentProgress(const QJsonObject& progress);

Q_SIGNALS:
    /**
     * @brief 用户请求发送消息信号
     * @param text 消息文本
     */
    void sendMessageRequested(const QString& text);

    /**
     * @brief 用户请求终止 agent 信号
     */
    void stopRequested();

    /**
     * @brief 用户选择答案信号
     * @param answer 用户选择的答案
     */
    void userAnswerSelected(const QString& answer);

    /// 用户点击绘图引用超链接（da-figure: 协议），转发给 DAAppController 处理
    void figureLinkRequested(const QString& href);

    // ---- plan-04 会话操作信号（→ Module） ----
    /// 用户在会话下拉切换会话
    void sessionSwitchRequested(const QString& sessionId);
    /// 用户点击 "+" 新建会话
    void sessionCreateRequested();
    /// 用户点击 "×" 删除会话
    void sessionDeleteRequested(const QString& sessionId);
    /// 用户点击 Rename 重命名会话
    void sessionRenameRequested(const QString& sessionId, const QString& newTitle);
    /// 会话管理对话框右键"停止"（审计 L14）：停止指定会话的后台运行
    ///（→ DAAgentInterface::stopSession，不必先切换再 Stop）
    void sessionStopRequested(const QString& sessionId);
    /// 会话切换开始时请求停止当前 agent 流式输出（MAJOR4 切换时请求停止）
    void agentStopRequested();

    // ---- 供应商/多模型选择 ----
    /// 用户在 web 两级选择器选定供应商+模型，请求设置激活（→ DAAgentInterface::setActiveModel）
    void activeModelChangeRequested(const QString& provider, const QString& model);

    // ---- 权限层（permission-layer P1） ----
    /// 用户在 web 模式选择器选定模式，请求设置（→ DAAgentInterface::setPermissionMode）
    void permissionModeChangeRequested(const QString& mode);
    /// 用户对审批卡的裁决（→ DAAgentInterface::sendToolApproval）
    void toolApprovalDecision(const QString& callId, bool approved, bool rememberSession);
    /// 启动 yolo 确认卡（A13）响应：false=用户拒绝保持，请求降级 auto（→ setPermissionMode("auto")）
    void startupModeConfirmResponse(bool keepYolo);

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    void setupUI();
    void setupWebChannel();
    void updateTitleLabel();
    QString formatTokenLabel(int totalTokens, int contextWindow, const QString& source) const;
    QString mapErrorMessage(const QString& original, const QString& errorType) const;
};
} // namespace DA
