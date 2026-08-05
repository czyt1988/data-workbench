#pragma once
#include "DAGuiAPI.h"
#include "DAAbstractSettingPage.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
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
class DAGUI_API DAAgentSettingsWidget : public DAAbstractSettingPage
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit DAAgentSettingsWidget(QWidget* parent = nullptr);

    /// DAAbstractSettingPage 必需重载（纯虚）
    /**
     * @brief 获取设置页标题
     * @return 设置页标题文本
     */
    QString getSettingPageTitle() const override { return tr("Agent LLM 设置"); }

    /**
     * @brief 获取设置页图标
     * @return 设置页图标
     */
    QIcon getSettingPageIcon() const override;

    /**
     * @brief 应用设置，保存配置信息
     */
    void apply() override;

    /// DPAPI 加密/解密辅助（public，供 DAAgentModule::getLLMConfig 调用）
    /**
     * @brief 加密 API Key（DPAPI，Windows）
     * @param apiKey 明文 API Key
     * @return 加密后的 Base64 字节数组
     */
    static QByteArray encryptApiKey(const QString& apiKey);

    /**
     * @brief 解密 API Key（DPAPI，Windows）
     * @param encrypted 加密的 Base64 字节数组
     * @return 解密后的明文 API Key
     */
    static QString decryptApiKey(const QByteArray& encrypted);

private Q_SLOTS:
    void onTestConnection();

private:
    void setupUI();
    void loadConfig();
    void saveConfig();

private:
    QLineEdit* m_baseUrlEdit;
    QLineEdit* m_apiKeyEdit;       // EchoMode::Password
    QLineEdit* m_modelEdit;        // 模型名
    QSpinBox* m_readyTimeoutSpin;  // ready 等待超时(秒)
    QSpinBox* m_stopTimeoutSpin;   // 停止等待超时(秒)
    QPushButton* m_testBtn;
    QLabel* m_statusLabel;
    QNetworkAccessManager* m_networkManager { nullptr };  // 主线程异步，构造函数中创建
    // 上下文管理
    QSpinBox* m_contextWindowSpin;             // 模型上下文窗口大小(tokens)
    QDoubleSpinBox* m_compactionThresholdSpin;  // 压缩触发比例(0-1)
    QSpinBox* m_maxRecentMsgSpin;              // 压缩后保留最近消息数
    QSpinBox* m_toolResultMaxCharsSpin;         // 工具结果截断阈值(字符)
    QSpinBox* m_toolResultPreviewCharsSpin;      // 工具结果预览长度(字符)
};

} // namespace DA
