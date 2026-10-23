#include "DAAppConfig.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDomDocument>
#include <QDomElement>
#include <utility>
#include "DADir.h"
#include "DALogCategory.h"

namespace DA
{

namespace
{
constexpr int kConfigVersion = 1;

// 日志级别名（下标与 DALogLevel 枚举值一一对应）
const char* const kLogLevelNames[] = { "trace", "debug", "info", "warn", "error", "critical", "off" };
constexpr int kLogLevelCount = int(sizeof(kLogLevelNames) / sizeof(kLogLevelNames[ 0 ]));

// ---- JSON 值读取（宽松：字符串与数字互认，避免手工改文件后不可用） ----

int jsonToInt(const QJsonValue& v, int def)
{
    if (v.isDouble()) {
        return v.toInt();
    }
    if (v.isString()) {
        bool ok   = false;
        const int r = v.toString().toInt(&ok);
        return ok ? r : def;
    }
    return def;
}

qint64 jsonToInt64(const QJsonValue& v, qint64 def)
{
    if (v.isDouble()) {
        return static_cast< qint64 >(v.toDouble());
    }
    if (v.isString()) {
        bool ok        = false;
        const qint64 r = v.toString().toLongLong(&ok);
        return ok ? r : def;
    }
    return def;
}

double jsonToDouble(const QJsonValue& v, double def)
{
    if (v.isDouble()) {
        return v.toDouble();
    }
    if (v.isString()) {
        bool ok         = false;
        const double r  = v.toString().toDouble(&ok);
        return ok ? r : def;
    }
    return def;
}

bool jsonToBool(const QJsonValue& v, bool def)
{
    if (v.isBool()) {
        return v.toBool();
    }
    if (v.isDouble()) {
        return v.toInt() != 0;
    }
    if (v.isString()) {
        const QString s = v.toString().trimmed().toLower();
        if (s == QLatin1String("true") || s == QLatin1String("1")) {
            return true;
        }
        if (s == QLatin1String("false") || s == QLatin1String("0")) {
            return false;
        }
    }
    return def;
}

QString jsonToString(const QJsonValue& v, const QString& def)
{
    if (v.isString()) {
        return v.toString();
    }
    return def;
}

QStringList jsonToStringList(const QJsonValue& v, const QStringList& def)
{
    if (v.isArray()) {
        QStringList r;
        const QJsonArray arr = v.toArray();
        for (const QJsonValue& item : arr) {
            if (item.isString()) {
                r << item.toString();
            }
        }
        return r;
    }
    if (v.isString()) {
        return v.toString().isEmpty() ? def : QStringList { v.toString() };
    }
    return def;
}

// ---- 枚举解析/序列化（JSON 用可读字符串，同时兼容旧版整数） ----

DALogLevel logLevelFromValue(const QJsonValue& v, DALogLevel def)
{
    if (v.isDouble()) {
        const int i = v.toInt();
        if (i >= 0 && i < kLogLevelCount) {
            return static_cast< DALogLevel >(i);
        }
        return def;
    }
    if (v.isString()) {
        const QString s = v.toString().trimmed().toLower();
        for (int i = 0; i < kLogLevelCount; ++i) {
            if (s == QLatin1String(kLogLevelNames[ i ])) {
                return static_cast< DALogLevel >(i);
            }
        }
        // 兼容常见写法
        if (s == QLatin1String("warning")) {
            return DALogLevel::Warn;
        }
    }
    return def;
}

QString logLevelToValue(DALogLevel v)
{
    const int i = static_cast< int >(v);
    if (i >= 0 && i < kLogLevelCount) {
        return QString::fromLatin1(kLogLevelNames[ i ]);
    }
    return QString::fromLatin1(kLogLevelNames[ 0 ]);
}

DALogRotationMode rotationModeFromValue(const QJsonValue& v, DALogRotationMode def)
{
    if (v.isDouble()) {
        const int i = v.toInt();
        if (i >= 0 && i <= 2) {
            return static_cast< DALogRotationMode >(i);
        }
        return def;
    }
    if (v.isString()) {
        const QString s = v.toString().trimmed().toLower();
        if (s == QLatin1String("rotating")) {
            return DALogRotationMode::Rotating;
        }
        if (s == QLatin1String("daily")) {
            return DALogRotationMode::Daily;
        }
        if (s == QLatin1String("console")) {
            return DALogRotationMode::Console;
        }
    }
    return def;
}

QString rotationModeToValue(DALogRotationMode v)
{
    switch (v) {
    case DALogRotationMode::Daily:
        return QStringLiteral("daily");
    case DALogRotationMode::Console:
        return QStringLiteral("console");
    case DALogRotationMode::Rotating:
    default:
        return QStringLiteral("rotating");
    }
}

DADockTabPosition tabPositionFromValue(const QJsonValue& v, DADockTabPosition def)
{
    if (v.isDouble()) {
        const int i = v.toInt();
        if (i == 0) {
            return DADockTabPosition::Bottom;
        }
        if (i == 1) {
            return DADockTabPosition::Top;
        }
        return def;
    }
    if (v.isString()) {
        const QString s = v.toString().trimmed().toLower();
        if (s == QLatin1String("top")) {
            return DADockTabPosition::Top;
        }
        if (s == QLatin1String("bottom")) {
            return DADockTabPosition::Bottom;
        }
    }
    return def;
}

QString tabPositionToValue(DADockTabPosition v)
{
    return (v == DADockTabPosition::Top) ? QStringLiteral("top") : QStringLiteral("bottom");
}

DAWorkspaceOverwritePolicy overwritePolicyFromValue(const QJsonValue& v, DAWorkspaceOverwritePolicy def)
{
    if (v.isDouble()) {
        const int i = v.toInt();
        if (i >= 0 && i <= 2) {
            return static_cast< DAWorkspaceOverwritePolicy >(i);
        }
        return def;
    }
    if (v.isString()) {
        const QString s = v.toString().trimmed().toLower();
        if (s == QLatin1String("ask")) {
            return DAWorkspaceOverwritePolicy::Ask;
        }
        if (s == QLatin1String("always-local") || s == QLatin1String("alwayslocal")) {
            return DAWorkspaceOverwritePolicy::AlwaysLocal;
        }
        if (s == QLatin1String("always-zip") || s == QLatin1String("alwayszip")) {
            return DAWorkspaceOverwritePolicy::AlwaysZip;
        }
    }
    return def;
}

QString overwritePolicyToValue(DAWorkspaceOverwritePolicy v)
{
    switch (v) {
    case DAWorkspaceOverwritePolicy::AlwaysLocal:
        return QStringLiteral("always-local");
    case DAWorkspaceOverwritePolicy::AlwaysZip:
        return QStringLiteral("always-zip");
    case DAWorkspaceOverwritePolicy::Ask:
    default:
        return QStringLiteral("ask");
    }
}

// ---- 旧版 XML 文本转换 ----

bool xmlTextToBool(const QString& s, bool def)
{
    const QString t = s.trimmed().toLower();
    if (t == QLatin1String("true") || t == QLatin1String("1")) {
        return true;
    }
    if (t == QLatin1String("false") || t == QLatin1String("0")) {
        return false;
    }
    return def;
}

// ---- 取值范围规范化（加载与 setter 共用） ----

void normalizeUI(DAUISettings& s)
{
    if (s.ribbonTheme < 0) {
        s.ribbonTheme = -1;
    }
    if (s.fontPointSize <= 0.0) {
        s.fontPointSize = -1.0;
    }
    if (s.undoStackLimit < 0) {
        s.undoStackLimit = 20;
    }
    s.language = s.language.trimmed();
}

void normalizeLog(DALogSettings& s)
{
    const int level = static_cast< int >(s.level);
    if (level < 0 || level >= kLogLevelCount) {
        s.level = DALogLevel::Trace;
    }
    const int queueLevel = static_cast< int >(s.queueLevel);
    if (queueLevel < 0 || queueLevel >= kLogLevelCount) {
        s.queueLevel = DALogLevel::Info;
    }
    const int rotationMode = static_cast< int >(s.rotationMode);
    if (rotationMode < 0 || rotationMode > 2) {
        s.rotationMode = DALogRotationMode::Rotating;
    }
    if (s.maxSizeBytes <= 0) {
        s.maxSizeBytes = qint64(10) * 1024 * 1024;
    }
    if (s.maxFiles <= 0) {
        s.maxFiles = 5;
    }
    if (s.showLogNum <= 10 || s.showLogNum >= 999999) {
        s.showLogNum = 5000;
    }
}

void normalizePython(DAPythonSettings& s)
{
    s.interpreterPath = s.interpreterPath.trimmed();
    QStringList cleaned;
    for (const QString& p : std::as_const(s.extraPaths)) {
        const QString t = p.trimmed();
        if (!t.isEmpty()) {
            cleaned << t;
        }
    }
    s.extraPaths = cleaned;
}

void normalizeAdvanced(DAAdvancedSettings& s)
{
    if (s.recentFilesMax <= 0) {
        s.recentFilesMax = 10;
    }
    if (s.dumpRetentionDays <= 0) {
        s.dumpRetentionDays = 7;
    }
    if (s.autosaveIntervalMin < 0) {
        s.autosaveIntervalMin = 0;
    }
}

void normalizeScript(DAScriptSettings& s)
{
    s.workspaceDir = s.workspaceDir.trimmed();
    const int policy = static_cast< int >(s.overwritePolicy);
    if (policy < 0 || policy > 2) {
        s.overwritePolicy = DAWorkspaceOverwritePolicy::Ask;
    }
    if (s.scriptTimeoutSec < 0) {
        s.scriptTimeoutSec = 300;
    }
    if (s.scriptResultMaxChars <= 0) {
        s.scriptResultMaxChars = 10000;
    }
}

}  // namespace

DAAppConfig::DAAppConfig(const QString& configFilePath, QObject* parent)
    : QObject(parent)
    , mConfigFilePath(configFilePath.isEmpty() ? getAbsoluteConfigFilePath() : configFilePath)
{
}

DAAppConfig::~DAAppConfig() = default;

/**
 * @brief 从磁盘加载配置
 *
 * 优先级：新的 JSON 配置 → 旧版 XML 配置（一次性迁移）→ 默认值。
 * 任一阶段得到的值都会经过 @ref normalize 规范化；加载过程不发变更信号。
 * @param createIfMissing 没有任何配置文件时是否生成一份默认配置
 * @return 是否成功读到配置（生成默认配置也算成功）
 */
bool DAAppConfig::load(bool createIfMissing)
{
    QFile jsonFile(mConfigFilePath);
    if (!jsonFile.exists()) {
        // 没有 JSON：尝试从旧版 XML 一次性迁移
        const QString legacyPath = legacyConfigFilePath();
        if (QFileInfo::exists(legacyPath)) {
            if (loadLegacyXml(legacyPath)) {
                normalize();
                if (!save()) {
                    daWarning.noquote() << tr("Migrated configuration but failed to write \"%1\"")  // cn:已迁移配置，但写入\"%1\"失败
                                               .arg(mConfigFilePath);
                    return true;  // 内存中已是迁移后的值，可以继续运行
                }
                const QString bakPath = legacyPath + QStringLiteral(".bak");
                QFile::remove(bakPath);
                if (QFile::rename(legacyPath, bakPath)) {
                    daInfo.noquote() << tr("Migrated configuration from \"%1\" to \"%2\"")  // cn:已把配置从\"%1\"迁移为\"%2\"
                                            .arg(legacyPath, mConfigFilePath);
                } else {
                    daWarning.noquote() << tr("Migrated configuration but cannot rename \"%1\"")  // cn:已迁移配置，但无法重命名\"%1\"
                                               .arg(legacyPath);
                }
                return true;
            }
            daWarning.noquote() << tr("Cannot load config file \"%1\"")  // cn:无法加载配置文件\"%1\"
                                       .arg(legacyPath);
        }
        normalize();
        if (createIfMissing) {
            return save();
        }
        return false;
    }

    if (!jsonFile.open(QIODevice::ReadOnly)) {
        daWarning.noquote() << tr("Cannot open config file \"%1\": %2")  // cn:无法打开配置文件\"%1\"，原因是%2
                                   .arg(mConfigFilePath, jsonFile.errorString());
        normalize();
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
    jsonFile.close();
    if (parseError.error != QJsonParseError::NoError) {
        daCritical.noquote() << tr("Cannot load config file \"%1\": %2")  // cn:无法加载配置文件\"%1\"，原因是%2
                                    .arg(mConfigFilePath, parseError.errorString());
        normalize();
        return false;
    }
    if (!doc.isObject()) {
        daCritical.noquote() << tr("Cannot load config file \"%1\": root is not an object")  // cn:无法加载配置文件\"%1\"：根节点不是对象
                                    .arg(mConfigFilePath);
        normalize();
        return false;
    }
    loadFromJsonObject(doc.object());
    normalize();
    return true;
}

/**
 * @brief 保存到磁盘
 *
 * 采用 QSaveFile 原子写：先写临时文件再重命名，避免写到一半崩溃留下截断的配置。
 * 未知键会从加载时的原始根对象原样保留。
 * @return 是否保存成功
 */
bool DAAppConfig::save() const
{
    const QFileInfo fi(mConfigFilePath);
    QDir().mkpath(fi.absolutePath());
    QSaveFile file(mConfigFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        daCritical.noquote() << tr("Cannot open config file \"%1\": %2")  // cn:无法打开配置文件\"%1\"，原因是%2
                                    .arg(mConfigFilePath, file.errorString());
        return false;
    }
    const QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        daCritical.noquote() << tr("Cannot write config file \"%1\": %2")  // cn:无法写入配置文件\"%1\"，原因是%2
                                    .arg(mConfigFilePath, file.errorString());
        return false;
    }
    return true;
}

/**
 * @brief 恢复全部默认值并通知各分节
 */
void DAAppConfig::resetToDefaults()
{
    mUiSettings       = DAUISettings();
    mLogSettings      = DALogSettings();
    mPythonSettings   = DAPythonSettings();
    mAdvancedSettings = DAAdvancedSettings();
    mScriptSettings   = DAScriptSettings();
    normalize();
    for (Section s : { Section::UI, Section::Log, Section::Python, Section::Advanced, Section::Script }) {
        Q_EMIT sectionChanged(s);
    }
    Q_EMIT changed();
}

const DAUISettings& DAAppConfig::ui() const
{
    return mUiSettings;
}

/**
 * @brief 设置通用/界面配置
 *
 * 写入前做规范化，常用于避免设置页写入非法值。
 * @param v 新的配置值
 */
void DAAppConfig::setUI(const DAUISettings& v)
{
    mUiSettings = v;
    normalizeUI(mUiSettings);
    Q_EMIT sectionChanged(Section::UI);
    Q_EMIT changed();
}

const DALogSettings& DAAppConfig::log() const
{
    return mLogSettings;
}

void DAAppConfig::setLog(const DALogSettings& v)
{
    mLogSettings = v;
    normalizeLog(mLogSettings);
    Q_EMIT sectionChanged(Section::Log);
    Q_EMIT changed();
}

const DAPythonSettings& DAAppConfig::python() const
{
    return mPythonSettings;
}

void DAAppConfig::setPython(const DAPythonSettings& v)
{
    mPythonSettings = v;
    normalizePython(mPythonSettings);
    Q_EMIT sectionChanged(Section::Python);
    Q_EMIT changed();
}

const DAAdvancedSettings& DAAppConfig::advanced() const
{
    return mAdvancedSettings;
}

void DAAppConfig::setAdvanced(const DAAdvancedSettings& v)
{
    mAdvancedSettings = v;
    normalizeAdvanced(mAdvancedSettings);
    Q_EMIT sectionChanged(Section::Advanced);
    Q_EMIT changed();
}

const DAScriptSettings& DAAppConfig::script() const
{
    return mScriptSettings;
}

void DAAppConfig::setScript(const DAScriptSettings& v)
{
    mScriptSettings = v;
    normalizeScript(mScriptSettings);
    Q_EMIT sectionChanged(Section::Script);
    Q_EMIT changed();
}

/**
 * @brief 当前配置文件名
 */
QString DAAppConfig::getConfigFileName()
{
    return QStringLiteral("dawork-config.json");
}

/**
 * @brief 旧版 XML 配置文件名（仅用于一次性迁移）
 */
QString DAAppConfig::getLegacyConfigFileName()
{
    return QStringLiteral("dawork-config.xml");
}

/**
 * @brief 当前配置文件的绝对路径
 */
QString DAAppConfig::getAbsoluteConfigFilePath()
{
    return QDir::toNativeSeparators(DADir::getConfigPath() + QDir::separator() + getConfigFileName());
}

/**
 * @brief 旧版 XML 配置文件的绝对路径（与当前配置文件同目录）
 */
QString DAAppConfig::legacyConfigFilePath() const
{
    return QFileInfo(mConfigFilePath).absolutePath() + QDir::separator() + getLegacyConfigFileName();
}

/**
 * @brief 全部字段的取值规范化
 */
void DAAppConfig::normalize()
{
    normalizeUI(mUiSettings);
    normalizeLog(mLogSettings);
    normalizePython(mPythonSettings);
    normalizeAdvanced(mAdvancedSettings);
    normalizeScript(mScriptSettings);
}

/**
 * @brief 组装为 JSON 对象
 *
 * 以加载时的原始根对象为底，只覆盖已知路径，从而保留将来版本写入的未知键。
 */
QJsonObject DAAppConfig::toJson() const
{
    QJsonObject root = mUnknownRoot;
    root.insert(QStringLiteral("version"), kConfigVersion);

    QJsonObject ui = root.value(QStringLiteral("ui")).toObject();
    ui.insert(QStringLiteral("ribbon-style"), mUiSettings.ribbonStyle);
    ui.insert(QStringLiteral("ribbon-theme"), mUiSettings.ribbonTheme);
    ui.insert(QStringLiteral("language"), mUiSettings.language);
    ui.insert(QStringLiteral("dock-tab-position"), tabPositionToValue(mUiSettings.dockTabPosition));
    ui.insert(QStringLiteral("font-family"), mUiSettings.fontFamily);
    ui.insert(QStringLiteral("font-point-size"), mUiSettings.fontPointSize);
    ui.insert(QStringLiteral("save-ui-state-on-close"), mUiSettings.saveUiStateOnClose);
    ui.insert(QStringLiteral("undo-stack-limit"), mUiSettings.undoStackLimit);
    root.insert(QStringLiteral("ui"), ui);

    QJsonObject logObj = root.value(QStringLiteral("log")).toObject();
    logObj.insert(QStringLiteral("level"), logLevelToValue(mLogSettings.level));
    logObj.insert(QStringLiteral("queue-level"), logLevelToValue(mLogSettings.queueLevel));
    logObj.insert(QStringLiteral("output-stdout"), mLogSettings.outputStdout);
    logObj.insert(QStringLiteral("rotation-mode"), rotationModeToValue(mLogSettings.rotationMode));
    logObj.insert(QStringLiteral("max-size-bytes"), mLogSettings.maxSizeBytes);
    logObj.insert(QStringLiteral("max-files"), mLogSettings.maxFiles);
    logObj.insert(QStringLiteral("show-log-num"), mLogSettings.showLogNum);
    root.insert(QStringLiteral("log"), logObj);

    QJsonObject python = root.value(QStringLiteral("python")).toObject();
    python.insert(QStringLiteral("interpreter-path"), mPythonSettings.interpreterPath);
    QJsonArray extraPaths;
    for (const QString& p : std::as_const(mPythonSettings.extraPaths)) {
        extraPaths.append(p);
    }
    python.insert(QStringLiteral("extra-paths"), extraPaths);
    root.insert(QStringLiteral("python"), python);

    QJsonObject advanced = root.value(QStringLiteral("advanced")).toObject();
    advanced.insert(QStringLiteral("recent-files-max"), mAdvancedSettings.recentFilesMax);
    advanced.insert(QStringLiteral("dump-retention-days"), mAdvancedSettings.dumpRetentionDays);
    advanced.insert(QStringLiteral("show-splash"), mAdvancedSettings.showSplash);
    advanced.insert(QStringLiteral("autosave-interval-min"), mAdvancedSettings.autosaveIntervalMin);
    root.insert(QStringLiteral("advanced"), advanced);

    QJsonObject script = root.value(QStringLiteral("script")).toObject();
    script.insert(QStringLiteral("workspace-dir"), mScriptSettings.workspaceDir);
    script.insert(QStringLiteral("workspace-overwrite-policy"), overwritePolicyToValue(mScriptSettings.overwritePolicy));
    script.insert(QStringLiteral("timeout-sec"), mScriptSettings.scriptTimeoutSec);
    script.insert(QStringLiteral("result-max-chars"), mScriptSettings.scriptResultMaxChars);
    root.insert(QStringLiteral("script"), script);

    return root;
}

/**
 * @brief 从 JSON 根对象读取各分节
 *
 * 缺失的键保持当前值（默认值），因此只写部分键的配置文件也能正常使用。
 * @param root JSON 根对象
 */
void DAAppConfig::loadFromJsonObject(const QJsonObject& root)
{
    mUnknownRoot = root;

    const int version = jsonToInt(root.value(QStringLiteral("version")), kConfigVersion);
    if (version > kConfigVersion) {
        daWarning.noquote() << tr("Config file \"%1\" has a newer version %2")  // cn:配置文件\"%1\"的版本 %2 比当前程序新
                                   .arg(mConfigFilePath)
                                   .arg(version);
    }

    const QJsonObject ui = root.value(QStringLiteral("ui")).toObject();
    mUiSettings.ribbonStyle        = jsonToInt(ui.value(QStringLiteral("ribbon-style")), mUiSettings.ribbonStyle);
    mUiSettings.ribbonTheme        = jsonToInt(ui.value(QStringLiteral("ribbon-theme")), mUiSettings.ribbonTheme);
    mUiSettings.language           = jsonToString(ui.value(QStringLiteral("language")), mUiSettings.language);
    mUiSettings.dockTabPosition    = tabPositionFromValue(ui.value(QStringLiteral("dock-tab-position")), mUiSettings.dockTabPosition);
    mUiSettings.fontFamily         = jsonToString(ui.value(QStringLiteral("font-family")), mUiSettings.fontFamily);
    mUiSettings.fontPointSize      = jsonToDouble(ui.value(QStringLiteral("font-point-size")), mUiSettings.fontPointSize);
    mUiSettings.saveUiStateOnClose = jsonToBool(ui.value(QStringLiteral("save-ui-state-on-close")), mUiSettings.saveUiStateOnClose);
    mUiSettings.undoStackLimit     = jsonToInt(ui.value(QStringLiteral("undo-stack-limit")), mUiSettings.undoStackLimit);

    const QJsonObject logObj = root.value(QStringLiteral("log")).toObject();
    mLogSettings.level         = logLevelFromValue(logObj.value(QStringLiteral("level")), mLogSettings.level);
    mLogSettings.queueLevel    = logLevelFromValue(logObj.value(QStringLiteral("queue-level")), mLogSettings.queueLevel);
    mLogSettings.outputStdout  = jsonToBool(logObj.value(QStringLiteral("output-stdout")), mLogSettings.outputStdout);
    mLogSettings.rotationMode  = rotationModeFromValue(logObj.value(QStringLiteral("rotation-mode")), mLogSettings.rotationMode);
    mLogSettings.maxSizeBytes  = jsonToInt64(logObj.value(QStringLiteral("max-size-bytes")), mLogSettings.maxSizeBytes);
    mLogSettings.maxFiles      = jsonToInt(logObj.value(QStringLiteral("max-files")), mLogSettings.maxFiles);
    mLogSettings.showLogNum    = jsonToInt(logObj.value(QStringLiteral("show-log-num")), mLogSettings.showLogNum);

    const QJsonObject python = root.value(QStringLiteral("python")).toObject();
    mPythonSettings.interpreterPath = jsonToString(python.value(QStringLiteral("interpreter-path")), mPythonSettings.interpreterPath);
    mPythonSettings.extraPaths      = jsonToStringList(python.value(QStringLiteral("extra-paths")), mPythonSettings.extraPaths);

    const QJsonObject advanced = root.value(QStringLiteral("advanced")).toObject();
    mAdvancedSettings.recentFilesMax     = jsonToInt(advanced.value(QStringLiteral("recent-files-max")), mAdvancedSettings.recentFilesMax);
    mAdvancedSettings.dumpRetentionDays  = jsonToInt(advanced.value(QStringLiteral("dump-retention-days")), mAdvancedSettings.dumpRetentionDays);
    mAdvancedSettings.showSplash         = jsonToBool(advanced.value(QStringLiteral("show-splash")), mAdvancedSettings.showSplash);
    mAdvancedSettings.autosaveIntervalMin = jsonToInt(advanced.value(QStringLiteral("autosave-interval-min")), mAdvancedSettings.autosaveIntervalMin);

    const QJsonObject script = root.value(QStringLiteral("script")).toObject();
    mScriptSettings.workspaceDir = jsonToString(script.value(QStringLiteral("workspace-dir")), mScriptSettings.workspaceDir);
    mScriptSettings.overwritePolicy = overwritePolicyFromValue(script.value(QStringLiteral("workspace-overwrite-policy")),
                                                              mScriptSettings.overwritePolicy);
    mScriptSettings.scriptTimeoutSec     = jsonToInt(script.value(QStringLiteral("timeout-sec")), mScriptSettings.scriptTimeoutSec);
    mScriptSettings.scriptResultMaxChars = jsonToInt(script.value(QStringLiteral("result-max-chars")), mScriptSettings.scriptResultMaxChars);
}

/**
 * @brief 从旧版 XML 配置读取（一次性迁移）
 *
 * 只处理保留的键；workflow-timeout、plugin-extra-paths、node-script-paths
 * 已删除，读取时直接忽略。
 * @param path 旧版配置文件路径
 * @return 是否成功解析
 */
bool DAAppConfig::loadLegacyXml(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QString err;
    QDomDocument doc;
    if (!doc.setContent(&file, &err)) {
        daCritical.noquote() << tr("Cannot load config file \"%1\": %2")  // cn:无法加载配置文件\"%1\"，原因是%2
                                    .arg(path, err);
        return false;
    }
    const QDomElement configs = doc.firstChildElement(QStringLiteral("configs"));
    if (configs.isNull()) {
        daWarning.noquote() << tr("Config file (%1) is missing the <configs> tag")  // cn:配置文件(%1)缺失<configs>标签
                                   .arg(path);
        return false;
    }
    for (QDomElement prop = configs.firstChildElement(QStringLiteral("prop")); !prop.isNull();
         prop             = prop.nextSiblingElement(QStringLiteral("prop"))) {
        const QString key          = prop.attribute(QStringLiteral("key"));
        const QDomElement valueEle = prop.firstChildElement(QStringLiteral("value"));
        if (key.isEmpty() || valueEle.isNull()) {
            continue;
        }
        applyLegacyProp(key, valueEle);
    }
    return true;
}

/**
 * @brief 把旧版 XML 的一个键值对写入对应字段
 * @param key 旧版键名
 * @param valueElement 旧版 <value> 节点
 */
void DAAppConfig::applyLegacyProp(const QString& key, const QDomElement& valueElement)
{
    const QString text = valueElement.text().trimmed();
    if (key == QLatin1String("ribbon-style")) {
        mUiSettings.ribbonStyle = text.toInt();
    } else if (key == QLatin1String("ribbon-theme")) {
        mUiSettings.ribbonTheme = text.toInt();
    } else if (key == QLatin1String("language")) {
        mUiSettings.language = text;
    } else if (key == QLatin1String("dock-tab-position")) {
        mUiSettings.dockTabPosition = tabPositionFromValue(QJsonValue(text), mUiSettings.dockTabPosition);
    } else if (key == QLatin1String("app-font-family")) {
        mUiSettings.fontFamily = text;
    } else if (key == QLatin1String("app-font-size")) {
        mUiSettings.fontPointSize = text.toDouble();
    } else if (key == QLatin1String("save-ui-state-on-close")) {
        mUiSettings.saveUiStateOnClose = xmlTextToBool(text, mUiSettings.saveUiStateOnClose);
    } else if (key == QLatin1String("undo-stack-limit")) {
        mUiSettings.undoStackLimit = text.toInt();
    } else if (key == QLatin1String("python-interpreter-path")) {
        mPythonSettings.interpreterPath = text;
    } else if (key == QLatin1String("python-extra-paths")) {
        QStringList paths;
        for (QDomElement r = valueElement.firstChildElement(QStringLiteral("r")); !r.isNull();
             r             = r.nextSiblingElement(QStringLiteral("r"))) {
            const QString p = r.text().trimmed();
            if (!p.isEmpty()) {
                paths << p;
            }
        }
        mPythonSettings.extraPaths = paths;
    } else if (key == QLatin1String("log-level")) {
        mLogSettings.level = static_cast< DALogLevel >(text.toInt());
    } else if (key == QLatin1String("log-queue-level")) {
        mLogSettings.queueLevel = static_cast< DALogLevel >(text.toInt());
    } else if (key == QLatin1String("log-output-stdout")) {
        mLogSettings.outputStdout = xmlTextToBool(text, mLogSettings.outputStdout);
    } else if (key == QLatin1String("log-rotation-mode")) {
        mLogSettings.rotationMode = static_cast< DALogRotationMode >(text.toInt());
    } else if (key == QLatin1String("log-max-size")) {
        mLogSettings.maxSizeBytes = text.toLongLong();
    } else if (key == QLatin1String("log-max-files")) {
        mLogSettings.maxFiles = text.toInt();
    } else if (key == QLatin1String("show-log-num")) {
        mLogSettings.showLogNum = text.toInt();
    } else if (key == QLatin1String("recent-files-max")) {
        mAdvancedSettings.recentFilesMax = text.toInt();
    } else if (key == QLatin1String("dump-retention-days")) {
        mAdvancedSettings.dumpRetentionDays = text.toInt();
    } else if (key == QLatin1String("show-splash")) {
        mAdvancedSettings.showSplash = xmlTextToBool(text, mAdvancedSettings.showSplash);
    } else if (key == QLatin1String("autosave-interval")) {
        mAdvancedSettings.autosaveIntervalMin = text.toInt();
    } else if (key == QLatin1String("workspace-dir")) {
        mScriptSettings.workspaceDir = text;
    } else if (key == QLatin1String("workspace-overwrite-policy")) {
        mScriptSettings.overwritePolicy = overwritePolicyFromValue(QJsonValue(text), mScriptSettings.overwritePolicy);
    } else if (key == QLatin1String("script-timeout")) {
        mScriptSettings.scriptTimeoutSec = text.toInt();
    } else if (key == QLatin1String("script-result-max-chars")) {
        mScriptSettings.scriptResultMaxChars = text.toInt();
    }
    // 其余键（含已删除的 workflow-timeout / plugin-extra-paths / node-script-paths）忽略
}

}  // namespace DA
