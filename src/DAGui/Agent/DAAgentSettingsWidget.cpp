// DAAgentSettingsWidget.cpp
#include "DAAgentSettingsWidget.h"
#include "DADir.h"
#include "DALogCategory.h"
#include <QCoreApplication>
#include <QFormLayout>
#include <QSettings>
#include <QTimer>
#include <QJsonArray>
#include <QUrl>

// Config keys (static const, not DAAppConfig macros):
// DAAgent 库无法链接 APP 的 DAAppConfig，使用 QSettings 持久化（见 plan-06 "持久化方案选择"）
static const char* KEY_LLM_BASE_URL = "agent/llm_base_url";
static const char* KEY_LLM_API_KEY  = "agent/llm_api_key";   // stored encrypted
static const char* KEY_LLM_MODEL    = "agent/llm_model";
static const char* KEY_READY_TIMEOUT_SEC = "agent/ready_timeout_sec";  // ready 等待超时(秒)
static const char* KEY_STOP_TIMEOUT_SEC  = "agent/stop_timeout_sec";   // 停止等待超时(秒)

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#endif

namespace DA
{

DAAgentSettingsWidget::DAAgentSettingsWidget(QWidget* parent)
    : DAAbstractSettingPage(parent)
{
    setupUI();
    // QNetworkAccessManager 在构造函数中创建（父子对象托管），避免 onTestConnection 空指针解引用
    m_networkManager = new QNetworkAccessManager(this);
    loadConfig();
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
    m_stopTimeoutSpin = new QSpinBox(this);
    m_stopTimeoutSpin->setRange(1, 60);
    m_stopTimeoutSpin->setSuffix(tr(" s"));  //cn:秒
    m_stopTimeoutSpin->setToolTip(tr(
        "Waiting time for agent subprocess to exit when stopped."));  //cn:停止 agent 时等待子进程退出的超时(秒)。
    m_testBtn     = new QPushButton(tr("测试连接"), this);
    m_statusLabel = new QLabel(this);

    QFormLayout* form = new QFormLayout(this);
    form->addRow(tr("Base URL"), m_baseUrlEdit);
    form->addRow(tr("API Key"),  m_apiKeyEdit);
    form->addRow(tr("Model"),    m_modelEdit);
    form->addRow(tr("Ready Timeout"), m_readyTimeoutSpin);  //cn:就绪超时
    form->addRow(tr("Stop Timeout"),  m_stopTimeoutSpin);    //cn:停止超时
    form->addRow(m_testBtn);
    form->addRow(m_statusLabel);

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

    // 注：loadConfig() 期间无需 QSignalBlocker——
    // addPage()（连接 settingChanged → 脏页追踪器）在构造函数返回之后才执行，
    // 因此 loadConfig() 中触发的信号没有监听者，不会污染脏页集合。
}

void DAAgentSettingsWidget::loadConfig()
{
    // 显式 INI 路径(原为 QSettings 默认构造→注册表,org 为空时 setValue 静默失败)
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    QString baseUrl   = s.value(KEY_LLM_BASE_URL).toString();
    QString model     = s.value(KEY_LLM_MODEL).toString();
    // IniFormat 原生支持 QByteArray(@ByteArray 注解),api_key 直接读写
    QByteArray encKey = s.value(KEY_LLM_API_KEY).toByteArray();
    // 超时配置:默认 ready=60s(覆盖 langchain 冷启动导入)、stop=5s
    int readyTimeout = s.value(KEY_READY_TIMEOUT_SEC, 60).toInt();
    int stopTimeout  = s.value(KEY_STOP_TIMEOUT_SEC, 5).toInt();
    // 诊断日志: 显示从 QSettings 读到的原始值(绝不打印 api_key 明文,只显示加密 blob 大小)
    // 用于排查"重启后字段为空"问题——若 QSettings 路径不一致/注册表为空,这里一目了然
    daDebug << "[DAAgentSettings] loadConfig: base_url=" << baseUrl
            << " model=" << model
            << " api_key_enc_size=" << encKey.size()
            << " ready_timeout=" << readyTimeout
            << " stop_timeout=" << stopTimeout
            << " org=" << QCoreApplication::organizationName()
            << " app=" << QCoreApplication::applicationName();
    m_baseUrlEdit->setText(baseUrl);
    m_modelEdit->setText(model);
    if (!encKey.isEmpty()) {
        m_apiKeyEdit->setText(decryptApiKey(encKey));
    }
    m_readyTimeoutSpin->setValue(readyTimeout);
    m_stopTimeoutSpin->setValue(stopTimeout);
}

void DAAgentSettingsWidget::saveConfig()
{
    QSettings s(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
    QString baseUrl   = m_baseUrlEdit->text().trimmed();
    QString model     = m_modelEdit->text().trimmed();
    QByteArray encKey = encryptApiKey(m_apiKeyEdit->text());
    int readyTimeout = m_readyTimeoutSpin->value();
    int stopTimeout  = m_stopTimeoutSpin->value();
    s.setValue(KEY_LLM_BASE_URL, baseUrl);
    s.setValue(KEY_LLM_MODEL,    model);
    // IniFormat 原生支持 QByteArray(@ByteArray 注解),加密 blob 直接存储
    s.setValue(KEY_LLM_API_KEY,  encKey);
    s.setValue(KEY_READY_TIMEOUT_SEC, readyTimeout);
    s.setValue(KEY_STOP_TIMEOUT_SEC,  stopTimeout);
    // 诊断日志: 确认 saveConfig 真的被调用且写入了 QSettings(绝不打印 api_key 明文)
    // 若 ini 文件为空但此处显示有值,说明 QSettings 写入失败(环境/权限问题)
    daDebug << "[DAAgentSettings] saveConfig written: base_url=" << baseUrl
            << " model=" << model
            << " api_key_enc_size=" << encKey.size()
            << " ready_timeout=" << readyTimeout
            << " stop_timeout=" << stopTimeout;
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

#ifdef Q_OS_WIN
QByteArray DAAgentSettingsWidget::encryptApiKey(const QString& apiKey)
{
    if (apiKey.isEmpty()) return {};
    QByteArray utf8 = apiKey.toUtf8();
    DATA_BLOB inBlob;
    inBlob.pbData = reinterpret_cast<BYTE*>(utf8.data());
    inBlob.cbData = static_cast<DWORD>(utf8.size());
    DATA_BLOB outBlob;
    // 标志传 0 = 当前用户作用域，NOT CRYPTPROTECT_LOCAL_MACHINE
    if (!CryptProtectData(&inBlob, L"AgentApiKey", nullptr, nullptr, nullptr,
                          0, &outBlob)) {
        return {};
    }
    QByteArray enc(reinterpret_cast<const char*>(outBlob.pbData),
                   static_cast<int>(outBlob.cbData));
    LocalFree(outBlob.pbData);
    return enc.toBase64();
}

QString DAAgentSettingsWidget::decryptApiKey(const QByteArray& encrypted)
{
    if (encrypted.isEmpty()) return {};
    QByteArray raw = QByteArray::fromBase64(encrypted);
    DATA_BLOB inBlob;
    inBlob.pbData = reinterpret_cast<BYTE*>(raw.data());
    inBlob.cbData = static_cast<DWORD>(raw.size());
    DATA_BLOB outBlob;
    if (!CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr,
                            0, &outBlob)) {
        return {};
    }
    QString result = QString::fromUtf8(
        reinterpret_cast<const char*>(outBlob.pbData),
        static_cast<int>(outBlob.cbData));
    LocalFree(outBlob.pbData);
    return result;
}
#else
// Fallback: base64 encoding (NOT secure — for non-Windows development only)
QByteArray DAAgentSettingsWidget::encryptApiKey(const QString& apiKey)
{
    return apiKey.toUtf8().toBase64();
}

QString DAAgentSettingsWidget::decryptApiKey(const QByteArray& encrypted)
{
    if (encrypted.isEmpty()) return {};
    return QString::fromUtf8(QByteArray::fromBase64(encrypted));
}
#endif

} // namespace DA
