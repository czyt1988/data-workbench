// DAAgentSettingsWidget.h —— Agent LLM 设置页（继承 DAAbstractSettingPage）
#pragma once
#include "DAAgentAPI.h"
#include "DAAbstractSettingPage.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>

namespace DA
{

/**
 * @brief Agent LLM 设置页：配置 base_url / api_key / model_name，提供连接测试
 *
 * 继承 DAAbstractSettingPage（非 QWidget），注册到平台设置系统。
 * 配置通过 QSettings 持久化（DAAgent 库无法链接 APP 的 DAAppConfig），
 * API Key 使用 DPAPI 加密存储（Windows，当前用户作用域）。
 *
 * 参考实现：data-workbench/src/APP/SettingPages/DASettingPagePython.h
 */
class DAAgent_API DAAgentSettingsWidget : public DAAbstractSettingPage
{
    Q_OBJECT
public:
    explicit DAAgentSettingsWidget(QWidget* parent = nullptr);

    // DAAbstractSettingPage 必需重载（纯虚）
    QString getSettingPageTitle() const override { return tr("Agent LLM 设置"); }
    QIcon getSettingPageIcon() const override;
    void apply() override;

    // DPAPI 加密/解密辅助（public，供 DAAgentModule::getLLMConfig 调用）
    static QByteArray encryptApiKey(const QString& apiKey);
    static QString decryptApiKey(const QByteArray& encrypted);

private slots:
    void onTestConnection();

private:
    void setupUI();
    void loadConfig();
    void saveConfig();

private:
    QLineEdit* m_baseUrlEdit;
    QLineEdit* m_apiKeyEdit;       // EchoMode::Password
    QLineEdit* m_modelEdit;        // 模型名
    QPushButton* m_testBtn;
    QLabel* m_statusLabel;
    QNetworkAccessManager* m_networkManager { nullptr };  // 主线程异步，构造函数中创建
};

} // namespace DA
