// DAAgentConfig.h
#pragma once
#include "DAAgentAPI.h"
#include "DAAgentProvider.h"
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <optional>
#include "DAGlobals.h"

namespace DA
{
/**
 * @brief LLM/运行参数配置（原 getLLMConfig QJsonObject 键集的结构体化）
 *
 * 25 个配置项全部为 std::optional 稀疏字段：engaged = 显式设置过
 *（未设置的键不落盘、读取时 getter 兜底默认值）。默认值唯一定义于
 * 各 getter 处，与旧 agent-config.ini 时代的语义一致。
 *
 * 字段按 agent-config.json 的三个存储分组组织：llm 连接（11）/ execution（12）/
 * subagent（4）（存储分组仅是序列化投影，结构体本身是扁平的）。
 *
 * v2 格式：base_url/model/api_key/context_window/max_output_tokens 为派生字段
 *（由激活供应商+模型经 syncActiveConnection/applyActiveModel 重算，不持久化）；
 * llm 分组仅落盘 active_provider/active_model/max_retries/retry_interval_sec/
 * retry_interval_increment_sec/request_timeout_sec/providers，
 * 结构体中的这些字段仅作内存态运行值。
 */
class DAAgent_API DAAgentLLMConfig
{
public:
    // ---- llm 连接（存储分组 llm；base_url/model/api_key 由激活供应商派生） ----
    QString baseUrl() const { return mBaseUrl.value_or(QString()); }
    bool baseUrlSet() const { return mBaseUrl.has_value(); }
    void setBaseUrl(const QString& v) { mBaseUrl = v; }

    QString model() const { return mModel.value_or(QString()); }
    bool modelSet() const { return mModel.has_value(); }
    void setModel(const QString& v) { mModel = v; }

    // 内存态明文；持久化加密只发生在 DAAgentConfig::save()/load()
    QString apiKey() const { return mApiKey.value_or(QString()); }
    bool apiKeySet() const { return mApiKey.has_value(); }
    void setApiKey(const QString& v) { mApiKey = v; }

    int contextWindow() const { return mContextWindow.value_or(262144); }
    bool contextWindowSet() const { return mContextWindow.has_value(); }
    void setContextWindow(int v) { mContextWindow = v; }

    // 默认 131072(128K)：8K 输出上限会把长文档写作截断成不可解析的工具参数
    // （表现为 agent 运行中途静默结束）；个别严格网关超出模型真实上限会 400，
    // 用户可在模型设置里调低
    int maxOutputTokens() const { return mMaxOutputTokens.value_or(131072); }
    bool maxOutputTokensSet() const { return mMaxOutputTokens.has_value(); }
    void setMaxOutputTokens(int v) { mMaxOutputTokens = v; }

    // 重试策略（线性退避 m/n/p，设置页可配）：LLM 请求失败后第 k 次重试前
    // 等待 retryIntervalSec + (k-1)*retryIntervalIncrementSec 秒，最多 maxRetries 次。
    // 覆盖所有服务器返回的波动类错误（400/429/5xx/网络），认证失败与配额耗尽
    // 快速失败不重试；默认 5/5/1 即等待 5,6,7,8,9s
    int maxRetries() const { return mMaxRetries.value_or(5); }
    bool maxRetriesSet() const { return mMaxRetries.has_value(); }
    void setMaxRetries(int v) { mMaxRetries = v; }

    int retryIntervalSec() const { return mRetryIntervalSec.value_or(5); }
    bool retryIntervalSecSet() const { return mRetryIntervalSec.has_value(); }
    void setRetryIntervalSec(int v) { mRetryIntervalSec = v; }

    int retryIntervalIncrementSec() const { return mRetryIntervalIncrementSec.value_or(1); }
    bool retryIntervalIncrementSecSet() const { return mRetryIntervalIncrementSec.has_value(); }
    void setRetryIntervalIncrementSec(int v) { mRetryIntervalIncrementSec = v; }

    int requestTimeoutSec() const { return mRequestTimeoutSec.value_or(120); }
    bool requestTimeoutSecSet() const { return mRequestTimeoutSec.has_value(); }
    void setRequestTimeoutSec(int v) { mRequestTimeoutSec = v; }

    // ---- execution（存储分组 execution） ----
    int readyTimeoutSec() const { return mReadyTimeoutSec.value_or(60); }
    bool readyTimeoutSecSet() const { return mReadyTimeoutSec.has_value(); }
    void setReadyTimeoutSec(int v) { mReadyTimeoutSec = v; }

    int stopTimeoutSec() const { return mStopTimeoutSec.value_or(5); }
    bool stopTimeoutSecSet() const { return mStopTimeoutSec.has_value(); }
    void setStopTimeoutSec(int v) { mStopTimeoutSec = v; }

    int inactivityTimeoutSec() const { return mInactivityTimeoutSec.value_or(240); }
    bool inactivityTimeoutSecSet() const { return mInactivityTimeoutSec.has_value(); }
    void setInactivityTimeoutSec(int v) { mInactivityTimeoutSec = v; }

    int maxSubprocessRestarts() const { return mMaxSubprocessRestarts.value_or(3); }
    bool maxSubprocessRestartsSet() const { return mMaxSubprocessRestarts.has_value(); }
    void setMaxSubprocessRestarts(int v) { mMaxSubprocessRestarts = v; }

    // 默认 -1 = 不限制（Python 侧 _sanitize_recursion_limit 转为 langgraph 的 None）；
    // 循环防护由重复工具调用硬终止（连续 3 次相同完整签名）+ 软引导兜底
    int recursionLimit() const { return mRecursionLimit.value_or(-1); }
    bool recursionLimitSet() const { return mRecursionLimit.has_value(); }
    void setRecursionLimit(int v) { mRecursionLimit = v; }

    bool autoPrestart() const { return mAutoPrestart.value_or(true); }
    bool autoPrestartSet() const { return mAutoPrestart.has_value(); }
    void setAutoPrestart(bool v) { mAutoPrestart = v; }

    double compactionThreshold() const { return mCompactionThreshold.value_or(0.85); }
    bool compactionThresholdSet() const { return mCompactionThreshold.has_value(); }
    void setCompactionThreshold(double v) { mCompactionThreshold = v; }

    int maxRecentMessages() const { return mMaxRecentMessages.value_or(10); }
    bool maxRecentMessagesSet() const { return mMaxRecentMessages.has_value(); }
    void setMaxRecentMessages(int v) { mMaxRecentMessages = v; }

    int toolResultMaxChars() const { return mToolResultMaxChars.value_or(20000); }
    bool toolResultMaxCharsSet() const { return mToolResultMaxChars.has_value(); }
    void setToolResultMaxChars(int v) { mToolResultMaxChars = v; }

    int toolResultPreviewChars() const { return mToolResultPreviewChars.value_or(2000); }
    bool toolResultPreviewCharsSet() const { return mToolResultPreviewChars.has_value(); }
    void setToolResultPreviewChars(int v) { mToolResultPreviewChars = v; }

    int maxSessions() const { return mMaxSessions.value_or(20); }
    bool maxSessionsSet() const { return mMaxSessions.has_value(); }
    void setMaxSessions(int v) { mMaxSessions = v; }

    int sessionRetentionDays() const { return mSessionRetentionDays.value_or(30); }
    bool sessionRetentionDaysSet() const { return mSessionRetentionDays.has_value(); }
    void setSessionRetentionDays(int v) { mSessionRetentionDays = v; }

    // ---- subagent（存储分组 subagent；读取侧钳制与旧 getLLMConfig 一致） ----
    int subagentTimeoutSec() const { return std::max(1, mSubagentTimeoutSec.value_or(600)); }
    bool subagentTimeoutSecSet() const { return mSubagentTimeoutSec.has_value(); }
    void setSubagentTimeoutSec(int v) { mSubagentTimeoutSec = std::max(1, v); }

    int subagentRecursionLimit() const { return std::max(1, mSubagentRecursionLimit.value_or(60)); }
    bool subagentRecursionLimitSet() const { return mSubagentRecursionLimit.has_value(); }
    void setSubagentRecursionLimit(int v) { mSubagentRecursionLimit = std::max(1, v); }

    // 内部键：可调低不可调高（上限 2/4）。默认保守 1/2（审计问题 27 短期动作）：
    // 并发会话下 N 进程 × 每进程 M 子 agent = 同一 LLM 供应商 N×M 路并发请求，
    // 无跨进程配额协调（仅 retry_wrapper 退避兜底 429）——默认收敛到每进程
    // 1 并发/单批 2 任务，用户可显式调回上限
    int subagentMaxConcurrency() const
    {
        return std::min(2, std::max(1, mSubagentMaxConcurrency.value_or(1)));
    }
    bool subagentMaxConcurrencySet() const { return mSubagentMaxConcurrency.has_value(); }
    void setSubagentMaxConcurrency(int v) { mSubagentMaxConcurrency = std::min(2, std::max(1, v)); }

    int subagentBatchLimit() const { return std::min(4, std::max(1, mSubagentBatchLimit.value_or(2))); }
    bool subagentBatchLimitSet() const { return mSubagentBatchLimit.has_value(); }
    void setSubagentBatchLimit(int v) { mSubagentBatchLimit = std::min(4, std::max(1, v)); }

    // ---- 组合操作 ----
    // 仅吸收显式设置过的字段（原 setLLMConfig 的 contains 守卫语义；
    // api_key engaged 即覆盖，含空串清空，与旧版一致）
    void mergeFrom(const DAAgentLLMConfig& other);
    // 是否无任何显式设置（全新安装态）
    bool isEmpty() const;

private:
    std::optional< QString > mBaseUrl;
    std::optional< QString > mModel;
    std::optional< QString > mApiKey;
    std::optional< int > mContextWindow;
    std::optional< int > mMaxOutputTokens;
    std::optional< int > mMaxRetries;
    std::optional< int > mRetryIntervalSec;
    std::optional< int > mRetryIntervalIncrementSec;
    std::optional< int > mRequestTimeoutSec;
    std::optional< int > mReadyTimeoutSec;
    std::optional< int > mStopTimeoutSec;
    std::optional< int > mInactivityTimeoutSec;
    std::optional< int > mMaxSubprocessRestarts;
    std::optional< int > mRecursionLimit;
    std::optional< bool > mAutoPrestart;
    std::optional< double > mCompactionThreshold;
    std::optional< int > mMaxRecentMessages;
    std::optional< int > mToolResultMaxChars;
    std::optional< int > mToolResultPreviewChars;
    std::optional< int > mMaxSessions;
    std::optional< int > mSessionRetentionDays;
    std::optional< int > mSubagentTimeoutSec;
    std::optional< int > mSubagentRecursionLimit;
    std::optional< int > mSubagentMaxConcurrency;
    std::optional< int > mSubagentBatchLimit;
};

/**
 * @brief agent 配置领域模型（agent-config.json 的唯一读写入口）
 *
 * 组合 LLM/运行参数（DAAgentLLMConfig）+ 供应商列表 + 激活供应商 +
 * 权限层 5 个标量。load()/save() 是唯一接触存储格式的代码——将来更换
 * 格式（如 YAML）只需改这两个函数，所有消费方零改动。
 *
 * 三类职责分区：
 *  - 区 A 持久化：load()/save()（JSON 稀疏读写 + tmp/rename 原子写 +
 *    旧 agent-config.ini 一次性迁移 + normalizeAfterLoad 规范化，规则见
 *    DAAgentConfig.cpp 头部注释）
 *  - 区 B 业务逻辑：applyActiveModel()/syncActiveConnection()（激活连接派生）
 *  - 区 C 协议投影：toRunnerConfigJson()（init/reconfigure 消息的 config
 *    字段，扁平 key 与 Python agent_runner.py 协议逐键一致）
 *
 * v2 存储格式：llm 分组仅持久化 active_provider/active_model/max_retries/
 * retry_interval_sec/retry_interval_increment_sec/request_timeout_sec/
 * providers（providers 为唯一事实来源）；派生连接键
 * base_url/model/api_key/context_window/max_output_tokens 由 load() 后
 * syncActiveConnection() 重算（v1 json / 旧 ini 的同名 flat 键仅作兼容读取）。
 * 内存态 api_key 一律明文，DPAPI 加解密只发生在 save()/load() 边界。
 * 模块持有单一实例（DAAgentModule::PrivateData::mConfig），配置为
 * 「启动加载一次的内存模型」，运行期变更经接口方法同步内存并落盘。
 */
class DAAgent_API DAAgentConfig
{
public:
    DAAgentConfig();
    ~DAAgentConfig();
    DAAgentConfig(const DAAgentConfig& other);
    DAAgentConfig& operator=(const DAAgentConfig& other);

    // ---- 区 A：持久化（唯一接触存储格式的代码） ----
    // 读取 agent-config.json；含旧 agent-config.ini/.bak 的迁移与恢复
    //（幂等：json 已存在且合法、ini 不存在时仅解析 json）
    bool load();
    // 稀疏原子写（tmp + rename，只落盘显式设置过的字段）
    bool save() const;
    // 配置文件路径（三者同目录：DADir::getConfigPath()）
    static QString configFilePath();   ///< agent-config.json
    static QString legacyIniPath();    ///< agent-config.ini（迁移源）
    static QString legacyBakPath();    ///< agent-config.ini.bak（迁移后备份/恢复源）

    // ---- LLM/运行参数 ----
    DAAgentLLMConfig llm() const;               // 全量视图
    void setLLM(const DAAgentLLMConfig& c);     // 整体替换
    void mergeLLM(const DAAgentLLMConfig& c);   // 部分更新（engaged 字段生效）

    // ---- 供应商与激活连接 ----
    // 供应商列表（内存态明文）；列表为空且 flat key 有值时读时合成单个
    // "Default" 供应商（旧版仅有 flat key 的配置平滑升级，不落盘）
    QList< DAAgentProvider > providers() const;
    void setProviders(const QList< DAAgentProvider >& ps);
    // 激活供应商名（空=未配置；不在 providers 中时由 syncActiveConnection 兜底）
    QString activeProvider() const;
    void setActiveProvider(const QString& name);

    // ---- 区 B：业务逻辑 ----
    // 设置激活供应商+模型：校验存在于 providers 后同步派生 6 项
    //（active_provider/model/base_url/api_key/context_window/max_output_tokens）。
    // 未找到返回 false 不改状态
    bool applyActiveModel(const QString& provider, const QString& model);
    // 从激活供应商同步 base_url/api_key/model/context_window/max_output_tokens：
    // 激活供应商空/不存在时兜底取第一个；激活模型不属于该供应商时取其第一个；
    // 供应商 api_key 为空（解密失败遗留）不覆盖现有值
    void syncActiveConnection();

    // ---- 权限层 5 个标量（存储于 permission 分组；供 DAAgentPermissionManager） ----
    QString permissionMode() const;             // 默认 yolo，非法值回退 yolo
    bool permissionModeSet() const;             // 显式设置过（A13 启动确认卡判据）
    void setPermissionMode(const QString& m);   // 仅接受 yolo/auto/manual
    int toolApprovalTimeoutSec() const;         // 默认 600
    void setToolApprovalTimeoutSec(int v);
    QString judgeModel() const;                 // 默认空=未配置
    void setJudgeModel(const QString& v);
    int judgeTimeoutSec() const;                // 默认 30
    void setJudgeTimeoutSec(int v);
    bool manualBlockInappTools() const;         // 默认 false
    void setManualBlockInappTools(bool v);

    // ---- 区 C：协议投影 ----
    // 组装 init/reconfigure 消息的 config 字段（扁平 key，Python 协议不变；
    // api_key 为空时不携带该键，与旧 getLLMConfig 行为一致）
    QJsonObject toRunnerConfigJson() const;

private:
    DA_DECLARE_PRIVATE(DAAgentConfig)
};
}  // namespace DA
