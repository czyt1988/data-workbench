// DAAgentSessionChatWidget.cpp
#include "DAAgentSessionChatWidget.h"
#include "DAAgentWebChannel.h"
#include "DAAgentChatWebPage.h"
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWebChannel>
#include <QQueue>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

namespace DA
{

class DAAgentSessionChatWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAAgentSessionChatWidget)
public:
    explicit PrivateData(DAAgentSessionChatWidget* p);

    QWebEngineView* mWebView = nullptr;
    DAAgentChatWebPage* mPage = nullptr;  ///< 自定义页面（外部链接拦截），父对象为 mWebView
    DAAgentWebChannel* mChannel = nullptr;
    QString mSessionId;  ///< 本视图归属会话（空 = unbound 新会话草稿区）
    bool mAgentBusy = false;
    bool mAgentStarting = false;   ///< agent 子进程启动中，UI 显示"启动中"
    bool mWebReady = false;        ///< chat.js 握手完成（此前 callJS 会丢失，需缓存）
    // ---- 历史重放缓存：视图创建早于 web 就绪时，loadHistory/clearChat 暂存 ----
    bool mHasPendingHistory = false;
    bool mPendingClearBeforeLoad = false;
    QVector<QJsonObject> mPendingHistoryRecords;
    // ---- 内容型事件缓存：web 未就绪期间的增量事件（流式 token/工具卡/问题卡/
    //      审批卡等），onWebReady 在历史重放之后按序 flush ----
    QQueue<std::function<void()>> mPendingEvents;
    // ---- 模型选择：web 两级选择器状态（缓存供 onWebReady flush） ----
    QVariantList mAvailableModels;  ///< 缓存可用模型列表（flat {provider,model,...}）
    QString mCurrentProvider;       ///< 当前激活供应商
    QString mCurrentModel;          ///< 当前模型名称
    // ---- token 统计缓存：web 未就绪时丢失的推送，onWebReady 重推 ----
    int mLastInTokens = 0;
    int mLastOutTokens = 0;
    int mLastTotalTokens = 0;
    int mLastContextWindow = 0;
    QString mLastTokenSource;
    bool mHasTokenStats = false;
    // ---- 权限层：web 未就绪时缓存，onWebReady flush ----
    QString mCurrentPermissionMode = QStringLiteral("yolo");  ///< 当前权限模式
    bool mStartupYoloConfirmPending = false;  ///< A13 启动确认卡待弹（宿主保证仅一次）
    // ---- 跨工程会话提示条（决策点 5）：web 未就绪时缓存，onWebReady flush ----
    int mForeignSessionCount = 0;
    // ---- 右键菜单（仅 Copy/Paste/Select All，构建一次复用） ----
    QMenu* mContextMenu = nullptr;
    QAction* mCopyAction = nullptr;
};

DAAgentSessionChatWidget::PrivateData::PrivateData(DAAgentSessionChatWidget* p) : q_ptr(p)
{
}

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DAAgentSessionChatWidget::DAAgentSessionChatWidget(QWidget* parent)
    : QWidget(parent)
    , DA_PIMPL_CONSTRUCT
{
    setupUI();
    setupWebChannel();
}

/**
 * @brief 析构函数
 */
DAAgentSessionChatWidget::~DAAgentSessionChatWidget()
{
}

/**
 * @brief 获取本视图绑定的会话 ID
 * @return 会话 ID（空 = unbound）
 */
QString DAAgentSessionChatWidget::sessionId() const
{
    DA_DC(d);
    return d->mSessionId;
}

/**
 * @brief 绑定会话（unbound → bound）
 * @param sessionId 会话 ID
 *
 * 仅记录归属，不清空聊天内容——绑定发生在首条用户消息渲染之后（懒建会话路径），
 * 清空会擦掉刚显示的用户气泡（旧 MAJOR3 语义在多视图下的等价物）。
 */
void DAAgentSessionChatWidget::setSessionId(const QString& sessionId)
{
    DA_D(d);
    d->mSessionId = sessionId;
}

/**
 * @brief 是否已绑定会话
 */
bool DAAgentSessionChatWidget::isBound() const
{
    DA_DC(d);
    return !d->mSessionId.isEmpty();
}

/**
 * @brief 重放会话历史（web 未就绪时缓存，onWebReady 后 flush）
 * @param records 会话完整 JSONL 记录
 */
void DAAgentSessionChatWidget::loadHistory(const QVector<QJsonObject>& records)
{
    DA_D(d);
    // 决策点 4（审计问题 3）：落盘 error 记录的 message 是原始文案，重放前经
    // mapErrorMessage 预映射为用户文案——与实时路径（onAgentError 映射后才调
    // appendError）保持同一展示语义。C++ 侧源发的 tr 文案映射为幂等
    QVector<QJsonObject> mapped = records;
    for (int i = 0; i < mapped.size(); ++i) {
        QJsonObject& rec = mapped[i];
        if (rec.value("type").toString() != QLatin1String("error")) {
            continue;
        }
        QJsonObject msg = rec.value("message").toObject();
        msg["message"] = mapErrorMessage(msg.value("message").toString(),
                                         msg.value("error_type").toString());
        rec["message"] = msg;
    }
    if (d->mWebReady) {
        if (d->mChannel) {
            d->mChannel->clearChat();
            d->mChannel->loadHistory(mapped);
        }
        return;
    }
    // web 未就绪：缓存（覆盖旧缓存，clearChat 先于 loadHistory 执行）
    d->mHasPendingHistory      = true;
    d->mPendingClearBeforeLoad = true;
    d->mPendingHistoryRecords  = mapped;
}

/**
 * @brief 清空聊天区（web 未就绪时缓存，onWebReady 后 flush）
 */
void DAAgentSessionChatWidget::clearChat()
{
    DA_D(d);
    if (d->mWebReady) {
        if (d->mChannel) {
            d->mChannel->clearChat();
        }
        return;
    }
    d->mHasPendingHistory      = true;
    d->mPendingClearBeforeLoad = true;
    d->mPendingHistoryRecords.clear();
}

/**
 * @brief 注入可用模型列表（宿主广播；缓存供 onWebReady flush）
 */
void DAAgentSessionChatWidget::setAvailableModels(const QVariantList& models)
{
    DA_D(d);
    d->mAvailableModels = models;
    if (d->mChannel) {
        d->mChannel->setAvailableModels(models);
    }
}

/**
 * @brief 注入激活供应商+模型（宿主广播；缓存供 onWebReady flush）
 */
void DAAgentSessionChatWidget::setActiveModel(const QString& provider, const QString& model)
{
    DA_D(d);
    d->mCurrentProvider = provider;
    d->mCurrentModel    = model;
    if (d->mChannel) {
        d->mChannel->setActiveModel(provider, model);
    }
}

/**
 * @brief 注入当前权限模式（宿主广播；缓存供 onWebReady flush）
 */
void DAAgentSessionChatWidget::setPermissionMode(const QString& mode)
{
    DA_D(d);
    d->mCurrentPermissionMode = mode;
    if (d->mChannel) {
        d->mChannel->setPermissionMode(mode);
    }
}

/**
 * @brief 设置 A13 启动 yolo 确认卡待弹标志
 * @param pending true=本视图 web 就绪后弹出一次启动确认卡
 */
void DAAgentSessionChatWidget::setStartupYoloConfirmPending(bool pending)
{
    DA_D(d);
    d->mStartupYoloConfirmPending = pending;
}

/**
 * @brief 跨工程会话提示条（决策点 5 方案 c，审计问题 18）
 * @param count 跨工程存活会话数（>0 显示提示条，0 隐藏）
 *
 * web 未就绪时缓存计数，onWebReady 后 flush（提示条可能在视图创建前出现）。
 */
void DAAgentSessionChatWidget::setForeignSessionsBanner(int count)
{
    DA_D(d);
    d->mForeignSessionCount = count;
    if (d->mChannel) {
        d->mChannel->showForeignSessionsBanner(count);
    }
}

/**
 * @brief 内容型事件缓存执行
 * @param fn 渲染动作（捕获事件参数的 lambda）
 *
 * web 就绪立即执行；未就绪入队，onWebReady 在历史重放之后按序 flush。
 * 不可用于状态型事件（busy/starting/ready/usage——它们经成员缓存 + onWebReady
 * 补推机制覆盖，入队反而会与最终态推送乱序）。
 */
void DAAgentSessionChatWidget::runOrDefer(std::function<void()> fn)
{
    DA_D(d);
    if (d->mWebReady) {
        fn();
    } else {
        d->mPendingEvents.enqueue(fn);
    }
}

/**
 * @brief 初始化 UI 界面
 */
void DAAgentSessionChatWidget::setupUI()
{
    DA_D(d);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    // QWebEngineView 占满（chat.html 内含 聊天区+状态栏+输入区，一个连续 web 表面）
    d->mWebView = new QWebEngineView(this);
    d->mWebView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 自定义 Page：外部超链接交系统浏览器打开（禁止覆盖聊天内容）+ 新窗口中转
    d->mPage = new DAAgentChatWebPage(d->mWebView);
    d->mWebView->setPage(d->mPage);
    // 关闭 WebEngine 默认右键菜单（Back/Forward/Reload 等）：Chromium 异步回调
    // contextMenuRequested 会直接虚调用 view->contextMenuEvent() 弹默认菜单，
    // NoContextMenu 使回调直接返回（聊天视图无需右键菜单）
    d->mWebView->setContextMenuPolicy(Qt::NoContextMenu);
    // 说明：QWebEngineSettings 并不存在 DeveloperToolsEnabled 属性（Qt5/Qt6 均无，旧注释有误）。
    // 开启开发者工具需在程序启动前设置环境变量 QTWEBENGINE_REMOTE_DEBUGGING=<端口>，
    // 再用 Chrome 访问 http://localhost:<端口>；默认保持关闭
    mainLayout->addWidget(d->mWebView, 1);
    d->mWebView->setUrl(QUrl(QStringLiteral("qrc:///DAAgent/chat.html")));

    // 构建右键菜单（仅一次，后续右键复用）
    buildContextMenu();
}

/**
 * @brief 构建右键菜单（仅 Copy/Paste/Select All）
 *
 * webview 的 WebEngine 默认菜单（Back/Forward/Reload 等）已由
 * NoContextMenu 策略关闭；聊天视图保留最小编辑能力，输入框需要 Paste、
 * 消息文本需要 Copy/Select All。
 */
void DAAgentSessionChatWidget::buildContextMenu()
{
    DA_D(d);
    d->mContextMenu = new QMenu(this);

    // 复制
    d->mCopyAction = d->mContextMenu->addAction(tr("Copy"));  // cn:复制
    connect(d->mCopyAction, &QAction::triggered, this, [this]() {
        DA_D(d);
        if (d->mPage) {
            d->mPage->triggerAction(QWebEnginePage::Copy);
        }
    });

    // 粘贴（聊天输入框）
    QAction* pasteAct = d->mContextMenu->addAction(tr("Paste"));  // cn:粘贴
    connect(pasteAct, &QAction::triggered, this, [this]() {
        DA_D(d);
        if (d->mPage) {
            d->mPage->triggerAction(QWebEnginePage::Paste);
        }
    });

    // 全选
    QAction* selectAllAct = d->mContextMenu->addAction(tr("Select All"));  // cn:全选
    connect(selectAllAct, &QAction::triggered, this, [this]() {
        DA_D(d);
        if (d->mPage) {
            d->mPage->triggerAction(QWebEnginePage::SelectAll);
        }
    });
}

/**
 * @brief webview 右键事件处理：弹最小编辑菜单
 *
 * webview 的 contextMenuPolicy 为 NoContextMenu（见 setupUI），原生右键
 * 事件传播到本容器在此处理；Chromium 异步回调路径已被该策略关闭，
 * 不会再弹出 WebEngine 默认菜单（Back/Forward/Reload 等）。
 * @param event 右键事件
 */
void DAAgentSessionChatWidget::contextMenuEvent(QContextMenuEvent* event)
{
    DA_D(d);
    if (!d->mContextMenu) {
        return;
    }
    // Copy 仅在 web 侧有选中内容时可用
    d->mCopyAction->setEnabled(d->mPage && d->mPage->action(QWebEnginePage::Copy)->isEnabled());
    d->mContextMenu->exec(event->globalPos());
}

/**
 * @brief 初始化 WebChannel（每视图独立 channel 实例，注册名与 chat.js 一致）
 */
void DAAgentSessionChatWidget::setupWebChannel()
{
    DA_D(d);
    d->mChannel = new DAAgentWebChannel(d->mWebView, this);
    QWebChannel* webChannel = new QWebChannel(this);
    // JS 侧通过 channel.objects.chatBridge 访问，名字必须与 chat.js 一致
    webChannel->registerObject(QStringLiteral("chatBridge"), d->mChannel);
    d->mWebView->page()->setWebChannel(webChannel);

    // 用户答案 → 本视图信号转发（宿主负责"交互即激活"后转接口层）
    connect(d->mChannel, &DAAgentWebChannel::userAnswerSelected,
            this, &DAAgentSessionChatWidget::onUserAnswer);
    // 本地跳转超链接点击：chat.js 拦截 da-<kind>: 链接 → onLinkActivated → 信号转发
    connect(d->mChannel, &DAAgentWebChannel::linkActivated,
            this, &DAAgentSessionChatWidget::onLinkActivated);
    // web 输入区发送：chat.js onUserMessage → userMessageSent → C++ 编排（appendUserMessage + emit）
    connect(d->mChannel, &DAAgentWebChannel::userMessageSent,
            this, &DAAgentSessionChatWidget::onUserMessageReceived);
    // web 就绪握手：flush 当前态（i18n/busy/model/tokenStats/pendingHistory）
    connect(d->mChannel, &DAAgentWebChannel::webReady,
            this, &DAAgentSessionChatWidget::onWebReady);
    // web 输入区 Stop 按钮：直达 C++ 终止流程
    connect(d->mChannel, &DAAgentWebChannel::stopRequested,
            this, &DAAgentSessionChatWidget::onStopClicked);
    // web 两级模型选择器：用户选定供应商+模型 → activeModelChangeRequested
    connect(d->mChannel, &DAAgentWebChannel::modelChangeRequested,
            this, &DAAgentSessionChatWidget::onModelSelect);
    // ---- 权限层（permission-layer P1）：web 用户操作 → 接口 ----
    connect(d->mChannel, &DAAgentWebChannel::permissionModeChangeRequested,
            this, &DAAgentSessionChatWidget::onPermissionModeSelect);
    connect(d->mChannel, &DAAgentWebChannel::toolApprovalDecision,
            this, &DAAgentSessionChatWidget::onToolApprovalDecision);
    connect(d->mChannel, &DAAgentWebChannel::startupModeConfirmResponse,
            this, &DAAgentSessionChatWidget::onStartupModeConfirmResponse);
    // 跨工程会话提示条点击（决策点 5 方案 c）：透传宿主 → 打开会话管理对话框
    //（"全部工程"视图查看并停止旧工程后台会话）
    connect(d->mChannel, &DAAgentWebChannel::foreignBannerClicked,
            this, &DAAgentSessionChatWidget::foreignBannerClicked);
}

/**
 * @brief web 输入区发送消息（C++ 编排：渲染用户气泡 + 向外发消息）
 * @param text 用户输入的消息文本
 */
void DAAgentSessionChatWidget::onUserMessageReceived(const QString& text)
{
    DA_D(d);
    // C++ 仍是编排者：JS 已清框并调 chatBridge.onUserMessage(text)，此槽负责
    // 渲染用户气泡 + 向外发消息（宿主负责"交互即激活"后转发 sendMessage）
    if (d->mAgentBusy || d->mAgentStarting) {
        return;  // 忙碌/启动中时不发送（按钮此时禁用，理论不会触发；防御）
    }
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    d->mChannel->appendUserMessage(trimmed);  // C++ 渲染用户气泡（单一权威）
    emit sendMessageRequested(trimmed);
}

/**
 * @brief web 侧就绪握手：注入静态 i18n 标签 + flush 当前态 + 补发缓存历史
 */
void DAAgentSessionChatWidget::onWebReady()
{
    DA_D(d);
    // web 侧就绪：注入静态 i18n 标签 + flush 当前态，缓解 JS-ready 竞态
    // （agent 信号若在 chat.html 加载完成前触发，此处补推当前 busy/model/token）
    if (!d->mChannel) return;
    d->mWebReady = true;
    d->mChannel->setI18nLabels(QVariantMap{
        {"send", tr("Send")},                       // cn:发送
        {"stop", tr("Stop")},                        // cn:终止
        {"ready", tr("Ready")},                      // cn:就绪
        {"starting", tr("Agent starting...")},       // cn:Agent 启动中...
        {"thinking", tr("Agent thinking...")},       // cn:Agent 思考中...
        {"stopping", tr("Stopping...")},             // cn:终止中...
        {"inputPlaceholder", tr("Type a message...")},  // cn:输入消息...
        {"tokenEmpty", tr("tokens: -")},             // cn:token: -
        {"popoverInput", tr("input: %1")},           // cn:输入：%1
        {"popoverOutput", tr("output: %1")},         // cn:输出：%1
        {"popoverTotal", tr("total: %1")},           // cn:总计：%1
        {"popoverWindow", tr("window: %1")},         // cn:窗口：%1
        {"popoverSource", tr("source: %1")},          // cn:来源：%1
        {"popoverSourceUnknown", tr("unknown")},      // cn:未知
        {"modelEmpty", tr("No model")},               // cn:无模型
        {"modelSelectTip", tr("Select LLM model")},   // cn:选择 LLM 模型
        {"modelProvidersTitle", tr("Providers")},     // cn:供应商
        {"modelBack", tr("Back")},                    // cn:返回
        {"errorDetails", tr("Details")},             // cn:详细信息
        {"errorCopy", tr("Copy")},                   // cn:复制
        {"errorCopied", tr("Copied")},               // cn:已复制
        {"errorTruncated", tr("[truncated]")},       // cn:[已截断]
        // —— 权限模式选择器（permission-layer P1）——
        {"modeSelectTip", tr("Permission mode")},    // cn:权限模式
        {"modeYolo", tr("Full Auto")},               // cn:全自动
        {"modeAuto", tr("Auto")},                    // cn:自动
        {"modeManual", tr("Ask Every Time")},        // cn:每次询问
        {"modeYoloTip", tr("Run everything without asking (system directories still blocked)")},  // cn:全部直接执行不再询问（系统目录仍拦截）
        {"modeAutoTip", tr("Reads and chart edits pass; file writes and code execution judged by rules")},  // cn:读取与图表编辑放行；文件写入与代码执行按规则判定
        {"modeManualTip", tr("File writes and code execution need approval every time")},  // cn:文件写入与代码执行每次都需批准
        {"modeYoloConfirm", tr("Switch to Full Auto mode? Code execution and file writes will no longer ask for confirmation.")},  // cn:切换到全自动模式？代码执行与文件写入将不再请求确认。
        {"modeYoloConfirmOk", tr("Switch")},         // cn:切换
        {"modeYoloConfirmCancel", tr("Cancel")},     // cn:取消
        // —— 工具审批卡（permission-layer P1）——
        {"approvalNeeds", tr("needs your approval")},  // cn:需要你的批准
        {"approvalApprove", tr("Approve")},          // cn:批准
        {"approvalDeny", tr("Deny")},                // cn:拒绝
        // 审计 L10：该译文经 web textContent 设置，Qt 助记符 "&&" 会按字面
        // 显示为双 and 符——推送前剥离为单 &（web 无助记符语义）
        {"approvalApproveRemember", QString(tr("Approve && remember for this session"))
                                        .replace(QLatin1String("&&"), QLatin1String("&"))},  // cn:批准并本会话记住
        {"approvalApproved", tr("Approved")},        // cn:已批准
        {"approvalDenied", tr("Denied")},            // cn:已拒绝
        {"approvalApprovedRemembered", tr("Approved (remembered for this session)")},  // cn:已批准（本会话已记住）
        {"approvalCodeMoreLines", tr("%1 more lines")},  // cn:还有 %1 行
        {"approvalFromSubagent", tr("From subagent: %1")},  // cn:来自子 Agent：%1
        // —— 工具排队状态（决策点 2 ③，审计问题 12）——
        {"toolQueued", tr("queued")},     // cn:排队中
        {"toolRunning", tr("running")},   // cn:运行中
        // —— 工具结果截断（审计问题 29）——
        {"toolResultTruncated", tr("result truncated")},  // cn:结果已截断
        // —— 问题卡提交失败提示（审计 L9）——
        {"answerSendFailed", tr("Answer not sent, please retry")},  // cn:回答未发送，请重试
        // —— 重放问题卡标签（审计 L10）：与实时路径 appendQuestion 的 tr 同源，
        // 历史重放不再恒显英文 fallback（源文本一致，翻译条目复用）
        {"questionSubmit", tr("Submit")},                          // cn:提交
        {"questionCustomPlaceholder", tr("Type your own answer...")},  // cn:输入自定义回答...
        // —— 跨工程会话提示条（决策点 5 方案 c，审计问题 18）——
        {"foreignBannerText", tr("%1 session(s) from the previous project are still running in the background")},  // cn:%1 个上一工程的会话仍在后台运行
        {"foreignBannerTip", tr("Click to view and stop these sessions")},  // cn:点击查看并停止这些会话
        // —— 子 agent 进度卡片（subagent-phase1 C）——
        {"subagentTaskCount", tr("%1 subagent task(s)")},  // cn:%1 个子 Agent 任务
        {"subagentProgress", tr("%1/%2 done")},             // cn:%1/%2 已完成
        {"subagentCompleted", tr("completed")},             // cn:已完成
        {"subagentQueued", tr("queued")},                   // cn:排队中
        {"subagentRunning", tr("running")},                 // cn:运行中
        {"subagentDone", tr("done")},                       // cn:完成
        {"subagentFailed", tr("failed")},                   // cn:失败
        {"subagentTimeout", tr("timeout")},                 // cn:超时
        {"subagentStopped", tr("stopped")},                 // cn:已停止
        // —— 历史分段懒加载（超长会话只渲染尾部段，顶部哨兵加载更早）——
        {"loadEarlier", tr("Load earlier messages")}        // cn:加载更早消息
    });
    // 启动中优先推 starting 态，缓解 JS-ready 竞态——agent 信号若在 chat.html 加载
    // 完成前触发，此处补推当前 starting/busy 态
    if (d->mAgentStarting) {
        d->mChannel->setStarting();
    } else {
        d->mChannel->setBusy(d->mAgentBusy);
    }
    // 补发缓存的历史重放（视图创建早于 web 就绪：clearChat + loadHistory）
    if (d->mHasPendingHistory) {
        d->mHasPendingHistory = false;
        if (d->mPendingClearBeforeLoad) {
            d->mChannel->clearChat();
            d->mPendingClearBeforeLoad = false;
        }
        if (!d->mPendingHistoryRecords.isEmpty()) {
            d->mChannel->loadHistory(d->mPendingHistoryRecords);
        }
        d->mPendingHistoryRecords.clear();
    }
    // 补发缓存的内容型事件（视图创建于会话运行中：挂起重发的问题卡/审批卡、
    // 创建之后的流式 token 增量等，按到达顺序渲染在历史之后）
    while (!d->mPendingEvents.isEmpty()) {
        std::function<void()> fn = d->mPendingEvents.dequeue();
        fn();
    }
    // 推送可用模型列表 + 激活供应商/模型给 web 两级选择器
    d->mChannel->setAvailableModels(d->mAvailableModels);
    d->mChannel->setActiveModel(d->mCurrentProvider, d->mCurrentModel);
    // 权限层：推送当前权限模式给 web 模式选择器；显式设置的 yolo 启动弹一次确认卡（A13），
    // 默认值（未显式设置）静默进入全自动不弹卡（待弹标志由宿主控制，保证每次启动仅一次）
    d->mChannel->setPermissionMode(d->mCurrentPermissionMode);
    if (d->mCurrentPermissionMode == QLatin1String("yolo") && d->mStartupYoloConfirmPending) {
        d->mStartupYoloConfirmPending = false;
        d->mChannel->appendStartupYoloConfirm(
            tr("The permission mode is Full Auto from last session. Code execution and file writes will run without asking. Keep Full Auto mode?"),
            //cn:上次会话留在全自动权限模式。代码执行与文件写入将不再询问直接执行。是否保持全自动模式？
            tr("Keep Full Auto"),    //cn:保持全自动
            tr("Switch to Auto"));   //cn:切换为自动
        emit startupYoloConfirmShown();
    }
    if (d->mHasTokenStats) {
        d->mChannel->setTokenStats(formatTokenLabel(d->mLastTotalTokens, d->mLastContextWindow, d->mLastTokenSource),
                                   d->mLastInTokens, d->mLastOutTokens, d->mLastTotalTokens,
                                   d->mLastContextWindow, d->mLastTokenSource);
    }
    // 跨工程会话提示条（决策点 5）：视图创建前收到的计数在此补推
    if (d->mForeignSessionCount > 0) {
        d->mChannel->showForeignSessionsBanner(d->mForeignSessionCount);
    }
    d->mChannel->focusInput();
}

/**
 * @brief Stop 按钮点击：定稿当前流式输出 + 发送 stopRequested 信号
 */
void DAAgentSessionChatWidget::onStopClicked()
{
    DA_D(d);
    // 审计 L8：空闲（无 busy/starting）时不进 Stopping 过渡态——旧实现无条件
    // setStopping（按钮+输入禁用），完成竞态/崩溃恢复窗口点 Stop 时若 Module
    // 侧空转且无外部信号拯救，UI 永久冻结（空闲时按钮理论禁用，此为防御；
    // Module::stop() 空转路径已同步补发 busy(false) 兜底）
    if (!d->mAgentBusy && !d->mAgentStarting) {
        return;
    }
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
void DAAgentSessionChatWidget::onUserAnswer(const QString& answer)
{
    emit userAnswerSelected(answer);
}

/**
 * @brief 本地跳转超链接点击转发（协议无关，分发由上层 DAAgentLinkDispatcher 完成）
 * @param href 超链接 href
 */
void DAAgentSessionChatWidget::onLinkActivated(const QString& href)
{
    emit linkActivated(href);
}

// ===========================================================================
// 会话级事件槽（首参 sessionId，仅本会话渲染）
// ===========================================================================

/**
 * @brief 处理 Agent 流式 token 信号
 * @param sessionId 信号归属会话
 * @param token 当前 token 文本
 */
void DAAgentSessionChatWidget::onAgentToken(const QString& sessionId, const QString& token)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, token ]() {
        if (ch) {
            ch->appendToken(token);
        }
    });
}

/**
 * @brief 处理 Agent 消息完成信号
 * @param sessionId 信号归属会话
 * @param fullText 完整消息文本
 */
void DAAgentSessionChatWidget::onAgentMessageComplete(const QString& sessionId, const QString& fullText)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, fullText ]() {
        if (ch) {
            ch->finalizeAgentMessage(fullText);
        }
    });
}

/**
 * @brief 处理 Agent 工具调用信号
 * @param sessionId 信号归属会话
 * @param toolName 工具名称
 * @param args 工具参数
 */
void DAAgentSessionChatWidget::onAgentToolCall(const QString& sessionId, const QString& toolName, const QJsonObject& args)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, toolName, args ]() {
        if (ch) {
            ch->appendToolCall(toolName, args);
        }
    });
}

/**
 * @brief 处理工具排队状态（决策点 2 ③：全局执行队列排队可见）
 * @param sessionId 信号归属会话
 * @param toolName 工具名称
 * @param position 队列位置（1-based）；0=开始执行（恢复"运行中"）
 */
void DAAgentSessionChatWidget::onAgentToolQueued(const QString& sessionId, const QString& toolName, int position)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, toolName, position ]() {
        if (ch) {
            ch->markToolQueued(toolName, position);
        }
    });
}

/**
 * @brief 处理挂起问题卡作废（子进程退出/崩溃/用户 Stop/桥退役）
 * @param sessionId 信号归属会话
 *
 * 镜像 onToolApprovalDismissed 契约（审计问题 17）：通知 web 移除未回答的
 * 问题卡，防止用户对幽灵卡作答（答案经死桥发送必然蒸发）。
 */
void DAAgentSessionChatWidget::onQuestionDismissed(const QString& sessionId)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch ]() {
        if (ch) {
            ch->dismissQuestion();
        }
    });
}

/**
 * @brief 处理 Agent 工具结果信号
 * @param sessionId 信号归属会话
 * @param toolName 工具名称
 * @param result 工具执行结果
 */
void DAAgentSessionChatWidget::onAgentToolResult(const QString& sessionId, const QString& toolName,
                                                 const QJsonObject& result)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, toolName, result ]() {
        if (ch) {
            ch->appendToolResult(toolName, result);
        }
    });
}

/**
 * @brief 处理 Agent 提问信号
 * @param sessionId 信号归属会话
 * @param text 问题文本
 * @param options 选项列表
 * @param multiSelect 是否允许多选
 */
void DAAgentSessionChatWidget::onAgentQuestion(const QString& sessionId, const QString& text,
                                               const QStringList& options, bool multiSelect)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, text, options, multiSelect ]() {
        if (ch) {
            ch->appendQuestion(text, options, multiSelect);
        }
    });
}

/**
 * @brief 处理 Agent 错误信号
 * @param sessionId 信号归属会话
 * @param message 错误信息
 * @param errorType 错误类型（quota_exhausted/auth_error/...），空表示未知
 * @param detail 详细错误描述（如原始异常信息），可为空
 */
void DAAgentSessionChatWidget::onAgentError(const QString& sessionId, const QString& message,
                                            const QString& errorType, const QString& detail)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    // 根据 errorType 选择用户文案
    const QString displayMessage = mapErrorMessage(message, errorType);
    // 调用 chat.js 渲染错误卡片：主文案为映射后的用户文案，detail（原始异常文本）
    // 经可折叠面板展示（默认折叠 + 截断，避免长 traceback 占满对话界面），并提供
    // 复制按钮一键复制完整异常信息。
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, displayMessage, errorType, detail ]() {
        if (ch) {
            ch->appendError(displayMessage, errorType, detail);
        }
    });
}

/**
 * @brief 处理 Agent 重试信号（LLM 调用指数退避期间）
 * @param sessionId 信号归属会话
 * @param attempt 当前重试次数
 * @param maxAttempts 最大重试次数
 * @param delayMs 本次退避延迟毫秒数
 * @param errorType 触发重试的错误类型
 * @param errorMessage 触发重试的错误消息
 */
void DAAgentSessionChatWidget::onAgentRetrying(const QString& sessionId, int attempt, int maxAttempts, int delayMs,
                                               const QString& errorType, const QString& errorMessage)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, attempt, maxAttempts, delayMs, errorType, errorMessage ]() {
        if (ch) {
            ch->showRetryStatus(attempt, maxAttempts, delayMs, errorType, errorMessage);
        }
    });
}

/**
 * @brief 处理 Agent 启动信号（预启动/懒启动/崩溃重启均触发）
 * @param sessionId 信号归属会话
 *
 * UI 进入"启动中"过渡态：按钮+输入禁用、状态"启动中"。
 * 与 onAgentBusy(thinking) 区分——启动中并非思考中。ready/error 后清除。
 */
void DAAgentSessionChatWidget::onAgentStarting(const QString& sessionId)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    d->mAgentStarting = true;
    if (d->mChannel) {
        d->mChannel->setStarting();
    }
}

/**
 * @brief 处理 Agent 就绪信号
 * @param sessionId 信号归属会话
 * @param model 模型名称
 */
void DAAgentSessionChatWidget::onAgentReady(const QString& sessionId, const QString& model)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    d->mAgentStarting = false;  // 启动完成，清除启动态
    d->mCurrentModel = model;
    // 审计问题 15：不再无条件清 busy——冷启动时序中 sendMessage 的 busy(true)
    // 被 onAgentBusy 的 starting 守卫吞掉 web 推送（内部 mAgentBusy 已记 true），
    // ready 到达时本轮对话才真正开始。若此处强制复位，整轮回复期间 UI 显示
    // Ready、按钮 Send、输入可用：无法 Stop 运行中的回合，且发送守卫放行第二条
    // 消息插进流式输出中间（转写时序错乱）。改为按内部状态补推 web（恢复被吞
    // 掉的推送）：mAgentBusy=true → thinking+Stop；false（预热/空闲就绪）→ Ready。
    if (d->mChannel) {
        d->mChannel->setBusy(d->mAgentBusy);
        // 推送激活供应商+模型给 web 选择器（触发按钮文案 + 选中高亮）
        d->mChannel->setActiveModel(d->mCurrentProvider, d->mCurrentModel);
    }
}

/**
 * @brief 处理 Agent 忙碌状态信号
 * @param sessionId 信号归属会话
 * @param busy 是否忙碌
 */
void DAAgentSessionChatWidget::onAgentBusy(const QString& sessionId, bool busy)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    d->mAgentBusy = busy;
    // busy(false) 兜底清除启动态——崩溃恢复最终失败/正常退出/用户停止均 emit agentBusy(false)
    if (!busy && d->mAgentStarting) {
        d->mAgentStarting = false;
    }
    // 启动中优先于思考中：懒启动 fallback 时 startAgent 的 agentStarting 与
    // sendMessage 的 agentBusy(true) 几乎同时到达，但子进程实际在启动而非思考。
    // 保持"启动中"显示直到 ready，避免误导用户为"思考中"。
    if (d->mAgentStarting) {
        return;
    }
    // busy 打包：JS 解释按钮 Send/Stop 切换 + 输入禁用 + 状态文案（thinking/ready）
    if (d->mChannel) {
        d->mChannel->setBusy(busy);
    }
}

/**
 * @brief 处理 token 使用量更新（契约2：5 参含 contextWindow 与 source）
 * @param sessionId 信号归属会话
 * @param inputTokens 输入 token
 * @param outputTokens 输出 token
 * @param totalTokens 总 token
 * @param contextWindow 上下文窗口大小
 * @param source 来源（tiktoken / usage_metadata）
 */
void DAAgentSessionChatWidget::onAgentUsage(const QString& sessionId, int inputTokens, int outputTokens,
                                            int totalTokens, int contextWindow, const QString& source)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
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
 * @brief Python load_session 重建完成：重新断言 busy 态（消除可能的 Stopping 残留）
 * @param sessionId 信号归属会话
 */
void DAAgentSessionChatWidget::onAgentSessionLoaded(const QString& sessionId)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    bool busy = d->mAgentBusy;
    runOrDefer([ ch, busy ]() {
        // 重新断言当前 busy 态（若非忙则状态文案置 Ready），消除可能的 Stopping 残留
        if (ch) {
            ch->setBusy(busy);
        }
    });
}

/**
 * @brief 工具调用需审批（ask 决策）：推送审批卡到 web
 * @param sessionId 信号归属会话
 * @param callId 工具调用 ID
 * @param toolName 工具名称
 * @param args 工具参数（含 _tier/_rememberable/_subagent）
 */
void DAAgentSessionChatWidget::onToolApprovalRequest(const QString& sessionId, const QString& callId,
                                                     const QString& toolName, const QJsonObject& args)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    if (!d->mChannel) return;
    DAAgentWebChannel* ch = d->mChannel;
    QJsonObject payload;
    payload[QStringLiteral("tool")] = toolName;
    // 剥离内部字段 _tier/_rememberable/_subagent 后作为展示参数，避免用户看到实现细节；
    // _subagent（子 agent 来源上下文，如 "explore #1"）转正为 payload.subagent 供 JS 渲染
    QJsonObject shownArgs = args;
    const QString tier       = shownArgs.take(QStringLiteral("_tier")).toString();
    const bool rememberable  = shownArgs.take(QStringLiteral("_rememberable")).toBool();
    const QString subagent   = shownArgs.take(QStringLiteral("_subagent")).toString();
    payload[QStringLiteral("args")]         = shownArgs;
    payload[QStringLiteral("tier")]         = tier;
    payload[QStringLiteral("rememberable")] = rememberable;
    payload[QStringLiteral("subagent")]     = subagent;
    runOrDefer([ ch, callId, payload ]() {
        ch->appendToolApproval(callId, payload);
    });
}

/**
 * @brief 审批作废（子进程退出/崩溃/切换会话）：通知 web 撤卡
 * @param sessionId 信号归属会话
 * @param callId 作废的审批对应工具调用 ID
 */
void DAAgentSessionChatWidget::onToolApprovalDismissed(const QString& sessionId, const QString& callId)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, callId ]() {
        if (ch) {
            ch->dismissToolApproval(callId);
        }
    });
}

/**
 * @brief 子 agent 任务进度：推送 subagent_progress 协议消息原文到 web
 * @param sessionId 信号归属会话
 * @param progress subagent_progress 协议消息原文
 *
 * 载荷含 call_id/task_id?/subagent?/state/message?/results?（母文档 §7）；
 * JS 端以 call_id 为键建进度卡片、按 task_id 幂等更新任务行，心跳（无
 * task_id 的 running 态）由 JS 忽略。
 */
void DAAgentSessionChatWidget::onAgentSubagentProgress(const QString& sessionId, const QJsonObject& progress)
{
    DA_D(d);
    if (sessionId != d->mSessionId) return;
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, progress ]() {
        if (ch) {
            ch->updateSubagentProgress(progress);
        }
    });
}

/**
 * @brief 处理系统消息信号（用户可见但不作为 LLM 对话内容的通知）
 * @param text 消息文本
 * @param level 级别："info" / "warning" / "error"
 */
void DAAgentSessionChatWidget::onSystemMessage(const QString& text, const QString& level)
{
    DA_D(d);
    DAAgentWebChannel* ch = d->mChannel;
    runOrDefer([ ch, text, level ]() {
        if (ch) {
            ch->appendSystemMessage(text, level);
        }
    });
}

// ===========================================================================
// web 用户操作 → 宿主转发
// ===========================================================================

/**
 * @brief web 两级选择器选定供应商+模型：emit activeModelChangeRequested
 *
 * 仅由 web 用户手动选择触发（chat.js onModelSelect → chatBridge.onModelSelect）。
 * 已是当前激活模型则不重复触发（避免重选相同项导致无谓停止运行中的 agent）。
 */
void DAAgentSessionChatWidget::onModelSelect(const QString& provider, const QString& model)
{
    DA_D(d);
    if (model.isEmpty()) {
        return;
    }
    // 已是当前激活供应商+模型则不重复触发
    if (provider == d->mCurrentProvider && model == d->mCurrentModel) {
        return;
    }
    // 不再终止当前生成：reconfigure 在 stdin 排队，当前轮用旧模型跑完，下一轮用新模型。
    // 选择器高亮经 setActiveModel 立即更新；ready 到达后模型标签确认。
    // 忙碌中切换时当前回复正常完成（message_end/done 自然到达），不截断不丢失。
    emit activeModelChangeRequested(provider, model);
}

/**
 * @brief web 权限模式选择器选定模式：透传 permissionModeChangeRequested
 *
 * JS 侧已对切 yolo 做二次确认，到达此处即为已确认的用户意图。
 * @param mode yolo / auto / manual
 */
void DAAgentSessionChatWidget::onPermissionModeSelect(const QString& mode)
{
    emit permissionModeChangeRequested(mode);
}

/**
 * @brief web 审批卡裁决：透传 toolApprovalDecision
 * @param callId 工具调用 ID
 * @param approved 是否批准
 * @param rememberSession 是否本会话记住（仅 file_write 生效）
 */
void DAAgentSessionChatWidget::onToolApprovalDecision(const QString& callId, bool approved, bool rememberSession)
{
    emit toolApprovalDecision(callId, approved, rememberSession);
}

/**
 * @brief web 启动 yolo 确认卡（A13）响应：透传 startupModeConfirmResponse
 * @param keepYolo true=保持 yolo，false=降级 auto
 */
void DAAgentSessionChatWidget::onStartupModeConfirmResponse(bool keepYolo)
{
    emit startupModeConfirmResponse(keepYolo);
}

// ---- 辅助方法 ----

/**
 * @brief 格式化 token 计量串：streaming_estimate 带 ~ 前缀，否则 "tokens: N / window"
 * @param totalTokens 总 token
 * @param contextWindow 上下文窗口大小
 * @param source 来源（tiktoken / usage_metadata / streaming_estimate）
 * @return 格式化后的 token 计量串
 */
QString DAAgentSessionChatWidget::formatTokenLabel(int totalTokens, int contextWindow, const QString& source) const
{
    // 返回 token 计量串：streaming_estimate 带 ~ 前缀，否则 "tokens: N / window"（已 tr 翻译）。
    // window<=0 显示 -1。popover 五项明细由 web 侧 JS 用注入的模板串渲染
    // （C++ 只推这 5 原始值，标签复用 tr("input: %1") 等既有翻译，JS 做 %1→值 替换）。
    int win = contextWindow > 0 ? contextWindow : -1;
    if (source == QStringLiteral("streaming_estimate")) {
        return tr("tokens: ~%1 / %2").arg(totalTokens).arg(win);  // cn:token: ~%1 / %2
    }
    return tr("tokens: %1 / %2").arg(totalTokens).arg(win);  // cn:token: %1 / %2
}

/**
 * @brief 根据 errorType 映射错误消息为翻译后的用户文案（plan-03 step8）
 * @param original 原始错误消息
 * @param errorType 错误类型
 * @return 翻译后的用户文案
 */
QString DAAgentSessionChatWidget::mapErrorMessage(const QString& original, const QString& errorType) const
{
    // 按 error_type 选择翻译后的用户文案
    if (errorType == "quota_exhausted") {
        return tr("API quota exhausted, please check account balance or change API key"); //cn:API 配额已耗尽，请检查账户余额或更换 API Key
    }
    if (errorType == "auth_error") {
        return tr("API key invalid or expired, please check settings"); //cn:API Key 无效或已过期，请在设置中检查配置
    }
    // 审计 L11：exhausted 族不再硬编码重试次数（旧 .arg(7) 在用户改
    // max_retries 后显示错误值）——真实进度已由重试条（agentRetrying 实时
    // 推送 attempt/max_attempts）展示，终态文案不断言具体次数
    if (errorType == "rate_limit_exhausted") {
        return tr("Failed after repeated retries: rate limited"); //cn:多次重试后仍失败：服务限流
    }
    if (errorType == "network_exhausted") {
        return tr("Failed after repeated retries: network error"); //cn:多次重试后仍失败：网络错误
    }
    if (errorType == "server_error_exhausted") {
        return tr("Failed after repeated retries: server error"); //cn:多次重试后仍失败：服务器错误
    }
    if (errorType == "bad_request") {
        // 标题不拼接原始报文（litellm 等网关包装的 400 原文可达数千字符且
        // 多为英文技术细节）；原文经 detail 折叠面板展示，可一键复制
        return tr("Request was rejected by the LLM service, see details for the original error"); //cn:请求被 LLM 服务拒绝，原始错误见详情
    }
    if (errorType == "context_overflow") {
        return tr("Context window exceeded and compaction failed"); //cn:上下文窗口超限且压缩失败
    }
    if (errorType == "timeout") {
        // 审计 L11：透传 Bridge 原文（其 tr 文案已含真实分钟数——旧 .arg(4)
        // 硬编码在用户改 inactivity_timeout_sec 后显示错误值）
        return original.isEmpty() ? tr("Agent response timeout") //cn:Agent 响应超时
                                  : original;
    }
    if (errorType == "crash_recovery") {
        // 审计 L11：透传 Bridge 原文（其 tr 文案已含真实进度 "(%2/%3)"——旧
        // 硬编码 "(1/3)" 使第 2、3 次恢复也永远显示 (1/3)）
        return original.isEmpty() ? tr("Agent process crashed, recovering...") //cn:Agent 进程异常退出，正在恢复...
                                  : original;
    }
    if (errorType == "crash_exhausted") {
        return tr("Agent process crashed repeatedly, unable to recover"); //cn:Agent 进程多次崩溃，无法恢复
    }
    if (errorType == "reconfigure_failed") {
        return tr("Failed to switch model, keeping current model"); //cn:模型切换失败，已保留当前模型
    }
    if (errorType == "unknown" || errorType.isEmpty()) {
        // 不清楚具体原因——给通用文案，原始错误经 detail 折叠面板查看
        return tr("Agent request failed, see details for the original error"); //cn:Agent 请求失败，原始错误见详情
    }
    // 其它未映射类型：透传原文（Python 端下发的 user_message，
    // 如 recursion_limit 的友好提示本身就是完整句子）
    return original.isEmpty() ? tr("Agent request failed, see details for the original error") //cn:Agent 请求失败，原始错误见详情
                              : tr("Agent error: %1").arg(original); //cn:Agent 错误：%1
}

}  // namespace DA
