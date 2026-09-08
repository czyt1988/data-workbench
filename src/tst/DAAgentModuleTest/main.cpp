// DAAgentModuleTest/main.cpp
// 单元测试：DAAgentModule 会话化编排层（agent-concurrent-refactor-audit 整改验证）
//
// 测试基建（假 agent）：Module 冷启动桥时经 detectPythonExePath()（读
// python-config.json 的 config.interpreter）与 detectAgentScriptPath()
// （applicationDirPath()/PyScripts/DAWorkbench/agent/agent_runner.py）解析
// 子进程命令。initTestCase 把 python-config.json 的 interpreter 指向测试 exe
// 自身（退出时备份-恢复，不污染同目录真实程序），并保证脚本文件存在。
// 子进程模式按 argv[1] 以 "agent_runner.py" 结尾识别，剧本经环境变量
// DA_FAKE_SCENARIO 选择（默认 module-echo），不依赖真实 Python/LLM 环境。
//
// 剧本清单（随整改批次逐个补充）：
//   module-echo    : init→ready；user_msg 按 "#fake:" 指令分发（question/error/
//                    crash/question_then_error），否则回 message_end+done；
//                    load_session→session_loaded+token("LOADED:<n>:<lastRole>")
//   crash-at-init  : spawn 后立即 exit(3)（不发 ready）——驱动崩溃自愈循环至
//                    crash_exhausted 终态（error+busy(false)、无 ready）
//
// 隔离策略：main() 起手 QStandardPaths::setTestModeEnabled(true) 重定向
// AppData 到临时目录（DADir 静态缓存路径，必须先于任何路径查询）；每用例
// init() 清空 sessions 目录与 agent 配置文件，用例间相互独立。

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QScopedPointer>
#include <QStandardPaths>

#include "DAAgentModule.h"
#include "DAAgentConfig.h"
#include "DAAgentSessionStore.h"
#include "DADir.h"

// ===========================================================================
// 假 agent 子进程模式
// ===========================================================================
namespace {

void fakeEmit(QFile& out, const QJsonObject& msg)
{
    out.write(QJsonDocument(msg).toJson(QJsonDocument::Compact) + "\n");
    out.flush();
}

/// 假 agent 主循环：同步读 stdin 行、按剧本回 stdout 协议消息
int fakeAgentMain(const QByteArray& scenario)
{
    if (scenario == "crash-at-init") {
        return 3;  // spawn 后立即崩溃（不发 ready），驱动崩溃自愈循环
    }

    QFile in;
    QFile out;
    if (!in.open(stdin, QIODevice::ReadOnly | QIODevice::Text)) {
        return 1;
    }
    if (!out.open(stdout, QIODevice::WriteOnly | QIODevice::Text)) {
        return 1;
    }

    // module-echo（默认剧本）
    // arm 指令（问题 8 测试用）：user_msg 武装后不发/半发回合消息，待 reconfigure
    // 到达时再发 retrying / done(incomplete)——此时测试已切走会话，事件以后台
    // 身份到达 Module，驱动"缓存-切回重发"链路（本地进程时序经 qWait 兜底）
    bool armedRetry = false;
    bool armedIncomplete = false;
    while (true) {
        const QByteArray line = in.readLine();
        if (line.isEmpty()) {
            break;  // stdin EOF / 关闭
        }
        const QJsonDocument doc = QJsonDocument::fromJson(line.trimmed());
        if (!doc.isObject()) {
            continue;
        }
        const QJsonObject obj = doc.object();
        const QString type = obj.value("type").toString();
        if (type == QLatin1String("reconfigure")) {
            if (armedRetry) {
                armedRetry = false;
                fakeEmit(out, {{ "type", "retrying" },
                               { "attempt", 2 }, { "max_attempts", 5 }, { "delay_ms", 500 },
                               { "error_type", "rate_limit_exhausted" },
                               { "error_message", "slow down" }});
            }
            if (armedIncomplete) {
                armedIncomplete = false;
                fakeEmit(out, {{ "type", "done" },
                               { "turn_summary", QJsonObject{
                                     { "possibly_incomplete", true }, { "tool_rounds", 2 } } }});
            }
            continue;
        }
        if (type == QLatin1String("init")) {
            const QString model = obj.value("config").toObject().value("model").toString();
            fakeEmit(out, {{ "type", "ready" }, { "model", model }});
        } else if (type == QLatin1String("load_session")) {
            // 回显快照统计：LOADED:<消息数>:<末尾消息 role>——问题 10 断言
            // "load_session 快照永不含将被 user_msg 重发的末尾 user 记录"
            const QJsonArray msgs = obj.value("messages").toArray();
            QString lastRole = QStringLiteral("none");
            if (!msgs.isEmpty()) {
                lastRole = msgs.last().toObject().value("role").toString();
            }
            fakeEmit(out, {{ "type", "session_loaded" }, { "session_id", obj.value("session_id").toString() }});
            fakeEmit(out, {{ "type", "token" },
                           { "content", QStringLiteral("LOADED:%1:%2").arg(msgs.size()).arg(lastRole) }});
        } else if (type == QLatin1String("user_msg")) {
            const QString content = obj.value("content").toString();
            if (content == QLatin1String("#fake:question")) {
                fakeEmit(out, {{ "type", "question" },
                               { "text", "pick one" },
                               { "options", QJsonArray{ "A", "B" } },
                               { "multi_select", false }});
            } else if (content == QLatin1String("#fake:question_then_error")) {
                // 问题挂起（FIFO 已入队）后回合中途出错——问题 1 的 agentError 清 FIFO 场景
                fakeEmit(out, {{ "type", "question" },
                               { "text", "pick one" },
                               { "options", QJsonArray{ "A", "B" } },
                               { "multi_select", false }});
                fakeEmit(out, {{ "type", "error" },
                               { "message", "fake error after question" },
                               { "error_type", "quota_exhausted" }});
                fakeEmit(out, {{ "type", "done" }});
            } else if (content == QLatin1String("#fake:error")) {
                fakeEmit(out, {{ "type", "error" },
                               { "message", "fake runtime error" },
                               { "error_type", "quota_exhausted" }});
                fakeEmit(out, {{ "type", "done" }});
            } else if (content == QLatin1String("#fake:crash")) {
                return 3;  // 模拟原生崩溃
            } else if (content == QLatin1String("#fake:tool")) {
                // 发起工具调用（全局执行队列全链用例，问题 12）——等 tool_result
                fakeEmit(out, {{ "type", "tool_call" },
                               { "call_id", "call-t1" },
                               { "tool", "fake_tool" },
                               { "arguments", QJsonObject{} }});
            } else if (content == QLatin1String("#fake:arm_retry")) {
                armedRetry = true;  // 回合挂起，等 reconfigure 再发 retrying
            } else if (content == QLatin1String("#fake:arm_incomplete")) {
                // 半截计划文本后挂起，等 reconfigure 再发 done(incomplete)
                fakeEmit(out, {{ "type", "message_end" }, { "content", "I will now generate a chart" }});
                armedIncomplete = true;
            } else if (content == QLatin1String("#fake:dispatch")) {
                // 在途子 Agent 派发（问题 28b）：spawned + 任务 running + 心跳，
                // 不发 done（回合挂起，模拟派发进行中切走/切回）
                fakeEmit(out, {{ "type", "subagent_progress" },
                               { "call_id", "d1" }, { "task_id", "explore #1" },
                               { "subagent", "explore" }, { "state", "spawned" },
                               { "message", "survey task" }});
                fakeEmit(out, {{ "type", "subagent_progress" },
                               { "call_id", "d1" }, { "task_id", "explore #1" },
                               { "state", "running" }, { "message", "working" }});
                fakeEmit(out, {{ "type", "subagent_progress" },
                               { "call_id", "d1" }, { "state", "running" }});  // 心跳：不入缓存
            } else {
                fakeEmit(out, {{ "type", "message_end" }, { "content", "echo:" + content }});
                fakeEmit(out, {{ "type", "done" }});
            }
        } else if (type == QLatin1String("user_answer")) {
            fakeEmit(out, {{ "type", "message_end" }, { "content", "answered" }});
            fakeEmit(out, {{ "type", "done" }});
        } else if (type == QLatin1String("tool_result")) {
            // 工具结果到达（C++ 全局队列执行完毕回传）→ 完成本轮
            fakeEmit(out, {{ "type", "message_end" }, { "content", "tool done" }});
            fakeEmit(out, {{ "type", "done" }});
        } else if (type == QLatin1String("update_tools")) {
            // 工具规格热更新（问题 19）：回显规格数供 Module 广播链断言
            fakeEmit(out, {{ "type", "token" },
                           { "content", QStringLiteral("TOOLS_UPDATED:%1")
                                             .arg(obj.value("tools").toArray().size()) }});
        } else if (type == QLatin1String("stop")) {
            break;
        }
        // 其余类型（reconfigure/update_subagents/tool_exec_queued...）静默忽略
        //（reconfigure 例外见上方 arm 分支）
    }
    return 0;
}

/// 最小假工具：满足 prestartAgent 的"工具已注册"前置条件（L2 温暖化用例），
/// 并供全局执行队列全链用例计数断言（问题 12）。
/// registerTool 不取得所有权（provider 缺失仅告警），测试栈对象即可。
class FakeAgentTool : public DA::DAAbstractAgentTool
{
public:
    DA::DAAgentToolSpec getToolSpec() const override
    {
        DA::DAAgentToolSpec spec;
        spec.name        = QStringLiteral("fake_tool");
        spec.description = QStringLiteral("test fake tool");
        return spec;
    }
    QJsonObject execute(const QJsonObject&) override
    {
        ++mExecCount;
        return QJsonObject{{ "success", true }, { "echo", "fake-result" }};
    }
    QString getOwnerModule() const override
    {
        return QStringLiteral("DAAgentModuleTest");
    }
    int execCount() const { return mExecCount; }

private:
    int mExecCount = 0;
};

} // namespace

// ===========================================================================
// 测试类
// ===========================================================================
class DAAgentModuleTest : public QObject
{
    Q_OBJECT
private:
    static QString pythonConfigPath();
    static QString agentScriptPath();
    // 构造 Module 并注入假 LLM 配置（interpreter 已由 python-config.json 指向测试 exe）
    DA::DAAgentModule* makeModule();
    // 统计会话 JSONL 中指定 type 的记录数
    static int countRecords(const QString& sessionId, const QString& type);

    QByteArray mPythonConfigBackup;
    bool mHadPythonConfig = false;

private Q_SLOTS:
    void initTestCase();    // 备份并改写 python-config.json；保证假脚本存在
    void cleanupTestCase(); // 恢复 python-config.json
    void init();            // 每用例清配置与会话目录 + 清剧本环境变量

    void testStartingClearedOnBusyFalseWithoutReady(); // 问题2：崩溃耗尽终态不残留 starting
    void testSessionListChangedOnBusyFalse();          // 问题6：busy(false) 也刷新角标 payload
    void testNewSessionReassertsBusyFalse();           // 问题14：后台运行中点「+」→ busy(false) 重断言
    void testDeleteCurrentSessionEmitsCleared();       // 问题16：删除当前会话补发 sessionCleared + token 复位
    void testStopDismissesPendingQuestion();           // 问题17：Stop 清问题缓存 + 撤卡
    void testProcessExitDismissesPendingQuestion();    // 问题17：进程退出清问题缓存 + 撤卡
    void testStopClearsToolCallFifo();                 // 问题1：Stop 清 FIFO，幽灵答案不落孤儿 tool_result
    void testErrorClearsToolCallFifo();                // 问题1：错误清 FIFO + 联动撤问题卡
    void testColdStartSnapshotExcludesTrailingUser();  // 问题10：load_session 快照剔除末尾待重发 user
    void testWarmTakeoverLoadsSingleMessageHistory();  // L2：温暖化接管历史恰好 1 条也发 load_session
    void testStopNoopEmitsBusyFalse();                 // L8：stop() 空转回发 busy(false) 解除 Stopping 态
    void testErrorRecordPersisted();                   // 问题3/决策点4：error 记录经 Module 链路落盘 JSONL
    void testBackgroundRetryingReplayedOnSwitchBack(); // 问题8：后台重试条缓存-切回重发
    void testBackgroundIncompleteReplayedOnSwitchBack(); // 问题8：后台"话说一半"提醒缓存-豁免退役-切回重发
    void testToolExecutionViaGlobalQueue();            // 问题12：Module 全链（队列执行+落盘+排队信号）
    void testRegisterToolBroadcastsUpdateTools();      // 问题19：注册工具向存活桥广播 update_tools
    void testSubagentProgressReplayedOnSwitchBack();   // 问题28b：在途子 Agent 进度切回重放
    void testStopSessionStopsBackgroundBridge();       // L14：stopSession 直达后台会话桥
    void testNewSessionRetiresIdleBridge();            // 问题18：newSession 切离退役空闲桥
    void testForeignRunningSessionsNotified();         // 问题18/决策点5：跨工程存活会话通知
};

QString DAAgentModuleTest::pythonConfigPath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/python-config.json");
}

QString DAAgentModuleTest::agentScriptPath()
{
    return QCoreApplication::applicationDirPath()
           + QStringLiteral("/PyScripts/DAWorkbench/agent/agent_runner.py");
}

DA::DAAgentModule* DAAgentModuleTest::makeModule()
{
    auto* m = new DA::DAAgentModule(nullptr, nullptr);
    m->initialize(nullptr);
    DA::DAAgentLLMConfig cfg;
    cfg.setBaseUrl(QStringLiteral("http://fake.local"));
    cfg.setApiKey(QStringLiteral("fake-key"));
    cfg.setModel(QStringLiteral("fake-model"));
    cfg.setReadyTimeoutSec(15);
    cfg.setStopTimeoutSec(2);
    cfg.setInactivityTimeoutSec(120);
    // 崩溃自愈重启压到 0：崩溃即直达 crash_exhausted 终态（恢复链本身由
    // DAAgentBridgeTest 覆盖，Module 级测试只关心终态记账，且免于 1s×N 等待）
    cfg.setMaxSubprocessRestarts(0);
    m->setLLMConfig(cfg);
    return m;
}

int DAAgentModuleTest::countRecords(const QString& sessionId, const QString& type)
{
    DA::DAAgentSessionStore store;
    int n = 0;
    const QVector<QJsonObject> records = store.readAllRecords(sessionId);
    for (const QJsonObject& r : records) {
        if (r.value("type").toString() == type) {
            ++n;
        }
    }
    return n;
}

void DAAgentModuleTest::initTestCase()
{
    // 备份既有 python-config.json（同目录可能部署了真实程序），写入指向测试 exe
    // 自身的解释器配置——Module::detectPythonExePath 据此把假 agent 子进程化
    const QString cfgPath = pythonConfigPath();
    QFile cfgFile(cfgPath);
    if (cfgFile.exists() && cfgFile.open(QIODevice::ReadOnly)) {
        mPythonConfigBackup = cfgFile.readAll();
        mHadPythonConfig = true;
        cfgFile.close();
    }
    QJsonObject root;
    root[QStringLiteral("config")] = QJsonObject{
        { QStringLiteral("interpreter"), QCoreApplication::applicationFilePath() }
    };
    QVERIFY(cfgFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    cfgFile.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    cfgFile.close();

    // 保证 agent 脚本存在（部署目录通常已带真实脚本；假 agent 不读其内容，
    // 仅需通过 createBridgeForSession 的 QFile::exists 校验）
    const QString scriptPath = agentScriptPath();
    if (!QFile::exists(scriptPath)) {
        QDir().mkpath(QFileInfo(scriptPath).absolutePath());
        QFile f(scriptPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write("# fake agent_runner.py placeholder for DAAgentModuleTest\n");
        f.close();
    }
}

void DAAgentModuleTest::cleanupTestCase()
{
    // 恢复 python-config.json：有备份还原备份，无备份删除测试写入的文件
    const QString cfgPath = pythonConfigPath();
    QFile::remove(cfgPath);
    if (mHadPythonConfig) {
        QFile f(cfgPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(mPythonConfigBackup);
            f.close();
        }
    }
}

void DAAgentModuleTest::init()
{
    qunsetenv("DA_FAKE_SCENARIO");
    // 清 agent 配置文件（仿 DAAgentPermissionManagerTest::init）
    const QString cfgDir = DA::DADir::getConfigPath();
    QFile::remove(cfgDir + "/agent-permissions.json");
    QFile::remove(cfgDir + "/agent-permissions.json.tmp");
    QFile::remove(cfgDir + "/agent-config.json");
    QFile::remove(cfgDir + "/agent-config.json.tmp");
    QFile::remove(cfgDir + "/agent-config.ini");
    QFile::remove(cfgDir + "/agent-config.ini.bak");
    // 清空 sessions 目录（仿 DAAgentSessionStoreTest::init）
    QDir qd(DA::DADir::getAppDataPath(QStringLiteral("sessions")));
    if (qd.exists()) {
        const auto files = qd.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        for (const QFileInfo& fi : files) {
            QFile::remove(fi.absoluteFilePath());
        }
    }
}

/**
 * 问题2：mSessionStarting 在"error + busy(false) 但无 ready"的终态
 * （crash_exhausted）后永久残留——角标卡 "starting"、切离守卫拒绝退役死桥、
 * 切入会话时 Dock 输入永久冻结（死锁闭环）。修复后 agentBusy lambda 的
 * busy(false) 分支 remove mSessionStarting（与 Dock 侧兜底同构），终态角标
 * 应显示 "error"。
 */
void DAAgentModuleTest::testStartingClearedOnBusyFalseWithoutReady()
{
    qputenv("DA_FAKE_SCENARIO", "crash-at-init");
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    const QString sid = module->createSession();
    QVERIFY(!sid.isEmpty());

    module->sendMessage(QStringLiteral("hello"));
    // 冷启动即进入 starting；崩溃循环耗尽后终态 = error + busy(false)、无 ready
    QCOMPARE(module->sessionRuntimeState(sid), QStringLiteral("starting"));

    // 修复前：mSessionStarting 无人清除 → 状态永久 "starting"（本断言超时失败）
    QTRY_COMPARE_WITH_TIMEOUT(module->sessionRuntimeState(sid), QStringLiteral("error"), 60000);

    module->shutdown();
}

/**
 * 问题6：Bridge 错误路径（crash_exhausted 等）只发 error+busy(false) 不发
 * agentDone——修复前 agentBusy lambda 仅 busy(true) 时 emit sessionListChanged，
 * "running"→"error" 的角标变化不刷新，会话管理器要等下一个事件才更新。
 * 修复后 busy(false) 同样刷新：错误终态的最后一帧 payload state=="error"。
 */
void DAAgentModuleTest::testSessionListChangedOnBusyFalse()
{
    qputenv("DA_FAKE_SCENARIO", "crash-at-init");
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy listSpy(module.data(), &DA::DAAgentInterface::sessionListChanged);
    const QString sid = module->createSession();
    module->sendMessage(QStringLiteral("hello"));

    QTRY_COMPARE_WITH_TIMEOUT(module->sessionRuntimeState(sid), QStringLiteral("error"), 60000);

    // 错误终态由 error+busy(false) 同步连发（同一栈），最后一帧 payload 必须
    // 携带 "error" 角标（修复前停留在 busy(true) 时的 "starting"）
    QVERIFY(!listSpy.isEmpty());
    const QVariantList lastPayload = listSpy.last().at(0).toList();
    QString state;
    for (const QVariant& v : lastPayload) {
        const QVariantMap vm = v.toMap();
        if (vm.value(QStringLiteral("id")).toString() == sid) {
            state = vm.value(QStringLiteral("state")).toString();
        }
    }
    QCOMPARE(state, QStringLiteral("error"));

    module->shutdown();
}

/**
 * 问题14：后台会话 A 运行中点「+」新建会话 B——A 跑完时的 busy(false) 被
 * 活跃会话过滤挡掉（A≠B），修复前 newSession 无任何状态重断言，Dock 的
 * busy 守卫永不复位：新空会话显示 thinking、输入禁用、Stop 空转，用户被
 * 永久卡住。修复后 newSession 复用 switchSession step4 的重断言语义
 * （reassertActiveSessionState），Dock 侧 onSessionCreated/onSessionCleared
 * 同步复位（防御对称）。
 */
void DAAgentModuleTest::testNewSessionReassertsBusyFalse()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy busySpy(module.data(), &DA::DAAgentInterface::agentBusy);
    QSignalSpy questionSpy(module.data(), &DA::DAAgentInterface::agentQuestion);
    const QString sidA = module->createSession();
    QVERIFY(!sidA.isEmpty());

    // 会话 A 进入运行并挂起在 ask_user（busy=true 且无 done；角标 running 优先
    // 于 waiting_input——sessionRuntimeState 按 starting/running/等待输入排序）
    module->sendMessage(QStringLiteral("#fake:question"));
    QVERIFY(questionSpy.wait(30000));
    QVERIFY(!module->sessionRuntimeState(sidA).isEmpty());

    // 后台 A 挂起时点「+」新建会话 B（复用判断因 busy/pending 不成立 → 真新建）
    module->newSession();
    const QString sidB = module->currentSessionId();
    QVERIFY(sidB != sidA);

    // 修复核心断言：newSession 后重断言 busy(false)（修复前 busySpy 停留在 true）
    QVERIFY(!busySpy.isEmpty());
    QCOMPARE(busySpy.last().at(0).toBool(), false);
    // 新会话 B 无运行态
    QCOMPARE(module->sessionRuntimeState(sidB), QString());

    module->shutdown();
}

/**
 * 问题16：删除当前会话后 Dock 状态孤儿化——修复前 deleteSession 只发
 * sessionListChanged（Dock 仅缓存 payload+刷标题，不校验当前会话仍在列表），
 * 聊天区保留已删会话全部气泡；下一条消息新建会话（createSession 有意不发
 * sessionCreated）后新旧两个会话内容视觉混合，token 标签残留旧值。
 * 修复后补发 sessionCleared（Dock 已有完整处理槽）+ token UI 复位。
 */
void DAAgentModuleTest::testDeleteCurrentSessionEmitsCleared()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy clearedSpy(module.data(), &DA::DAAgentInterface::sessionCleared);
    QSignalSpy tokenSpy(module.data(), &DA::DAAgentInterface::tokenUsageUpdated);

    // 删除当前会话：sessionCleared 必发、current 清空、token 复位全 0
    const QString sidA = module->createSession();
    module->deleteSession(sidA);
    QCOMPARE(clearedSpy.count(), 1);
    QVERIFY(module->currentSessionId().isEmpty());
    QVERIFY(!tokenSpy.isEmpty());
    QCOMPARE(tokenSpy.last().at(0).toInt(), 0);
    QCOMPARE(tokenSpy.last().at(1).toInt(), 0);
    QCOMPARE(tokenSpy.last().at(2).toInt(), 0);

    // 删除非当前会话：不触发 sessionCleared（聊天区不受影响）
    const QString sidB = module->createSession();
    const QString sidC = module->createSession();  // sidC 为当前
    module->deleteSession(sidB);
    QCOMPARE(clearedSpy.count(), 1);
    QCOMPARE(module->currentSessionId(), sidC);

    module->shutdown();
}

/**
 * 问题17（Stop 路径）：ask_user 挂起时用户 Stop——修复前 mPendingQuestions
 * 只在 sendUserAnswer/retireBridge 清除，Stop 后缓存残留：角标卡死
 * waiting_input、切回重发幽灵问题卡、用户作答落盘孤儿 tool_result。
 * 修复后 stop() 同步清缓存并 emit agentQuestionDismissed（UI 撤卡）。
 */
void DAAgentModuleTest::testStopDismissesPendingQuestion()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy questionSpy(module.data(), &DA::DAAgentInterface::agentQuestion);
    QSignalSpy dismissSpy(module.data(), &DA::DAAgentInterface::agentQuestionDismissed);
    const QString sid = module->createSession();
    module->sendMessage(QStringLiteral("#fake:question"));
    QVERIFY(questionSpy.wait(30000));

    module->stop();
    // 撤卡信号必发（镜像审批 dismissed 契约），缓存清除后角标不再 waiting_input
    QCOMPARE(dismissSpy.count(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(module->sessionRuntimeState(sid) != QStringLiteral("waiting_input"), 15000);

    module->shutdown();
}

/**
 * 问题17（进程退出路径）：ask_user 挂起时子进程死亡（崩溃/被杀）——
 * processExited lambda（补捕获 sessionId）清缓存 + 撤卡。修复前该 lambda
 * 只清权限记忆，问题缓存无人作废。
 */
void DAAgentModuleTest::testProcessExitDismissesPendingQuestion()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy questionSpy(module.data(), &DA::DAAgentInterface::agentQuestion);
    QSignalSpy dismissSpy(module.data(), &DA::DAAgentInterface::agentQuestionDismissed);
    const QString sid = module->createSession();
    module->sendMessage(QStringLiteral("#fake:question"));
    QVERIFY(questionSpy.wait(30000));

    // 问题挂起时进程崩溃（maxSubprocessRestarts=0 → 直达 exhausted 终态）
    module->sendMessage(QStringLiteral("#fake:crash"));
    QTRY_COMPARE_WITH_TIMEOUT(dismissSpy.count(), 1, 30000);
    QTRY_VERIFY_WITH_TIMEOUT(module->sessionRuntimeState(sid) != QStringLiteral("waiting_input"), 15000);

    module->shutdown();
}

/**
 * 问题1（Stop 路径）：ask_user 的 tool_call uuid 已入会话 FIFO，用户按 Stop
 * 后 FIFO 必须清空——否则再发"继续"开启新一轮，下一轮的 tool_result 出队时
 * 配到旧 uuid，JSONL 中 tool_result.tool_call_id 挂错（永久污染）。
 * 联动断言（问题 17 撤卡后）：幽灵答案经 sendUserAnswer 不落盘孤儿记录。
 */
void DAAgentModuleTest::testStopClearsToolCallFifo()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy questionSpy(module.data(), &DA::DAAgentInterface::agentQuestion);
    const QString sid = module->createSession();
    module->sendMessage(QStringLiteral("#fake:question"));
    QVERIFY(questionSpy.wait(30000));

    module->stop();
    // 幽灵答案：FIFO 已清 → sendUserAnswer 空队列跳过持久化（修复前出队
    // 残留 uuid 落盘 1 条孤儿 tool_result，Python 从未收到）
    module->sendUserAnswer(QStringLiteral("ghost answer"));
    QCOMPARE(countRecords(sid, QStringLiteral("tool_result")), 0);

    module->shutdown();
}

/**
 * 问题1（错误路径）：回合中途出错（问题挂起后 error+done）——agentError
 * lambda 清 FIFO + 联动清问题缓存/撤卡（三处同批：只清 FIFO 不清卡会因
 * "答案消失"产生新症状）。幽灵答案不落盘孤儿 tool_result。
 */
void DAAgentModuleTest::testErrorClearsToolCallFifo()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy questionSpy(module.data(), &DA::DAAgentInterface::agentQuestion);
    QSignalSpy errSpy(module.data(), &DA::DAAgentInterface::agentError);
    QSignalSpy dismissSpy(module.data(), &DA::DAAgentInterface::agentQuestionDismissed);
    const QString sid = module->createSession();
    module->sendMessage(QStringLiteral("#fake:question_then_error"));
    QVERIFY(questionSpy.wait(30000));
    // question/error/done 由假 agent 连发，同一事件批次内到达——wait() 只等
    // "将来"的发射，用 QTRY 断言计数避免经典 QSignalSpy 时序陷阱
    QTRY_VERIFY_WITH_TIMEOUT(errSpy.count() >= 1, 30000);

    // 错误联动：问题缓存清除 + 活跃会话撤卡
    QCOMPARE(dismissSpy.count(), 1);
    // 幽灵答案不落盘孤儿 tool_result（修复前 FIFO 残留 uuid 被出队配对）
    module->sendUserAnswer(QStringLiteral("ghost answer"));
    QCOMPARE(countRecords(sid, QStringLiteral("tool_result")), 0);

    module->shutdown();
}

/**
 * 问题10：冷启动 sendMessage 的持久化先于桥启动（有意设计），load_session
 * 快照末尾必然是刚落盘的本轮 user 记录，随后 user_msg 又注入同一文本——
 * LLM 上下文中当前提问出现两遍（必然触发，非概率性）。统一约定修复：
 * 快照永不含将被重发的末尾 user 记录。假 agent 在 load_session 时回显
 * token("LOADED:<消息数>:<末尾role>")，据此断言。
 */
void DAAgentModuleTest::testColdStartSnapshotExcludesTrailingUser()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy doneSpy(module.data(), &DA::DAAgentInterface::agentDone);
    QSignalSpy tokenSpy(module.data(), &DA::DAAgentInterface::agentToken);

    // 第一轮：user(m1) + assistant(echo:m1) 落盘
    const QString sidA = module->createSession();
    QVERIFY(!sidA.isEmpty());
    module->sendMessage(QStringLiteral("m1"));
    QVERIFY(doneSpy.wait(30000));

    // 停掉进程再发消息：sendMessage 防御分支退役死桥并冷启动新桥
    //（与"切离桥退役后切回再发"“Stop/崩溃后重建再发"同为必然触发场景）
    module->stop();

    // 冷启动发 m2：快照 = [user(m1), ai(echo)]（剔除刚落盘的 user(m2)）
    module->sendMessage(QStringLiteral("m2"));
    QVERIFY(tokenSpy.wait(30000));
    // 修复前为 "LOADED:3:human"（快照含 m2，user_msg 再注入一遍 → 上下文重复提问）
    QCOMPARE(tokenSpy.at(0).at(0).toString(), QStringLiteral("LOADED:2:ai"));

    module->shutdown();
}

/**
 * L2：温暖化接管（switchSession step6 接管预热空闲桥）时历史恰好 1 条——
 * 修复前判定 messageCount > 1 不发 load_session，该条历史丢失出上下文
 * （如"发消息后进程崩溃、无应答"的会话仅剩 1 条 user 记录）。问题 10 的
 * 统一快照逻辑已把判定改为"快照非空"，本用例锁定该行为。
 */
void DAAgentModuleTest::testWarmTakeoverLoadsSingleMessageHistory()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    FakeAgentTool fakeTool;
    QVERIFY(module->registerTool(&fakeTool));  // prestartAgent 前置：工具已注册
    QSignalSpy tokenSpy(module.data(), &DA::DAAgentInterface::agentToken);

    // 会话 A 仅 1 条 user 记录：发消息后进程崩溃、无 assistant 应答
    const QString sidA = module->createSession();
    module->sendMessage(QStringLiteral("#fake:crash"));
    QTRY_COMPARE_WITH_TIMEOUT(module->sessionRuntimeState(sidA), QStringLiteral("error"), 30000);

    // 换到 B 再切回，使 A 的死桥经 switchSession step1 退役（映射清空）
    const QString sidB = module->createSession();  // current=B（A 死桥仍在映射）
    QVERIFY(module->switchSession(sidA));          // current=A（死桥保留，step6 不接管）
    QVERIFY(module->switchSession(sidB));          // step1 退役 A 死桥，current=B

    // 预热空闲桥 → 切回 A 触发 step6 温暖化接管 → load_session 下发 1 条历史
    module->prestartAgent();
    QVERIFY(module->switchSession(sidA));

    // 假 agent 回显快照统计：修复前 messageCount==1 不发 load_session（token 永不到达）
    QVERIFY(tokenSpy.wait(30000));
    QCOMPARE(tokenSpy.at(0).at(0).toString(), QStringLiteral("LOADED:1:human"));

    module->shutdown();
}

/**
 * L8：Stop 空转（无桥/桥未运行——完成竞态或死桥场景）时 Module::stop()
 * 不发任何信号，而 Dock onStopClicked 已进入 Stopping 过渡态（按钮+输入
 * 禁用）——UI 冻结直到外部信号拯救。修复后空转分支回发 busy(false)
 * （Dock 侧同步加 busy/starting 守卫，双侧防御）。
 */
void DAAgentModuleTest::testStopNoopEmitsBusyFalse()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy busySpy(module.data(), &DA::DAAgentInterface::agentBusy);
    const QString sid = module->createSession();
    QVERIFY(!sid.isEmpty());

    // 无桥空闲 Stop：no-op 但必须回发 busy(false) 解除 Dock Stopping 过渡态
    module->stop();
    QCOMPARE(busySpy.count(), 1);
    QCOMPARE(busySpy.at(0).at(0).toBool(), false);

    module->shutdown();
}

/**
 * 问题3/决策点4：错误此前从不持久化（JSONL 无 error 记录类型）——后台会话
 * 错误切回丢失、活跃错误切离切回后从重放消失、重启后全无痕迹。修复后
 * agentError 持久化按桥所属会话无条件写盘 type="error" 记录（载荷
 * message/error_type/detail），readMessagesForLoad 过滤不进 Python state，
 * 重放经 WebChannel/chat.js 渲染错误卡。
 */
void DAAgentModuleTest::testErrorRecordPersisted()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy errSpy(module.data(), &DA::DAAgentInterface::agentError);
    const QString sid = module->createSession();
    module->sendMessage(QStringLiteral("#fake:error"));
    QTRY_VERIFY_WITH_TIMEOUT(errSpy.count() >= 1, 30000);

    // error 记录已落盘（修复前 JSONL 无任何痕迹）
    QCOMPARE(countRecords(sid, QStringLiteral("error")), 1);

    // 载荷完整 + 不进 Python state + messageCount 不计
    DA::DAAgentSessionStore store;
    const QVector<QJsonObject> records = store.readAllRecords(sid);
    bool found = false;
    for (const QJsonObject& r : records) {
        if (r.value("type").toString() != QLatin1String("error")) {
            continue;
        }
        found = true;
        const QJsonObject msg = r.value("message").toObject();
        QCOMPARE(msg.value("message").toString(), QStringLiteral("fake runtime error"));
        QCOMPARE(msg.value("error_type").toString(), QStringLiteral("quota_exhausted"));
        QCOMPARE(r.value("session_id").toString(), sid);
    }
    QVERIFY(found);
    QCOMPARE(store.messageCount(sid), 1);  // 仅 user 1 条（error 轮无 assistant）
    const QJsonArray msgs = store.readMessagesForLoad(sid);
    QCOMPARE(msgs.size(), 1);
    QCOMPARE(msgs.at(0).toObject().value("role").toString(), QStringLiteral("human"));

    module->shutdown();
}

/**
 * 问题8（重试条）：后台会话经历 LLM 重试时，修复前 agentRetrying 被活跃
 * 会话过滤后直接丢弃——切回看不到重试条。修复后缓存最新一条，切回重发。
 * 构造：A 发 arm 指令挂起回合 → 切到 B → setLLMConfig 广播 reconfigure →
 * A 的假 agent 发 retrying（后台身份到达）→ 切回 A 断言重发。
 */
void DAAgentModuleTest::testBackgroundRetryingReplayedOnSwitchBack()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy retrySpy(module.data(), &DA::DAAgentInterface::agentRetrying);
    const QString sidA = module->createSession();
    module->sendMessage(QStringLiteral("#fake:arm_retry"));  // 回合挂起（busy=true）
    const QString sidB = module->createSession();            // current=B，A 转后台
    QVERIFY(sidB != sidA);

    // reconfigure 广播 → A 的假 agent 发 retrying → 后台缓存（不转发）
    DA::DAAgentLLMConfig cfg;
    cfg.setMaxRetries(6);
    module->setLLMConfig(cfg);
    QTest::qWait(2000);  // 等待本地子进程往返（retrying 到达并被缓存）
    QCOMPARE(retrySpy.count(), 0);  // 后台期间不转发

    // 切回 A：重发缓存的重试条（一次性，重发即清）
    QVERIFY(module->switchSession(sidA));
    QCOMPARE(retrySpy.count(), 1);
    QCOMPARE(retrySpy.at(0).at(0).toInt(), 2);   // attempt
    QCOMPARE(retrySpy.at(0).at(1).toInt(), 5);   // maxAttempts
    QCOMPARE(retrySpy.at(0).at(3).toString(), QStringLiteral("rate_limit_exhausted"));

    module->shutdown();
}

/**
 * 问题8（话说一半提醒）：后台会话回合疑似未完成时，修复前提醒被丢弃且
 * 会话随 agentDone 立即退役。修复后：incomplete 提醒缓存 + 退役豁免
 * （agentDone/切离守卫都检查），切回重发 agentTurnPossiblyIncomplete +
 * systemMessage 提醒卡（提示用户发"继续"）。
 */
void DAAgentModuleTest::testBackgroundIncompleteReplayedOnSwitchBack()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy incompleteSpy(module.data(), &DA::DAAgentInterface::agentTurnPossiblyIncomplete);
    QSignalSpy sysMsgSpy(module.data(), &DA::DAAgentInterface::systemMessage);
    const QString sidA = module->createSession();
    module->sendMessage(QStringLiteral("#fake:arm_incomplete"));  // 半截计划后挂起
    const QString sidB = module->createSession();                 // current=B，A 转后台
    QVERIFY(sidB != sidA);

    // reconfigure 广播 → A 的假 agent 发 done(incomplete) → 后台：提醒缓存 +
    // agentDone 豁免退役（桥保活，等切回重发）
    DA::DAAgentLLMConfig cfg;
    cfg.setMaxRetries(6);
    module->setLLMConfig(cfg);
    QTest::qWait(2000);
    QCOMPARE(incompleteSpy.count(), 0);  // 后台期间不转发

    // 切回 A：重发提醒（信号 + systemMessage 文案）
    QVERIFY(module->switchSession(sidA));
    QCOMPARE(incompleteSpy.count(), 1);
    QCOMPARE(incompleteSpy.at(0).at(0).toInt(), 2);  // toolRounds
    bool hasReminder = false;
    for (const auto& args : std::as_const(sysMsgSpy)) {
        if (args.at(0).toString().contains(QStringLiteral("unfinished"))) {
            hasReminder = true;
        }
    }
    QVERIFY(hasReminder);

    module->shutdown();
}

/**
 * 问题12（决策点 2 方案 c）Module 全链：attachBridge 注入全局执行队列后，
 * tool_call 经"权限门 → 入队（排队信号 position≥1）→ 出队存活检查 →
 * 执行（position=0）→ 结果回传 → agentToolResult 转发 + JSONL 落盘"。
 */
void DAAgentModuleTest::testToolExecutionViaGlobalQueue()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    FakeAgentTool fakeTool;
    QVERIFY(module->registerTool(&fakeTool));
    QSignalSpy resultSpy(module.data(), &DA::DAAgentInterface::agentToolResult);
    QSignalSpy queuedSpy(module.data(), &DA::DAAgentInterface::agentToolQueued);
    QSignalSpy doneSpy(module.data(), &DA::DAAgentInterface::agentDone);

    const QString sid = module->createSession();
    module->sendMessage(QStringLiteral("#fake:tool"));

    // 工具经全局队列真实执行，结果转发 + 回合完成
    QVERIFY(resultSpy.wait(30000));
    QCOMPARE(resultSpy.at(0).at(0).toString(), QStringLiteral("fake_tool"));
    QCOMPARE(fakeTool.execCount(), 1);
    QVERIFY(doneSpy.wait(30000));

    // 排队态信号（活跃会话转发）：入队 position=1 + 出队 position=0
    QVERIFY(queuedSpy.count() >= 2);
    QCOMPARE(queuedSpy.at(0).at(1).toInt(), 1);
    QCOMPARE(queuedSpy.at(1).at(1).toInt(), 0);

    // JSONL 落盘：tool_call 记录（type=assistant 带 tool_calls）+ tool_result
    // + 回合收尾 message_end（type=assistant）= assistant 2 条、tool_result 1 条
    QTRY_COMPARE(countRecords(sid, QStringLiteral("tool_result")), 1);
    QCOMPARE(countRecords(sid, QStringLiteral("assistant")), 2);

    module->shutdown();
}

/**
 * 问题19：registerTool/unregisterToolsByProvider 的 forEachLiveBridge 广播
 * 此前只 setTools（C++ 执行表）——Python/LLM 规格面停留在 init 时刻，插件
 * 热插拔后存活桥看不到新工具/仍调用已移除工具。修复后广播同时下发
 * update_tools 全量规格（假 agent 回显 token "TOOLS_UPDATED:<n>"，经活跃
 * 会话 agentToken 转发链路可观测）。
 */
void DAAgentModuleTest::testRegisterToolBroadcastsUpdateTools()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy tokenSpy(module.data(), &DA::DAAgentInterface::agentToken);
    QSignalSpy doneSpy(module.data(), &DA::DAAgentInterface::agentDone);

    // 先让会话桥存活（跑完一轮不退役）
    const QString sid = module->createSession();
    module->sendMessage(QStringLiteral("warm up"));
    QVERIFY(doneSpy.wait(30000));

    // 桥存活时注册工具 → 广播 update_tools（1 条规格）→ 假 agent 回显
    FakeAgentTool fakeTool;
    QVERIFY(module->registerTool(&fakeTool));
    QVERIFY(tokenSpy.wait(30000));
    bool seen = false;
    for (const auto& args : std::as_const(tokenSpy)) {
        if (args.at(0).toString() == QLatin1String("TOOLS_UPDATED:1")) {
            seen = true;
        }
    }
    QVERIFY(seen);

    module->shutdown();
}

/**
 * 问题28b：切回运行中会话时 clearChat 已复位前端 subagentCards，而 Module
 * 此前只缓存/重发 questions 与 approvals——在途派发的 running/终态进度全部
 * 因"无 entry 且非 spawned"被忽略，进度卡永不重建，整个派发过程切回后完全
 * 不可见。修复后 Module 按会话缓存本轮进度事件（心跳除外），switchSession
 * 切回逐条重放（chat.js 幂等重建 + 惰性建行双保险）；done/error/退役即清。
 */
void DAAgentModuleTest::testSubagentProgressReplayedOnSwitchBack()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy progressSpy(module.data(), &DA::DAAgentInterface::agentSubagentProgress);
    const QString sidA = module->createSession();
    module->sendMessage(QStringLiteral("#fake:dispatch"));

    // 活跃期间转发 3 条（spawned + 任务 running + 心跳），回合挂起不 done
    QTRY_VERIFY_WITH_TIMEOUT(progressSpy.count() >= 3, 30000);
    const int liveCount = progressSpy.count();

    // 切走（A busy → 桥保留后台）再切回：重放缓存的 2 条（心跳不入缓存）
    const QString sidB = module->createSession();
    QVERIFY(!sidB.isEmpty());
    QVERIFY(module->switchSession(sidA));
    QTRY_VERIFY_WITH_TIMEOUT(progressSpy.count() >= liveCount + 2, 15000);

    // 重放首条为 spawned 事件（完整载荷供前端重建进度卡）
    const QJsonObject first = progressSpy.at(liveCount).at(0).toJsonObject();
    QCOMPARE(first.value(QStringLiteral("state")).toString(), QStringLiteral("spawned"));
    QCOMPARE(first.value(QStringLiteral("call_id")).toString(), QStringLiteral("d1"));

    module->shutdown();
}

/**
 * L14（决策点 5 联动）：失控后台会话的停止入口——修复前必须先切换过去
 * 再按 Stop（结合问题 2 的切入冻结场景，starting 残留会话切过去也停不了）。
 * stopSession 直达该会话的桥：requestStop + 清挂起缓存；后台会话不 emit
 * agentQuestionDismissed（非活跃无卡可撤），仅刷角标。空闲/无桥/空 id
 * 静默 no-op。
 */
void DAAgentModuleTest::testStopSessionStopsBackgroundBridge()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy questionSpy(module.data(), &DA::DAAgentInterface::agentQuestion);
    QSignalSpy dismissSpy(module.data(), &DA::DAAgentInterface::agentQuestionDismissed);

    // 会话 A 挂起在 ask_user（桥存活），随后 B 成为活跃会话（A 转后台）
    const QString sidA = module->createSession();
    module->sendMessage(QStringLiteral("#fake:question"));
    QVERIFY(questionSpy.wait(30000));
    const QString sidB = module->createSession();
    QVERIFY(sidB != sidA);

    // 停止后台会话 A：进程终止、挂起问题缓存清除、角标恢复空闲
    module->stopSession(sidA);
    QTRY_COMPARE_WITH_TIMEOUT(module->sessionRuntimeState(sidA), QString(), 15000);
    // A 非活跃会话：无卡可撤，不发 dismiss（撤卡信号只服务活跃会话 UI）
    QCOMPARE(dismissSpy.count(), 0);

    // 防御 no-op：无桥会话 / 空 id 不崩溃、无副作用
    module->stopSession(sidB);
    module->stopSession(QString());

    module->shutdown();
}

/**
 * 问题18（资源累积面）：活跃会话跑完后桥不退役（免下轮冷启动），此时点「+」
 * 新建会话不走 switchSession——修复前旧会话空闲桥永久滞留，反复"聊一轮→
 * 点+→聊一轮"累积 N 个空闲 Python 子进程（每个数百 MB 级）直到应用关闭。
 * 修复后 newSession 真新建路径复用切离退役判定（空闲无挂起→retire）。
 */
void DAAgentModuleTest::testNewSessionRetiresIdleBridge()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy doneSpy(module.data(), &DA::DAAgentInterface::agentDone);
    const QString sidA = module->createSession();
    module->sendMessage(QStringLiteral("one round"));
    QVERIFY(doneSpy.wait(30000));
    QVERIFY(module->isRunning());  // A 的空闲桥存活（活跃会话跑完不退役）

    // 点「+」：A 非空（messageCount=1）不复用 → 真新建 + 切离退役 A 的空闲桥
    module->newSession();
    QVERIFY(module->currentSessionId() != sidA);
    // A 桥已退役（映射移除）——无任何存活子进程
    QTRY_VERIFY_WITH_TIMEOUT(!module->isRunning(), 15000);

    module->shutdown();
}

/**
 * 问题18/决策点 5 方案 c（可见性面）：打开工程 P2 时绑定 P1 的存活桥保持
 * 运行（不腰斩长任务），但工程过滤列表使其不可见/角标不可达/无法切入停止。
 * 修复后经 foreignAgentSessionsRunning 通知 UI（提示条 + 全部工程视图 +
 * 一键停止）；桥死亡/停止后通知空列表（提示条隐藏）。
 */
void DAAgentModuleTest::testForeignRunningSessionsNotified()
{
    QScopedPointer<DA::DAAgentModule> module(makeModule());
    QSignalSpy foreignSpy(module.data(), &DA::DAAgentInterface::foreignAgentSessionsRunning);
    QSignalSpy questionSpy(module.data(), &DA::DAAgentInterface::agentQuestion);

    // 工程 P1 下的会话挂起在 ask_user（桥长期存活）
    module->setCurrentProjectPath(QStringLiteral("C:/fake-proj/P1.daProject"));
    const QString sidA = module->createSession();
    module->sendMessage(QStringLiteral("#fake:question"));
    QVERIFY(questionSpy.wait(30000));

    // 打开工程 P2：不退役任何桥，通知跨工程存活会话（含 sidA）
    module->setCurrentProjectPath(QStringLiteral("C:/fake-proj/P2.daProject"));
    QVERIFY(!foreignSpy.isEmpty());
    QVariantList last = foreignSpy.last().at(0).toList();
    QCOMPARE(last.size(), 1);
    QCOMPARE(last.at(0).toMap().value(QStringLiteral("id")).toString(), sidA);
    QCOMPARE(last.at(0).toMap().value(QStringLiteral("projectPath")).toString(),
             QStringLiteral("C:/fake-proj/P1.daProject"));

    // 一键停止（跨工程视图入口，L14 stopSession）→ 通知空列表（提示条隐藏）
    module->stopSession(sidA);
    QTRY_VERIFY_WITH_TIMEOUT(foreignSpy.last().at(0).toList().isEmpty(), 15000);

    module->shutdown();
}

int main(int argc, char* argv[])
{
    // 假 agent 子进程模式：Module 以 "测试exe agent_runner.py" 启动子进程，
    // 在创建测试对象前拦截（同步 stdio 循环，无需事件循环）；剧本经环境变量
    // DA_FAKE_SCENARIO 选择（QProcess 默认继承父进程环境）
    for (int i = 1; i < argc; ++i) {
        const QByteArray arg(argv[i]);
        if (arg.startsWith("--fake-agent=")) {
            return fakeAgentMain(arg.mid(qstrlen("--fake-agent=")));
        }
        if (arg.endsWith("agent_runner.py")) {
            const QByteArray scenario = qgetenv("DA_FAKE_SCENARIO");
            return fakeAgentMain(scenario.isEmpty() ? QByteArray("module-echo") : scenario);
        }
    }

    // 必须在 QCoreApplication 与任何 DADir 路径查询之前打开 test 模式
    //（AppData/Config 重定向到临时目录，不污染真实 %APPDATA%）
    QStandardPaths::setTestModeEnabled(true);

    QCoreApplication app(argc, argv);
    DAAgentModuleTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "main.moc"
