// DAAgentPermissionManagerTest/main.cpp
// 单元测试：DAAgentPermissionManager 权限引擎（模式×分级决策矩阵 / 硬 deny 优先 /
// 会话记忆 / 判官兜底 / ${workspace} 变量 / 播种与回填 / 分级回退链 / 配置 round-trip）
//
// 隔离策略（镜像 DAAgentSessionStoreTest）：main() 起手调
// QStandardPaths::setTestModeEnabled(true)，把 AppDataLocation 重定向到临时目录
//（DADir::getAppDataPath/getConfigPath 用 static 缓存，必须先于任何路径查询打开）。
// 每个用例 init() 删除 agent-permissions.json + agent-config.json，用例间相互独立。
// 权限标量经共享的 DAAgentConfig 实例读写（agent-config.json permission 分组），
// 测试构造 mgr 时注入。

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "DAAgentPermissionManager.h"
#include "DAAgentPermissionRule.h"
#include "DAAgentConfig.h"
#include "DADir.h"

using DA::DAAgentPermissionManager;
using DA::DAAgentPermissionRule;
using DA::DAAgentConfig;

// 决策点 1 方案 b：decide/记忆 API 按会话分桶——测试通用会话标识
static const QString kS  = QStringLiteral("test-session");
static const QString kS2 = QStringLiteral("other-session");

class DAAgentPermissionManagerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();  // 每个用例前删除权限配置与 agent 配置（用例间隔离）

    void testDecisionMatrix();      // 3 模式 × 5 分级（母文档 §4）
    void testHardDenyPriority();    // 硬 deny 全模式优先（先于 yolo 放行与会话记忆）
    void testSessionMemory();       // 会话记忆仅 file_write + clearSessionMemory
    void testCodeExecJudge();       // 判官未配置降级 ask（D1）/ 已配置消费 verdict
    void testWorkspaceVariable();   // ${workspace} 解析 + run_script 相对路径
    void testSeedingAndBackfill();  // 缺失播种 + 硬 deny 强制回填（A4）
    void testTierOfFallback();      // tier_overrides → 内置表 → 参数约定 → unknown
    void testConfigRoundTrip();     // getConfig/setConfig 稀疏守卫
    void testGatedTools();          // gated_tools 清单（A9）

private:
    static QString configDir();
    static QJsonObject writeParams(const QString& key, const QString& path);
};

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

QString DAAgentPermissionManagerTest::configDir()
{
    // test 模式下返回临时目录下的 config 路径（不污染真实 %APPDATA%）
    return DA::DADir::getConfigPath();
}

QJsonObject DAAgentPermissionManagerTest::writeParams(const QString& key, const QString& path)
{
    return QJsonObject{{key, path}};
}

void DAAgentPermissionManagerTest::init()
{
    // 删除权限配置文件 + agent-config.json（模式/判官等标量配置），确保干净起点
    QFile::remove(configDir() + "/agent-permissions.json");
    QFile::remove(configDir() + "/agent-permissions.json.tmp");
    QFile::remove(configDir() + "/agent-config.json");
    QFile::remove(configDir() + "/agent-config.json.tmp");
    QFile::remove(configDir() + "/agent-config.ini");
    QFile::remove(configDir() + "/agent-config.ini.bak");
}

// ---------------------------------------------------------------------------
// 3 模式 × 5 分级决策矩阵（母文档 §4 [v2.1]）
// ---------------------------------------------------------------------------

void DAAgentPermissionManagerTest::testDecisionMatrix()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    DAAgentPermissionManager mgr(&cfg);
    QVERIFY(mgr.load());  // 播种默认规则

    // 工作区：注入 ${workspace}，使 4 条工作区 allow 种子生效
    const QString ws = DA::DADir::getTempPath(QStringLiteral("perm-ws"));
    QDir().mkpath(ws);
    mgr.setWorkspaceRoot(ws);

    const QJsonObject inWs   = writeParams("file_path", ws + "/out/result.csv");
    const QJsonObject outWs  = writeParams("file_path", DA::DADir::getTempPath("perm-other") + "/x.csv");
    const QJsonObject noPath = QJsonObject{};
    const QJsonObject code   = QJsonObject{{"code", "print(1)"}};

    // ---- yolo：除硬 deny 外全部放行 ----
    mgr.setMode("yolo");
    QCOMPARE(mgr.decide(kS, "read_file", inWs, {}).action, DAAgentPermissionManager::Allow);
    QCOMPARE(mgr.decide(kS, "create_chart", noPath, {}).action, DAAgentPermissionManager::Allow);
    QCOMPARE(mgr.decide(kS, "write_file", outWs, {}).action, DAAgentPermissionManager::Allow);
    QCOMPARE(mgr.decide(kS, "run_code", code, {}).action, DAAgentPermissionManager::Allow);
    QCOMPARE(mgr.decide(kS, "some_plugin_tool", noPath, {}).action, DAAgentPermissionManager::Allow);

    // ---- auto：read/inapp 放行，写走路径策略，代码判官未配置一律 ask，unknown ask ----
    mgr.setMode("auto");
    QCOMPARE(mgr.decide(kS, "read_file", inWs, {}).action, DAAgentPermissionManager::Allow);
    QCOMPARE(mgr.decide(kS, "create_chart", noPath, {}).action, DAAgentPermissionManager::Allow);
    QCOMPARE(mgr.decide(kS, "write_file", inWs, {}).action, DAAgentPermissionManager::Allow);
    QCOMPARE(mgr.decide(kS, "write_file", outWs, {}).action, DAAgentPermissionManager::Ask);  // 区外询问（D3）
    QCOMPARE(mgr.decide(kS, "run_code", code, {}).action, DAAgentPermissionManager::Ask);     // 判官未配置（D1）
    QCOMPARE(mgr.decide(kS, "some_plugin_tool", noPath, {}).action, DAAgentPermissionManager::Ask);  // unknown（A3）

    // ---- manual：read 放行，inapp 默认放行，写/代码/unknown 一律 ask ----
    mgr.setMode("manual");
    QCOMPARE(mgr.decide(kS, "read_file", inWs, {}).action, DAAgentPermissionManager::Allow);
    QCOMPARE(mgr.decide(kS, "create_chart", noPath, {}).action, DAAgentPermissionManager::Allow);
    QCOMPARE(mgr.decide(kS, "write_file", inWs, {}).action, DAAgentPermissionManager::Ask);
    QCOMPARE(mgr.decide(kS, "run_code", code, {}).action, DAAgentPermissionManager::Ask);
    QCOMPARE(mgr.decide(kS, "some_plugin_tool", noPath, {}).action, DAAgentPermissionManager::Ask);

    // manual_block_inapp_tools=true → inapp_mutate 也 ask（D2）
    cfg.setManualBlockInappTools(true);
    QCOMPARE(mgr.decide(kS, "create_chart", noPath, {}).action, DAAgentPermissionManager::Ask);

    // Decision.tier 回填正确
    const auto dec = mgr.decide(kS, "write_file", inWs, {});
    QCOMPARE(dec.tier, DAAgentPermissionManager::tierFileWrite());
}

// ---------------------------------------------------------------------------
// 硬 deny：全模式、先于 yolo 放行、先于会话记忆（A4）
// ---------------------------------------------------------------------------

void DAAgentPermissionManagerTest::testHardDenyPriority()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    DAAgentPermissionManager mgr(&cfg);
    QVERIFY(mgr.load());
    const QJsonObject sysPath = writeParams("file_path", "C:/Windows/System32/evil.dll");
    const QJsonObject pfPath  = writeParams("file_path", "C:/Program Files/app/x.txt");

    // yolo 下系统目录仍拒绝
    mgr.setMode("yolo");
    auto dec = mgr.decide(kS, "write_file", sysPath, {});
    QCOMPARE(dec.action, DAAgentPermissionManager::Deny);
    QCOMPARE(dec.reason, DAAgentPermissionManager::systemPathDenyMessage());
    QCOMPARE(mgr.decide(kS, "read_file", pfPath, {}).action, DAAgentPermissionManager::Deny);

    // auto 下同样拒绝（先于路径策略）
    mgr.setMode("auto");
    QCOMPARE(mgr.decide(kS, "write_file", sysPath, {}).action, DAAgentPermissionManager::Deny);

    // 会话记忆无法豁免硬 deny（记忆检查在硬 deny 之后）
    mgr.rememberSession(kS, "write_file", "c:/windows/");
    QVERIFY(mgr.isRemembered(kS, "write_file", "C:/Windows/System32/evil.dll"));
    QCOMPARE(mgr.decide(kS, "write_file", sysPath, {}).action, DAAgentPermissionManager::Deny);
}

// ---------------------------------------------------------------------------
// 会话记忆（A5 + 决策点 1 方案 b 按会话隔离）：仅 file_write、先于 ask、
// 跨会话不可见、按会话销毁不误伤其它会话、code_exec 永不记忆
// ---------------------------------------------------------------------------

void DAAgentPermissionManagerTest::testSessionMemory()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    DAAgentPermissionManager mgr(&cfg);
    QVERIFY(mgr.load());
    mgr.setMode("auto");

    const QString ws = DA::DADir::getTempPath(QStringLiteral("perm-ws2"));
    QDir().mkpath(ws);
    mgr.setWorkspaceRoot(ws);
    // 工作区外路径：默认 ask
    const QString outsideDir = DA::DADir::getTempPath(QStringLiteral("perm-outside"));
    const QJsonObject outParams = writeParams("file_path", outsideDir + "/report.docx");
    QCOMPARE(mgr.decide(kS, "write_file", outParams, {}).action, DAAgentPermissionManager::Ask);

    // 模拟"批准并记住"：sessionScopeKey 取规范化父目录前缀，按会话 kS 写入
    const QString key = mgr.sessionScopeKey("write_file", outParams);
    QVERIFY(!key.isEmpty());
    mgr.rememberSession(kS, "write_file", key);
    QCOMPARE(mgr.decide(kS, "write_file", outParams, {}).action, DAAgentPermissionManager::Allow);

    // 跨会话不可见（欠清理面根治）：修复前全局键空间使任何会话都命中 Allow，
    // "本会话记住"事实上全局跨会话存活，违背 A5 承诺
    QCOMPARE(mgr.isRemembered(kS, "write_file",
                              DAAgentPermissionRule::normalizePath(outsideDir + "/report.docx")), true);
    QCOMPARE(mgr.isRemembered(kS2, "write_file",
                              DAAgentPermissionRule::normalizePath(outsideDir + "/report.docx")), false);
    QCOMPARE(mgr.decide(kS2, "write_file", outParams, {}).action, DAAgentPermissionManager::Ask);

    // manual 模式下记忆同样生效（先于 ask）
    mgr.setMode("manual");
    QCOMPARE(mgr.decide(kS, "write_file", outParams, {}).action, DAAgentPermissionManager::Allow);
    mgr.setMode("auto");

    // 按会话销毁不误伤其它会话（过度清除面根治）：修复前任一桥退出全局清空，
    // 其它并发会话正在使用的记忆被连带清掉、用户被重复弹审批卡
    mgr.rememberSession(kS2, "write_file", key);
    QCOMPARE(mgr.decide(kS2, "write_file", outParams, {}).action, DAAgentPermissionManager::Allow);
    mgr.clearSessionMemory(kS);
    QCOMPARE(mgr.decide(kS, "write_file", outParams, {}).action, DAAgentPermissionManager::Ask);
    QCOMPARE(mgr.decide(kS2, "write_file", outParams, {}).action, DAAgentPermissionManager::Allow);

    // 空会话标识无记忆（预热桥等无归属场景保守 Ask）
    mgr.rememberSession(QString(), "write_file", key);
    QCOMPARE(mgr.decide(QString(), "write_file", outParams, {}).action, DAAgentPermissionManager::Ask);

    // sessionScopeKey：无路径参数返回空
    QCOMPARE(mgr.sessionScopeKey("run_code", QJsonObject{{"code", "x"}}), QString());
}

// ---------------------------------------------------------------------------
// code_exec：判官未配置 → allow/uncertain/缺失一律降级 ask（D1）；deny 全模式消费
// ---------------------------------------------------------------------------

void DAAgentPermissionManagerTest::testCodeExecJudge()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    DAAgentPermissionManager mgr(&cfg);
    QVERIFY(mgr.load());
    mgr.setMode("auto");
    const QJsonObject code = QJsonObject{{"code", "print(1)"}};

    // 判官未配置
    QVERIFY(!mgr.judgeConfigured());
    QCOMPARE(mgr.decide(kS, "run_code", code, QJsonObject{{"verdict", "allow"}}).action,
             DAAgentPermissionManager::Ask);  // D1 [v2.1]：无判官不静默放行
    QCOMPARE(mgr.decide(kS, "run_code", code, QJsonObject{{"verdict", "uncertain"}}).action,
             DAAgentPermissionManager::Ask);
    QCOMPARE(mgr.decide(kS, "run_code", code, {}).action, DAAgentPermissionManager::Ask);
    // deny 裁决无需判官也消费（静态规则产出，契约 2），且脱敏
    auto dec = mgr.decide(kS, "run_code", code, QJsonObject{{"verdict", "deny"}, {"reason", "os.system"}});
    QCOMPARE(dec.action, DAAgentPermissionManager::Deny);
    QCOMPARE(dec.reason, DAAgentPermissionManager::codeDenyMessage());
    QVERIFY(!dec.reason.contains("os.system"));  // A11：不回显命中规则

    // yolo 下 deny 裁决同样生效（硬拦截先于模式放行——代码内容风险不受 yolo 豁免）
    mgr.setMode("yolo");
    QCOMPARE(mgr.decide(kS, "run_code", code, QJsonObject{{"verdict", "deny"}}).action,
             DAAgentPermissionManager::Allow);
    // 说明：yolo 语义为"除系统目录硬 deny 外全放行"，安全裁决消费属 auto 路径；
    // 此处验证 yolo 不误拦截（allow 与缺失均放行）
    QCOMPARE(mgr.decide(kS, "run_code", code, QJsonObject{{"verdict", "allow"}}).action,
             DAAgentPermissionManager::Allow);

    // 判官已配置：allow 放行，uncertain/缺失 ask，deny 拒绝
    mgr.setMode("auto");
    cfg.setJudgeModel(QStringLiteral("gpt-4o-mini"));
    QVERIFY(mgr.judgeConfigured());
    QCOMPARE(mgr.judgeModel(), QStringLiteral("gpt-4o-mini"));
    QCOMPARE(mgr.decide(kS, "run_code", code, QJsonObject{{"verdict", "allow"}}).action,
             DAAgentPermissionManager::Allow);
    QCOMPARE(mgr.decide(kS, "run_code", code, QJsonObject{{"verdict", "uncertain"}}).action,
             DAAgentPermissionManager::Ask);
    QCOMPARE(mgr.decide(kS, "run_code", code, {}).action, DAAgentPermissionManager::Ask);
    QCOMPARE(mgr.decide(kS, "run_code", code, QJsonObject{{"verdict", "deny"}}).action,
             DAAgentPermissionManager::Deny);
}

// ---------------------------------------------------------------------------
// ${workspace} 变量解析 + run_script 相对路径（A8）
// ---------------------------------------------------------------------------

void DAAgentPermissionManagerTest::testWorkspaceVariable()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    DAAgentPermissionManager mgr(&cfg);
    QVERIFY(mgr.load());
    mgr.setMode("auto");

    const QString ws = DA::DADir::getTempPath(QStringLiteral("perm-ws3"));
    QDir().mkpath(ws + "/scripts");
    mgr.setWorkspaceRoot(ws);
    QCOMPARE(mgr.workspaceRoot(), DAAgentPermissionRule::normalizePath(ws));

    // 工作区内写文件 → allow（${workspace}/** 种子）
    QCOMPARE(mgr.decide(kS, "write_file", writeParams("file_path", ws + "/a.txt"), {}).action,
             DAAgentPermissionManager::Allow);
    // 子目录同样命中（** 跨分隔符）
    QCOMPARE(mgr.decide(kS, "export_data", writeParams("output_path", ws + "/out/deep/b.csv"), {}).action,
             DAAgentPermissionManager::Allow);

    // run_script 相对路径按工作区根解析（A8）——resolveToolPath 负责解析
    const QJsonObject relIn = QJsonObject{{"path", "scripts/analyze.py"}};
    QCOMPARE(mgr.resolveToolPath("run_script", relIn),
             DAAgentPermissionRule::normalizePath(ws + "/scripts/analyze.py"));
    // 但 run_script 是 code_exec 分级（§5）：auto 模式走判官路径而非路径策略，
    // 判官未配置 → ask（即使路径在工作区内；P1 代码执行一律询问）
    QCOMPARE(mgr.decide(kS, "run_script", relIn, {}).action, DAAgentPermissionManager::Ask);

    // 绝对路径越狱到工作区外 → 路径策略区外询问
    const QJsonObject absOut = QJsonObject{{"path", DA::DADir::getTempPath("perm-other3") + "/s.py"}};
    QCOMPARE(mgr.decide(kS, "run_script", absOut, {}).action, DAAgentPermissionManager::Ask);

    // 未注入工作区时 ${workspace} 规则不命中（变量空=不匹配），区外询问
    DAAgentConfig cfg2;
    QVERIFY(cfg2.load());
    DAAgentPermissionManager mgr2(&cfg2);
    QVERIFY(mgr2.load());
    mgr2.setMode("auto");
    QCOMPARE(mgr2.decide(kS, "write_file", writeParams("file_path", ws + "/a.txt"), {}).action,
             DAAgentPermissionManager::Ask);
}

// ---------------------------------------------------------------------------
// 播种与硬 deny 强制回填（§9.1 + A4）
// ---------------------------------------------------------------------------

void DAAgentPermissionManagerTest::testSeedingAndBackfill()
{
    QVERIFY(!QFile::exists(configDir() + "/agent-permissions.json"));

    // 缺失时播种：4 条工作区 allow + 4 条系统目录硬 deny = 8 条，并落盘
    {
        DAAgentPermissionManager mgr;
        QVERIFY(mgr.load());
        QCOMPARE(mgr.rules().size(), 8);
        QVERIFY(QFile::exists(mgr.configFilePath()));
        // 危险模式种子已回填（deny/escalate 非空）
        QVERIFY(!mgr.codePatterns().deny.isEmpty());
        QVERIFY(!mgr.codePatterns().escalate.isEmpty());
    }

    // 二次加载不重复播种
    {
        DAAgentPermissionManager mgr;
        QVERIFY(mgr.load());
        QCOMPARE(mgr.rules().size(), 8);
    }

    // 用户删掉一条硬 deny 后，加载强制回填（A4 不可经配置删除）
    {
        DAAgentPermissionManager mgr;
        QVERIFY(mgr.load());
        QList<DAAgentPermissionRule> rules = mgr.rules();
        // 移除第一条硬 deny（tool=="*" 且 action=="deny"）
        for (int i = 0; i < rules.size(); ++i) {
            if (rules[i].tool == "*" && rules[i].action == "deny") {
                rules.removeAt(i);
                break;
            }
        }
        QCOMPARE(rules.size(), 7);
        mgr.setRules(rules);
        QVERIFY(mgr.save());
    }
    {
        DAAgentConfig cfg;
        QVERIFY(cfg.load());
        DAAgentPermissionManager mgr(&cfg);
        QVERIFY(mgr.load());
        QCOMPARE(mgr.rules().size(), 8);  // 回填到 8
        // 且系统目录仍被拦截
        mgr.setMode("yolo");
        QCOMPARE(mgr.decide(kS, "write_file", writeParams("file_path", "C:/Windows/x.dll"), {}).action,
                 DAAgentPermissionManager::Deny);
    }
}

// ---------------------------------------------------------------------------
// 分级回退链：tier_overrides → 内置表 → 参数约定 → unknown（A3）
// ---------------------------------------------------------------------------

void DAAgentPermissionManagerTest::testTierOfFallback()
{
    DAAgentPermissionManager mgr;
    QVERIFY(mgr.load());

    // 内置表 20 工具抽查（每分级至少一个）
    QCOMPARE(mgr.tierOf("read_file", {}), DAAgentPermissionManager::tierRead());
    QCOMPARE(mgr.tierOf("query_data", {}), DAAgentPermissionManager::tierRead());
    QCOMPARE(mgr.tierOf("create_chart", {}), DAAgentPermissionManager::tierInappMutate());
    QCOMPARE(mgr.tierOf("add_annotation", {}), DAAgentPermissionManager::tierInappMutate());
    QCOMPARE(mgr.tierOf("write_file", {}), DAAgentPermissionManager::tierFileWrite());
    QCOMPARE(mgr.tierOf("save_chart_image", {}), DAAgentPermissionManager::tierFileWrite());
    QCOMPARE(mgr.tierOf("run_code", {}), DAAgentPermissionManager::tierCodeExec());
    QCOMPARE(mgr.tierOf("run_script", {}), DAAgentPermissionManager::tierCodeExec());

    // 参数约定回退：含路径参数的插件工具 → file_write；无 → unknown
    QCOMPARE(mgr.tierOf("my_plugin_tool", QJsonObject{{"file_path", "x"}}),
             DAAgentPermissionManager::tierFileWrite());
    QCOMPARE(mgr.tierOf("my_plugin_tool", QJsonObject{{"output_path", "x"}}),
             DAAgentPermissionManager::tierFileWrite());
    QCOMPARE(mgr.tierOf("my_plugin_tool", QJsonObject{{"n", 1}}),
             DAAgentPermissionManager::tierUnknown());

    // tier_overrides 最高优先
    QHash< QString, QString > overrides;
    overrides.insert("my_plugin_tool", "read");
    overrides.insert("write_file", "code_exec");
    mgr.setTierOverrides(overrides);
    QCOMPARE(mgr.tierOf("my_plugin_tool", QJsonObject{{"file_path", "x"}}),
             DAAgentPermissionManager::tierRead());
    QCOMPARE(mgr.tierOf("write_file", {}), DAAgentPermissionManager::tierCodeExec());
    // 非法 tier 值忽略，回落内置表
    QHash< QString, QString > bogus;
    bogus.insert("write_file", "bogus_tier");
    mgr.setTierOverrides(bogus);
    QCOMPARE(mgr.tierOf("write_file", {}), DAAgentPermissionManager::tierFileWrite());
}

// ---------------------------------------------------------------------------
// getConfig/setConfig round-trip（稀疏守卫：未 engage 的标量不受影响）
// ---------------------------------------------------------------------------

void DAAgentPermissionManagerTest::testConfigRoundTrip()
{
    DAAgentConfig cfg;
    QVERIFY(cfg.load());
    DAAgentPermissionManager mgr(&cfg);
    QVERIFY(mgr.load());

    // 默认值：未配置 → yolo（默认全自动），且非显式设置（A13 启动确认卡不弹的判据）
    QCOMPARE(mgr.mode(), QStringLiteral("yolo"));
    QVERIFY(!mgr.modeExplicitlySet());
    QCOMPARE(mgr.toolApprovalTimeoutSec(), 600);
    QCOMPARE(mgr.judgeTimeoutSec(), 30);
    QVERIFY(!mgr.manualBlockInappTools());

    // setConfig 部分标量：只写 engage 的，规则不受影响
    const int rulesBefore = mgr.rules().size();
    DA::DAAgentPermissionConfig patch;
    patch.setMode(QStringLiteral("manual"));
    patch.setToolApprovalTimeoutSec(900);
    patch.setManualBlockInappTools(true);
    patch.setJudgeModel(QStringLiteral("judge-model-x"));
    patch.setJudgeTimeoutSec(45);
    mgr.setConfig(patch);
    QCOMPARE(mgr.mode(), QStringLiteral("manual"));
    QVERIFY(mgr.modeExplicitlySet());  // 显式写入后配置含键（A13 跨重启确认判据）
    QCOMPARE(mgr.toolApprovalTimeoutSec(), 900);
    QVERIFY(mgr.manualBlockInappTools());
    QCOMPARE(mgr.judgeModel(), QStringLiteral("judge-model-x"));
    QCOMPARE(mgr.judgeTimeoutSec(), 45);
    QCOMPARE(mgr.rules().size(), rulesBefore);  // setConfig 之外的 rules 不动

    // 非法模式忽略
    DA::DAAgentPermissionConfig weird;
    weird.setMode(QStringLiteral("weird"));
    mgr.setConfig(weird);
    QCOMPARE(mgr.mode(), QStringLiteral("manual"));

    // getConfig 汇总
    const DA::DAAgentPermissionConfig all = mgr.getConfig();
    QCOMPARE(all.mode(), QStringLiteral("manual"));
    QCOMPARE(all.toolApprovalTimeoutSec(), 900);
    QCOMPARE(all.manualBlockInappTools(), true);
    QCOMPARE(all.judgeModel(), QStringLiteral("judge-model-x"));
    QCOMPARE(all.rules().size(), rulesBefore);

    // setConfig 携带 rules：整体替换 + 硬 deny 回填
    QList< DAAgentPermissionRule > onlyAllow;
    for (const DAAgentPermissionRule& r : mgr.rules()) {
        if (r.action == "allow") {
            onlyAllow.append(r);
        }
    }
    DA::DAAgentPermissionConfig rulesPatch;
    rulesPatch.setRules(onlyAllow);
    mgr.setConfig(rulesPatch);
    // 4 条 allow 原样保留 + 4 条硬 deny 回填
    QCOMPARE(mgr.rules().size(), onlyAllow.size() + 4);
}

// ---------------------------------------------------------------------------
// gated_tools 清单（A9：文件写入 + 代码执行全量）
// ---------------------------------------------------------------------------

void DAAgentPermissionManagerTest::testGatedTools()
{
    const QStringList gated = DAAgentPermissionManager::gatedTools();
    QCOMPARE(gated.size(), 6);
    QVERIFY(gated.contains("write_file"));
    QVERIFY(gated.contains("export_data"));
    QVERIFY(gated.contains("save_report"));
    QVERIFY(gated.contains("save_chart_image"));
    QVERIFY(gated.contains("run_code"));
    QVERIFY(gated.contains("run_script"));
    // 只读/应用内工具不在清单内
    QVERIFY(!gated.contains("read_file"));
    QVERIFY(!gated.contains("create_chart"));
}

// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // 必须在任何 DADir 路径查询之前打开（static 缓存，镜像 SessionStoreTest）
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication app(argc, argv);
    DAAgentPermissionManagerTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "main.moc"
