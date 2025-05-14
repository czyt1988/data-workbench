#include "AppMainWindow.h"
// stl
#include <iostream>
// windows system only
#ifdef Q_OS_WIN
#include <windows.h>
#endif
// Qt
#include <QCommandLineParser>
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
#include "DADir.h"
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
static QString daVersionInfo();
//--describe的返回内容，返回详细信息
static QString daDescribe();
//--help的返回内容，返回帮助
static QString daHelp();
// 解析参数，如果返回false，则直接return 0,不会进入gui
bool parsingConsoleArguments(const QStringList& args);
// 初始化所有命令
void initCommandLine(QCommandLineParser* cmd);
/**
 * @brief main
 * @param argc
 * @param argv
 * 参数：--version 返回版本信息
 * 参数：--describe 返回详细信息
 * 参数：--help 返回帮助信息
 * 参数：文件地址 直接打开文件
 * @return
 */
int main(int argc, char* argv[])
{
	// 进行dump捕获
	DA::DADumpCapture::initDump([]() -> QString { return appPreposeDump(); });
	//

	// 注册旋转文件消息捕获
	DA::daRegisterRotatingMessageHandler(DA::DADir::getFullLogFilePath());
	// DA::daRegisterConsolMessageHandler();
	for (int i = 0; i < argc; ++i) {
		qDebug() << "argv[" << i << "]" << argv[ i ] << endl;
	}
	// 高清屏的适配
	enableHDPIScaling();
	// 打印程序默认路径
	qDebug() << DA::DADir();
	// 启动app
	QApplication app(argc, argv);
	QApplication::setApplicationVersion(daVersionInfo());
	QApplication::setApplicationName("DAWorkbench");
	QCommandLineParser parser;
	parser.setApplicationDescription(daDescribe());

	// 安装翻译
	DA::DATranslatorManeger datr;
	datr.installAllTranslator();

	// 解析命令行参数
	QStringList appArguments = app.arguments();
	if (!parsingConsoleArguments(appArguments)) {
		// 刷新再退出
		std::cout.flush();
		return 0;
	}
	setAppFont();
	DA::DAAppCore& core = DA::DAAppCore::getInstance();
	// 初始化python环境
	if (!core.initialized()) {
		qCritical() << QObject::tr("Kernel initialization failed");  // cn:内核初始化失败
		return -1;
	}

	// TODO 此处进行一些核心的初始化操作
	DA::AppMainWindow w;
	if (appArguments.size() > 1) {
		// 说明有可能是双击文件打开，这时候要看参数2是否为一个工程文件
		QFileInfo openfi(appArguments[ 1 ]);
		if (openfi.exists()) {
			w.openProject(openfi.absoluteFilePath());
		}
	}
	w.show();
	int r = app.exec();
	DA::daUnregisterMessageHandler();
	return r;
}

/**
 * @brief 初始化所有的命令
 * @param cmd
 */
void initCommandLine(QCommandLineParser* cmd)
{
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
	QString descibe =
		QString("version:%1,compile datetime:%2,enable python:%3").arg(DA_VERSION).arg(DA_COMPILE_DATETIME).arg(DA_ENABLE_PYTHON);
	return descibe;
}

/**
 * @brief 解析参数，如果返回false，则直接return 0,不会进入gui
 * @param args
 * @return
 */
bool parsingConsoleArguments(const QStringList& args)
{
	if (args.contains("--help") || args.contains("--version") || args.contains("--describe")) {
#ifdef Q_OS_WIN
		bool hasConsole = AttachConsole(ATTACH_PARENT_PROCESS);
		if (hasConsole) {
			// 重定向标准输出
			FILE* newStdout = nullptr;
			if (freopen_s(&newStdout, "CONOUT$", "w", stdout) == 0) {
				// 使用文件描述符重定向
				int fd = _fileno(newStdout);
				if (fd != -1) {
					_dup2(fd, _fileno(stdout));           // 将新流的描述符复制到标准输出
					setvbuf(stdout, nullptr, _IONBF, 0);  // 禁用缓冲
				}
			}
			// 重定向标准错误输出
			FILE* newStderr = nullptr;
			if (freopen_s(&newStderr, "CONOUT$", "w", stderr) == 0) {
				int fdErr = _fileno(newStderr);
				if (fdErr != -1) {
					_dup2(fdErr, _fileno(stderr));
					setvbuf(stderr, nullptr, _IONBF, 0);
				}
			}
		}
#endif
		QTextStream st(stdout);
		if (args.contains("--help")) {
			st << daHelp() << Qt::endl;
		} else if (args.contains("--version")) {
			st << daVersionInfo() << Qt::endl;
		} else if (args.contains("--describe")) {
			st << daDescribe() << Qt::endl;
		}
		st.flush();

#ifdef Q_OS_WIN
		if (hasConsole) {
			FreeConsole();
		}
#endif
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
#ifdef Q_OS_WIN
	QFont font = QApplication::font();
	font.setFamily(QStringLiteral(u"微软雅黑"));
	QApplication::setFont(font);
#endif
}

/**
 * @brief dump前处理，设置dump文件名，同时生成一个系统信息记录
 *
 * 这里会生成一个dumpxxx.sysinfo的文件，记录了da的必要信息
 * @return
 */
QString appPreposeDump()
{
	QString dumpFileDir = DA::DADir::getAppDataPath();
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
