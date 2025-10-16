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
#include "DAAppUtils.h"
#include "DAConfigs.h"
#include "DAAppCore.h"
#include "DAMessageHandler.h"
#include "DATranslatorManeger.h"
#include "DADumpCapture.h"
#include "DADir.h"

#include "DAAbstractArchiveTask.h"
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

const static QString CS_CMD_IMPORTDATA = QStringLiteral("import-data");

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
	DA::daRegisterRotatingMessageHandler(DA::DADir::getLogFilePath());
	// DA::daRegisterConsolMessageHandler();
	for (int i = 0; i < argc; ++i) {
        qDebug() << "argv[" << i << "]" << argv[ i ];
	}
	// 高清屏的适配
	enableHDPIScaling();
	// 打印程序默认路径
	qDebug() << DA::DADir();
	// 启动app
	QApplication app(argc, argv);
	QApplication::setApplicationVersion(DA_VERSION);
	QApplication::setApplicationName(DA_PROJECT_NAME);

    // 注册所有元对象

    DA::app_register_all_metatypes();
	// 安装翻译
	DA::DATranslatorManeger datr;
	datr.installAllTranslator();

	// 命令初始化
	QCommandLineParser cmdParser;
	initCommandLine(&cmdParser);
	// 解析命令行参数
	cmdParser.process(app);

	// 字体设置
	setAppFont();

	// 接口初始化
	DA::DAAppCore& core = DA::DAAppCore::getInstance();
	// 初始化python环境
	if (!core.initialized()) {
		qCritical() << QObject::tr("Kernel initialization failed");  // cn:内核初始化失败
		return -1;
	}

	// gui初始化
	DA::AppMainWindow w;
	QStringList positionalArgs = cmdParser.positionalArguments();
	qDebug() << "positionalArgs:" << positionalArgs;
	if (positionalArgs.size() == 1) {
		// 说明有可能是双击文件打开，这时候要看参数是否为一个工程文件
		QFileInfo openfi(positionalArgs[ 0 ]);
		if (openfi.exists()) {
			w.openProject(openfi.absoluteFilePath());
		}
	}
	// 处理其它命令
	if (cmdParser.isSet(CS_CMD_IMPORTDATA)) {
		// impot-data 命令
		const QStringList filePaths = cmdParser.values(CS_CMD_IMPORTDATA);
		for (const QString& path : filePaths) {
			w.importData(path, QVariantMap());
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
	cmd->setApplicationDescription(QCoreApplication::translate("main", "version:%1,compile datetime:%2,enable python:%3")
                                       .arg(DA_VERSION)
                                       .arg(DA_COMPILE_DATETIME)
                                       .arg(DA_ENABLE_PYTHON));
	cmd->addHelpOption();
	cmd->addVersionOption();
	cmd->addPositionalArgument("file",
                               QCoreApplication::translate("main", "The project file to open"),  // cn:要打开的工程文件
                               "[project]"  // 语法表示（可选）
	);
    QCommandLineOption
        importDataOption(CS_CMD_IMPORTDATA,
                         QCoreApplication::
                             translate("main",
                                       "Import data into the application, supporting formats such as CSV, XLSX, TXT, "
                                       "PKL, etc.If you want to import multiple datasets, you can use the command "
                                       "multiple times; the program will execute them one by one"),  // cn：导入数据到应用程序中，支持csv/xlsx/txt/pkl等格式，如果要导入多个数据，你可以使用多次命令，程序会逐一执行
                         "path");
	cmd->addOption(importDataOption);
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
	QString dumpFileDir = DA::DADir::getDumpFilePath();
	if (dumpFileDir.isEmpty()) {
		dumpFileDir = QDir::toNativeSeparators(QApplication::applicationDirPath() + "/dumps");
		QDir().mkpath(dumpFileDir);
	}

    QString baseName     = QDateTime::currentDateTime().toString("yyyyMMddhhmmss.zzz");
    QString dumpfileName = QString("dump%1.dmp").arg(baseName);
	return QDir::toNativeSeparators(dumpFileDir + "/" + dumpfileName);
}
