// DAAgentDockWidget.cpp
#include "DAAgentDockWidget.h"
#include "DAAgentWebChannel.h"
#include "Dialog/DADialogAgentSessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWebEngineSettings>
#include <QShortcut>
#include <QKeySequence>
#include <QProgressBar>
#include <QMenu>
#include <QAction>
#include <QEvent>
#include <QMouseEvent>
#include <QCursor>
#include <QLabel>
#include <QResizeEvent>

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

    // ---- 顶部会话栏：标题（左，过长右端省略）+ 会话管理 + 新建会话（右） ----
    QWidget* sessionBar = new QWidget(this);
    sessionBar->setObjectName(QStringLiteral("da_agentSessionBar"));
    sessionBar->setStyleSheet(QStringLiteral(
        "QWidget#da_agentSessionBar { background: #f5f5f5; border-bottom: 1px solid #ddd; }"));
    QHBoxLayout* sbLayout = new QHBoxLayout(sessionBar);
    sbLayout->setContentsMargins(8, 4, 4, 4);
    sbLayout->setSpacing(4);
    m_titleLabel = new QLabel(sessionBar);
    m_titleLabel->setObjectName(QStringLiteral("da_agentTitleLabel"));
    m_titleLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: #333; padding: 0 4px; }"));
    m_titleLabel->setToolTip(QString());  // 由 updateTitleLabel 设置
    m_titleLabel->installEventFilter(this);  // resize 时重新计算省略文本
    sbLayout->addWidget(m_titleLabel, 1);
    m_sessionManagerBtn = new QPushButton(tr("Session Manager"), sessionBar);  // cn:会话管理
    m_sessionManagerBtn->setObjectName(QStringLiteral("da_agentSessionManagerBtn"));
    m_newSessionBtn = new QPushButton(tr("New Session"), sessionBar);  // cn:新建会话
    m_newSessionBtn->setObjectName(QStringLiteral("da_agentNewSessionBtn"));
    sbLayout->addWidget(m_sessionManagerBtn);
    sbLayout->addWidget(m_newSessionBtn);
    mainLayout->insertWidget(0, sessionBar);

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

    // ---- plan-04 step2: 底部 token 占比状态栏（D7：占比条 + 点击展开分类明细） ----
    // 原 m_statusLabel 改为并排容纳 m_tokenBar(QProgressBar 占比条) 与 m_tokenLabel(文字总量)。
    QWidget* statusBar = new QWidget(this);
    statusBar->setObjectName(QStringLiteral("da_agentStatusBar"));
    QHBoxLayout* stLayout = new QHBoxLayout(statusBar);
    stLayout->setContentsMargins(4, 1, 4, 1);
    stLayout->setSpacing(4);
    m_statusLabel = new QLabel(statusBar);
    m_statusLabel->setObjectName(QStringLiteral("da_agentStatusLabel"));
    m_statusLabel->setStyleSheet(QStringLiteral("QLabel { padding: 4px 8px; background: #f0f0f0; border-top: 1px solid #ddd; }"));
    m_statusLabel->setText(tr("Ready"));  // cn:就绪
    stLayout->addWidget(m_statusLabel, 1);
    m_tokenBar = new QProgressBar(statusBar);  // D7 占比条
    m_tokenBar->setRange(0, 100);
    m_tokenBar->setValue(0);
    m_tokenBar->setFixedWidth(120);
    m_tokenBar->setFormat(QStringLiteral("%p%"));
    stLayout->addWidget(m_tokenBar);
    m_tokenLabel = new QLabel(QStringLiteral("tokens: -"), statusBar);  // cn:token 计量
    m_tokenLabel->setStyleSheet(QStringLiteral("color:#666; padding:0 4px;"));
    m_tokenLabel->setCursor(Qt::PointingHandCursor);
    m_tokenLabel->installEventFilter(this);  // 点击弹 m_tokenMenu
    stLayout->addWidget(m_tokenLabel);
    m_tokenMenu = new QMenu(this);  // D7 点击弹分类明细
    mainLayout->addWidget(statusBar);

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

    // 发送/终止切换按钮：根据 m_agentBusy 状态分流
    connect(m_sendButton, &QPushButton::clicked, this, [this]() {
        if (m_agentBusy) {
            onStopClicked();
        } else {
            onSendClicked();
        }
    });

    // ---- 会话栏按钮信号 ----
    connect(m_newSessionBtn, &QPushButton::clicked, this, &DAAgentDockWidget::onNewSessionClicked);
    connect(m_sessionManagerBtn, &QPushButton::clicked, this, &DAAgentDockWidget::onSessionManagerClicked);
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
    // 绘图引用超链接点击：chat.js 拦截 da-figure: 链接 → onFigureLink → 此信号转发
    connect(m_channel, &DAAgentWebChannel::figureLinkRequested,
            this, &DAAgentDockWidget::onFigureLink);
}

void DAAgentDockWidget::onSendClicked()
{
    // 忙碌时不发送——按钮此时为 Stop 功能，由 lambda 分流到 onStopClicked
    if (m_agentBusy) {
        return;
    }
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

void DAAgentDockWidget::onStopClicked()
{
    // 定稿当前流式输出中的 agent 消息 + 关闭工具分组，避免半截消息悬挂
    if (m_channel) {
        m_channel->onAgentStopped();
    }
    // 禁用按钮防止重复点击，等待 onProcessFinished→agentBusy(false) 恢复
    m_sendButton->setEnabled(false);
    m_statusLabel->setText(tr("Stopping..."));  // cn:终止中...
    emit stopRequested();
}

void DAAgentDockWidget::onUserAnswer(const QString& answer)
{
    emit userAnswerSelected(answer);
}

void DAAgentDockWidget::onFigureLink(const QString& href)
{
    emit figureLinkRequested(href);
}

void DAAgentDockWidget::onAgentToken(const QString& token)
{
    // MAJOR4: UI 侧切换守卫——切换期间丢弃旧会话残余 token，避免污染新聊天区
    if (m_switching) return;
    if (m_channel) {
        m_channel->appendToken(token);
    }
}

void DAAgentDockWidget::onAgentMessageComplete(const QString& fullText)
{
    if (m_switching) return;  // MAJOR4
    if (m_channel) {
        m_channel->finalizeAgentMessage(fullText);
    }
}

void DAAgentDockWidget::onAgentToolCall(const QString& toolName, const QJsonObject& args)
{
    if (m_switching) return;  // MAJOR4
    if (m_channel) {
        m_channel->appendToolCall(toolName, args);
    }
}

void DAAgentDockWidget::onAgentToolResult(const QString& toolName, const QJsonObject& result)
{
    if (m_switching) return;  // MAJOR4
    if (m_channel) {
        m_channel->appendToolResult(toolName, result);
    }
}

void DAAgentDockWidget::onAgentQuestion(const QString& text, const QStringList& options, bool multiSelect)
{
    if (m_switching) return;  // MAJOR4
    if (m_channel) {
        m_channel->appendQuestion(text, options, multiSelect);
    }
}

void DAAgentDockWidget::onAgentError(const QString& message)
{
    // MAJOR1(round-6): agentError 路径也复位 m_switching=false。
    // switchSession 后若 load_session 失败走 agentError 而非 session_loaded，
    // 不复位会冻结后续渲染（守卫永真）。onAgentError 本身不被 m_switching 跳过。
    m_switching = false;
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
    m_sendButton->setText(tr("Send"));  // cn:发送
    m_sendButton->setStyleSheet(QString());
    m_sendButton->setEnabled(true);
}

void DAAgentDockWidget::onAgentBusy(bool busy)
{
    m_agentBusy = busy;
    if (busy) {
        m_statusLabel->setText(tr("Agent thinking..."));  // cn:Agent 思考中...
        m_inputEdit->setEnabled(false);
        // 切换为终止按钮：红色背景，可点击终止 agent
        m_sendButton->setText(tr("Stop"));  // cn:终止
        m_sendButton->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: #d9534f; color: white; }"));
        m_sendButton->setEnabled(true);
    } else {
        m_statusLabel->setText(tr("Ready"));  // cn:就绪
        m_inputEdit->setEnabled(true);
        // 切换回发送按钮
        m_sendButton->setText(tr("Send"));  // cn:发送
        m_sendButton->setStyleSheet(QString());
        m_sendButton->setEnabled(true);
    }
}

// ===========================================================================
// plan-04: 会话栏按钮槽 + token UI 槽 + 辅助方法
// ===========================================================================

void DAAgentDockWidget::onNewSessionClicked()
{
    emit sessionCreateRequested();
}

void DAAgentDockWidget::onSessionManagerClicked()
{
    // 弹出会话管理对话框，操作经 signal→signal 直连转发到 DAAgentInterface
    DADialogAgentSessionManager dlg(m_sessions, m_currentSessionId, this);
    connect(&dlg, &DADialogAgentSessionManager::switchRequested,
            this, &DAAgentDockWidget::sessionSwitchRequested);
    connect(&dlg, &DADialogAgentSessionManager::renameRequested,
            this, &DAAgentDockWidget::sessionRenameRequested);
    connect(&dlg, &DADialogAgentSessionManager::deleteRequested,
            this, &DAAgentDockWidget::sessionDeleteRequested);
    dlg.exec();
    // 对话框关闭后 sessionListChanged 会从 Module 回灌权威状态刷新标题
}

void DAAgentDockWidget::onTokenLabelClicked()
{
    // 点击 token 文字 → 弹分类明细 QMenu（D7）
    if (m_tokenMenu) {
        m_tokenMenu->exec(QCursor::pos());
    }
}

bool DAAgentDockWidget::eventFilter(QObject* obj, QEvent* ev)
{
    // m_tokenLabel 鼠标左键点击 → 弹 m_tokenMenu
    if (obj == m_tokenLabel && ev->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(ev);
        if (me->button() == Qt::LeftButton) {
            onTokenLabelClicked();
            return true;
        }
    }
    // m_titleLabel 尺寸变化 → 重新计算省略文本（标题过长右端 …）
    if (obj == m_titleLabel && ev->type() == QEvent::Resize) {
        updateTitleLabel();
        return false;
    }
    return QWidget::eventFilter(obj, ev);
}

// ---- plan-04 step3: 新槽 ----

void DAAgentDockWidget::onAgentUsage(int inputTokens, int outputTokens,
                                     int totalTokens, int contextWindow,
                                     const QString& source)
{
    // 契约2: 5 参含 contextWindow 与 source。一期不做 system/tools/history/current 四分类估算。
    int pct = contextWindow > 0 ? int(totalTokens * 100 / contextWindow) : 0;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    m_tokenBar->setValue(pct);  // D7 占比条值 = tot*100/window
    m_tokenLabel->setText(tr("tokens: %1 / %2")  // cn:token: 当前 / 窗口
                              .arg(totalTokens)
                              .arg(contextWindow > 0 ? contextWindow : -1));
    rebuildTokenMenu(inputTokens, outputTokens, totalTokens, contextWindow, source);
}

void DAAgentDockWidget::onAgentSessionLoaded(const QString& sessionId)
{
    Q_UNUSED(sessionId);
    // Python load_session 重建完成，解除 UI 切换守卫（MAJOR4）
    m_switching = false;
    m_statusLabel->setText(tr("Ready"));  // cn:就绪
}

void DAAgentDockWidget::onSessionSwitched(const QString& sessionId,
                                          const QVector<QJsonObject>& allRecords)
{
    // MAJOR4: UI 侧切换守卫——先清空，进行中的 token 经 m_switching 丢弃
    m_switching = true;
    if (m_channel) {
        m_channel->clearChat();
        m_channel->loadHistory(allRecords);  // 重放新会话 UI（C++ 合并后事件，见 WebChannel::loadHistory）
    }
    m_currentSessionId = sessionId;
    updateTitleLabel();
}

void DAAgentDockWidget::onSessionListChanged(QVariantList sessions)
{
    // 契约3: 缓存 payload（含 updatedAt/messageCount 元信息），刷新标题
    m_sessions = sessions;
    updateTitleLabel();
}

void DAAgentDockWidget::onSessionCreated(const QString& sessionId)
{
    // MAJOR7: 仅 newSession 路径触发本槽——新会话清空聊天 + 复位守卫。
    // 标题刷新由 sessionListChanged(payload) 信号驱动。
    // Bug2 修复：新会话无 usage，复位 token 控件避免拋留上一会话数值。
    m_switching = false;
    m_currentSessionId = sessionId;
    if (m_channel) {
        m_channel->clearChat();
    }
    resetTokenStats();
    updateTitleLabel();
}

void DAAgentDockWidget::onSessionCleared()
{
    // 启动/打开工程后始终全新对话，不自动恢复上次会话。
    // 清空残留聊天区、复位 token 控件、清空标题、解除切换守卫。
    m_switching = false;
    m_currentSessionId.clear();
    if (m_channel) {
        m_channel->clearChat();
    }
    resetTokenStats();
    updateTitleLabel();
}

void DAAgentDockWidget::resetTokenStats()
{
    // 复位到无活跃会话初始态：进度条 0%、标签 "tokens: -"、明细菜单清空
    if (m_tokenBar) m_tokenBar->setValue(0);
    if (m_tokenLabel) m_tokenLabel->setText(tr("tokens: -"));  // cn:token: -
    if (m_tokenMenu) m_tokenMenu->clear();
}

// ---- 辅助方法 ----

void DAAgentDockWidget::updateTitleLabel()
{
    // 按 m_currentSessionId 在缓存中查标题；空标题显示「(untitled)」
    if (!m_titleLabel) return;
    QString fullTitle;
    if (!m_currentSessionId.isEmpty()) {
        for (int i = 0; i < m_sessions.size(); ++i) {
            QVariantMap vm = m_sessions.at(i).toMap();
            if (vm.value("id").toString() == m_currentSessionId) {
                fullTitle = vm.value("title").toString();
                break;
            }
        }
        if (fullTitle.isEmpty()) {
            fullTitle = tr("(untitled)");  // cn:（未命名）
        }
    }
    m_currentSessionFullTitle = fullTitle;
    // tooltip 显示完整标题（空标题不弹 tooltip）
    m_titleLabel->setToolTip(fullTitle);
    // 按当前可用宽度省略渲染（右端 …）
    int w = m_titleLabel->width();
    if (w <= 0) {
        // 尚未布局完成，直接放全文，resize 事件触发时会重新省略
        m_titleLabel->setText(fullTitle);
        return;
    }
    // 减去内边距避免 … 紧贴右边缘
    const int pad = 12;
    QString shown = m_titleLabel->fontMetrics().elidedText(
        fullTitle, Qt::ElideRight, qMax(0, w - pad));
    m_titleLabel->setText(shown);
}

void DAAgentDockWidget::rebuildTokenMenu(int inT, int outT, int tot,
                                         int window, const QString& source)
{
    if (!m_tokenMenu) return;
    // D7 分类明细 QMenu：input/output/total/window/source 五项（一期不做四分类）
    m_tokenMenu->clear();
    m_tokenMenu->addAction(tr("input: %1").arg(inT));   // cn:输入：
    m_tokenMenu->addAction(tr("output: %1").arg(outT)); // cn:输出：
    m_tokenMenu->addAction(tr("total: %1").arg(tot));   // cn:总计：
    m_tokenMenu->addAction(tr("window: %1").arg(window > 0 ? window : -1));  // cn:窗口：
    m_tokenMenu->addSeparator();
    m_tokenMenu->addAction(tr("source: %1").arg(source.isEmpty() ? tr("unknown") : source));  // cn:来源：
}

} // namespace DA
