#include "DASettingPagePython.h"
#include "DAConfigs.h"
#if DA_ENABLE_PYTHON
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QSignalBlocker>
#include <QListWidgetItem>
#include "ui_DASettingPagePython.h"
#include "DAAppConfig.h"
#include "DAPyInterpreter.h"
#include "DALogCategory.h"
namespace DA
{

DASettingPagePython::DASettingPagePython(QWidget* parent)
    : DAAbstractSettingPage(parent)
    , ui(new Ui::DASettingPagePython)
{
    ui->setupUi(this);
    connect(ui->lineEditPythonPath, &QLineEdit::textChanged, this, &DASettingPagePython::onLineEditPythonPathTextChanged);
    connect(ui->toolButtonBrowse, &QToolButton::clicked, this, &DASettingPagePython::onToolButtonBrowseClicked);
    connect(ui->toolButtonAutoDetect, &QToolButton::clicked, this, &DASettingPagePython::onToolButtonAutoDetectClicked);
    connect(ui->toolButtonTest, &QToolButton::clicked, this, &DASettingPagePython::onToolButtonTestClicked);
    connect(ui->toolButtonAddPath, &QToolButton::clicked, this, &DASettingPagePython::onToolButtonAddPathClicked);
    connect(ui->toolButtonRemovePath, &QToolButton::clicked, this, &DASettingPagePython::onToolButtonRemovePathClicked);
}

DASettingPagePython::~DASettingPagePython()
{
    delete ui;
}

QString DASettingPagePython::readPythonConfigJson() const
{
    QString cfgFile = DA::DAPyInterpreter::getAppPythonConfigFile();
    QFile f(cfgFile);
    if (!f.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    return doc.object().value("config").toObject().value("interpreter").toString();
}

void DASettingPagePython::writePythonConfigJson(const QString& interpreterPath)
{
    QString cfgFile = DA::DAPyInterpreter::getAppPythonConfigFile();
    QJsonObject root;
    QJsonObject config;
    config[ "interpreter" ] = interpreterPath;
    root[ "config" ] = config;
    QJsonDocument doc(root);
    QFile f(cfgFile);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(doc.toJson());
        f.close();
    } else {
        daWarning << tr("Cannot write python config file: %1").arg(cfgFile);  // cn:无法写入Python配置文件：%1
    }
}

void DASettingPagePython::apply()
{
    if (nullptr == mAppConfig) {
        return;
    }
    DAAppConfig& cfg    = *mAppConfig;
    QString path        = ui->lineEditPythonPath->text().trimmed();
    cfg[ DA_CONFIG_KEY_PYTHON_INTERPRETER_PATH ] = path;
    // 额外模块路径
    QStringList extraPaths;
    for (int i = 0; i < ui->listWidgetExtraPaths->count(); ++i) {
        QListWidgetItem* it = ui->listWidgetExtraPaths->item(i);
        if (it) {
            extraPaths << it->text().trimmed();
        }
    }
    cfg[ DA_CONFIG_KEY_PYTHON_EXTRA_PATHS ] = extraPaths;
    // 同步到 python-config.json，下次启动由 DAPyInterpreter 读取
    writePythonConfigJson(path);
    cfg.apply();
    emit settingApplyed();
}

QString DASettingPagePython::getSettingPageTitle() const
{
    return tr("Python");  // cn:Python
}

QIcon DASettingPagePython::getSettingPageIcon() const
{
    return QIcon(":/DAGui/icon/setting-python.svg");
}

bool DASettingPagePython::setAppConfig(DAAppConfig* p)
{
    if (nullptr == p) {
        return false;
    }
    QSignalBlocker blocker(this);
    mAppConfig = p;
    DAAppConfig& cfg = *p;
    // 解释器路径：优先配置键，其次 python-config.json，最后自动检测
    QString path = cfg[ DA_CONFIG_KEY_PYTHON_INTERPRETER_PATH ].toString();
    if (path.isEmpty()) {
        path = readPythonConfigJson();
    }
    if (path.isEmpty()) {
        path = DA::DAPyInterpreter::getPythonInterpreterPath();
    }
    ui->lineEditPythonPath->setText(path);
    // 额外模块路径
    QStringList extraPaths = cfg[ DA_CONFIG_KEY_PYTHON_EXTRA_PATHS ].toStringList();
    ui->listWidgetExtraPaths->clear();
    for (const QString& s : extraPaths) {
        if (!s.trimmed().isEmpty()) {
            ui->listWidgetExtraPaths->addItem(s);
        }
    }
    return true;
}

void DASettingPagePython::onLineEditPythonPathTextChanged(const QString& text)
{
    Q_UNUSED(text);
    emit settingChanged();
}

void DASettingPagePython::onToolButtonBrowseClicked()
{
    QString f = QFileDialog::getOpenFileName(this, tr("Select Python Interpreter"));  // cn:选择Python解释器
    if (!f.isEmpty()) {
        ui->lineEditPythonPath->setText(f);
        emit settingChanged();
    }
}

void DASettingPagePython::onToolButtonAutoDetectClicked()
{
    QList< QFileInfo > list = DA::DAPyInterpreter::wherePython();
    if (list.isEmpty()) {
        QMessageBox::information(this,
                                 tr("Information"),  // cn:信息
                                 tr("No Python interpreter found in system PATH"));  // cn:系统PATH中未找到Python解释器
        return;
    }
    ui->lineEditPythonPath->setText(list.first().absoluteFilePath());
    emit settingChanged();
}

void DASettingPagePython::onToolButtonTestClicked()
{
    QString path = ui->lineEditPythonPath->text().trimmed();
    if (path.isEmpty()) {
        ui->labelTestResult->setText(tr("Please specify a Python interpreter path"));  // cn:请指定Python解释器路径
        return;
    }
    if (!QFileInfo::exists(path)) {
        ui->labelTestResult->setText(tr("File does not exist: %1").arg(path));  // cn:文件不存在：%1
        return;
    }
    QProcess proc(this);
    proc.start(path, QStringList() << "--version");
    if (!proc.waitForFinished(5000)) {
        ui->labelTestResult->setText(tr("Failed to run: %1").arg(proc.errorString()));  // cn:运行失败：%1
        return;
    }
    QString out = QString::fromLocal8Bit(proc.readAllStandardOutput() + proc.readAllStandardError()).trimmed();
    ui->labelTestResult->setText(out.isEmpty() ? tr("OK") : out);  // cn:正常
}

void DASettingPagePython::onToolButtonAddPathClicked()
{
    QString d = QFileDialog::getExistingDirectory(this, tr("Select module search path"));  // cn:选择模块搜索路径
    if (!d.isEmpty()) {
        ui->listWidgetExtraPaths->addItem(d);
        emit settingChanged();
    }
}

void DASettingPagePython::onToolButtonRemovePathClicked()
{
    int row = ui->listWidgetExtraPaths->currentRow();
    if (row >= 0) {
        delete ui->listWidgetExtraPaths->takeItem(row);
        emit settingChanged();
    }
}

}  // end DA
#endif  // DA_ENABLE_PYTHON
