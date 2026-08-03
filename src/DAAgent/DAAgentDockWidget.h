// DAAgentDockWidget.h —— QWidget 子类（不是 QDockWidget，平台使用 ADS 管理停靠）
#pragma once
#include <QWidget>
#include <QWebEngineView>
#include <QWebChannel>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QJsonObject>
#include <QStringList>
#include "DAAgentAPI.h"

namespace DA
{
class DAAgentWebChannel;

// 注意：本类继承 QWidget，而非 QDockWidget。
// 本项目使用 Qt-Advanced-Docking-System (ADS)，平台 DAAppDockingArea 的
// createDockWidget(QWidget*, ...) 会把本 QWidget 包装进 ads::CDockWidget。
// 若继承 QDockWidget 再交给 createDockWidget()，会出现双标题栏、float/拖拽冲突。
class DAAgent_API DAAgentDockWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DAAgentDockWidget(QWidget* parent = nullptr);
    ~DAAgentDockWidget();

    DAAgentWebChannel* webChannel() const { return m_channel; }

private slots:
    void onSendClicked();
    void onUserAnswer(const QString& answer);

public slots:
    // 来自 Bridge 的信号 → 推送到 WebChannel
    void onAgentToken(const QString& token);
    void onAgentMessageComplete(const QString& fullText);
    void onAgentToolCall(const QString& toolName, const QJsonObject& args);
    void onAgentToolResult(const QString& toolName, const QJsonObject& result);
    void onAgentQuestion(const QString& text, const QStringList& options);
    void onAgentError(const QString& message);
    void onAgentReady(const QString& model);
    void onAgentBusy(bool busy);

signals:
    void sendMessageRequested(const QString& text);
    void userAnswerSelected(const QString& answer);

private:
    void setupUI();
    void setupWebChannel();

    QWebEngineView* m_webView;
    DAAgentWebChannel* m_channel;
    QTextEdit* m_inputEdit;
    QPushButton* m_sendButton;
    QLabel* m_statusLabel;
    bool m_agentBusy = false;
};
} // namespace DA
