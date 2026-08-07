// DAAgentDockWidget.cpp
#include "DAAgentDockWidget.h"
#include "DAAgentWebChannel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWebEngineSettings>
#include <QShortcut>
#include <QKeySequence>
#include <QComboBox>
#include <QInputDialog>
#include <QProgressBar>
#include <QMenu>
#include <QAction>
#include <QEvent>
#include <QMouseEvent>
#include <QCursor>
#include <QLabel>

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

    // ---- plan-04 step1: 顶部会话栏（D8：下拉 + 新建/删除/重命名按钮，紧凑不占聊天区） ----
    QWidget* sessionBar = new QWidget(this);
    sessionBar->setObjectName(QStringLiteral("da_agentSessionBar"));
    QHBoxLayout* sbLayout = new QHBoxLayout(sessionBar);
    sbLayout->setContentsMargins(4, 2, 4, 2);
    sbLayout->setSpacing(4);
    sbLayout->addWidget(new QLabel(tr("Session")));  // cn:会话
    m_sessionCombo = new QComboBox(this);
    m_sessionCombo->setObjectName(QStringLiteral("da_agentSessionCombo"));
    sbLayout->addWidget(m_sessionCombo, 1);
    m_newSessionBtn = new QPushButton("+", this);  // cn:新建
    m_newSessionBtn->setObjectName(QStringLiteral("da_agentNewSessionBtn"));
    m_newSessionBtn->setFixedWidth(28);
    m_renameSessionBtn = new QPushButton(tr("Rename"), this);  // cn:重命名
    m_renameSessionBtn->setObjectName(QStringLiteral("da_agentRenameSessionBtn"));
    m_deleteSessionBtn = new QPushButton(QString(QChar(0xD7)), this);  // × //cn:删除
    m_deleteSessionBtn->setObjectName(QStringLiteral("da_agentDeleteSessionBtn"));
    m_deleteSessionBtn->setFixedWidth(28);
    sbLayout->addWidget(m_newSessionBtn);
    sbLayout->addWidget(m_renameSessionBtn);
    sbLayout->addWidget(m_deleteSessionBtn);
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

    // ---- plan-04 step1: 会话栏按钮/下拉信号 ----
    connect(m_sessionCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &DAAgentDockWidget::onSessionComboActivated);
    connect(m_newSessionBtn, &QPushButton::clicked, this, &DAAgentDockWidget::onNewSessionClicked);
    connect(m_deleteSessionBtn, &QPushButton::clicked, this, &DAAgentDockWidget::onDeleteSessionClicked);
    connect(m_renameSessionBtn, &QPushButton::clicked, this, &DAAgentDockWidget::onRenameSessionClicked);
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

void DAAgentDockWidget::onSessionComboActivated(int index)
{
    QString sid = m_sessionCombo->itemData(index).toString();
    if (sid.isEmpty()) return;
    emit sessionSwitchRequested(sid);
}

void DAAgentDockWidget::onNewSessionClicked()
{
    emit sessionCreateRequested();
}

void DAAgentDockWidget::onDeleteSessionClicked()
{
    // 边界：下拉为空时直接返回
    if (m_sessionCombo->count() == 0) return;
    QString sid = m_sessionCombo->currentData().toString();
    if (sid.isEmpty()) return;
    emit sessionDeleteRequested(sid);
}

void DAAgentDockWidget::onRenameSessionClicked()
{
    // 边界：下拉为空时直接返回
    if (m_sessionCombo->count() == 0) return;
    QString sid = m_sessionCombo->currentData().toString();
    if (sid.isEmpty()) return;
    QString oldTitle = m_sessionCombo->currentText();
    bool ok = false;
    QString newTitle = QInputDialog::getText(this,
        tr("Rename Session"),  // cn:重命名会话
        tr("New title:"),     // cn:新标题：
        QLineEdit::Normal,
        oldTitle,
        &ok);
    if (!ok || newTitle.trimmed().isEmpty()) return;
    emit sessionRenameRequested(sid, newTitle.trimmed());
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
    int idx = findSessionIndex(sessionId);
    if (idx >= 0) {
        // 切换下拉选中项时屏蔽信号，避免 activated 误触发新一轮 switchSession
        m_sessionCombo->blockSignals(true);
        m_sessionCombo->setCurrentIndex(idx);
        m_sessionCombo->blockSignals(false);
    }
}

void DAAgentDockWidget::onSessionListChanged(QVariantList sessions)
{
    // 契约3: 直接用 payload 填充 m_sessionCombo，无需 Module 指针、无需回调 listSessions()
    refreshSessionCombo(sessions);
}

void DAAgentDockWidget::onSessionCreated(const QString& sessionId)
{
    Q_UNUSED(sessionId);
    // MAJOR7: 仅 newSession 路径触发本槽——新会话清空聊天 + 复位守卫。
    // 下拉刷新由 sessionListChanged(payload) 信号驱动。
    // Bug2 修复：新会话无 usage，复位 token 控件避免拋留上一会话数值。
    m_switching = false;
    if (m_channel) {
        m_channel->clearChat();
    }
    resetTokenStats();
}

void DAAgentDockWidget::onSessionCleared()
{
    // Bug1 修复：restoreLastActiveSession 未命中且无工程会话时发射 sessionCleared。
    // 清空残留聊天区（原游离会话历史）、复位 token 控件、下拉不选中、解除切换守卫。
    m_switching = false;
    if (m_channel) {
        m_channel->clearChat();
    }
    resetTokenStats();
    if (m_sessionCombo) {
        m_sessionCombo->blockSignals(true);
        m_sessionCombo->setCurrentIndex(-1);
        m_sessionCombo->blockSignals(false);
    }
}

void DAAgentDockWidget::resetTokenStats()
{
    // 复位到无活跃会话初始态：进度条 0%、标签 "tokens: -"、明细菜单清空
    if (m_tokenBar) m_tokenBar->setValue(0);
    if (m_tokenLabel) m_tokenLabel->setText(tr("tokens: -"));  // cn:token: -
    if (m_tokenMenu) m_tokenMenu->clear();
}

// ---- 辅助方法 ----

void DAAgentDockWidget::refreshSessionCombo(const QVariantList& sessions)
{
    // 保留当前选中会话 ID，刷新后若仍存在则保持选中
    QString prevId = (m_sessionCombo->count() > 0)
                         ? m_sessionCombo->currentData().toString()
                         : QString();
    m_sessionCombo->blockSignals(true);  // 避免清空/插入触发 activated
    m_sessionCombo->clear();
    int newIndex = -1;
    for (int i = 0; i < sessions.size(); ++i) {
        QVariantMap vm = sessions.at(i).toMap();
        QString id = vm.value("id").toString();
        QString title = vm.value("title").toString();
        if (title.isEmpty()) {
            title = tr("(untitled)");  // cn:（未命名）
        }
        m_sessionCombo->addItem(title, id);
        if (id == prevId) newIndex = i;
    }
    if (newIndex >= 0) {
        m_sessionCombo->setCurrentIndex(newIndex);
    } else if (m_sessionCombo->count() > 0) {
        m_sessionCombo->setCurrentIndex(0);
    }
    m_sessionCombo->blockSignals(false);
}

int DAAgentDockWidget::findSessionIndex(const QString& sid) const
{
    if (!m_sessionCombo) return -1;
    for (int i = 0; i < m_sessionCombo->count(); ++i) {
        if (m_sessionCombo->itemData(i).toString() == sid) {
            return i;
        }
    }
    return -1;
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
