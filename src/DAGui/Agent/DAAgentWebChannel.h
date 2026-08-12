#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QVector>
#include <QVariantMap>
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
    void appendUserMessage(const QString& text);
    void appendToken(const QString& token);
    void finalizeAgentMessage(const QString& fullText);
    void appendToolCall(const QString& toolName, const QJsonObject& args);
    void appendToolResult(const QString& toolName, const QJsonObject& result);
    void appendQuestion(const QString& text, const QStringList& options, bool multiSelect);
    void showRetryStatus(int attempt, int maxAttempts, int delayMs,
                         const QString& errorType, const QString& errorMessage);
    void appendError(const QString& message, const QString& errorType);
    void clearChat();
    void loadHistory(const QVector<QJsonObject>& records);
    void setBusy(bool busy);
    void setStopping();
    void setModel(const QString& label);
    void setTokenStats(const QString& label, int inputTokens, int outputTokens,
                       int totalTokens, int contextWindow, const QString& source);
    void resetTokenStats();
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

private:
    void callJS(const QString& funcCall);

    QWebEngineView* mView;
};
} // namespace DA
