// DAAgentSettingsWidget.cpp
// 持久化经 mAgentInterface->getProviders/setProviders（供应商结构体列表，api_key 持久化时
// 由配置层加密）与 get/setLLMConfig（其它设置）走 agent-config.json。设置页只传明文。
// 供应商增删改经弹出对话框（DAProviderEditDialog），主页面仅只读展示。
#include "DAAgentSettingsWidget.h"
#include "Dialog/DAProviderEditDialog.h"
#include "DALogCategory.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QFrame>
#include <QSignalBlocker>
#include <QIcon>

namespace DA
{

/** @brief 构造 Agent 设置页控件 */
DAAgentSettingsWidget::DAAgentSettingsWidget(QWidget* parent)
    : DAAbstractSettingPage(parent)
{
    setupUI();
}

/** @brief 注入 Agent 接口并加载已有配置 */
void DAAgentSettingsWidget::setAgentInterface(DAAgentInterface* p)
{
    mAgentInterface = p;
    if (p) {
        QSignalBlocker blocker(this);
        loadConfig();
    }
}

/** @brief 构建设置页界面（QTabWidget 两页） */
void DAAgentSettingsWidget::setupUI()
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    mTabWidget = new QTabWidget(this);
    root->addWidget(mTabWidget);
    setupProvidersTab();
    setupAgentSettingsTab();
}

/** @brief 构建供应商管理 Tab（左列表只读选择 + 右只读信息面板，分割器可调） */
void DAAgentSettingsWidget::setupProvidersTab()
{
    QWidget* page = new QWidget(this);
    QVBoxLayout* pageLay = new QVBoxLayout(page);
    pageLay->setContentsMargins(4, 4, 4, 4);
    pageLay->setSpacing(4);

    // 左侧：上方按钮行 + 供应商列表
    QWidget* leftPanel = new QWidget(page);
    QVBoxLayout* leftLay = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(4);
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(4);
    mAddProviderBtn = new QPushButton(leftPanel);
    mAddProviderBtn->setIcon(QIcon(QStringLiteral(":/app/bright/Icon/addProvider.svg")));
    mAddProviderBtn->setIconSize(QSize(18, 18));
    mAddProviderBtn->setFixedSize(28, 28);
    mAddProviderBtn->setToolTip(tr("Add Provider"));  // cn:新增供应商
    mAddProviderBtn->setCursor(Qt::PointingHandCursor);
    mEditProviderBtn = new QPushButton(leftPanel);
    mEditProviderBtn->setIcon(QIcon(QStringLiteral(":/app/bright/Icon/editProvider.svg")));
    mEditProviderBtn->setIconSize(QSize(18, 18));
    mEditProviderBtn->setFixedSize(28, 28);
    mEditProviderBtn->setToolTip(tr("Edit Provider"));  // cn:修改供应商
    mEditProviderBtn->setCursor(Qt::PointingHandCursor);
    mRemoveProviderBtn = new QPushButton(leftPanel);
    mRemoveProviderBtn->setIcon(QIcon(QStringLiteral(":/app/bright/Icon/removeProvider.svg")));
    mRemoveProviderBtn->setIconSize(QSize(18, 18));
    mRemoveProviderBtn->setFixedSize(28, 28);
    mRemoveProviderBtn->setToolTip(tr("Remove Provider"));  // cn:删除供应商
    mRemoveProviderBtn->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(mAddProviderBtn);
    btnRow->addWidget(mEditProviderBtn);
    btnRow->addWidget(mRemoveProviderBtn);
    btnRow->addStretch();
    leftLay->addLayout(btnRow);
    mProviderList = new QListWidget(leftPanel);
    mProviderList->setToolTip(tr("Configured LLM providers. Select to view details; edit via the buttons above."));  // cn:已配置的 LLM 供应商，选中查看详情，通过上方按钮编辑
    leftLay->addWidget(mProviderList, 1);

    // 右侧：只读信息面板
    QWidget* rightPanel = new QWidget(page);
    QFormLayout* rightForm = new QFormLayout(rightPanel);
    rightForm->setContentsMargins(8, 8, 8, 8);
    rightForm->setSpacing(6);
    mInfoName = new QLabel(rightPanel);
    mInfoName->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mInfoBaseUrl = new QLabel(rightPanel);
    mInfoBaseUrl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mInfoBaseUrl->setWordWrap(true);
    mInfoApiKey = new QLabel(rightPanel);
    mInfoApiKey->setTextInteractionFlags(Qt::TextSelectableByMouse);
    rightForm->addRow(tr("Name"), mInfoName);        // cn:名称
    rightForm->addRow(tr("Base URL"), mInfoBaseUrl);  // cn:基础地址
    rightForm->addRow(tr("API Key"), mInfoApiKey);    // cn:API 密钥
    rightForm->addRow(new QLabel(tr("Models"), rightPanel));  // cn:模型
    mInfoModelTable = new QTableWidget(0, 3, rightPanel);
    mInfoModelTable->setHorizontalHeaderLabels({
        tr("Model Name"),        // cn:模型名
        tr("Context Size"),      // cn:上下文大小
        tr("Max Output Tokens")  // cn:最大输出 token
    });
    mInfoModelTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    mInfoModelTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mInfoModelTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    mInfoModelTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mInfoModelTable->setSelectionMode(QAbstractItemView::NoSelection);
    mInfoModelTable->setFocusPolicy(Qt::NoFocus);
    rightForm->addRow(mInfoModelTable);

    // 分割器：左右可调比例
    mSplitter = new QSplitter(Qt::Horizontal, page);
    mSplitter->addWidget(leftPanel);
    mSplitter->addWidget(rightPanel);
    mSplitter->setStretchFactor(0, 1);
    mSplitter->setStretchFactor(1, 2);
    mSplitter->setSizes({ 200, 400 });
    pageLay->addWidget(mSplitter);

    mTabWidget->addTab(page, tr("Model Providers"));  // cn:模型供应商

    connect(mAddProviderBtn, &QPushButton::clicked, this, &DAAgentSettingsWidget::onAddProvider);
    connect(mEditProviderBtn, &QPushButton::clicked, this, &DAAgentSettingsWidget::onEditProvider);
    connect(mRemoveProviderBtn, &QPushButton::clicked, this, &DAAgentSettingsWidget::onRemoveProvider);
    connect(mProviderList, &QListWidget::currentRowChanged, this, &DAAgentSettingsWidget::onProviderSelected);
}

/** @brief 构建 Agent 其它设置 Tab */
void DAAgentSettingsWidget::setupAgentSettingsTab()
{
    QWidget* page = new QWidget(this);
    QFormLayout* form = new QFormLayout(page);

    mReadyTimeoutSpin = new QSpinBox(this);
    mReadyTimeoutSpin->setRange(5, 300);
    mReadyTimeoutSpin->setSuffix(tr(" s"));  //cn:秒
    mReadyTimeoutSpin->setToolTip(tr("Waiting time (seconds) for the agent subprocess to become ready. Recommended: 60."));  //cn:子进程就绪等待超时(秒)。建议 60。
    mReadyTimeoutSpin->setValue(60);
    mStopTimeoutSpin = new QSpinBox(this);
    mStopTimeoutSpin->setRange(1, 60);
    mStopTimeoutSpin->setSuffix(tr(" s"));  //cn:秒
    mStopTimeoutSpin->setToolTip(tr("Waiting time (seconds) for the subprocess to exit on stop. Recommended: 5."));  //cn:停止时等待子进程退出超时(秒)。建议 5。
    mStopTimeoutSpin->setValue(5);

    mCompactionThresholdSpin = new QDoubleSpinBox(this);
    mCompactionThresholdSpin->setRange(0.50, 1.0);
    mCompactionThresholdSpin->setSingleStep(0.05);
    mCompactionThresholdSpin->setDecimals(2);
    mCompactionThresholdSpin->setToolTip(tr("Compaction trigger ratio of the context window (per active model). 0.85 = compact at 85%."));  //cn:压缩触发比例(相对激活模型上下文窗口)。0.85=85% 时触发。
    mCompactionThresholdSpin->setValue(0.85);

    mMaxRecentMsgSpin = new QSpinBox(this);
    mMaxRecentMsgSpin->setRange(4, 50);
    mMaxRecentMsgSpin->setToolTip(tr("Messages retained verbatim after compaction. Recommended: 10."));  //cn:压缩后保留为原文的最近消息条数。建议 10。
    mMaxRecentMsgSpin->setValue(10);

    mToolResultMaxCharsSpin = new QSpinBox(this);
    mToolResultMaxCharsSpin->setRange(1000, 500000);
    mToolResultMaxCharsSpin->setSingleStep(1000);
    mToolResultMaxCharsSpin->setToolTip(tr("Tool output truncation threshold (chars). Recommended: 20000."));  //cn:工具输出截断阈值(字符)。建议 20000。
    mToolResultMaxCharsSpin->setValue(20000);

    mToolResultPreviewCharsSpin = new QSpinBox(this);
    mToolResultPreviewCharsSpin->setRange(100, 10000);
    mToolResultPreviewCharsSpin->setSingleStep(100);
    mToolResultPreviewCharsSpin->setToolTip(tr("Preview length (chars) of truncated tool output. Recommended: 2000."));  //cn:工具输出截断预览长度(字符)。建议 2000。
    mToolResultPreviewCharsSpin->setValue(2000);

    mMaxSessionsSpin = new QSpinBox(this);
    mMaxSessionsSpin->setRange(5, 200);
    mMaxSessionsSpin->setValue(20);
    mMaxSessionsSpin->setToolTip(tr("Max free sessions retained. Recommended: 20."));  //cn:保留的自由会话最大数量。建议 20。
    mSessionRetentionDaysSpin = new QSpinBox(this);
    mSessionRetentionDaysSpin->setRange(1, 365);
    mSessionRetentionDaysSpin->setValue(30);
    mSessionRetentionDaysSpin->setSuffix(tr(" d"));  //cn:天
    mSessionRetentionDaysSpin->setToolTip(tr("Free sessions older than this are deleted on startup. Recommended: 30."));  //cn:早于此天数的自由会话启动时删除。建议 30。

    // 重试策略（线性退避 m/n/p）：覆盖所有服务器波动类错误（400/429/5xx/网络），
    // 认证失败与配额耗尽快速失败不重试
    mSpinMaxRetries = new QSpinBox(this);
    mSpinMaxRetries->setRange(0, 20);
    mSpinMaxRetries->setSuffix(tr(" times"));  //cn:次
    mSpinMaxRetries->setToolTip(tr("Max automatic retries when the LLM service returns an error (rate limit, server error, bad request, network). 0 disables. Recommended: 5."));  //cn:LLM 服务返回错误（限流、服务器错误、请求被拒、网络）时的自动重试次数。0 不重试。建议 5。
    mSpinMaxRetries->setValue(5);

    mSpinRetryInterval = new QSpinBox(this);
    mSpinRetryInterval->setRange(0, 300);
    mSpinRetryInterval->setSuffix(tr(" sec"));  //cn:秒
    mSpinRetryInterval->setToolTip(tr("Seconds to wait before the first retry. Recommended: 5."));  //cn:首次重试前的等待秒数。建议 5。
    mSpinRetryInterval->setValue(5);

    mSpinRetryIncrement = new QSpinBox(this);
    mSpinRetryIncrement->setRange(0, 60);
    mSpinRetryIncrement->setSuffix(tr(" sec"));  //cn:秒
    mSpinRetryIncrement->setToolTip(tr("Extra seconds added to the wait after each failed retry (e.g. interval 5, increment 1 waits 5, 6, 7...). Recommended: 1."));  //cn:每次重试失败后等待时间递增的秒数（如间隔 5、递增 1 则依次等待 5、6、7...秒）。建议 1。
    mSpinRetryIncrement->setValue(1);

    mSpinRequestTimeout = new QSpinBox(this);
    mSpinRequestTimeout->setRange(10, 600);
    mSpinRequestTimeout->setSuffix(tr(" sec"));  //cn:秒
    mSpinRequestTimeout->setToolTip(tr("Timeout (seconds) for a single LLM request. Recommended: 120."));  //cn:单次 LLM 请求超时(秒)。建议 120。
    mSpinRequestTimeout->setValue(120);

    mSpinInactivityTimeout = new QSpinBox(this);
    mSpinInactivityTimeout->setRange(60, 600);
    mSpinInactivityTimeout->setSuffix(tr(" sec"));  //cn:秒
    mSpinInactivityTimeout->setToolTip(tr("Watchdog timeout: stop subprocess if no message within this period. Recommended: 240."));  //cn:看门狗超时:此时间内无消息则停止子进程。建议 240。
    mSpinInactivityTimeout->setValue(240);

    mSpinMaxRestarts = new QSpinBox(this);
    mSpinMaxRestarts->setRange(0, 10);
    mSpinMaxRestarts->setSuffix(tr(" times"));  //cn:次
    mSpinMaxRestarts->setToolTip(tr("Max auto restarts after subprocess crash. 0 disables. Recommended: 3."));  //cn:子进程崩溃后自动重启最大次数。0 不重启。建议 3。
    mSpinMaxRestarts->setValue(3);

    mSpinRecursionLimit = new QSpinBox(this);
    // 下限 -1 + 特殊文本：勾选"不限制"时 setValue(-1)，禁用态显示"无限制"。
    // 用户正常交互下不会手动输入 -1（勾选即禁用），range 含 -1 仅为承载该值
    mSpinRecursionLimit->setRange(-1, 1000000);
    mSpinRecursionLimit->setSpecialValueText(tr("No limit"));  //cn:无限制
    mSpinRecursionLimit->setToolTip(tr("Max graph reasoning steps per turn (each tool-call cycle consumes 3 steps). Check 'No limit' to disable. Recommended: 150."));  //cn:单回合图最大推理步数（每轮工具调用耗 3 步）。勾选"不限制"可关闭该上限。建议 150。
    // 默认不限制（与 DAAgentLLMConfig::recursionLimit 默认 -1 一致）
    mSpinRecursionLimit->setValue(-1);

    // 不限制勾选框：勾选 → 禁用 SpinBox 并保存 -1（Python 侧转换为无限制），
    // 用户无需知晓 -1 的含义
    mCheckRecursionNoLimit = new QCheckBox(tr("No limit"), this);  //cn:不限制
    mCheckRecursionNoLimit->setToolTip(tr("Unlimited reasoning steps per turn. Loop protection still applies: repeated identical tool calls are terminated automatically."));  //cn:单回合推理步数不设上限。循环防护仍然生效：连续重复相同的工具调用会被自动终止。
    // 默认勾选（置于 connect 之前，避免触发 toggled 联动）
    mCheckRecursionNoLimit->setChecked(true);
    connect(mCheckRecursionNoLimit, &QCheckBox::toggled, this, [this](bool on) {
        if (on) {
            mSpinRecursionLimit->setValue(-1);  // -1 经 Python 守卫转为无限制
            mSpinRecursionLimit->setEnabled(false);
        } else {
            mSpinRecursionLimit->setEnabled(true);
            mSpinRecursionLimit->setValue(150);  // 恢复建议值
        }
    });

    // ---- 子 agent（subagent-phase1 C，Q16）：全局统一作用于所有子 agent ----
    mSpinSubagentTimeout = new QSpinBox(this);
    mSpinSubagentTimeout->setRange(60, 3600);
    mSpinSubagentTimeout->setSuffix(tr(" sec"));  //cn:秒
    mSpinSubagentTimeout->setToolTip(tr("Wall-clock timeout (seconds) for each subagent task. Waiting for approval counts towards this limit. Recommended: 600."));  //cn:单个子 Agent 任务的墙钟超时(秒)。等待用户批准也计入该时限。建议 600。
    mSpinSubagentTimeout->setValue(600);
    mSpinSubagentRecursionLimit = new QSpinBox(this);
    // -1 = 不限制（Python 编排器转换为 langgraph 的 None）
    mSpinSubagentRecursionLimit->setRange(-1, 1000000);
    mSpinSubagentRecursionLimit->setSpecialValueText(tr("No limit"));  //cn:无限制
    mSpinSubagentRecursionLimit->setToolTip(tr("Max reasoning steps for each subagent task. Set to -1 for no limit. Recommended: 60."));  //cn:单个子 Agent 任务的最大推理步数。设为 -1 表示不限制。建议 60。
    mSpinSubagentRecursionLimit->setValue(60);

    mCheckAutoPrestart = new QCheckBox(this);
    mCheckAutoPrestart->setToolTip(tr("Prestart the agent subprocess on launch. Disable to save memory."));  //cn:启动时预启动 agent 子进程。关闭可节省内存。
    mCheckAutoPrestart->setChecked(true);

    form->addRow(tr("Ready Timeout"), mReadyTimeoutSpin);  //cn:就绪超时
    form->addRow(tr("Stop Timeout"),  mStopTimeoutSpin);    //cn:停止超时
    form->addRow(tr("Compaction Threshold"), mCompactionThresholdSpin);  //cn:压缩阈值
    form->addRow(tr("Max Recent Messages"), mMaxRecentMsgSpin);         //cn:保留最近消息数
    form->addRow(tr("Tool Result Max Chars"), mToolResultMaxCharsSpin);  //cn:工具结果截断阈值
    form->addRow(tr("Tool Result Preview Chars"), mToolResultPreviewCharsSpin);  //cn:工具结果预览长度
    form->addRow(tr("Max Sessions"), mMaxSessionsSpin);  //cn:最大会话数
    form->addRow(tr("Session Retention Days"), mSessionRetentionDaysSpin);  //cn:会话保留天数
    form->addRow(tr("Max retries"), mSpinMaxRetries);  //cn:最大重试次数
    form->addRow(tr("Retry interval"), mSpinRetryInterval);  //cn:重试间隔
    form->addRow(tr("Retry interval increment"), mSpinRetryIncrement);  //cn:重试间隔递增
    form->addRow(tr("Request timeout"), mSpinRequestTimeout);  //cn:请求超时
    form->addRow(tr("Inactivity timeout"), mSpinInactivityTimeout);  //cn:无活动超时
    form->addRow(tr("Max process restarts"), mSpinMaxRestarts);  //cn:最大进程重启次数
    {   // 推理上限行：SpinBox + "不限制"勾选框并排
        QWidget* row = new QWidget(this);
        QHBoxLayout* lay = new QHBoxLayout(row);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(6);
        lay->addWidget(mSpinRecursionLimit, 1);
        lay->addWidget(mCheckRecursionNoLimit, 0);
        form->addRow(tr("Reasoning iteration limit"), row);  //cn:推理迭代上限
    }
    form->addRow(tr("Subagent Timeout"), mSpinSubagentTimeout);  //cn:子 Agent 超时
    form->addRow(tr("Subagent Reasoning Limit"), mSpinSubagentRecursionLimit);  //cn:子 Agent 推理上限
    form->addRow(tr("Auto prestart on launch"), mCheckAutoPrestart);  //cn:启动时自动预热

    mTabWidget->addTab(page, tr("Agent Settings"));  //cn:Agent 设置

    auto mark = [this]() { emit settingChanged(); };
    connect(mReadyTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mStopTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mCompactionThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark);
    connect(mMaxRecentMsgSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mToolResultMaxCharsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mToolResultPreviewCharsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mMaxSessionsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSessionRetentionDaysSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinMaxRetries, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinRetryInterval, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinRetryIncrement, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinRequestTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinInactivityTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinMaxRestarts, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinRecursionLimit, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mCheckRecursionNoLimit, &QCheckBox::toggled, this, mark);
    connect(mSpinSubagentTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinSubagentRecursionLimit, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mCheckAutoPrestart, &QCheckBox::toggled, this, mark);
}

// ===========================================================================
// 供应商管理槽（弹对话框，仅 apply 时持久化）
// ===========================================================================

/** @brief 收集供应商名称（excludeIdx 排除某项，用于编辑时排除自身） */
QStringList DAAgentSettingsWidget::collectProviderNames(int excludeIdx) const
{
    QStringList names;
    for (int i = 0; i < mProviders.size(); ++i) {
        if (i == excludeIdx) continue;
        names.append(mProviders.at(i).name);
    }
    return names;
}

/** @brief 新增供应商：弹 DAProviderEditDialog（空），accept 后加入内存并刷新 */
void DAAgentSettingsWidget::onAddProvider()
{
    QStringList existing = collectProviderNames();
    DAProviderEditDialog dlg(DAAgentProvider(), existing, QString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        mProviders.append(dlg.getProvider());
        refreshProviderList();
        mProviderList->setCurrentRow(mProviders.size() - 1);
        emit settingChanged();
    }
}

/** @brief 修改当前选中供应商：弹 DAProviderEditDialog（预填） */
void DAAgentSettingsWidget::onEditProvider()
{
    int idx = mProviderList->currentRow();
    if (idx < 0 || idx >= mProviders.size()) return;
    QString oldName = mProviders.at(idx).name;
    QStringList existing = collectProviderNames(idx);
    DAProviderEditDialog dlg(mProviders.at(idx), existing, oldName, this);
    if (dlg.exec() == QDialog::Accepted) {
        mProviders.replace(idx, dlg.getProvider());
        refreshProviderList();
        mProviderList->setCurrentRow(idx);
        showProviderInfo(idx);
        emit settingChanged();
    }
}

/** @brief 删除当前选中供应商 */
void DAAgentSettingsWidget::onRemoveProvider()
{
    int idx = mProviderList->currentRow();
    if (idx < 0 || idx >= mProviders.size()) return;
    mProviders.removeAt(idx);
    refreshProviderList();
    int next = qMin(idx, mProviders.size() - 1);
    if (next >= 0) {
        mProviderList->setCurrentRow(next);
    } else {
        showProviderInfo(-1);
    }
    emit settingChanged();
}

/** @brief 供应商列表选中变化：刷新右侧只读信息 */
void DAAgentSettingsWidget::onProviderSelected(int row)
{
    showProviderInfo(row);
}

// ===========================================================================
// 供应商辅助
// ===========================================================================

/** @brief 用 mProviders 刷新供应商列表 */
void DAAgentSettingsWidget::refreshProviderList()
{
    QSignalBlocker blocker(mProviderList);
    mProviderList->clear();
    for (const DAAgentProvider& p : std::as_const(mProviders)) {
        QString name = p.name;
        if (name.isEmpty()) name = tr("(unnamed)");  //cn:（未命名）
        new QListWidgetItem(name, mProviderList);
    }
}

/** @brief 显示选中供应商的只读信息（名称/base_url/api_key 脱敏/模型表格） */
void DAAgentSettingsWidget::showProviderInfo(int idx)
{
    mInfoModelTable->setRowCount(0);
    if (idx < 0 || idx >= mProviders.size()) {
        mInfoName->clear();
        mInfoBaseUrl->clear();
        mInfoApiKey->clear();
        return;
    }
    const DAAgentProvider& p = mProviders.at(idx);
    mInfoName->setText(p.name);
    mInfoBaseUrl->setText(p.baseUrl);
    // api_key 脱敏：仅显示是否已设置（不显示明文/掩码）
    if (p.apiKey.isEmpty()) {
        mInfoApiKey->setText(tr("not set"));  //cn:未设置
    } else {
        mInfoApiKey->setText(tr("set (hidden)"));  //cn:已设置(隐藏)
    }
    for (const DAAgentModel& m : p.models) {
        int row = mInfoModelTable->rowCount();
        mInfoModelTable->insertRow(row);
        mInfoModelTable->setItem(row, 0, new QTableWidgetItem(m.id));
        mInfoModelTable->setItem(row, 1, new QTableWidgetItem(QString::number(m.contextWindow)));
        mInfoModelTable->setItem(row, 2, new QTableWidgetItem(QString::number(m.maxOutputTokens)));
    }
}

// ===========================================================================
// 配置加载/保存
// ===========================================================================

/** @brief 从接口加载配置到界面 */
void DAAgentSettingsWidget::loadConfig()
{
    if (!mAgentInterface) {
        daDebug << "[DAAgentSettings] loadConfig skipped: no agent interface injected";
        return;
    }
    mProviders = mAgentInterface->getProviders();
    refreshProviderList();
    if (mProviders.size() > 0) {
        mProviderList->setCurrentRow(0);
    } else {
        showProviderInfo(-1);
    }
    const DAAgentLLMConfig c = mAgentInterface->getLLMConfig();
    mReadyTimeoutSpin->setValue(c.readyTimeoutSec());
    mStopTimeoutSpin->setValue(c.stopTimeoutSec());
    mCompactionThresholdSpin->setValue(c.compactionThreshold());
    mMaxRecentMsgSpin->setValue(c.maxRecentMessages());
    mToolResultMaxCharsSpin->setValue(c.toolResultMaxChars());
    mToolResultPreviewCharsSpin->setValue(c.toolResultPreviewChars());
    mMaxSessionsSpin->setValue(c.maxSessions());
    mSessionRetentionDaysSpin->setValue(c.sessionRetentionDays());
    mSpinMaxRetries->setValue(c.maxRetries());
    mSpinRetryInterval->setValue(c.retryIntervalSec());
    mSpinRetryIncrement->setValue(c.retryIntervalIncrementSec());
    mSpinRequestTimeout->setValue(c.requestTimeoutSec());
    mSpinInactivityTimeout->setValue(c.inactivityTimeoutSec());
    mSpinMaxRestarts->setValue(c.maxSubprocessRestarts());
    // 推理上限：≤0（-1=不限制）→ 勾选并禁用 SpinBox；>0 正常回显。
    // blockSignals 防止勾选触发的 setValue 联动误发 settingChanged
    const int recursionVal = c.recursionLimit();
    mCheckRecursionNoLimit->blockSignals(true);
    mCheckRecursionNoLimit->setChecked(recursionVal <= 0);
    mCheckRecursionNoLimit->blockSignals(false);
    mSpinRecursionLimit->setEnabled(recursionVal > 0);
    mSpinRecursionLimit->setValue(recursionVal > 0 ? recursionVal : -1);
    mSpinSubagentTimeout->setValue(c.subagentTimeoutSec());
    mSpinSubagentRecursionLimit->setValue(c.subagentRecursionLimit());
    mCheckAutoPrestart->setChecked(c.autoPrestart());
}

/** @brief 将界面配置保存到接口 */
void DAAgentSettingsWidget::saveConfig()
{
    if (!mAgentInterface) {
        daDebug << "[DAAgentSettings] saveConfig skipped: no agent interface injected";
        return;
    }
    // 保存供应商（api_key 明文传入，持久化时由配置层加密）
    mAgentInterface->setProviders(mProviders);
    // 保存其它设置（不含 context_window/max_output_tokens，由激活模型派生）
    DAAgentLLMConfig c;
    c.setReadyTimeoutSec(mReadyTimeoutSpin->value());
    c.setStopTimeoutSec(mStopTimeoutSpin->value());
    c.setCompactionThreshold(mCompactionThresholdSpin->value());
    c.setMaxRecentMessages(mMaxRecentMsgSpin->value());
    c.setToolResultMaxChars(mToolResultMaxCharsSpin->value());
    c.setToolResultPreviewChars(mToolResultPreviewCharsSpin->value());
    c.setMaxSessions(mMaxSessionsSpin->value());
    c.setSessionRetentionDays(mSessionRetentionDaysSpin->value());
    c.setMaxRetries(mSpinMaxRetries->value());
    c.setRetryIntervalSec(mSpinRetryInterval->value());
    c.setRetryIntervalIncrementSec(mSpinRetryIncrement->value());
    c.setRequestTimeoutSec(mSpinRequestTimeout->value());
    c.setInactivityTimeoutSec(mSpinInactivityTimeout->value());
    c.setMaxSubprocessRestarts(mSpinMaxRestarts->value());
    c.setRecursionLimit(mSpinRecursionLimit->value());
    c.setSubagentTimeoutSec(mSpinSubagentTimeout->value());
    c.setSubagentRecursionLimit(mSpinSubagentRecursionLimit->value());
    c.setAutoPrestart(mCheckAutoPrestart->isChecked());
    mAgentInterface->setLLMConfig(c);
}

/** @brief 应用设置页的配置变更 */
void DAAgentSettingsWidget::apply()
{
    daDebug << "[DAAgentSettings] apply entered";
    saveConfig();
    emit settingApplyed();
    daDebug << "[DAAgentSettings] apply done";
}

/** @brief 获取设置页图标 */
QIcon DAAgentSettingsWidget::getSettingPageIcon() const
{
    return QIcon(":/DAGui/icon/setting-agent.svg");
}

} // namespace DA
