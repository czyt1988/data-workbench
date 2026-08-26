// DAAgentPermissionSettingsWidget.cpp
// permission-layer P1 骨架：权限模式默认值 / 审批超时 / manual 拦截开关 / 判官配置。
// 读写经 DAAgentInterface::getPermissionConfig / setPermissionConfig（contains 守卫，
// 规则表 / 危险模式 / 分级覆盖等未携带的 key 不受影响）。
#include "DAAgentPermissionSettingsWidget.h"
#include "DALogCategory.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QSignalBlocker>
#include <QIcon>

namespace {
// Qt5/Qt6 双兼容 helper：取值并兜底默认值（QJsonObject::value(key,default) Qt5 不存在）
int jsonInt(const QJsonObject& o, const char* key, int def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isDouble() ? v.toInt() : def;
}

bool jsonBool(const QJsonObject& o, const char* key, bool def)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isBool() ? v.toBool() : def;
}

QString jsonString(const QJsonObject& o, const char* key)
{
    QJsonValue v = o.value(QLatin1String(key));
    return v.isString() ? v.toString() : QString();
}
}  // namespace

namespace DA
{

/** @brief 构造 Agent 权限设置页控件 */
DAAgentPermissionSettingsWidget::DAAgentPermissionSettingsWidget(QWidget* parent)
    : DAAbstractSettingPage(parent)
{
    setupUI();
}

/** @brief 注入 Agent 接口并加载已有配置 */
void DAAgentPermissionSettingsWidget::setAgentInterface(DAAgentInterface* p)
{
    mAgentInterface = p;
    if (p) {
        QSignalBlocker blocker(this);
        loadConfig();
    }
}

/** @brief 构建设置页界面 */
void DAAgentPermissionSettingsWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // ---- 默认权限模式 ----
    QGroupBox* modeBox = new QGroupBox(tr("Default Permission Mode"), this);  //cn:默认权限模式
    QFormLayout* modeForm = new QFormLayout(modeBox);
    mModeCombo = new QComboBox(modeBox);
    // 展示顺序：auto（推荐默认）→ manual → yolo；data 存协议值
    mModeCombo->addItem(tr("Auto (rules + judge)"), QStringLiteral("auto"));       //cn:自动（规则+判官）
    mModeCombo->addItem(tr("Manual (ask every write/code)"), QStringLiteral("manual"));  //cn:手动（写入/代码每次询问）
    mModeCombo->addItem(tr("Full Auto (yolo)"), QStringLiteral("yolo"));           //cn:全自动（yolo）
    modeForm->addRow(tr("Mode:"), mModeCombo);  //cn:模式：
    mainLayout->addWidget(modeBox);

    // ---- 审批与拦截 ----
    QGroupBox* askBox = new QGroupBox(tr("Approval"), this);  //cn:审批
    QFormLayout* askForm = new QFormLayout(askBox);
    mApprovalTimeoutSpin = new QSpinBox(askBox);
    mApprovalTimeoutSpin->setRange(10, 86400);
    mApprovalTimeoutSpin->setSuffix(QStringLiteral(" s"));
    mApprovalTimeoutSpin->setToolTip(tr("Max wait for user approval on file writes / code execution"));  //cn:文件写入/代码执行等待用户批准的最长时间
    askForm->addRow(tr("Approval timeout:"), mApprovalTimeoutSpin);  //cn:审批超时：
    mManualBlockInapp = new QCheckBox(tr("In manual mode, also ask before in-app chart edits"), askBox);  //cn:手动模式下，应用内图表修改也需询问
    askForm->addRow(QString(), mManualBlockInapp);
    mainLayout->addWidget(askBox);

    // ---- 判官配置 ----
    QGroupBox* judgeBox = new QGroupBox(tr("Code Judge (optional)"), this);  //cn:代码判官（可选）
    QFormLayout* judgeForm = new QFormLayout(judgeBox);
    mJudgeModelEdit = new QLineEdit(judgeBox);
    mJudgeModelEdit->setPlaceholderText(tr("Leave empty to disable the judge"));  //cn:留空则不启用判官
    judgeForm->addRow(tr("Judge model:"), mJudgeModelEdit);  //cn:判官模型：
    mJudgeTimeoutSpin = new QSpinBox(judgeBox);
    mJudgeTimeoutSpin->setRange(1, 600);
    mJudgeTimeoutSpin->setSuffix(QStringLiteral(" s"));
    judgeForm->addRow(tr("Judge timeout:"), mJudgeTimeoutSpin);  //cn:判官超时：
    mJudgeHint = new QLabel(judgeBox);
    mJudgeHint->setWordWrap(true);
    mJudgeHint->setStyleSheet(QStringLiteral("color: #727272;"));
    mJudgeHint->setText(tr("When the judge is not configured, code execution in Auto mode "
                           "always asks for approval."));  //cn:未配置判官时，自动模式下代码执行仍会每次询问。
    judgeForm->addRow(QString(), mJudgeHint);
    mainLayout->addWidget(judgeBox);

    mainLayout->addStretch(1);

    // 任意控件变更 → 标记设置页为 dirty（顶层据此启用"应用"）
    auto mark = [this]() { emit settingChanged(); };
    connect(mModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, mark);
    connect(mApprovalTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
    connect(mManualBlockInapp, &QCheckBox::toggled, this, mark);
    connect(mJudgeModelEdit, &QLineEdit::textChanged, this, mark);
    connect(mJudgeTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, mark);
}

/** @brief 从接口加载权限配置到界面 */
void DAAgentPermissionSettingsWidget::loadConfig()
{
    if (!mAgentInterface) {
        daDebug << "[DAAgentPermissionSettings] loadConfig skipped: no agent interface injected";
        return;
    }
    const QJsonObject c = mAgentInterface->getPermissionConfig();
    const QString mode = jsonString(c, "mode");
    int idx = mModeCombo->findData(mode);
    if (idx < 0) {
        idx = mModeCombo->findData(QStringLiteral("auto"));  // 未知/缺失回退 auto
    }
    mModeCombo->setCurrentIndex(qMax(0, idx));
    mApprovalTimeoutSpin->setValue(jsonInt(c, "tool_approval_timeout_sec", 600));
    mManualBlockInapp->setChecked(jsonBool(c, "manual_block_inapp_tools", false));
    mJudgeModelEdit->setText(jsonString(c, "judge_model"));
    mJudgeTimeoutSpin->setValue(jsonInt(c, "judge_timeout_sec", 30));
}

/** @brief 将界面配置写回接口（contains 守卫，未携带的 key 不受影响） */
void DAAgentPermissionSettingsWidget::saveConfig()
{
    if (!mAgentInterface) {
        daDebug << "[DAAgentPermissionSettings] saveConfig skipped: no agent interface injected";
        return;
    }
    QJsonObject c;
    c[QStringLiteral("mode")]                      = mModeCombo->currentData().toString();
    c[QStringLiteral("tool_approval_timeout_sec")] = mApprovalTimeoutSpin->value();
    c[QStringLiteral("manual_block_inapp_tools")]  = mManualBlockInapp->isChecked();
    c[QStringLiteral("judge_model")]               = mJudgeModelEdit->text().trimmed();
    c[QStringLiteral("judge_timeout_sec")]         = mJudgeTimeoutSpin->value();
    mAgentInterface->setPermissionConfig(c);
}

/** @brief 应用设置页的配置变更 */
void DAAgentPermissionSettingsWidget::apply()
{
    daDebug << "[DAAgentPermissionSettings] apply entered";
    saveConfig();
    emit settingApplyed();
    daDebug << "[DAAgentPermissionSettings] apply done";
}

/** @brief 获取设置页图标（复用 agent 设置图标） */
QIcon DAAgentPermissionSettingsWidget::getSettingPageIcon() const
{
    return QIcon(":/DAGui/icon/setting-agent.svg");
}

} // namespace DA
