// DAAgentConfig.cpp
#include "DAAgentConfig.h"
#include "DADir.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonParseError>

// DPAPI（CryptProtectData/CryptUnprotectData）——api_key 持久化加解密。
// 自 DAAgentModule.cpp 匿名命名空间迁移而来（配置加解密归属 DAAgentConfig
// 序列化边界），Windows 走 DPAPI，非 Windows 走 base64 fallback（与原实现
// 逐字一致，历史加密 blob 可互解）。
#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#endif

namespace {
// DPAPI 加解密：内存态一律明文，仅 DAAgentConfig::load()/save() 边界调用
QByteArray encryptApiKey(const QString& apiKey)
{
#ifdef Q_OS_WIN
    if (apiKey.isEmpty()) return {};
    QByteArray utf8 = apiKey.toUtf8();
    DATA_BLOB inBlob;
    inBlob.pbData = reinterpret_cast<BYTE*>(utf8.data());
    inBlob.cbData = static_cast<DWORD>(utf8.size());
    DATA_BLOB outBlob;
    if (!CryptProtectData(&inBlob, L"AgentApiKey", nullptr, nullptr, nullptr, 0, &outBlob)) {
        return {};
    }
    QByteArray enc(reinterpret_cast<const char*>(outBlob.pbData), static_cast<int>(outBlob.cbData));
    LocalFree(outBlob.pbData);
    return enc.toBase64();
#else
    return apiKey.toUtf8().toBase64();
#endif
}

QString decryptApiKey(const QByteArray& encrypted)
{
    if (encrypted.isEmpty()) return {};
#ifdef Q_OS_WIN
    QByteArray raw = QByteArray::fromBase64(encrypted);
    DATA_BLOB inBlob;
    inBlob.pbData = reinterpret_cast<BYTE*>(raw.data());
    inBlob.cbData = static_cast<DWORD>(raw.size());
    DATA_BLOB outBlob;
    if (!CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob)) {
        qWarning("decryptApiKey: CryptUnprotectData failed, GetLastError=%lu", GetLastError());
        return {};
    }
    QString result = QString::fromUtf8(reinterpret_cast<const char*>(outBlob.pbData), static_cast<int>(outBlob.cbData));
    LocalFree(outBlob.pbData);
    return result;
#else
    return QString::fromUtf8(QByteArray::fromBase64(encrypted));
#endif
}

/// 正整数守卫读取（<=0 回退默认；镜像旧 modelContextWindowOf/modelMaxOutputOf）
int positiveOr(int v, int def)
{
    return v > 0 ? v : def;
}

/// 存储格式常量（agent-config.json 分组名与字段名；仅本文件接触）
constexpr const char* kGroupLlm        = "llm";
constexpr const char* kGroupExecution  = "execution";
constexpr const char* kGroupSubagent   = "subagent";
constexpr const char* kGroupPermission = "permission";

/// 旧 agent-config.ini 的 [agent] 组 key（迁移源；仅本文件接触）
constexpr const char* kIniPrefix = "agent/";

// ---- providers JSON 数组 ↔ 结构体列表（api_key 在此边界加解密） ----

QList< DA::DAAgentProvider > providersFromJsonArray(const QJsonArray& arr)
{
    QList< DA::DAAgentProvider > out;
    for (const QJsonValue& pv : arr) {
        if (!pv.isObject()) {
            continue;
        }
        const QJsonObject po = pv.toObject();
        DA::DAAgentProvider p;
        p.name    = po.value("name").toString();
        p.baseUrl = po.value("base_url").toString();
        const QString enc = po.value("api_key").toString();
        p.apiKey  = enc.isEmpty() ? QString() : decryptApiKey(enc.toUtf8());
        // 模型条目兼容旧格式字符串（规范化为对象语义）
        const QJsonArray models = po.value("models").toArray();
        for (const QJsonValue& mv : models) {
            DA::DAAgentModel m;
            if (mv.isObject()) {
                const QJsonObject mo = mv.toObject();
                m.id             = mo.value("id").toString();
                m.contextWindow  = positiveOr(mo.value("context_window").toInt(262144), 262144);
                m.maxOutputTokens = positiveOr(mo.value("max_output_tokens").toInt(131072), 131072);
            } else if (mv.isString()) {
                m.id = mv.toString();  // 旧格式字符串 → 默认 262144/131072
            } else {
                continue;
            }
            p.models.append(m);
        }
        out.append(p);
    }
    return out;
}

QJsonArray providersToJsonArray(const QList< DA::DAAgentProvider >& providers)
{
    QJsonArray arr;
    for (const DA::DAAgentProvider& p : providers) {
        QJsonObject po;
        po["name"]     = p.name;
        po["base_url"] = p.baseUrl;
        po["api_key"]  = QString::fromUtf8(encryptApiKey(p.apiKey));
        QJsonArray models;
        for (const DA::DAAgentModel& m : p.models) {
            QJsonObject mo;
            mo["id"]                = m.id;
            mo["context_window"]    = m.contextWindow;
            mo["max_output_tokens"] = m.maxOutputTokens;
            models.append(mo);
        }
        po["models"] = models;
        arr.append(po);
    }
    return arr;
}
}  // namespace

namespace DA
{

// ===========================================================================
// DAAgentLLMConfig 组合操作
// ===========================================================================

/**
 * @brief 仅吸收显式设置过的字段（原 setLLMConfig 的 contains 守卫语义）
 * @param other 配置来源（其 engaged 字段覆盖本对象同名字段）
 */
void DAAgentLLMConfig::mergeFrom(const DAAgentLLMConfig& other)
{
    if (other.mBaseUrl) mBaseUrl = other.mBaseUrl;
    if (other.mModel) mModel = other.mModel;
    // api_key engaged 即覆盖（含空串清空，与旧版 setLLMConfig 无条件写一致）
    if (other.mApiKey) mApiKey = other.mApiKey;
    if (other.mContextWindow) mContextWindow = other.mContextWindow;
    if (other.mMaxOutputTokens) mMaxOutputTokens = other.mMaxOutputTokens;
    if (other.mMaxRetries) mMaxRetries = other.mMaxRetries;
    if (other.mRequestTimeoutSec) mRequestTimeoutSec = other.mRequestTimeoutSec;
    if (other.mReadyTimeoutSec) mReadyTimeoutSec = other.mReadyTimeoutSec;
    if (other.mStopTimeoutSec) mStopTimeoutSec = other.mStopTimeoutSec;
    if (other.mInactivityTimeoutSec) mInactivityTimeoutSec = other.mInactivityTimeoutSec;
    if (other.mMaxSubprocessRestarts) mMaxSubprocessRestarts = other.mMaxSubprocessRestarts;
    if (other.mRecursionLimit) mRecursionLimit = other.mRecursionLimit;
    if (other.mAutoPrestart) mAutoPrestart = other.mAutoPrestart;
    if (other.mCompactionThreshold) mCompactionThreshold = other.mCompactionThreshold;
    if (other.mMaxRecentMessages) mMaxRecentMessages = other.mMaxRecentMessages;
    if (other.mToolResultMaxChars) mToolResultMaxChars = other.mToolResultMaxChars;
    if (other.mToolResultPreviewChars) mToolResultPreviewChars = other.mToolResultPreviewChars;
    if (other.mMaxSessions) mMaxSessions = other.mMaxSessions;
    if (other.mSessionRetentionDays) mSessionRetentionDays = other.mSessionRetentionDays;
    if (other.mSubagentTimeoutSec) mSubagentTimeoutSec = other.mSubagentTimeoutSec;
    if (other.mSubagentRecursionLimit) mSubagentRecursionLimit = other.mSubagentRecursionLimit;
    if (other.mSubagentMaxConcurrency) mSubagentMaxConcurrency = other.mSubagentMaxConcurrency;
    if (other.mSubagentBatchLimit) mSubagentBatchLimit = other.mSubagentBatchLimit;
}

/**
 * @brief 是否无任何显式设置（全新安装态）
 * @return 全部字段未 engage 返回 true
 */
bool DAAgentLLMConfig::isEmpty() const
{
    return !mBaseUrl && !mModel && !mApiKey && !mContextWindow && !mMaxOutputTokens && !mMaxRetries
           && !mRequestTimeoutSec && !mReadyTimeoutSec && !mStopTimeoutSec && !mInactivityTimeoutSec
           && !mMaxSubprocessRestarts && !mRecursionLimit && !mAutoPrestart && !mCompactionThreshold
           && !mMaxRecentMessages && !mToolResultMaxChars && !mToolResultPreviewChars && !mMaxSessions
           && !mSessionRetentionDays && !mSubagentTimeoutSec && !mSubagentRecursionLimit
           && !mSubagentMaxConcurrency && !mSubagentBatchLimit;
}

// ===========================================================================
// PrivateData
// ===========================================================================

class DAAgentConfig::PrivateData
{
public:
    explicit PrivateData(DAAgentConfig* p);
    ~PrivateData();

    // 从 JSON 根对象应用配置（存在的键 engage；api_key/providers 在此解密）
    void applyJson(const QJsonObject& root);
    // 从旧 ini 文件应用配置（存在的键覆盖 engage；用于迁移与 .bak 恢复）
    void applyIni(const QString& iniPath);
    // load() 数据源应用后规范化：flat-only 老配置固化合成 Default 供应商 +
    // syncActiveConnection 重算派生连接（v2 起派生 flat 键不持久化）
    void normalizeAfterLoad();

    DAAgentLLMConfig mLlm;                     ///< LLM/运行参数（23 个稀疏字段）
    QList< DAAgentProvider > mProviders;       ///< 供应商列表（内存态明文 api_key）
    bool mProvidersSet = false;                ///< providers 是否显式配置过（区分"未配置"与"配置为空"）
    QString mActiveProvider;                   ///< 激活供应商名（空=未配置）
    std::optional< QString > mPermissionMode;
    std::optional< int > mToolApprovalTimeoutSec;
    std::optional< QString > mJudgeModel;
    std::optional< int > mJudgeTimeoutSec;
    std::optional< bool > mManualBlockInappTools;

private:
    DA_DECLARE_PUBLIC(DAAgentConfig)
};

DAAgentConfig::PrivateData::PrivateData(DAAgentConfig* p) : q_ptr(p)
{
}

DAAgentConfig::PrivateData::~PrivateData()
{
}

/**
 * @brief 从 agent-config.json 根对象应用配置（稀疏：仅 engage 存在的键）
 * @param root JSON 根对象
 */
void DAAgentConfig::PrivateData::applyJson(const QJsonObject& root)
{
    // ---- llm 分组 ----
    // v2 起 llm 分组仅持久化 active_provider/active_model/max_retries/
    // request_timeout_sec/providers；下列 flat 派生键（base_url/api_key/model/
    // context_window/max_output_tokens）仅作 v1/flat-only 老配置的兼容读取
    //（normalizeAfterLoad 中合成 Default 供应商的数据源），随后被重算覆盖
    const QJsonObject llmG = root.value(QLatin1String(kGroupLlm)).toObject();
    if (llmG.contains("base_url"))
        mLlm.setBaseUrl(llmG.value("base_url").toString());
    if (llmG.contains("active_model"))
        mLlm.setModel(llmG.value("active_model").toString());
    else if (llmG.contains("model"))  // v1 键名兼容（flat 快照时代）
        mLlm.setModel(llmG.value("model").toString());
    if (llmG.contains("api_key")) {
        const QString enc = llmG.value("api_key").toString();
        mLlm.setApiKey(enc.isEmpty() ? QString() : decryptApiKey(enc.toUtf8()));
    }
    if (llmG.contains("providers")) {
        mProviders    = providersFromJsonArray(llmG.value("providers").toArray());
        mProvidersSet = true;
    }
    if (llmG.contains("active_provider"))
        mActiveProvider = llmG.value("active_provider").toString();
    if (llmG.contains("context_window"))
        mLlm.setContextWindow(llmG.value("context_window").toInt(262144));
    if (llmG.contains("max_output_tokens"))
        mLlm.setMaxOutputTokens(llmG.value("max_output_tokens").toInt(131072));
    if (llmG.contains("max_retries"))
        mLlm.setMaxRetries(llmG.value("max_retries").toInt(7));
    if (llmG.contains("request_timeout_sec"))
        mLlm.setRequestTimeoutSec(llmG.value("request_timeout_sec").toInt(120));

    // ---- execution 分组 ----
    const QJsonObject execG = root.value(QLatin1String(kGroupExecution)).toObject();
    if (execG.contains("ready_timeout_sec"))
        mLlm.setReadyTimeoutSec(execG.value("ready_timeout_sec").toInt(60));
    if (execG.contains("stop_timeout_sec"))
        mLlm.setStopTimeoutSec(execG.value("stop_timeout_sec").toInt(5));
    if (execG.contains("inactivity_timeout_sec"))
        mLlm.setInactivityTimeoutSec(execG.value("inactivity_timeout_sec").toInt(240));
    if (execG.contains("max_subprocess_restarts"))
        mLlm.setMaxSubprocessRestarts(execG.value("max_subprocess_restarts").toInt(3));
    if (execG.contains("recursion_limit"))
        mLlm.setRecursionLimit(execG.value("recursion_limit").toInt(-1));
    if (execG.contains("auto_prestart"))
        mLlm.setAutoPrestart(execG.value("auto_prestart").toBool(true));
    if (execG.contains("compaction_threshold"))
        mLlm.setCompactionThreshold(execG.value("compaction_threshold").toDouble(0.85));
    if (execG.contains("max_recent_messages"))
        mLlm.setMaxRecentMessages(execG.value("max_recent_messages").toInt(10));
    if (execG.contains("tool_result_max_chars"))
        mLlm.setToolResultMaxChars(execG.value("tool_result_max_chars").toInt(20000));
    if (execG.contains("tool_result_preview_chars"))
        mLlm.setToolResultPreviewChars(execG.value("tool_result_preview_chars").toInt(2000));
    if (execG.contains("max_sessions"))
        mLlm.setMaxSessions(execG.value("max_sessions").toInt(20));
    if (execG.contains("session_retention_days"))
        mLlm.setSessionRetentionDays(execG.value("session_retention_days").toInt(30));

    // ---- subagent 分组 ----
    const QJsonObject subG = root.value(QLatin1String(kGroupSubagent)).toObject();
    if (subG.contains("timeout_sec"))
        mLlm.setSubagentTimeoutSec(subG.value("timeout_sec").toInt(600));
    if (subG.contains("recursion_limit"))
        mLlm.setSubagentRecursionLimit(subG.value("recursion_limit").toInt(60));
    if (subG.contains("max_concurrency"))
        mLlm.setSubagentMaxConcurrency(subG.value("max_concurrency").toInt(2));
    if (subG.contains("batch_limit"))
        mLlm.setSubagentBatchLimit(subG.value("batch_limit").toInt(4));

    // ---- permission 分组 ----
    const QJsonObject permG = root.value(QLatin1String(kGroupPermission)).toObject();
    if (permG.contains("mode"))
        mPermissionMode = permG.value("mode").toString();
    if (permG.contains("tool_approval_timeout_sec"))
        mToolApprovalTimeoutSec = qMax(1, permG.value("tool_approval_timeout_sec").toInt(600));
    if (permG.contains("judge_model"))
        mJudgeModel = permG.value("judge_model").toString().trimmed();
    if (permG.contains("judge_timeout_sec"))
        mJudgeTimeoutSec = qMax(1, permG.value("judge_timeout_sec").toInt(30));
    if (permG.contains("manual_block_inapp_tools"))
        mManualBlockInappTools = permG.value("manual_block_inapp_tools").toBool(false);
}

/**
 * @brief 从旧 agent-config.ini 应用配置（存在的键覆盖 engage）
 *
 * 迁移与 .bak 恢复共用。覆盖式合并：json 已有而 ini 缺失的键保留 json 值
 *（兼容「回滚旧版再用→再升级」时 ini 只含旧版已知键的场景）。
 * @param iniPath ini 文件绝对路径
 */
void DAAgentConfig::PrivateData::applyIni(const QString& iniPath)
{
    QSettings s(iniPath, QSettings::IniFormat);
    const QString p = QLatin1String(kIniPrefix);
    // ---- llm 连接 ----
    if (s.contains(p + "llm_base_url"))
        mLlm.setBaseUrl(s.value(p + "llm_base_url").toString());
    if (s.contains(p + "llm_model"))
        mLlm.setModel(s.value(p + "llm_model").toString());
    if (s.contains(p + "llm_api_key")) {
        // IniFormat 原生支持 QByteArray(@ByteArray 注解)的 DPAPI 加密 blob
        const QByteArray enc = s.value(p + "llm_api_key").toByteArray();
        if (!enc.isEmpty()) {
            mLlm.setApiKey(decryptApiKey(enc));
        }
    }
    if (s.contains(p + "providers")) {
        // 旧格式：Compact JSON 字符串；空字符串视为未配置（保持旧 getProviders 语义）
        const QString raw = s.value(p + "providers").toString();
        if (!raw.isEmpty()) {
            const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
            mProviders    = providersFromJsonArray(doc.array());
            mProvidersSet = true;
        }
    }
    if (s.contains(p + "active_provider"))
        mActiveProvider = s.value(p + "active_provider").toString();
    if (s.contains(p + "context_window"))
        mLlm.setContextWindow(s.value(p + "context_window", 262144).toInt());
    if (s.contains(p + "max_output_tokens"))
        mLlm.setMaxOutputTokens(s.value(p + "max_output_tokens", 131072).toInt());
    if (s.contains(p + "llm_max_retries"))
        mLlm.setMaxRetries(s.value(p + "llm_max_retries", 7).toInt());
    if (s.contains(p + "llm_request_timeout_sec"))
        mLlm.setRequestTimeoutSec(s.value(p + "llm_request_timeout_sec", 120).toInt());
    // ---- execution ----
    if (s.contains(p + "ready_timeout_sec"))
        mLlm.setReadyTimeoutSec(s.value(p + "ready_timeout_sec", 60).toInt());
    if (s.contains(p + "stop_timeout_sec"))
        mLlm.setStopTimeoutSec(s.value(p + "stop_timeout_sec", 5).toInt());
    if (s.contains(p + "inactivity_timeout_sec"))
        mLlm.setInactivityTimeoutSec(s.value(p + "inactivity_timeout_sec", 240).toInt());
    if (s.contains(p + "max_subprocess_restarts"))
        mLlm.setMaxSubprocessRestarts(s.value(p + "max_subprocess_restarts", 3).toInt());
    if (s.contains(p + "recursion_limit"))
        mLlm.setRecursionLimit(s.value(p + "recursion_limit", -1).toInt());
    if (s.contains(p + "auto_prestart"))
        mLlm.setAutoPrestart(s.value(p + "auto_prestart", true).toBool());
    if (s.contains(p + "compaction_threshold"))
        mLlm.setCompactionThreshold(s.value(p + "compaction_threshold", 0.85).toDouble());
    if (s.contains(p + "max_recent_messages"))
        mLlm.setMaxRecentMessages(s.value(p + "max_recent_messages", 10).toInt());
    if (s.contains(p + "tool_result_max_chars"))
        mLlm.setToolResultMaxChars(s.value(p + "tool_result_max_chars", 20000).toInt());
    if (s.contains(p + "tool_result_preview_chars"))
        mLlm.setToolResultPreviewChars(s.value(p + "tool_result_preview_chars", 2000).toInt());
    if (s.contains(p + "max_sessions"))
        mLlm.setMaxSessions(s.value(p + "max_sessions", 20).toInt());
    if (s.contains(p + "session_retention_days"))
        mLlm.setSessionRetentionDays(s.value(p + "session_retention_days", 30).toInt());
    // ---- subagent ----
    if (s.contains(p + "subagent_timeout_sec"))
        mLlm.setSubagentTimeoutSec(s.value(p + "subagent_timeout_sec", 600).toInt());
    if (s.contains(p + "subagent_recursion_limit"))
        mLlm.setSubagentRecursionLimit(s.value(p + "subagent_recursion_limit", 60).toInt());
    if (s.contains(p + "subagent_max_concurrency"))
        mLlm.setSubagentMaxConcurrency(s.value(p + "subagent_max_concurrency", 1).toInt());
    if (s.contains(p + "subagent_batch_limit"))
        mLlm.setSubagentBatchLimit(s.value(p + "subagent_batch_limit", 2).toInt());
    // ---- permission ----
    if (s.contains(p + "permission_mode"))
        mPermissionMode = s.value(p + "permission_mode").toString();
    if (s.contains(p + "tool_approval_timeout_sec"))
        mToolApprovalTimeoutSec = qMax(1, s.value(p + "tool_approval_timeout_sec", 600).toInt());
    if (s.contains(p + "judge_model"))
        mJudgeModel = s.value(p + "judge_model").toString().trimmed();
    if (s.contains(p + "judge_timeout_sec"))
        mJudgeTimeoutSec = qMax(1, s.value(p + "judge_timeout_sec", 30).toInt());
    if (s.contains(p + "manual_block_inapp_tools"))
        mManualBlockInappTools = s.value(p + "manual_block_inapp_tools", false).toBool();
}

/**
 * @brief load() 数据源应用后的规范化（v2 格式核心不变量）
 *
 *  1. flat-only 老配置（无 providers 键，仅有 base_url/api_key/model 等 flat 键）：
 *     固化合成 "Default" 供应商（engage providers）——v2 起派生 flat 键不再持久化，
 *     不固化则下次 save() 后 api_key/base_url 将随 flat 键一并丢失
 *  2. syncActiveConnection() 重算派生连接：base_url/api_key/model/context_window/
 *     max_output_tokens 一律以 providers+active_provider 为准（v1 json 中的 flat
 *     快照值被重算覆盖，仅充当 flat-only 合成的数据源）
 */
void DAAgentConfig::PrivateData::normalizeAfterLoad()
{
    if (!mProvidersSet) {
        const QString baseUrl = mLlm.baseUrl();
        const QString apiKey  = mLlm.apiKey();
        const QString model   = mLlm.model();
        if (!baseUrl.isEmpty() || !apiKey.isEmpty() || !model.isEmpty()) {
            DAAgentProvider p;
            p.name    = QStringLiteral("Default");
            p.baseUrl = baseUrl;
            p.apiKey  = apiKey;
            if (!model.isEmpty()) {
                DAAgentModel m;
                m.id              = model;
                m.contextWindow   = mLlm.contextWindow();
                m.maxOutputTokens = mLlm.maxOutputTokens();
                p.models.append(m);
            }
            mProviders    = { p };
            mProvidersSet = true;
            qInfo() << "DAAgentConfig: synthesized Default provider from legacy flat keys";
        }
    }
    q_ptr->syncActiveConnection();
}

// ===========================================================================
// ctor / dtor / copy（PIMPL 深拷贝）
// ===========================================================================

DAAgentConfig::DAAgentConfig() : DA_PIMPL_CONSTRUCT
{
}

DAAgentConfig::~DAAgentConfig()
{
}

/**
 * @brief 拷贝构造（PIMPL 深拷贝，PrivateData 值语义可默认拷贝）
 */
DAAgentConfig::DAAgentConfig(const DAAgentConfig& other) : d_ptr(new PrivateData(*other.d_ptr))
{
}

/**
 * @brief 拷贝赋值（PIMPL 深拷贝）
 */
DAAgentConfig& DAAgentConfig::operator=(const DAAgentConfig& other)
{
    if (this != &other) {
        d_ptr = std::make_unique< PrivateData >(*other.d_ptr);
    }
    return *this;
}

// ===========================================================================
// 区 A：持久化（唯一接触存储格式的代码；换格式只改 load/save）
// ===========================================================================

QString DAAgentConfig::configFilePath()
{
    return DA::DADir::getConfigPath() + "/agent-config.json";
}

QString DAAgentConfig::legacyIniPath()
{
    return DA::DADir::getConfigPath() + "/agent-config.ini";
}

QString DAAgentConfig::legacyBakPath()
{
    return DA::DADir::getConfigPath() + "/agent-config.ini.bak";
}

/**
 * @brief 读取 agent-config.json，含旧 ini 的迁移与恢复（幂等）
 *
 * 判定顺序（覆盖升级/回滚/损坏全部场景）：
 *  1. json 存在且合法 → 解析（仅 engage 存在的键，稀疏）
 *  2. 旧 agent-config.ini 存在 → ini 键覆盖式合并到当前状态 → save() 落盘
 *     → ini 重命名 .bak。同时覆盖两个场景：
 *     a) 老版本首次升级（json 不存在，ini 全量迁入）
 *     b) 回滚旧版用过再升级（json 与 ini 并存，旧版重写的 ini 键为最新值，
 *        其余键保留 json 值，两侧数据都不丢）
 *  3. json 缺失/损坏且无 ini：.bak 存在 → 从 .bak 恢复并 save() 重建 json
 *  4. json 损坏且无任何恢复源 → 默认值自愈（save() 覆盖损坏文件）
 *  5. 全新安装（三者皆无）→ 稀疏空配置，不落盘（首次 save 才生成文件）
 *
 * 各数据源应用后统一 normalizeAfterLoad()（flat-only 合成 Default 供应商 +
 * 重算派生连接），随后落盘的 save() 均写出 v2 格式（llm 下无派生 flat 键）。
 * @return 是否成功（全新安装/迁移成功均返回 true；仅落盘失败返回 false）
 */
bool DAAgentConfig::load()
{
    DA_D(d);
    const QString jsonPath = configFilePath();
    bool jsonOk = false;
    if (QFile::exists(jsonPath)) {
        QFile f(jsonPath);
        if (f.open(QIODevice::ReadOnly)) {
            const QByteArray bytes = f.readAll();
            f.close();
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                d->applyJson(doc.object());
                jsonOk = true;
            } else {
                qWarning() << "DAAgentConfig: config parse failed:" << err.errorString()
                           << "- trying legacy recovery, path:" << jsonPath;
            }
        } else {
            qWarning() << "DAAgentConfig: failed to open config for read:" << jsonPath << f.errorString();
        }
    }
    // 旧 ini 存在（升级迁移 / 回滚后再升级合并）
    if (QFile::exists(legacyIniPath())) {
        d->applyIni(legacyIniPath());
        d->normalizeAfterLoad();
        const bool saved = save();
        // 迁移完成后重命名 .bak（旧 .bak 先删除；回滚旧版需手动改回原名）
        if (QFile::exists(legacyBakPath())) {
            QFile::remove(legacyBakPath());
        }
        QFile iniFile(legacyIniPath());
        if (!iniFile.rename(legacyBakPath())) {
            qWarning() << "DAAgentConfig: failed to rename legacy ini to backup:" << legacyIniPath();
        } else {
            qInfo() << "DAAgentConfig: migrated legacy agent-config.ini to agent-config.json";
        }
        return saved;
    }
    if (jsonOk) {
        d->normalizeAfterLoad();
        return true;  // 正常路径：json 已解析
    }
    // json 缺失/损坏且无 ini：从 .bak 恢复重建
    if (QFile::exists(legacyBakPath())) {
        qWarning() << "DAAgentConfig: restoring config from legacy backup:" << legacyBakPath();
        d->applyIni(legacyBakPath());
        d->normalizeAfterLoad();
        return save();
    }
    // json 损坏且无恢复源 → 默认值自愈覆盖；全新安装 → 不落盘
    if (QFile::exists(jsonPath)) {
        qWarning() << "DAAgentConfig: no recovery source, reseeding defaults over corrupt config:" << jsonPath;
        return save();
    }
    return true;
}

/**
 * @brief 稀疏原子写 agent-config.json（tmp + rename，镜像 PermissionManager::save）
 *
 * 只序列化显式设置过的字段。v2 格式：llm 分组仅持久化 active_provider/
 * active_model/max_retries/request_timeout_sec/providers，派生连接键（base_url/
 * api_key/model/context_window/max_output_tokens）不落盘——它们由
 * load()/syncActiveConnection() 从激活供应商重算，providers 为唯一事实来源。
 * api_key 于此处加密（内存态明文 → base64）。
 * @return 是否成功
 */
bool DAAgentConfig::save() const
{
    DA_DC(d);
    const DAAgentLLMConfig& c = d->mLlm;
    QJsonObject root;
    root["version"] = 1;

    QJsonObject llmG;
    if (d->mProvidersSet)
        llmG["providers"] = providersToJsonArray(d->mProviders);
    if (!d->mActiveProvider.isEmpty())
        llmG["active_provider"] = d->mActiveProvider;
    if (c.modelSet() && !c.model().isEmpty())
        llmG["active_model"] = c.model();
    if (c.maxRetriesSet())
        llmG["max_retries"] = c.maxRetries();
    if (c.requestTimeoutSecSet())
        llmG["request_timeout_sec"] = c.requestTimeoutSec();
    if (!llmG.isEmpty())
        root[kGroupLlm] = llmG;

    QJsonObject execG;
    if (c.readyTimeoutSecSet())
        execG["ready_timeout_sec"] = c.readyTimeoutSec();
    if (c.stopTimeoutSecSet())
        execG["stop_timeout_sec"] = c.stopTimeoutSec();
    if (c.inactivityTimeoutSecSet())
        execG["inactivity_timeout_sec"] = c.inactivityTimeoutSec();
    if (c.maxSubprocessRestartsSet())
        execG["max_subprocess_restarts"] = c.maxSubprocessRestarts();
    if (c.recursionLimitSet())
        execG["recursion_limit"] = c.recursionLimit();
    if (c.autoPrestartSet())
        execG["auto_prestart"] = c.autoPrestart();
    if (c.compactionThresholdSet())
        execG["compaction_threshold"] = c.compactionThreshold();
    if (c.maxRecentMessagesSet())
        execG["max_recent_messages"] = c.maxRecentMessages();
    if (c.toolResultMaxCharsSet())
        execG["tool_result_max_chars"] = c.toolResultMaxChars();
    if (c.toolResultPreviewCharsSet())
        execG["tool_result_preview_chars"] = c.toolResultPreviewChars();
    if (c.maxSessionsSet())
        execG["max_sessions"] = c.maxSessions();
    if (c.sessionRetentionDaysSet())
        execG["session_retention_days"] = c.sessionRetentionDays();
    if (!execG.isEmpty())
        root[kGroupExecution] = execG;

    QJsonObject subG;
    if (c.subagentTimeoutSecSet())
        subG["timeout_sec"] = c.subagentTimeoutSec();
    if (c.subagentRecursionLimitSet())
        subG["recursion_limit"] = c.subagentRecursionLimit();
    if (c.subagentMaxConcurrencySet())
        subG["max_concurrency"] = c.subagentMaxConcurrency();
    if (c.subagentBatchLimitSet())
        subG["batch_limit"] = c.subagentBatchLimit();
    if (!subG.isEmpty())
        root[kGroupSubagent] = subG;

    QJsonObject permG;
    if (d->mPermissionMode)
        permG["mode"] = *d->mPermissionMode;
    if (d->mToolApprovalTimeoutSec)
        permG["tool_approval_timeout_sec"] = *d->mToolApprovalTimeoutSec;
    if (d->mJudgeModel)
        permG["judge_model"] = *d->mJudgeModel;
    if (d->mJudgeTimeoutSec)
        permG["judge_timeout_sec"] = *d->mJudgeTimeoutSec;
    if (d->mManualBlockInappTools)
        permG["manual_block_inapp_tools"] = *d->mManualBlockInappTools;
    if (!permG.isEmpty())
        root[kGroupPermission] = permG;

    const QString path = configFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    const QString tmpPath = path + ".tmp";
    QFile tf(tmpPath);
    if (!tf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "DAAgentConfig: failed to open tmp file for write:" << tmpPath << tf.errorString();
        return false;
    }
    tf.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    tf.close();
    if (QFile::exists(path)) {
        QFile::remove(path);
    }
    if (!tf.rename(path)) {
        qWarning() << "DAAgentConfig: failed to rename tmp file:" << tmpPath << tf.errorString();
        return false;
    }
    return true;
}

// ===========================================================================
// LLM/运行参数
// ===========================================================================

/**
 * @brief LLM/运行参数全量视图
 * @return 配置副本（含稀疏 engage 状态）
 */
DAAgentLLMConfig DAAgentConfig::llm() const
{
    DA_DC(d);
    return d->mLlm;
}

/**
 * @brief 整体替换 LLM/运行参数
 * @param c 新配置
 */
void DAAgentConfig::setLLM(const DAAgentLLMConfig& c)
{
    DA_D(d);
    d->mLlm = c;
}

/**
 * @brief 部分更新 LLM/运行参数（仅 engaged 字段生效）
 * @param c 配置增量
 */
void DAAgentConfig::mergeLLM(const DAAgentLLMConfig& c)
{
    DA_D(d);
    d->mLlm.mergeFrom(c);
}

// ===========================================================================
// 供应商与激活连接
// ===========================================================================

/**
 * @brief 供应商列表（内存态明文 api_key）
 *
 * flat-only 老配置已在 normalizeAfterLoad() 中固化为 "Default" 供应商，正常
 * 加载后本分支不再触达；保留合成逻辑兜底未经 load() 直接构造的场景（与旧
 * getProviders 迁移分支语义一致，不 engage、不落盘）。
 * @return 供应商列表
 */
QList< DAAgentProvider > DAAgentConfig::providers() const
{
    DA_DC(d);
    if (d->mProvidersSet) {
        return d->mProviders;
    }
    DAAgentProvider p;
    p.name    = QStringLiteral("Default");
    p.baseUrl = d->mLlm.baseUrl();
    p.apiKey  = d->mLlm.apiKey();
    const QString model = d->mLlm.model();
    if (!model.isEmpty()) {
        DAAgentModel m;
        m.id             = model;
        m.contextWindow  = d->mLlm.contextWindow();
        m.maxOutputTokens = d->mLlm.maxOutputTokens();  // 派生：与 getter 默认值同源（旧实现硬编码 8192）
        p.models.append(m);
    }
    return { p };
}

/**
 * @brief 整体替换供应商列表（内存态明文；持久化加密在 save()）
 *
 * 激活供应商被重命名时同步跟进：旧 active_provider 消失且新列表恰有一个
 * 新名字（集合差集 1:1）视为重命名，active_provider 随之更新，避免激活项
 * 被 syncActiveConnection 静默兜底到第一个供应商。
 * @param ps 供应商列表
 */
void DAAgentConfig::setProviders(const QList< DAAgentProvider >& ps)
{
    DA_D(d);
    // 重命名检测（仅对已有显式配置的列表做差集）
    const QString active = d->mActiveProvider;
    if (d->mProvidersSet && !active.isEmpty()) {
        QSet< QString > oldNames;
        for (const DAAgentProvider& p : std::as_const(d->mProviders)) {
            oldNames.insert(p.name);
        }
        QSet< QString > newNames;
        for (const DAAgentProvider& p : std::as_const(ps)) {
            newNames.insert(p.name);
        }
        // active 的旧名在旧列表存在、在新列表消失 → 可能被重命名（差集 1:1 判定）
        if (oldNames.contains(active) && !newNames.contains(active)) {
            QSet< QString > added   = newNames;
            QSet< QString > removed = oldNames;
            added.subtract(oldNames);
            removed.subtract(newNames);
            if (added.size() == 1 && removed.size() == 1) {
                d->mActiveProvider = *added.constBegin();
                qInfo() << "DAAgentConfig: active provider renamed:" << *removed.constBegin() << "->"
                        << *added.constBegin();
            }
        }
    }
    d->mProviders    = ps;
    d->mProvidersSet = true;
}

/**
 * @brief 激活供应商名
 * @return 名称；未配置返回空
 */
QString DAAgentConfig::activeProvider() const
{
    DA_DC(d);
    return d->mActiveProvider;
}

/**
 * @brief 设置激活供应商名
 * @param name 名称
 */
void DAAgentConfig::setActiveProvider(const QString& name)
{
    DA_D(d);
    d->mActiveProvider = name;
}

// ===========================================================================
// 区 B：业务逻辑
// ===========================================================================

/**
 * @brief 设置激活供应商+模型并同步派生 6 项（原 DAAgentModule::setActiveModel 纯逻辑）
 *
 * 校验 provider+model 存在于 providers() 后写入 active_provider/model/base_url/
 * api_key/context_window/max_output_tokens（内存态派生；持久化仅 active_provider/
 * active_model，其余由 load 重算）。供应商 api_key 为空（DPAPI 解密失败
 * 遗留）时不覆盖现有 api_key（保留可能有效的旧值）。
 * @param provider 供应商名称
 * @param model 模型 id
 * @return 校验通过并应用返回 true；未找到返回 false（状态不变）
 */
bool DAAgentConfig::applyActiveModel(const QString& provider, const QString& model)
{
    DA_D(d);
    const QList< DAAgentProvider > providers = this->providers();
    QString baseUrl, apiKey;
    int ctxWin = 262144, maxOut = 8192;
    bool found = false;
    for (const DAAgentProvider& p : providers) {
        if (p.name != provider) {
            continue;
        }
        for (const DAAgentModel& m : p.models) {
            if (m.id == model) {
                baseUrl = p.baseUrl;
                apiKey  = p.apiKey;
                ctxWin  = m.contextWindow;
                maxOut  = m.maxOutputTokens;
                found   = true;
                break;
            }
        }
        break;
    }
    if (!found) {
        return false;
    }
    d->mActiveProvider   = provider;
    d->mLlm.setModel(model);
    d->mLlm.setBaseUrl(baseUrl);
    // api_key 为空不覆盖（解密失败时保留旧值，避免 destroy 有效配置）
    if (!apiKey.isEmpty()) {
        d->mLlm.setApiKey(apiKey);
    }
    d->mLlm.setContextWindow(ctxWin);
    d->mLlm.setMaxOutputTokens(maxOut);
    return true;
}

/**
 * @brief 从激活供应商同步派生连接（原 DAAgentModule::syncActiveConnection 纯逻辑）
 *
 * v2 起为 load() 后派生重算的唯一入口：base_url/api_key/model/context_window/
 * max_output_tokens 一律从 providers+active_provider 推导（flat 快照不再持久化，
 * providers 是唯一事实来源）。激活供应商为空或已不存在（被删除）时兜底取第一个
 * 供应商；激活模型保留原值（若仍属于激活供应商），否则改用其第一个模型并同步
 * context_window/max_output_tokens；无模型则清空 model。供应商 api_key 为空
 *（解密失败遗留）时不覆盖现有值。
 */
void DAAgentConfig::syncActiveConnection()
{
    DA_D(d);
    const QList< DAAgentProvider > providers = this->providers();
    if (providers.isEmpty()) {
        return;  // 无供应商，无可同步
    }
    QString active = d->mActiveProvider;
    // 校验 active 是否仍存在于 providers；空或不存在则兜底取第一个
    bool activeExists = false;
    for (const DAAgentProvider& p : providers) {
        if (p.name == active) {
            activeExists = true;
            break;
        }
    }
    if (!activeExists) {
        active = providers.first().name;
        d->mActiveProvider = active;
    }
    for (const DAAgentProvider& p : providers) {
        if (p.name != active) {
            continue;
        }
        d->mLlm.setBaseUrl(p.baseUrl);
        // api_key 为空（DPAPI 解密失败遗留）不覆盖现有值，避免每次启动
        // pushModelSelection→syncActiveConnection 清空有效 api_key
        if (!p.apiKey.isEmpty()) {
            d->mLlm.setApiKey(p.apiKey);
        } else {
            qWarning("DAAgentConfig::syncActiveConnection: provider api_key empty, "
                     "keeping existing value to avoid destroying it");
        }
        // 激活模型：保留原值（若属于本供应商），否则取第一个；无模型则清空
        const QString curModel = d->mLlm.model();
        int matchedIdx = -1;
        for (int i = 0; i < p.models.size(); ++i) {
            if (p.models.at(i).id == curModel) {
                matchedIdx = i;
                break;
            }
        }
        if (matchedIdx < 0 && !p.models.isEmpty()) {
            matchedIdx = 0;
            d->mLlm.setModel(p.models.first().id);
        } else if (p.models.isEmpty()) {
            d->mLlm.setModel(QString());  // 无模型则清空
        }
        if (matchedIdx >= 0) {
            d->mLlm.setContextWindow(p.models.at(matchedIdx).contextWindow);
            d->mLlm.setMaxOutputTokens(p.models.at(matchedIdx).maxOutputTokens);
        }
        break;
    }
}

// ===========================================================================
// 权限层标量
// ===========================================================================

/**
 * @brief 当前权限模式（未设置默认 yolo；非法存储值回退 yolo）
 * @return yolo / auto / manual
 */
QString DAAgentConfig::permissionMode() const
{
    DA_DC(d);
    if (!d->mPermissionMode) {
        return QStringLiteral("yolo");
    }
    const QString m = *d->mPermissionMode;
    if (m == QStringLiteral("yolo") || m == QStringLiteral("auto") || m == QStringLiteral("manual")) {
        return m;
    }
    return QStringLiteral("yolo");
}

/**
 * @brief 模式是否显式设置过（A13 启动 yolo 确认卡判据：仅显式设置弹卡）
 * @return 显式设置返回 true
 */
bool DAAgentConfig::permissionModeSet() const
{
    DA_DC(d);
    return d->mPermissionMode.has_value();
}

/**
 * @brief 写入权限模式（仅接受 yolo/auto/manual，其余忽略并告警）
 * @param m 模式名
 */
void DAAgentConfig::setPermissionMode(const QString& m)
{
    DA_D(d);
    if (m != QStringLiteral("yolo") && m != QStringLiteral("auto") && m != QStringLiteral("manual")) {
        qWarning() << "DAAgentConfig::setPermissionMode: invalid mode ignored:" << m;
        return;
    }
    d->mPermissionMode = m;
}

/**
 * @brief gated_tools 批准后执行超时秒（默认 600，最小 1）
 * @return 秒数
 */
int DAAgentConfig::toolApprovalTimeoutSec() const
{
    DA_DC(d);
    return qMax(1, d->mToolApprovalTimeoutSec.value_or(600));
}

/**
 * @brief 写入批准执行超时秒（钳制最小 1）
 * @param v 秒数
 */
void DAAgentConfig::setToolApprovalTimeoutSec(int v)
{
    DA_D(d);
    d->mToolApprovalTimeoutSec = qMax(1, v);
}

/**
 * @brief 判官模型名（空=未配置，读取时 trim）
 * @return 模型名
 */
QString DAAgentConfig::judgeModel() const
{
    DA_DC(d);
    return d->mJudgeModel.value_or(QString()).trimmed();
}

/**
 * @brief 写入判官模型名
 * @param v 模型名
 */
void DAAgentConfig::setJudgeModel(const QString& v)
{
    DA_D(d);
    d->mJudgeModel = v.trimmed();
}

/**
 * @brief 判官单次调用超时秒（默认 30，最小 1）
 * @return 秒数
 */
int DAAgentConfig::judgeTimeoutSec() const
{
    DA_DC(d);
    return qMax(1, d->mJudgeTimeoutSec.value_or(30));
}

/**
 * @brief 写入判官超时秒（钳制最小 1）
 * @param v 秒数
 */
void DAAgentConfig::setJudgeTimeoutSec(int v)
{
    DA_D(d);
    d->mJudgeTimeoutSec = qMax(1, v);
}

/**
 * @brief manual 模式是否拦截应用内修改工具（默认 false）
 * @return 是否拦截
 */
bool DAAgentConfig::manualBlockInappTools() const
{
    DA_DC(d);
    return d->mManualBlockInappTools.value_or(false);
}

/**
 * @brief 写入 manual 模式拦截开关
 * @param v 是否拦截
 */
void DAAgentConfig::setManualBlockInappTools(bool v)
{
    DA_D(d);
    d->mManualBlockInappTools = v;
}

// ===========================================================================
// 区 C：协议投影
// ===========================================================================

/**
 * @brief 组装 init/reconfigure 消息的 config 字段（扁平 key，Python 协议不变）
 *
 * 输出与旧 getLLMConfig() 逐键一致（含 subagent 钳制与默认值兜底）；
 * api_key 为空时不携带该键（与旧实现一致，启动前置校验同判空）。
 * @return 协议 config 对象
 */
QJsonObject DAAgentConfig::toRunnerConfigJson() const
{
    DA_DC(d);
    const DAAgentLLMConfig& c = d->mLlm;
    QJsonObject config;
    config["base_url"] = c.baseUrl();
    config["model"]    = c.model();
    if (!c.apiKey().isEmpty()) {
        config["api_key"] = c.apiKey();
    }
    config["context_window"]            = c.contextWindow();
    config["max_output_tokens"]         = c.maxOutputTokens();
    config["compaction_threshold"]      = c.compactionThreshold();
    config["max_recent_messages"]       = c.maxRecentMessages();
    config["tool_result_max_chars"]     = c.toolResultMaxChars();
    config["tool_result_preview_chars"] = c.toolResultPreviewChars();
    config["ready_timeout_sec"]         = c.readyTimeoutSec();
    config["stop_timeout_sec"]          = c.stopTimeoutSec();
    config["max_sessions"]              = c.maxSessions();
    config["session_retention_days"]    = c.sessionRetentionDays();
    config["max_retries"]              = c.maxRetries();
    config["request_timeout_sec"]      = c.requestTimeoutSec();
    config["inactivity_timeout_sec"]   = c.inactivityTimeoutSec();
    config["max_subprocess_restarts"]  = c.maxSubprocessRestarts();
    config["recursion_limit"]          = c.recursionLimit();
    config["auto_prestart"]            = c.autoPrestart();
    config["subagent_timeout_sec"]      = c.subagentTimeoutSec();
    config["subagent_recursion_limit"]  = c.subagentRecursionLimit();
    config["subagent_max_concurrency"]  = c.subagentMaxConcurrency();
    config["subagent_batch_limit"]      = c.subagentBatchLimit();
    return config;
}

}  // namespace DA
