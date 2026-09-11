// DAAgentPermissionManager.cpp
#include "DAAgentPermissionManager.h"
#include "DAAgentConfig.h"
#include "DADir.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUuid>
#include <utility>  // std::as_const（非 const 容器范围迭代防 COW）

namespace DA
{

// ===========================================================================
// 常量
// ===========================================================================

namespace {

/// 原子写 tmp 文件后缀（审计 L17）：进程号 + 随机短码——固定 ".tmp" 名在
/// 同机多开 data-workbench 共享 appData 时会互相截断/丢更新
QString uniqueTmpSuffix()
{
    return QStringLiteral(".%1.%2.tmp")
        .arg(QCoreApplication::applicationPid())
        .arg(QString::fromLatin1(QUuid::createUuid().toString(QUuid::Id128).left(8).toLatin1()));
}

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

// ---- tierOverrides ↔ JSON 对象（QHash 无自带投影，此处集中转换） ----

QHash< QString, QString > tierOverridesFromJson(const QJsonObject& o)
{
    QHash< QString, QString > out;
    for (auto it = o.constBegin(); it != o.constEnd(); ++it) {
        if (it.value().isString()) {
            out.insert(it.key(), it.value().toString());
        }
    }
    return out;
}

QJsonObject tierOverridesToJson(const QHash< QString, QString >& overrides)
{
    QJsonObject o;
    for (auto it = overrides.constBegin(); it != overrides.constEnd(); ++it) {
        o[it.key()] = it.value();
    }
    return o;
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
    static DAAgentCodePatterns seedCodePatterns();

    QList< DAAgentPermissionRule > mRules;
    DAAgentCodePatterns mCodePatterns;                   ///< {deny:[...], escalate:[...]}
    QHash< QString, QString > mTierOverrides;            ///< {tool: tier}
    QString mWorkspaceRoot;                  ///< 脚本工作区根（${workspace}），规范化
    QString mProjectDir;                     ///< 工程文件所在目录（${project}），规范化
    // 会话记忆两级分桶（决策点 1 方案 b）：sessionId → (tool → 已批准路径前缀集)。
    // 全局单一键空间曾使"本会话记住"事实上跨会话存活（欠清理），且任一桥退出
    // 全局清空又误伤其它会话（过度清除）——按会话分桶两面同时根治
    QHash< QString, QHash< QString, QStringList > > mSessionMemory;

    // 会话上下文（审计问题 25）：attachBridge 时捕获的 workspace/project，
    // ${workspace}/${project} 变量按会话解析，工程切换不使后台会话判定漂移
    struct SessionContext {
        QString workspaceRoot;  ///< 该会话绑定的脚本工作区根（空=回退全局）
        QString projectDir;     ///< 该会话绑定的工程目录（空=回退全局）
    };
    QHash< QString, SessionContext > mSessionContexts;
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

DAAgentCodePatterns DAAgentPermissionManager::PrivateData::seedCodePatterns()
{
    // §9.2 种子清单（Python 计划二消费）
    DAAgentCodePatterns p;
    p.deny = QStringList{
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
    p.escalate = QStringList{
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
 * @param config 权限标量的共享配置模型（agent-config.json permission 分组；
 *               空指针时标量取默认值，供独立测试构造）
 */
DAAgentPermissionManager::DAAgentPermissionManager(DAAgentConfig* config)
    : DA_PIMPL_CONSTRUCT, mConfig(config)
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
    d->mCodePatterns = DAAgentCodePatterns::fromJson(root.value(QStringLiteral("code_patterns")).toObject());
    if (d->mCodePatterns.deny.isEmpty() && d->mCodePatterns.escalate.isEmpty()) {
        d->mCodePatterns = PrivateData::seedCodePatterns();
    }
    d->mTierOverrides = tierOverridesFromJson(root.value(QStringLiteral("tier_overrides")).toObject());

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
    root[QStringLiteral("code_patterns")]  = d->mCodePatterns.toJson();
    root[QStringLiteral("tier_overrides")] = tierOverridesToJson(d->mTierOverrides);

    const QString path = configFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    // L17：tmp 名带进程号+随机短码，同机多实例共享 appData 不互相截断
    const QString tmpPath = path + uniqueTmpSuffix();
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
 * @return 危险模式结构体
 */
DAAgentCodePatterns DAAgentPermissionManager::codePatterns() const
{
    DA_DC(d);
    return d->mCodePatterns;
}

/**
 * @brief 设置代码危险模式
 * @param patterns 危险模式结构体
 */
void DAAgentPermissionManager::setCodePatterns(const DAAgentCodePatterns& patterns)
{
    DA_D(d);
    d->mCodePatterns = patterns;
}

/**
 * @brief 分级覆盖表 {tool: tier}
 * @return 覆盖表
 */
QHash< QString, QString > DAAgentPermissionManager::tierOverrides() const
{
    DA_DC(d);
    return d->mTierOverrides;
}

/**
 * @brief 设置分级覆盖表
 * @param overrides 覆盖表
 */
void DAAgentPermissionManager::setTierOverrides(const QHash< QString, QString >& overrides)
{
    DA_D(d);
    d->mTierOverrides = overrides;
}

// ===========================================================================
// 模式
// ===========================================================================

/**
 * @brief 当前模式（未配置默认 yolo 全自动，非法值回退 yolo）
 * @return yolo / auto / manual
 */
QString DAAgentPermissionManager::mode() const
{
    // 经共享配置模型读取（未注入时默认 yolo）
    return mConfig ? mConfig->permissionMode() : QString::fromLatin1(kModeYolo);
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
    if (mConfig) {
        mConfig->setPermissionMode(mode);
    }
}

/**
 * @brief 模式是否由用户显式写入过（配置含 permission.mode 键）
 *
 * 未显式设置时 mode() 返回默认值 yolo；A13 启动确认卡仅对显式设置的
 * yolo 弹出——默认值静默进入全自动，不视为用户的危险态选择。
 * @return true=配置中存在模式键（曾显式设置）
 */
bool DAAgentPermissionManager::modeExplicitlySet() const
{
    return mConfig ? mConfig->permissionModeSet() : false;
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
 * @brief 注入会话上下文（attachBridge 时以当时全局值捕获，审计问题 25）
 * @param sessionId 会话 ID（空则忽略）
 * @param workspaceRoot 该会话绑定的脚本工作区根（规范化；空=回退全局）
 * @param projectDir 该会话绑定的工程目录（规范化；空=回退全局）
 */
void DAAgentPermissionManager::setSessionContext(const QString& sessionId, const QString& workspaceRoot, const QString& projectDir)
{
    DA_D(d);
    if (sessionId.isEmpty()) {
        return;
    }
    PrivateData::SessionContext ctx;
    ctx.workspaceRoot = DAAgentPermissionRule::normalizePath(workspaceRoot);
    ctx.projectDir    = DAAgentPermissionRule::normalizePath(projectDir);
    d->mSessionContexts.insert(sessionId, ctx);
}

/**
 * @brief 清除会话上下文（桥退役/会话删除）
 * @param sessionId 会话 ID
 */
void DAAgentPermissionManager::clearSessionContext(const QString& sessionId)
{
    DA_D(d);
    d->mSessionContexts.remove(sessionId);
}

/**
 * @brief 该会话视角的全部变量值（会话上下文覆盖全局默认）
 * @param sessionId 会话 ID（空或无上下文时等价全局 variables()）
 * @return 变量名→值
 */
QHash< QString, QString > DAAgentPermissionManager::variables(const QString& sessionId) const
{
    DA_DC(d);
    QHash< QString, QString > vars = variables();
    if (sessionId.isEmpty()) {
        return vars;
    }
    const auto it = d->mSessionContexts.constFind(sessionId);
    if (it == d->mSessionContexts.constEnd()) {
        return vars;
    }
    if (!it->workspaceRoot.isEmpty()) {
        vars[QStringLiteral("workspace")] = it->workspaceRoot;
    }
    if (!it->projectDir.isEmpty()) {
        vars[QStringLiteral("project")] = it->projectDir;
    }
    return vars;
}

/**
 * @brief 该会话视角的工作区根（Bridge buildPermissionConfig 下发 Python 判官用）
 * @param sessionId 会话 ID（空或无上下文时回退全局）
 * @return 规范化工作区根
 */
QString DAAgentPermissionManager::workspaceRootForSession(const QString& sessionId) const
{
    DA_DC(d);
    if (!sessionId.isEmpty()) {
        const auto it = d->mSessionContexts.constFind(sessionId);
        if (it != d->mSessionContexts.constEnd() && !it->workspaceRoot.isEmpty()) {
            return it->workspaceRoot;
        }
    }
    return d->mWorkspaceRoot;
}

/**
 * @brief 提取并规范化工具参数中的路径（母文档 §6.1 约定）
 *
 * file_path 优先，回退 path/output_path/report_path；run_script 的相对 path
 * 先按工作区根解析（A8）；相对路径按当前目录绝对化；无法解析返回空串。
 * @param sessionId 会话 ID（run_script 相对路径按该会话工作区解析，问题 25）
 * @param tool 工具名
 * @param params 工具参数
 * @return 规范化绝对路径，无路径参数返回空串
 */
QString DAAgentPermissionManager::resolveToolPath(const QString& sessionId, const QString& tool, const QJsonObject& params) const
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
    // run_script 的 path 是工作区相对路径（工具侧约定），先按该会话的
    // ${workspace} 解析（无会话上下文回退全局，审计问题 25：工程切换不使
    // 后台会话的相对路径判定漂移到新工程）
    const QString ws = workspaceRootForSession(sessionId);
    if (tool == QLatin1String("run_script") && QDir::isRelativePath(raw) && !ws.isEmpty()) {
        raw = ws + QLatin1Char('/') + raw;
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
 * @brief 记录批准的路径前缀（A5：调用方保证仅 file_write 生效；按会话分桶）
 * @param sessionId 桥所属会话（空则拒绝——无归属的批准不记忆，保守方向）
 * @param tool 工具名
 * @param scopeKey 规范化目录前缀
 */
void DAAgentPermissionManager::rememberSession(const QString& sessionId, const QString& tool, const QString& scopeKey)
{
    DA_D(d);
    if (sessionId.isEmpty() || tool.isEmpty() || scopeKey.isEmpty()) {
        return;
    }
    QStringList& prefixes = d->mSessionMemory[sessionId][tool];
    if (!prefixes.contains(scopeKey)) {
        prefixes.append(scopeKey);
    }
}

/**
 * @brief 判断路径是否命中该会话已批准的前缀
 * @param sessionId 桥所属会话
 * @param tool 工具名
 * @param normalizedAbsPath 规范化绝对路径
 * @return 是否已批准
 */
bool DAAgentPermissionManager::isRemembered(const QString& sessionId, const QString& tool, const QString& normalizedAbsPath) const
{
    DA_DC(d);
    if (sessionId.isEmpty() || normalizedAbsPath.isEmpty()) {
        return false;
    }
    const auto sessionIt = d->mSessionMemory.constFind(sessionId);
    if (sessionIt == d->mSessionMemory.constEnd()) {
        return false;
    }
    const auto it = sessionIt->constFind(tool);
    if (it == sessionIt->constEnd()) {
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
 * @brief 销毁指定会话的记忆（会话删除/桥退役/该会话进程退出时调用）
 * @param sessionId 会话 ID（空则忽略）
 */
void DAAgentPermissionManager::clearSessionMemory(const QString& sessionId)
{
    DA_D(d);
    if (sessionId.isEmpty()) {
        return;
    }
    d->mSessionMemory.remove(sessionId);
}

/**
 * @brief 从工具参数推导记忆前缀（规范化父目录 + '/'）
 * @param sessionId 会话 ID（run_script 相对路径按会话工作区解析）
 * @param tool 工具名
 * @param params 工具参数
 * @return 前缀，无路径参数返回空串
 */
QString DAAgentPermissionManager::sessionScopeKey(const QString& sessionId, const QString& tool, const QJsonObject& params) const
{
    const QString p = resolveToolPath(sessionId, tool, params);
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
// 配置模型派生标量（agent-config.json permission 分组，经注入的 DAAgentConfig）
// ===========================================================================

/**
 * @brief 判官是否已配置（judge_model 非空，D1 兜底判据）
 * @return 是否配置
 */
bool DAAgentPermissionManager::judgeConfigured() const
{
    return !judgeModel().isEmpty();
}

/**
 * @brief 判官模型名（可空=未配置）
 * @return 模型名
 */
QString DAAgentPermissionManager::judgeModel() const
{
    return mConfig ? mConfig->judgeModel() : QString();
}

/**
 * @brief 判官单次调用超时秒（默认 30）
 * @return 秒数
 */
int DAAgentPermissionManager::judgeTimeoutSec() const
{
    return mConfig ? mConfig->judgeTimeoutSec() : 30;
}

/**
 * @brief gated_tools 批准后的执行超时秒（默认 600，A9）
 *
 * 用户审批等待不计入：权限门进入 Ask 时 Bridge 下发 approval_pending，
 * Python 侧挂起计时；批准后下发 tool_exec_start，从执行起点开始计时。
 * @return 秒数
 */
int DAAgentPermissionManager::toolApprovalTimeoutSec() const
{
    return mConfig ? mConfig->toolApprovalTimeoutSec() : 600;
}

/**
 * @brief manual 模式是否拦截应用内修改工具（默认 false，D2）
 * @return 是否拦截
 */
bool DAAgentPermissionManager::manualBlockInappTools() const
{
    return mConfig ? mConfig->manualBlockInappTools() : false;
}

// ===========================================================================
// 核心决策
// ===========================================================================

/**
 * @brief 路径策略评估：按规则顺序首条命中返回其动作，无命中返回 Ask（D3 区外询问兜底）
 * @param sessionId 会话 ID（${workspace}/${project} 按会话上下文解析，问题 25）
 * @param tool 工具名
 * @param normalizedAbsPath 规范化绝对路径
 * @param reason 输出参数：deny 时的 error 文本（可空）
 * @return Allow / Deny / Ask
 */
DAAgentPermissionManager::Action
DAAgentPermissionManager::evaluatePath(const QString& sessionId, const QString& tool, const QString& normalizedAbsPath, QString* reason) const
{
    DA_DC(d);
    const QHash< QString, QString > vars = variables(sessionId);
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
 * @param sessionId 桥所属会话（决策点 1 方案 b：记忆按会话查询；空则无记忆）
 * @param tool 工具名
 * @param params 工具参数
 * @param safety Python 侧安全裁决 {verdict, reason, source}（可空，计划二生产）
 * @return 决策结果
 */
DAAgentPermissionManager::Decision
DAAgentPermissionManager::decide(const QString& sessionId, const QString& tool, const QJsonObject& params, const QJsonObject& safety) const
{
    const QString tier = tierOf(tool, params);
    const QString m    = mode();
    const QString path = resolveToolPath(sessionId, tool, params);

    // 0. 硬 deny：全模式、全分级之前先行求值（母文档 §4 [v2.1]）——
    //    a) 系统目录种子（A4：即使被配置篡改也由 load 强制回填，此处按种子直查兜底）
    //    b) tool=="*" 且 action==deny 的用户规则（通配工具=用户显式全局封禁）
    if (!path.isEmpty()) {
        const QHash< QString, QString > vars = variables(sessionId);
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

    // 2. 会话记忆：仅 file_write 生效，等价于用户已批准，优先于 ask（A5 [v2.1]）；
    //    按桥所属会话查询（决策点 1 方案 b），跨会话不可见
    if (tier == QLatin1String(kTierFileWrite) && !path.isEmpty() && isRemembered(sessionId, tool, path)) {
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
            const Action a = evaluatePath(sessionId, tool, path, &reason);
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
 * @return 权限配置结构体（标量总是 engage，值类型部分为当前引擎状态）
 */
DAAgentPermissionConfig DAAgentPermissionManager::getConfig() const
{
    DA_DC(d);
    DAAgentPermissionConfig c;
    c.setMode(mode());
    c.setToolApprovalTimeoutSec(toolApprovalTimeoutSec());
    c.setManualBlockInappTools(manualBlockInappTools());
    c.setJudgeModel(judgeModel());
    c.setJudgeTimeoutSec(judgeTimeoutSec());
    c.setRules(d->mRules);
    c.setCodePatterns(d->mCodePatterns);
    c.setTierOverrides(d->mTierOverrides);
    return c;
}

/**
 * @brief 按稀疏守卫写入配置（标量未 engage 不受影响；规则/危险模式/覆盖表整体替换）
 * @param config 权限配置结构体（标量部分写共享配置模型，值类型部分变更落盘
 *        agent-permissions.json；配置模型落盘由调用方 DAAgentModule 统一 save）
 */
void DAAgentPermissionManager::setConfig(const DAAgentPermissionConfig& config)
{
    DA_D(d);

    // 标量：经共享配置模型写入（未 engage 的标量不受影响，保持原稀疏守卫语义）
    if (config.modeSet()) {
        setMode(config.mode());
    }
    if (config.toolApprovalTimeoutSecSet() && mConfig) {
        mConfig->setToolApprovalTimeoutSec(config.toolApprovalTimeoutSec());
    }
    if (config.manualBlockInappToolsSet() && mConfig) {
        mConfig->setManualBlockInappTools(config.manualBlockInappTools());
    }
    if (config.judgeModelSet() && mConfig) {
        mConfig->setJudgeModel(config.judgeModel());
    }
    if (config.judgeTimeoutSecSet() && mConfig) {
        mConfig->setJudgeTimeoutSec(config.judgeTimeoutSec());
    }
    // 值类型：engage 才整体替换（原 JSON contains 守卫；含清空场景），硬 deny 强制回填
    bool fileChanged = false;
    if (config.rulesSet()) {
        d->mRules = config.rules();
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
        fileChanged = true;
    }
    if (config.codePatternsSet()) {
        d->mCodePatterns = config.codePatterns();
        fileChanged      = true;
    }
    if (config.tierOverridesSet()) {
        d->mTierOverrides = config.tierOverrides();
        fileChanged       = true;
    }
    if (fileChanged) {
        save();  // 规则/危险模式/覆盖表落盘 agent-permissions.json
    }
}

} // namespace DA
