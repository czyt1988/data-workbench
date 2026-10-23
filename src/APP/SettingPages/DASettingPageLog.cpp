#include "DASettingPageLog.h"
#include <QSignalBlocker>
#include "ui_DASettingPageLog.h"
#include "DAAppConfig.h"
#include "DALogger.h"
#include "DALogCategory.h"
namespace DA
{

DASettingPageLog::DASettingPageLog(QWidget* parent)
    : DAAbstractSettingPage(parent)
    , ui(new Ui::DASettingPageLog)
{
    ui->setupUi(this);
    // 填充级别下拉
    fillLogLevelCombo(ui->comboBoxLogLevel);
    fillLogLevelCombo(ui->comboBoxQueueLevel);
    // 轮转模式
    ui->comboBoxRotationMode->blockSignals(true);
    ui->comboBoxRotationMode->clear();
    ui->comboBoxRotationMode->addItem(tr("Rotating"), 0);  // cn:按大小轮转
    ui->comboBoxRotationMode->addItem(tr("Daily"), 1);       // cn:按日期分割
    ui->comboBoxRotationMode->addItem(tr("Console only"), 2);  // cn:仅控制台
    ui->comboBoxRotationMode->blockSignals(false);
    // 数值范围
    ui->spinBoxMaxSize->setRange(1, 1024);
    ui->spinBoxMaxSize->setSuffix(tr(" MB"));
    ui->spinBoxMaxFiles->setRange(1, 100);
    ui->spinBoxDisplayLogsNum->setRange(10, 99999);
    ui->spinBoxDisplayLogsNum->setSingleStep(1000);
    // 信号连接
    connect(ui->comboBoxLogLevel,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASettingPageLog::onComboBoxLogLevelCurrentIndexChanged);
    connect(ui->comboBoxQueueLevel,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASettingPageLog::onComboBoxQueueLevelCurrentIndexChanged);
    connect(ui->checkBoxOutputStdout, &QCheckBox::stateChanged, this, &DASettingPageLog::onCheckBoxOutputStdoutStateChanged);
    connect(ui->comboBoxRotationMode,
            QOverload< int >::of(&QComboBox::currentIndexChanged),
            this,
            &DASettingPageLog::onComboBoxRotationModeCurrentIndexChanged);
    connect(ui->spinBoxMaxSize,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageLog::onSpinBoxMaxSizeValueChanged);
    connect(ui->spinBoxMaxFiles,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageLog::onSpinBoxMaxFilesValueChanged);
    connect(ui->spinBoxDisplayLogsNum,
            QOverload< int >::of(&QSpinBox::valueChanged),
            this,
            &DASettingPageLog::onSpinBoxDisplayLogsNumValueChanged);
}

DASettingPageLog::~DASettingPageLog()
{
    delete ui;
}

void DASettingPageLog::fillLogLevelCombo(QComboBox* combo)
{
    combo->blockSignals(true);
    combo->clear();
    combo->addItem(tr("Trace"), static_cast< int >(DA::DALogLevel::Trace));    // cn:跟踪
    combo->addItem(tr("Debug"), static_cast< int >(DA::DALogLevel::Debug));    // cn:调试
    combo->addItem(tr("Info"), static_cast< int >(DA::DALogLevel::Info));      // cn:信息
    combo->addItem(tr("Warning"), static_cast< int >(DA::DALogLevel::Warn));  // cn:警告
    combo->addItem(tr("Error"), static_cast< int >(DA::DALogLevel::Error));   // cn:错误
    combo->addItem(tr("Critical"), static_cast< int >(DA::DALogLevel::Critical));  // cn:严重
    combo->addItem(tr("Off"), static_cast< int >(DA::DALogLevel::Off));            // cn:关闭
    combo->blockSignals(false);
}

void DASettingPageLog::apply()
{
    if (nullptr == mAppConfig) {
        return;
    }
    DALogSettings s = mAppConfig->log();
    s.level         = static_cast< DALogLevel >(ui->comboBoxLogLevel->currentData().toInt());
    s.queueLevel    = static_cast< DALogLevel >(ui->comboBoxQueueLevel->currentData().toInt());
    s.outputStdout  = ui->checkBoxOutputStdout->isChecked();
    s.rotationMode  = static_cast< DALogRotationMode >(ui->comboBoxRotationMode->currentData().toInt());
    s.maxSizeBytes  = qint64(ui->spinBoxMaxSize->value()) * 1024 * 1024;  // MB -> bytes
    s.maxFiles      = ui->spinBoxMaxFiles->value();
    s.showLogNum    = ui->spinBoxDisplayLogsNum->value();
    // setLog 会发出 sectionChanged，由主窗口负责热应用；保存由设置对话框触发
    mAppConfig->setLog(s);
    emit settingApplyed();
}

QString DASettingPageLog::getSettingPageTitle() const
{
    return tr("Log");  // cn:日志
}

QIcon DASettingPageLog::getSettingPageIcon() const
{
    return QIcon(":/DAGui/icon/setting-log.svg");
}

bool DASettingPageLog::setAppConfig(DAAppConfig* p)
{
    if (nullptr == p) {
        return false;
    }
    QSignalBlocker blocker(this);
    mAppConfig = p;
    const DALogSettings& s = p->log();
    // 日志级别
    int level = static_cast< int >(s.level);
    int lidx  = ui->comboBoxLogLevel->findData(level);
    ui->comboBoxLogLevel->setCurrentIndex(lidx >= 0 ? lidx : 0);
    // UI 队列级别
    int qlevel = static_cast< int >(s.queueLevel);
    int qidx   = ui->comboBoxQueueLevel->findData(qlevel);
    ui->comboBoxQueueLevel->setCurrentIndex(qidx >= 0 ? qidx : 0);
    // stdout
    ui->checkBoxOutputStdout->setChecked(s.outputStdout);
    // 轮转模式
    int mode = static_cast< int >(s.rotationMode);
    int midx = ui->comboBoxRotationMode->findData(mode);
    ui->comboBoxRotationMode->setCurrentIndex(midx >= 0 ? midx : 0);
    // 单文件大小(bytes -> MB)，取值已在加载时规范化
    ui->spinBoxMaxSize->setValue(static_cast< int >(s.maxSizeBytes / (1024 * 1024)));
    // 保留文件数
    ui->spinBoxMaxFiles->setValue(s.maxFiles);
    // 显示条数
    ui->spinBoxDisplayLogsNum->setValue(s.showLogNum);
    return true;
}

void DASettingPageLog::onComboBoxLogLevelCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    emit settingChanged();
}

void DASettingPageLog::onComboBoxQueueLevelCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    emit settingChanged();
}

void DASettingPageLog::onCheckBoxOutputStdoutStateChanged(int state)
{
    Q_UNUSED(state);
    emit settingChanged();
}

void DASettingPageLog::onComboBoxRotationModeCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    emit settingChanged();
}

void DASettingPageLog::onSpinBoxMaxSizeValueChanged(int v)
{
    Q_UNUSED(v);
    emit settingChanged();
}

void DASettingPageLog::onSpinBoxMaxFilesValueChanged(int v)
{
    Q_UNUSED(v);
    emit settingChanged();
}

void DASettingPageLog::onSpinBoxDisplayLogsNumValueChanged(int v)
{
    Q_UNUSED(v);
    emit settingChanged();
}

}  // end DA
