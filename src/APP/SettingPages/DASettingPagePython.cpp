#include "DASettingPagePython.h"
#include "DAConfigs.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QSignalBlocker>
#include "ui_DASettingPagePython.h"
#include "DAAppConfig.h"
#include "DAFilePathEditWidget.h"
#include "DAPyInterpreter.h"
#include "DAStringListEditWidget.h"
#include "DALogCategory.h"
namespace DA
{

DASettingPagePython::DASettingPagePython(QWidget* parent)
    : DAAbstractSettingPage(parent)
    , ui(new Ui::DASettingPagePython)
{
    ui->setupUi(this);
    ui->filePathEditPythonPath->setMode(DAFilePathEditWidget::Mode::OpenFile);
    ui->stringListEditExtraPaths->setAddMode(DAStringListEditWidget::AddMode::External);
    ui->stringListEditExtraPaths->setPlaceholderText(tr("Add module search paths..."));  // cn:添加模块搜索路径...
    connect(ui->filePathEditPythonPath,
            &DAFilePathEditWidget::pathChanged,
            this,
            &DASettingPagePython::onInterpreterPathChanged);
    connect(ui->toolButtonAutoDetect, &QToolButton::clicked, this, &DASettingPagePython::onToolButtonAutoDetectClicked);
    connect(ui->toolButtonTest, &QToolButton::clicked, this, &DASettingPagePython::onToolButtonTestClicked);
    connect(ui->stringListEditExtraPaths,
            &DAStringListEditWidget::addRequested,
            this,
            &DASettingPagePython::onExtraPathAddRequested);
    connect(ui->stringListEditExtraPaths,
            &DAStringListEditWidget::itemsChanged,
            this,
            &DASettingPagePython::settingChanged);
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
    DAPythonSettings s = mAppConfig->python();
    s.interpreterPath  = ui->filePathEditPythonPath->getFilePath().trimmed();
    // 额外模块路径（空串由控件保留，保存时过滤）
    QStringList extraPaths;
    const QStringList rawPaths = ui->stringListEditExtraPaths->items();
    for (const QString& p : rawPaths) {
        const QString trimmed = p.trimmed();
        if (!trimmed.isEmpty()) {
            extraPaths << trimmed;
        }
    }
    s.extraPaths = extraPaths;
    // setPython 会发出 sectionChanged；解释器路径需重启生效
    mAppConfig->setPython(s);
    // 同步到 python-config.json，下次启动由 DAPyInterpreter 读取
    writePythonConfigJson(s.interpreterPath);
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
    // 解释器路径：优先配置，其次 python-config.json，最后自动检测
    QString path = p->python().interpreterPath;
    if (path.isEmpty()) {
        path = readPythonConfigJson();
    }
    if (path.isEmpty()) {
        path = DA::DAPyInterpreter::getPythonInterpreterPath();
    }
    ui->filePathEditPythonPath->setFilePath(path);
    // 额外模块路径
    QStringList extraPaths;
    for (const QString& s : p->python().extraPaths) {
        if (!s.trimmed().isEmpty()) {
            extraPaths << s;
        }
    }
    ui->stringListEditExtraPaths->setItems(extraPaths);
    return true;
}

void DASettingPagePython::onInterpreterPathChanged(const QString& path)
{
    Q_UNUSED(path);
    emit settingChanged();
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
    ui->filePathEditPythonPath->setFilePath(list.first().absoluteFilePath());
    emit settingChanged();
}

void DASettingPagePython::onToolButtonTestClicked()
{
    QString path = ui->filePathEditPythonPath->getFilePath().trimmed();
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

void DASettingPagePython::onExtraPathAddRequested()
{
    QString d = QFileDialog::getExistingDirectory(this, tr("Select module search path"));  // cn:选择模块搜索路径
    if (!d.isEmpty()) {
        ui->stringListEditExtraPaths->appendItem(d);
    }
}

}  // end DA
