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
            } else {
                fakeEmit(out, {{ "type", "message_end" }, { "content", "echo:" + content }});
                fakeEmit(out, {{ "type", "done" }});
            }
        } else if (type == QLatin1String("user_answer")) {
            fakeEmit(out, {{ "type", "message_end" }, { "content", "answered" }});
            fakeEmit(out, {{ "type", "done" }});
        } else if (type == QLatin1String("stop")) {
            break;
        }
        // 其余类型（reconfigure/update_subagents/tool_result...）静默忽略
    }
    return 0;
}

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
