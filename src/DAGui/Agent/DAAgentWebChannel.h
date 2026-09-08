#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>
#include "DAGuiAPI.h"

class QWebEngineView;

namespace DA
{

/**
 * @brief WebChannel 桥接对象，由 DAAgentDockWidget 通过 QWebChannel::registerObject 暴露给 JS
 *
 * 本类是 QObject（不是 QWebChannel）。QWebChannel 实例本身由 setupWebChannel() 中
 * `new QWebChannel(this)` 单独创建，并通过 registerObject("chatBridge", m_channel) 注册本对象。
 * 继承 QObject 即可拥有 Q_OBJECT/槽，供 JS 调用。
 */
class DAGUI_API DAAgentWebChannel : public QObject
{
    Q_OBJECT
public:
    explicit DAAgentWebChannel(QWebEngineView* view, QObject* parent = nullptr);
    Q_INVOKABLE void onUserSelect(const QString& answer);
    Q_INVOKABLE void onUserMessage(const QString& text);
    Q_INVOKABLE void onFigureLink(const QString& href);
    Q_INVOKABLE void onReady();
    Q_INVOKABLE void onStopRequested();
    /// JS 调用：用户在 web 两级模型选择器选定供应商+模型
    Q_INVOKABLE void onModelSelect(const QString& provider, const QString& model);
    /// JS 调用：用户在权限模式选择器选定模式（切 yolo 前 JS 已做二次确认）
    Q_INVOKABLE void onPermissionModeSelect(const QString& mode);
    /// JS 调用：用户对审批卡的裁决（callId/approved/rememberSession）
    Q_INVOKABLE void onToolApproval(const QString& callId, bool approved, bool rememberSession);
    /// JS 调用：启动 yolo 确认卡（A13）的用户响应（true=保持 yolo）
    Q_INVOKABLE void onModeConfirmResponse(bool keepYolo);
    /// JS 调用：跨工程会话提示条点击（决策点 5 方案 c，审计问题 18）
    Q_INVOKABLE void onForeignBannerClicked();
    void appendUserMessage(const QString& text);
    void appendToken(const QString& token);
    void finalizeAgentMessage(const QString& fullText);
    void appendToolCall(const QString& toolName, const QJsonObject& args);
    /// 推送工具排队状态（决策点 2 ③）：position>0=排队中第 N 位，0=开始执行
    void markToolQueued(const QString& toolName, int position);
    void appendToolResult(const QString& toolName, const QJsonObject& result);
    void appendQuestion(const QString& text, const QStringList& options, bool multiSelect);
    /// 推送挂起问题卡作废（JS 移除未回答问题卡，镜像 dismissToolApproval，审计问题 17）
    void dismissQuestion();
    void showRetryStatus(int attempt, int maxAttempts, int delayMs,
                         const QString& errorType, const QString& errorMessage);
    void appendError(const QString& message, const QString& errorType, const QString& detail = QString());
    void appendSystemMessage(const QString& text, const QString& level);
    void clearChat();
    void loadHistory(const QVector<QJsonObject>& records);
    void setBusy(bool busy);
    void setStopping();
    void setStarting();
    /// 推送可用模型列表（flat 数组 {provider,model,...}，JS 据此分组渲染两级选择器）
    void setAvailableModels(const QVariantList& models);
    /// 推送激活供应商+模型（JS 更新触发按钮文案 + 选中高亮）
    void setActiveModel(const QString& provider, const QString& model);
    /// 推送当前权限模式（JS 更新模式选择器触发按钮 + 高亮）
    void setPermissionMode(const QString& mode);
    /// 推送工具审批请求（JS 渲染审批卡；payload 含 tool/args/tier/rememberable）
    void appendToolApproval(const QString& callId, const QJsonObject& payload);
    /// 推送审批作废（JS 撤卡）
    void dismissToolApproval(const QString& callId);
    /// 推送启动 yolo 确认卡（A13：启动读到 yolo 时弹一次确认，拒绝则降级 auto）
    void appendStartupYoloConfirm(const QString& text, const QString& okLabel, const QString& cancelLabel);
    /// 推送子 agent 任务进度（subagent_progress 协议消息原文，JS 渲染进度卡片）
    void updateSubagentProgress(const QJsonObject& payload);
    void setTokenStats(const QString& label, int inputTokens, int outputTokens,
                       int totalTokens, int contextWindow, const QString& source);
    void resetTokenStats();
    /// 推送跨工程存活会话提示条（决策点 5 方案 c）：count>0 显示"N 个上一工程
    /// 的会话仍在后台运行"（点击打开会话管理对话框），0 隐藏
    void showForeignSessionsBanner(int count);
    void setI18nLabels(const QVariantMap& labels);
    void focusInput();
    void onAgentStopped();

Q_SIGNALS:
    /**
     * @brief 用户选择答案信号
     * @param answer 用户选择的答案
     */
    void userAnswerSelected(const QString& answer);

    /**
     * @brief 用户发送消息信号
     * @param text 用户输入的消息文本
     */
    void userMessageSent(const QString& text);

    /**
     * @brief 用户点击绘图引用超链接信号
     * @param href 超链接 href，形如 da-figure:&lt;figure_name&gt; 或 da-figure:id=&lt;uuid&gt;
     */
    void figureLinkRequested(const QString& href);

    /**
     * @brief web 侧就绪信号（chat.js init() 握手，C++ 收到后 flush 当前态）
     */
    void webReady();

    /**
     * @brief 用户在 web 输入区点 Stop 按钮信号（直达 C++ 终止流程）
     */
    void stopRequested();

    /**
     * @brief 跨工程会话提示条点击信号（决策点 5 方案 c，审计问题 18）
     *
     * Dock 收到后打开会话管理对话框（含"全部工程"视图与一键停止）
     */
    void foreignBannerClicked();

    /**
     * @brief 用户在 web 两级模型选择器选定供应商+模型信号
     * @param provider 供应商名称
     * @param model 模型 id
     */
    void modelChangeRequested(const QString& provider, const QString& model);

    // ---- 权限层（permission-layer P1） ----
    /**
     * @brief 用户在权限模式选择器选定模式信号（切 yolo 前 JS 已二次确认）
     * @param mode yolo / auto / manual
     */
    void permissionModeChangeRequested(const QString& mode);

    /**
     * @brief 用户对审批卡的裁决信号
     * @param callId 工具调用 ID
     * @param approved 是否批准
     * @param rememberSession 是否本会话记住（仅 file_write 生效）
     */
    void toolApprovalDecision(const QString& callId, bool approved, bool rememberSession);

    /**
     * @brief 启动 yolo 确认卡（A13）用户响应信号
     * @param keepYolo true=保持 yolo，false=降级 auto
     */
    void startupModeConfirmResponse(bool keepYolo);

private:
    void callJS(const QString& funcCall);

    QWebEngineView* mView;
};
} // namespace DA
