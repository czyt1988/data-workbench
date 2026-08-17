// DAAgentSettingsWidget.cpp
// 持久化经 mAgentInterface->getProviders/setProviders（供应商，api_key 内部加解密）
// 与 get/setLLMConfig（其它设置）走 agent-config.ini。设置页只传明文。
#include "DAAgentSettingsWidget.h"
#include "DALogCategory.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QTimer>
#include <QJsonArray>
#include <QUrl>
#include <QLatin1String>
#include <QSignalBlocker>
#include <QListWidgetItem>

namespace {
// Qt5/Qt6 双兼容：QJsonObject::value(key, default) 在 Qt5 不存在（Qt5 的 value 只接受 1 个参数），
// 故用这两个 helper 提供"取值并兜底默认值"语义。
int jsonInt(const QJsonObject& o, const char* key, int def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isDouble() ? v.toInt() : def;
}

/** @brief 从 QJsonObject 中读取 double 值并兜底默认值 */
double jsonDouble(const QJsonObject& o, const char* key, double def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isDouble() ? v.toDouble() : def;
}

/** @brief 从 QJsonObject 中读取 bool 值并兜底默认值 */
bool jsonBool(const QJsonObject& o, const char* key, bool def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isBool() ? v.toBool() : def;
}
}  // namespace

namespace DA
{

/** @brief 构造 Agent 设置页控件 */
DAAgentSettingsWidget::DAAgentSettingsWidget(QWidget* parent)
    : DAAbstractSettingPage(parent)
{
    setupUI();
    // QNetworkAccessManager 在构造函数中创建（父子对象托管），避免 onTestConnection 空指针解引用
    mNetworkManager = new QNetworkAccessManager(this);
}

/** @brief 注入 Agent 接口并加载已有配置 */
void DAAgentSettingsWidget::setAgentInterface(DAAgentInterface* p)
{
    mAgentInterface = p;
    if (p) {
        // 接口注入后立即加载配置（构造函数中接口未注入，loadConfig 被跳过）。
        // QSignalBlocker 防止 load 期间 setValue 触发 settingChanged 误标脏页。
        QSignalBlocker blocker(this);
        loadConfig();
    }
}

/** @brief 构建设置页界面控件与布局（QTabWidget 两页） */
void DAAgentSettingsWidget::setupUI()
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    mTabWidget = new QTabWidget(this);
    root->addWidget(mTabWidget);

    setupProvidersTab();
    setupAgentSettingsTab();
}

/** @brief 构建供应商管理 Tab */
void DAAgentSettingsWidget::setupProvidersTab()
{
    QWidget* page = new QWidget(this);
    QGridLayout* grid = new QGridLayout(page);
    grid->setContentsMargins(8, 8, 8, 8);
    grid->setSpacing(6);

    // 左：供应商列表 + 增删按钮
    mProviderList = new QListWidget(page);
    mProviderList->setToolTip(tr("Configured LLM providers. Select one to edit its connection and models."));  //cn:已配置的 LLM 供应商，选中可编辑其连接与模型
    mAddProviderBtn = new QPushButton(tr("+ Add Provider"), page);  //cn:+ 新增供应商
    mRemoveProviderBtn = new QPushButton(tr("- Remove"), page);     //cn:- 删除
    QVBoxLayout* provBtnLayout = new QVBoxLayout();
    provBtnLayout->setSpacing(4);
    provBtnLayout->addWidget(mAddProviderBtn);
    provBtnLayout->addWidget(mRemoveProviderBtn);
    provBtnLayout->addStretch();
    QWidget* provBtnPanel = new QWidget(page);
    provBtnPanel->setLayout(provBtnLayout);

    // 右：选中供应商的编辑表单
    mProviderNameEdit = new QLineEdit(page);
    mProviderNameEdit->setToolTip(tr("Provider display name, e.g. 'OpenAI', 'DeepSeek', 'Qwen'."));  //cn:供应商显示名，如 OpenAI/DeepSeek/Qwen
    mProviderBaseUrlEdit = new QLineEdit(page);
    mProviderBaseUrlEdit->setToolTip(tr(
        "OpenAI-compatible API base URL. The system appends '/chat/completions' "
        "automatically, so only provide the base path. "
        "Recommended: 'https://api.openai.com/v1' or your provider's equivalent. "
        "Must end with a version path (e.g. /v1), not with '/chat/completions'."));  //cn:OpenAI 兼容 API 基础地址。系统会自动拼接 '/chat/completions'，故只需填基础路径。建议 'https://api.openai.com/v1' 或服务商等价地址。须以版本路径(如 /v1)结尾,不要以 '/chat/completions' 结尾。
    mProviderApiKeyEdit = new QLineEdit(page);
    mProviderApiKeyEdit->setEchoMode(QLineEdit::Password);
    mProviderApiKeyEdit->setToolTip(tr(
        "Authentication key for this provider. Masked here and encrypted at rest with DPAPI on Windows. "
        "Pass an empty string to clear the stored key."));  //cn:该供应商身份认证密钥。界面密文显示,Windows 下本地用 DPAPI 加密存储。传空串可清除已存密钥。
    QFormLayout* provForm = new QFormLayout();
    provForm->setSpacing(4);
    provForm->addRow(tr("Name"), mProviderNameEdit);      //cn:名称
    provForm->addRow(tr("Base URL"), mProviderBaseUrlEdit);  //cn:基础地址
    provForm->addRow(tr("API Key"), mProviderApiKeyEdit);    //cn:API 密钥

    // 模型列表 + 增删
    mModelList = new QListWidget(page);
    mModelList->setToolTip(tr("Model ids available under this provider, e.g. 'gpt-4o', 'deepseek-chat'."));  //cn:该供应商下可用模型 id,如 gpt-4o/deepseek-chat
    mModelIdEdit = new QLineEdit(page);
    mModelIdEdit->setPlaceholderText(tr("new model id"));  //cn:新模型 id
    mAddModelBtn = new QPushButton(tr("Add Model"), page);  //cn:新增模型
    mRemoveModelBtn = new QPushButton(tr("Remove Model"), page);  //cn:删除模型
    QHBoxLayout* modelBtnRow = new QHBoxLayout();
    modelBtnRow->setSpacing(4);
    modelBtnRow->addWidget(mModelIdEdit, 1);
    modelBtnRow->addWidget(mAddModelBtn);
    modelBtnRow->addWidget(mRemoveModelBtn);
    QVBoxLayout* modelCol = new QVBoxLayout();
    modelCol->setSpacing(4);
    modelCol->addLayout(provForm);
    QLabel* modelsTitle = new QLabel(tr("Models"), page);  //cn:模型
    modelCol->addWidget(modelsTitle);
    modelCol->addWidget(mModelList, 1);
    modelCol->addLayout(modelBtnRow);

    // 测试连接
    mTestBtn = new QPushButton(tr("Test Connection"), page);  //cn:测试连接
    mStatusLabel = new QLabel(page);
    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->setSpacing(6);
    rightCol->addLayout(modelCol);
    QHBoxLayout* testRow = new QHBoxLayout();
    testRow->addWidget(mTestBtn);
    testRow->addWidget(mStatusLabel, 1);
    rightCol->addLayout(testRow);
    rightCol->addStretch();
    QWidget* rightPanel = new QWidget(page);
    rightPanel->setLayout(rightCol);

    grid->addWidget(mProviderList, 0, 0, 1, 1);
    grid->addWidget(provBtnPanel, 1, 0, 1, 1);
    grid->addWidget(rightPanel, 0, 1, 2, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 2);

    mTabWidget->addTab(page, tr("Model Providers"));  //cn:模型供应商

    // 信号
    connect(mProviderList, &QListWidget::currentRowChanged, this, &DAAgentSettingsWidget::onProviderSelected);
    connect(mAddProviderBtn, &QPushButton::clicked, this, &DAAgentSettingsWidget::onAddProvider);
    connect(mRemoveProviderBtn, &QPushButton::clicked, this, &DAAgentSettingsWidget::onRemoveProvider);
    connect(mAddModelBtn, &QPushButton::clicked, this, &DAAgentSettingsWidget::onAddModel);
    connect(mRemoveModelBtn, &QPushButton::clicked, this, &DAAgentSettingsWidget::onRemoveModel);
    connect(mTestBtn, &QPushButton::clicked, this, &DAAgentSettingsWidget::onTestConnection);
    // 名称编辑即时回写内存（刷新列表显示名）
    connect(mProviderNameEdit, &QLineEdit::textChanged, this, &DAAgentSettingsWidget::onProviderNameEdited);

    // 字段变更连接到 settingChanged()，让平台 applyChanged() 机制感知脏页
    connect(mProviderBaseUrlEdit, &QLineEdit::textChanged, this, [this]() { emit settingChanged(); });
    connect(mProviderApiKeyEdit, &QLineEdit::textChanged, this, [this]() { emit settingChanged(); });
    connect(mProviderNameEdit, &QLineEdit::textChanged, this, [this]() { emit settingChanged(); });
    // 模型列表增删也标记脏页（onAddModel/onRemoveModel 内部已处理，此处补充列表模型变更信号）
    connect(mModelList, &QListWidget::itemChanged, this, [this]() { emit settingChanged(); });
}

/** @brief 构建 Agent 其它设置 Tab（超时/上下文/会话/容错/预启动） */
void DAAgentSettingsWidget::setupAgentSettingsTab()
{
    QWidget* page = new QWidget(this);
    QFormLayout* form = new QFormLayout(page);

    // 超时设置
    mReadyTimeoutSpin = new QSpinBox(this);
    mReadyTimeoutSpin->setRange(5, 300);
    mReadyTimeoutSpin->setSuffix(tr(" s"));  //cn:秒
    mReadyTimeoutSpin->setToolTip(tr(
        "Waiting time (seconds) for the agent subprocess to become ready after start. "
        "Cold start imports of langchain may take ~17s. "
        "Recommended: 60 (safe margin over cold start)."));  //cn:agent 子进程启动后等待就绪的超时(秒)。冷启动导入 langchain 约 17s。建议 60。
    mReadyTimeoutSpin->setValue(60);
    mStopTimeoutSpin = new QSpinBox(this);
    mStopTimeoutSpin->setRange(1, 60);
    mStopTimeoutSpin->setSuffix(tr(" s"));  //cn:秒
    mStopTimeoutSpin->setToolTip(tr(
        "Waiting time (seconds) for the agent subprocess to exit when stopped. "
        "If the process does not exit within this period it is force-killed. Recommended: 5."));  //cn:停止 agent 时等待子进程退出的超时(秒),超时则强制终止。建议 5。
    mStopTimeoutSpin->setValue(5);

    // 上下文管理
    mContextWindowSpin = new QSpinBox(this);
    mContextWindowSpin->setRange(8192, 2097152);
    mContextWindowSpin->setSingleStep(1024);
    mContextWindowSpin->setSuffix(tr(" tokens"));
    mContextWindowSpin->setToolTip(tr(
        "Maximum context window of the LLM model in tokens. "
        "Set this to match the model's actual context window "
        "(e.g. 128000 for GPT-4o, 256000 for qwen3-coder). Default 262144 (256K)."));  //cn:LLM 模型最大上下文窗口(tokens),须与模型实际窗口匹配。默认 262144(256K)。
    mContextWindowSpin->setValue(262144);

    mCompactionThresholdSpin = new QDoubleSpinBox(this);
    mCompactionThresholdSpin->setRange(0.50, 1.0);
    mCompactionThresholdSpin->setSingleStep(0.05);
    mCompactionThresholdSpin->setDecimals(2);
    mCompactionThresholdSpin->setToolTip(tr(
        "Compaction trigger ratio: when used tokens reach this fraction of the "
        "context window, older messages are summarized. 0.85 = compact at 85%."));  //cn:压缩触发比例:已用 token 达到窗口的此比例时触发压缩。0.85=窗口 85% 时触发。
    mCompactionThresholdSpin->setValue(0.85);

    mMaxRecentMsgSpin = new QSpinBox(this);
    mMaxRecentMsgSpin->setRange(4, 50);
    mMaxRecentMsgSpin->setToolTip(tr(
        "Number of most recent messages retained verbatim after compaction. Recommended: 10."));  //cn:压缩后保留为原文的最近消息条数。建议 10。
    mMaxRecentMsgSpin->setValue(10);

    mToolResultMaxCharsSpin = new QSpinBox(this);
    mToolResultMaxCharsSpin->setRange(1000, 500000);
    mToolResultMaxCharsSpin->setSingleStep(1000);
    mToolResultMaxCharsSpin->setToolTip(tr(
        "Character threshold: tool outputs longer than this are truncated to a preview. Recommended: 20000."));  //cn:字符阈值:工具输出超此长度则截断为预览。建议 20000。
    mToolResultMaxCharsSpin->setValue(20000);

    mToolResultPreviewCharsSpin = new QSpinBox(this);
    mToolResultPreviewCharsSpin->setRange(100, 10000);
    mToolResultPreviewCharsSpin->setSingleStep(100);
    mToolResultPreviewCharsSpin->setToolTip(tr(
        "Preview length (characters) shown when a tool output is truncated. Recommended: 2000."));  //cn:工具输出被截断时显示的预览长度(字符)。建议 2000。
    mToolResultPreviewCharsSpin->setValue(2000);

    // 会话持久化
    mMaxSessionsSpin = new QSpinBox(this);
    mMaxSessionsSpin->setRange(5, 200);
    mMaxSessionsSpin->setValue(20);
    mMaxSessionsSpin->setToolTip(tr(
        "Maximum number of free (unsaved) sessions retained. Recommended: 20."));  //cn:保留的自由(未另存)会话最大数量。建议 20。
    mSessionRetentionDaysSpin = new QSpinBox(this);
    mSessionRetentionDaysSpin->setRange(1, 365);
    mSessionRetentionDaysSpin->setValue(30);
    mSessionRetentionDaysSpin->setSuffix(tr(" d"));  //cn:天
    mSessionRetentionDaysSpin->setToolTip(tr(
        "Free (unsaved) sessions older than this many days are deleted on startup. Recommended: 30."));  //cn:早于此天数的自由会话在启动时删除。建议 30。

    // 重连与容错
    mSpinMaxRetries = new QSpinBox(this);
    mSpinMaxRetries->setRange(0, 20);
    mSpinMaxRetries->setSuffix(tr(" times"));  //cn:次
    mSpinMaxRetries->setToolTip(tr(
        "Maximum automatic retries for LLM API calls on transient errors. 0 disables retry. Recommended: 7."));  //cn:LLM API 调用临时错误的自动重试次数。0 表示不重试。建议 7。
    mSpinMaxRetries->setValue(7);

    mSpinRequestTimeout = new QSpinBox(this);
    mSpinRequestTimeout->setRange(10, 600);
    mSpinRequestTimeout->setSuffix(tr(" sec"));  //cn:秒
    mSpinRequestTimeout->setToolTip(tr(
        "Timeout (seconds) for a single LLM API request. Recommended: 120."));  //cn:单次 LLM API 请求超时(秒)。建议 120。
    mSpinRequestTimeout->setValue(120);

    mSpinInactivityTimeout = new QSpinBox(this);
    mSpinInactivityTimeout->setRange(60, 600);
    mSpinInactivityTimeout->setSuffix(tr(" sec"));  //cn:秒
    mSpinInactivityTimeout->setToolTip(tr(
        "Watchdog timeout (seconds): if no protocol message is received, the subprocess is stopped. Recommended: 240."));  //cn:看门狗超时(秒):此时间内无消息则停止子进程。建议 240。
    mSpinInactivityTimeout->setValue(240);

    mSpinMaxRestarts = new QSpinBox(this);
    mSpinMaxRestarts->setRange(0, 10);
    mSpinMaxRestarts->setSuffix(tr(" times"));  //cn:次
    mSpinMaxRestarts->setToolTip(tr(
        "Maximum automatic restarts after the subprocess crashes. 0 disables. Recommended: 3."));  //cn:子进程崩溃后自动重启最大次数。0 表示不自动重启。建议 3。
    mSpinMaxRestarts->setValue(3);

    mSpinRecursionLimit = new QSpinBox(this);
    mSpinRecursionLimit->setRange(20, 1000);
    mSpinRecursionLimit->setToolTip(tr(
        "Maximum graph reasoning steps (compact->agent->tools cycle). Each tool-call cycle consumes 3 steps. Recommended: 150."));  //cn:图最大推理步数(compact→agent→tools 循环)。每轮工具调用耗 3 步。建议 150。
    mSpinRecursionLimit->setValue(150);

    mCheckAutoPrestart = new QCheckBox(this);
    mCheckAutoPrestart->setToolTip(tr(
        "Start the agent subprocess in the background when the program launches, so chat is ready immediately. "
        "Disable to save memory when you rarely use the agent."));  //cn:程序启动时后台预启动 agent 子进程,使对话立即可用。关闭可节省内存。
    mCheckAutoPrestart->setChecked(true);

    form->addRow(tr("Ready Timeout"), mReadyTimeoutSpin);  //cn:就绪超时
    form->addRow(tr("Stop Timeout"),  mStopTimeoutSpin);    //cn:停止超时
    form->addRow(tr("Context Window"), mContextWindowSpin);              //cn:上下文窗口
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

    // 字段变更 → settingChanged
    auto mark = [this]() { emit settingChanged(); };
    connect(mReadyTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mStopTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mContextWindowSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
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
// 供应商管理辅助
// ===========================================================================

/** @brief 用 mProviders 刷新供应商列表 */
void DAAgentSettingsWidget::refreshProviderList()
{
    QSignalBlocker blocker(mProviderList);  // 避免 currentRowChanged 误触发
    mProviderList->clear();
    for (int i = 0; i < mProviders.size(); ++i) {
        QString name = mProviders.at(i).toObject().value("name").toString();
        if (name.isEmpty()) name = tr("(unnamed)");  //cn:（未命名）
        QListWidgetItem* item = new QListWidgetItem(name, mProviderList);
        item->setData(Qt::UserRole, i);
    }
}

/** @brief 从 mProviders[idx] 载入表单 */
void DAAgentSettingsWidget::loadProviderToForm(int idx)
{
    mCurrentProviderIdx = idx;
    if (idx < 0 || idx >= mProviders.size()) {
        mProviderNameEdit->clear();
        mProviderBaseUrlEdit->clear();
        mProviderApiKeyEdit->clear();
        mModelList->clear();
        return;
    }
    QJsonObject p = mProviders.at(idx).toObject();
    QSignalBlocker bn(mProviderNameEdit);
    QSignalBlocker bb(mProviderBaseUrlEdit);
    QSignalBlocker bk(mProviderApiKeyEdit);
    QSignalBlocker bm(mModelList);
    mProviderNameEdit->setText(p.value("name").toString());
    mProviderBaseUrlEdit->setText(p.value("base_url").toString());
    mProviderApiKeyEdit->setText(p.value("api_key").toString());
    mModelList->clear();
    const QJsonArray models = p.value("models").toArray();
    for (const QJsonValue& mv : models) {
        new QListWidgetItem(mv.toString(), mModelList);
    }
}

/** @brief 把当前表单回写到 mProviders[idx] */
void DAAgentSettingsWidget::saveFormToProvider(int idx)
{
    if (idx < 0 || idx >= mProviders.size()) return;
    QJsonObject p = mProviders.at(idx).toObject();
    p["name"]     = mProviderNameEdit->text().trimmed();
    p["base_url"] = mProviderBaseUrlEdit->text().trimmed();
    p["api_key"]  = mProviderApiKeyEdit->text();  // 明文,接口内部加密
    // 模型列表由 onAddModel/onRemoveModel 即时维护 mProviders，此处不重建
    mProviders.replace(idx, p);
}

/** @brief 当前选中供应商行号 */
int DAAgentSettingsWidget::currentProviderRow() const
{
    int row = mProviderList->currentRow();
    if (row < 0 && mProviderList->count() > 0) row = 0;
    return row;
}

/** @brief 当前选中模型行号 */
int DAAgentSettingsWidget::currentModelIndex() const
{
    int row = mModelList->currentRow();
    if (row < 0 && mModelList->count() > 0) row = 0;
    return row;
}

// ===========================================================================
// 供应商/模型 CRUD 槽
// ===========================================================================

/** @brief 新增供应商：追加空供应商并选中 */
void DAAgentSettingsWidget::onAddProvider()
{
    // 切换前先保存当前表单
    saveFormToProvider(mCurrentProviderIdx);
    QJsonObject p;
    p["name"]     = tr("New Provider");  //cn:新供应商
    p["base_url"] = QString();
    p["api_key"]  = QString();
    p["models"]   = QJsonArray();
    mProviders.append(p);
    refreshProviderList();
    mProviderList->setCurrentRow(mProviders.size() - 1);
    emit settingChanged();
}

/** @brief 删除当前选中供应商 */
void DAAgentSettingsWidget::onRemoveProvider()
{
    int row = mProviderList->currentRow();
    if (row < 0 || row >= mProviders.size()) return;
    mProviders.removeAt(row);
    refreshProviderList();
    int next = qMin(row, mProviders.size() - 1);
    if (next >= 0) {
        mProviderList->setCurrentRow(next);
    } else {
        loadProviderToForm(-1);
    }
    emit settingChanged();
}

/** @brief 供应商列表选中变化：先保存旧表单，再载入新表单 */
void DAAgentSettingsWidget::onProviderSelected(int row)
{
    if (row < 0) return;
    saveFormToProvider(mCurrentProviderIdx);  // 保存离开的供应商
    loadProviderToForm(row);
}

/** @brief 供应商名称编辑：即时回写 mProviders + 刷新列表显示名 */
void DAAgentSettingsWidget::onProviderNameEdited(const QString& text)
{
    if (mCurrentProviderIdx < 0 || mCurrentProviderIdx >= mProviders.size()) return;
    QJsonObject p = mProviders.at(mCurrentProviderIdx).toObject();
    p["name"] = text.trimmed();
    mProviders.replace(mCurrentProviderIdx, p);
    // 刷新列表项显示名（不重建列表，避免丢失选中）
    QListWidgetItem* item = mProviderList->item(mCurrentProviderIdx);
    if (item) {
        item->setText(text.trimmed().isEmpty() ? tr("(unnamed)") : text.trimmed());  //cn:（未命名）
    }
}

/** @brief 新增模型 id 到当前供应商 */
void DAAgentSettingsWidget::onAddModel()
{
    if (mCurrentProviderIdx < 0 || mCurrentProviderIdx >= mProviders.size()) return;
    QString mid = mModelIdEdit->text().trimmed();
    if (mid.isEmpty()) return;
    // 去重
    QJsonObject p = mProviders.at(mCurrentProviderIdx).toObject();
    QJsonArray models = p.value("models").toArray();
    for (const QJsonValue& mv : models) {
        if (mv.toString() == mid) {
            mModelIdEdit->clear();
            return;  // 已存在
        }
    }
    models.append(mid);
    p["models"] = models;
    mProviders.replace(mCurrentProviderIdx, p);
    new QListWidgetItem(mid, mModelList);
    mModelIdEdit->clear();
    emit settingChanged();
}

/** @brief 删除当前选中模型 */
void DAAgentSettingsWidget::onRemoveModel()
{
    if (mCurrentProviderIdx < 0 || mCurrentProviderIdx >= mProviders.size()) return;
    int row = mModelList->currentRow();
    if (row < 0) return;
    QJsonObject p = mProviders.at(mCurrentProviderIdx).toObject();
    QJsonArray models = p.value("models").toArray();
    if (row >= models.size()) return;
    models.removeAt(row);
    p["models"] = models;
    mProviders.replace(mCurrentProviderIdx, p);
    delete mModelList->takeItem(row);
    emit settingChanged();
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
    // 供应商
    mProviders = mAgentInterface->getProviders();
    refreshProviderList();
    if (mProviders.size() > 0) {
        mProviderList->setCurrentRow(0);  // 触发 onProviderSelected 载入表单
    } else {
        loadProviderToForm(-1);
    }
    // 其它设置：经 getLLMConfig（不含 base_url/api_key/model，这些由供应商管理）
    QJsonObject c = mAgentInterface->getLLMConfig();
    mReadyTimeoutSpin->setValue(jsonInt(c, "ready_timeout_sec", 60));
    mStopTimeoutSpin->setValue(jsonInt(c, "stop_timeout_sec", 5));
    mContextWindowSpin->setValue(jsonInt(c, "context_window", 262144));
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
    // 保存当前编辑中的供应商表单回内存
    saveFormToProvider(mCurrentProviderIdx);
    // 保存供应商（api_key 明文传入，接口内部加密）
    mAgentInterface->setProviders(mProviders);
    // 保存其它设置（不含 base_url/api_key/model，由供应商激活连接派生）
    QJsonObject c;
    c["ready_timeout_sec"]         = mReadyTimeoutSpin->value();
    c["stop_timeout_sec"]          = mStopTimeoutSpin->value();
    c["context_window"]            = mContextWindowSpin->value();
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

/** @brief 测试当前选中供应商的连接（用其首个模型） */
void DAAgentSettingsWidget::onTestConnection()
{
    saveFormToProvider(mCurrentProviderIdx);
    if (mCurrentProviderIdx < 0 || mCurrentProviderIdx >= mProviders.size()) {
        mStatusLabel->setStyleSheet("color: red;");
        mStatusLabel->setText(tr("Select a provider first"));  //cn:请先选择一个供应商
        return;
    }
    QJsonObject p = mProviders.at(mCurrentProviderIdx).toObject();
    QString baseUrl = p.value("base_url").toString().trimmed();
    QString apiKey  = p.value("api_key").toString().trimmed();
    QJsonArray models = p.value("models").toArray();
    QString model = models.isEmpty() ? QString() : models.first().toString().trimmed();
    if (baseUrl.isEmpty() || model.isEmpty()) {
        mStatusLabel->setStyleSheet("color: red;");
        mStatusLabel->setText(tr("Base URL and at least one model are required"));  //cn:需要基础地址和至少一个模型
        return;
    }
    while (baseUrl.endsWith('/')) baseUrl.chop(1);

    QNetworkRequest request(QUrl(baseUrl + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QJsonObject body;
    body["model"]       = model;
    body["max_tokens"]  = 5;
    QJsonArray messages;
    QJsonObject msg;
    msg["role"]    = "user";
    msg["content"] = "Hi";
    messages.append(msg);
    body["messages"] = messages;

    mTestBtn->setEnabled(false);
    mStatusLabel->setText(tr("Testing..."));  //cn:测试中...
    QNetworkReply* reply = mNetworkManager->post(request, QJsonDocument(body).toJson());

    QTimer::singleShot(10000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, model]() {
        mTestBtn->setEnabled(true);
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
            QString modelUsed = resp.value("model").toString(model);
            mStatusLabel->setStyleSheet("color: green;");
            mStatusLabel->setText(tr("✓ Connected (%1)").arg(modelUsed));  //cn:✓ 连接成功 (%1)
        } else {
            mStatusLabel->setStyleSheet("color: red;");
            mStatusLabel->setText(tr("✗ Connection failed: %1").arg(reply->errorString()));  //cn:✗ 连接失败: %1
        }
        reply->deleteLater();
    });
}

} // namespace DA
