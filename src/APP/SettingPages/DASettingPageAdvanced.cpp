#include "DASettingPageAdvanced.h"
#include <QFileDialog>
#include <QListWidgetItem>
#include <QSignalBlocker>
#include "ui_DASettingPageAdvanced.h"
#include "DAAppConfig.h"
#include "DALogCategory.h"
namespace DA
{

DASettingPageAdvanced::DASettingPageAdvanced(QWidget* parent)
    : DAAbstractSettingPage(parent)
    , ui(new Ui::DASettingPageAdvanced)
{
    ui->setupUi(this);
    // 工作流超时：-1 表示无限
    ui->doubleSpinBoxWorkflowTimeout->setRange(-1.0, 86400.0);
    ui->doubleSpinBoxWorkflowTimeout->setDecimals(1);
    ui->doubleSpinBoxWorkflowTimeout->setSingleStep(10.0);
    ui->doubleSpinBoxWorkflowTimeout->setSuffix(tr(" s"));
    ui->doubleSpinBoxWorkflowTimeout->setSpecialValueText(tr("Unlimited"));  // cn:无限
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
    ui->lineEditWorkspaceDir->setPlaceholderText(tr("Empty for system temporary directory"));  // cn:留空使用系统临时目录
    ui->spinBoxScriptTimeout->setRange(0, 3600);
    ui->spinBoxScriptTimeout->setSuffix(tr(" s"));
    ui->spinBoxScriptTimeout->setSpecialValueText(tr("Disabled"));  // cn:禁用
    ui->spinBoxScriptResultMaxChars->setRange(1000, 100000);
    // 信号连接
    connect(ui->doubleSpinBoxWorkflowTimeout,
            QOverload< double >::of(&QDoubleSpinBox::valueChanged),
            this,
            &DASettingPageAdvanced::onDoubleSpinBoxWorkflowTimeoutValueChanged);
    connect(ui->spinBoxRecentFilesMax,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageAdvanced::onSpinBoxRecentFilesMaxValueChanged);
    connect(ui->spinBoxDumpRetentionDays,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageAdvanced::onSpinBoxDumpRetentionDaysValueChanged);
    connect(ui->lineEditPluginPath, &QLineEdit::textChanged, this, &DASettingPageAdvanced::onLineEditPluginPathTextChanged);
    connect(ui->toolButtonPluginBrowse, &QToolButton::clicked, this, &DASettingPageAdvanced::onToolButtonPluginBrowseClicked);
    connect(ui->toolButtonAddNodePath, &QToolButton::clicked, this, &DASettingPageAdvanced::onToolButtonAddNodePathClicked);
    connect(ui->toolButtonRemoveNodePath,
            &QToolButton::clicked,
            this,
            &DASettingPageAdvanced::onToolButtonRemoveNodePathClicked);
    connect(ui->checkBoxShowSplash, &QCheckBox::stateChanged, this, &DASettingPageAdvanced::onCheckBoxShowSplashStateChanged);
    connect(ui->spinBoxAutosaveInterval,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageAdvanced::onSpinBoxAutosaveIntervalValueChanged);
    connect(ui->lineEditWorkspaceDir,
            &QLineEdit::textChanged,
            this,
            &DASettingPageAdvanced::onLineEditWorkspaceDirTextChanged);
    connect(ui->toolButtonWorkspaceDirBrowse,
            &QToolButton::clicked,
            this,
            &DASettingPageAdvanced::onToolButtonWorkspaceDirBrowseClicked);
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
    DAAppConfig& cfg = *mAppConfig;
    cfg[ DA_CONFIG_KEY_WORKFLOW_TIMEOUT ]       = ui->doubleSpinBoxWorkflowTimeout->value();
    cfg[ DA_CONFIG_KEY_RECENT_FILES_MAX ]       = ui->spinBoxRecentFilesMax->value();
    cfg[ DA_CONFIG_KEY_DUMP_RETENTION_DAYS ]    = ui->spinBoxDumpRetentionDays->value();
    // 插件路径（单路径，存入 QStringList 的首元素）
    QString pluginPath                          = ui->lineEditPluginPath->text().trimmed();
    cfg[ DA_CONFIG_KEY_PLUGIN_EXTRA_PATHS ]     = pluginPath.isEmpty() ? QStringList() : QStringList() << pluginPath;
    // 节点脚本路径
    QStringList nodePaths;
    for (int i = 0; i < ui->listWidgetNodePaths->count(); ++i) {
        QListWidgetItem* it = ui->listWidgetNodePaths->item(i);
        if (it && !it->text().trimmed().isEmpty()) {
            nodePaths << it->text().trimmed();
        }
    }
    cfg[ DA_CONFIG_KEY_NODE_SCRIPT_PATHS ] = nodePaths;
    cfg[ DA_CONFIG_KEY_SHOW_SPLASH ]       = ui->checkBoxShowSplash->isChecked();
    cfg[ DA_CONFIG_KEY_AUTOSAVE_INTERVAL ] = ui->spinBoxAutosaveInterval->value();
    // 脚本工作区
    cfg[ DA_CONFIG_KEY_WORKSPACE_DIR ]             = ui->lineEditWorkspaceDir->text().trimmed();
    cfg[ DA_CONFIG_KEY_SCRIPT_TIMEOUT ]            = ui->spinBoxScriptTimeout->value();
    cfg[ DA_CONFIG_KEY_SCRIPT_RESULT_MAX_CHARS ]   = ui->spinBoxScriptResultMaxChars->value();
    cfg.apply();
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
    DAAppConfig& cfg = *p;
    // 工作流超时
    ui->doubleSpinBoxWorkflowTimeout->setValue(cfg[ DA_CONFIG_KEY_WORKFLOW_TIMEOUT ].toDouble());
    // 最近文件数
    int recentMax = cfg[ DA_CONFIG_KEY_RECENT_FILES_MAX ].toInt();
    ui->spinBoxRecentFilesMax->setValue(recentMax > 0 ? recentMax : 10);
    // dump 保留天数
    int dumpDays = cfg[ DA_CONFIG_KEY_DUMP_RETENTION_DAYS ].toInt();
    ui->spinBoxDumpRetentionDays->setValue(dumpDays > 0 ? dumpDays : 7);
    // 插件路径
    QStringList pluginPaths = cfg[ DA_CONFIG_KEY_PLUGIN_EXTRA_PATHS ].toStringList();
    ui->lineEditPluginPath->setText(pluginPaths.isEmpty() ? QString() : pluginPaths.first());
    // 节点脚本路径
    QStringList nodePaths = cfg[ DA_CONFIG_KEY_NODE_SCRIPT_PATHS ].toStringList();
    ui->listWidgetNodePaths->clear();
    for (const QString& s : nodePaths) {
        if (!s.trimmed().isEmpty()) {
            ui->listWidgetNodePaths->addItem(s);
        }
    }
    // 启动画面
    ui->checkBoxShowSplash->setChecked(cfg[ DA_CONFIG_KEY_SHOW_SPLASH ].toBool());
    // 自动保存间隔
    ui->spinBoxAutosaveInterval->setValue(cfg[ DA_CONFIG_KEY_AUTOSAVE_INTERVAL ].toInt());
    // 脚本工作区
    ui->lineEditWorkspaceDir->setText(cfg[ DA_CONFIG_KEY_WORKSPACE_DIR ].toString());
    int scriptTimeout = cfg[ DA_CONFIG_KEY_SCRIPT_TIMEOUT ].toInt();
    ui->spinBoxScriptTimeout->setValue(scriptTimeout >= 0 ? scriptTimeout : 300);
    int resultMaxChars = cfg[ DA_CONFIG_KEY_SCRIPT_RESULT_MAX_CHARS ].toInt();
    ui->spinBoxScriptResultMaxChars->setValue(resultMaxChars > 0 ? resultMaxChars : 10000);
    return true;
}

void DASettingPageAdvanced::onDoubleSpinBoxWorkflowTimeoutValueChanged(double v)
{
    Q_UNUSED(v);
    emit settingChanged();
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

void DASettingPageAdvanced::onLineEditPluginPathTextChanged(const QString& text)
{
    Q_UNUSED(text);
    emit settingChanged();
}

void DASettingPageAdvanced::onToolButtonPluginBrowseClicked()
{
    QString d = QFileDialog::getExistingDirectory(this, tr("Select plugin search path"));  // cn:选择插件搜索路径
    if (!d.isEmpty()) {
        ui->lineEditPluginPath->setText(d);
        emit settingChanged();
    }
}

void DASettingPageAdvanced::onToolButtonAddNodePathClicked()
{
    QString d = QFileDialog::getExistingDirectory(this, tr("Select node script search path"));  // cn:选择节点脚本搜索路径
    if (!d.isEmpty()) {
        ui->listWidgetNodePaths->addItem(d);
        emit settingChanged();
    }
}

void DASettingPageAdvanced::onToolButtonRemoveNodePathClicked()
{
    int row = ui->listWidgetNodePaths->currentRow();
    if (row >= 0) {
        delete ui->listWidgetNodePaths->takeItem(row);
        emit settingChanged();
    }
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

void DASettingPageAdvanced::onLineEditWorkspaceDirTextChanged(const QString& text)
{
    Q_UNUSED(text);
    emit settingChanged();
}

void DASettingPageAdvanced::onToolButtonWorkspaceDirBrowseClicked()
{
    QString d = QFileDialog::getExistingDirectory(this, tr("Select script workspace directory"));  // cn:选择脚本工作区目录
    if (!d.isEmpty()) {
        ui->lineEditWorkspaceDir->setText(d);
        emit settingChanged();
    }
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
