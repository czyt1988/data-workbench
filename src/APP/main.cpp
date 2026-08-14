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
#include <QSettings>
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
#include "DAPybind11InQt.h"
#include "DAPyInterpreter.h"
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
 * @brief 一次性数据迁移:从注册表(NativeFormat)搬到 INI 文件
 *
 * 用 NativeFormat 显式构造读注册表(绕过 setDefaultFormat),仅在 INI 文件不存在时迁移。
 * 旧注册表值保留不删(作为备份)。迁移 key 包括 agent LLM 配置(5 项)和最近文件列表。
 */
static void migrateSettingsFromRegistry()
{
    // 用 NativeFormat 显式构造读注册表(绕过全局 setDefaultFormat)
    QSettings reg(QSettings::NativeFormat, QSettings::UserScope, "DA", "DAWorkBench");
    QString configDir = DA::DADir::getConfigPath();

    // 迁移 agent 配置(5 个 key:base_url/model/api_key + ready/stop_timeout)
    QString agentIni = configDir + "/agent-config.ini";
    if (!QFile::exists(agentIni)
        && (reg.contains("agent/llm_base_url") || reg.contains("agent/llm_model")
            || reg.contains("agent/llm_api_key") || reg.contains("agent/ready_timeout_sec")
            || reg.contains("agent/stop_timeout_sec"))) {
        QSettings ini(agentIni, QSettings::IniFormat);
        if (reg.contains("agent/llm_base_url"))
            ini.setValue("agent/llm_base_url", reg.value("agent/llm_base_url"));
        if (reg.contains("agent/llm_model"))
            ini.setValue("agent/llm_model", reg.value("agent/llm_model"));
        // api_key 是 QByteArray(DPAPI 加密 blob),IniFormat 原生支持 QByteArray
        if (reg.contains("agent/llm_api_key")) {
            QByteArray encKey = reg.value("agent/llm_api_key").toByteArray();
            if (!encKey.isEmpty())
                ini.setValue("agent/llm_api_key", encKey);
        }
        if (reg.contains("agent/ready_timeout_sec"))
            ini.setValue("agent/ready_timeout_sec", reg.value("agent/ready_timeout_sec"));
        if (reg.contains("agent/stop_timeout_sec"))
            ini.setValue("agent/stop_timeout_sec", reg.value("agent/stop_timeout_sec"));
        ini.sync();
    }

    // 迁移 recent files
    QString rfIni = configDir + "/recent-files.ini";
    if (reg.contains("RecentFiles") && !QFile::exists(rfIni)) {
        QSettings ini(rfIni, QSettings::IniFormat);
        ini.setValue("RecentFiles", reg.value("RecentFiles"));
        ini.sync();
    }
}

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
        daWarning << QObject::tr("Failed to set console output codepage to UTF-8");  //cn:设置控制台输出代码页为 UTF-8 失败
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
    // 设置 organizationName:保留以兼容第三方库(如 SARibbon)内部 QSettings 两参数构造。
    // 原注册表路径(HKCU\Software\DA\DAWorkBench)的配置已迁移到 JSON 文件,
    // 详见下方 setDefaultFormat 全局重定向 + migrateSettingsFromRegistry()。
    QApplication::setOrganizationName("DA");

    // ── 全局 QSettings 重定向:NativeFormat → IniFormat,写入 AppData/config 目录 ──
    // 必须在所有 QSettings 对象创建之前调用;对默认构造、两参数构造(org,app)均生效。
    // 自有代码(Agent/RecentFiles)已用显式路径精确控制文件名,此全局设置作为安全网,
    // 覆盖第三方库(如 3rdparty DAWidgets 的 DARecentFilesManager 副本)的默认/两参数构造。
    // 注:JsonFormat 是 Qt6+ 特性,Qt5 仅支持 IniFormat;项目需兼容 Qt5,故用 IniFormat。
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, DA::DADir::getConfigPath());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // ── 一次性数据迁移:从注册表搬到 JSON 文件 ──
    migrateSettingsFromRegistry();
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
    cmd->setApplicationDescription(QCoreApplication::translate("main", "version:%1,compile datetime:%2")
                                       .arg(DA_VERSION)
                                       .arg(DA_COMPILE_DATETIME));
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
}
