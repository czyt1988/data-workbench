// DAAgentDockWidget.cpp
#include "DAAgentDockWidget.h"
#include "DAAgentSessionChatWidget.h"
#include "DAAgentSessionComponentsFactory.h"
#include "DAAgentWebChannel.h"
#include "Dialog/DADialogAgentSessionManager.h"
// ADS
#include "DockManager.h"
#include "DockWidget.h"
#include "DockAreaWidget.h"
// Qt
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QIcon>
#include <QPointer>

namespace DA
{

/**
 * @brief unbound 视图（新会话草稿区）dock 的固定 objectName
 *
 * 与 UiNames::Dock 的 AgentDockWidget("da_agentDockWidget") 区分；
 * bound 会话 dock 的 objectName 直接使用会话 ID（figureId 模式，稳定唯一）。
 */
static const char* kUnboundSessionDockObjectName = "da_agentUnboundSessionDock";

class DAAgentDockWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAAgentDockWidget)
public:
    explicit PrivateData(DAAgentDockWidget* p);

    QStackedWidget* mStack = nullptr;             ///< 占位页(0) / 嵌套停靠区(1)
    QWidget* mPlaceholder = nullptr;              ///< 无任何视图时的占位页（新建会话入口）
    ads::CDockManager* mDockManager = nullptr;    ///< 嵌套停靠管理器（会话 dock 宇宙）

    // ---- 会话视图映射（bound） ----
    QHash< QString, DAAgentSessionChatWidget* > mSessionViews;  ///< 会话 ID → 视图
    QHash< QString, ads::CDockWidget* > mSessionDocks;          ///< 会话 ID → dock
    // ---- unbound 视图（至多一个，懒建会话草稿区） ----
    DAAgentSessionChatWidget* mUnboundView = nullptr;
    ads::CDockWidget* mUnboundDock = nullptr;

    QString mCurrentSessionId;   ///< 模块当前会话缓存（sessionSwitched/Created/Cleared 信号驱动）
    QVariantList mSessions;      ///< sessionListChanged payload 缓存（标题/状态/元信息）
    QVariantList mForeignSessions;  ///< foreignAgentSessionsRunning payload 缓存（跨工程存活会话，决策点 5）

    bool mSuppressCurrentChanged = false;  ///< 程序性 add/raise/remove 期间抑制聚焦回调

    // ---- 全局状态缓存（创建新视图时注入初始值） ----
    QVariantList mAvailableModels;
    QString mCurrentProvider;
    QString mCurrentModel;
    QString mCurrentPermissionMode = QStringLiteral("yolo");
    bool mPermissionModeExplicit = false;
    bool mStartupYoloConfirmShown = false;  ///< A13 启动确认卡每次启动仅弹一次

    // ---- 辅助 ----
    ads::CDockAreaWidget* targetAreaForNewDock() const;
    DAAgentSessionChatWidget* viewOfSession(const QString& sessionId) const;
    QVariantMap sessionMeta(const QString& sessionId) const;
    QString sessionState(const QString& sessionId) const;
    QString sessionTitle(const QString& sessionId) const;
    QIcon stateIcon(const QString& state) const;
    QString stateDisplayText(const QString& state) const;
    void injectGlobalState(DAAgentSessionChatWidget* view) const;
    void syncSessionDockAppearance(const QString& sessionId);
    void updateStartupYoloConfirmPending();
    void updatePlaceholder();
    void ensureViewActive(DAAgentSessionChatWidget* view);
    QList< ads::CDockWidget* > allDocks() const;
};

DAAgentDockWidget::PrivateData::PrivateData(DAAgentDockWidget* p) : q_ptr(p)
{
}

/**
 * @brief 新 dock 的目标停靠区：从自身视图 dock 列表倒序取第一个可用 area
 *
 * 不使用 focusedDockWidget()——FocusHighlighting 下嵌套管理器的焦点控制器
 * 与顶层管理器共享 window 属性，可能返回顶层 dock（chart-dock-nesting.md 陷阱）。
 */
ads::CDockAreaWidget* DAAgentDockWidget::PrivateData::targetAreaForNewDock() const
{
    QList< ads::CDockWidget* > docks = allDocks();
    for (int i = docks.size() - 1; i >= 0; --i) {
        if (docks.at(i) && docks.at(i)->dockAreaWidget()) {
            return docks.at(i)->dockAreaWidget();
        }
    }
    return nullptr;  // 首个视图：在容器根创建 area
}

DAAgentSessionChatWidget* DAAgentDockWidget::PrivateData::viewOfSession(const QString& sessionId) const
{
    return mSessionViews.value(sessionId, nullptr);
}

QVariantMap DAAgentDockWidget::PrivateData::sessionMeta(const QString& sessionId) const
{
    for (const QVariant& v : mSessions) {
        const QVariantMap vm = v.toMap();
        if (vm.value("id").toString() == sessionId) {
            return vm;
        }
    }
    return QVariantMap();
}

QString DAAgentDockWidget::PrivateData::sessionState(const QString& sessionId) const
{
    return sessionMeta(sessionId).value("state").toString();
}

QString DAAgentDockWidget::PrivateData::sessionTitle(const QString& sessionId) const
{
    const QString t = sessionMeta(sessionId).value("title").toString();
    return t.isEmpty() ? QObject::tr("(untitled)") : t;  // cn:（未命名）
}

QIcon DAAgentDockWidget::PrivateData::stateIcon(const QString& state) const
{
    if (state == QLatin1String("starting") || state == QLatin1String("running")) {
        return QIcon(QStringLiteral(":/DAGui/icon/session-running.svg"));
    }
    if (state == QLatin1String("waiting_input")) {
        return QIcon(QStringLiteral(":/DAGui/icon/session-waiting.svg"));
    }
    if (state == QLatin1String("error")) {
        return QIcon(QStringLiteral(":/DAGui/icon/session-error.svg"));
    }
    return QIcon(QStringLiteral(":/DAGui/icon/session.svg"));
}

QString DAAgentDockWidget::PrivateData::stateDisplayText(const QString& state) const
{
    if (state == QLatin1String("starting")) {
        return QObject::tr("Starting");  // cn:启动中
    }
    if (state == QLatin1String("running")) {
        return QObject::tr("Running");  // cn:运行中
    }
    if (state == QLatin1String("waiting_input")) {
        return QObject::tr("Waiting for you");  // cn:等待输入
    }
    if (state == QLatin1String("error")) {
        return QObject::tr("Error");  // cn:出错
    }
    return QString();
}

/**
 * @brief 向新视图注入宿主缓存的全局状态（模型列表/激活模型/权限模式/A13 待弹标志）
 */
void DAAgentDockWidget::PrivateData::injectGlobalState(DAAgentSessionChatWidget* view) const
{
    view->setAvailableModels(mAvailableModels);
    view->setActiveModel(mCurrentProvider, mCurrentModel);
    view->setPermissionMode(mCurrentPermissionMode);
    const bool pending = (mCurrentPermissionMode == QLatin1String("yolo") && mPermissionModeExplicit
                          && !mStartupYoloConfirmShown);
    view->setStartupYoloConfirmPending(pending);
}

/**
 * @brief 同步会话 dock 的标签标题/图标/tooltip（sessionListChanged payload 驱动）
 */
void DAAgentDockWidget::PrivateData::syncSessionDockAppearance(const QString& sessionId)
{
    const auto it = mSessionDocks.constFind(sessionId);
    if (it == mSessionDocks.constEnd()) {
        return;
    }
    const QString title = sessionTitle(sessionId);
    const QString state = sessionState(sessionId);
    const QString stateText = stateDisplayText(state);
    it.value()->setWindowTitle(title);
    it.value()->setIcon(stateIcon(state));
    it.value()->setToolTip(stateText.isEmpty() ? title
                                               : QStringLiteral("%1 — %2").arg(title, stateText));
}

/**
 * @brief 刷新全部视图的 A13 启动确认卡待弹标志（每次启动仅首个就绪视图弹出）
 */
void DAAgentDockWidget::PrivateData::updateStartupYoloConfirmPending()
{
    const bool pending = (mCurrentPermissionMode == QLatin1String("yolo") && mPermissionModeExplicit
                          && !mStartupYoloConfirmShown);
    const auto setPending = [ this, pending ](DAAgentSessionChatWidget* v) { v->setStartupYoloConfirmPending(pending); };
    for (auto it = mSessionViews.cbegin(); it != mSessionViews.cend(); ++it) {
        setPending(it.value());
    }
    if (mUnboundView) {
        setPending(mUnboundView);
    }
}

void DAAgentDockWidget::PrivateData::updatePlaceholder()
{
    if (!mStack) {
        return;
    }
    const bool empty = mSessionDocks.isEmpty() && nullptr == mUnboundDock;
    mStack->setCurrentIndex(empty ? 0 : 1);
}

/**
 * @brief "交互即激活"：确保来源视图会话为模块当前会话
 *
 * Module 下行调用（sendMessage/sendUserAnswer）按"当前会话"路由，用户在后台
 * 视图（分屏可见）交互时先同步切换再转发。unbound 视图 → 清模块当前会话
 * （首条消息由 sendMessage 懒建会话）。
 */
void DAAgentDockWidget::PrivateData::ensureViewActive(DAAgentSessionChatWidget* view)
{
    const QString sid = view ? view->sessionId() : QString();
    if (sid.isEmpty()) {
        if (!mCurrentSessionId.isEmpty()) {
            mCurrentSessionId.clear();
            emit q_ptr->currentSessionClearedRequested();
        }
        return;
    }
    if (sid != mCurrentSessionId) {
        // 乐观更新缓存（switchSession 对已当前会话返回 false 不回发 sessionSwitched）
        mCurrentSessionId = sid;
        emit q_ptr->sessionSwitchRequested(sid);
    }
}

QList< ads::CDockWidget* > DAAgentDockWidget::PrivateData::allDocks() const
{
    QList< ads::CDockWidget* > docks;
    docks.reserve(mSessionDocks.size() + 1);
    for (auto it = mSessionDocks.cbegin(); it != mSessionDocks.cend(); ++it) {
        docks.append(it.value());
    }
    if (mUnboundDock) {
        docks.append(mUnboundDock);
    }
    return docks;
}

//===================================================
// DAAgentDockWidget
//===================================================

/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DAAgentDockWidget::DAAgentDockWidget(QWidget* parent)
    : QWidget(parent)
    , DA_PIMPL_CONSTRUCT
{
    setupUI();
    // 启动即备一个 unbound 视图（等价旧版"空聊天区待输入"）
    createUnboundView();
}

/**
 * @brief 析构函数
 *
 * 嵌套 CDockManager 为本部件子对象，由 Qt 自动销毁（连带全部视图 dock）；
 * 视图禁止浮动，无浮动窗口需额外清理（DAChartOperateWidget 同款析构语义）。
 */
DAAgentDockWidget::~DAAgentDockWidget()
{
}

/**
 * @brief 初始化 UI：占位页 + 嵌套停靠管理器
 */
void DAAgentDockWidget::setupUI()
{
    DA_D(d);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    d->mStack = new QStackedWidget(this);
    mainLayout->addWidget(d->mStack);

    // ---- 占位页：全部视图关闭后显示（新建会话入口） ----
    d->mPlaceholder = new QWidget(d->mStack);
    QVBoxLayout* phLayout = new QVBoxLayout(d->mPlaceholder);
    phLayout->setContentsMargins(0, 0, 0, 0);
    phLayout->setSpacing(8);
    phLayout->addStretch(1);
    QLabel* phIcon = new QLabel(d->mPlaceholder);
    phIcon->setPixmap(QIcon(QStringLiteral(":/DAGui/icon/session-new.svg")).pixmap(48, 48));
    phIcon->setAlignment(Qt::AlignCenter);
    phLayout->addWidget(phIcon);
    QLabel* phText = new QLabel(tr("No agent session is open"), d->mPlaceholder);  // cn:没有打开的 Agent 会话
    phText->setAlignment(Qt::AlignCenter);
    phLayout->addWidget(phText);
    QPushButton* phNewBtn = new QPushButton(tr("New Session"), d->mPlaceholder);  // cn:新建会话
    phNewBtn->setIcon(QIcon(QStringLiteral(":/DAGui/icon/session-new.svg")));
    phLayout->addWidget(phNewBtn, 0, Qt::AlignHCenter);
    phLayout->addStretch(1);
    connect(phNewBtn, &QPushButton::clicked, this, &DAAgentDockWidget::onPlaceholderNewSessionClicked);
    d->mStack->addWidget(d->mPlaceholder);

    // ---- 嵌套停靠管理器（会话 dock 停靠宇宙，仿 DAChartOperateWidget） ----
    d->mDockManager = new ads::CDockManager(this);
    d->mStack->addWidget(d->mDockManager);
    // 注入自定义组件工厂：标题栏「+/会话管理」按钮 + 标签右键菜单（重命名/删除/停止），
    // 仅影响本嵌套管理器内的会话 dock，不影响顶层停靠区
    d->mDockManager->setComponentsFactory(new DAAgentSessionComponentsFactory(this));
    // 禁止会话 dock 浮动为独立窗口（全局锁，对所有当前及后续 dock 生效），保留分屏/并栏/拖拽
    d->mDockManager->lockDockWidgetFeaturesGlobally(ads::CDockWidget::DockWidgetFloatable);
    // 嵌套停靠区聚焦改变：onFocusedDockChanged 内部过滤非本管理器的 dock，
    // 规避 FocusHighlighting 下嵌套焦点控制器跨管理器回调顶层 dock 的问题
    connect(d->mDockManager, &ads::CDockManager::focusedDockWidgetChanged,
            this, &DAAgentDockWidget::onFocusedDockChanged);

    d->updatePlaceholder();
}

/**
 * @brief 「+」入口：已有 unbound 视图则 raise，否则创建
 *
 * 懒创建语义：不落盘建会话，首条消息发出后才由 Module::sendMessage 建会话并
 * 经 sessionCreated 信号绑定本视图。标题栏「+」按钮、占位页按钮与 Ribbon
 * 「新建会话」共用本入口。
 */
void DAAgentDockWidget::requestNewSession()
{
    DA_D(d);
    if (d->mUnboundDock) {
        d->mSuppressCurrentChanged = true;
        d->mUnboundDock->raise();
        d->mSuppressCurrentChanged = false;
        // raise 不一定触发聚焦回调（已是当前标签），主动同步当前会话语义
        handleCurrentViewChanged(d->mUnboundDock);
        return;
    }
    createUnboundView();
}

/**
 * @brief 弹出会话管理对话框
 *
 * 操作经 signal→signal 直连转发到 DAAgentInterface（switch/rename/delete）。
 * 标题栏「会话管理」按钮与 Ribbon「会话管理」共用本入口。
 */
void DAAgentDockWidget::requestShowSessionManager()
{
    DA_D(d);
    DADialogAgentSessionManager dlg(d->mSessions, d->mCurrentSessionId, this);
    // 决策点 5 方案 c：跨工程存活会话注入"全部工程"视图（提示条点击/手动
    // 勾选均可查看并停止旧工程后台会话）
    dlg.setForeignSessions(d->mForeignSessions);
    connect(&dlg, &DADialogAgentSessionManager::switchRequested,
            this, &DAAgentDockWidget::sessionSwitchRequested);
    connect(&dlg, &DADialogAgentSessionManager::renameRequested,
            this, &DAAgentDockWidget::sessionRenameRequested);
    connect(&dlg, &DADialogAgentSessionManager::deleteRequested,
            this, &DAAgentDockWidget::sessionDeleteRequested);
    // 审计 L14：右键"停止"→ stopSessionRequested（后台运行会话不必先切换再 Stop）
    connect(&dlg, &DADialogAgentSessionManager::stopRequested,
            this, &DAAgentDockWidget::stopSessionRequested);
    dlg.exec();
    // 对话框关闭后 sessionListChanged 会从 Module 回灌权威状态刷新标签
}

/**
 * @brief 标签右键菜单入口：重命名会话（输入框确认后发射 sessionRenameRequested）
 */
void DAAgentDockWidget::requestRenameSession(const QString& sessionId)
{
    DA_D(d);
    if (sessionId.isEmpty() || !d->mSessionDocks.contains(sessionId)) {
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this,
                                               tr("Rename Session"),  // cn:重命名会话
                                               tr("Session name:"),   // cn:会话名称：
                                               QLineEdit::Normal,
                                               d->sessionTitle(sessionId),
                                               &ok);
    if (ok && !name.trimmed().isEmpty()) {
        emit sessionRenameRequested(sessionId, name.trimmed());
    }
}

/**
 * @brief 标签右键菜单入口：删除会话（确认后发射 sessionDeleteRequested）
 */
void DAAgentDockWidget::requestDeleteSession(const QString& sessionId)
{
    DA_D(d);
    if (sessionId.isEmpty() || !d->mSessionDocks.contains(sessionId)) {
        return;
    }
    const QMessageBox::StandardButton btn = QMessageBox::question(
        this,
        tr("Delete Session"),                                          // cn:删除会话
        tr("Delete session \"%1\"? Its chat history will be removed.")  // cn:删除会话"%1"？其聊天记录将被移除。
            .arg(d->sessionTitle(sessionId)));
    if (QMessageBox::Yes == btn) {
        emit sessionDeleteRequested(sessionId);
    }
}

/**
 * @brief 会话是否处于运行/启动/等待输入态（决定右键菜单"停止会话"项与关闭弹窗）
 */
bool DAAgentDockWidget::isSessionActive(const QString& sessionId) const
{
    DA_DC(d);
    const QString state = d->sessionState(sessionId);
    return state == QLatin1String("starting") || state == QLatin1String("running")
           || state == QLatin1String("waiting_input");
}

/**
 * @brief 标签右键菜单入口：停止会话生成
 */
void DAAgentDockWidget::requestStopSession(const QString& sessionId)
{
    if (!sessionId.isEmpty()) {
        emit stopSessionRequested(sessionId);
    }
}

// ===========================================================================
// 视图创建 / 移除
// ===========================================================================

/**
 * @brief 创建会话视图 dock（objectName=会话 ID），加载历史并 raise
 * @param sessionId 会话 ID
 * @param records 会话完整 JSONL 记录（来自 sessionSwitched，视图内部缓存至 web 就绪后重放）
 */
void DAAgentDockWidget::createSessionView(const QString& sessionId, const QVector<QJsonObject>& records)
{
    DA_D(d);
    if (sessionId.isEmpty() || d->mSessionDocks.contains(sessionId)) {
        return;
    }
    DAAgentSessionChatWidget* view = new DAAgentSessionChatWidget();
    view->setSessionId(sessionId);
    d->injectGlobalState(view);
    if (!records.isEmpty()) {
        view->loadHistory(records);
    }
    ads::CDockWidget* dock = new ads::CDockWidget(d->mDockManager, d->sessionTitle(sessionId));
    dock->setObjectName(sessionId);  // 会话 ID = 稳定持久 id（figureId 模式）
    dock->setWidget(view, ads::CDockWidget::ForceNoScrollArea);
    // 关闭按钮触发 closeRequested 而非自动隐藏，便于运行中会话弹三选确认
    dock->setFeature(ads::CDockWidget::CustomCloseHandling, true);
    dock->setIcon(d->stateIcon(d->sessionState(sessionId)));
    connect(dock, &ads::CDockWidget::closeRequested, this, [ this, dock ]() {
        onDockCloseRequested(dock);
    });
    d->mSessionDocks[ sessionId ] = dock;
    d->mSessionViews[ sessionId ] = view;
    setupViewConnections(view);
    // 抑制聚焦改变：add/raise 触发的聚焦回调不产生额外切换
    //（模块当前会话已由 switchSession 设为目标会话）
    d->mSuppressCurrentChanged = true;
    ads::CDockAreaWidget* area = d->targetAreaForNewDock();
    if (area) {
        d->mDockManager->addDockWidgetTabToArea(dock, area);  // 已有视图：作为标签加入
    } else {
        d->mDockManager->addDockWidget(ads::CenterDockWidgetArea, dock);  // 首个：容器根
    }
    dock->raise();
    d->mSuppressCurrentChanged = false;
    d->updatePlaceholder();
    // attach 通知：模块记录"有视图"并重发挂起的 ask_user 问题卡/审批卡
    emit sessionViewAttachedChanged(sessionId, true);
}

/**
 * @brief 创建 unbound 视图 dock（懒创建草稿区，不落盘）
 */
void DAAgentDockWidget::createUnboundView()
{
    DA_D(d);
    if (d->mUnboundDock) {
        return;
    }
    DAAgentSessionChatWidget* view = new DAAgentSessionChatWidget();
    d->injectGlobalState(view);
    ads::CDockWidget* dock = new ads::CDockWidget(d->mDockManager, tr("New Session"));  // cn:新建会话
    dock->setObjectName(QString::fromLatin1(kUnboundSessionDockObjectName));
    dock->setWidget(view, ads::CDockWidget::ForceNoScrollArea);
    dock->setFeature(ads::CDockWidget::CustomCloseHandling, true);
    dock->setIcon(QIcon(QStringLiteral(":/DAGui/icon/session-new.svg")));
    dock->setToolTip(tr("New Session"));  // cn:新建会话
    connect(dock, &ads::CDockWidget::closeRequested, this, [ this, dock ]() {
        onDockCloseRequested(dock);
    });
    d->mUnboundDock = dock;
    d->mUnboundView = view;
    setupViewConnections(view);
    d->mSuppressCurrentChanged = true;
    ads::CDockAreaWidget* area = d->targetAreaForNewDock();
    if (area) {
        d->mDockManager->addDockWidgetTabToArea(dock, area);
    } else {
        d->mDockManager->addDockWidget(ads::CenterDockWidgetArea, dock);
    }
    dock->raise();
    d->mSuppressCurrentChanged = false;
    d->updatePlaceholder();
    // 激活 unbound 视图 = 无当前会话（首条消息懒建）
    handleCurrentViewChanged(dock);
}

/**
 * @brief 移除视图 dock（关闭标签/删除会话对账/工程切换清理）
 * @param dock 目标 dock
 * @param notifyDetach true=发射 sessionViewAttachedChanged(sid,false)（模块更新视图集合）
 */
void DAAgentDockWidget::removeViewDock(ads::CDockWidget* dock, bool notifyDetach)
{
    DA_D(d);
    if (!dock) {
        return;
    }
    // QPointer 防 area 悬空：removeDockWidget 可能连带销毁空 area
    QPointer< ads::CDockAreaWidget > area = dock->dockAreaWidget();
    if (dock == d->mUnboundDock) {
        d->mUnboundDock = nullptr;
        d->mUnboundView = nullptr;
    } else {
        const QString sid = dock->objectName();
        if (!d->mSessionDocks.contains(sid)) {
            return;  // 非本宿主管理（防御）
        }
        d->mSessionDocks.remove(sid);
        d->mSessionViews.remove(sid);
        if (notifyDetach) {
            emit sessionViewAttachedChanged(sid, false);
        }
        // 关闭的是当前会话视图：模块当前会话指针同步清除
        //（空闲桥随 clearCurrentSession 退役，忙碌桥留后台继续）
        if (sid == d->mCurrentSessionId) {
            d->mCurrentSessionId.clear();
            emit currentSessionClearedRequested();
        }
    }
    d->mSuppressCurrentChanged = true;
    d->mDockManager->removeDockWidget(dock);
    dock->deleteLater();  // 级联删除视图（含 WebView），延迟到事件循环
    d->mSuppressCurrentChanged = false;
    d->updatePlaceholder();
    // 主动同步新当前视图的会话语义（removeDockWidget 期间的聚焦事件已被抑制窗口丢弃）：
    // ADS 移除标签后同 area 当前标签自动前移，此处按移除后的实际状态取值
    if (area) {
        if (ads::CDockWidget* cur = area->currentDockWidget()) {
            handleCurrentViewChanged(cur);
        } else if (ads::CDockAreaWidget* aliveArea = d->targetAreaForNewDock()) {
            // 原 area 已空：聚焦转移到其他 area，按任一存活 area 的当前标签兜底
            if (ads::CDockWidget* cur = aliveArea->currentDockWidget()) {
                handleCurrentViewChanged(cur);
            }
        }
    }
}

/**
 * @brief 连接视图的用户操作信号（"交互即激活"后转发）
 * @param view 目标视图
 */
void DAAgentDockWidget::setupViewConnections(DAAgentSessionChatWidget* view)
{
    DA_D(d);
    // 发消息：先确保来源视图会话为模块当前会话（后台分屏视图发消息的场景）
    connect(view, &DAAgentSessionChatWidget::sendMessageRequested, this,
            [ this, d, view ](const QString& text) {
                d->ensureViewActive(view);
                emit sendMessageRequested(text);
            });
    // Stop：绑定会话 → 按会话停止（后台视图可直接停自己的会话）；unbound → 常规 stop
    connect(view, &DAAgentSessionChatWidget::stopRequested, this, [ this, view ]() {
        const QString sid = view->sessionId();
        if (!sid.isEmpty()) {
            emit stopSessionRequested(sid);
        } else {
            emit stopRequested();
        }
    });
    // 回答问题：先激活来源会话（sendUserAnswer 按当前会话路由 + FIFO 配对）
    connect(view, &DAAgentSessionChatWidget::userAnswerSelected, this,
            [ this, d, view ](const QString& answer) {
                d->ensureViewActive(view);
                emit userAnswerSelected(answer);
            });
    // 其余用户操作：直接转发（审批经 callId 路由，无需激活来源会话）
    connect(view, &DAAgentSessionChatWidget::figureLinkRequested,
            this, &DAAgentDockWidget::figureLinkRequested);
    connect(view, &DAAgentSessionChatWidget::activeModelChangeRequested,
            this, &DAAgentDockWidget::activeModelChangeRequested);
    connect(view, &DAAgentSessionChatWidget::permissionModeChangeRequested,
            this, &DAAgentDockWidget::permissionModeChangeRequested);
    connect(view, &DAAgentSessionChatWidget::toolApprovalDecision,
            this, &DAAgentDockWidget::toolApprovalDecision);
    connect(view, &DAAgentSessionChatWidget::startupModeConfirmResponse,
            this, &DAAgentDockWidget::startupModeConfirmResponse);
    connect(view, &DAAgentSessionChatWidget::startupYoloConfirmShown,
            this, &DAAgentDockWidget::onViewStartupYoloConfirmShown);
    // 跨工程会话提示条点击（决策点 5 方案 c）：打开会话管理对话框
    //（对话框经 setForeignSessions 提供"全部工程"视图与一键停止）
    connect(view, &DAAgentSessionChatWidget::foreignBannerClicked,
            this, &DAAgentDockWidget::requestShowSessionManager);
}

// ===========================================================================
// 聚焦路由（标签切换 = 会话切换）
// ===========================================================================

/**
 * @brief 嵌套停靠区聚焦 dock 改变（标签点击/拖拽并栏后的当前标签变化）
 *
 * FocusHighlighting 下嵌套管理器的 CDockFocusController 与顶层管理器共享
 * window 属性，用户聚焦顶层 dock（如工作流操作）时本信号也会被回调——
 * 按 dockManager() 过滤，只处理属于本嵌套管理器的会话 dock。
 */
void DAAgentDockWidget::onFocusedDockChanged(ads::CDockWidget* oldDock, ads::CDockWidget* nowDock)
{
    Q_UNUSED(oldDock);
    DA_D(d);
    if (d->mSuppressCurrentChanged) {
        return;
    }
    if (nowDock && nowDock->dockManager() != d->mDockManager) {
        return;  // 过滤跨管理器回调
    }
    if (nowDock) {
        handleCurrentViewChanged(nowDock);
    }
}

/**
 * @brief 当前会话视图变化：绑定会话 → 发射切换请求；unbound → 清当前会话
 * @param dock 变为当前的 dock
 */
void DAAgentDockWidget::handleCurrentViewChanged(ads::CDockWidget* dock)
{
    DA_D(d);
    if (!dock) {
        return;
    }
    if (dock == d->mUnboundDock) {
        // unbound 视图激活 = 无当前会话（首条消息懒建）
        if (!d->mCurrentSessionId.isEmpty()) {
            d->mCurrentSessionId.clear();
            emit currentSessionClearedRequested();
        }
        return;
    }
    const QString sid = dock->objectName();
    if (sid.isEmpty() || !d->mSessionViews.contains(sid)) {
        return;  // 非会话视图（防御）
    }
    if (sid == d->mCurrentSessionId) {
        return;  // 未变化
    }
    // 乐观更新缓存（switchSession 对已当前会话返回 false 不回发 sessionSwitched；
    // 有视图时模块走快速路径：不重放历史，仅同步状态）
    d->mCurrentSessionId = sid;
    emit sessionSwitchRequested(sid);
}

/**
 * @brief dock 关闭请求（CustomCloseHandling）：运行中/等待输入会话弹三选确认
 * @param dock 目标 dock
 *
 * 关闭视图 ≠ 删除会话：会话记录始终保留在磁盘，可随时从会话管理重新打开。
 * unbound / 空闲 / 出错会话直接关闭；启动中/运行中/等待输入弹窗：
 * 后台继续运行（默认）/ 停止会话并关闭 / 取消。
 */
void DAAgentDockWidget::onDockCloseRequested(ads::CDockWidget* dock)
{
    DA_D(d);
    if (!dock) {
        return;
    }
    if (dock == d->mUnboundDock) {
        removeViewDock(dock, false);
        return;
    }
    const QString sid = dock->objectName();
    if (sid.isEmpty() || !d->mSessionDocks.contains(sid)) {
        return;
    }
    if (!isSessionActive(sid)) {
        // 空闲/出错：直接关闭（会话记录保留，可从会话管理重新打开）
        removeViewDock(dock, true);
        return;
    }
    const QString state = d->sessionState(sid);
    QString text;
    if (state == QLatin1String("waiting_input")) {
        text = tr("Session \"%1\" is waiting for your input. What do you want to do?");  // cn:会话"%1"正在等待你的输入。要如何处理？
    } else {
        text = tr("Session \"%1\" is still running. What do you want to do?");  // cn:会话"%1"仍在运行中。要如何处理？
    }
    QMessageBox box(QMessageBox::Question,
                    tr("Close Session"),  // cn:关闭会话
                    text.arg(d->sessionTitle(sid)),
                    QMessageBox::Cancel,
                    this);
    QPushButton* backgroundBtn =
        box.addButton(tr("Keep Running in Background"), QMessageBox::AcceptRole);  // cn:后台继续运行
    QPushButton* stopCloseBtn =
        box.addButton(tr("Stop Session and Close"), QMessageBox::DestructiveRole);  // cn:停止会话并关闭
    box.setDefaultButton(backgroundBtn);
    box.exec();
    if (box.clickedButton() == box.button(QMessageBox::Cancel)) {
        return;  // 取消：不关闭
    }
    if (box.clickedButton() == stopCloseBtn) {
        emit stopSessionRequested(sid);
    }
    removeViewDock(dock, true);
}

// ===========================================================================
// 会话级信号槽（路由到对应视图）
// ===========================================================================

/**
 * @brief 处理 Agent 流式 token 信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentToken(const QString& sessionId, const QString& token)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentToken(sessionId, token);
    }
}

/**
 * @brief 处理 Agent 消息完成信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentMessageComplete(const QString& sessionId, const QString& fullText)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentMessageComplete(sessionId, fullText);
    }
}

/**
 * @brief 处理 Agent 工具调用信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentToolCall(const QString& sessionId, const QString& toolName, const QJsonObject& args)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentToolCall(sessionId, toolName, args);
    }
}

/**
 * @brief 处理工具排队状态（决策点 2 ③：全局执行队列排队可见，路由到会话视图）
 * @param toolName 工具名称
 * @param position 队列位置（1-based）；0=开始执行（恢复"运行中"）
 */
void DAAgentDockWidget::onAgentToolQueued(const QString& sessionId, const QString& toolName, int position)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentToolQueued(sessionId, toolName, position);
    }
}

/**
 * @brief 处理 Agent 工具结果信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentToolResult(const QString& sessionId, const QString& toolName, const QJsonObject& result)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentToolResult(sessionId, toolName, result);
    }
}

/**
 * @brief 处理挂起问题卡作废（子进程退出/崩溃/用户 Stop/桥退役，路由到会话视图）
 *
 * 镜像 onToolApprovalDismissed 契约（审计问题 17）：通知 web 移除未回答的
 * 问题卡，防止用户对幽灵卡作答（答案经死桥发送必然蒸发）。
 */
void DAAgentDockWidget::onQuestionDismissed(const QString& sessionId)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onQuestionDismissed(sessionId);
    }
}

/**
 * @brief 处理 Agent 提问信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentQuestion(const QString& sessionId, const QString& text, const QStringList& options,
                                        bool multiSelect)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentQuestion(sessionId, text, options, multiSelect);
    }
}

/**
 * @brief 处理 Agent 错误信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentError(const QString& sessionId, const QString& message, const QString& errorType,
                                     const QString& detail)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentError(sessionId, message, errorType, detail);
    }
}

/**
 * @brief 处理 Agent 重试信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentRetrying(const QString& sessionId, int attempt, int maxAttempts, int delayMs,
                                         const QString& errorType, const QString& errorMessage)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentRetrying(sessionId, attempt, maxAttempts, delayMs, errorType, errorMessage);
    }
}

/**
 * @brief 处理 Agent 启动信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentStarting(const QString& sessionId)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentStarting(sessionId);
    }
}

/**
 * @brief 处理 Agent 就绪信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentReady(const QString& sessionId, const QString& model)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentReady(sessionId, model);
    }
}

/**
 * @brief 处理 Agent 忙碌状态信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentBusy(const QString& sessionId, bool busy)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentBusy(sessionId, busy);
    }
}

/**
 * @brief 处理 token 使用量更新（路由到会话视图）
 */
void DAAgentDockWidget::onAgentUsage(const QString& sessionId, int inputTokens, int outputTokens, int totalTokens,
                                     int contextWindow, const QString& source)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentUsage(sessionId, inputTokens, outputTokens, totalTokens, contextWindow, source);
    }
}

/**
 * @brief 处理 load_session 重建完成信号（路由到会话视图）
 */
void DAAgentDockWidget::onAgentSessionLoaded(const QString& sessionId)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentSessionLoaded(sessionId);
    }
}

/**
 * @brief 处理工具审批请求（路由到会话视图）
 */
void DAAgentDockWidget::onToolApprovalRequest(const QString& sessionId, const QString& callId, const QString& toolName,
                                              const QJsonObject& args)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onToolApprovalRequest(sessionId, callId, toolName, args);
    }
}

/**
 * @brief 处理审批作废（路由到会话视图撤卡）
 */
void DAAgentDockWidget::onToolApprovalDismissed(const QString& sessionId, const QString& callId)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onToolApprovalDismissed(sessionId, callId);
    }
}

/**
 * @brief 处理子 agent 任务进度（路由到会话视图）
 */
void DAAgentDockWidget::onAgentSubagentProgress(const QString& sessionId, const QJsonObject& progress)
{
    DA_D(d);
    if (DAAgentSessionChatWidget* v = d->viewOfSession(sessionId)) {
        v->onAgentSubagentProgress(sessionId, progress);
    }
}

// ===========================================================================
// 会话生命周期
// ===========================================================================

/**
 * @brief 会话切换完成
 * @param sessionId 新会话 ID
 * @param allRecords 新会话完整记录（有视图快速路径为空数组）
 *
 * 有视图（用户点标签切换）：模块走快速路径（不重放历史），此处仅同步缓存；
 * 无视图（会话管理对话框切换到未打开会话）：创建视图并重放历史。
 */
void DAAgentDockWidget::onSessionSwitched(const QString& sessionId, const QVector<QJsonObject>& allRecords)
{
    DA_D(d);
    d->mCurrentSessionId = sessionId;
    if (d->mSessionDocks.contains(sessionId)) {
        return;  // 视图已存在（标签切换快速路径），内容实时最新
    }
    createSessionView(sessionId, allRecords);
}

/**
 * @brief 会话列表变化：同步标签外观 + 对账移除已删除会话的视图
 * @param sessions sessionListChanged payload（每元素含 id/title/state/...）
 */
void DAAgentDockWidget::onSessionListChanged(QVariantList sessions)
{
    DA_D(d);
    d->mSessions = sessions;
    // 对账：视图存在但会话已不在列表（被删除）→ 移除视图
    //（工程切换场景由 onSessionCleared 统一清理，此处兜底删除对账）
    QList< ads::CDockWidget* > staleDocks;
    for (auto it = d->mSessionDocks.cbegin(); it != d->mSessionDocks.cend(); ++it) {
        if (d->sessionMeta(it.key()).isEmpty()) {
            staleDocks.append(it.value());
        }
    }
    for (ads::CDockWidget* dock : std::as_const(staleDocks)) {
        removeViewDock(dock, true);
    }
    // 同步标题/状态徽标
    for (auto it = d->mSessionDocks.cbegin(); it != d->mSessionDocks.cend(); ++it) {
        d->syncSessionDockAppearance(it.key());
    }
}

/**
 * @brief 新会话创建：把 unbound 视图绑定到该会话
 * @param sessionId 新会话 ID
 *
 * 触发路径：①首条消息懒建（sendMessage → createSession + sessionCreated）——
 * unbound 视图已渲染用户气泡，绑定不清空内容；②newSession()（程序化路径）。
 * 无 unbound 视图且无该会话视图时（如全部标签关闭后 runAgent），创建空视图。
 */
void DAAgentDockWidget::onSessionCreated(const QString& sessionId)
{
    DA_D(d);
    if (sessionId.isEmpty() || d->mSessionDocks.contains(sessionId)) {
        return;
    }
    if (d->mUnboundDock) {
        // unbound → bound：objectName 换为会话 ID（稳定持久 id），标题/图标同步
        ads::CDockWidget* dock = d->mUnboundDock;
        DAAgentSessionChatWidget* view = d->mUnboundView;
        d->mUnboundDock = nullptr;
        d->mUnboundView = nullptr;
        dock->setObjectName(sessionId);
        d->mSessionDocks[ sessionId ] = dock;
        d->mSessionViews[ sessionId ] = view;
        if (view) {
            view->setSessionId(sessionId);  // 仅记录归属，不清空（用户气泡已渲染）
        }
        d->syncSessionDockAppearance(sessionId);
        d->updatePlaceholder();
        emit sessionViewAttachedChanged(sessionId, true);
        return;
    }
    createSessionView(sessionId, {});
}

/**
 * @brief 当前无活跃会话（启动/打开工程）：关闭全部绑定视图，保留一个 unbound 视图
 *
 * 始终以全新对话开始（不自动恢复上次会话）——历史会话仍保留在磁盘与会话管理
 * 列表中，用户可随时重新打开。运行中会话的桥留后台继续（视图关闭仅 detach）。
 */
void DAAgentDockWidget::onSessionCleared()
{
    DA_D(d);
    d->mSuppressCurrentChanged = true;
    // 关闭全部绑定视图（会话可能仍在后台运行：仅 detach，不停止）
    const QList< ads::CDockWidget* > docks = d->allDocks();
    for (ads::CDockWidget* dock : docks) {
        if (dock != d->mUnboundDock) {
            const QString sid = dock->objectName();
            d->mSessionDocks.remove(sid);
            d->mSessionViews.remove(sid);
            d->mDockManager->removeDockWidget(dock);
            dock->deleteLater();
            emit sessionViewAttachedChanged(sid, false);
        }
    }
    d->mCurrentSessionId.clear();
    d->mSuppressCurrentChanged = false;
    d->updatePlaceholder();
    // 保证恰好一个 unbound 视图（等价"空聊天区待输入"）
    if (!d->mUnboundDock) {
        createUnboundView();  // 内部处理激活语义
    } else {
        d->mSuppressCurrentChanged = true;
        d->mUnboundDock->raise();
        d->mSuppressCurrentChanged = false;
        handleCurrentViewChanged(d->mUnboundDock);
    }
}

// ===========================================================================
// 全局信号槽（广播到全部视图）
// ===========================================================================

/**
 * @brief 跨工程存活会话变化（决策点 5 方案 c，审计问题 18）
 * @param sessions 绑定其它工程的存活桥会话 payload（空列表=无，隐藏提示条）
 *
 * 缓存列表供会话管理对话框"全部工程"视图取数；各视图聊天区顶部显示/更新/
 * 隐藏提示条（"N 个上一工程的会话仍在后台运行"，点击打开对话框）。
 */
void DAAgentDockWidget::onForeignAgentSessionsRunning(QVariantList sessions)
{
    DA_D(d);
    d->mForeignSessions = sessions;
    for (auto it = d->mSessionViews.cbegin(); it != d->mSessionViews.cend(); ++it) {
        it.value()->setForeignSessionsBanner(sessions.size());
    }
    if (d->mUnboundView) {
        d->mUnboundView->setForeignSessionsBanner(sessions.size());
    }
}

/**
 * @brief 系统消息：广播到全部视图（LLM 未配置等全局性提示）
 */
void DAAgentDockWidget::onSystemMessage(const QString& text, const QString& level)
{
    DA_D(d);
    for (auto it = d->mSessionViews.cbegin(); it != d->mSessionViews.cend(); ++it) {
        it.value()->onSystemMessage(text, level);
    }
    if (d->mUnboundView) {
        d->mUnboundView->onSystemMessage(text, level);
    }
}

/**
 * @brief 可用模型列表变化：缓存 + 广播到全部视图
 */
void DAAgentDockWidget::onAvailableModelsChanged(QVariantList models)
{
    DA_D(d);
    d->mAvailableModels = models;
    for (auto it = d->mSessionViews.cbegin(); it != d->mSessionViews.cend(); ++it) {
        it.value()->setAvailableModels(models);
    }
    if (d->mUnboundView) {
        d->mUnboundView->setAvailableModels(models);
    }
}

/**
 * @brief 激活模型变化：缓存 + 广播到全部视图
 */
void DAAgentDockWidget::onActiveModelChanged(const QString& provider, const QString& model)
{
    DA_D(d);
    d->mCurrentProvider = provider;
    d->mCurrentModel    = model;
    for (auto it = d->mSessionViews.cbegin(); it != d->mSessionViews.cend(); ++it) {
        it.value()->setActiveModel(provider, model);
    }
    if (d->mUnboundView) {
        d->mUnboundView->setActiveModel(provider, model);
    }
}

/**
 * @brief 权限模式变化：缓存 + 广播到全部视图
 */
void DAAgentDockWidget::onPermissionModeChanged(const QString& mode)
{
    DA_D(d);
    d->mCurrentPermissionMode = mode;
    for (auto it = d->mSessionViews.cbegin(); it != d->mSessionViews.cend(); ++it) {
        it.value()->setPermissionMode(mode);
    }
    if (d->mUnboundView) {
        d->mUnboundView->setPermissionMode(mode);
    }
    d->updateStartupYoloConfirmPending();
}

/**
 * @brief 权限模式"显式设置"状态（启动推送）：缓存并刷新 A13 待弹标志
 */
void DAAgentDockWidget::onPermissionModeExplicitChanged(bool explicitSet)
{
    DA_D(d);
    d->mPermissionModeExplicit = explicitSet;
    d->updateStartupYoloConfirmPending();
}

// ===========================================================================
// 视图用户操作槽
// ===========================================================================

/**
 * @brief 视图用户操作槽说明
 *
 * 发消息/Stop/回答的"交互即激活"转发逻辑在 setupViewConnections 的 lambda 中
 * 完成（需要捕获来源视图指针），不经过独立槽。
 */

/**
 * @brief A13 启动确认卡已在某视图弹出：置全局 shown 标志，其余视图不再弹
 */
void DAAgentDockWidget::onViewStartupYoloConfirmShown()
{
    DA_D(d);
    d->mStartupYoloConfirmShown = true;
    d->updateStartupYoloConfirmPending();
}

/**
 * @brief 占位页「新建会话」按钮
 */
void DAAgentDockWidget::onPlaceholderNewSessionClicked()
{
    requestNewSession();
}

}  // namespace DA
