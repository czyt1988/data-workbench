// DAAgentWebChannel.h —— QObject 桥对象，由 DAAgentDockWidget 通过 QWebChannel::registerObject 暴露给 JS
// 注意：本类是 QObject（不是 QWebChannel）。QWebChannel 实例本身由 setupWebChannel() 中
// `new QWebChannel(this)` 单独创建，并通过 registerObject("chatBridge", m_channel) 注册本对象
// （见 plan-03 §2.3 setupWebChannel）。继承 QObject 即可拥有 Q_OBJECT/槽，供 JS 调用。
#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QWebEngineView>
#include "DAAgentAPI.h"

namespace DA
{
class DAAgent_API DAAgentWebChannel : public QObject
{
    Q_OBJECT
public:
    explicit DAAgentWebChannel(QWebEngineView* view, QObject* parent = nullptr);

    // JS 调用 C++（Q_INVOKABLE）
    Q_INVOKABLE void onUserSelect(const QString& answer);
    Q_INVOKABLE void onUserMessage(const QString& text);

    // C++ 调用 JS（通过 runJavaScript）
    void appendUserMessage(const QString& text);
    void appendToken(const QString& token);
    void finalizeAgentMessage(const QString& fullText);
    void appendToolCall(const QString& toolName, const QJsonObject& args);
    void appendToolResult(const QString& toolName, const QJsonObject& result);
    void appendQuestion(const QString& text, const QStringList& options);
    void clearChat();
    void setBusy(bool busy);

signals:
    void userAnswerSelected(const QString& answer);
    void userMessageSent(const QString& text);

private:
    void callJS(const QString& funcCall);

    QWebEngineView* m_view;
};
} // namespace DA
