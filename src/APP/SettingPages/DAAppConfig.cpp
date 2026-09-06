#include "DAAppConfig.h"
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QApplication>
#include <QObject>
#include "SARibbonBar.h"
#include "SARibbonMainWindow.h"
#include "AppMainWindow.h"
#include "DAAppCore.h"
#include "DAAppUI.h"
#include "DAAppActions.h"
#include "DARecentFilesManager.h"
#include "DAMessageLogQueue.h"
#include "DALogger.h"
#include "DALogCategory.h"
#include "DAAbstractSettingPage.h"
#include "DADumpCapture.h"
#include "DAPyScriptRunner.h"
#include "DAAppDockingArea.h"
#include "DAPyWorkFlowOperateWidget.h"
#include "Chart/DAChartOperateWidget.h"
#include "DADataOperateWidget.h"
namespace DA
{

DAAppConfig::DAAppConfig()
{
    mVersion        = QVersionNumber(0, 0, 3);
    mConfigFilePath = getAbsoluteConfigFilePath();
    // 先设置默认参数，这些默认参数后续如果配置文件中有会被替换掉
    insert(DA_CONFIG_KEY_RIBBON_STYLE, static_cast< int >(SARibbonBar::RibbonStyleCompactTwoRow));
    insert(DA_CONFIG_KEY_SHOW_LOG_NUM, 5000);             // 5000条日志
    insert(DA_CONFIG_KEY_SAVE_UI_STATE_ON_CLOSE, false);  // 程序在退出时是否保存ui的状态
    // 通用/UI
    insert(DA_CONFIG_KEY_LANGUAGE, QString());                        // 空=跟随系统
    insert(DA_CONFIG_KEY_RIBBON_THEME, -1);                           // -1=不覆盖框架默认主题
    insert(DA_CONFIG_KEY_APP_FONT_FAMILY, QString());                 // 空=系统默认字体
    insert(DA_CONFIG_KEY_APP_FONT_POINT_SIZE, -1.0);                  // <=0=系统默认字号
    insert(DA_CONFIG_KEY_DOCK_TAB_POSITION, QStringLiteral("bottom"));  // 操作窗口内部标签页默认在下方
    // Python
    insert(DA_CONFIG_KEY_PYTHON_INTERPRETER_PATH, QString());         // 空=自动检测
    insert(DA_CONFIG_KEY_PYTHON_EXTRA_PATHS, QStringList());          // 额外 sys.path
    // 日志
    insert(DA_CONFIG_KEY_LOG_LEVEL, static_cast< int >(DA::DALogLevel::Trace));
    insert(DA_CONFIG_KEY_LOG_QUEUE_LEVEL, static_cast< int >(DA::DALogLevel::Info));
    insert(DA_CONFIG_KEY_LOG_OUTPUT_STDOUT, true);
    insert(DA_CONFIG_KEY_LOG_ROTATION_MODE, 0);                       // 0=rotating
    insert(DA_CONFIG_KEY_LOG_MAX_SIZE, 10 * 1024 * 1024);             // 10MB
    insert(DA_CONFIG_KEY_LOG_MAX_FILES, 5);
    // 工作流/数据/高级
    insert(DA_CONFIG_KEY_WORKFLOW_TIMEOUT, -1.0);                     // <0=无限等待
    insert(DA_CONFIG_KEY_RECENT_FILES_MAX, 10);
    insert(DA_CONFIG_KEY_DUMP_RETENTION_DAYS, 7);
    insert(DA_CONFIG_KEY_NODE_SCRIPT_PATHS, QStringList());
    insert(DA_CONFIG_KEY_PLUGIN_EXTRA_PATHS, QStringList());
    insert(DA_CONFIG_KEY_SHOW_SPLASH, true);
    insert(DA_CONFIG_KEY_AUTOSAVE_INTERVAL, 0);                      // 0=禁用
    // 脚本工作区/脚本执行引擎
    insert(DA_CONFIG_KEY_WORKSPACE_DIR, QString());                  // 空=用系统临时目录
    insert(DA_CONFIG_KEY_SCRIPT_TIMEOUT, 300);                       // 默认 300 秒，0=禁用（显式选择）
    insert(DA_CONFIG_KEY_SCRIPT_RESULT_MAX_CHARS, 10000);            // 默认 10000 字符
    insert(DA_CONFIG_KEY_WORKSPACE_OVERWRITE_POLICY, QStringLiteral("ask"));
}

DAAppConfig::~DAAppConfig()
{
}

void DAAppConfig::setCore(DAAppCore* core)
{
    mCore       = core;
    mUI         = core->getAppUi();
    mMainWindow = qobject_cast< AppMainWindow* >(mUI->getMainWindow());
}

bool DAAppConfig::loadConfig(bool noFileCreateNewOne)
{
    QFile xmlConfigFile(mConfigFilePath);
    if (!xmlConfigFile.exists()) {
        // 没有配置文件
        if (noFileCreateNewOne) {
            return saveConfig();
        }
        return false;
    }
    if (!xmlConfigFile.open(QIODevice::ReadWrite)) {
        // 有配置文件，但打开失败
        daWarning.noquote() << QObject::tr("Cannot open config file \"%1\": %2")  // cn:无法打开配置文件\"%1\"，原因是%2
                                   .arg(mConfigFilePath, xmlConfigFile.errorString());
        return false;
    }

    // 文件存在了才加载文件
    QString err;
    QDomDocument doc;
    if (!doc.setContent(&xmlConfigFile, &err)) {
        daCritical.noquote() << QObject::tr("Cannot load config file \"%1\": %2")  // cn:无法加载配置文件\"%1\"，原因是%2
                                    .arg(mConfigFilePath, err);
        return false;
    }
    QDomElement configsEle = doc.firstChildElement("configs");
    if (configsEle.isNull()) {
        daWarning.noquote() << QObject::tr("Config file (%1) is missing the <configs> tag").arg(mConfigFilePath);  // cn:配置文件(%1)缺失<configs>标签
        return false;
    }
    QVersionNumber version = mVersion;
    QString ver            = configsEle.attribute("ver");
    if (ver.isEmpty()) {
        version = QVersionNumber();
    }
    // 加载所有管理的配置
    if (!loadFromXml(&configsEle, version)) {
        return false;
    }

    return true;
}

bool DAAppConfig::saveConfig()
{
    QFile xmlConfigFile(mConfigFilePath);
    // 一定要带上QIODevice::Truncate
    if (!xmlConfigFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        // 有配置文件，但打开失败
        daCritical << QObject::tr("Cannot open config file \"%1\": %2")  // cn:无法打开配置文件\"%1\"，原因是%2
                          .arg(mConfigFilePath, xmlConfigFile.errorString());
        return false;
    }
    QDomDocument doc;
    QDomElement configsEle = doc.createElement("configs");
    configsEle.setAttribute("ver", mVersion.toString());
    doc.appendChild(configsEle);
    // 加载所有管理的配置
    if (!saveToXml(&doc, &configsEle, mVersion)) {
        return false;
    }
    QTextStream s(&xmlConfigFile);
    doc.save(s, 2);
    xmlConfigFile.close();
    return true;
}

/**
 * @brief 保存到xml
 * @param doc
 * @param parentElement
 * @return
 */
bool DAAppConfig::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
    auto i = begin();
    for (; i != end(); ++i) {
        QDomElement propEle = doc->createElement("prop");
        propEle.setAttribute("key", i.key());
        QDomElement valueEle = makeElement(i.value(), "value", doc);
        propEle.appendChild(valueEle);
        parentElement->appendChild(propEle);
    }
    parentElement->setAttribute("name", cConfigName);
    return true;
}

/**
 * @brief 从xml加载
 * @param parentElement
 * @return
 */
bool DAAppConfig::loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver)
{
    QDomNodeList ns = parentElement->childNodes();
    for (int i = 0; i < ns.size(); ++i) {
        QDomElement propEle = ns.at(i).toElement();
        if (propEle.isNull()) {
            continue;
        }
        if (propEle.tagName() != "prop") {
            continue;
        }
        QString k            = propEle.attribute("key");
        QDomElement valueEle = propEle.firstChildElement("value");
        if (valueEle.isNull()) {
            continue;
        }
        QVariant v;
        if (!loadElement(v, &valueEle)) {
            continue;
        }
        insert(k, v);
    }
    return true;
}

/**
 * @brief 获取配置文件名字
 * @return
 */
QString DAAppConfig::getConfigFileName()
{
    return "dawork-config.xml";
}

/**
 * @brief 获取配置文件的绝对路径
 * @return
 */
QString DAAppConfig::getAbsoluteConfigFilePath()
{
    return QDir::toNativeSeparators(DAAbstractSettingPage::getConfigFileSavePath() + QDir::separator()
                                    + getConfigFileName());
}

/**
 * @brief 应用配置
 *
 * 此函数应用运行时可生效的配置项。对于在 main.cpp 早期初始化的项
 * （语言、字体、Python 解释器、日志轮转参数、启动画面等），此处不重新初始化，
 * 它们通过 @ref loadEarlyAppConfig 在下次启动时早期读取生效。
 * @return
 */
bool DAAppConfig::apply()
{
    qDebug() << "apply setting";
    SARibbonBar* bar = mMainWindow->ribbonBar();
    if (bar) {
        SARibbonBar::RibbonStyles ribbonStyle =
            static_cast< SARibbonBar::RibbonStyles >(value(DA_CONFIG_KEY_RIBBON_STYLE).toInt());
        bar->setRibbonStyle(ribbonStyle);
    }
    // ribbon 主题（-1 表示不覆盖框架默认主题）
    if (mMainWindow) {
        int ribbonTheme = value(DA_CONFIG_KEY_RIBBON_THEME).toInt();
        if (ribbonTheme >= 0) {
            mMainWindow->setRibbonTheme(static_cast< SARibbonTheme >(ribbonTheme));
        }
    }
    // 日志级别与 UI 队列级别
    DA::DALogger::instance().setLevel(static_cast< DA::DALogLevel >(value(DA_CONFIG_KEY_LOG_LEVEL).toInt()));
    DA::DALogger::instance().setQueueLevel(static_cast< DA::DALogLevel >(value(DA_CONFIG_KEY_LOG_QUEUE_LEVEL).toInt()));
    // 日志显示条数
    bool isOK  = false;
    int logNum = value(DA_CONFIG_KEY_SHOW_LOG_NUM).toInt(&isOK);
    if (isOK && logNum > 10 && logNum < 999999) {
        DA::DAMessageLogQueue::instance().setCapacity(logNum);
    }
    // 最近文件最大条目数
    if (mUI) {
        DAAppActions* actions = mUI->getAppActions();
        if (actions && actions->recentFilesManager) {
            int recentMax = value(DA_CONFIG_KEY_RECENT_FILES_MAX).toInt();
            if (recentMax > 0) {
                actions->recentFilesManager->setMaxEntries(recentMax);
            }
        }
    }
    // 崩溃转储保留天数清理（仅 Windows/MSVC 有效）
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
    int dumpDays = value(DA_CONFIG_KEY_DUMP_RETENTION_DAYS).toInt();
    if (dumpDays > 0) {
        DA::DADumpCapture::cleanupOldDumps(dumpDays);
    }
#endif
    // 自动保存间隔（0=禁用）
    if (mMainWindow) {
        int autosaveMin = value(DA_CONFIG_KEY_AUTOSAVE_INTERVAL).toInt();
        mMainWindow->setupAutosaveTimer(autosaveMin);
    }
    // 脚本执行引擎参数（立即生效；workspace-dir 在下次打开工程时生效）
    DAPyScriptRunner::setScriptTimeout(value(DA_CONFIG_KEY_SCRIPT_TIMEOUT).toInt());
    DAPyScriptRunner::setResultMaxChars(value(DA_CONFIG_KEY_SCRIPT_RESULT_MAX_CHARS).toInt());
    // 操作窗口内部标签页（工作流页/图表/数据表）方位（立即生效，仅影响嵌套停靠区，外层 dock 区域保持默认）
    if (mUI) {
        DAAppDockingArea* dockArea = mUI->getAppDockingArea();
        if (dockArea) {
            const bool tabsAtBottom = value(DA_CONFIG_KEY_DOCK_TAB_POSITION).toString() != QLatin1String("top");
            if (DAPyWorkFlowOperateWidget* wfo = dockArea->getWorkFlowOperateWidget()) {
                wfo->setInnerDockTabsAtBottom(tabsAtBottom);
            }
            if (DAChartOperateWidget* chartOw = dockArea->getChartOperateWidget()) {
                chartOw->setInnerDockTabsAtBottom(tabsAtBottom);
            }
            if (DADataOperateWidget* dataOw = dockArea->getDataOperateWidget()) {
                dataOw->setInnerDockTabsAtBottom(tabsAtBottom);
            }
        }
    }
    // 退出时是否保存ui的状态
    bool isSaveUIState = value(DA_CONFIG_KEY_SAVE_UI_STATE_ON_CLOSE).toBool();
    if (mMainWindow) {
        mMainWindow->setSaveUIStateOnClose(isSaveUIState);
    }
    return true;
}

}  // end da
