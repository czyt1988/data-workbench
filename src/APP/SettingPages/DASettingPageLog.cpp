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
    DAAppConfig& cfg = *mAppConfig;
    cfg[ DA_CONFIG_KEY_LOG_LEVEL ]           = ui->comboBoxLogLevel->currentData().toInt();
    cfg[ DA_CONFIG_KEY_LOG_QUEUE_LEVEL ]     = ui->comboBoxQueueLevel->currentData().toInt();
    cfg[ DA_CONFIG_KEY_LOG_OUTPUT_STDOUT ]   = ui->checkBoxOutputStdout->isChecked();
    cfg[ DA_CONFIG_KEY_LOG_ROTATION_MODE ]   = ui->comboBoxRotationMode->currentData().toInt();
    cfg[ DA_CONFIG_KEY_LOG_MAX_SIZE ]        = ui->spinBoxMaxSize->value() * 1024 * 1024;  // MB -> bytes
    cfg[ DA_CONFIG_KEY_LOG_MAX_FILES ]       = ui->spinBoxMaxFiles->value();
    cfg[ DA_CONFIG_KEY_SHOW_LOG_NUM ]        = ui->spinBoxDisplayLogsNum->value();
    cfg.apply();
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
    DAAppConfig& cfg = *p;
    // 日志级别
    int level = cfg[ DA_CONFIG_KEY_LOG_LEVEL ].toInt();
    int lidx  = ui->comboBoxLogLevel->findData(level);
    ui->comboBoxLogLevel->setCurrentIndex(lidx >= 0 ? lidx : 0);
    // UI 队列级别
    int qlevel = cfg[ DA_CONFIG_KEY_LOG_QUEUE_LEVEL ].toInt();
    int qidx   = ui->comboBoxQueueLevel->findData(qlevel);
    ui->comboBoxQueueLevel->setCurrentIndex(qidx >= 0 ? qidx : static_cast< int >(DA::DALogLevel::Info));
    // stdout
    ui->checkBoxOutputStdout->setChecked(cfg[ DA_CONFIG_KEY_LOG_OUTPUT_STDOUT ].toBool());
    // 轮转模式
    int mode = cfg[ DA_CONFIG_KEY_LOG_ROTATION_MODE ].toInt();
    int midx = ui->comboBoxRotationMode->findData(mode);
    ui->comboBoxRotationMode->setCurrentIndex(midx >= 0 ? midx : 0);
    // 单文件大小(bytes -> MB)
    int maxBytes = cfg[ DA_CONFIG_KEY_LOG_MAX_SIZE ].toInt();
    int maxMB    = (maxBytes > 0) ? (maxBytes / (1024 * 1024)) : 10;
    if (maxMB < 1) {
        maxMB = 1;
    }
    ui->spinBoxMaxSize->setValue(maxMB);
    // 保留文件数
    ui->spinBoxMaxFiles->setValue(cfg[ DA_CONFIG_KEY_LOG_MAX_FILES ].toInt());
    // 显示条数
    bool isOK   = false;
    int logNum  = cfg[ DA_CONFIG_KEY_SHOW_LOG_NUM ].toInt(&isOK);
    ui->spinBoxDisplayLogsNum->setValue((isOK && logNum >= 10) ? logNum : 5000);
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
