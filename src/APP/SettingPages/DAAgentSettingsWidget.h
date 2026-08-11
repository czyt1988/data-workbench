#pragma once
#include "DAAbstractSettingPage.h"
#include "DAAgentInterface.h"
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
 * 持久化经 setAgentInterface 注入的 DAAgentInterface 的 get/setLLMConfig
 * 走 agent-config.ini（DAAgent 库内部加解密 api_key），设置页只传明文 QJsonObject。
 *
 * 参考实现：data-workbench/src/APP/SettingPages/DASettingPagePython.h
 */
class DAAgentSettingsWidget : public DAAbstractSettingPage
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

    /**
     * @brief 注入 Agent 接口，loadConfig/saveConfig 经此接口持久化（api_key 明文经接口，DAAgent 内部加解密）
     * @param p DAAgentInterface 实例（由 DAAppSettingDialog 经 config->getCore()->getAgentInterface() 取得）
     */
    void setAgentInterface(DAAgentInterface* p);

private Q_SLOTS:
    void onTestConnection();

private:
    void setupUI();
    void loadConfig();
    void saveConfig();

private:
    DAAgentInterface* m_agentInterface { nullptr };  ///< 持久化接口（loadConfig/saveConfig 经此走 agent-config.ini）
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
    // 会话持久化（plan-06）：自由会话保留数量与保留天数，供 DAAgentModule::cleanupSessions 读取
    QSpinBox* m_maxSessionsSpin;                // 自由会话保留数量(5-200)
    QSpinBox* m_sessionRetentionDaysSpin;       // 自由会话保留天数(1-365)
    // 重连与容错（plan-05）：LLM 重试/超时/子进程重启，供 DAAgentModule get/setLLMConfig 读写
    QSpinBox* m_spinMaxRetries;           // LLM 最大重试次数(0-20)
    QSpinBox* m_spinRequestTimeout;       // LLM 单次请求超时(秒)
    QSpinBox* m_spinInactivityTimeout;    // 无活动看门狗超时(秒)
    QSpinBox* m_spinMaxRestarts;          // 子进程最大重启次数(0-10)
    QSpinBox* m_spinRecursionLimit;       // LangGraph 图最大迭代步数(20-1000)
};

} // namespace DA
