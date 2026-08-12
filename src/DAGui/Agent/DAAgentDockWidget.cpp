// DAAgentDockWidget.cpp
#include "DAAgentDockWidget.h"
#include "DAAgentWebChannel.h"
#include "Dialog/DADialogAgentSessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QWebChannel>
#include <QPushButton>
#include <QLabel>
#include <QIcon>
#include <QEvent>
#include <QResizeEvent>

namespace DA
{

class DAAgentDockWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAAgentDockWidget)
public:
    PrivateData(DAAgentDockWidget* p);

    QWebEngineView* mWebView;
    DAAgentWebChannel* mChannel;
    bool mAgentBusy;

    // ---- 顶部会话栏：标题（省略）+ 会话管理 + 新建会话 ----
    QLabel* mTitleLabel;
    QPushButton* mSessionManagerBtn;
    QPushButton* mNewSessionBtn;
    QString mCurrentSessionId;        ///< 当前活跃会话 ID
    QString mCurrentSessionFullTitle; ///< 当前会话完整标题（供省略渲染与 tooltip）
    QVariantList mSessions;           ///< 缓存 sessionListChanged payload（含元信息）
    QString mCurrentModel;            ///< 当前模型名称（由 onAgentReady 回填，onWebReady 推给 web）
    // ---- token 统计缓存：web 未就绪时丢失的推送，onWebReady 重推 ----
    int mLastInTokens;
    int mLastOutTokens;
    int mLastTotalTokens;
    int mLastContextWindow;
    QString mLastTokenSource;
    bool mHasTokenStats;
    // ---- MAJOR4 UI 侧切换守卫：true 时渲染槽跳过，避免旧会话残余 token 渲染到新聊天区 ----
    bool mSwitching;
};

DAAgentDockWidget::PrivateData::PrivateData(DAAgentDockWidget* p)
    : q_ptr(p)
    , mWebView(nullptr)
    , mChannel(nullptr)
    , mAgentBusy(false)
    , mTitleLabel(nullptr)
    , mSessionManagerBtn(nullptr)
    , mNewSessionBtn(nullptr)
    , mLastInTokens(0)
    , mLastOutTokens(0)
    , mLastTotalTokens(0)
    , mLastContextWindow(0)
    , mHasTokenStats(false)
    , mSwitching(false)
{
}

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DAAgentDockWidget::DAAgentDockWidget(QWidget* parent)
    : QWidget(parent)
    , DA_PIMPL_CONSTRUCT
{
    setupUI();
    setupWebChannel();
}

/**
 * @brief 析构函数
 */
DAAgentDockWidget::~DAAgentDockWidget()
{
}

/**
 * @brief 获取关联的 WebChannel 对象
 * @return WebChannel 指针
 */
DAAgentWebChannel* DAAgentDockWidget::webChannel() const
{
    DA_DC(d);
    return d->mChannel;
}

/**
 * @brief 初始化 UI 界面
 */
void DAAgentDockWidget::setupUI()
{
    DA_D(d);
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
    d->mTitleLabel = new QLabel(sessionBar);
    d->mTitleLabel->setObjectName(QStringLiteral("da_agentTitleLabel"));
    d->mTitleLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: #333; padding: 0 4px; }"));
    d->mTitleLabel->setToolTip(QString());  // 由 updateTitleLabel 设置
    d->mTitleLabel->installEventFilter(this);  // resize 时重新计算省略文本
    sbLayout->addWidget(d->mTitleLabel, 1);
    // 会话管理 / 新建会话：图标按钮（svg），tooltip 承载翻译文案
    d->mSessionManagerBtn = new QPushButton(sessionBar);
    d->mSessionManagerBtn->setObjectName(QStringLiteral("da_agentSessionManagerBtn"));
    d->mSessionManagerBtn->setIcon(QIcon(QStringLiteral(":/DAGui/icon/session-manager.svg")));
    d->mSessionManagerBtn->setIconSize(QSize(18, 18));
    d->mSessionManagerBtn->setFixedSize(28, 28);
    d->mSessionManagerBtn->setToolTip(tr("Session Manager"));  // cn:会话管理
    d->mSessionManagerBtn->setCursor(Qt::PointingHandCursor);
    d->mNewSessionBtn = new QPushButton(sessionBar);
    d->mNewSessionBtn->setObjectName(QStringLiteral("da_agentNewSessionBtn"));
    d->mNewSessionBtn->setIcon(QIcon(QStringLiteral(":/DAGui/icon/session-new.svg")));
    d->mNewSessionBtn->setIconSize(QSize(18, 18));
    d->mNewSessionBtn->setFixedSize(28, 28);
    d->mNewSessionBtn->setToolTip(tr("New Session"));  // cn:新建会话
    d->mNewSessionBtn->setCursor(Qt::PointingHandCursor);
    sbLayout->addWidget(d->mSessionManagerBtn);
    sbLayout->addWidget(d->mNewSessionBtn);
    mainLayout->insertWidget(0, sessionBar);

    // QWebEngineView 占主要空间
    d->mWebView = new QWebEngineView(this);
    d->mWebView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 启用开发者工具（生产环境可设为 false）
    // Qt5 无 DeveloperToolsEnabled 属性，可通过环境变量
    // QTWEBENGINE_CHROMIUM_FLAGS=--remote-debugging-port=9222 替代
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    d->mWebView->settings()->setAttribute(QWebEngineSettings::DeveloperToolsEnabled, true);
#endif
    mainLayout->addWidget(d->mWebView, 1);

    // 加载 chat.html（现在内含 聊天区+状态栏+输入区，一个连续 web 表面）
    d->mWebView->setUrl(QUrl(QStringLiteral("qrc:///DAAgent/chat.html")));

    // ---- 会话栏按钮信号 ----
    connect(d->mNewSessionBtn, &QPushButton::clicked, this, &DAAgentDockWidget::onNewSessionClicked);
    connect(d->mSessionManagerBtn, &QPushButton::clicked, this, &DAAgentDockWidget::onSessionManagerClicked);
}

/**
 * @brief 初始化 WebChannel
 */
void DAAgentDockWidget::setupWebChannel()
{
    DA_D(d);
    d->mChannel = new DAAgentWebChannel(d->mWebView, this);
    QWebChannel* webChannel = new QWebChannel(this);
    // JS 侧通过 channel.objects.chatBridge 访问，名字必须与 chat.js 一致
    webChannel->registerObject(QStringLiteral("chatBridge"), d->mChannel);
    d->mWebView->page()->setWebChannel(webChannel);

    // Connect the web channel's userAnswerSelected signal to this dock widget's signal
    // so the Bridge can receive user answers to agent questions.
    connect(d->mChannel, &DAAgentWebChannel::userAnswerSelected,
            this, &DAAgentDockWidget::onUserAnswer);
    // 绘图引用超链接点击：chat.js 拦截 da-figure: 链接 → onFigureLink → 此信号转发
    connect(d->mChannel, &DAAgentWebChannel::figureLinkRequested,
            this, &DAAgentDockWidget::onFigureLink);
    // web 输入区发送：chat.js onUserMessage → userMessageSent → C++ 编排（appendUserMessage + emit）
    connect(d->mChannel, &DAAgentWebChannel::userMessageSent,
            this, &DAAgentDockWidget::onUserMessageReceived);
    // web 就绪握手：flush 当前态（i18n/busy/model/tokenStats）
    connect(d->mChannel, &DAAgentWebChannel::webReady,
            this, &DAAgentDockWidget::onWebReady);
    // web 输入区 Stop 按钮：直达 C++ 终止流程（替代旧原生 m_sendButton 分流）
    connect(d->mChannel, &DAAgentWebChannel::stopRequested,
            this, &DAAgentDockWidget::onStopClicked);
}

/**
 * @brief web 输入区发送消息（C++ 编排：渲染用户气泡 + 向外发消息）
 * @param text 用户输入的消息文本
 */
void DAAgentDockWidget::onUserMessageReceived(const QString& text)
{
    DA_D(d);
    // C++ 仍是编排者：JS 已清框并调 chatBridge.onUserMessage(text)，此槽负责
    // 渲染用户气泡 + 向外发消息。与旧 onSendClicked 同构（文本来源从 QTextEdit 改为 JS）。
    if (d->mAgentBusy) {
        return;  // 忙碌时不发送（web 按钮此时为 Stop，理论不会触发；防御）
    }
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    d->mChannel->appendUserMessage(trimmed);  // C++ 渲染用户气泡（单一权威）
    emit sendMessageRequested(trimmed);
}

/**
 * @brief web 侧就绪握手：注入静态 i18n 标签 + flush 当前态
 */
void DAAgentDockWidget::onWebReady()
{
    DA_D(d);
    // web 侧就绪：注入静态 i18n 标签 + flush 当前态，缓解 JS-ready 竞态
    // （agent 信号若在 chat.html 加载完成前触发，此处补推当前 busy/model/token）
    if (!d->mChannel) return;
    d->mChannel->setI18nLabels(QVariantMap{
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
    d->mChannel->setBusy(d->mAgentBusy);
    d->mChannel->setModel(formatModelLabel());
    if (d->mHasTokenStats) {
        d->mChannel->setTokenStats(formatTokenLabel(d->mLastTotalTokens, d->mLastContextWindow, d->mLastTokenSource),
                                  d->mLastInTokens, d->mLastOutTokens, d->mLastTotalTokens,
                                  d->mLastContextWindow, d->mLastTokenSource);
    }
    d->mChannel->focusInput();
}

/**
 * @brief Stop 按钮点击：定稿当前流式输出 + 发送 stopRequested 信号
 */
void DAAgentDockWidget::onStopClicked()
{
    DA_D(d);
    // 定稿当前流式输出中的 agent 消息 + 关闭工具分组，避免半截消息悬挂
    if (d->mChannel) {
        d->mChannel->onAgentStopped();
        // 停止过渡态：web 按钮禁用防重复点 + 状态 Stopping...，持续到 onAgentBusy(false)/Ready 恢复
        d->mChannel->setStopping();
    }
    emit stopRequested();
}

/**
 * @brief 用户选择答案后转发信号
 * @param answer 用户选择的答案
 */
void DAAgentDockWidget::onUserAnswer(const QString& answer)
{
    emit userAnswerSelected(answer);
}

/**
 * @brief 绘图引用超链接点击转发
 * @param href 超链接 href
 */
void DAAgentDockWidget::onFigureLink(const QString& href)
{
    emit figureLinkRequested(href);
}

/**
 * @brief 处理 Agent 流式 token 信号
 * @param token 当前 token 文本
 */
void DAAgentDockWidget::onAgentToken(const QString& token)
{
    DA_D(d);
    // MAJOR4: UI 侧切换守卫——切换期间丢弃旧会话残余 token，避免污染新聊天区
    if (d->mSwitching) return;
    if (d->mChannel) {
        d->mChannel->appendToken(token);
    }
}

/**
 * @brief 处理 Agent 消息完成信号
 * @param fullText 完整消息文本
 */
void DAAgentDockWidget::onAgentMessageComplete(const QString& fullText)
{
    DA_D(d);
    if (d->mSwitching) return;  // MAJOR4
    if (d->mChannel) {
        d->mChannel->finalizeAgentMessage(fullText);
    }
}

/**
 * @brief 处理 Agent 工具调用信号
 * @param toolName 工具名称
 * @param args 工具参数
 */
void DAAgentDockWidget::onAgentToolCall(const QString& toolName, const QJsonObject& args)
{
    DA_D(d);
    if (d->mSwitching) return;  // MAJOR4
    if (d->mChannel) {
        d->mChannel->appendToolCall(toolName, args);
    }
}

/**
 * @brief 处理 Agent 工具结果信号
 * @param toolName 工具名称
 * @param result 工具执行结果
 */
void DAAgentDockWidget::onAgentToolResult(const QString& toolName, const QJsonObject& result)
{
    DA_D(d);
    if (d->mSwitching) return;  // MAJOR4
    if (d->mChannel) {
        d->mChannel->appendToolResult(toolName, result);
    }
}

/**
 * @brief 处理 Agent 提问信号
 * @param text 问题文本
 * @param options 选项列表
 * @param multiSelect 是否允许多选
 */
void DAAgentDockWidget::onAgentQuestion(const QString& text, const QStringList& options, bool multiSelect)
{
    DA_D(d);
    if (d->mSwitching) return;  // MAJOR4
    if (d->mChannel) {
        d->mChannel->appendQuestion(text, options, multiSelect);
    }
}

/**
 * @brief 处理 Agent 错误信号
 * @param message 错误信息
 * @param errorType 错误类型（quota_exhausted/auth_error/...），空表示未知
 * @param detail 详细错误描述（如原始异常信息），可为空
 */
void DAAgentDockWidget::onAgentError(const QString& message, const QString& errorType, const QString& detail)
{
    DA_D(d);
    Q_UNUSED(detail);
    // switchSession 后若 load_session 失败走 agentError 而非 session_loaded，
    // 不复位 m_switching 会冻结后续渲染（守卫永真）——沿用现有逻辑
    d->mSwitching = false;
    // 根据 errorType 选择用户文案
    QString displayMessage = mapErrorMessage(message, errorType);
    // 调用 chat.js 渲染错误（standalone error card，由 plan-06 实现）
    if (d->mChannel) {
        d->mChannel->appendError(displayMessage, errorType);
    }
}

/**
 * @brief 处理 Agent 重试信号（LLM 调用指数退避期间）
 * @param attempt 当前重试次数
 * @param maxAttempts 最大重试次数
 * @param delayMs 本次退避延迟毫秒数
 * @param errorType 触发重试的错误类型
 * @param errorMessage 触发重试的错误消息
 */
void DAAgentDockWidget::onAgentRetrying(int attempt, int maxAttempts, int delayMs,
                                         const QString& errorType, const QString& errorMessage)
{
    DA_D(d);
    // 通过 WebChannel 调用 chat.js 的 showRetryStatus
    if (d->mChannel) {
        d->mChannel->showRetryStatus(attempt, maxAttempts, delayMs, errorType, errorMessage);
    }
}

/**
 * @brief 处理 Agent 就绪信号
 * @param model 模型名称
 */
void DAAgentDockWidget::onAgentReady(const QString& model)
{
    DA_D(d);
    d->mCurrentModel = model;
    d->mAgentBusy = false;
    if (d->mChannel) {
        d->mChannel->setBusy(false);             // 复位为 ready：按钮 Send + 输入启用 + 状态 Ready
        d->mChannel->setModel(formatModelLabel());  // 推送 "Model: <name>"
    }
}

/**
 * @brief 处理 Agent 忙碌状态信号
 * @param busy 是否忙碌
 */
void DAAgentDockWidget::onAgentBusy(bool busy)
{
    DA_D(d);
    d->mAgentBusy = busy;
    // busy 打包：JS 解释按钮 Send/Stop 切换 + 输入禁用 + 状态文案（thinking/ready）
    if (d->mChannel) {
        d->mChannel->setBusy(busy);
    }
}

// ===========================================================================
// plan-04: 会话栏按钮槽 + token UI 槽 + 辅助方法
// ===========================================================================

/**
 * @brief 新建会话按钮点击
 */
void DAAgentDockWidget::onNewSessionClicked()
{
    emit sessionCreateRequested();
}

/**
 * @brief 会话管理按钮点击，弹出管理对话框
 */
void DAAgentDockWidget::onSessionManagerClicked()
{
    DA_D(d);
    // 弹出会话管理对话框，操作经 signal→signal 直连转发到 DAAgentInterface
    DADialogAgentSessionManager dlg(d->mSessions, d->mCurrentSessionId, this);
    connect(&dlg, &DADialogAgentSessionManager::switchRequested,
            this, &DAAgentDockWidget::sessionSwitchRequested);
    connect(&dlg, &DADialogAgentSessionManager::renameRequested,
            this, &DAAgentDockWidget::sessionRenameRequested);
    connect(&dlg, &DADialogAgentSessionManager::deleteRequested,
            this, &DAAgentDockWidget::sessionDeleteRequested);
    dlg.exec();
    // 对话框关闭后 sessionListChanged 会从 Module 回灌权威状态刷新标题
}

/**
 * @brief 事件过滤器：m_titleLabel 尺寸变化时重新计算省略文本
 * @param obj 监听对象
 * @param ev 事件
 * @return 是否过滤事件
 */
bool DAAgentDockWidget::eventFilter(QObject* obj, QEvent* ev)
{
    DA_D(d);
    // m_titleLabel 尺寸变化 → 重新计算省略文本（标题过长右端 …）
    // （输入区/状态栏/token 明细已迁 web，eventFilter 只剩会话栏标题省略）
    if (obj == d->mTitleLabel && ev->type() == QEvent::Resize) {
        updateTitleLabel();
        return false;
    }
    return QWidget::eventFilter(obj, ev);
}

/**
 * @brief 处理 token 使用量更新（契约2：5 参含 contextWindow 与 source）
 * @param inputTokens 输入 token
 * @param outputTokens 输出 token
 * @param totalTokens 总 token
 * @param contextWindow 上下文窗口大小
 * @param source 来源（tiktoken / usage_metadata）
 */
void DAAgentDockWidget::onAgentUsage(int inputTokens, int outputTokens,
                                     int totalTokens, int contextWindow,
                                     const QString& source)
{
    DA_D(d);
    // 契约2: 5 参含 contextWindow 与 source。一期不做 system/tools/history/current 四分类估算。
    // 缓存最近一次 usage：web 未就绪时丢失的推送，onWebReady 重推。
    d->mLastInTokens = inputTokens;
    d->mLastOutTokens = outputTokens;
    d->mLastTotalTokens = totalTokens;
    d->mLastContextWindow = contextWindow;
    d->mLastTokenSource = source;
    d->mHasTokenStats = true;
    // streaming_estimate 期间显示 ~ 前缀，表示是流式估算值而非权威统计；
    // 真实 usage 到达后（source 为 agent/summary）前缀消失。
    if (d->mChannel) {
        d->mChannel->setTokenStats(formatTokenLabel(totalTokens, contextWindow, source),
                                  inputTokens, outputTokens, totalTokens,
                                  contextWindow, source);
    }
}

/**
 * @brief Python load_session 重建完成，解除 UI 切换守卫
 * @param sessionId 会话 ID
 */
void DAAgentDockWidget::onAgentSessionLoaded(const QString& sessionId)
{
    DA_D(d);
    Q_UNUSED(sessionId);
    // Python load_session 重建完成，解除 UI 切换守卫（MAJOR4）
    d->mSwitching = false;
    // 重新断言当前 busy 态（若非忙则状态文案置 Ready），消除可能的 Stopping 残留
    if (d->mChannel) {
        d->mChannel->setBusy(d->mAgentBusy);
    }
}

/**
 * @brief 会话切换完成（Module::sessionSwitched），UI 侧守卫 + clearChat + loadHistory
 * @param sessionId 新会话 ID
 * @param allRecords 新会话完整 JSONL 记录
 */
void DAAgentDockWidget::onSessionSwitched(const QString& sessionId,
                                          const QVector<QJsonObject>& allRecords)
{
    DA_D(d);
    // MAJOR4: UI 侧切换守卫——先清空，进行中的 token 经 m_switching 丢弃
    d->mSwitching = true;
    if (d->mChannel) {
        d->mChannel->clearChat();
        d->mChannel->loadHistory(allRecords);  // 重放新会话 UI（C++ 合并后事件，见 WebChannel::loadHistory）
    }
    d->mCurrentSessionId = sessionId;
    updateTitleLabel();
}

/**
 * @brief 会话列表变化（契约3：payload 每元素 QVariantMap{id,title}），直接填充下拉
 * @param sessions 会话列表 payload
 */
void DAAgentDockWidget::onSessionListChanged(QVariantList sessions)
{
    DA_D(d);
    // 契约3: 缓存 payload（含 updatedAt/messageCount 元信息），刷新标题
    d->mSessions = sessions;
    updateTitleLabel();
}

/**
 * @brief 新会话创建（newSession 路径），清空聊天 + 复位守卫 + 复位 token 控件
 * @param sessionId 新会话 ID
 */
void DAAgentDockWidget::onSessionCreated(const QString& sessionId)
{
    DA_D(d);
    // MAJOR7: 仅 newSession 路径触发本槽——新会话清空聊天 + 复位守卫。
    // 标题刷新由 sessionListChanged(payload) 信号驱动。
    // Bug2 修复：新会话无 usage，复位 token 控件避免抛留上一会话数值。
    d->mSwitching = false;
    d->mCurrentSessionId = sessionId;
    d->mHasTokenStats = false;  // 新会话无 usage，复位缓存
    if (d->mChannel) {
        d->mChannel->clearChat();
        d->mChannel->resetTokenStats();  // 复位 web 侧 token 标签 + popover
        d->mChannel->focusInput();       // 新会话聚焦输入框
    }
    updateTitleLabel();
}

/**
 * @brief 当前无活跃会话（启动/打开工程后始终全新对话，不自动恢复上次会话）
 *
 * 清空残留聊天区、复位 token 控件、下拉不选中、解除切换守卫。
 */
void DAAgentDockWidget::onSessionCleared()
{
    DA_D(d);
    // 启动/打开工程后始终全新对话，不自动恢复上次会话。
    // 清空残留聊天区、复位 token 控件、清空标题、解除切换守卫。
    d->mSwitching = false;
    d->mCurrentSessionId.clear();
    d->mHasTokenStats = false;
    if (d->mChannel) {
        d->mChannel->clearChat();
        d->mChannel->resetTokenStats();
        d->mChannel->focusInput();
    }
    updateTitleLabel();
}

// ---- 辅助方法 ----

/**
 * @brief 用 sessionListChanged payload 刷新会话缓存并更新标题
 *
 * 按当前会话 ID 在缓存中查标题并更新标题标签（过长右端省略 + tooltip 全文）
 */
void DAAgentDockWidget::updateTitleLabel()
{
    DA_D(d);
    // 按 m_currentSessionId 在缓存中查标题；空标题显示「(untitled)」
    if (!d->mTitleLabel) return;
    QString fullTitle;
    if (!d->mCurrentSessionId.isEmpty()) {
        for (int i = 0; i < d->mSessions.size(); ++i) {
            QVariantMap vm = d->mSessions.at(i).toMap();
            if (vm.value("id").toString() == d->mCurrentSessionId) {
                fullTitle = vm.value("title").toString();
                break;
            }
        }
        if (fullTitle.isEmpty()) {
            fullTitle = tr("(untitled)");  // cn:（未命名）
        }
    }
    d->mCurrentSessionFullTitle = fullTitle;
    // tooltip 显示完整标题（空标题不弹 tooltip）
    d->mTitleLabel->setToolTip(fullTitle);
    // 按当前可用宽度省略渲染（右端 …）
    int w = d->mTitleLabel->width();
    if (w <= 0) {
        // 尚未布局完成，直接放全文，resize 事件触发时会重新省略
        d->mTitleLabel->setText(fullTitle);
        return;
    }
    // 减去内边距避免 … 紧贴右边缘
    const int pad = 12;
    QString shown = d->mTitleLabel->fontMetrics().elidedText(
        fullTitle, Qt::ElideRight, qMax(0, w - pad));
    d->mTitleLabel->setText(shown);
}

/**
 * @brief 格式化模型标签串：空模型返回 "Model: -"，非空返回 "Model: <name>"（已 tr 翻译）
 * @return 格式化后的模型标签串
 */
QString DAAgentDockWidget::formatModelLabel() const
{
    DA_DC(d);
    // 返回模型标签串：空模型 "Model: -"，非空 "Model: <name>"（已 tr 翻译）。
    // 省略由 web 侧 CSS text-overflow:ellipsis 处理，tooltip 由 JS setModel 设置。
    if (d->mCurrentModel.isEmpty()) {
        return tr("Model: -");  // cn:模型：-
    }
    return tr("Model: %1").arg(d->mCurrentModel);  // cn:模型：%1
}

/**
 * @brief 格式化 token 计量串：streaming_estimate 带 ~ 前缀，否则 "tokens: N / window"
 * @param totalTokens 总 token
 * @param contextWindow 上下文窗口大小
 * @param source 来源（tiktoken / usage_metadata / streaming_estimate）
 * @return 格式化后的 token 计量串
 */
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

/**
 * @brief 根据 errorType 映射错误消息为翻译后的用户文案（plan-03 step8）
 * @param original 原始错误消息
 * @param errorType 错误类型
 * @return 翻译后的用户文案
 */
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
