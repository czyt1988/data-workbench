// DAAgentSettingsWidget.cpp
#include "DAAgentSettingsWidget.h"
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
    m_testBtn     = new QPushButton(tr("测试连接"), this);
    m_statusLabel = new QLabel(this);

    QFormLayout* form = new QFormLayout(this);
    form->addRow(tr("Base URL"), m_baseUrlEdit);
    form->addRow(tr("API Key"),  m_apiKeyEdit);
    form->addRow(tr("Model"),    m_modelEdit);
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

    // 注：loadConfig() 期间无需 QSignalBlocker——
    // addPage()（连接 settingChanged → 脏页追踪器）在构造函数返回之后才执行，
    // 因此 loadConfig() 中触发的信号没有监听者，不会污染脏页集合。
}

void DAAgentSettingsWidget::loadConfig()
{
    QSettings s;
    m_baseUrlEdit->setText(s.value(KEY_LLM_BASE_URL).toString());
    m_modelEdit->setText(s.value(KEY_LLM_MODEL).toString());
    QByteArray encKey = s.value(KEY_LLM_API_KEY).toByteArray();
    if (!encKey.isEmpty()) {
        m_apiKeyEdit->setText(decryptApiKey(encKey));
    }
}

void DAAgentSettingsWidget::saveConfig()
{
    QSettings s;
    s.setValue(KEY_LLM_BASE_URL, m_baseUrlEdit->text().trimmed());
    s.setValue(KEY_LLM_MODEL,    m_modelEdit->text().trimmed());
    s.setValue(KEY_LLM_API_KEY,  encryptApiKey(m_apiKeyEdit->text()));
}

void DAAgentSettingsWidget::apply()
{
    saveConfig();
    emit settingApplyed();
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
