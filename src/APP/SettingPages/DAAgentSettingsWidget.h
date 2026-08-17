#pragma once
#include "DAAbstractSettingPage.h"
#include "DAAgentInterface.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>

class QTabWidget;
class QSplitter;
class QListWidget;
class QTableWidget;

namespace DA
{

/**
 * @brief Agent LLM 设置页：多供应商多模型管理 + Agent 其它设置
 *
 * QTabWidget 两页：
 *  - "Model Providers"：左侧 QSplitter 供应商列表（只读选择）+ 上方 Add/Edit/Remove 图标按钮；
 *    右侧只读信息面板（名称/base_url/api_key/模型表格）。增删改经弹出对话框完成，不在主页面内联编辑。
 *  - "Agent Settings"：超时/上下文阈值/会话/容错/预启动（context_window 已移至模型属性，不再在此编辑）。
 */
class DAAgentSettingsWidget : public DAAbstractSettingPage
{
    Q_OBJECT
public:
    // 构造函数
    explicit DAAgentSettingsWidget(QWidget* parent = nullptr);

    QString getSettingPageTitle() const override { return tr("Agent LLM Settings"); }  //cn:Agent LLM 设置
    QIcon getSettingPageIcon() const override;
    void apply() override;
    void setAgentInterface(DAAgentInterface* p);

private Q_SLOTS:
    void onAddProvider();
    void onEditProvider();
    void onRemoveProvider();
    void onProviderSelected(int row);

private:
    void setupUI();
    void setupProvidersTab();
    void setupAgentSettingsTab();
    void loadConfig();
    void saveConfig();
    // 供应商 Tab 辅助
    void refreshProviderList();
    void showProviderInfo(int idx);
    QStringList collectProviderNames(int excludeIdx = -1) const;

private:
    DAAgentInterface* mAgentInterface { nullptr };

    // 顶层 Tab
    QTabWidget* mTabWidget;

    // ---- Tab1: 供应商管理（只读展示） ----
    QSplitter* mSplitter;
    QListWidget* mProviderList;
    QPushButton* mAddProviderBtn;
    QPushButton* mEditProviderBtn;
    QPushButton* mRemoveProviderBtn;
    // 右侧只读信息面板
    QLabel* mInfoName;
    QLabel* mInfoBaseUrl;
    QLabel* mInfoApiKey;
    QTableWidget* mInfoModelTable;
    QJsonArray mProviders;  ///< 内存中的供应商缓存（api_key 明文）

    // ---- Tab2: Agent 其它设置 ----
    QSpinBox* mReadyTimeoutSpin;
    QSpinBox* mStopTimeoutSpin;
    QDoubleSpinBox* mCompactionThresholdSpin;
    QSpinBox* mMaxRecentMsgSpin;
    QSpinBox* mToolResultMaxCharsSpin;
    QSpinBox* mToolResultPreviewCharsSpin;
    QSpinBox* mMaxSessionsSpin;
    QSpinBox* mSessionRetentionDaysSpin;
    QSpinBox* mSpinMaxRetries;
    QSpinBox* mSpinRequestTimeout;
    QSpinBox* mSpinInactivityTimeout;
    QSpinBox* mSpinMaxRestarts;
    QSpinBox* mSpinRecursionLimit;
    QCheckBox* mCheckAutoPrestart;
};

} // namespace DA
