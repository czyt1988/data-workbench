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
    static void startWithScenario(DA::DAAgentBridge& bridge, const char* scenario, int readyTimeoutMs = 10000);

private Q_SLOTS:
    void testInitErrorClosesChannelAndExits();  // 问题9：init 阶段错误 → 关写通道、正常退出、无崩溃自愈
    void testRuntimeErrorKeepsProcessAlive();   // 问题9：运行期错误 → 不关写通道、进程存活可继续对话
    void testReadyTimeoutNoCrashRecoveryLoop(); // 问题20：ready 超时 kill → 不进崩溃自愈循环
    void testStartupFailureEmitsTerminalSignals(); // 问题21：waitForStarted 失败补终止语义（状态机黑洞）
    void testReconfigureSyncsRecoveryConfig();     // 问题22：reconfigure 同步缓存，崩溃恢复用新配置
};

void DAAgentBridgeTest::startWithScenario(DA::DAAgentBridge& bridge, const char* scenario, int readyTimeoutMs)
{
    const QString exe = QCoreApplication::applicationFilePath();
    bridge.startAgent(fakeLlmConfig(), QJsonArray(), QStringLiteral("test prompt"), QJsonArray(),
                      exe, QStringLiteral("--fake-agent=%1").arg(QLatin1String(scenario)),
                      readyTimeoutMs, 3000);
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
