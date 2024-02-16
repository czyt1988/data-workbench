#include "AppMainWindow.h"
// stl
#include <iostream>
// Qt
#include <QProcess>
#include <QObject>
#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
// DA
#include "DAConfigs.h"
#include "DAAppCore.h"
#include "DAMessageHandler.h"
#include "DATranslatorManeger.h"
#include "DADumpCapture.h"
#if DA_ENABLE_PYTHON
#include "DAPybind11InQt.h"
#include "DAPyScripts.h"
#include "DAPyInterpreter.h"
#include "DAPybind11QtTypeCast.h"
#endif
// SARibbon
#include "SARibbonBar.h"

void setAppFont();
QString appPreposeDump();
void enableHDPIScaling();
//--version的返回内容，返回版本信息
QString daVersionInfo();
//--describe的返回内容，返回详细信息
QString daDescribe();
//--help的返回内容，返回帮助
QString daHelp();
// 解析参数，如果返回false，则直接return 0,不会进入gui
bool parsingArguments(const QStringList& args);

/**
 * @brief main
 * @param argc
 * @param argv
 * 参数：--version 返回版本信息
 * 参数：--describe 返回详细信息
 * 参数：--help 返回帮助信息
 * @return
 */
int main(int argc, char* argv[])
{
    // 程序路径捕获
    QFileInfo fi(argv[ 0 ]);
    // 进行dump捕获
    DA::DADumpCapture::initDump([]() -> QString { return appPreposeDump(); });
    //
    QString logfilePath = QDir::toNativeSeparators(fi.absolutePath() + "/log/da_log.log");
    // 注册旋转文件消息捕获
    DA::daRegisterRotatingMessageHandler(logfilePath);
    // DA::daRegisterConsolMessageHandler();
    for (int i = 0; i < argc; ++i) {
        qDebug() << "argv[" << i << "]" << argv[ i ];
    }
    // 高清屏的适配
    enableHDPIScaling();

    // 启动app
    QApplication app(argc, argv);

    // 安装翻译
    DA::DATranslatorManeger datr;
    datr.installAllTranslator();

    // 解析命令行参数
    QStringList appArguments = app.arguments();
    if (!parsingArguments(appArguments)) {
        return 0;
    }
#if DA_ENABLE_PYTHON
    // 注册元对象
    DA::PY::registerMetaType();
#endif
    setAppFont();
    DA::DAAppCore& core = DA::DAAppCore::getInstance();
    // 初始化python环境
    if (!core.initialized()) {
        qCritical() << QObject::tr("Kernel initialization failed");  // cn:内核初始化失败
        return -1;
    }

    // TODO 此处进行一些核心的初始化操作
    DA::AppMainWindow w;

    w.show();
    int r = app.exec();
    DA::daUnregisterMessageHandler();
    return r;
}

/**
 * @brief 帮助，--help的返回内容
 * @return
 */
QString daHelp()
{
    QString str = QObject::tr(""
                              "%1(%2) build %3\n"  // DAWorkbench(0.0.2) build 240215
                              "params:\n"
                              "--version : version information\n"    //--version :版本信息
                              "--describe : detailed information\n"  //--describe :详细信息
                              )
                      .arg(DA_PROJECT_NAME)
                      .arg(DA_VERSION)
                      .arg(DA_COMPILE_DATETIME);
    return str;
}

/**
 * @brief 版本信息，--version的返回内容
 * @return
 */
QString daVersionInfo()
{
    return DA_VERSION;
}

/**
 * @brief 描述信息，--describe的返回内容
 * @return
 */
QString daDescribe()
{
    QString descibe = QString("version:%1,compile datetime:%2,enable python:%3")
                          .arg(DA_VERSION)
                          .arg(DA_COMPILE_DATETIME)
                          .arg(DA_ENABLE_PYTHON);
    return descibe;
}

/**
 * @brief 解析参数，如果返回false，则直接return 0,不会进入gui
 * @param args
 * @return
 */
bool parsingArguments(const QStringList& args)
{
    if (args.contains("--help")) {
        QTextStream st(stdout);
        st << daHelp() << Qt::endl;
        return false;
    } else if (args.contains("--version")) {
        QTextStream st(stdout);
        st << daVersionInfo() << Qt::endl;
        return false;
    } else if (args.contains("--describe")) {
        QTextStream st(stdout);
        st << daDescribe() << Qt::endl;
        return false;
    }
    return true;
}

/**
 * @brief 开启高dpi适配
 */
void enableHDPIScaling()
{
    SARibbonBar::initHighDpi();
}

/**
 * @brief 设置字体
 */
void setAppFont()
{
    QFont font = QApplication::font();
    font.setFamily(QStringLiteral(u"微软雅黑"));
    QApplication::setFont(font);
}

/**
 * @brief dump前处理，设置dump文件名，同时生成一个系统信息记录
 *
 * 这里会生成一个dumpxxx.sysinfo的文件，记录了da的必要信息
 * @return
 */
QString appPreposeDump()
{
    QString dumpFileDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dumpFileDir.isEmpty()) {
        dumpFileDir = QApplication::applicationDirPath();
    }
    dumpFileDir = QDir::toNativeSeparators(dumpFileDir + "/dumps");
    QDir dir;
    // 如果无法创建路径，将qcritual
    // mkpath回保证能有此路径，否则会返回false
    if (!dir.mkpath(dumpFileDir)) {
        qCritical() << QObject::tr("Unable to create dump file path:%1");  // cn:无法创建音乐配置路径：%1
    }
    QString baseName           = QDateTime::currentDateTime().toString("yyyyMMddhhmmss.zzz");
    QString dumpfileName       = QString("dump%1.dmp").arg(baseName);
    QString systemInfofileName = QString("dump%1.sysinfo").arg(baseName);
    // 生成sysinfo
    QFile sysfi(QDir::toNativeSeparators(dumpFileDir + "/" + systemInfofileName));
    if (sysfi.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream st(&sysfi);
        QSysInfo s;
        // 先写入da的信息
        st << daDescribe() << "\r\n" << s << Qt::endl;
    }
    sysfi.close();
    return QDir::toNativeSeparators(dumpFileDir + "/" + dumpfileName);
}
