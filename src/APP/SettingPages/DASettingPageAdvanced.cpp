#include "DASettingPageAdvanced.h"
#include <QSignalBlocker>
#include "ui_DASettingPageAdvanced.h"
#include "DAAppConfig.h"
#include "DAFilePathEditWidget.h"
#include "DALogCategory.h"
namespace DA
{

DASettingPageAdvanced::DASettingPageAdvanced(QWidget* parent)
    : DAAbstractSettingPage(parent)
    , ui(new Ui::DASettingPageAdvanced)
{
    ui->setupUi(this);
    // 最近文件数
    ui->spinBoxRecentFilesMax->setRange(1, 100);
    // dump 保留天数
    ui->spinBoxDumpRetentionDays->setRange(1, 365);
    ui->spinBoxDumpRetentionDays->setSuffix(tr(" day"));  // cn: 天
    // 自动保存间隔
    ui->spinBoxAutosaveInterval->setRange(0, 1440);
    ui->spinBoxAutosaveInterval->setSuffix(tr(" min"));  // cn: 分钟
    ui->spinBoxAutosaveInterval->setSpecialValueText(tr("Disabled"));  // cn:禁用
    // 脚本工作区
    ui->filePathEditWorkspaceDir->setMode(DAFilePathEditWidget::Mode::ExistingDirectory);
    ui->filePathEditWorkspaceDir->setPlaceholderText(tr("Empty for system temporary directory"));  // cn:留空使用系统临时目录
    ui->spinBoxScriptTimeout->setRange(0, 3600);
    ui->spinBoxScriptTimeout->setSuffix(tr(" s"));
    ui->spinBoxScriptTimeout->setSpecialValueText(tr("Disabled"));  // cn:禁用
    ui->spinBoxScriptResultMaxChars->setRange(1000, 100000);
    // 信号连接
    connect(ui->spinBoxRecentFilesMax,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageAdvanced::onSpinBoxRecentFilesMaxValueChanged);
    connect(ui->spinBoxDumpRetentionDays,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageAdvanced::onSpinBoxDumpRetentionDaysValueChanged);
    connect(ui->checkBoxShowSplash, &QCheckBox::stateChanged, this, &DASettingPageAdvanced::onCheckBoxShowSplashStateChanged);
    connect(ui->spinBoxAutosaveInterval,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageAdvanced::onSpinBoxAutosaveIntervalValueChanged);
    connect(ui->filePathEditWorkspaceDir,
            &DAFilePathEditWidget::pathChanged,
            this,
            &DASettingPageAdvanced::onWorkspaceDirChanged);
    connect(ui->spinBoxScriptTimeout,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageAdvanced::onSpinBoxScriptTimeoutValueChanged);
    connect(ui->spinBoxScriptResultMaxChars,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageAdvanced::onSpinBoxScriptResultMaxCharsValueChanged);
}

DASettingPageAdvanced::~DASettingPageAdvanced()
{
    delete ui;
}

void DASettingPageAdvanced::apply()
{
    if (nullptr == mAppConfig) {
        return;
    }
    // 高级分节
    DAAdvancedSettings advanced = mAppConfig->advanced();
    advanced.recentFilesMax     = ui->spinBoxRecentFilesMax->value();
    advanced.dumpRetentionDays  = ui->spinBoxDumpRetentionDays->value();
    advanced.showSplash         = ui->checkBoxShowSplash->isChecked();
    advanced.autosaveIntervalMin = ui->spinBoxAutosaveInterval->value();
    // 脚本分节
    DAScriptSettings script = mAppConfig->script();
    script.workspaceDir     = ui->filePathEditWorkspaceDir->getFilePath().trimmed();
    script.scriptTimeoutSec = ui->spinBoxScriptTimeout->value();
    script.scriptResultMaxChars = ui->spinBoxScriptResultMaxChars->value();
    // setter 会发出 sectionChanged，由主窗口负责热应用；保存由设置对话框触发
    mAppConfig->setAdvanced(advanced);
    mAppConfig->setScript(script);
    emit settingApplyed();
}

QString DASettingPageAdvanced::getSettingPageTitle() const
{
    return tr("Advanced");  // cn:高级
}

QIcon DASettingPageAdvanced::getSettingPageIcon() const
{
    return QIcon(":/DAGui/icon/setting-advanced.svg");
}

bool DASettingPageAdvanced::setAppConfig(DAAppConfig* p)
{
    if (nullptr == p) {
        return false;
    }
    QSignalBlocker blocker(this);
    mAppConfig = p;
    const DAAdvancedSettings& advanced = p->advanced();
    // 最近文件数
    ui->spinBoxRecentFilesMax->setValue(advanced.recentFilesMax);
    // dump 保留天数
    ui->spinBoxDumpRetentionDays->setValue(advanced.dumpRetentionDays);
    // 启动画面
    ui->checkBoxShowSplash->setChecked(advanced.showSplash);
    // 自动保存间隔
    ui->spinBoxAutosaveInterval->setValue(advanced.autosaveIntervalMin);
    // 脚本工作区与执行引擎（取值已在加载时规范化）
    const DAScriptSettings& script = p->script();
    ui->filePathEditWorkspaceDir->setFilePath(script.workspaceDir);
    ui->spinBoxScriptTimeout->setValue(script.scriptTimeoutSec);
    ui->spinBoxScriptResultMaxChars->setValue(script.scriptResultMaxChars);
    return true;
}

void DASettingPageAdvanced::onSpinBoxRecentFilesMaxValueChanged(int v)
{
    Q_UNUSED(v);
    emit settingChanged();
}

void DASettingPageAdvanced::onSpinBoxDumpRetentionDaysValueChanged(int v)
{
    Q_UNUSED(v);
    emit settingChanged();
}

void DASettingPageAdvanced::onCheckBoxShowSplashStateChanged(int state)
{
    Q_UNUSED(state);
    emit settingChanged();
}

void DASettingPageAdvanced::onSpinBoxAutosaveIntervalValueChanged(int v)
{
    Q_UNUSED(v);
    emit settingChanged();
}

void DASettingPageAdvanced::onWorkspaceDirChanged(const QString& path)
{
    Q_UNUSED(path);
    emit settingChanged();
}

void DASettingPageAdvanced::onSpinBoxScriptTimeoutValueChanged(int v)
{
    Q_UNUSED(v);
    emit settingChanged();
}

void DASettingPageAdvanced::onSpinBoxScriptResultMaxCharsValueChanged(int v)
{
    Q_UNUSED(v);
    emit settingChanged();
}

}  // end DA
