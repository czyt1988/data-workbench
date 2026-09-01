// DAAgentConfigTest/main.cpp
// 单元测试：DAAgentConfig 配置领域模型（agent-config.json 稀疏读写 / 旧 ini 一次性迁移 /
// .bak 恢复 / 损坏自愈 / mergeFrom 稀疏合并 / providers 合成与旧格式规范化 /
// toRunnerConfigJson 协议投影 / 权限标量与 modeExplicitlySet 语义）
//
// 隔离策略（镜像 DAAgentPermissionManagerTest）：main() 起手调
// QStandardPaths::setTestModeEnabled(true)，把 AppDataLocation 重定向到临时目录
//（DADir::getAppDataPath/getConfigPath 用 static 缓存，必须先于任何路径查询打开）。
// 每个用例 init() 删除 agent-config.json / agent-config.ini / agent-config.ini.bak，
// 用例间相互独立。

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

#include "DAAgentConfig.h"
#include "DAAgentProvider.h"
#include "DADir.h"

using DA::DAAgentConfig;
using DA::DAAgentLLMConfig;
using DA::DAAgentModel;
using DA::DAAgentProvider;

class DAAgentConfigTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();                        // 每用例前删除配置文件（用例间隔离）
    void testDefaultsAndEmptyLoad();    // 全新安装：默认值 + 不落盘
    void testSparseRoundTrip();         // 稀疏写：只落盘 engage 键，读回一致
    void testIniMigration();            // 旧 ini 全类型 key 迁移 + ini 改名 .bak
    void testRollbackThenUpgrade();     // json+ini 并存（回滚旧版再升级）：ini 键覆盖合并
    void testCorruptJsonRecovery();     // json 损坏 + .bak 存在 → 恢复重建
    void testCorruptJsonNoSource();     // json 损坏 + 无恢复源 → 默认值自愈
    void testBakOnlyRestore();          // 仅 .bak（json 被删）→ 恢复重建
    void testMergeFrom();               // DAAgentLLMConfig::mergeFrom 稀疏合并（原 contains 守卫）
    void testRunnerConfigJson();        // toRunnerConfigJson 协议 key 集与旧版一致
    void testProvidersSynthesis();      // 无 providers 时 flat key 合成 Default 供应商
    void testProvidersLegacyFormats();  // 旧字符串模型 / Compact JSON providers 迁移规范化
    void testPermissionScalars();       // 权限标量 + modeExplicitlySet 迁移/运行期两路径
    void testApplyActiveModel();        // 校验+派生 6 项 + api_key 空不覆盖
    void testSyncActiveConnection();    // 激活供应商兜底/模型保留/上下文同步

private:
    static QString configDir();
    static void removeConfigFiles();
    static void writeLegacyIni(const QHash< QString, QVariant >& kv);
    static QJsonObject readJsonFile(const QString& path);
    static QJsonValue jsonGet(const QJsonObject& o, const char* key);
};

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

QString DAAgentConfigTest::configDir()
{
    return DA::DADir::getConfigPath();
}

void DAAgentConfigTest::removeConfigFiles()
{
    QFile::remove(configDir() + "/agent-config.json");
    QFile::remove(configDir() + "/agent-config.json.tmp");
    QFile::remove(configDir() + "/agent-config.ini");
    QFile::remove(configDir() + "/agent-config.ini.bak");
}

void DAAgentConfigTest::writeLegacyIni(const QHash< QString, QVariant >& kv)
{
    QSettings s(configDir() + "/agent-config.ini", QSettings::IniFormat);
    for (auto it = kv.constBegin(); it != kv.constEnd(); ++it) {
        s.setValue(it.key(), it.value());
    }
    s.sync();
}

QJsonObject DAAgentConfigTest::readJsonFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(f.readAll()).object();
}

QJsonValue DAAgentConfigTest::jsonGet(const QJsonObject& o, const char* key)
{
    return o.value(QLatin1String(key));
}

void DAAgentConfigTest::init()
{
    removeConfigFiles();
}

// ---------------------------------------------------------------------------
// 全新安装：无任何配置文件 → 全默认值，且不落盘文件
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testDefaultsAndEmptyLoad()
{
    QVERIFY(!QFile::exists(DAAgentConfig::configFilePath()));
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    const DAAgentLLMConfig c = cfg.llm();
    QCOMPARE(c.baseUrl(), QString());
    QCOMPARE(c.model(), QString());
    QCOMPARE(c.apiKey(), QString());
    QCOMPARE(c.contextWindow(), 262144);
    QCOMPARE(c.maxOutputTokens(), 8192);
    QCOMPARE(c.compactionThreshold(), 0.85);
    QCOMPARE(c.maxRecentMessages(), 10);
    QCOMPARE(c.toolResultMaxChars(), 20000);
    QCOMPARE(c.toolResultPreviewChars(), 2000);
    QCOMPARE(c.readyTimeoutSec(), 60);
    QCOMPARE(c.stopTimeoutSec(), 5);
    QCOMPARE(c.maxSessions(), 20);
    QCOMPARE(c.sessionRetentionDays(), 30);
    QCOMPARE(c.maxRetries(), 7);
    QCOMPARE(c.requestTimeoutSec(), 120);
    QCOMPARE(c.inactivityTimeoutSec(), 240);
    QCOMPARE(c.maxSubprocessRestarts(), 3);
    QCOMPARE(c.recursionLimit(), 150);
    QCOMPARE(c.autoPrestart(), true);
    QCOMPARE(c.subagentTimeoutSec(), 600);
    QCOMPARE(c.subagentRecursionLimit(), 60);
    QCOMPARE(c.subagentMaxConcurrency(), 2);
    QCOMPARE(c.subagentBatchLimit(), 4);
    QVERIFY(c.isEmpty());
    // 稀疏空配置不落盘（首次 save 才生成文件）
    QVERIFY(!QFile::exists(DAAgentConfig::configFilePath()));
    // 权限默认
    QCOMPARE(cfg.permissionMode(), QStringLiteral("yolo"));
    QVERIFY(!cfg.permissionModeSet());
    QCOMPARE(cfg.toolApprovalTimeoutSec(), 600);
    QCOMPARE(cfg.judgeModel(), QString());
    QCOMPARE(cfg.judgeTimeoutSec(), 30);
    QCOMPARE(cfg.manualBlockInappTools(), false);
}

// ---------------------------------------------------------------------------
// 稀疏写 round-trip：只落盘 engage 键
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testSparseRoundTrip()
{
    {
        DAAgentConfig cfg;
        QVERIFY(cfg.load());
        DAAgentLLMConfig c;
        c.setReadyTimeoutSec(120);
        c.setAutoPrestart(false);
        c.setApiKey(QStringLiteral("sk-test"));
        cfg.mergeLLM(c);
        cfg.setPermissionMode(QStringLiteral("manual"));
        QVERIFY(cfg.save());
    }
    const QJsonObject root = readJsonFile(DAAgentConfig::configFilePath());
    // 只写 engage 的键
    QCOMPARE(root.value("version").toInt(), 1);
    const QJsonObject execG = root.value("execution").toObject();
    QCOMPARE(execG.value("ready_timeout_sec").toInt(), 120);
    QCOMPARE(execG.value("auto_prestart").toBool(), false);
    QVERIFY(!execG.contains("stop_timeout_sec"));  // 未 engage 不落盘
    const QJsonObject llmG = root.value("llm").toObject();
    QVERIFY(!llmG.value("api_key").toString().isEmpty());  // 加密非空
    QVERIFY(llmG.value("api_key").toString() != QStringLiteral("sk-test"));  // 非明文
    const QJsonObject permG = root.value("permission").toObject();
    QCOMPARE(permG.value("mode").toString(), QStringLiteral("manual"));
    QVERIFY(!root.contains("subagent"));  // 整组未 engage 不落盘
    // 读回
    {
        DAAgentConfig cfg;
        QVERIFY(cfg.load());
        const DAAgentLLMConfig c = cfg.llm();
        QCOMPARE(c.readyTimeoutSec(), 120);
        QCOMPARE(c.stopTimeoutSec(), 5);   // 未设置 → 默认
        QCOMPARE(c.autoPrestart(), false);
        QCOMPARE(c.apiKey(), QStringLiteral("sk-test"));  // 加解密 round-trip
        QCOMPARE(cfg.permissionMode(), QStringLiteral("manual"));
        QVERIFY(cfg.permissionModeSet());
    }
}

// ---------------------------------------------------------------------------
// 旧 ini 全类型 key 迁移：json 生成逐值正确 + ini 改名 .bak
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testIniMigration()
{
    // 构造覆盖全类型的旧 ini（api_key 用非加密 base64 兜底路径——非 Windows 同款；
    // Windows DPAPI 对任意 blob 解密失败返回空，api_key 断言按「存在加密 blob 即迁移」验证）
    QHash< QString, QVariant > kv;
    kv.insert("agent/llm_base_url", QStringLiteral("https://api.openai.com/v1"));
    kv.insert("agent/llm_model", QStringLiteral("gpt-4o"));
    kv.insert("agent/context_window", 131072);
    kv.insert("agent/max_output_tokens", 4096);
    kv.insert("agent/ready_timeout_sec", 90);
    kv.insert("agent/stop_timeout_sec", 8);
    kv.insert("agent/compaction_threshold", 0.9);
    kv.insert("agent/max_recent_messages", 12);
    kv.insert("agent/tool_result_max_chars", 30000);
    kv.insert("agent/tool_result_preview_chars", 1500);
    kv.insert("agent/max_sessions", 25);
    kv.insert("agent/session_retention_days", 45);
    kv.insert("agent/llm_max_retries", 5);
    kv.insert("agent/llm_request_timeout_sec", 180);
    kv.insert("agent/inactivity_timeout_sec", 300);
    kv.insert("agent/max_subprocess_restarts", 2);
    kv.insert("agent/recursion_limit", 200);
    kv.insert("agent/auto_prestart", false);
    kv.insert("agent/subagent_timeout_sec", 700);
    kv.insert("agent/subagent_recursion_limit", 70);
    kv.insert("agent/subagent_max_concurrency", 1);
    kv.insert("agent/subagent_batch_limit", 3);
    kv.insert("agent/permission_mode", QStringLiteral("auto"));
    kv.insert("agent/tool_approval_timeout_sec", 660);
    kv.insert("agent/judge_model", QStringLiteral("gpt-4o-mini"));
    kv.insert("agent/judge_timeout_sec", 25);
    kv.insert("agent/manual_block_inapp_tools", true);
    kv.insert("agent/active_provider", QStringLiteral("OpenAI"));
    // providers：Compact JSON 字符串（旧格式），模型含旧字符串条目
    QJsonArray models;
    models.append(QStringLiteral("gpt-4o"));  // 旧格式字符串
    QJsonObject mo;
    mo["id"] = QStringLiteral("gpt-4o-mini");
    mo["context_window"] = 128000;
    mo["max_output_tokens"] = 16384;
    models.append(mo);
    QJsonObject p;
    p["name"]     = QStringLiteral("OpenAI");
    p["base_url"] = QStringLiteral("https://api.openai.com/v1");
    p["api_key"]  = QString();  // 空密钥（避免 DPAPI 环境差异）
    p["models"]   = models;
    kv.insert("agent/providers", QString::fromUtf8(QJsonDocument(QJsonArray{ p }).toJson(QJsonDocument::Compact)));
    writeLegacyIni(kv);

    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    // 迁移后 ini 改名 .bak、json 已生成
    QVERIFY(!QFile::exists(DAAgentConfig::legacyIniPath()));
    QVERIFY(QFile::exists(DAAgentConfig::legacyBakPath()));
    QVERIFY(QFile::exists(DAAgentConfig::configFilePath()));

    const DAAgentLLMConfig c = cfg.llm();
    QCOMPARE(c.baseUrl(), QStringLiteral("https://api.openai.com/v1"));
    QCOMPARE(c.model(), QStringLiteral("gpt-4o"));
    QCOMPARE(c.contextWindow(), 131072);
    QCOMPARE(c.maxOutputTokens(), 4096);
    QCOMPARE(c.readyTimeoutSec(), 90);
    QCOMPARE(c.stopTimeoutSec(), 8);
    QCOMPARE(c.compactionThreshold(), 0.9);
    QCOMPARE(c.maxRecentMessages(), 12);
    QCOMPARE(c.toolResultMaxChars(), 30000);
    QCOMPARE(c.toolResultPreviewChars(), 1500);
    QCOMPARE(c.maxSessions(), 25);
    QCOMPARE(c.sessionRetentionDays(), 45);
    QCOMPARE(c.maxRetries(), 5);
    QCOMPARE(c.requestTimeoutSec(), 180);
    QCOMPARE(c.inactivityTimeoutSec(), 300);
    QCOMPARE(c.maxSubprocessRestarts(), 2);
    QCOMPARE(c.recursionLimit(), 200);
    QCOMPARE(c.autoPrestart(), false);
    QCOMPARE(c.subagentTimeoutSec(), 700);
    QCOMPARE(c.subagentRecursionLimit(), 70);
    QCOMPARE(c.subagentMaxConcurrency(), 1);
    QCOMPARE(c.subagentBatchLimit(), 3);
    QCOMPARE(cfg.activeProvider(), QStringLiteral("OpenAI"));
    // 权限标量迁移
    QCOMPARE(cfg.permissionMode(), QStringLiteral("auto"));
    QVERIFY(cfg.permissionModeSet());  // ini 有键 → 显式设置（A13 判据保持）
    QCOMPARE(cfg.toolApprovalTimeoutSec(), 660);
    QCOMPARE(cfg.judgeModel(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(cfg.judgeTimeoutSec(), 25);
    QCOMPARE(cfg.manualBlockInappTools(), true);
    // providers 迁移 + 旧字符串模型规范化
    const QList< DAAgentProvider > providers = cfg.providers();
    QCOMPARE(providers.size(), 1);
    QCOMPARE(providers.first().name, QStringLiteral("OpenAI"));
    QCOMPARE(providers.first().models.size(), 2);
    QCOMPARE(providers.first().models.at(0).id, QStringLiteral("gpt-4o"));
    QCOMPARE(providers.first().models.at(0).contextWindow, 262144);   // 旧字符串 → 默认
    QCOMPARE(providers.first().models.at(0).maxOutputTokens, 8192);
    QCOMPARE(providers.first().models.at(1).id, QStringLiteral("gpt-4o-mini"));
    QCOMPARE(providers.first().models.at(1).contextWindow, 128000);
    QCOMPARE(providers.first().models.at(1).maxOutputTokens, 16384);
    // json 落盘为分组嵌套（providers 原生数组）
    const QJsonObject root = readJsonFile(DAAgentConfig::configFilePath());
    QVERIFY(root.value("llm").toObject().value("providers").isArray());
}

// ---------------------------------------------------------------------------
// 回滚旧版再用→再升级：json 与 ini 并存，ini 键（旧版最新值）覆盖，其余保留
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testRollbackThenUpgrade()
{
    // 先建立 json 配置
    {
        DAAgentConfig cfg;
        QVERIFY(cfg.load());
        DAAgentLLMConfig c;
        c.setReadyTimeoutSec(60);
        c.setRecursionLimit(150);
        cfg.mergeLLM(c);
        QVERIFY(cfg.save());
    }
    // 用户回滚旧版：旧版程序重新写 ini（只有旧版已知 key，且 ready 值变了）
    QHash< QString, QVariant > kv;
    kv.insert("agent/ready_timeout_sec", 45);   // 旧版最新值
    kv.insert("agent/llm_model", QStringLiteral("gpt-4o"));
    writeLegacyIni(kv);
    // 再升级回新版：load 合并
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    QCOMPARE(cfg.llm().readyTimeoutSec(), 45);    // ini 键覆盖（旧版最新值）
    QCOMPARE(cfg.llm().recursionLimit(), 150);    // json 独有键保留
    QCOMPARE(cfg.llm().model(), QStringLiteral("gpt-4o"));
    // ini 消费完改名 .bak
    QVERIFY(!QFile::exists(DAAgentConfig::legacyIniPath()));
    QVERIFY(QFile::exists(DAAgentConfig::legacyBakPath()));
}

// ---------------------------------------------------------------------------
// json 损坏 + .bak 存在 → 从 .bak 恢复重建
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testCorruptJsonRecovery()
{
    // 构造 .bak（合法旧 ini）
    QHash< QString, QVariant > kv;
    kv.insert("agent/ready_timeout_sec", 77);
    kv.insert("agent/permission_mode", QStringLiteral("manual"));
    writeLegacyIni(kv);
    QFile::rename(DAAgentConfig::legacyIniPath(), DAAgentConfig::legacyBakPath());
    // 写损坏 json
    {
        QFile f(DAAgentConfig::configFilePath());
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{corrupted!!!");
        f.close();
    }
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    QCOMPARE(cfg.llm().readyTimeoutSec(), 77);
    QCOMPARE(cfg.permissionMode(), QStringLiteral("manual"));
    QVERIFY(QFile::exists(DAAgentConfig::configFilePath()));  // 已重建
    // 再次 load：json 合法，正常解析（幂等）
    DAAgentConfig cfg2;
    QVERIFY(cfg2.load());
    QCOMPARE(cfg2.llm().readyTimeoutSec(), 77);
}

// ---------------------------------------------------------------------------
// json 损坏 + 无恢复源 → 默认值自愈覆盖
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testCorruptJsonNoSource()
{
    {
        QFile f(DAAgentConfig::configFilePath());
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("not json at all");
        f.close();
    }
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    QCOMPARE(cfg.llm().readyTimeoutSec(), 60);  // 默认值
    QCOMPARE(cfg.permissionMode(), QStringLiteral("yolo"));
    QVERIFY(!cfg.permissionModeSet());
    // 自愈覆盖后文件为合法 json
    const QJsonObject root = readJsonFile(DAAgentConfig::configFilePath());
    QCOMPARE(root.value("version").toInt(), 1);
}

// ---------------------------------------------------------------------------
// 仅 .bak（json 被手动删除）→ 恢复重建
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testBakOnlyRestore()
{
    QHash< QString, QVariant > kv;
    kv.insert("agent/llm_base_url", QStringLiteral("https://example.com/v1"));
    kv.insert("agent/stop_timeout_sec", 9);
    writeLegacyIni(kv);
    QFile::rename(DAAgentConfig::legacyIniPath(), DAAgentConfig::legacyBakPath());
    QVERIFY(!QFile::exists(DAAgentConfig::configFilePath()));

    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    QCOMPARE(cfg.llm().baseUrl(), QStringLiteral("https://example.com/v1"));
    QCOMPARE(cfg.llm().stopTimeoutSec(), 9);
    QVERIFY(QFile::exists(DAAgentConfig::configFilePath()));  // json 重建
    QVERIFY(QFile::exists(DAAgentConfig::legacyBakPath()));   // .bak 保留
}

// ---------------------------------------------------------------------------
// mergeFrom 稀疏合并（原 setLLMConfig 的 contains 守卫语义）
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testMergeFrom()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    DAAgentLLMConfig base;
    base.setReadyTimeoutSec(60);
    base.setModel(QStringLiteral("m1"));
    cfg.mergeLLM(base);
    // 增量：只改 ready，model 未 engage 保留
    DAAgentLLMConfig delta;
    delta.setReadyTimeoutSec(99);
    cfg.mergeLLM(delta);
    const DAAgentLLMConfig c = cfg.llm();
    QCOMPARE(c.readyTimeoutSec(), 99);
    QCOMPARE(c.model(), QStringLiteral("m1"));  // 保留
    // api_key engaged 空串 → 清空
    DAAgentLLMConfig clear;
    clear.setApiKey(QString());
    cfg.mergeLLM(clear);
    QVERIFY(cfg.llm().apiKeySet());
    QCOMPARE(cfg.llm().apiKey(), QString());
}

// ---------------------------------------------------------------------------
// toRunnerConfigJson：扁平 key 集与旧 getLLMConfig 逐键一致
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testRunnerConfigJson()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    DAAgentLLMConfig c;
    c.setBaseUrl(QStringLiteral("https://api.example.com/v1"));
    c.setModel(QStringLiteral("m1"));
    c.setApiKey(QStringLiteral("sk-xyz"));
    c.setReadyTimeoutSec(88);
    cfg.mergeLLM(c);

    const QJsonObject j = cfg.toRunnerConfigJson();
    // 与旧版 getLLMConfig 完全相同的 key 集
    const QStringList expectedKeys = {
        "base_url",           "model",             "context_window",      "max_output_tokens",
        "compaction_threshold", "max_recent_messages", "tool_result_max_chars", "tool_result_preview_chars",
        "ready_timeout_sec",  "stop_timeout_sec",  "max_sessions",        "session_retention_days",
        "max_retries",        "request_timeout_sec", "inactivity_timeout_sec", "max_subprocess_restarts",
        "recursion_limit",    "auto_prestart",     "subagent_timeout_sec", "subagent_recursion_limit",
        "subagent_max_concurrency", "subagent_batch_limit",
    };
    QCOMPARE(j.size(), expectedKeys.size() + 1);  // + api_key（非空时携带）
    for (const QString& k : expectedKeys) {
        QVERIFY2(j.contains(k), qPrintable(QStringLiteral("missing key: %1").arg(k)));
    }
    QCOMPARE(j.value("base_url").toString(), QStringLiteral("https://api.example.com/v1"));
    QCOMPARE(j.value("model").toString(), QStringLiteral("m1"));
    QCOMPARE(j.value("api_key").toString(), QStringLiteral("sk-xyz"));
    QCOMPARE(j.value("ready_timeout_sec").toInt(), 88);
    QCOMPARE(j.value("context_window").toInt(), 262144);  // 默认兜底
    QCOMPARE(j.value("auto_prestart").toBool(), true);
    // api_key 为空时不携带该键（与旧实现一致）
    DAAgentConfig cfg2;
    QVERIFY(cfg2.load());
    QVERIFY(!cfg2.toRunnerConfigJson().contains("api_key"));
}

// ---------------------------------------------------------------------------
// 无 providers 时 flat key 合成 Default 供应商（旧配置平滑升级）
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testProvidersSynthesis()
{
    QHash< QString, QVariant > kv;
    kv.insert("agent/llm_base_url", QStringLiteral("https://old.example.com/v1"));
    kv.insert("agent/llm_model", QStringLiteral("legacy-model"));
    kv.insert("agent/context_window", 65536);
    writeLegacyIni(kv);

    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    const QList< DAAgentProvider > providers = cfg.providers();
    QCOMPARE(providers.size(), 1);
    QCOMPARE(providers.first().name, QStringLiteral("Default"));
    QCOMPARE(providers.first().baseUrl, QStringLiteral("https://old.example.com/v1"));
    QCOMPARE(providers.first().models.size(), 1);
    QCOMPARE(providers.first().models.first().id, QStringLiteral("legacy-model"));
    QCOMPARE(providers.first().models.first().contextWindow, 65536);
    QCOMPARE(providers.first().models.first().maxOutputTokens, 8192);
}

// ---------------------------------------------------------------------------
// applyActiveModel：校验+派生 + 未找到不改状态 + api_key 空不覆盖
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testApplyActiveModel()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    DAAgentProvider p;
    p.name    = QStringLiteral("P1");
    p.baseUrl = QStringLiteral("https://p1.example.com/v1");
    p.apiKey  = QStringLiteral("sk-p1");
    DAAgentModel m1;
    m1.id = QStringLiteral("m1");
    m1.contextWindow  = 100000;
    m1.maxOutputTokens = 5000;
    DAAgentModel m2;
    m2.id = QStringLiteral("m2");
    p.models = { m1, m2 };
    cfg.setProviders({ p });

    // 未找到 → false 不改状态
    DAAgentLLMConfig before = cfg.llm();
    QVERIFY(!cfg.applyActiveModel(QStringLiteral("P1"), QStringLiteral("nope")));
    QVERIFY(!cfg.applyActiveModel(QStringLiteral("nope"), QStringLiteral("m1")));
    QCOMPARE(cfg.llm().model(), before.model());

    // 命中 → 派生 6 项
    QVERIFY(cfg.applyActiveModel(QStringLiteral("P1"), QStringLiteral("m2")));
    QCOMPARE(cfg.activeProvider(), QStringLiteral("P1"));
    QCOMPARE(cfg.llm().model(), QStringLiteral("m2"));
    QCOMPARE(cfg.llm().baseUrl(), QStringLiteral("https://p1.example.com/v1"));
    QCOMPARE(cfg.llm().apiKey(), QStringLiteral("sk-p1"));
    QCOMPARE(cfg.llm().contextWindow(), 262144);   // m2 默认
    QCOMPARE(cfg.llm().maxOutputTokens(), 8192);

    // api_key 为空的供应商 → 不覆盖现有值（解密失败守卫）
    DAAgentProvider p2;
    p2.name    = QStringLiteral("P2");
    p2.baseUrl = QStringLiteral("https://p2.example.com/v1");
    p2.apiKey  = QString();  // 空
    DAAgentModel m3;
    m3.id = QStringLiteral("m3");
    p2.models = { m3 };
    QList< DAAgentProvider > ps = cfg.providers();
    ps.append(p2);
    cfg.setProviders(ps);
    QVERIFY(cfg.applyActiveModel(QStringLiteral("P2"), QStringLiteral("m3")));
    QCOMPARE(cfg.llm().baseUrl(), QStringLiteral("https://p2.example.com/v1"));
    QCOMPARE(cfg.llm().apiKey(), QStringLiteral("sk-p1"));  // 保留旧值
}

// ---------------------------------------------------------------------------
// syncActiveConnection：激活供应商兜底/模型保留/无模型清空
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testSyncActiveConnection()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    // 无已配置供应商：providers() 读时合成空 Default（与旧 getProviders 语义一致），
    // syncActiveConnection 对其同步为 no-op 变化（baseUrl/model 均空）
    cfg.syncActiveConnection();
    QCOMPARE(cfg.llm().model(), QString());
    QCOMPARE(cfg.llm().baseUrl(), QString());

    DAAgentProvider p1;
    p1.name    = QStringLiteral("P1");
    p1.baseUrl = QStringLiteral("https://p1.example.com/v1");
    p1.apiKey  = QStringLiteral("sk-1");
    DAAgentModel a;
    a.id = QStringLiteral("a");
    DAAgentModel b;
    b.id = QStringLiteral("b");
    b.contextWindow  = 999999;
    b.maxOutputTokens = 12345;
    p1.models = { a, b };
    cfg.setProviders({ p1 });

    // 激活供应商为空 → 兜底取第一个；模型取第一个
    cfg.syncActiveConnection();
    QCOMPARE(cfg.activeProvider(), QStringLiteral("P1"));
    QCOMPARE(cfg.llm().model(), QStringLiteral("a"));
    QCOMPARE(cfg.llm().baseUrl(), QStringLiteral("https://p1.example.com/v1"));
    QCOMPARE(cfg.llm().apiKey(), QStringLiteral("sk-1"));

    // 当前模型属于激活供应商 → 保留并同步其上下文
    QVERIFY(cfg.applyActiveModel(QStringLiteral("P1"), QStringLiteral("b")));
    cfg.syncActiveConnection();
    QCOMPARE(cfg.llm().model(), QStringLiteral("b"));
    QCOMPARE(cfg.llm().contextWindow(), 999999);
    QCOMPARE(cfg.llm().maxOutputTokens(), 12345);

    // 激活供应商被删除 → 兜底取第一个
    DAAgentProvider p2;
    p2.name    = QStringLiteral("P2");
    p2.baseUrl = QStringLiteral("https://p2.example.com/v1");
    p2.apiKey  = QStringLiteral("sk-2");
    DAAgentModel c1;
    c1.id = QStringLiteral("c1");
    p2.models = { c1 };
    cfg.setProviders({ p2 });  // P1 被删除
    cfg.syncActiveConnection();
    QCOMPARE(cfg.activeProvider(), QStringLiteral("P2"));
    QCOMPARE(cfg.llm().model(), QStringLiteral("c1"));
    QCOMPARE(cfg.llm().baseUrl(), QStringLiteral("https://p2.example.com/v1"));

    // 无模型的供应商 → 模型清空
    DAAgentProvider p3;
    p3.name    = QStringLiteral("P3");
    p3.baseUrl = QStringLiteral("https://p3.example.com/v1");
    cfg.setProviders({ p3 });
    cfg.syncActiveConnection();
    QCOMPARE(cfg.activeProvider(), QStringLiteral("P3"));
    QCOMPARE(cfg.llm().model(), QString());
}

// ---------------------------------------------------------------------------
// 权限标量：运行期设置 + 非法值回退 + modeExplicitlySet 两路径
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testPermissionScalars()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    // 运行期显式设置
    cfg.setPermissionMode(QStringLiteral("manual"));
    QVERIFY(cfg.permissionModeSet());
    QCOMPARE(cfg.permissionMode(), QStringLiteral("manual"));
    // 非法值忽略
    cfg.setPermissionMode(QStringLiteral("hack"));
    QCOMPARE(cfg.permissionMode(), QStringLiteral("manual"));
    // 钳制与 round-trip
    cfg.setToolApprovalTimeoutSec(0);
    QCOMPARE(cfg.toolApprovalTimeoutSec(), 1);  // 最小 1
    cfg.setJudgeModel(QStringLiteral("  judge-x  "));
    QCOMPARE(cfg.judgeModel(), QStringLiteral("judge-x"));  // trim
    cfg.setJudgeTimeoutSec(50);
    cfg.setManualBlockInappTools(true);
    QVERIFY(cfg.save());
    {
        DAAgentConfig cfg2;
        QVERIFY(cfg2.load());
        QCOMPARE(cfg2.permissionMode(), QStringLiteral("manual"));
        QVERIFY(cfg2.permissionModeSet());
        QCOMPARE(cfg2.toolApprovalTimeoutSec(), 1);
        QCOMPARE(cfg2.judgeModel(), QStringLiteral("judge-x"));
        QCOMPARE(cfg2.judgeTimeoutSec(), 50);
        QCOMPARE(cfg2.manualBlockInappTools(), true);
    }
    // 迁移路径的 modeExplicitlySet（ini 有键）已在 testIniMigration 验证；
    // 此处验证「ini 无 permission key → 未显式设置」（隔离前面 save 的持久化状态）
    removeConfigFiles();
    writeLegacyIni({ { "agent/ready_timeout_sec", 50 } });
    DAAgentConfig cfg3;
    QVERIFY(cfg3.load());
    QVERIFY(!cfg3.permissionModeSet());
    QCOMPARE(cfg3.permissionMode(), QStringLiteral("yolo"));
}

// ---------------------------------------------------------------------------
// providers 旧格式（Compact JSON 字符串 + 字符串模型条目）迁移规范化
//（ini 内嵌 providers 场景已在 testIniMigration 覆盖；此处补 json 内旧字符串模型）
// ---------------------------------------------------------------------------

void DAAgentConfigTest::testProvidersLegacyFormats()
{
    // json 中 providers 数组的模型条目为旧字符串格式（历史版本写出）
    QJsonObject mo;
    mo["id"] = QStringLiteral("m-obj");
    QJsonArray models { QStringLiteral("m-str"), mo };
    QJsonObject p;
    p["name"]     = QStringLiteral("Legacy");
    p["base_url"] = QStringLiteral("https://legacy.example.com/v1");
    p["api_key"]  = QString();
    p["models"]   = models;
    QJsonObject llmG;
    llmG["providers"] = QJsonArray{ p };
    QJsonObject root;
    root["version"] = 1;
    root["llm"]     = llmG;
    {
        QFile f(DAAgentConfig::configFilePath());
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
    }
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    const QList< DAAgentProvider > providers = cfg.providers();
    QCOMPARE(providers.size(), 1);
    QCOMPARE(providers.first().models.size(), 2);
    // 字符串条目 → 规范化为默认 262144/8192
    QCOMPARE(providers.first().models.at(0).id, QStringLiteral("m-str"));
    QCOMPARE(providers.first().models.at(0).contextWindow, 262144);
    // 对象条目保持原值
    QCOMPARE(providers.first().models.at(1).id, QStringLiteral("m-obj"));
}

QTEST_MAIN(DAAgentConfigTest)
#include "main.moc"
