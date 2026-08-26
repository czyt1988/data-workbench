#pragma once
#include "DAAbstractSettingPage.h"
#include "DAAgentInterface.h"
#include <QJsonObject>

class QComboBox;
class QSpinBox;
class QCheckBox;
class QLineEdit;
class QLabel;
class QTableWidget;
class QPushButton;
class QPlainTextEdit;

namespace DA
{

/**
 * @brief Agent 权限设置页（permission-layer P1 骨架 + P2 补全）
 *
 * 提供权限模式默认值、审批超时、manual 拦截开关、判官模型等基础项，以及
 * 路径规则表（增删改，全局硬 deny 行锁定）、代码危险模式清单（deny/escalate
 * 编辑，附咨询性防线说明）、工具分级覆盖（tier_overrides）的可视化编辑。
 * 读写经 DAAgentInterface::getPermissionConfig / setPermissionConfig，
 * 保存后由接口侧触发 reconfigure 下发 Python（链路在 P1 就绪）。
 */
class DAAgentPermissionSettingsWidget : public DAAbstractSettingPage
{
    Q_OBJECT
public:
    explicit DAAgentPermissionSettingsWidget(QWidget* parent = nullptr);

    QString getSettingPageTitle() const override
    {
        return tr("Agent Permission Settings");  //cn:Agent 权限设置
    }
    QIcon getSettingPageIcon() const override;
    void apply() override;
    void setAgentInterface(DAAgentInterface* p);

private:
    void setupUI();
    void loadConfig();
    void saveConfig();
    // 路径规则表
    void appendRuleRow(const QString& tool, const QString& scope, const QString& action);
    void applyRowLock(int row);
    // 分级覆盖表
    void appendTierRow(const QString& tool, const QString& tier);
    // tool=="*" 且 action=="deny" 的全局拒绝行为硬 deny，界面锁定（母文档 §7.1）
    static bool isLockedRule(const QString& tool, const QString& action);

private:
    DAAgentInterface* mAgentInterface { nullptr };

    QComboBox* mModeCombo;          ///< 默认权限模式（yolo/auto/manual）
    QSpinBox* mApprovalTimeoutSpin; ///< gated_tools 审批长超时（秒）
    QCheckBox* mManualBlockInapp;   ///< manual 模式是否拦截应用内修改工具
    QLineEdit* mJudgeModelEdit;     ///< 判官模型名（空=未配置）
    QSpinBox* mJudgeTimeoutSpin;    ///< 判官单次调用超时（秒）
    QLabel* mJudgeHint;             ///< 判官未配置时 auto 仍会询问的说明

    QTableWidget* mRulesTable;      ///< 路径规则表（tool / scope / action）
    QPushButton* mRuleAddBtn;       ///< 新增规则
    QPushButton* mRuleRemoveBtn;    ///< 删除选中规则（锁定行不可删）
    QPlainTextEdit* mDenyPatternsEdit;     ///< code_patterns.deny（每行一条正则）
    QPlainTextEdit* mEscalatePatternsEdit; ///< code_patterns.escalate（每行一条正则）
    QTableWidget* mTierTable;       ///< 分级覆盖表（tool / tier）
    QPushButton* mTierAddBtn;       ///< 新增分级覆盖
    QPushButton* mTierRemoveBtn;    ///< 删除选中分级覆盖
};

} // namespace DA
