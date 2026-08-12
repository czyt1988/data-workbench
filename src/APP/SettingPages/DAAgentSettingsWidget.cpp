// DAAgentSettingsWidget.cpp
// 持久化经 mAgentInterface->get/setLLMConfig 走 agent-config.ini（DAAgent 内部 DPAPI 加解密 api_key），
// 设置页只传明文 QJsonObject——不再持有 QSettings/DPAPI 代码（plan-01 加解密内化、plan-05 迁页）。
#include "DAAgentSettingsWidget.h"
#include "DALogCategory.h"
#include <QFormLayout>
#include <QTimer>
#include <QJsonArray>
#include <QUrl>
#include <QLatin1String>
#include <QSignalBlocker>

namespace {
// Qt5/Qt6 双兼容：QJsonObject::value(key, default) 在 Qt5 不存在（Qt5 的 value 只接受 1 个参数），
// 故用这两个 helper 提供"取值并兜底默认值"语义。getLLMConfig 本应总返回全部 key（带默认），
// 此处再兜一层防止接口实现退化时把 UI 默认值带偏。
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
    // loadConfig() 移到 setAgentInterface：构造时 mAgentInterface 尚未注入，
    // 此处调用只会因空指针守卫提前返回，移除以避免误导。
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

/** @brief 构建设置页界面控件与布局 */
void DAAgentSettingsWidget::setupUI()
{
    mBaseUrlEdit = new QLineEdit(this);
    mBaseUrlEdit->setToolTip(tr(
        "OpenAI-compatible API base URL. The system appends '/chat/completions' "
        "automatically, so only provide the base path. "
        "Recommended: 'https://api.openai.com/v1' or your provider's equivalent. "
        "Must end with a version path (e.g. /v1), not with '/chat/completions'."));  //cn:OpenAI 兼容 API 基础地址。系统会自动拼接 '/chat/completions'，故只需填基础路径。建议填 'https://api.openai.com/v1' 或服务商等价地址。须以版本路径(如 /v1)结尾,不要以 '/chat/completions' 结尾。
    mApiKeyEdit  = new QLineEdit(this);
    mApiKeyEdit->setEchoMode(QLineEdit::Password);   // API Key 密文显示
    mApiKeyEdit->setToolTip(tr(
        "Authentication key for the LLM provider. Obtain it from the provider's "
        "dashboard. It is masked here and encrypted at rest with DPAPI on Windows. "
        "Pass an empty string to clear the stored key. "
        "Avoid logging or sharing this value."));  //cn:LLM 服务商身份认证密钥,从服务商控制台获取。界面密文显示,Windows 下本地用 DPAPI 加密存储。传空串可清除已存密钥。请勿在日志或外部渠道泄露此值。
    mModelEdit   = new QLineEdit(this);
    mModelEdit->setToolTip(tr(
        "Model name to use for chat completions. Must be a model supported by "
        "the provider behind Base URL. "
        "Examples: 'gpt-4o', 'deepseek-chat', 'qwen3-coder'. "
        "A wrong model name usually yields a 404/error from the provider."));  //cn:用于对话补全的模型名称,须为 Base URL 服务商支持的模型。示例:'gpt-4o'、'deepseek-chat'、'qwen3-coder'。填错通常返回 404 或服务商错误。
    // 超时设置:可配,默认 ready=60s(覆盖 langchain 冷启动导入~17s)、stop=5s
    mReadyTimeoutSpin = new QSpinBox(this);
    mReadyTimeoutSpin->setRange(5, 300);
    mReadyTimeoutSpin->setSuffix(tr(" s"));  //cn:秒
    mReadyTimeoutSpin->setToolTip(tr(
        "Waiting time (seconds) for the agent subprocess to become ready after start. "
        "Cold start imports of langchain may take ~17s. "
        "Recommended: 60 (safe margin over cold start). "
        "Larger: more tolerant of slow machines / first launch, but you wait longer "
        "when the process is actually hung. "
        "Smaller: detects startup failure faster, but may kill a normal cold start "
        "that is still importing dependencies."));  //cn:agent 子进程启动后等待就绪的超时(秒)。冷启动导入 langchain 约 17s。建议 60(留足冷启动余量)。调大:更容忍慢机/首次启动,但进程卡死时等更久;调小:更快发现启动失败,但可能误杀仍在导入依赖的正常冷启动。
    mReadyTimeoutSpin->setValue(60);
    mStopTimeoutSpin = new QSpinBox(this);
    mStopTimeoutSpin->setRange(1, 60);
    mStopTimeoutSpin->setSuffix(tr(" s"));  //cn:秒
    mStopTimeoutSpin->setToolTip(tr(
        "Waiting time (seconds) for the agent subprocess to exit when stopped. "
        "If the process does not exit within this period it is force-killed. "
        "Recommended: 5. "
        "Larger: gives the subprocess more time for a graceful shutdown "
        "(flush logs, release resources). "
        "Smaller: force-terminates faster, but may lose unwritten stderr/log output."));  //cn:停止 agent 时等待子进程退出的超时(秒),超时则强制终止。建议 5。调大:给子进程更多优雅退出时间(刷新日志、释放资源);调小:更快强制终止,但可能丢失未写入的日志输出。
    mStopTimeoutSpin->setValue(5);
    mTestBtn     = new QPushButton(tr("Test Connection"), this);  //cn:测试连接
    mStatusLabel = new QLabel(this);

    // 上下文管理控件
    mContextWindowSpin = new QSpinBox(this);
    mContextWindowSpin->setRange(8192, 2097152);
    mContextWindowSpin->setSingleStep(1024);
    mContextWindowSpin->setSuffix(tr(" tokens"));
    mContextWindowSpin->setToolTip(tr(
        "Maximum context window of the LLM model in tokens. "
        "Set this to match the model's actual context window "
        "(e.g. 128000 for GPT-4o, 256000 for qwen3-coder, 1048576 for deepseek-v4). "
        "Default 262144 (256K). "
        "Larger: allows more conversation history before compaction, but exceeding "
        "the model's real hard limit causes API errors. "
        "Smaller: triggers compaction earlier, which may discard useful historical "
        "context to save tokens."));  //cn:LLM 模型最大上下文窗口(tokens),须与模型实际窗口匹配(如 GPT-4o=128000,qwen3-coder=256000,deepseek-v4=1048576)。默认 262144(256K)。调大:压缩前可容纳更多对话历史,但超出模型硬限会导致 API 报错;调小:更早触发压缩省 token,但可能丢弃有用的历史上下文。
    mContextWindowSpin->setValue(262144);

    mCompactionThresholdSpin = new QDoubleSpinBox(this);
    mCompactionThresholdSpin->setRange(0.50, 1.0);
    mCompactionThresholdSpin->setSingleStep(0.05);
    mCompactionThresholdSpin->setDecimals(2);
    mCompactionThresholdSpin->setToolTip(tr(
        "Compaction trigger ratio: when used tokens reach this fraction of the "
        "context window, older messages are summarized to free space. "
        "0.85 = compact at 85% of the context window. "
        "Larger: delays compaction, keeps more raw history, but risks hitting the "
        "model's hard context limit on long sessions. "
        "Smaller: compacts earlier, saving tokens and cost, but may discard earlier "
        "context prematurely."));  //cn:压缩触发比例:已用 token 达到上下文窗口的此比例时,将较早的消息摘要以腾出空间。0.85=窗口 85% 时触发。调大:更晚压缩、保留更多原始历史,但长会话可能撞模型上下文硬限;调小:更早压缩省 token 省成本,但可能过早丢弃早期上下文。
    mCompactionThresholdSpin->setValue(0.85);

    mMaxRecentMsgSpin = new QSpinBox(this);
    mMaxRecentMsgSpin->setRange(4, 50);
    mMaxRecentMsgSpin->setToolTip(tr(
        "Number of most recent messages retained verbatim after compaction. "
        "Older messages are summarized; these recent ones stay as-is. "
        "Recommended: 10. "
        "Larger: keeps more recent context intact (better continuity), but "
        "consumes more tokens per turn. "
        "Smaller: saves tokens, but may lose continuity for multi-step reasoning "
        "that spans many recent turns."));  //cn:压缩后保留为原文的最近消息条数,更早的消息被摘要,这些最近的保持原样。建议 10。调大:保留更多近期上下文(连续性更好),但每轮消耗更多 token;调小:更省 token,但跨多轮的多步推理可能丢失连续性。
    mMaxRecentMsgSpin->setValue(10);

    mToolResultMaxCharsSpin = new QSpinBox(this);
    mToolResultMaxCharsSpin->setRange(1000, 500000);
    mToolResultMaxCharsSpin->setSingleStep(1000);
    mToolResultMaxCharsSpin->setToolTip(tr(
        "Character threshold: tool outputs longer than this are truncated to a "
        "preview (see Tool Result Preview Chars). Shorter outputs are kept in full. "
        "Recommended: 20000. "
        "Larger: preserves more tool output detail, but large outputs quickly "
        "consume context window. "
        "Smaller: saves tokens, but may cut off key information in tool results "
        "(e.g. a DataFrame tail row or an error message)."));  //cn:字符阈值:工具输出超此长度则截断为预览(见"工具结果预览长度"),更短的保留全文。建议 20000。调大:保留更多工具输出细节,但大输出会快速消耗上下文窗口;调小:更省 token,但可能截断关键信息(如 DataFrame 末行或错误消息)。
    mToolResultMaxCharsSpin->setValue(20000);

    mToolResultPreviewCharsSpin = new QSpinBox(this);
    mToolResultPreviewCharsSpin->setRange(100, 10000);
    mToolResultPreviewCharsSpin->setSingleStep(100);
    mToolResultPreviewCharsSpin->setToolTip(tr(
        "Preview length (characters) shown when a tool output is truncated. "
        "Only takes effect on outputs exceeding Tool Result Max Chars. "
        "Recommended: 2000. "
        "Larger: the truncated preview is more complete, but each preview occupies "
        "more context. "
        "Smaller: saves tokens, but the preview may be too short to be useful."));  //cn:工具输出被截断时显示的预览长度(字符),仅对超过"工具结果截断阈值"的输出生效。建议 2000。调大:截断预览更完整,但每个预览占用更多上下文;调小:更省 token,但预览可能太短而缺乏参考价值。
    mToolResultPreviewCharsSpin->setValue(2000);

    // 会话持久化控件（plan-06）：自由会话保留数量与天数，供 cleanupSessions 读取
    mMaxSessionsSpin = new QSpinBox(this);
    mMaxSessionsSpin->setRange(5, 200);
    mMaxSessionsSpin->setValue(20);
    mMaxSessionsSpin->setToolTip(tr(
        "Maximum number of free (unsaved) sessions retained in the config directory. "
        "Sessions beyond this count are deleted oldest-first on startup. "
        "Recommended: 20. "
        "Larger: keeps more session history available for review. "
        "Smaller: cleans up sooner, saving disk space."));  //cn:配置目录保留的自由(未另存)会话最大数量,超出此数的会话按最旧优先在启动时删除。建议 20。调大:保留更多历史会话供回看;调小:更早清理节省磁盘。
    mSessionRetentionDaysSpin = new QSpinBox(this);
    mSessionRetentionDaysSpin->setRange(1, 365);
    mSessionRetentionDaysSpin->setValue(30);
    mSessionRetentionDaysSpin->setSuffix(tr(" d"));  //cn:天
    mSessionRetentionDaysSpin->setToolTip(tr(
        "Free (unsaved) sessions older than this many days are deleted on startup. "
        "Recommended: 30. "
        "Larger: keeps session history for a longer period. "
        "Smaller: frees disk space sooner by removing old sessions earlier."));  //cn:早于此天数的自由(未另存)会话在启动时删除。建议 30。调大:历史会话保留更久;调小:更早删除旧会话,更快释放磁盘。

    // 重连与容错控件（plan-05）
    mSpinMaxRetries = new QSpinBox(this);
    mSpinMaxRetries->setRange(0, 20);
    mSpinMaxRetries->setSuffix(tr(" times"));  //cn:次
    mSpinMaxRetries->setToolTip(tr(
        "Maximum number of automatic retries for LLM API calls on transient errors "
        "(HTTP 429 rate-limit, 5xx server errors, network failures). "
        "0 disables retry entirely. Recommended: 7. "
        "Larger: more tolerant of temporary provider outages, but increases "
        "end-to-end latency when the provider is genuinely down. "
        "Smaller: fails faster, useful when you prefer quick failure over long "
        "waits; 0 means a single attempt with no retry."));  //cn:LLM API 调用遇到临时错误(HTTP 429 限流、5xx 服务端错误、网络故障)时的自动重试次数。0 表示完全不重试。建议 7。调大:更容忍服务商临时故障,但服务商确实宕机时端到端延迟增加;调小:更快失败,适合需要快速失败而非长时间等待的场景,0 表示仅尝试一次不重试。
    mSpinMaxRetries->setValue(7);

    mSpinRequestTimeout = new QSpinBox(this);
    mSpinRequestTimeout->setRange(10, 600);
    mSpinRequestTimeout->setSuffix(tr(" sec"));  //cn:秒
    mSpinRequestTimeout->setToolTip(tr(
        "Timeout (seconds) for a single LLM API request, covering connection and "
        "first-byte waiting. "
        "Recommended: 120. "
        "Larger: tolerates slow responses (e.g. complex multi-step reasoning), "
        "but a hung request blocks the agent longer before failing over. "
        "Smaller: fails faster on unresponsive providers, but may interrupt "
        "normal long-running reasoning that simply needs more time."));  //cn:单次 LLM API 请求的超时(秒),覆盖连接与首字节等待。建议 120。调大:容忍慢响应(如复杂多步推理),但请求挂起时 agent 阻塞更久才故障转移;调小:对无响应服务商更快失败,但可能中断本可正常完成的长时间推理。
    mSpinRequestTimeout->setValue(120);

    mSpinInactivityTimeout = new QSpinBox(this);
    mSpinInactivityTimeout->setRange(60, 600);
    mSpinInactivityTimeout->setSuffix(tr(" sec"));  //cn:秒
    mSpinInactivityTimeout->setToolTip(tr(
        "Watchdog timeout (seconds): if no protocol message is received from the "
        "agent subprocess within this period, it is considered hung and stopped. "
        "Recommended: 240. "
        "Larger: more tolerant of long-running tool executions that legitimately "
        "produce no output for a while. "
        "Smaller: detects a hung subprocess faster, but may prematurely kill a "
        "process that is executing a slow but valid task (e.g. a long pandas "
        "computation)."));  //cn:看门狗超时(秒):若此时间内未收到 agent 子进程的任何协议消息,则视为卡死并停止。建议 240。调大:更容忍长时间无输出的工具执行;调小:更快检测卡死进程,但可能误杀正在执行慢但有效任务(如耗时 pandas 计算)的进程。
    mSpinInactivityTimeout->setValue(240);

    mSpinMaxRestarts = new QSpinBox(this);
    mSpinMaxRestarts->setRange(0, 10);
    mSpinMaxRestarts->setSuffix(tr(" times"));  //cn:次
    mSpinMaxRestarts->setToolTip(tr(
        "Maximum number of automatic restarts after the agent subprocess crashes "
        "unexpectedly. 0 disables auto-restart. Recommended: 3. "
        "Larger: gives more automatic recovery opportunities, useful on unstable "
        "environments, but may repeatedly restart a fundamentally broken process. "
        "Smaller: fewer restart attempts; 0 means the agent stays stopped after "
        "a crash until manually restarted."));  //cn:agent 子进程意外崩溃后的自动重启最大次数。0 表示不自动重启。建议 3。调大:更多自动恢复机会,适合不稳定环境,但可能反复重启根本损坏的进程;调小:更少重启,0 表示崩溃后保持停止直到手动重启。
    mSpinMaxRestarts->setValue(3);

    // 推理迭代上限：LangGraph 图最大步数（数据分析需频繁调用工具查数据，150 步≈50 轮工具调用）
    mSpinRecursionLimit = new QSpinBox(this);
    mSpinRecursionLimit->setRange(20, 1000);
    mSpinRecursionLimit->setToolTip(tr(
        "Maximum number of graph reasoning steps (compact->agent->tools cycle). "
        "Each tool-call cycle consumes 3 steps. "
        "Recommended: 150 (~50 tool-call cycles), suits data-analysis tasks that "
        "frequently query data. "
        "Larger: allows complex multi-step analysis, but increases cost and may "
        "let a looping agent run longer before stopping. "
        "Smaller: stops runaway loops sooner and saves cost, but may cut off "
        "legitimate long analysis chains. "
        "Also guarded by automatic repeated-tool-call detection."));  //cn:图最大推理步数(compact→agent→tools 循环)。每轮工具调用耗 3 步。建议 150(约 50 轮工具调用),适合数据分析频繁查数据的场景。调大:支持更复杂的多步分析,但成本增加且循环 agent 运行更久才停止;调小:更早中止失控循环并节省成本,但可能截断合理的长分析链。另有自动重复调用检测兜底。
    mSpinRecursionLimit->setValue(150);

    QFormLayout* form = new QFormLayout(this);
    form->addRow(tr("Base URL"), mBaseUrlEdit);
    form->addRow(tr("API Key"),  mApiKeyEdit);
    form->addRow(tr("Model"),    mModelEdit);
    form->addRow(tr("Ready Timeout"), mReadyTimeoutSpin);  //cn:就绪超时
    form->addRow(tr("Stop Timeout"),  mStopTimeoutSpin);    //cn:停止超时
    form->addRow(mTestBtn);
    form->addRow(mStatusLabel);
    // 上下文管理
    form->addRow(tr("Context Window"), mContextWindowSpin);              //cn:上下文窗口
    form->addRow(tr("Compaction Threshold"), mCompactionThresholdSpin);  //cn:压缩阈值
    form->addRow(tr("Max Recent Messages"), mMaxRecentMsgSpin);         //cn:保留最近消息数
    form->addRow(tr("Tool Result Max Chars"), mToolResultMaxCharsSpin);  //cn:工具结果截断阈值
    form->addRow(tr("Tool Result Preview Chars"), mToolResultPreviewCharsSpin);  //cn:工具结果预览长度
    // 会话持久化（plan-06）：平铺风格，不分组，对齐既有 addRow 习惯
    form->addRow(tr("Max Sessions"), mMaxSessionsSpin);  //cn:最大会话数
    form->addRow(tr("Session Retention Days"), mSessionRetentionDaysSpin);  //cn:会话保留天数
    // 重连与容错（plan-05）：平铺风格，不分组，对齐既有 addRow 习惯
    form->addRow(tr("Max retries"), mSpinMaxRetries);  //cn:最大重试次数
    form->addRow(tr("Request timeout"), mSpinRequestTimeout);  //cn:请求超时
    form->addRow(tr("Inactivity timeout"), mSpinInactivityTimeout);  //cn:无活动超时
    form->addRow(tr("Max process restarts"), mSpinMaxRestarts);  //cn:最大进程重启次数
    form->addRow(tr("Reasoning iteration limit"), mSpinRecursionLimit);  //cn:推理迭代上限

    connect(mTestBtn, &QPushButton::clicked, this, &DAAgentSettingsWidget::onTestConnection);

    // CRITICAL: 将字段变更连接到 settingChanged()，让平台的 applyChanged()
    // 机制知道本页有未保存改动。若缺失此连接，OK/Apply 永远不会调用 apply()，
    // 配置永远不会保存（参照 DASettingPagePython.cpp:24,129 的实现模式）。
    connect(mBaseUrlEdit, &QLineEdit::textChanged, this, [this]() {
        emit settingChanged();
    });
    connect(mApiKeyEdit, &QLineEdit::textChanged, this, [this]() {
        emit settingChanged();
    });
    connect(mModelEdit, &QLineEdit::textChanged, this, [this]() {
        emit settingChanged();
    });
    connect(mReadyTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(mStopTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    // 上下文管理控件的 settingChanged 连接
    connect(mContextWindowSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(mCompactionThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(mMaxRecentMsgSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(mToolResultMaxCharsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(mToolResultPreviewCharsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    // 会话持久化控件（plan-06）：同样须连接 settingChanged，否则 apply 不触发
    connect(mMaxSessionsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(mSessionRetentionDaysSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    // 重连与容错控件（plan-05）：同样须连接 settingChanged，否则 apply 不触发
    connect(mSpinMaxRetries, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(mSpinRequestTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(mSpinInactivityTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(mSpinMaxRestarts, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(mSpinRecursionLimit, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });

    // 注：loadConfig() 期间无需 QSignalBlocker——
    // addPage()（连接 settingChanged → 脏页追踪器）在构造函数返回之后才执行，
    // 因此 loadConfig() 中触发的信号没有监听者，不会污染脏页集合。
}

/** @brief 从 agent-config.ini 加载配置到界面控件 */
void DAAgentSettingsWidget::loadConfig()
{
    // 经接口读 agent-config.ini（DAAgent 内部 DPAPI 解密 api_key 为明文）。
    // mAgentInterface 未注入时（如旧构建路径误用）直接返回，避免空指针解引用。
    if (!mAgentInterface) {
        daDebug << "[DAAgentSettings] loadConfig skipped: no agent interface injected";
        return;
    }
    QJsonObject c = mAgentInterface->getLLMConfig();  // api_key 已是明文
    // 诊断日志: 确认读到配置(绝不打印 api_key 明文,只显示 api_key_empty 标志)
    daDebug << "[DAAgentSettings] loadConfig: base_url=" << c.value("base_url").toString()
            << " model=" << c.value("model").toString()
            << " api_key_empty=" << c.value("api_key").toString().isEmpty()
            << " context_window=" << c.value("context_window").toInt()
            << " max_sessions=" << c.value("max_sessions").toInt();
    mBaseUrlEdit->setText(c.value("base_url").toString());
    mModelEdit->setText(c.value("model").toString());
    mApiKeyEdit->setText(c.value("api_key").toString());  // 明文
    mReadyTimeoutSpin->setValue(jsonInt(c, "ready_timeout_sec", 60));
    mStopTimeoutSpin->setValue(jsonInt(c, "stop_timeout_sec", 5));
    mContextWindowSpin->setValue(jsonInt(c, "context_window", 262144));
    mCompactionThresholdSpin->setValue(jsonDouble(c, "compaction_threshold", 0.85));
    mMaxRecentMsgSpin->setValue(jsonInt(c, "max_recent_messages", 10));
    mToolResultMaxCharsSpin->setValue(jsonInt(c, "tool_result_max_chars", 20000));
    mToolResultPreviewCharsSpin->setValue(jsonInt(c, "tool_result_preview_chars", 2000));
    mMaxSessionsSpin->setValue(jsonInt(c, "max_sessions", 20));
    mSessionRetentionDaysSpin->setValue(jsonInt(c, "session_retention_days", 30));
    // 重连与容错（plan-05）
    mSpinMaxRetries->setValue(jsonInt(c, "max_retries", 7));
    mSpinRequestTimeout->setValue(jsonInt(c, "request_timeout_sec", 120));
    mSpinInactivityTimeout->setValue(jsonInt(c, "inactivity_timeout_sec", 240));
    mSpinMaxRestarts->setValue(jsonInt(c, "max_subprocess_restarts", 3));
    mSpinRecursionLimit->setValue(jsonInt(c, "recursion_limit", 150));
}

/** @brief 将界面配置保存到 agent-config.ini */
void DAAgentSettingsWidget::saveConfig()
{
    // 经接口写 agent-config.ini（DAAgent 内部 DPAPI 加密 api_key）。页只组装明文 QJsonObject。
    if (!mAgentInterface) {
        daDebug << "[DAAgentSettings] saveConfig skipped: no agent interface injected";
        return;
    }
    QJsonObject c;
    c["base_url"]                  = mBaseUrlEdit->text().trimmed();
    c["model"]                     = mModelEdit->text().trimmed();
    c["api_key"]                   = mApiKeyEdit->text();  // 明文,DAAgent 内部加密
    c["ready_timeout_sec"]         = mReadyTimeoutSpin->value();
    c["stop_timeout_sec"]          = mStopTimeoutSpin->value();
    c["context_window"]            = mContextWindowSpin->value();
    c["compaction_threshold"]      = mCompactionThresholdSpin->value();
    c["max_recent_messages"]       = mMaxRecentMsgSpin->value();
    c["tool_result_max_chars"]     = mToolResultMaxCharsSpin->value();
    c["tool_result_preview_chars"] = mToolResultPreviewCharsSpin->value();
    c["max_sessions"]              = mMaxSessionsSpin->value();
    c["session_retention_days"]    = mSessionRetentionDaysSpin->value();
    // 重连与容错（plan-05）
    c["max_retries"]              = mSpinMaxRetries->value();
    c["request_timeout_sec"]      = mSpinRequestTimeout->value();
    c["inactivity_timeout_sec"]   = mSpinInactivityTimeout->value();
    c["max_subprocess_restarts"]  = mSpinMaxRestarts->value();
    c["recursion_limit"]          = mSpinRecursionLimit->value();
    // 诊断日志: 确认 saveConfig 真的被调用且收集到了值(绝不打印 api_key 明文)
    daDebug << "[DAAgentSettings] saveConfig: base_url=" << c.value("base_url").toString()
            << " model=" << c.value("model").toString()
            << " api_key_empty=" << c.value("api_key").toString().isEmpty()
            << " context_window=" << c.value("context_window").toInt()
            << " max_sessions=" << c.value("max_sessions").toInt();
    mAgentInterface->setLLMConfig(c);
}

/** @brief 应用设置页的配置变更 */
void DAAgentSettingsWidget::apply()
{
    // 诊断日志: 若此行不出现在日志里,说明 dirty 机制未触发,apply 没被调用
    // (DASettingDialog OK→applyChanged 遍历 mChangedPages,只有脏页才调 apply)
    daDebug << "[DAAgentSettings] apply entered";
    saveConfig();
    emit settingApplyed();
    daDebug << "[DAAgentSettings] apply done";
}

/** @brief 获取设置页图标 */
QIcon DAAgentSettingsWidget::getSettingPageIcon() const
{
    // 暂不创建专用图标资源（plan-01 的 agent.qrc 未启用）
    return QIcon();
}

/** @brief 测试 LLM API 连接是否可用 */
void DAAgentSettingsWidget::onTestConnection()
{
    QString baseUrl = mBaseUrlEdit->text().trimmed();
    QString apiKey  = mApiKeyEdit->text().trimmed();
    QString model   = mModelEdit->text().trimmed();

    // 去除 base URL 末尾斜杠，避免 baseUrl + "/chat/completions" 拼成双斜杠
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

    // 发送（主线程异步，基于事件循环）
    mTestBtn->setEnabled(false);
    mStatusLabel->setText(tr("Testing..."));  //cn:测试中...
    QNetworkReply* reply = mNetworkManager->post(request, QJsonDocument(body).toJson());

    // 10s 超时：网络挂起时 abort，触发 finished 信号（error() 为 OperationCanceledError）
    QTimer::singleShot(10000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, model]() {
        mTestBtn->setEnabled(true);
        if (reply->error() == QNetworkReply::NoError) {
            // 解析响应中的 model 字段（多数 OpenAI 兼容接口会回显实际使用的模型）
            QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
            QString modelUsed = resp.value("model").toString(model);  // 回退到用户输入的 model
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
