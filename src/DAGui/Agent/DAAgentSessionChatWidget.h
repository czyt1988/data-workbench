#pragma once
#include <QWidget>
#include <QJsonObject>
#include <QVector>
#include <QStringList>
#include <QVariantList>
#include <functional>
#include "DAGuiAPI.h"
#include "DAGlobals.h"

class QMenu;
class QContextMenuEvent;

namespace DA
{
class DAAgentWebChannel;
class DAAgentChatWebPage;

/**
 * @brief 单个 Agent 会话的聊天视图（session-tabs）
 *
 * 每个 view 持有独立的 QWebEngineView（qrc:///DAAgent/chat.html）与
 * DAAgentWebChannel，承载一个会话（或未绑定会话的"新会话"草稿区）的全部
 * 聊天渲染。会话级槽以 sessionId 首参匹配本会话才渲染；全局状态（模型列表/
 * 激活模型/权限模式）由宿主 DAAgentDockWidget 广播注入。
 *
 * 本类为纯 QWidget（项目 ADS 约定：由 ads::CDockWidget 包装，不继承停靠类）。
 * sessionId 为空表示未绑定会话（unbound）——首条消息发出后由宿主绑定并更新
 * 所属 dock 的 objectName/标题。
 */
class DAGUI_API DAAgentSessionChatWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAgentSessionChatWidget)
public:
    explicit DAAgentSessionChatWidget(QWidget* parent = nullptr);
    ~DAAgentSessionChatWidget();

    /// 本视图绑定的会话 ID（空 = 未绑定）
    QString sessionId() const;
    /// 绑定会话（unbound → bound）。仅记录归属，不清空聊天内容
    /// （绑定发生在首条消息渲染之后，清空会擦掉刚显示的用户气泡）
    void setSessionId(const QString& sessionId);
    /// 是否已绑定会话
    bool isBound() const;

    /// 重放会话历史（web 未就绪时缓存，onWebReady 后 flush）
    void loadHistory(const QVector<QJsonObject>& records);
    /// 清空聊天区（web 未就绪时缓存，onWebReady 后 flush）
    void clearChat();

    // ---- 全局状态注入（宿主广播；同时缓存供 onWebReady flush） ----
    /// 可用模型列表变化（flat {provider,model,...}）
    void setAvailableModels(const QVariantList& models);
    /// 激活供应商+模型变化
    void setActiveModel(const QString& provider, const QString& model);
    /// 当前权限模式变化（yolo/auto/manual）
    void setPermissionMode(const QString& mode);
    /// A13 启动 yolo 确认卡待弹标志（宿主保证每次启动仅首个就绪视图弹出）
    void setStartupYoloConfirmPending(bool pending);
    /// 跨工程会话提示条（决策点 5 方案 c，审计问题 18）：count>0 显示
    /// "N 个上一工程的会话仍在后台运行"，0 隐藏（web 未就绪时缓存）
    void setForeignSessionsBanner(int count);

public Q_SLOTS:
    // ---- 会话级事件（首参 sessionId，仅本会话渲染） ----
    void onAgentToken(const QString& sessionId, const QString& token);
    void onAgentMessageComplete(const QString& sessionId, const QString& fullText);
    void onAgentToolCall(const QString& sessionId, const QString& toolName, const QJsonObject& args);
    /// 工具排队状态（决策点 2 ③）：position>0=排队中第 N 位，0=开始执行
    void onAgentToolQueued(const QString& sessionId, const QString& toolName, int position);
    void onAgentToolResult(const QString& sessionId, const QString& toolName, const QJsonObject& result);
    void onAgentQuestion(const QString& sessionId, const QString& text, const QStringList& options, bool multiSelect);
    /// 挂起问题卡作废（子进程退出/崩溃/用户 Stop/桥退役），通知 web 移除问题卡
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

    // ---- 全局事件（无会话归属，直接渲染） ----
    void onSystemMessage(const QString& text, const QString& level = QStringLiteral("info"));

Q_SIGNALS:
    // ---- 用户操作（宿主转发到接口层） ----
    /// 用户发送消息（宿主负责"交互即激活"：先确保该视图会话为当前会话再转发）
    void sendMessageRequested(const QString& text);
    /// 用户点击 Stop（宿主按视图会话归属路由 stopSession/stop）
    void stopRequested();
    /// 用户选择问题答案
    void userAnswerSelected(const QString& answer);
    /// 用户点击绘图引用超链接（da-figure: 协议）
    void figureLinkRequested(const QString& href);
    /// 用户在 web 两级模型选择器选定供应商+模型
    void activeModelChangeRequested(const QString& provider, const QString& model);
    /// 用户在 web 权限模式选择器选定模式
    void permissionModeChangeRequested(const QString& mode);
    /// 用户对审批卡的裁决
    void toolApprovalDecision(const QString& callId, bool approved, bool rememberSession);
    /// 启动 yolo 确认卡（A13）响应
    void startupModeConfirmResponse(bool keepYolo);
    /// A13 确认卡已弹出（宿主收到后置全局 shown 标志并清除其他视图的待弹标志）
    void startupYoloConfirmShown();
    /// 跨工程会话提示条点击（宿主打开会话管理对话框的"全部工程"视图）
    void foreignBannerClicked();

protected:
    // webview 设为 NoContextMenu 后，右键事件传播到本容器，弹出最小编辑菜单
    //（Copy/Paste/Select All；WebEngine 默认菜单的 Back/Forward/Reload 已去除）
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void setupUI();
    void setupWebChannel();
    /// 构建右键菜单（仅 Copy/Paste/Select All，webview 默认菜单已由
    /// NoContextMenu 策略关闭，见 setupUI）
    void buildContextMenu();
    /// web 就绪前的事件缓存执行：就绪立即执行渲染，未就绪入队（onWebReady 后按序 flush）。
    /// 覆盖内容型事件（流式 token/工具卡/问题卡/审批卡等）——视图创建于会话运行中时，
    /// setSessionViewAttached 重发的挂起卡与创建后的流式增量在 chat.html 加载完成前
    /// 经 callJS 下发会丢失，必须缓存到就绪后补发
    void runOrDefer(std::function<void()> fn);
    QString formatTokenLabel(int totalTokens, int contextWindow, const QString& source) const;
    QString mapErrorMessage(const QString& original, const QString& errorType) const;

private Q_SLOTS:
    void onUserMessageReceived(const QString& text);
    void onWebReady();
    void onStopClicked();
    void onUserAnswer(const QString& answer);
    void onFigureLink(const QString& href);
    void onModelSelect(const QString& provider, const QString& model);
    void onPermissionModeSelect(const QString& mode);
    void onToolApprovalDecision(const QString& callId, bool approved, bool rememberSession);
    void onStartupModeConfirmResponse(bool keepYolo);
};
}  // namespace DA
