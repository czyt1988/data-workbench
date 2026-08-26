#pragma once
#include "DAAbstractSettingPage.h"
#include "DAAgentInterface.h"
#include <QJsonObject>

class QComboBox;
class QSpinBox;
class QCheckBox;
class QLineEdit;
class QLabel;

namespace DA
{

/**
 * @brief Agent 权限设置页（permission-layer P1 骨架）
 *
 * 提供权限模式默认值、审批超时、manual 拦截开关、判官模型等基础项的查看与编辑。
 * 规则表 / 危险模式 / 分级覆盖的可视化编辑属计划二，本页暂不承载。
 * 读写经 DAAgentInterface::getPermissionConfig / setPermissionConfig。
 */
class DAAgentPermissionSettingsWidget : public DAAbstractSettingPage
{
    Q_OBJECT
public:
    explicit DAAgentPermissionSettingsWidget(QWidget* parent = nullptr);

    QString getSettingPageTitle() const override { return tr("Agent Permission Settings"); }  //cn:Agent 权限设置
    QIcon getSettingPageIcon() const override;
    void apply() override;
    void setAgentInterface(DAAgentInterface* p);

private:
    void setupUI();
    void loadConfig();
    void saveConfig();

private:
    DAAgentInterface* mAgentInterface { nullptr };

    QComboBox* mModeCombo;          ///< 默认权限模式（yolo/auto/manual）
    QSpinBox* mApprovalTimeoutSpin; ///< gated_tools 审批长超时（秒）
    QCheckBox* mManualBlockInapp;   ///< manual 模式是否拦截应用内修改工具
    QLineEdit* mJudgeModelEdit;     ///< 判官模型名（空=未配置）
    QSpinBox* mJudgeTimeoutSpin;    ///< 判官单次调用超时（秒）
    QLabel* mJudgeHint;             ///< 判官未配置时 auto 仍会询问的说明
};

} // namespace DA
