#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include "DAAppConfig.h"

using namespace DA;

namespace
{

QJsonObject readJsonObject(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    return QJsonDocument::fromJson(f.readAll()).object();
}

bool writeTextFile(const QString& path, const QByteArray& content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return f.write(content) == content.size();
}

}  // namespace

/**
 * @brief DAAppConfig 的强类型字段、JSON 读写、旧版 XML 迁移与变更通知
 */
class DAAppConfigTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void defaultsAreValid();
    void jsonRoundTrip();
    void normalizeInvalidValues();
    void setterClampsValues();
    void migrateLegacyXml();
    void unknownKeysArePreserved();
    void sectionSignals();
};

/**
 * @brief 默认值应符合文档约定
 */
void DAAppConfigTest::defaultsAreValid()
{
    QTemporaryDir dir;
    DAAppConfig cfg(QDir(dir.path()).filePath(QStringLiteral("not-exists.json")));
    QCOMPARE(cfg.ui().ribbonStyle, 34);
    QCOMPARE(cfg.ui().ribbonTheme, -1);
    QCOMPARE(cfg.ui().fontPointSize, -1.0);
    QCOMPARE(cfg.ui().undoStackLimit, 20);
    QCOMPARE(cfg.ui().dockTabPosition, DADockTabPosition::Bottom);
    QCOMPARE(static_cast< int >(cfg.log().level), static_cast< int >(DALogLevel::Trace));
    QCOMPARE(static_cast< int >(cfg.log().queueLevel), static_cast< int >(DALogLevel::Info));
    QCOMPARE(cfg.log().rotationMode, DALogRotationMode::Rotating);
    QCOMPARE(cfg.log().maxSizeBytes, qint64(10) * 1024 * 1024);
    QCOMPARE(cfg.log().maxFiles, 5);
    QCOMPARE(cfg.log().showLogNum, 5000);
    QCOMPARE(cfg.advanced().recentFilesMax, 10);
    QCOMPARE(cfg.advanced().dumpRetentionDays, 7);
    QCOMPARE(cfg.advanced().showSplash, true);
    QCOMPARE(cfg.advanced().autosaveIntervalMin, 0);
    QCOMPARE(cfg.script().overwritePolicy, DAWorkspaceOverwritePolicy::Ask);
    QCOMPARE(cfg.script().scriptTimeoutSec, 300);
    QCOMPARE(cfg.script().scriptResultMaxChars, 10000);
}

/**
 * @brief 保存后重新加载，各字段应保持一致
 */
void DAAppConfigTest::jsonRoundTrip()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).filePath(QStringLiteral("config.json"));

    {
        DAAppConfig cfg(path);
        DAUISettings ui   = cfg.ui();
        ui.ribbonStyle    = 18;
        ui.ribbonTheme    = 5;
        ui.language       = QStringLiteral("zh_CN");
        ui.dockTabPosition = DADockTabPosition::Top;
        ui.fontFamily     = QStringLiteral("Consolas");
        ui.fontPointSize  = 12.5;
        ui.saveUiStateOnClose = true;
        ui.undoStackLimit = 50;
        cfg.setUI(ui);

        DALogSettings log = cfg.log();
        log.level         = DALogLevel::Warn;
        log.queueLevel    = DALogLevel::Error;
        log.outputStdout  = false;
        log.rotationMode  = DALogRotationMode::Daily;
        log.maxSizeBytes  = qint64(2) * 1024 * 1024;
        log.maxFiles      = 3;
        log.showLogNum    = 8000;
        cfg.setLog(log);

        DAPythonSettings python = cfg.python();
        python.interpreterPath  = QStringLiteral("D:/Python/python.exe");
        python.extraPaths       = QStringList { QStringLiteral("D:/a"), QStringLiteral("D:/b") };
        cfg.setPython(python);

        DAAdvancedSettings advanced = cfg.advanced();
        advanced.recentFilesMax     = 25;
        advanced.dumpRetentionDays  = 30;
        advanced.showSplash         = false;
        advanced.autosaveIntervalMin = 10;
        cfg.setAdvanced(advanced);

        DAScriptSettings script = cfg.script();
        script.workspaceDir     = QStringLiteral("D:/workspace");
        script.overwritePolicy  = DAWorkspaceOverwritePolicy::AlwaysZip;
        script.scriptTimeoutSec = 60;
        script.scriptResultMaxChars = 20000;
        cfg.setScript(script);

        QVERIFY(cfg.save());
    }

    DAAppConfig cfg(path);
    QVERIFY(cfg.load());
    QCOMPARE(cfg.ui().ribbonStyle, 18);
    QCOMPARE(cfg.ui().ribbonTheme, 5);
    QCOMPARE(cfg.ui().language, QStringLiteral("zh_CN"));
    QCOMPARE(cfg.ui().dockTabPosition, DADockTabPosition::Top);
    QCOMPARE(cfg.ui().fontFamily, QStringLiteral("Consolas"));
    QCOMPARE(cfg.ui().fontPointSize, 12.5);
    QCOMPARE(cfg.ui().saveUiStateOnClose, true);
    QCOMPARE(cfg.ui().undoStackLimit, 50);
    QCOMPARE(cfg.log().level, DALogLevel::Warn);
    QCOMPARE(cfg.log().queueLevel, DALogLevel::Error);
    QCOMPARE(cfg.log().outputStdout, false);
    QCOMPARE(cfg.log().rotationMode, DALogRotationMode::Daily);
    QCOMPARE(cfg.log().maxSizeBytes, qint64(2) * 1024 * 1024);
    QCOMPARE(cfg.log().maxFiles, 3);
    QCOMPARE(cfg.log().showLogNum, 8000);
    QCOMPARE(cfg.python().interpreterPath, QStringLiteral("D:/Python/python.exe"));
    QCOMPARE(cfg.python().extraPaths, QStringList({ QStringLiteral("D:/a"), QStringLiteral("D:/b") }));
    QCOMPARE(cfg.advanced().recentFilesMax, 25);
    QCOMPARE(cfg.advanced().dumpRetentionDays, 30);
    QCOMPARE(cfg.advanced().showSplash, false);
    QCOMPARE(cfg.advanced().autosaveIntervalMin, 10);
    QCOMPARE(cfg.script().workspaceDir, QStringLiteral("D:/workspace"));
    QCOMPARE(cfg.script().overwritePolicy, DAWorkspaceOverwritePolicy::AlwaysZip);
    QCOMPARE(cfg.script().scriptTimeoutSec, 60);
    QCOMPARE(cfg.script().scriptResultMaxChars, 20000);
}

/**
 * @brief 非法取值在加载时应被规范化
 */
void DAAppConfigTest::normalizeInvalidValues()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).filePath(QStringLiteral("config.json"));

    QJsonObject ui;
    ui.insert(QStringLiteral("ribbon-theme"), -7);
    ui.insert(QStringLiteral("font-point-size"), 0.0);
    ui.insert(QStringLiteral("undo-stack-limit"), -3);
    ui.insert(QStringLiteral("dock-tab-position"), QStringLiteral("left"));

    QJsonObject log;
    log.insert(QStringLiteral("level"), QStringLiteral("bogus"));
    log.insert(QStringLiteral("queue-level"), 99);
    log.insert(QStringLiteral("rotation-mode"), QStringLiteral("weekly"));
    log.insert(QStringLiteral("max-size-bytes"), -5);
    log.insert(QStringLiteral("max-files"), 0);
    log.insert(QStringLiteral("show-log-num"), 1);

    QJsonObject advanced;
    advanced.insert(QStringLiteral("recent-files-max"), 0);
    advanced.insert(QStringLiteral("dump-retention-days"), -1);
    advanced.insert(QStringLiteral("autosave-interval-min"), -10);

    QJsonObject script;
    script.insert(QStringLiteral("workspace-overwrite-policy"), QStringLiteral("unknown"));
    script.insert(QStringLiteral("timeout-sec"), -9);
    script.insert(QStringLiteral("result-max-chars"), 0);

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("ui"), ui);
    root.insert(QStringLiteral("log"), log);
    root.insert(QStringLiteral("advanced"), advanced);
    root.insert(QStringLiteral("script"), script);
    QVERIFY(writeTextFile(path, QJsonDocument(root).toJson()));

    DAAppConfig cfg(path);
    QVERIFY(cfg.load());
    QCOMPARE(cfg.ui().ribbonTheme, -1);
    QCOMPARE(cfg.ui().fontPointSize, -1.0);
    QCOMPARE(cfg.ui().undoStackLimit, 20);
    QCOMPARE(cfg.ui().dockTabPosition, DADockTabPosition::Bottom);
    QCOMPARE(static_cast< int >(cfg.log().level), static_cast< int >(DALogLevel::Trace));
    QCOMPARE(static_cast< int >(cfg.log().queueLevel), static_cast< int >(DALogLevel::Info));
    QCOMPARE(cfg.log().rotationMode, DALogRotationMode::Rotating);
    QCOMPARE(cfg.log().maxSizeBytes, qint64(10) * 1024 * 1024);
    QCOMPARE(cfg.log().maxFiles, 5);
    QCOMPARE(cfg.log().showLogNum, 5000);
    QCOMPARE(cfg.advanced().recentFilesMax, 10);
    QCOMPARE(cfg.advanced().dumpRetentionDays, 7);
    QCOMPARE(cfg.advanced().autosaveIntervalMin, 0);
    QCOMPARE(cfg.script().overwritePolicy, DAWorkspaceOverwritePolicy::Ask);
    QCOMPARE(cfg.script().scriptTimeoutSec, 300);
    QCOMPARE(cfg.script().scriptResultMaxChars, 10000);
}

/**
 * @brief setter 也做同样的钳制，防止设置页写入非法值
 */
void DAAppConfigTest::setterClampsValues()
{
    QTemporaryDir dir;
    DAAppConfig cfg(QDir(dir.path()).filePath(QStringLiteral("config.json")));

    DAUISettings ui;
    ui.undoStackLimit = -5;
    ui.fontPointSize  = -2.0;
    cfg.setUI(ui);
    QCOMPARE(cfg.ui().undoStackLimit, 20);
    QCOMPARE(cfg.ui().fontPointSize, -1.0);

    DALogSettings log;
    log.maxSizeBytes = -1;
    log.maxFiles     = -1;
    cfg.setLog(log);
    QCOMPARE(cfg.log().maxSizeBytes, qint64(10) * 1024 * 1024);
    QCOMPARE(cfg.log().maxFiles, 5);
}

/**
 * @brief 旧版 XML 应被一次性迁移为 JSON，并按新路径映射
 */
void DAAppConfigTest::migrateLegacyXml()
{
    QTemporaryDir dir;
    const QString jsonPath = QDir(dir.path()).filePath(QStringLiteral("dawork-config.json"));
    const QString xmlPath  = QDir(dir.path()).filePath(QStringLiteral("dawork-config.xml"));

    const QByteArray legacyXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<configs ver="0.0.3" name="da-app">
  <prop key="ribbon-style"><value type="int">18</value></prop>
  <prop key="language"><value type="QString">zh_CN</value></prop>
  <prop key="app-font-size"><value type="int">11</value></prop>
  <prop key="log-level"><value type="int">4</value></prop>
  <prop key="log-rotation-mode"><value type="int">1</value></prop>
  <prop key="log-max-size"><value type="int">2097152</value></prop>
  <prop key="workspace-overwrite-policy"><value type="QString">always-local</value></prop>
  <prop key="python-extra-paths"><value class="QStringList" size="2"><r>D:/a</r><r>D:/b</r></value></prop>
  <prop key="workflow-timeout"><value type="double">60</value></prop>
  <prop key="plugin-extra-paths"><value class="QStringList" size="1"><r>D:/plugins</r></value></prop>
  <prop key="node-script-paths"><value class="QStringList" size="1"><r>D:/nodes</r></value></prop>
</configs>)";
    QVERIFY(writeTextFile(xmlPath, legacyXml));

    DAAppConfig cfg(jsonPath);
    QVERIFY(cfg.load());
    QCOMPARE(cfg.ui().ribbonStyle, 18);
    QCOMPARE(cfg.ui().language, QStringLiteral("zh_CN"));
    QCOMPARE(cfg.ui().fontPointSize, 11.0);
    QCOMPARE(static_cast< int >(cfg.log().level), static_cast< int >(DALogLevel::Error));
    QCOMPARE(cfg.log().rotationMode, DALogRotationMode::Daily);
    QCOMPARE(cfg.log().maxSizeBytes, qint64(2097152));
    QCOMPARE(cfg.script().overwritePolicy, DAWorkspaceOverwritePolicy::AlwaysLocal);
    QCOMPARE(cfg.python().extraPaths, QStringList({ QStringLiteral("D:/a"), QStringLiteral("D:/b") }));

    // 已写出 JSON，旧文件改名为 .bak
    QVERIFY(QFile::exists(jsonPath));
    QVERIFY(!QFile::exists(xmlPath));
    QVERIFY(QFile::exists(xmlPath + QStringLiteral(".bak")));

    // 已删除的键不应出现在新配置中
    const QJsonObject root = readJsonObject(jsonPath);
    QVERIFY(!root.contains(QStringLiteral("workflow-timeout")));
    QVERIFY(!root.contains(QStringLiteral("plugin-extra-paths")));
    QVERIFY(!root.contains(QStringLiteral("node-script-paths")));
}

/**
 * @brief 未知键在保存后应原样保留（便于版本降级）
 */
void DAAppConfigTest::unknownKeysArePreserved()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).filePath(QStringLiteral("config.json"));

    QJsonObject ui;
    ui.insert(QStringLiteral("language"), QStringLiteral("en_US"));
    ui.insert(QStringLiteral("future-ui-key"), QStringLiteral("keep"));

    QJsonObject future;
    future.insert(QStringLiteral("enabled"), true);

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("ui"), ui);
    root.insert(QStringLiteral("future-feature"), future);
    QVERIFY(writeTextFile(path, QJsonDocument(root).toJson()));

    DAAppConfig cfg(path);
    QVERIFY(cfg.load());
    QVERIFY(cfg.save());

    const QJsonObject out = readJsonObject(path);
    QVERIFY(out.contains(QStringLiteral("future-feature")));
    QCOMPARE(out.value(QStringLiteral("future-feature")).toObject().value(QStringLiteral("enabled")).toBool(), true);
    QCOMPARE(out.value(QStringLiteral("ui")).toObject().value(QStringLiteral("future-ui-key")).toString(),
             QStringLiteral("keep"));
    QCOMPARE(out.value(QStringLiteral("ui")).toObject().value(QStringLiteral("language")).toString(),
             QStringLiteral("en_US"));
}

/**
 * @brief setter 发出分节通知；load() 不发通知
 */
void DAAppConfigTest::sectionSignals()
{
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).filePath(QStringLiteral("config.json"));

    DAAppConfig cfg(path);
    int sectionCount = 0;
    DAAppConfig::Section lastSection = DAAppConfig::Section::UI;
    connect(&cfg, &DAAppConfig::sectionChanged, this, [&](DAAppConfig::Section s) {
        ++sectionCount;
        lastSection = s;
    });
    QSignalSpy changedSpy(&cfg, &DAAppConfig::changed);

    DAUISettings ui = cfg.ui();
    cfg.setUI(ui);
    QCOMPARE(sectionCount, 1);
    QCOMPARE(lastSection, DAAppConfig::Section::UI);
    QCOMPARE(changedSpy.count(), 1);

    DALogSettings log = cfg.log();
    cfg.setLog(log);
    QCOMPARE(sectionCount, 2);
    QCOMPARE(lastSection, DAAppConfig::Section::Log);
    QCOMPARE(changedSpy.count(), 2);

    QVERIFY(cfg.save());

    // 加载过程不发信号
    DAAppConfig cfg2(path);
    int loadSectionCount = 0;
    connect(&cfg2, &DAAppConfig::sectionChanged, this, [&](DAAppConfig::Section) { ++loadSectionCount; });
    QSignalSpy loadChangedSpy(&cfg2, &DAAppConfig::changed);
    QVERIFY(cfg2.load());
    QCOMPARE(loadSectionCount, 0);
    QCOMPARE(loadChangedSpy.count(), 0);
}

QTEST_MAIN(DAAppConfigTest)

#include "main.moc"
