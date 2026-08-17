#pragma once
#include "DAAbstractSettingPage.h"
#include "DAAgentInterface.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>

class QTabWidget;

namespace DA
{

/**
 * @brief Agent LLM 设置页：多供应商多模型管理 + Agent 其它设置（超时/上下文/会话/容错）
 *
 * 采用 QTabWidget 分两页：
 *  - Tab "Model Providers"：供应商 CRUD（名称/base_url/api_key/模型 id 列表），可增删供应商与模型，
 *    提供连接测试。持久化经 DAAgentInterface::getProviders/setProviders（api_key 内部加解密）。
 *  - Tab "Agent Settings"：超时/上下文管理/会话保留/重连容错/预启动开关，经 getLLMConfig/setLLMConfig。
 *
 * 继承 DAAbstractSettingPage（非 QWidget），注册到平台设置系统。
 */
class DAAgentSettingsWidget : public DAAbstractSettingPage
{
    Q_OBJECT
public:
    // 构造函数
    explicit DAAgentSettingsWidget(QWidget* parent = nullptr);

    /// DAAbstractSettingPage 必需重载（纯虚）
    // 获取设置页标题
    QString getSettingPageTitle() const override { return tr("Agent LLM Settings"); }  //cn:Agent LLM 设置

    // 获取设置页图标
    QIcon getSettingPageIcon() const override;

    // 应用设置，保存配置信息
    void apply() override;

    // 注入 Agent 接口，loadConfig/saveConfig 经此接口持久化（api_key 明文经接口，DAAgent 内部加解密）
    void setAgentInterface(DAAgentInterface* p);

private Q_SLOTS:
    void onTestConnection();
    // 供应商 CRUD
    void onAddProvider();
    void onRemoveProvider();
    void onProviderSelected(int row);
    void onProviderNameEdited(const QString& text);
    // 模型 CRUD
    void onAddModel();
    void onRemoveModel();

private:
    void setupUI();
    void setupProvidersTab();
    void setupAgentSettingsTab();
    void loadConfig();
    void saveConfig();
    // 供应商 Tab 辅助
    void refreshProviderList();
    void loadProviderToForm(int idx);
    void saveFormToProvider(int idx);
    int currentProviderRow() const;
    int currentModelIndex() const;

private:
    DAAgentInterface* mAgentInterface { nullptr };  ///< 持久化接口

    // ---- 顶层 Tab ----
    QTabWidget* mTabWidget;

    // ---- Tab1: 供应商管理 ----
    QListWidget* mProviderList;          ///< 供应商列表（每项名称 + 激活标记）
    QLineEdit* mProviderNameEdit;        ///< 供应商名称
    QLineEdit* mProviderBaseUrlEdit;     ///< base_url
    QLineEdit* mProviderApiKeyEdit;      // api_key (EchoMode::Password)
    QListWidget* mModelList;             ///< 模型 id 列表
    QLineEdit* mModelIdEdit;             ///< 新增模型 id 输入
    QPushButton* mAddModelBtn;
    QPushButton* mRemoveModelBtn;
    QPushButton* mAddProviderBtn;
    QPushButton* mRemoveProviderBtn;
    QPushButton* mTestBtn;               ///< 测试连接（测试当前选中供应商 + 其首个模型）
    QLabel* mStatusLabel;                ///< 测试结果
    QNetworkAccessManager* mNetworkManager { nullptr };
    QJsonArray mProviders;               ///< 内存中的供应商缓存（api_key 明文）
    int mCurrentProviderIdx { -1 };      ///< 当前选中供应商索引

    // ---- Tab2: Agent 其它设置 ----
    QSpinBox* mReadyTimeoutSpin;  // ready 等待超时(秒)
    QSpinBox* mStopTimeoutSpin;   // 停止等待超时(秒)
    // 上下文管理
    QSpinBox* mContextWindowSpin;             // 模型上下文窗口大小(tokens)
    QDoubleSpinBox* mCompactionThresholdSpin;  // 压缩触发比例(0-1)
    QSpinBox* mMaxRecentMsgSpin;              // 压缩后保留最近消息数
    QSpinBox* mToolResultMaxCharsSpin;         // 工具结果截断阈值(字符)
    QSpinBox* mToolResultPreviewCharsSpin;      // 工具结果预览长度(字符)
    // 会话持久化（plan-06）
    QSpinBox* mMaxSessionsSpin;                // 自由会话保留数量(5-200)
    QSpinBox* mSessionRetentionDaysSpin;       // 自由会话保留天数(1-365)
    // 重连与容错（plan-05）
    QSpinBox* mSpinMaxRetries;           // LLM 最大重试次数(0-20)
    QSpinBox* mSpinRequestTimeout;       // LLM 单次请求超时(秒)
    QSpinBox* mSpinInactivityTimeout;    // 无活动看门狗超时(秒)
    QSpinBox* mSpinMaxRestarts;          // 子进程最大重启次数(0-10)
    QSpinBox* mSpinRecursionLimit;       // LangGraph 图最大迭代步数(20-1000)
    QCheckBox* mCheckAutoPrestart;        // 启动时自动预热 Agent（默认勾选）
};

} // namespace DA
