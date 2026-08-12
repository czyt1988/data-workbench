// DAAgentDockWidget.cpp
#include "DAAgentDockWidget.h"
#include "DAAgentWebChannel.h"
#include "Dialog/DADialogAgentSessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWebEngineSettings>
#include <QIcon>
#include <QEvent>
#include <QLabel>
#include <QResizeEvent>

namespace DA
{

DAAgentDockWidget::DAAgentDockWidget(QWidget* parent)
    : QWidget(parent)
    , m_webView(nullptr)
    , m_channel(nullptr)
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
    // 会话管理 / 新建会话：图标按钮（svg），tooltip 承载翻译文案
    m_sessionManagerBtn = new QPushButton(sessionBar);
    m_sessionManagerBtn->setObjectName(QStringLiteral("da_agentSessionManagerBtn"));
    m_sessionManagerBtn->setIcon(QIcon(QStringLiteral(":/DAGui/icon/session-manager.svg")));
    m_sessionManagerBtn->setIconSize(QSize(18, 18));
    m_sessionManagerBtn->setFixedSize(28, 28);
    m_sessionManagerBtn->setToolTip(tr("Session Manager"));  // cn:会话管理
    m_sessionManagerBtn->setCursor(Qt::PointingHandCursor);
    m_newSessionBtn = new QPushButton(sessionBar);
    m_newSessionBtn->setObjectName(QStringLiteral("da_agentNewSessionBtn"));
    m_newSessionBtn->setIcon(QIcon(QStringLiteral(":/DAGui/icon/session-new.svg")));
    m_newSessionBtn->setIconSize(QSize(18, 18));
    m_newSessionBtn->setFixedSize(28, 28);
    m_newSessionBtn->setToolTip(tr("New Session"));  // cn:新建会话
    m_newSessionBtn->setCursor(Qt::PointingHandCursor);
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

    // 加载 chat.html（现在内含 聊天区+状态栏+输入区，一个连续 web 表面）
    m_webView->setUrl(QUrl(QStringLiteral("qrc:///DAAgent/chat.html")));

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
    // web 输入区发送：chat.js onUserMessage → userMessageSent → C++ 编排（appendUserMessage + emit）
    connect(m_channel, &DAAgentWebChannel::userMessageSent,
            this, &DAAgentDockWidget::onUserMessageReceived);
    // web 就绪握手：flush 当前态（i18n/busy/model/tokenStats）
    connect(m_channel, &DAAgentWebChannel::webReady,
            this, &DAAgentDockWidget::onWebReady);
    // web 输入区 Stop 按钮：直达 C++ 终止流程（替代旧原生 m_sendButton 分流）
    connect(m_channel, &DAAgentWebChannel::stopRequested,
            this, &DAAgentDockWidget::onStopClicked);
}

void DAAgentDockWidget::onUserMessageReceived(const QString& text)
{
    // C++ 仍是编排者：JS 已清框并调 chatBridge.onUserMessage(text)，此槽负责
    // 渲染用户气泡 + 向外发消息。与旧 onSendClicked 同构（文本来源从 QTextEdit 改为 JS）。
    if (m_agentBusy) {
        return;  // 忙碌时不发送（web 按钮此时为 Stop，理论不会触发；防御）
    }
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    m_channel->appendUserMessage(trimmed);  // C++ 渲染用户气泡（单一权威）
    emit sendMessageRequested(trimmed);
}

void DAAgentDockWidget::onWebReady()
{
    // web 侧就绪：注入静态 i18n 标签 + flush 当前态，缓解 JS-ready 竞态
    // （agent 信号若在 chat.html 加载完成前触发，此处补推当前 busy/model/token）
    if (!m_channel) return;
    m_channel->setI18nLabels(QVariantMap{
        {"send", tr("Send")},                       // cn:发送
        {"stop", tr("Stop")},                        // cn:终止
        {"ready", tr("Ready")},                      // cn:就绪
        {"thinking", tr("Agent thinking...")},       // cn:Agent 思考中...
        {"stopping", tr("Stopping...")},             // cn:终止中...
        {"inputPlaceholder", tr("Type a message...")},  // cn:输入消息...
        {"tokenEmpty", tr("tokens: -")},             // cn:token: -
        {"popoverInput", tr("input: %1")},           // cn:输入：%1
        {"popoverOutput", tr("output: %1")},         // cn:输出：%1
        {"popoverTotal", tr("total: %1")},           // cn:总计：%1
        {"popoverWindow", tr("window: %1")},         // cn:窗口：%1
        {"popoverSource", tr("source: %1")},          // cn:来源：%1
        {"popoverSourceUnknown", tr("unknown")}      // cn:未知
    });
    m_channel->setBusy(m_agentBusy);
    m_channel->setModel(formatModelLabel());
    if (m_hasTokenStats) {
        m_channel->setTokenStats(formatTokenLabel(m_lastTotalTokens, m_lastContextWindow, m_lastTokenSource),
                                  m_lastInTokens, m_lastOutTokens, m_lastTotalTokens,
                                  m_lastContextWindow, m_lastTokenSource);
    }
    m_channel->focusInput();
}

void DAAgentDockWidget::onStopClicked()
{
    // 定稿当前流式输出中的 agent 消息 + 关闭工具分组，避免半截消息悬挂
    if (m_channel) {
        m_channel->onAgentStopped();
        // 停止过渡态：web 按钮禁用防重复点 + 状态 Stopping...，持续到 onAgentBusy(false)/Ready 恢复
        m_channel->setStopping();
    }
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

void DAAgentDockWidget::onAgentError(const QString& message, const QString& errorType, const QString& detail)
{
    Q_UNUSED(detail);
    // switchSession 后若 load_session 失败走 agentError 而非 session_loaded，
    // 不复位 m_switching 会冻结后续渲染（守卫永真）——沿用现有逻辑
    m_switching = false;
    // 根据 errorType 选择用户文案
    QString displayMessage = mapErrorMessage(message, errorType);
    // 调用 chat.js 渲染错误（standalone error card，由 plan-06 实现）
    if (m_channel) {
        m_channel->appendError(displayMessage, errorType);
    }
}

void DAAgentDockWidget::onAgentRetrying(int attempt, int maxAttempts, int delayMs,
                                         const QString& errorType, const QString& errorMessage)
{
    // 通过 WebChannel 调用 chat.js 的 showRetryStatus
    if (m_channel) {
        m_channel->showRetryStatus(attempt, maxAttempts, delayMs, errorType, errorMessage);
    }
}

void DAAgentDockWidget::onAgentReady(const QString& model)
{
    m_currentModel = model;
    m_agentBusy = false;
    if (m_channel) {
        m_channel->setBusy(false);             // 复位为 ready：按钮 Send + 输入启用 + 状态 Ready
        m_channel->setModel(formatModelLabel());  // 推送 "Model: <name>"
    }
}

void DAAgentDockWidget::onAgentBusy(bool busy)
{
    m_agentBusy = busy;
    // busy 打包：JS 解释按钮 Send/Stop 切换 + 输入禁用 + 状态文案（thinking/ready）
    if (m_channel) {
        m_channel->setBusy(busy);
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

bool DAAgentDockWidget::eventFilter(QObject* obj, QEvent* ev)
{
    // m_titleLabel 尺寸变化 → 重新计算省略文本（标题过长右端 …）
    // （输入区/状态栏/token 明细已迁 web，eventFilter 只剩会话栏标题省略）
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
    // 缓存最近一次 usage：web 未就绪时丢失的推送，onWebReady 重推。
    m_lastInTokens = inputTokens;
    m_lastOutTokens = outputTokens;
    m_lastTotalTokens = totalTokens;
    m_lastContextWindow = contextWindow;
    m_lastTokenSource = source;
    m_hasTokenStats = true;
    // streaming_estimate 期间显示 ~ 前缀，表示是流式估算值而非权威统计；
    // 真实 usage 到达后（source 为 agent/summary）前缀消失。
    if (m_channel) {
        m_channel->setTokenStats(formatTokenLabel(totalTokens, contextWindow, source),
                                  inputTokens, outputTokens, totalTokens,
                                  contextWindow, source);
    }
}

void DAAgentDockWidget::onAgentSessionLoaded(const QString& sessionId)
{
    Q_UNUSED(sessionId);
    // Python load_session 重建完成，解除 UI 切换守卫（MAJOR4）
    m_switching = false;
    // 重新断言当前 busy 态（若非忙则状态文案置 Ready），消除可能的 Stopping 残留
    if (m_channel) {
        m_channel->setBusy(m_agentBusy);
    }
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
    m_hasTokenStats = false;  // 新会话无 usage，复位缓存
    if (m_channel) {
        m_channel->clearChat();
        m_channel->resetTokenStats();  // 复位 web 侧 token 标签 + popover
        m_channel->focusInput();       // 新会话聚焦输入框
    }
    updateTitleLabel();
}

void DAAgentDockWidget::onSessionCleared()
{
    // 启动/打开工程后始终全新对话，不自动恢复上次会话。
    // 清空残留聊天区、复位 token 控件、清空标题、解除切换守卫。
    m_switching = false;
    m_currentSessionId.clear();
    m_hasTokenStats = false;
    if (m_channel) {
        m_channel->clearChat();
        m_channel->resetTokenStats();
        m_channel->focusInput();
    }
    updateTitleLabel();
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

QString DAAgentDockWidget::formatModelLabel() const
{
    // 返回模型标签串：空模型 "Model: -"，非空 "Model: <name>"（已 tr 翻译）。
    // 省略由 web 侧 CSS text-overflow:ellipsis 处理，tooltip 由 JS setModel 设置。
    if (m_currentModel.isEmpty()) {
        return tr("Model: -");  // cn:模型：-
    }
    return tr("Model: %1").arg(m_currentModel);  // cn:模型：%1
}

QString DAAgentDockWidget::formatTokenLabel(int totalTokens, int contextWindow, const QString& source) const
{
    // 返回 token 计量串：streaming_estimate 带 ~ 前缀，否则 "tokens: N / window"（已 tr 翻译）。
    // window<=0 显示 -1。popover 五项明细由 web 侧 JS 用注入的模板串渲染
    // （C++ 只推这 5 原始值，标签复用 tr("input: %1") 等既有翻译，JS 做 %1→值 替换）。
    int win = contextWindow > 0 ? contextWindow : -1;
    if (source == QStringLiteral("streaming_estimate")) {
        return tr("tokens: ~%1 / %2").arg(totalTokens).arg(win);  // cn:token: ~当前 / 窗口
    }
    return tr("tokens: %1 / %2").arg(totalTokens).arg(win);  // cn:token: 当前 / 窗口
}

QString DAAgentDockWidget::mapErrorMessage(const QString& original, const QString& errorType) const
{
    // 按 error_type 选择翻译后的用户文案
    if (errorType == "quota_exhausted") {
        return tr("API quota exhausted, please check account balance or change API key"); //cn:API 配额已耗尽，请检查账户余额或更换 API Key
    }
    if (errorType == "auth_error") {
        return tr("API key invalid or expired, please check settings"); //cn:API Key 无效或已过期，请在设置中检查配置
    }
    if (errorType == "rate_limit_exhausted") {
        return tr("Failed after %1 retries: rate limited").arg(7); //cn:重试 7 次后仍失败：服务限流
    }
    if (errorType == "network_exhausted") {
        return tr("Failed after %1 retries: network error").arg(7); //cn:重试 7 次后仍失败：网络错误
    }
    if (errorType == "server_error_exhausted") {
        return tr("Failed after %1 retries: server error").arg(7); //cn:重试 7 次后仍失败：服务器错误
    }
    if (errorType == "bad_request") {
        return tr("Request format error: %1").arg(original); //cn:请求格式错误：%1
    }
    if (errorType == "context_overflow") {
        return tr("Context window exceeded and compaction failed"); //cn:上下文窗口超限且压缩失败
    }
    if (errorType == "timeout") {
        return tr("Agent response timeout (no activity for %1 minutes)").arg(4); //cn:Agent 响应超时（%1 分钟无活动）
    }
    if (errorType == "crash_recovery") {
        return tr("Agent process crashed, recovering... (%1/3)").arg(1); //cn:Agent 进程异常退出，正在恢复... (%1/3)
    }
    if (errorType == "crash_exhausted") {
        return tr("Agent process crashed repeatedly, unable to recover"); //cn:Agent 进程多次崩溃，无法恢复
    }
    // unknown 或空
    return tr("Agent error: %1").arg(original); //cn:Agent 错误：%1
}

} // namespace DA
