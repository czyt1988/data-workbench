// DAAgentBridgeTest/main.cpp
// 单元测试：DAAgentBridge 子进程协议级行为（agent-concurrent-refactor-audit 整改验证）
//
// 测试基建（假 agent）：DAAgentBridge::startAgent 以 QProcess 启动
// "pythonExePath agentScriptPath"，本测试把 pythonExePath 指向测试 exe 自身、
// agentScriptPath 设为 "--fake-agent=<scenario>"。子进程模式在 main() 最前端
// 拦截（不创建 QTest 对象），用同步 stdio 循环按剧本收发 JSON Lines 协议消息，
// 模拟 ready/error/done/崩溃等序列——不依赖真实 Python 环境，协议级可复现。
//
// 剧本清单（随整改批次逐个补充）：
//   init-error    : 发 error(init_failed) 后立即退出（模拟 init 失败 main() return）
//   runtime-error : ready 后第一条 user_msg 回 error(quota_exhausted)+done 但进程
//                   保持存活；第二条 user_msg 正常回 token+done（验证写通道未关）
//   never-ready   : 永不发 ready/booting（模拟环境性失败，验证 ready 超时路径）
//   echo-model-crash-on-msg : init.model 经 ready 回显；user_msg 即崩溃 exit(3)
//                   （验证崩溃自愈重启所用配置缓存，问题 22）
//   done-then-idle-crash : user_msg 回 done 后立即 exit(42)（模拟回合完成后
//                   空闲期崩溃，验证自愈不重发已回答消息，问题 11）
//   ready-after-first-msg : 收到第一条 user_msg 才发 ready（复现冷启动时序，
//                   验证 ready 时回合进行中重断言 busy(true)，问题 15）
//   ignore-stop   : ready 后忽略一切 stdin 消息（验证 requestStop 的 kill
//                   定时器兜底在二次调用后仍生效，L3）
//   crash-on-user-msg : ready 后 user_msg 即 exit(7) 崩溃（验证恢复窗口内
//                   requestStop 取消延迟重启，L4）

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include "DAAgentBridge.h"

// ===========================================================================
// 假 agent 子进程模式
// ===========================================================================
namespace {

void fakeEmit(QFile& out, const QJsonObject& msg)
{
    out.write(QJsonDocument(msg).toJson(QJsonDocument::Compact) + "\n");
    out.flush();
}

/// 假 agent 主循环：同步读 stdin 行、按剧本回 stdout 协议消息，EOF/stop 退出
int fakeAgentMain(const QByteArray& scenario)
{
    QFile in;
    QFile out;
    if (!in.open(stdin, QIODevice::ReadOnly | QIODevice::Text)) {
        return 1;
    }
    if (!out.open(stdout, QIODevice::WriteOnly | QIODevice::Text)) {
        return 1;
    }

    if (scenario == "init-error") {
        // init 阶段失败：发 error 后 main() 提前 return（进程 exit 0）
        fakeEmit(out, {{ "type", "error" }, { "message", "config missing" }, { "error_type", "init_failed" }});
        return 0;
    }
    if (scenario == "echo-model-crash-on-msg") {
        // init 时记录 config.model 并在 ready 中回显；收到 user_msg 即以退出码 3
        // 崩溃。Bridge 崩溃自愈用 mSavedLlmConfig 重启——第二次 ready 的 model
        // 反映 reconfigureAgent 是否同步了配置缓存（问题 22）。
        while (true) {
            const QByteArray line = in.readLine();
            if (line.isEmpty()) {
                break;
            }
            const QJsonDocument doc = QJsonDocument::fromJson(line.trimmed());
            if (!doc.isObject()) {
                continue;
            }
            const QJsonObject obj = doc.object();
            const QString type = obj.value("type").toString();
            if (type == QLatin1String("init")) {
                const QString model = obj.value("config").toObject().value("model").toString();
                fakeEmit(out, {{ "type", "ready" }, { "model", model }});
            } else if (type == QLatin1String("user_msg")) {
                return 3;  // 模拟原生崩溃（CrashExit）
            }
        }
        return 0;
    }
    if (scenario == "crash-on-user-msg") {
        // ready 后收到 user_msg 即以退出码 7 崩溃（触发 crash_recovery 调度），
        // 用于验证恢复窗口内 requestStop 取消延迟重启（L4）。
        fakeEmit(out, {{ "type", "ready" }, { "model", "fake-model" }});
        while (true) {
            const QByteArray line = in.readLine();
            if (line.isEmpty()) {
                break;
            }
            const QJsonDocument doc = QJsonDocument::fromJson(line.trimmed());
            if (!doc.isObject()) {
                continue;
            }
            const QString type = doc.object().value("type").toString();
            if (type == QLatin1String("user_msg")) {
                return 7;  // 模拟原生崩溃
            } else if (type == QLatin1String("stop")) {
                break;
            }
        }
        return 0;
    }
    if (scenario == "ignore-stop") {
        // ready 后忽略一切 stdin 消息（含 stop）——模拟 Python 端无视停止请求，
        // 验证 requestStop 的 kill 定时器兜底（L3：二次调用也须重建）。
        fakeEmit(out, {{ "type", "ready" }, { "model", "fake-model" }});
        while (true) {
            const QByteArray line = in.readLine();
            if (line.isEmpty()) {
                break;
            }
            // 故意不处理任何消息（直到被 kill）
        }
        return 0;
    }
    if (scenario == "ready-after-first-msg") {
        // init 后不立即发 ready——收到第一条 user_msg 才发 ready（复现冷启动时序：
        // sendMessage 的 busy(true) 先于 ready 到达），随后正常回 token+done。
        while (true) {
            const QByteArray line = in.readLine();
            if (line.isEmpty()) {
                break;
            }
            const QJsonDocument doc = QJsonDocument::fromJson(line.trimmed());
            if (!doc.isObject()) {
                continue;
            }
            const QString type = doc.object().value("type").toString();
            if (type == QLatin1String("user_msg")) {
                fakeEmit(out, {{ "type", "ready" }, { "model", "fake-model" }});
                fakeEmit(out, {{ "type", "token" }, { "content", "answer" }});
                fakeEmit(out, {{ "type", "done" }});
            } else if (type == QLatin1String("stop")) {
                break;
            }
        }
        return 0;
    }
    if (scenario == "done-then-idle-crash") {
        // ready 后收到 user_msg 回 done，随即以退出码 42 崩溃——模拟"回合正常
        // 完成后空闲期进程死亡"（问题 11：自愈链不得重发已回答过的消息）。
        fakeEmit(out, {{ "type", "ready" }, { "model", "fake-model" }});
        while (true) {
            const QByteArray line = in.readLine();
            if (line.isEmpty()) {
                break;
            }
            const QJsonDocument doc = QJsonDocument::fromJson(line.trimmed());
            if (!doc.isObject()) {
                continue;
            }
            const QString type = doc.object().value("type").toString();
            if (type == QLatin1String("user_msg")) {
                fakeEmit(out, {{ "type", "done" }});
                return 42;  // 空闲期崩溃（CrashExit 触发 1s 后自愈重启）
            } else if (type == QLatin1String("stop")) {
                break;
            }
        }
        return 0;
    }
    if (scenario == "never-ready") {
        // 环境性失败：永不发 ready/booting 心跳，只读 stdin 直到被 kill / EOF / stop
        while (true) {
            const QByteArray line = in.readLine();
            if (line.isEmpty()) {
                break;
            }
            const QJsonDocument doc = QJsonDocument::fromJson(line.trimmed());
            if (doc.isObject() && doc.object().value("type").toString() == QLatin1String("stop")) {
                break;
            }
        }
        return 0;
    }
    if (scenario == "runtime-error") {
        // 运行期错误（agent_runner 语义）：error+done 后主循环继续、进程存活。
        // 若 C++ 侧错误地关闭了 stdin 写通道，readLine 会收到 EOF → break → exit 0，
        // 第二条 user_msg 的 token 应答将永不到达（测试据此断言）。
        fakeEmit(out, {{ "type", "ready" }, { "model", "fake-model" }});
        bool errored = false;
        while (true) {
            const QByteArray line = in.readLine();
            if (line.isEmpty()) {
                break;  // stdin EOF / 关闭
            }
            const QJsonDocument doc = QJsonDocument::fromJson(line.trimmed());
            if (!doc.isObject()) {
                continue;
            }
            const QString type = doc.object().value("type").toString();
            if (type == QLatin1String("user_msg")) {
                if (!errored) {
                    errored = true;
                    fakeEmit(out, {{ "type", "error" }, { "message", "quota exhausted" }, { "error_type", "quota_exhausted" }});
                    fakeEmit(out, {{ "type", "done" }});
                } else {
                    fakeEmit(out, {{ "type", "token" }, { "content", "alive-after-error" }});
                    fakeEmit(out, {{ "type", "done" }});
                }
            } else if (type == QLatin1String("stop")) {
                break;
            }
        }
        return 0;
    }
    return 2;  // 未知剧本
}

QJsonObject fakeLlmConfig()
{
    return QJsonObject{
        { QStringLiteral("base_url"), QStringLiteral("http://fake.local") },
        { QStringLiteral("api_key"), QStringLiteral("fake-key") },
        { QStringLiteral("model"), QStringLiteral("fake-model") },
    };
}

/// 等待信号 spy 收到第 n 条（含）以上信号，超时返回 false
bool waitForCount(QSignalSpy& spy, int n, int timeoutMs = 15000)
{
    const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + timeoutMs;
    while (spy.count() < n) {
        const int remain = static_cast<int>(deadline - QDateTime::currentMSecsSinceEpoch());
        if (remain <= 0 || !spy.wait(remain)) {
            if (spy.count() < n) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

// ===========================================================================
// 测试类
// ===========================================================================
class DAAgentBridgeTest : public QObject
{
    Q_OBJECT
private:
    // 以假 agent 剧本启动桥（pythonExePath=测试 exe 自身）
    static void startWithScenario(DA::DAAgentBridge& bridge, const char* scenario,
                                  int readyTimeoutMs = 10000, int stopTimeoutMs = 3000);

private Q_SLOTS:
    void testInitErrorClosesChannelAndExits();  // 问题9：init 阶段错误 → 关写通道、正常退出、无崩溃自愈
    void testRuntimeErrorKeepsProcessAlive();   // 问题9：运行期错误 → 不关写通道、进程存活可继续对话
    void testReadyTimeoutNoCrashRecoveryLoop(); // 问题20：ready 超时 kill → 不进崩溃自愈循环
    void testStartupFailureEmitsTerminalSignals(); // 问题21：waitForStarted 失败补终止语义（状态机黑洞）
    void testReconfigureSyncsRecoveryConfig();     // 问题22：reconfigure 同步缓存，崩溃恢复用新配置
    void testSendWithoutProcessRollsBack();        // 问题23：writeJson 失败回滚 busy/看门狗 + 明确错误
    void testIdleCrashDoesNotResendAnsweredMessage(); // 问题11：done 清 mLastUserMessage，空闲期崩溃不自发重放
    void testReadyDuringActiveTurnReassertsBusy();    // 问题15：ready 到达时回合进行中 → 重断言 busy(true)
    void testSecondRequestStopRebuildsKillFallback(); // L3：stop 被忽略时二次 requestStop 仍重建 kill 兜底
    void testStopDuringRecoveryWindowCancelsRestart(); // L4：恢复窗口内 Stop 取消延迟重启
};

void DAAgentBridgeTest::startWithScenario(DA::DAAgentBridge& bridge, const char* scenario,
                                          int readyTimeoutMs, int stopTimeoutMs)
{
    const QString exe = QCoreApplication::applicationFilePath();
    bridge.startAgent(fakeLlmConfig(), QJsonArray(), QStringLiteral("test prompt"), QJsonArray(),
                      exe, QStringLiteral("--fake-agent=%1").arg(QLatin1String(scenario)),
                      readyTimeoutMs, stopTimeoutMs);
}

/**
 * 问题9（init 阶段）：未收到 ready 即 error → 关闭写通道让进程正常退出，
 * 且退出码 0 不触发崩溃自愈（全程只有一条 error）。
 */
void DAAgentBridgeTest::testInitErrorClosesChannelAndExits()
{
    DA::DAAgentBridge bridge;
    QSignalSpy errSpy(&bridge, &DA::DAAgentBridge::agentError);
    QSignalSpy busySpy(&bridge, &DA::DAAgentBridge::agentBusy);
    startWithScenario(bridge, "init-error");

    QVERIFY(waitForCount(errSpy, 1));
    QCOMPARE(errSpy.at(0).at(1).toString(), QStringLiteral("init_failed"));

    // 进程自行退出（exit 0）→ isRunning 复位、busy(false) 兜底
    QTRY_VERIFY_WITH_TIMEOUT(!bridge.isRunning(), 15000);
    QVERIFY(waitForCount(busySpy, 1));
    QCOMPARE(busySpy.last().at(0).toBool(), false);

    // 无 crash_recovery/crash_exhausted 二次错误（NormalExit 0 不进自愈循环）
    QTest::qWait(500);
    QCOMPARE(errSpy.count(), 1);
}

/**
 * 问题9（运行期）：收到过 ready 后的 error（quota_exhausted 等）不得关闭
 * stdin 写通道——Python 侧 error+done 后主循环继续、进程设计为存活；
 * 修复前：EOF → 进程 exit 0 → 第二条消息无应答。
 */
void DAAgentBridgeTest::testRuntimeErrorKeepsProcessAlive()
{
    DA::DAAgentBridge bridge;
    QSignalSpy readySpy(&bridge, &DA::DAAgentBridge::agentReady);
    QSignalSpy errSpy(&bridge, &DA::DAAgentBridge::agentError);
    QSignalSpy doneSpy(&bridge, &DA::DAAgentBridge::agentDone);
    QSignalSpy tokenSpy(&bridge, &DA::DAAgentBridge::agentToken);
    QSignalSpy busySpy(&bridge, &DA::DAAgentBridge::agentBusy);
    startWithScenario(bridge, "runtime-error");

    QVERIFY(waitForCount(readySpy, 1));

    // 第一轮：触发运行期错误
    bridge.sendMessage(QStringLiteral("first"));
    QVERIFY(waitForCount(errSpy, 1));
    QCOMPARE(errSpy.at(0).at(1).toString(), QStringLiteral("quota_exhausted"));
    QVERIFY(waitForCount(doneSpy, 1));

    // 运行期错误后进程必须保持存活（修复前：写通道被关 → EOF → exit 0）
    QVERIFY(bridge.isRunning());

    // 第二轮：写通道仍开放，进程正常应答 token
    bridge.sendMessage(QStringLiteral("second"));
    QVERIFY(waitForCount(tokenSpy, 1));
    QCOMPARE(tokenSpy.at(0).at(0).toString(), QStringLiteral("alive-after-error"));
    QVERIFY(bridge.isRunning());
    // 运行期错误不应触发写 stdin 失败类二次错误（仅 quota_exhausted 一条）
    QCOMPARE(errSpy.count(), 1);

    // 等待进程真实退出（onProcessFinished 用户停止分支补发 busy(false)），
    // 避免析构时 QProcess 仍在运行
    bridge.requestStop();
    const int busyCountBefore = busySpy.count();
    QTRY_VERIFY_WITH_TIMEOUT(busySpy.count() > busyCountBefore, 15000);
    QCOMPARE(busySpy.last().at(0).toBool(), false);
}

/**
 * 问题20：ready 超时属环境性失败（重启必然再次超时），kill 前置用户停止
 * 标志走"非崩溃"退出分支——只报 1 条错误直接终态，不进 3 轮自愈循环
 * （修复前：最长 4×readyTimeout 等待 + not-ready/crash_recovery/exhausted
 * 共 5 条错误轰炸，且终态落入 starting 残留）。
 */
void DAAgentBridgeTest::testReadyTimeoutNoCrashRecoveryLoop()
{
    DA::DAAgentBridge bridge;
    QSignalSpy errSpy(&bridge, &DA::DAAgentBridge::agentError);
    QSignalSpy busySpy(&bridge, &DA::DAAgentBridge::agentBusy);
    // ready 超时压到 1s，快速触发超时路径
    startWithScenario(bridge, "never-ready", 1000);

    QVERIFY(waitForCount(errSpy, 1));
    QVERIFY(errSpy.at(0).at(0).toString().contains(QStringLiteral("not ready")));

    // kill → onProcessFinished 走用户停止分支 → busy(false) 兜底复位 UI
    QVERIFY(waitForCount(busySpy, 1));
    QCOMPARE(busySpy.last().at(0).toBool(), false);

    // 自愈循环会在 1s 后重启并再次超时报错——等 3s 确认全程只有 1 条错误
    QTest::qWait(3000);
    QCOMPARE(errSpy.count(), 1);
}

/**
 * 问题21：进程无法启动（FailedToStart）时 Qt 不发 finished，
 * onProcessFinished 不执行——修复前该路径是状态机黑洞（无 busy(false)/
 * processExited，恢复路径无任何兜底，UI 永久"思考中/启动中"）。
 * 修复后 startAgent 同步补齐终止语义：agentError + busy(false) +
 * processExited + mRecovering/mRunning 复位。
 */
void DAAgentBridgeTest::testStartupFailureEmitsTerminalSignals()
{
    DA::DAAgentBridge bridge;
    QSignalSpy errSpy(&bridge, &DA::DAAgentBridge::agentError);
    QSignalSpy busySpy(&bridge, &DA::DAAgentBridge::agentBusy);
    QSignalSpy exitSpy(&bridge, &DA::DAAgentBridge::processExited);

    // 模拟崩溃恢复路径（黑洞场景）：mRecovering=true 时启动失败
    bridge.setRecovering(true);
    bridge.startAgent(fakeLlmConfig(), QJsonArray(), QStringLiteral("test prompt"), QJsonArray(),
                      QStringLiteral("Z:/nonexistent-dir/fake-python-not-exists.exe"),
                      QStringLiteral("fake_script.py"), 5000, 3000);

    // waitForStarted 失败在 startAgent 栈内同步补齐全部终止语义
    QCOMPARE(errSpy.count(), 1);
    QCOMPARE(busySpy.count(), 1);
    QCOMPARE(busySpy.at(0).at(0).toBool(), false);
    QCOMPARE(exitSpy.count(), 1);
    QVERIFY(!bridge.isRecovering());
    QVERIFY(!bridge.isRunning());
}

/**
 * 问题22：reconfigureAgent 不更新 mSavedLlmConfig → 崩溃恢复用陈旧配置
 * 复活（用户换模型/密钥后恢复进程仍跑旧值；旧 key 失效时恢复必然再失败
 * 进 ready 超时循环）。修复后 reconfigure 同步缓存，自愈重启的 init 携带
 * 新 model——假 agent 在 ready 中回显 init.model，据此断言。
 */
void DAAgentBridgeTest::testReconfigureSyncsRecoveryConfig()
{
    DA::DAAgentBridge bridge;
    QSignalSpy readySpy(&bridge, &DA::DAAgentBridge::agentReady);
    startWithScenario(bridge, "echo-model-crash-on-msg");

    QVERIFY(waitForCount(readySpy, 1));
    QCOMPARE(readySpy.at(0).at(0).toString(), QStringLiteral("fake-model"));

    // 热替换模型（不重启进程）
    QJsonObject newCfg = fakeLlmConfig();
    newCfg[QStringLiteral("model")] = QStringLiteral("new-model");
    bridge.reconfigureAgent(newCfg);

    // 触发崩溃：user_msg → exit(3) → 自愈 1s 后用 mSavedLlmConfig 重启
    bridge.sendMessage(QStringLiteral("crash trigger"));

    // 第二次 ready 的 model 应为 reconfigure 后的新值（修复前为陈旧 fake-model）
    QVERIFY(waitForCount(readySpy, 2, 20000));
    QCOMPARE(readySpy.at(1).at(0).toString(), QStringLiteral("new-model"));

    bridge.requestStop();
}

/**
 * 问题23：writeJson 失败（进程未运行/管道已关闭）时消息静默丢失——
 * 修复前 busy(true) 挂到 4 分钟看门狗超时才报错。修复后 sendMessage/
 * sendUserAnswer 检查返回值并同步回滚：busy(false) + 明确错误 + 不启动看门狗。
 */
void DAAgentBridgeTest::testSendWithoutProcessRollsBack()
{
    // 未 startAgent 的桥：mProcess=nullptr → writeJson 必失败
    {
        DA::DAAgentBridge bridge;
        QSignalSpy busySpy(&bridge, &DA::DAAgentBridge::agentBusy);
        QSignalSpy errSpy(&bridge, &DA::DAAgentBridge::agentError);
        bridge.sendMessage(QStringLiteral("hello"));
        // busy(true) 后立即回滚 busy(false)，错误明确（非通用 stdin 错误）
        QCOMPARE(busySpy.count(), 2);
        QCOMPARE(busySpy.at(0).at(0).toBool(), true);
        QCOMPARE(busySpy.at(1).at(0).toBool(), false);
        QCOMPARE(errSpy.count(), 1);
        QVERIFY(errSpy.at(0).at(0).toString().contains(QStringLiteral("not running")));
    }
    // sendUserAnswer 同样回滚（死桥答 ask_user 卡场景：答案蒸发须显式报错）
    {
        DA::DAAgentBridge bridge;
        QSignalSpy busySpy(&bridge, &DA::DAAgentBridge::agentBusy);
        QSignalSpy errSpy(&bridge, &DA::DAAgentBridge::agentError);
        bridge.sendUserAnswer(QStringLiteral("answer"));
        QCOMPARE(busySpy.count(), 1);
        QCOMPARE(busySpy.at(0).at(0).toBool(), false);
        QCOMPARE(errSpy.count(), 1);
        QVERIFY(errSpy.at(0).at(0).toString().contains(QStringLiteral("not running")));
    }
}

/**
 * 问题11：回合正常完成（done）后 mLastUserMessage 必须清除——活跃会话的桥
 * 跑完不退役，若进程在空闲期崩溃，自愈链 ready→load_session→
 * resendLastMessage 会把已回答过的消息重新注入（UI 自发"思考中"、
 * 重复答案写进 JSONL、白耗一轮 token）。修复后 resendLastMessage 走空
 * 分支：busy(false)、不写 user_msg。
 */
void DAAgentBridgeTest::testIdleCrashDoesNotResendAnsweredMessage()
{
    DA::DAAgentBridge bridge;
    QSignalSpy readySpy(&bridge, &DA::DAAgentBridge::agentReady);
    QSignalSpy doneSpy(&bridge, &DA::DAAgentBridge::agentDone);
    QSignalSpy busySpy(&bridge, &DA::DAAgentBridge::agentBusy);
    startWithScenario(bridge, "done-then-idle-crash");

    QVERIFY(waitForCount(readySpy, 1));
    bridge.sendMessage(QStringLiteral("answered question"));
    QVERIFY(waitForCount(doneSpy, 1));  // done → 修复后此处清 mLastUserMessage

    // 空闲期崩溃（exit 42）→ crash_recovery → 1s 后自愈重启 → 第二次 ready
    QVERIFY(waitForCount(readySpy, 2, 20000));

    // Module 语义：session_loaded 后调 resendLastMessage。
    // 断言走空分支：同步 emit busy(false)（修复前为 busy(true)+重发 user_msg）
    const int busyBefore = busySpy.count();
    bridge.resendLastMessage();
    QCOMPARE(busySpy.count(), busyBefore + 1);
    QCOMPARE(busySpy.last().at(0).toBool(), false);
    QVERIFY(!bridge.isRecovering());

    bridge.requestStop();
}

/**
 * 问题15：冷启动时序——sendMessage 的 busy(true) 在启动期被 Dock 的 starting
 * 守卫吞掉 web 推送，~16s 后 ready 到达时本轮对话才真正开始。修复前 Dock
 * onAgentReady 强制清 busy → 整轮纯文本回复期间 UI 显示 Ready、无 Stop、
 * 可双发。修复后 Bridge 在 ready 时若 mTurnActive 仍为 true 则重发 busy(true)
 * （Dock 侧 onAgentReady 同步改为按内部状态补推，不再强制复位）。
 */
void DAAgentBridgeTest::testReadyDuringActiveTurnReassertsBusy()
{
    DA::DAAgentBridge bridge;
    QSignalSpy readySpy(&bridge, &DA::DAAgentBridge::agentReady);
    QSignalSpy busySpy(&bridge, &DA::DAAgentBridge::agentBusy);
    QSignalSpy doneSpy(&bridge, &DA::DAAgentBridge::agentDone);
    startWithScenario(bridge, "ready-after-first-msg");

    // ready 未到达即发消息（复现冷启动：busy(true) 先于 ready）
    bridge.sendMessage(QStringLiteral("question"));
    QCOMPARE(busySpy.count(), 1);
    QCOMPARE(busySpy.at(0).at(0).toBool(), true);

    // ready 到达 → Bridge 重断言 busy(true)（mTurnActive 仍为 true）
    QVERIFY(waitForCount(readySpy, 1));
    QVERIFY(waitForCount(busySpy, 2));
    QCOMPARE(busySpy.at(1).at(0).toBool(), true);

    // 本轮结束 done → busy(false)
    QVERIFY(waitForCount(doneSpy, 1));
    QVERIFY(waitForCount(busySpy, 3));
    QCOMPARE(busySpy.at(2).at(0).toBool(), false);

    // 停止并等待进程真实退出（避免析构时 QProcess 仍在运行）
    bridge.requestStop();
    QVERIFY(waitForCount(busySpy, 4));
}

/**
 * L3：requestStop 的 kill 兜底缺陷——① 定时器在自身 timeout 槽内裸 delete
 * 发送者；② 二次 requestStop 因 mRunning 已 false 跳过定时器重建，而入口处
 * 刚取消了上一个 kill 定时器：Python 忽略 stop 时进程永不退出、无人兜底。
 * 修复后按进程实际状态重建 kill 定时器 + deleteLater。
 */
void DAAgentBridgeTest::testSecondRequestStopRebuildsKillFallback()
{
    DA::DAAgentBridge bridge;
    QSignalSpy readySpy(&bridge, &DA::DAAgentBridge::agentReady);
    QSignalSpy busySpy(&bridge, &DA::DAAgentBridge::agentBusy);
    // kill 兜底超时压到 1s，加速验证
    startWithScenario(bridge, "ignore-stop", 10000, 1000);

    QVERIFY(waitForCount(readySpy, 1));

    bridge.requestStop();  // 第一次：stop 被假 agent 忽略，1s kill 兜底启动
    bridge.requestStop();  // 第二次（修复前：取消旧定时器且不重建 → 无人兜底）

    // kill 兜底必须生效：onProcessFinished 用户停止分支 → busy(false)
    QVERIFY(waitForCount(busySpy, 1, 10000));
    QCOMPARE(busySpy.last().at(0).toBool(), false);
}

/**
 * L4：崩溃自愈 1s 恢复窗口内用户 Stop 被静默忽略——恢复 singleShot 无句柄
 * 不可取消，1s 后照常重启重放，违背用户终止意图。修复后恢复定时器持句柄，
 * requestStop 取消重启、清恢复标志与重发缓存、回发 busy(false)；
 * Module::stop() 守卫同步放行 isRecovering 窗口。
 */
void DAAgentBridgeTest::testStopDuringRecoveryWindowCancelsRestart()
{
    DA::DAAgentBridge bridge;
    QSignalSpy readySpy(&bridge, &DA::DAAgentBridge::agentReady);
    QSignalSpy errSpy(&bridge, &DA::DAAgentBridge::agentError);
    QSignalSpy busySpy(&bridge, &DA::DAAgentBridge::agentBusy);
    startWithScenario(bridge, "crash-on-user-msg");

    QVERIFY(waitForCount(readySpy, 1));
    bridge.sendMessage(QStringLiteral("doomed"));  // → exit(7) 崩溃 → crash_recovery + 1s 恢复窗口

    // crash_recovery 错误到达 = 恢复窗口已打开（调度时即置 mRecovering）
    QVERIFY(waitForCount(errSpy, 1));
    QCOMPARE(errSpy.at(0).at(1).toString(), QStringLiteral("crash_recovery"));
    QVERIFY(bridge.isRecovering());

    // 窗口内用户 Stop → 取消延迟重启
    bridge.requestStop();
    QVERIFY(!bridge.isRecovering());
    // busy(false) 回发解除 UI 忙碌态
    QVERIFY(waitForCount(busySpy, 2));  // [true(sendMessage), false(Stop 取消恢复)]
    QCOMPARE(busySpy.last().at(0).toBool(), false);

    // 修复前：1s 后照常重启 → 第二次 ready。等 3s 确认不再重启
    QTest::qWait(3000);
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(errSpy.count(), 1);  // 无后续 crash_recovery/exhausted 错误
}

int main(int argc, char* argv[])
{
    // 假 agent 子进程模式：在创建测试对象前拦截（同步 stdio 循环，无需事件循环）
    for (int i = 1; i < argc; ++i) {
        const QByteArray arg(argv[i]);
        if (arg.startsWith("--fake-agent=")) {
            return fakeAgentMain(arg.mid(qstrlen("--fake-agent=")));
        }
    }

    QCoreApplication app(argc, argv);
    DAAgentBridgeTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "main.moc"
