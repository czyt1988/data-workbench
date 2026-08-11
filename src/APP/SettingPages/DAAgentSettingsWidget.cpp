// DAAgentSettingsWidget.cpp
// 持久化经 m_agentInterface->get/setLLMConfig 走 agent-config.ini（DAAgent 内部 DPAPI 加解密 api_key），
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
double jsonDouble(const QJsonObject& o, const char* key, double def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isDouble() ? v.toDouble() : def;
}
}  // namespace

namespace DA
{

DAAgentSettingsWidget::DAAgentSettingsWidget(QWidget* parent)
    : DAAbstractSettingPage(parent)
{
    setupUI();
    // QNetworkAccessManager 在构造函数中创建（父子对象托管），避免 onTestConnection 空指针解引用
    m_networkManager = new QNetworkAccessManager(this);
    // loadConfig() 移到 setAgentInterface：构造时 m_agentInterface 尚未注入，
    // 此处调用只会因空指针守卫提前返回，移除以避免误导。
}

void DAAgentSettingsWidget::setAgentInterface(DAAgentInterface* p)
{
    m_agentInterface = p;
    if (p) {
        // 接口注入后立即加载配置（构造函数中接口未注入，loadConfig 被跳过）。
        // QSignalBlocker 防止 load 期间 setValue 触发 settingChanged 误标脏页。
        QSignalBlocker blocker(this);
        loadConfig();
    }
}

void DAAgentSettingsWidget::setupUI()
{
    m_baseUrlEdit = new QLineEdit(this);
    m_apiKeyEdit  = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);   // API Key 密文显示
    m_modelEdit   = new QLineEdit(this);
    // 超时设置:可配,默认 ready=60s(覆盖 langchain 冷启动导入~17s)、stop=5s
    m_readyTimeoutSpin = new QSpinBox(this);
    m_readyTimeoutSpin->setRange(5, 300);
    m_readyTimeoutSpin->setSuffix(tr(" s"));  //cn:秒
    m_readyTimeoutSpin->setToolTip(tr(
        "Waiting time for agent subprocess to become ready after start. "
        "Cold start imports of langchain may take ~17s, default 60s is safe."));  //cn:agent 子进程启动后等待就绪的超时(秒)。冷启动导入 langchain 约 17s,默认 60s 较安全。
    m_readyTimeoutSpin->setValue(60);
    m_stopTimeoutSpin = new QSpinBox(this);
    m_stopTimeoutSpin->setRange(1, 60);
    m_stopTimeoutSpin->setSuffix(tr(" s"));  //cn:秒
    m_stopTimeoutSpin->setToolTip(tr(
        "Waiting time for agent subprocess to exit when stopped."));  //cn:停止 agent 时等待子进程退出的超时(秒)。
    m_stopTimeoutSpin->setValue(5);
    m_testBtn     = new QPushButton(tr("测试连接"), this);
    m_statusLabel = new QLabel(this);

    // 上下文管理控件
    m_contextWindowSpin = new QSpinBox(this);
    m_contextWindowSpin->setRange(8192, 2097152);
    m_contextWindowSpin->setSingleStep(1024);
    m_contextWindowSpin->setSuffix(tr(" tokens"));
    m_contextWindowSpin->setToolTip(tr(
        "Maximum context window of the LLM model in tokens. "
        "Set this to match the model's actual context window. "
        "Default 262144 (256K)."));  //cn:LLM 模型最大上下文窗口(tokens)。须与模型实际上下文窗口匹配。默认 262144(256K)
    m_contextWindowSpin->setValue(262144);

    m_compactionThresholdSpin = new QDoubleSpinBox(this);
    m_compactionThresholdSpin->setRange(0.50, 1.0);
    m_compactionThresholdSpin->setSingleStep(0.05);
    m_compactionThresholdSpin->setDecimals(2);
    m_compactionThresholdSpin->setToolTip(tr(
        "Compaction trigger ratio (0.85 = compact at 85% of context window)"));  //cn:压缩触发比例(0.85=窗口 85%时触发)
    m_compactionThresholdSpin->setValue(0.85);

    m_maxRecentMsgSpin = new QSpinBox(this);
    m_maxRecentMsgSpin->setRange(4, 50);
    m_maxRecentMsgSpin->setToolTip(tr(
        "Number of recent messages to retain after compaction"));  //cn:压缩后保留最近消息条数
    m_maxRecentMsgSpin->setValue(10);

    m_toolResultMaxCharsSpin = new QSpinBox(this);
    m_toolResultMaxCharsSpin->setRange(1000, 500000);
    m_toolResultMaxCharsSpin->setSingleStep(1000);
    m_toolResultMaxCharsSpin->setToolTip(tr(
        "Tool results exceeding this length will be truncated to a preview"));  //cn:工具结果超此长度将截断为预览
    m_toolResultMaxCharsSpin->setValue(20000);

    m_toolResultPreviewCharsSpin = new QSpinBox(this);
    m_toolResultPreviewCharsSpin->setRange(100, 10000);
    m_toolResultPreviewCharsSpin->setSingleStep(100);
    m_toolResultPreviewCharsSpin->setToolTip(tr(
        "Preview length for truncated tool results"));  //cn:截断后工具结果预览长度
    m_toolResultPreviewCharsSpin->setValue(2000);

    // 会话持久化控件（plan-06）：自由会话保留数量与天数，供 cleanupSessions 读取
    m_maxSessionsSpin = new QSpinBox(this);
    m_maxSessionsSpin->setRange(5, 200);
    m_maxSessionsSpin->setValue(20);
    m_maxSessionsSpin->setToolTip(tr(
        "Maximum number of free sessions retained in the config directory. "
        "Older sessions beyond this count are cleaned up on startup."));  //cn:配置目录保留的自由会话最大数量,超出此数的旧会话在启动时清理
    m_sessionRetentionDaysSpin = new QSpinBox(this);
    m_sessionRetentionDaysSpin->setRange(1, 365);
    m_sessionRetentionDaysSpin->setValue(30);
    m_sessionRetentionDaysSpin->setSuffix(tr(" d"));  //cn:天
    m_sessionRetentionDaysSpin->setToolTip(tr(
        "Free sessions older than this many days are cleaned up on startup."));  //cn:早于此天数的自由会话在启动时清理

    // 重连与容错控件（plan-05）
    m_spinMaxRetries = new QSpinBox(this);
    m_spinMaxRetries->setRange(0, 20);
    m_spinMaxRetries->setSuffix(tr(" times"));  //cn:次
    m_spinMaxRetries->setToolTip(tr(
        "Maximum number of retries for LLM API calls (429/5xx/network errors). "
        "0 disables retry."));  //cn:LLM API 调用最大重试次数(429/5xx/网络错误)。0 表示不重试。
    m_spinMaxRetries->setValue(7);

    m_spinRequestTimeout = new QSpinBox(this);
    m_spinRequestTimeout->setRange(10, 600);
    m_spinRequestTimeout->setSuffix(tr(" sec"));  //cn:秒
    m_spinRequestTimeout->setToolTip(tr(
        "Timeout in seconds for a single LLM API request."));  //cn:单次 LLM API 请求超时(秒)。
    m_spinRequestTimeout->setValue(120);

    m_spinInactivityTimeout = new QSpinBox(this);
    m_spinInactivityTimeout->setRange(60, 600);
    m_spinInactivityTimeout->setSuffix(tr(" sec"));  //cn:秒
    m_spinInactivityTimeout->setToolTip(tr(
        "If no protocol message is received for this duration, the agent "
        "subprocess is considered hung and will be stopped."));  //cn:若此时间内未收到任何协议消息,agent 子进程将被视为卡死并停止。
    m_spinInactivityTimeout->setValue(240);

    m_spinMaxRestarts = new QSpinBox(this);
    m_spinMaxRestarts->setRange(0, 10);
    m_spinMaxRestarts->setSuffix(tr(" times"));  //cn:次
    m_spinMaxRestarts->setToolTip(tr(
        "Maximum number of automatic restarts after agent subprocess crash. "
        "0 disables auto-restart."));  //cn:agent 子进程崩溃后自动重启最大次数。0 表示不自动重启。
    m_spinMaxRestarts->setValue(3);

    QFormLayout* form = new QFormLayout(this);
    form->addRow(tr("Base URL"), m_baseUrlEdit);
    form->addRow(tr("API Key"),  m_apiKeyEdit);
    form->addRow(tr("Model"),    m_modelEdit);
    form->addRow(tr("Ready Timeout"), m_readyTimeoutSpin);  //cn:就绪超时
    form->addRow(tr("Stop Timeout"),  m_stopTimeoutSpin);    //cn:停止超时
    form->addRow(m_testBtn);
    form->addRow(m_statusLabel);
    // 上下文管理
    form->addRow(tr("Context Window"), m_contextWindowSpin);              //cn:上下文窗口
    form->addRow(tr("Compaction Threshold"), m_compactionThresholdSpin);  //cn:压缩阈值
    form->addRow(tr("Max Recent Messages"), m_maxRecentMsgSpin);         //cn:保留最近消息数
    form->addRow(tr("Tool Result Max Chars"), m_toolResultMaxCharsSpin);  //cn:工具结果截断阈值
    form->addRow(tr("Tool Result Preview Chars"), m_toolResultPreviewCharsSpin);  //cn:工具结果预览长度
    // 会话持久化（plan-06）：平铺风格，不分组，对齐既有 addRow 习惯
    form->addRow(tr("Max Sessions"), m_maxSessionsSpin);  //cn:最大会话数
    form->addRow(tr("Session Retention Days"), m_sessionRetentionDaysSpin);  //cn:会话保留天数
    // 重连与容错（plan-05）：平铺风格，不分组，对齐既有 addRow 习惯
    form->addRow(tr("Max retries"), m_spinMaxRetries);  //cn:最大重试次数
    form->addRow(tr("Request timeout"), m_spinRequestTimeout);  //cn:请求超时
    form->addRow(tr("Inactivity timeout"), m_spinInactivityTimeout);  //cn:无活动超时
    form->addRow(tr("Max process restarts"), m_spinMaxRestarts);  //cn:最大进程重启次数

    connect(m_testBtn, &QPushButton::clicked, this, &DAAgentSettingsWidget::onTestConnection);

    // CRITICAL: 将字段变更连接到 settingChanged()，让平台的 applyChanged()
    // 机制知道本页有未保存改动。若缺失此连接，OK/Apply 永远不会调用 apply()，
    // 配置永远不会保存（参照 DASettingPagePython.cpp:24,129 的实现模式）。
    connect(m_baseUrlEdit, &QLineEdit::textChanged, this, [this]() {
        emit settingChanged();
    });
    connect(m_apiKeyEdit, &QLineEdit::textChanged, this, [this]() {
        emit settingChanged();
    });
    connect(m_modelEdit, &QLineEdit::textChanged, this, [this]() {
        emit settingChanged();
    });
    connect(m_readyTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(m_stopTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    // 上下文管理控件的 settingChanged 连接
    connect(m_contextWindowSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(m_compactionThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(m_maxRecentMsgSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(m_toolResultMaxCharsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(m_toolResultPreviewCharsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    // 会话持久化控件（plan-06）：同样须连接 settingChanged，否则 apply 不触发
    connect(m_maxSessionsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(m_sessionRetentionDaysSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    // 重连与容错控件（plan-05）：同样须连接 settingChanged，否则 apply 不触发
    connect(m_spinMaxRetries, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(m_spinRequestTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(m_spinInactivityTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });
    connect(m_spinMaxRestarts, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit settingChanged();
    });

    // 注：loadConfig() 期间无需 QSignalBlocker——
    // addPage()（连接 settingChanged → 脏页追踪器）在构造函数返回之后才执行，
    // 因此 loadConfig() 中触发的信号没有监听者，不会污染脏页集合。
}

void DAAgentSettingsWidget::loadConfig()
{
    // 经接口读 agent-config.ini（DAAgent 内部 DPAPI 解密 api_key 为明文）。
    // m_agentInterface 未注入时（如旧构建路径误用）直接返回，避免空指针解引用。
    if (!m_agentInterface) {
        daDebug << "[DAAgentSettings] loadConfig skipped: no agent interface injected";
        return;
    }
    QJsonObject c = m_agentInterface->getLLMConfig();  // api_key 已是明文
    // 诊断日志: 确认读到配置(绝不打印 api_key 明文,只显示 api_key_empty 标志)
    daDebug << "[DAAgentSettings] loadConfig: base_url=" << c.value("base_url").toString()
            << " model=" << c.value("model").toString()
            << " api_key_empty=" << c.value("api_key").toString().isEmpty()
            << " context_window=" << c.value("context_window").toInt()
            << " max_sessions=" << c.value("max_sessions").toInt();
    m_baseUrlEdit->setText(c.value("base_url").toString());
    m_modelEdit->setText(c.value("model").toString());
    m_apiKeyEdit->setText(c.value("api_key").toString());  // 明文
    m_readyTimeoutSpin->setValue(jsonInt(c, "ready_timeout_sec", 60));
    m_stopTimeoutSpin->setValue(jsonInt(c, "stop_timeout_sec", 5));
    m_contextWindowSpin->setValue(jsonInt(c, "context_window", 262144));
    m_compactionThresholdSpin->setValue(jsonDouble(c, "compaction_threshold", 0.85));
    m_maxRecentMsgSpin->setValue(jsonInt(c, "max_recent_messages", 10));
    m_toolResultMaxCharsSpin->setValue(jsonInt(c, "tool_result_max_chars", 20000));
    m_toolResultPreviewCharsSpin->setValue(jsonInt(c, "tool_result_preview_chars", 2000));
    m_maxSessionsSpin->setValue(jsonInt(c, "max_sessions", 20));
    m_sessionRetentionDaysSpin->setValue(jsonInt(c, "session_retention_days", 30));
    // 重连与容错（plan-05）
    m_spinMaxRetries->setValue(jsonInt(c, "max_retries", 7));
    m_spinRequestTimeout->setValue(jsonInt(c, "request_timeout_sec", 120));
    m_spinInactivityTimeout->setValue(jsonInt(c, "inactivity_timeout_sec", 240));
    m_spinMaxRestarts->setValue(jsonInt(c, "max_subprocess_restarts", 3));
}

void DAAgentSettingsWidget::saveConfig()
{
    // 经接口写 agent-config.ini（DAAgent 内部 DPAPI 加密 api_key）。页只组装明文 QJsonObject。
    if (!m_agentInterface) {
        daDebug << "[DAAgentSettings] saveConfig skipped: no agent interface injected";
        return;
    }
    QJsonObject c;
    c["base_url"]                  = m_baseUrlEdit->text().trimmed();
    c["model"]                     = m_modelEdit->text().trimmed();
    c["api_key"]                   = m_apiKeyEdit->text();  // 明文,DAAgent 内部加密
    c["ready_timeout_sec"]         = m_readyTimeoutSpin->value();
    c["stop_timeout_sec"]          = m_stopTimeoutSpin->value();
    c["context_window"]            = m_contextWindowSpin->value();
    c["compaction_threshold"]      = m_compactionThresholdSpin->value();
    c["max_recent_messages"]       = m_maxRecentMsgSpin->value();
    c["tool_result_max_chars"]     = m_toolResultMaxCharsSpin->value();
    c["tool_result_preview_chars"] = m_toolResultPreviewCharsSpin->value();
    c["max_sessions"]              = m_maxSessionsSpin->value();
    c["session_retention_days"]    = m_sessionRetentionDaysSpin->value();
    // 重连与容错（plan-05）
    c["max_retries"]              = m_spinMaxRetries->value();
    c["request_timeout_sec"]      = m_spinRequestTimeout->value();
    c["inactivity_timeout_sec"]   = m_spinInactivityTimeout->value();
    c["max_subprocess_restarts"]  = m_spinMaxRestarts->value();
    // 诊断日志: 确认 saveConfig 真的被调用且收集到了值(绝不打印 api_key 明文)
    daDebug << "[DAAgentSettings] saveConfig: base_url=" << c.value("base_url").toString()
            << " model=" << c.value("model").toString()
            << " api_key_empty=" << c.value("api_key").toString().isEmpty()
            << " context_window=" << c.value("context_window").toInt()
            << " max_sessions=" << c.value("max_sessions").toInt();
    m_agentInterface->setLLMConfig(c);
}

void DAAgentSettingsWidget::apply()
{
    // 诊断日志: 若此行不出现在日志里,说明 dirty 机制未触发,apply 没被调用
    // (DASettingDialog OK→applyChanged 遍历 mChangedPages,只有脏页才调 apply)
    daDebug << "[DAAgentSettings] apply entered";
    saveConfig();
    emit settingApplyed();
    daDebug << "[DAAgentSettings] apply done";
}

QIcon DAAgentSettingsWidget::getSettingPageIcon() const
{
    // 暂不创建专用图标资源（plan-01 的 agent.qrc 未启用）
    return QIcon();
}

void DAAgentSettingsWidget::onTestConnection()
{
    QString baseUrl = m_baseUrlEdit->text().trimmed();
    QString apiKey  = m_apiKeyEdit->text().trimmed();
    QString model   = m_modelEdit->text().trimmed();

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
    m_testBtn->setEnabled(false);
    m_statusLabel->setText("测试中...");
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());

    // 10s 超时：网络挂起时 abort，触发 finished 信号（error() 为 OperationCanceledError）
    QTimer::singleShot(10000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, model]() {
        m_testBtn->setEnabled(true);
        if (reply->error() == QNetworkReply::NoError) {
            // 解析响应中的 model 字段（多数 OpenAI 兼容接口会回显实际使用的模型）
            QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
            QString modelUsed = resp.value("model").toString(model);  // 回退到用户输入的 model
            m_statusLabel->setStyleSheet("color: green;");
            m_statusLabel->setText(QString("✓ 连接成功 (%1)").arg(modelUsed));
        } else {
            m_statusLabel->setStyleSheet("color: red;");
            m_statusLabel->setText(QString("✗ 连接失败: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });
}

} // namespace DA
