// DAAgentPermissionManager.cpp
#include "DAAgentPermissionManager.h"
#include "DADir.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>
#include <QSettings>
#include <QCoreApplication>
#include <QStandardPaths>
#include <utility>  // std::as_const（非 const 容器范围迭代防 COW）

namespace DA
{

// ===========================================================================
// 常量
// ===========================================================================

namespace {

const char* kModeYolo   = "yolo";
const char* kModeAuto   = "auto";
const char* kModeManual = "manual";

const char* kTierRead        = "read";
const char* kTierInappMutate = "inapp_mutate";
const char* kTierFileWrite   = "file_write";
const char* kTierCodeExec    = "code_exec";
const char* kTierUnknown     = "unknown";

const char* kActionAllow = "allow";
const char* kActionDeny  = "deny";
const char* kActionAsk   = "ask";

/// ini key（agent-config.ini agent/ 组，母文档 §7.2）
const char* kKeyPermissionMode        = "agent/permission_mode";
const char* kKeyApprovalTimeoutSec    = "agent/tool_approval_timeout_sec";
const char* kKeyJudgeModel            = "agent/judge_model";
const char* kKeyJudgeTimeoutSec       = "agent/judge_timeout_sec";
const char* kKeyManualBlockInappTools = "agent/manual_block_inapp_tools";

/// 20 个内置工具的风险分级表（母文档 §5）
const QHash< QString, QString >& builtinTierTable()
{
    static const QHash< QString, QString > table = {
        // read（6）
        {QStringLiteral("list_data"), QString::fromLatin1(kTierRead)},
        {QStringLiteral("get_data_info"), QString::fromLatin1(kTierRead)},
        {QStringLiteral("query_data"), QString::fromLatin1(kTierRead)},
        {QStringLiteral("get_column_stats"), QString::fromLatin1(kTierRead)},
        {QStringLiteral("list_figures"), QString::fromLatin1(kTierRead)},
        {QStringLiteral("read_file"), QString::fromLatin1(kTierRead)},
        // inapp_mutate（8 图表类）
        {QStringLiteral("create_chart"), QString::fromLatin1(kTierInappMutate)},
        {QStringLiteral("create_subplots"), QString::fromLatin1(kTierInappMutate)},
        {QStringLiteral("add_curve"), QString::fromLatin1(kTierInappMutate)},
        {QStringLiteral("update_curve_style"), QString::fromLatin1(kTierInappMutate)},
        {QStringLiteral("set_axis"), QString::fromLatin1(kTierInappMutate)},
        {QStringLiteral("set_chart_style"), QString::fromLatin1(kTierInappMutate)},
        {QStringLiteral("remove_chart_item"), QString::fromLatin1(kTierInappMutate)},
        {QStringLiteral("add_annotation"), QString::fromLatin1(kTierInappMutate)},
        // file_write（4）
        {QStringLiteral("write_file"), QString::fromLatin1(kTierFileWrite)},
        {QStringLiteral("export_data"), QString::fromLatin1(kTierFileWrite)},
        {QStringLiteral("save_report"), QString::fromLatin1(kTierFileWrite)},
        {QStringLiteral("save_chart_image"), QString::fromLatin1(kTierFileWrite)},
        // code_exec（2）
        {QStringLiteral("run_code"), QString::fromLatin1(kTierCodeExec)},
        {QStringLiteral("run_script"), QString::fromLatin1(kTierCodeExec)},
    };
    return table;
}

/// 路径类参数提取顺序（母文档 §6.1：file_path 优先，回退 path/output_path/report_path）
const char* kPathKeys[] = {"file_path", "path", "output_path", "report_path"};

/// 判断分级名是否合法
bool isValidTier(const QString& tier)
{
    return tier == QLatin1String(kTierRead) || tier == QLatin1String(kTierInappMutate)
           || tier == QLatin1String(kTierFileWrite) || tier == QLatin1String(kTierCodeExec)
           || tier == QLatin1String(kTierUnknown);
}

/// 打开 agent-config.ini（显式路径，与 DAAgentModule 各读写点一致）
QSettings openAgentIni()
{
    return QSettings(DA::DADir::getConfigPath() + "/agent-config.ini", QSettings::IniFormat);
}

} // namespace

// ===========================================================================
// PrivateData
// ===========================================================================

class DAAgentPermissionManager::PrivateData
{
    DA_DECLARE_PUBLIC(DAAgentPermissionManager)
public:
    explicit PrivateData(DAAgentPermissionManager* p);

    // 4 条系统目录硬 deny 种子（A4：全模式生效，加载强制回填，不可经设置删除）
    static QList< DAAgentPermissionRule > hardDenySeeds();
    // 工作区内 4 个写工具 allow 种子（§9.1）
    static QList< DAAgentPermissionRule > workspaceAllowSeeds();
    // 代码危险模式种子（§9.2）
    static QJsonObject seedCodePatterns();

    QList< DAAgentPermissionRule > mRules;
    QJsonObject mCodePatterns;               ///< {deny:[...], escalate:[...]}
    QJsonObject mTierOverrides;              ///< {tool: tier}
    QString mWorkspaceRoot;                  ///< 脚本工作区根（${workspace}），规范化
    QString mProjectDir;                     ///< 工程文件所在目录（${project}），规范化
    QHash< QString, QStringList > mSessionMemory;  ///< tool → 已批准路径前缀（规范化）
};

DAAgentPermissionManager::PrivateData::PrivateData(DAAgentPermissionManager* p) : q_ptr(p)
{
}

QList< DAAgentPermissionRule > DAAgentPermissionManager::PrivateData::hardDenySeeds()
{
    // 迁移自三处 isPathSafe 的同一黑名单（c:/windows/system32 被 c:/windows/** 覆盖）
    static const char* kScopes[] = {
        "c:/windows/**",
        "c:/program files/**",
        "c:/program files (x86)/**",
        "c:/programdata/**",
    };
    QList< DAAgentPermissionRule > seeds;
    for (const char* scope : kScopes) {
        DAAgentPermissionRule r;
        r.tool   = QStringLiteral("*");
        r.scope  = QString::fromLatin1(scope);
        r.action = QString::fromLatin1(kActionDeny);
        seeds.append(r);
    }
    return seeds;
}

QList< DAAgentPermissionRule > DAAgentPermissionManager::PrivateData::workspaceAllowSeeds()
{
    static const char* kTools[] = {"write_file", "export_data", "save_report", "save_chart_image"};
    QList< DAAgentPermissionRule > seeds;
    for (const char* tool : kTools) {
        DAAgentPermissionRule r;
        r.tool   = QString::fromLatin1(tool);
        r.scope  = QStringLiteral("${workspace}/**");
        r.action = QString::fromLatin1(kActionAllow);
        seeds.append(r);
    }
    return seeds;
}

QJsonObject DAAgentPermissionManager::PrivateData::seedCodePatterns()
{
    // §9.2 种子清单（Python 计划二消费）
    QJsonObject p;
    p[QStringLiteral("deny")] = QJsonArray{
        QStringLiteral("subprocess"),
        QStringLiteral("os\\.system"),
        QStringLiteral("os\\.popen"),
        QStringLiteral("shutil\\.rmtree"),
        QStringLiteral("os\\.remove\\b"),
        QStringLiteral("os\\.unlink"),
        QStringLiteral("os\\.rmdir"),
        QStringLiteral("ctypes"),
        QStringLiteral("pickle\\.loads"),
        QStringLiteral("sys\\.exit"),
    };
    p[QStringLiteral("escalate")] = QJsonArray{
        QStringLiteral("socket\\."),
        QStringLiteral("requests\\.(get|post|put|delete)"),
        QStringLiteral("urllib"),
        QStringLiteral("__import__"),
        QStringLiteral("importlib"),
        QStringLiteral("\\beval\\s*\\("),
        QStringLiteral("\\bexec\\s*\\("),
        QStringLiteral("open\\s*\\([^)]*['\"][wa]\\+?b?['\"]"),
        QStringLiteral("\\.write_"),
        QStringLiteral("shutil\\.(copy|move)"),
    };
    return p;
}

// ===========================================================================
// 静态常量访问
// ===========================================================================

/**
 * @brief 分级名 read
 */
QString DAAgentPermissionManager::tierRead()
{
    return QString::fromLatin1(kTierRead);
}

/**
 * @brief 分级名 inapp_mutate
 */
QString DAAgentPermissionManager::tierInappMutate()
{
    return QString::fromLatin1(kTierInappMutate);
}

/**
 * @brief 分级名 file_write
 */
QString DAAgentPermissionManager::tierFileWrite()
{
    return QString::fromLatin1(kTierFileWrite);
}

/**
 * @brief 分级名 code_exec
 */
QString DAAgentPermissionManager::tierCodeExec()
{
    return QString::fromLatin1(kTierCodeExec);
}

/**
 * @brief 分级名 unknown
 */
QString DAAgentPermissionManager::tierUnknown()
{
    return QString::fromLatin1(kTierUnknown);
}

/**
 * @brief 模式名 yolo
 */
QString DAAgentPermissionManager::modeYolo()
{
    return QString::fromLatin1(kModeYolo);
}

/**
 * @brief 模式名 auto
 */
QString DAAgentPermissionManager::modeAuto()
{
    return QString::fromLatin1(kModeAuto);
}

/**
 * @brief 模式名 manual
 */
QString DAAgentPermissionManager::modeManual()
{
    return QString::fromLatin1(kModeManual);
}

/**
 * @brief code_exec deny 固定脱敏文本（A11：不向 LLM 暴露命中规则）
 */
QString DAAgentPermissionManager::codeDenyMessage()
{
    return QStringLiteral("denied by safety policy: code rejected, try a safer approach");
}

/**
 * @brief 系统目录硬 deny 文本（与原工具侧 isPathSafe 文案一致，保基线）
 */
QString DAAgentPermissionManager::systemPathDenyMessage()
{
    return QStringLiteral("Access denied: path is in a system-protected directory");
}

// ===========================================================================
// ctor / dtor
// ===========================================================================

/**
 * @brief 构造函数
 */
DAAgentPermissionManager::DAAgentPermissionManager() : DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构函数
 */
DAAgentPermissionManager::~DAAgentPermissionManager()
{
}

// ===========================================================================
// 持久化
// ===========================================================================

/**
 * @brief 配置文件绝对路径（与 agent-config.ini 同目录）
 * @return agent-permissions.json 路径
 */
QString DAAgentPermissionManager::configFilePath() const
{
    return DA::DADir::getConfigPath() + "/agent-permissions.json";
}

/**
 * @brief 读取配置；文件缺失时播种并落盘，存在时解析 + 硬 deny 强制回填（A4）
 * @return 是否成功（文件缺失时播种成功亦返回 true）
 */
bool DAAgentPermissionManager::load()
{
    DA_D(d);
    const QString path = configFilePath();
    QFile f(path);
    if (!f.exists()) {
        ensureDefaultRules();
        return save();
    }
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "DAAgentPermissionManager: failed to open config for read:" << path;
        ensureDefaultRules();
        return false;
    }
    const QByteArray bytes = f.readAll();
    f.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "DAAgentPermissionManager: config parse failed:" << err.errorString()
                   << "- reseeding defaults";
        ensureDefaultRules();
        return save();
    }
    const QJsonObject root = doc.object();

    // rules
    d->mRules.clear();
    const QJsonArray rulesArr = root.value(QStringLiteral("rules")).toArray();
    for (const QJsonValue& v : rulesArr) {
        if (v.isObject()) {
            d->mRules.append(DAAgentPermissionRule::fromJson(v.toObject()));
        }
    }
    // code_patterns / tier_overrides（缺失时回填种子，保证 init 全量下发有内容）
    d->mCodePatterns = root.value(QStringLiteral("code_patterns")).toObject();
    if (d->mCodePatterns.isEmpty()) {
        d->mCodePatterns = PrivateData::seedCodePatterns();
    }
    d->mTierOverrides = root.value(QStringLiteral("tier_overrides")).toObject();

    // 硬 deny 强制回填（A4：不可经配置删除）
    const QList< DAAgentPermissionRule > hardSeeds = PrivateData::hardDenySeeds();
    bool changed = false;
    for (const DAAgentPermissionRule& seed : hardSeeds) {
        bool found = false;
        for (const DAAgentPermissionRule& r : std::as_const(d->mRules)) {
            if (r.equals(seed)) {
                found = true;
                break;
            }
        }
        if (!found) {
            d->mRules.append(seed);
            changed = true;
            qWarning() << "DAAgentPermissionManager: hard-deny rule force-appended:"
                       << seed.scope;
        }
    }
    if (changed) {
        save();
    }
    return true;
}

/**
 * @brief 落盘当前规则/危险模式/分级覆盖（原子写 tmp+rename，镜像 SessionStore 索引写法）
 * @return 是否成功
 */
bool DAAgentPermissionManager::save() const
{
    DA_DC(d);
    QJsonObject root;
    root[QStringLiteral("version")] = 2;
    QJsonArray rulesArr;
    for (const DAAgentPermissionRule& r : d->mRules) {
        rulesArr.append(r.toJson());
    }
    root[QStringLiteral("rules")]          = rulesArr;
    root[QStringLiteral("code_patterns")]  = d->mCodePatterns;
    root[QStringLiteral("tier_overrides")] = d->mTierOverrides;

    const QString path = configFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    const QString tmpPath = path + ".tmp";
    QFile tf(tmpPath);
    if (!tf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAgentPermissionManager: failed to open tmp file for write:" << tmpPath
                   << tf.errorString();
        return false;
    }
    tf.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    tf.close();
    if (QFile::exists(path)) {
        QFile::remove(path);
    }
    if (!tf.rename(path)) {
        qWarning() << "DAAgentPermissionManager: failed to rename tmp file:" << tmpPath
                   << tf.errorString();
        return false;
    }
    return true;
}

/**
 * @brief 播种默认规则（§9.1，仅文件缺失时由 load 调用；播种后归用户）
 */
void DAAgentPermissionManager::ensureDefaultRules()
{
    DA_D(d);
    QList< DAAgentPermissionRule > seeds = PrivateData::workspaceAllowSeeds();
    seeds.append(PrivateData::hardDenySeeds());
    d->mRules         = seeds;
    d->mCodePatterns  = PrivateData::seedCodePatterns();
    // tier_overrides 保持空（用户显式配置项）
}

// ===========================================================================
// 规则/危险模式/分级覆盖
// ===========================================================================

/**
 * @brief 当前规则列表（顺序敏感）
 * @return 规则列表
 */
QList< DAAgentPermissionRule > DAAgentPermissionManager::rules() const
{
    DA_DC(d);
    return d->mRules;
}

/**
 * @brief 整体替换规则
 * @param rules 新规则列表
 */
void DAAgentPermissionManager::setRules(const QList< DAAgentPermissionRule >& rules)
{
    DA_D(d);
    d->mRules = rules;
}

/**
 * @brief 代码危险模式 {deny:[...], escalate:[...]}
 * @return 危险模式对象
 */
QJsonObject DAAgentPermissionManager::codePatterns() const
{
    DA_DC(d);
    return d->mCodePatterns;
}

/**
 * @brief 设置代码危险模式
 * @param patterns 危险模式对象
 */
void DAAgentPermissionManager::setCodePatterns(const QJsonObject& patterns)
{
    DA_D(d);
    d->mCodePatterns = patterns;
}

/**
 * @brief 分级覆盖表 {tool: tier}
 * @return 覆盖表
 */
QJsonObject DAAgentPermissionManager::tierOverrides() const
{
    DA_DC(d);
    return d->mTierOverrides;
}

/**
 * @brief 设置分级覆盖表
 * @param overrides 覆盖表
 */
void DAAgentPermissionManager::setTierOverrides(const QJsonObject& overrides)
{
    DA_D(d);
    d->mTierOverrides = overrides;
}

// ===========================================================================
// 模式
// ===========================================================================

/**
 * @brief 当前模式（非法值回退 auto）
 * @return yolo / auto / manual
 */
QString DAAgentPermissionManager::mode() const
{
    QSettings s = openAgentIni();
    const QString m = s.value(QLatin1String(kKeyPermissionMode), QLatin1String(kModeAuto)).toString();
    if (m == QLatin1String(kModeYolo) || m == QLatin1String(kModeAuto) || m == QLatin1String(kModeManual)) {
        return m;
    }
    return QString::fromLatin1(kModeAuto);
}

/**
 * @brief 写入模式（仅接受 yolo/auto/manual，其余忽略）
 * @param mode 模式名
 */
void DAAgentPermissionManager::setMode(const QString& mode)
{
    if (mode != QLatin1String(kModeYolo) && mode != QLatin1String(kModeAuto)
        && mode != QLatin1String(kModeManual)) {
        qWarning() << "DAAgentPermissionManager::setMode: invalid mode ignored:" << mode;
        return;
    }
    QSettings s = openAgentIni();
    s.setValue(QLatin1String(kKeyPermissionMode), mode);
}

// ===========================================================================
// 工具分级
// ===========================================================================

/**
 * @brief 工具分级：tier_overrides → 内置表 → 参数约定回退（含路径参数→file_write，
 *        否则→unknown，A3 [v2.1]）
 * @param tool 工具名
 * @param params 工具参数
 * @return 分级名
 */
QString DAAgentPermissionManager::tierOf(const QString& tool, const QJsonObject& params) const
{
    DA_DC(d);
    // 1. tier_overrides 最高优先（用户显式配置）
    const QJsonValue ov = d->mTierOverrides.value(tool);
    if (ov.isString() && isValidTier(ov.toString())) {
        return ov.toString();
    }
    // 2. 内置表（20 个已知工具）
    const auto& table = builtinTierTable();
    const auto it     = table.constFind(tool);
    if (it != table.constEnd()) {
        return it.value();
    }
    // 3. 参数约定回退（插件工具）
    for (const char* key : kPathKeys) {
        if (params.contains(QLatin1String(key))) {
            return QString::fromLatin1(kTierFileWrite);
        }
    }
    return QString::fromLatin1(kTierUnknown);
}

/**
 * @brief gated_tools：文件写入 + 代码执行全量（Python 长超时清单，A9 与模式解耦）
 * @return 工具名列表
 */
QStringList DAAgentPermissionManager::gatedTools()
{
    return QStringList{
        QStringLiteral("write_file"),  QStringLiteral("export_data"),
        QStringLiteral("save_report"), QStringLiteral("save_chart_image"),
        QStringLiteral("run_code"),    QStringLiteral("run_script"),
    };
}

// ===========================================================================
// 路径提取与变量
// ===========================================================================

/**
 * @brief 设置脚本工作区根目录（${workspace}）
 * @param dir 目录路径（空=未保存工程/启动无工程）
 */
void DAAgentPermissionManager::setWorkspaceRoot(const QString& dir)
{
    DA_D(d);
    d->mWorkspaceRoot = DAAgentPermissionRule::normalizePath(dir);
}

/**
 * @brief 当前脚本工作区根目录
 * @return 规范化路径，未注入为空
 */
QString DAAgentPermissionManager::workspaceRoot() const
{
    DA_DC(d);
    return d->mWorkspaceRoot;
}

/**
 * @brief 设置工程文件所在目录（${project}）
 * @param dir 目录路径
 */
void DAAgentPermissionManager::setProjectDir(const QString& dir)
{
    DA_D(d);
    d->mProjectDir = DAAgentPermissionRule::normalizePath(dir);
}

/**
 * @brief 当前工程目录
 * @return 规范化路径，无工程为空
 */
QString DAAgentPermissionManager::projectDir() const
{
    DA_DC(d);
    return d->mProjectDir;
}

/**
 * @brief 全部 ${var} 变量当前值（workspace/project/data/exe/home）
 * @return 变量名→值
 */
QHash< QString, QString > DAAgentPermissionManager::variables() const
{
    DA_DC(d);
    QHash< QString, QString > vars;
    vars[QStringLiteral("workspace")] = d->mWorkspaceRoot;
    vars[QStringLiteral("project")]   = d->mProjectDir;
    vars[QStringLiteral("data")]      = DAAgentPermissionRule::normalizePath(DA::DADir::getAppDataPath());
    vars[QStringLiteral("exe")]       = DAAgentPermissionRule::normalizePath(QCoreApplication::applicationDirPath());
    vars[QStringLiteral("home")]      = DAAgentPermissionRule::normalizePath(QDir::homePath());
    return vars;
}

/**
 * @brief 提取并规范化工具参数中的路径（母文档 §6.1 约定）
 *
 * file_path 优先，回退 path/output_path/report_path；run_script 的相对 path
 * 先按工作区根解析（A8）；相对路径按当前目录绝对化；无法解析返回空串。
 * @param tool 工具名
 * @param params 工具参数
 * @return 规范化绝对路径，无路径参数返回空串
 */
QString DAAgentPermissionManager::resolveToolPath(const QString& tool, const QJsonObject& params) const
{
    DA_DC(d);
    QString raw;
    for (const char* key : kPathKeys) {
        const QJsonValue v = params.value(QLatin1String(key));
        if (v.isString() && !v.toString().trimmed().isEmpty()) {
            raw = v.toString().trimmed();
            break;
        }
    }
    if (raw.isEmpty()) {
        return QString();
    }
    // run_script 的 path 是工作区相对路径（工具侧约定），先按 ${workspace} 解析
    if (tool == QLatin1String("run_script") && QDir::isRelativePath(raw) && !d->mWorkspaceRoot.isEmpty()) {
        raw = d->mWorkspaceRoot + QLatin1Char('/') + raw;
    }
    QString p = DAAgentPermissionRule::normalizePath(raw);
    if (p.isEmpty()) {
        return QString();
    }
    // 相对路径绝对化（按当前工作目录）
    if (QDir::isRelativePath(p)) {
        p = DAAgentPermissionRule::normalizePath(QDir::current().absoluteFilePath(p));
    }
    return p;
}

// ===========================================================================
// 会话记忆
// ===========================================================================

/**
 * @brief 记录批准的路径前缀（A5：调用方保证仅 file_write 生效）
 * @param tool 工具名
 * @param scopeKey 规范化目录前缀
 */
void DAAgentPermissionManager::rememberSession(const QString& tool, const QString& scopeKey)
{
    DA_D(d);
    if (tool.isEmpty() || scopeKey.isEmpty()) {
        return;
    }
    QStringList& prefixes = d->mSessionMemory[tool];
    if (!prefixes.contains(scopeKey)) {
        prefixes.append(scopeKey);
    }
}

/**
 * @brief 判断路径是否命中本会话已批准的前缀
 * @param tool 工具名
 * @param normalizedAbsPath 规范化绝对路径
 * @return 是否已批准
 */
bool DAAgentPermissionManager::isRemembered(const QString& tool, const QString& normalizedAbsPath) const
{
    DA_DC(d);
    const auto it = d->mSessionMemory.constFind(tool);
    if (it == d->mSessionMemory.constEnd() || normalizedAbsPath.isEmpty()) {
        return false;
    }
    for (const QString& prefix : it.value()) {
        if (normalizedAbsPath.startsWith(prefix, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 清空会话记忆（切换会话/进程退出/崩溃恢复时调用）
 */
void DAAgentPermissionManager::clearSessionMemory()
{
    DA_D(d);
    d->mSessionMemory.clear();
}

/**
 * @brief 从工具参数推导记忆前缀（规范化父目录 + '/'）
 * @param tool 工具名
 * @param params 工具参数
 * @return 前缀，无路径参数返回空串
 */
QString DAAgentPermissionManager::sessionScopeKey(const QString& tool, const QJsonObject& params) const
{
    const QString p = resolveToolPath(tool, params);
    if (p.isEmpty()) {
        return QString();
    }
    QString dir = DAAgentPermissionRule::normalizePath(QFileInfo(p).absolutePath());
    if (dir.isEmpty()) {
        return QString();
    }
    if (!dir.endsWith(QLatin1Char('/'))) {
        dir += QLatin1Char('/');
    }
    return dir;
}

// ===========================================================================
// ini 派生配置
// ===========================================================================

/**
 * @brief 判官是否已配置（agent/judge_model 非空，D1 兜底判据）
 * @return 是否配置
 */
bool DAAgentPermissionManager::judgeConfigured() const
{
    QSettings s = openAgentIni();
    return !s.value(QLatin1String(kKeyJudgeModel)).toString().trimmed().isEmpty();
}

/**
 * @brief 判官模型名（可空=未配置）
 * @return 模型名
 */
QString DAAgentPermissionManager::judgeModel() const
{
    QSettings s = openAgentIni();
    return s.value(QLatin1String(kKeyJudgeModel)).toString().trimmed();
}

/**
 * @brief 判官单次调用超时秒（默认 30）
 * @return 秒数
 */
int DAAgentPermissionManager::judgeTimeoutSec() const
{
    QSettings s = openAgentIni();
    return qMax(1, s.value(QLatin1String(kKeyJudgeTimeoutSec), 30).toInt());
}

/**
 * @brief gated_tools 的 RPC 长超时秒（默认 600，A9）
 * @return 秒数
 */
int DAAgentPermissionManager::toolApprovalTimeoutSec() const
{
    QSettings s = openAgentIni();
    return qMax(1, s.value(QLatin1String(kKeyApprovalTimeoutSec), 600).toInt());
}

/**
 * @brief manual 模式是否拦截应用内修改工具（默认 false，D2）
 * @return 是否拦截
 */
bool DAAgentPermissionManager::manualBlockInappTools() const
{
    QSettings s = openAgentIni();
    return s.value(QLatin1String(kKeyManualBlockInappTools), false).toBool();
}

// ===========================================================================
// 核心决策
// ===========================================================================

/**
 * @brief 路径策略评估：按规则顺序首条命中返回其动作，无命中返回 Ask（D3 区外询问兜底）
 * @param tool 工具名
 * @param normalizedAbsPath 规范化绝对路径
 * @param reason 输出参数：deny 时的 error 文本（可空）
 * @return Allow / Deny / Ask
 */
DAAgentPermissionManager::Action
DAAgentPermissionManager::evaluatePath(const QString& tool, const QString& normalizedAbsPath, QString* reason) const
{
    DA_DC(d);
    const QHash< QString, QString > vars = variables();
    for (const DAAgentPermissionRule& r : d->mRules) {
        if (!r.matchesTool(tool)) {
            continue;
        }
        if (!r.matchesPath(normalizedAbsPath, vars)) {
            continue;
        }
        const QString act = r.action.toLower();
        if (act == QLatin1String(kActionAllow)) {
            return Allow;
        }
        if (act == QLatin1String(kActionDeny)) {
            if (reason) {
                *reason = QStringLiteral("Access denied: path denied by permission policy (%1)").arg(r.scope);
            }
            return Deny;
        }
        return Ask;
    }
    return Ask;  // 无匹配 → 区外询问（D3）
}

/**
 * @brief 核心决策：按模式×分级×安全裁决产出 Allow/Deny/Ask（母文档 §4 矩阵 [v2.1]）
 *
 * 顺序：硬 deny（全模式）→ yolo 放行 → 会话记忆（仅 file_write，优先于 ask）
 * → auto 分级处理（code_exec 判官未配置时 allow/uncertain/缺失一律 ask，D1）
 * → manual 分级处理（unknown 一律 ask，A3）。
 * @param tool 工具名
 * @param params 工具参数
 * @param safety Python 侧安全裁决 {verdict, reason, source}（可空，计划二生产）
 * @return 决策结果
 */
DAAgentPermissionManager::Decision
DAAgentPermissionManager::decide(const QString& tool, const QJsonObject& params, const QJsonObject& safety) const
{
    const QString tier = tierOf(tool, params);
    const QString m    = mode();
    const QString path = resolveToolPath(tool, params);

    // 0. 硬 deny：全模式、全分级之前先行求值（母文档 §4 [v2.1]）——
    //    a) 系统目录种子（A4：即使被配置篡改也由 load 强制回填，此处按种子直查兜底）
    //    b) tool=="*" 且 action==deny 的用户规则（通配工具=用户显式全局封禁）
    if (!path.isEmpty()) {
        const QHash< QString, QString > vars = variables();
        const QList< DAAgentPermissionRule > hardSeeds = PrivateData::hardDenySeeds();
        for (const DAAgentPermissionRule& seed : hardSeeds) {
            if (seed.matchesPath(path, vars)) {
                Decision dec;
                dec.action = Deny;
                dec.tier   = tier;
                dec.reason = systemPathDenyMessage();
                return dec;
            }
        }
        DA_DC(d);
        for (const DAAgentPermissionRule& r : d->mRules) {
            if (r.tool != QLatin1String("*") || r.action.compare(QLatin1String(kActionDeny), Qt::CaseInsensitive) != 0) {
                continue;
            }
            if (r.matchesPath(path, vars)) {
                Decision dec;
                dec.action = Deny;
                dec.tier   = tier;
                dec.reason = systemPathDenyMessage();
                return dec;
            }
        }
    }

    // 1. yolo：除硬 deny 外全部放行
    if (m == QLatin1String(kModeYolo)) {
        Decision dec;
        dec.action = Allow;
        dec.tier   = tier;
        return dec;
    }

    // 2. 会话记忆：仅 file_write 生效，等价于用户已批准，优先于 ask（A5 [v2.1]）
    if (tier == QLatin1String(kTierFileWrite) && !path.isEmpty() && isRemembered(tool, path)) {
        Decision dec;
        dec.action = Allow;
        dec.tier   = tier;
        return dec;
    }

    Decision dec;
    dec.tier = tier;

    if (m == QLatin1String(kModeAuto)) {
        if (tier == QLatin1String(kTierRead) || tier == QLatin1String(kTierInappMutate)) {
            dec.action = Allow;
            return dec;
        }
        if (tier == QLatin1String(kTierFileWrite)) {
            if (path.isEmpty()) {
                dec.action = Ask;  // 无路径参数无法评估 → 保守询问
                return dec;
            }
            QString reason;
            const Action a = evaluatePath(tool, path, &reason);
            dec.action     = a;
            if (a == Deny) {
                dec.reason = reason;
            }
            return dec;
        }
        if (tier == QLatin1String(kTierCodeExec)) {
            const QString verdict = safety.value(QStringLiteral("verdict")).toString().toLower();
            // deny 裁决全模式优先拒绝（即使判官未配置也消费静态规则产出，契约 2）
            if (verdict == QLatin1String("deny")) {
                dec.action = Deny;
                dec.reason = codeDenyMessage();  // A11 脱敏
                return dec;
            }
            // 判官未配置：allow/uncertain/缺失一律降级 ask（D1 [v2.1]，避免静默放行）
            if (!judgeConfigured()) {
                dec.action = Ask;
                return dec;
            }
            if (verdict == QLatin1String("allow")) {
                dec.action = Allow;
                return dec;
            }
            dec.action = Ask;  // uncertain / 缺失
            return dec;
        }
        // unknown（A3 [v2.1]：保守 ask）
        dec.action = Ask;
        return dec;
    }

    // 3. manual
    if (tier == QLatin1String(kTierRead)) {
        dec.action = Allow;
        return dec;
    }
    if (tier == QLatin1String(kTierInappMutate)) {
        dec.action = manualBlockInappTools() ? Ask : Allow;  // D2
        return dec;
    }
    // file_write / code_exec / unknown 一律 ask（硬 deny 已在步骤 0 拦截）
    dec.action = Ask;
    return dec;
}

// ===========================================================================
// 设置页 round-trip
// ===========================================================================

/**
 * @brief 汇总全部权限配置（模式/超时/开关/判官 + 规则/危险模式/分级覆盖）
 * @return 配置 JSON
 */
QJsonObject DAAgentPermissionManager::getConfig() const
{
    DA_DC(d);
    QJsonObject c;
    c[QStringLiteral("mode")]                     = mode();
    c[QStringLiteral("tool_approval_timeout_sec")] = toolApprovalTimeoutSec();
    c[QStringLiteral("manual_block_inapp_tools")]  = manualBlockInappTools();
    c[QStringLiteral("judge_model")]               = judgeModel();
    c[QStringLiteral("judge_timeout_sec")]         = judgeTimeoutSec();
    QJsonArray rulesArr;
    for (const DAAgentPermissionRule& r : d->mRules) {
        rulesArr.append(r.toJson());
    }
    c[QStringLiteral("rules")]          = rulesArr;
    c[QStringLiteral("code_patterns")]  = d->mCodePatterns;
    c[QStringLiteral("tier_overrides")] = d->mTierOverrides;
    return c;
}

/**
 * @brief 按 contains 守卫写入配置（镜像 setLLMConfig 风格：key 存在即写）
 * @param config 配置 JSON（setConfig 后规则/危险模式/覆盖表变更会落盘）
 */
void DAAgentPermissionManager::setConfig(const QJsonObject& config)
{
    DA_D(d);
    bool fileChanged = false;

    if (config.contains(QStringLiteral("mode"))) {
        setMode(config.value(QStringLiteral("mode")).toString());
    }
    QSettings s = openAgentIni();
    if (config.contains(QStringLiteral("tool_approval_timeout_sec"))) {
        s.setValue(QLatin1String(kKeyApprovalTimeoutSec),
                   qMax(1, config.value(QStringLiteral("tool_approval_timeout_sec")).toInt(600)));
    }
    if (config.contains(QStringLiteral("manual_block_inapp_tools"))) {
        s.setValue(QLatin1String(kKeyManualBlockInappTools),
                   config.value(QStringLiteral("manual_block_inapp_tools")).toBool(false));
    }
    if (config.contains(QStringLiteral("judge_model"))) {
        s.setValue(QLatin1String(kKeyJudgeModel),
                   config.value(QStringLiteral("judge_model")).toString().trimmed());
    }
    if (config.contains(QStringLiteral("judge_timeout_sec"))) {
        s.setValue(QLatin1String(kKeyJudgeTimeoutSec),
                   qMax(1, config.value(QStringLiteral("judge_timeout_sec")).toInt(30)));
    }
    if (config.contains(QStringLiteral("rules"))) {
        QList< DAAgentPermissionRule > rules;
        const QJsonArray arr = config.value(QStringLiteral("rules")).toArray();
        for (const QJsonValue& v : arr) {
            if (v.isObject()) {
                rules.append(DAAgentPermissionRule::fromJson(v.toObject()));
            }
        }
        d->mRules    = rules;
        fileChanged = true;
        // 硬 deny 强制回填（A4：设置页不允许删除）
        const QList< DAAgentPermissionRule > hardSeeds = PrivateData::hardDenySeeds();
        for (const DAAgentPermissionRule& seed : hardSeeds) {
            bool found = false;
            for (const DAAgentPermissionRule& r : std::as_const(d->mRules)) {
                if (r.equals(seed)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                d->mRules.append(seed);
            }
        }
    }
    if (config.contains(QStringLiteral("code_patterns"))) {
        d->mCodePatterns = config.value(QStringLiteral("code_patterns")).toObject();
        fileChanged      = true;
    }
    if (config.contains(QStringLiteral("tier_overrides"))) {
        d->mTierOverrides = config.value(QStringLiteral("tier_overrides")).toObject();
        fileChanged       = true;
    }
    if (fileChanged) {
        save();
    }
}

} // namespace DA
