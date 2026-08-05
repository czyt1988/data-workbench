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
#include "DALogCategory.h"
#include <QLocale>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
// DA
#include "DAAppUtils.h"
#include "DAConfigs.h"
#include "DAAppCore.h"
#include "DALogger.h"
#include "DATranslatorManeger.h"
#include "DADumpCapture.h"
#include "DADir.h"
#include "DACoreInterface.h"
#include "DAAbstractArchiveTask.h"
#include "DASplashScreen.h"
#include "SettingPages/DAAppConfig.h"
#if DA_ENABLE_PYTHON
#include "DAPybind11InQt.h"
#include "DAPyInterpreter.h"
#endif
// SARibbon
#include "SARibbonBar.h"

void setAppFont(const DA::DAAppConfig& cfg);
QString appPreposeDump();
void enableHDPIScaling();
void initializePythonInterpreter(const DA::DAAppConfig& cfg);
void setupLogger(const DA::DAAppConfig& cfg);

const static QString CS_CMD_IMPORTDATA = QStringLiteral("import-data");
const static QString CS_CMD_NOSPLASH   = QStringLiteral("no-splash");
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
#ifdef Q_OS_WIN
    // 设置控制台输出代码页为 UTF-8 (65001)
    if (!SetConsoleOutputCP(CP_UTF8)) {
        daWarning << "Failed to set console output codepage to UTF-8";
    }
#endif
    // 进行dump捕获
    DA::DADumpCapture::initDump([]() -> QString { return appPreposeDump(); });

    // 早期加载应用配置，供日志/Python/字体/翻译/启动画面等早期初始化使用。
    // 这些项的变更需重启程序生效（配置由设置对话框写入，下次启动在此读取）。
    DA::DAAppConfig earlyConfig;
    earlyConfig.loadConfig();

    // 注册旋转文件消息捕获（依据配置：轮转模式/大小/数量/stdout/级别）
    setupLogger(earlyConfig);
    for (int i = 0; i < argc; ++i) {
        daDebug << "argv[" << i << "]" << argv[ i ];
    }
    // 打印程序默认路径
    daDebug << DA::DADir();
    // 初始化python环境,不启用python直接返回
    initializePythonInterpreter(earlyConfig);
    // 高清屏的适配
    enableHDPIScaling();
    // 启动app
    QApplication app(argc, argv);
    QApplication::setApplicationVersion(DA_VERSION);
    QApplication::setApplicationName(DA_PROJECT_NAME);
    // 必须设置 organizationName：QSettings 默认构造用 organizationName()+applicationName()
    // 作为注册表路径(HKCU\Software\<org>\<app>)，org 为空时 setValue 静默失败，
    // 表现为 DAAgentSettingsWidget::saveConfig 代码执行但配置重启后丢失。
    // "DA" 与 DAAppActions.cpp 中 DARecentFilesManager(this,10,"DA","DAWorkBench") 保持一致，
    // 确保所有 QSettings 用户写到同一路径 HKCU\Software\DA\DAWorkBench。
    QApplication::setOrganizationName("DA");
    // 命令初始化
    QCommandLineParser cmdParser;
    initCommandLine(&cmdParser);
    // 解析命令行参数
    cmdParser.process(app);
    // 字体设置（依据配置：字体族/字号）
    setAppFont(earlyConfig);

    //  安装翻译（依据配置：语言代码，空表示跟随系统）
    DA::DATranslatorManeger datr;
    QString langCode = earlyConfig.value(DA_CONFIG_KEY_LANGUAGE).toString();
    if (langCode.isEmpty()) {
        datr.installAllTranslator();
    } else {
        datr.setLocale(QLocale(langCode));
        datr.installAllTranslator(langCode);
    }

    // 创建并显示启动画面(启动画面必须在QApplication之后创建)
    // --no-splash 参数可跳过启动画面，适用于调试场景；同时受配置项 show-splash 控制
    bool showSplash = earlyConfig.value(DA_CONFIG_KEY_SHOW_SPLASH).toBool();
    showSplash      = showSplash && !cmdParser.isSet(CS_CMD_NOSPLASH);
    DA::DASplashScreen* splash = nullptr;
    if (showSplash) {
        splash = new DA::DASplashScreen();
        // 支持从外部文件加载自定义背景图片
        QString customSplashPath = QApplication::applicationDirPath() + QStringLiteral("/splash.png");
        splash->loadBackgroundPixmap(customSplashPath);  // 若文件不存在则保持默认背景
        splash->show();
        splash->showMessage(QObject::tr("Initializing..."));  // cn:正在初始化...
    }

    // 接口初始化，同步会初始化python环境
    if (splash) {
        splash->showMessage(QObject::tr("Initializing core components..."));  // cn:正在初始化核心组件...
    }
    DA::DAAppCore& core = DA::DAAppCore::getInstance();
    if (!core.initialized()) {
        daCritical << QObject::tr("Kernel initialization failed");  // cn:内核初始化失败
        if (splash) {
            delete splash;
        }
        return -1;
    }

    // gui初始化
    if (splash) {
        splash->showMessage(QObject::tr("Loading user interface..."));  // cn:正在加载用户界面...
    }
    DA::AppMainWindow w;
    QStringList positionalArgs = cmdParser.positionalArguments();
    daDebug << "positionalArgs:" << positionalArgs;
    if (positionalArgs.size() == 1) {
        // 说明有可能是双击文件打开，这时候要看参数是否为一个工程文件
        QFileInfo openfi(positionalArgs[ 0 ]);
        if (openfi.exists()) {
            if (splash) {
                splash->showMessage(QObject::tr("Opening project..."));  // cn:正在打开工程...
            }
            w.openProject(openfi.absoluteFilePath());
        }
    }
    // 处理其它命令
    if (cmdParser.isSet(CS_CMD_IMPORTDATA)) {
        if (splash) {
            splash->showMessage(QObject::tr("Importing data..."));  // cn:正在导入数据...
        }
        // impot-data 命令
        const QStringList filePaths = cmdParser.values(CS_CMD_IMPORTDATA);
        for (const QString& path : filePaths) {
            w.importData(path, QVariantMap());
        }
    }
    if (splash) {
        splash->showMessage(QObject::tr("Ready"));  // cn:启动完成
    }
    w.show();
    if (splash) {
        splash->finish(&w);
        delete splash;
    }
    int r = app.exec();
    // DALogger 单例析构时自动注销 message handler 和 spdlog
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
                               "[project]"                                                       // 语法表示（可选）
    );
    QCommandLineOption importDataOption(
        CS_CMD_IMPORTDATA,
        QCoreApplication::translate(
            "main",
            "Import data into the application, supporting formats such as CSV, XLSX, TXT, "
            "PKL, etc.If you want to import multiple datasets, you can use the command "
            "multiple times; the program will execute them one by one"),  // cn:导入数据到应用程序中，支持csv/xlsx/txt/pkl等格式，如果要导入多个数据，你可以使用多次命令，程序会逐一执行
        "path");
    cmd->addOption(importDataOption);
    QCommandLineOption noSplashOption(
        CS_CMD_NOSPLASH,
        QCoreApplication::translate("main",
                                    "Disable the splash screen during startup, useful for debugging to avoid "
                                    "the splash window blocking the IDE")  // cn:禁用启动画面，适用于调试时避免启动窗口遮挡IDE
    );
    cmd->addOption(noSplashOption);
}

/**
 * @brief 开启高dpi适配
 */
void enableHDPIScaling()
{
    SARibbonBar::initHighDpi();
}

/**
 * @brief 根据配置设置应用字体
 *
 * 配置项为空/<=0 时回退到系统默认（Windows 下回退到微软雅黑）。
 * @param cfg 早期加载的应用配置
 */
void setAppFont(const DA::DAAppConfig& cfg)
{
    QFont font = QApplication::font();
    QString family = cfg.value(DA_CONFIG_KEY_APP_FONT_FAMILY).toString();
    double pointSize = cfg.value(DA_CONFIG_KEY_APP_FONT_POINT_SIZE).toDouble();
    if (!family.isEmpty()) {
        font.setFamily(family);
    } else {
#ifdef Q_OS_WIN
        font.setFamily(QStringLiteral(u"微软雅黑"));
#endif
    }
    if (pointSize > 0) {
        font.setPointSizeF(pointSize);
    }
    QApplication::setFont(font);
}

/**
 * @brief 根据配置初始化日志系统
 *
 * 依据轮转模式选择 rotating/daily/console，并设置日志级别与 UI 队列级别。
 * @param cfg 早期加载的应用配置
 */
void setupLogger(const DA::DAAppConfig& cfg)
{
    QString logPath   = DA::DADir::getLogFilePath();
    int mode          = cfg.value(DA_CONFIG_KEY_LOG_ROTATION_MODE).toInt();
    int maxSize       = cfg.value(DA_CONFIG_KEY_LOG_MAX_SIZE).toInt();
    int maxFiles      = cfg.value(DA_CONFIG_KEY_LOG_MAX_FILES).toInt();
    bool outputStdout = cfg.value(DA_CONFIG_KEY_LOG_OUTPUT_STDOUT).toBool();
    if (maxSize <= 0) {
        maxSize = 10 * 1024 * 1024;
    }
    if (maxFiles <= 0) {
        maxFiles = 5;
    }
    switch (mode) {
    case 1:  // daily
        DA::DALogger::instance().setupDailyFile(logPath, maxFiles, outputStdout);
        break;
    case 2:  // console only
        DA::DALogger::instance().setupConsole();
        break;
    default:  // rotating
        DA::DALogger::instance().setupRotatingFile(logPath, maxSize, maxFiles, outputStdout);
        break;
    }
    DA::DALogger::instance().setLevel(static_cast< DA::DALogLevel >(cfg.value(DA_CONFIG_KEY_LOG_LEVEL).toInt()));
    DA::DALogger::instance().setQueueLevel(static_cast< DA::DALogLevel >(cfg.value(DA_CONFIG_KEY_LOG_QUEUE_LEVEL).toInt()));
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

/**
 * @brief 根据配置初始化 Python 解释器
 *
 * 解释器路径由 DAPyInterpreter::getPythonInterpreterPath() 解析（优先 python-config.json）。
 * 初始化后追加用户配置的额外模块搜索路径到 sys.path。
 * @param cfg 早期加载的应用配置
 */
void initializePythonInterpreter(const DA::DAAppConfig& cfg)
{
#if DA_ENABLE_PYTHON
    QString pythonHomePath;
    QString pypath = DA::DAPyInterpreter::getPythonInterpreterPath();
    if (!pypath.isEmpty()) {
        daInfo << QObject::tr("Python interpreter path is %1").arg(pypath);  // cn:Python解释器路径为%1
        QFileInfo fi(pypath);
        pythonHomePath = fi.absolutePath();
        daInfo << QObject::tr("Python home path is %1").arg(pythonHomePath);  // cn:Python主目录路径为%1
    }
    DA::DAPyInterpreter::initializePythonInterpreter(pythonHomePath);
    // 追加用户配置的额外模块搜索路径
    QStringList extraPaths = cfg.value(DA_CONFIG_KEY_PYTHON_EXTRA_PATHS).toStringList();
    for (const QString& p : extraPaths) {
        if (!p.trimmed().isEmpty()) {
            DA::DAPyInterpreter::appendSysPath(p);
        }
    }
#else
    Q_UNUSED(cfg)
#endif
}
