// DAAgentDockWidget.cpp
#include "DAAgentDockWidget.h"
#include "DAAgentWebChannel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWebEngineSettings>
#include <QShortcut>
#include <QKeySequence>

namespace DA
{

DAAgentDockWidget::DAAgentDockWidget(QWidget* parent)
    : QWidget(parent)
    , m_webView(nullptr)
    , m_channel(nullptr)
    , m_inputEdit(nullptr)
    , m_sendButton(nullptr)
    , m_statusLabel(nullptr)
    , m_agentBusy(false)
{
    setupUI();
    setupWebChannel();
}

DAAgentDockWidget::~DAAgentDockWidget()
{
}

void DAAgentDockWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // QWebEngineView 占主要空间
    m_webView = new QWebEngineView(this);
    m_webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 启用开发者工具（生产环境可设为 false）
    // Qt5 无 DeveloperToolsEnabled 属性，可通过环境变量
    // QTWEBENGINE_CHROMIUM_FLAGS=--remote-debugging-port=9222 替代
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_webView->settings()->setAttribute(QWebEngineSettings::DeveloperToolsEnabled, true);
#endif
    mainLayout->addWidget(m_webView, 1);

    // 底部状态标签
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("da_agentStatusLabel"));
    m_statusLabel->setStyleSheet(QStringLiteral("QLabel { padding: 4px 8px; background: #f0f0f0; border-top: 1px solid #ddd; }"));
    m_statusLabel->setText(tr("Ready"));  // cn:就绪
    mainLayout->addWidget(m_statusLabel);

    // 输入区：QTextEdit + 发送按钮
    QWidget* inputContainer = new QWidget(this);
    inputContainer->setObjectName(QStringLiteral("da_agentInputContainer"));
    QHBoxLayout* inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(4, 4, 4, 4);
    inputLayout->setSpacing(4);

    m_inputEdit = new QTextEdit(inputContainer);
    m_inputEdit->setObjectName(QStringLiteral("da_agentInputEdit"));
    m_inputEdit->setPlaceholderText(tr("Type a message... (Ctrl+Enter to send)"));  // cn:输入消息...（Ctrl+Enter 发送）
    m_inputEdit->setMaximumHeight(80);
    m_inputEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_sendButton = new QPushButton(tr("Send"), inputContainer);  // cn:发送
    m_sendButton->setObjectName(QStringLiteral("da_agentSendButton"));
    m_sendButton->setFixedSize(60, 32);

    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(m_sendButton);

    mainLayout->addWidget(inputContainer);

    // 加载 chat.html
    m_webView->setUrl(QUrl(QStringLiteral("qrc:///DAAgent/chat.html")));

    // Ctrl+Enter 快捷键发送
    QShortcut* sendShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Return")), this);
    connect(sendShortcut, &QShortcut::activated, this, &DAAgentDockWidget::onSendClicked);

    // 发送按钮
    connect(m_sendButton, &QPushButton::clicked, this, &DAAgentDockWidget::onSendClicked);
}

void DAAgentDockWidget::setupWebChannel()
{
    m_channel = new DAAgentWebChannel(m_webView, this);
    QWebChannel* webChannel = new QWebChannel(this);
    // JS 侧通过 channel.objects.chatBridge 访问，名字必须与 chat.js 一致
    webChannel->registerObject(QStringLiteral("chatBridge"), m_channel);
    m_webView->page()->setWebChannel(webChannel);

    // Connect the web channel's userAnswerSelected signal to this dock widget's signal
    // so the Bridge can receive user answers to agent questions.
    connect(m_channel, &DAAgentWebChannel::userAnswerSelected,
            this, &DAAgentDockWidget::onUserAnswer);
}

void DAAgentDockWidget::onSendClicked()
{
    // 1. 获取输入文本
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        return;
    }
    // 2. 清空输入框
    m_inputEdit->clear();
    // 3. 在 UI 上显示用户消息（在 emit 之前调用，确保 UI 即时更新）
    m_channel->appendUserMessage(text);
    // 4. 通知 Bridge 发送消息
    emit sendMessageRequested(text);
}

void DAAgentDockWidget::onUserAnswer(const QString& answer)
{
    emit userAnswerSelected(answer);
}

void DAAgentDockWidget::onAgentToken(const QString& token)
{
    if (m_channel) {
        m_channel->appendToken(token);
    }
}

void DAAgentDockWidget::onAgentMessageComplete(const QString& fullText)
{
    if (m_channel) {
        m_channel->finalizeAgentMessage(fullText);
    }
}

void DAAgentDockWidget::onAgentToolCall(const QString& toolName, const QJsonObject& args)
{
    if (m_channel) {
        m_channel->appendToolCall(toolName, args);
    }
}

void DAAgentDockWidget::onAgentToolResult(const QString& toolName, const QJsonObject& result)
{
    if (m_channel) {
        m_channel->appendToolResult(toolName, result);
    }
}

void DAAgentDockWidget::onAgentQuestion(const QString& text, const QStringList& options)
{
    if (m_channel) {
        m_channel->appendQuestion(text, options);
    }
}

void DAAgentDockWidget::onAgentError(const QString& message)
{
    if (m_channel) {
        m_channel->appendToken(QStringLiteral("**Error:** ") + message);
        m_channel->finalizeAgentMessage(QStringLiteral("**Error:** ") + message);
    }
}

void DAAgentDockWidget::onAgentReady(const QString& model)
{
    Q_UNUSED(model);
    m_agentBusy = false;
    m_statusLabel->setText(tr("Ready"));  // cn:就绪
    m_inputEdit->setEnabled(true);
    m_sendButton->setEnabled(true);
}

void DAAgentDockWidget::onAgentBusy(bool busy)
{
    m_agentBusy = busy;
    if (busy) {
        m_statusLabel->setText(tr("Agent thinking..."));  // cn:Agent 思考中...
        m_inputEdit->setEnabled(false);
        m_sendButton->setEnabled(false);
    } else {
        m_statusLabel->setText(tr("Ready"));  // cn:就绪
        m_inputEdit->setEnabled(true);
        m_sendButton->setEnabled(true);
    }
}

} // namespace DA
