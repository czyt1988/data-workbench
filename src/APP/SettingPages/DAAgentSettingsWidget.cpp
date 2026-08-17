// DAAgentSettingsWidget.cpp
// 持久化经 mAgentInterface->getProviders/setProviders（供应商，api_key 内部加解密）
// 与 get/setLLMConfig（其它设置）走 agent-config.ini。设置页只传明文。
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

namespace {
// Qt5/Qt6 双兼容 helper：取值并兜底默认值（QJsonObject::value(key,default) Qt5 不存在）
int jsonInt(const QJsonObject& o, const char* key, int def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isDouble() ? v.toInt() : def;
}

double jsonDouble(const QJsonObject& o, const char* key, double def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isDouble() ? v.toDouble() : def;
}

bool jsonBool(const QJsonObject& o, const char* key, bool def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isBool() ? v.toBool() : def;
}

// 取模型 id（兼容字符串与对象格式）
QString modelIdOf(const QJsonValue& mv)
{
    if (mv.isString()) return mv.toString();
    if (mv.isObject()) return mv.toObject().value("id").toString();
    return {};
}
}  // namespace

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
    mInfoModelTable->setHorizontalHeaderLabels({ tr("Model Name"), tr("Context Size"), tr("Max Output Tokens") });  // cn:模型名//上下文大小//最大输出 token
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

    mSpinMaxRetries = new QSpinBox(this);
    mSpinMaxRetries->setRange(0, 20);
    mSpinMaxRetries->setSuffix(tr(" times"));  //cn:次
    mSpinMaxRetries->setToolTip(tr("Max automatic retries on transient LLM errors. 0 disables. Recommended: 7."));  //cn:临时错误自动重试次数。0 不重试。建议 7。
    mSpinMaxRetries->setValue(7);

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
    mSpinRecursionLimit->setRange(20, 1000);
    mSpinRecursionLimit->setToolTip(tr("Max graph reasoning steps. Each tool-call cycle consumes 3 steps. Recommended: 150."));  //cn:图最大推理步数。每轮工具调用耗 3 步。建议 150。
    mSpinRecursionLimit->setValue(150);

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
    form->addRow(tr("Request timeout"), mSpinRequestTimeout);  //cn:请求超时
    form->addRow(tr("Inactivity timeout"), mSpinInactivityTimeout);  //cn:无活动超时
    form->addRow(tr("Max process restarts"), mSpinMaxRestarts);  //cn:最大进程重启次数
    form->addRow(tr("Reasoning iteration limit"), mSpinRecursionLimit);  //cn:推理迭代上限
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
    connect(mSpinRequestTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinInactivityTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinMaxRestarts, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mSpinRecursionLimit, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
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
        names.append(mProviders.at(i).toObject().value("name").toString());
    }
    return names;
}

/** @brief 新增供应商：弹 DAProviderEditDialog（空），accept 后加入内存并刷新 */
void DAAgentSettingsWidget::onAddProvider()
{
    QStringList existing = collectProviderNames();
    DAProviderEditDialog dlg(QJsonObject(), existing, QString(), this);
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
    QJsonObject cur = mProviders.at(idx).toObject();
    QString oldName = cur.value("name").toString();
    QStringList existing = collectProviderNames(idx);
    DAProviderEditDialog dlg(cur, existing, oldName, this);
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
    for (int i = 0; i < mProviders.size(); ++i) {
        QString name = mProviders.at(i).toObject().value("name").toString();
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
    QJsonObject p = mProviders.at(idx).toObject();
    mInfoName->setText(p.value("name").toString());
    mInfoBaseUrl->setText(p.value("base_url").toString());
    // api_key 脱敏：仅显示是否已设置（不显示明文/掩码）
    QString key = p.value("api_key").toString();
    mInfoApiKey->setText(key.isEmpty() ? tr("not set") : tr("set (hidden)"));  //cn:未设置//已设置(隐藏)
    const QJsonArray models = p.value("models").toArray();
    for (const QJsonValue& mv : models) {
        int row = mInfoModelTable->rowCount();
        mInfoModelTable->insertRow(row);
        mInfoModelTable->setItem(row, 0, new QTableWidgetItem(modelIdOf(mv)));
        QJsonObject mo = mv.toObject();
        mInfoModelTable->setItem(row, 1, new QTableWidgetItem(QString::number(mo.value("context_window").toInt(262144))));
        mInfoModelTable->setItem(row, 2, new QTableWidgetItem(QString::number(mo.value("max_output_tokens").toInt(8192))));
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
    QJsonObject c = mAgentInterface->getLLMConfig();
    mReadyTimeoutSpin->setValue(jsonInt(c, "ready_timeout_sec", 60));
    mStopTimeoutSpin->setValue(jsonInt(c, "stop_timeout_sec", 5));
    mCompactionThresholdSpin->setValue(jsonDouble(c, "compaction_threshold", 0.85));
    mMaxRecentMsgSpin->setValue(jsonInt(c, "max_recent_messages", 10));
    mToolResultMaxCharsSpin->setValue(jsonInt(c, "tool_result_max_chars", 20000));
    mToolResultPreviewCharsSpin->setValue(jsonInt(c, "tool_result_preview_chars", 2000));
    mMaxSessionsSpin->setValue(jsonInt(c, "max_sessions", 20));
    mSessionRetentionDaysSpin->setValue(jsonInt(c, "session_retention_days", 30));
    mSpinMaxRetries->setValue(jsonInt(c, "max_retries", 7));
    mSpinRequestTimeout->setValue(jsonInt(c, "request_timeout_sec", 120));
    mSpinInactivityTimeout->setValue(jsonInt(c, "inactivity_timeout_sec", 240));
    mSpinMaxRestarts->setValue(jsonInt(c, "max_subprocess_restarts", 3));
    mSpinRecursionLimit->setValue(jsonInt(c, "recursion_limit", 150));
    mCheckAutoPrestart->setChecked(jsonBool(c, "auto_prestart", true));
}

/** @brief 将界面配置保存到接口 */
void DAAgentSettingsWidget::saveConfig()
{
    if (!mAgentInterface) {
        daDebug << "[DAAgentSettings] saveConfig skipped: no agent interface injected";
        return;
    }
    // 保存供应商（api_key 明文传入，接口内部加密）
    mAgentInterface->setProviders(mProviders);
    // 保存其它设置（不含 context_window/max_output_tokens，由激活模型派生）
    QJsonObject c;
    c["ready_timeout_sec"]         = mReadyTimeoutSpin->value();
    c["stop_timeout_sec"]          = mStopTimeoutSpin->value();
    c["compaction_threshold"]      = mCompactionThresholdSpin->value();
    c["max_recent_messages"]       = mMaxRecentMsgSpin->value();
    c["tool_result_max_chars"]     = mToolResultMaxCharsSpin->value();
    c["tool_result_preview_chars"] = mToolResultPreviewCharsSpin->value();
    c["max_sessions"]              = mMaxSessionsSpin->value();
    c["session_retention_days"]    = mSessionRetentionDaysSpin->value();
    c["max_retries"]              = mSpinMaxRetries->value();
    c["request_timeout_sec"]      = mSpinRequestTimeout->value();
    c["inactivity_timeout_sec"]   = mSpinInactivityTimeout->value();
    c["max_subprocess_restarts"]  = mSpinMaxRestarts->value();
    c["recursion_limit"]          = mSpinRecursionLimit->value();
    c["auto_prestart"]            = mCheckAutoPrestart->isChecked();
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
