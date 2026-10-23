#include <QtTest/QtTest>
#include <QElapsedTimer>
#include <QFile>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QThread>
#include <cstring>
#include <thread>
#include "DALog.h"  // 聚合头：DALogger + DALogCategory + DAMessageLogQueue + DASlowLog
#include "DAMessageLogQueue.h"

namespace
{

// 捕获 daworkbench.slow / da.* category 输出（在 DALogger 接管 message handler 之前安装）
struct CaptureData
{
    QStringList slowLines;
    QStringList businessLines;
};

CaptureData* gCapture = nullptr;

void captureHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    Q_UNUSED(type)
    if (!gCapture || !ctx.category) {
        return;
    }
    if (std::strcmp(ctx.category, "daworkbench.slow") == 0) {
        gCapture->slowLines << msg;
    } else if (std::strncmp(ctx.category, "da.", 3) == 0) {
        gCapture->businessLines << msg;
    }
}

// 取走并清空已捕获的慢日志行
QStringList takeSlowLines()
{
    QStringList r;
    if (gCapture) {
        r.swap(gCapture->slowLines);
    }
    return r;
}

int findLine(const QStringList& lines, const QString& needle)
{
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[ i ].contains(needle)) {
            return i;
        }
    }
    return -1;
}

// 从记录行中解析整型字段（如 ms= / self_ms= / count=）；ms 前带空白避免误匹配 self_ms
bool parseField(const QString& line, const QString& field, qint64* out)
{
    const QString pattern = (field == QLatin1String("ms"))
                                ? QStringLiteral("(?:^|\\s)ms=(\\d+)")
                                : QStringLiteral("(?:^|\\s)%1=(-?\\d+)").arg(field);
    const QRegularExpressionMatch m = QRegularExpression(pattern).match(line);
    if (!m.hasMatch()) {
        return false;
    }
    *out = m.captured(1).toLongLong();
    return true;
}

QString readFileAll(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

// 异步 logger 的 flush 只把刷新请求入队，不阻塞等待落盘，文件内容可能稍后才可见
QString readFileAllEventually(const QString& path, const QStringList& needles, int timeoutMs = 3000)
{
    QElapsedTimer timer;
    timer.start();
    QString content = readFileAll(path);
    while (timer.elapsed() < timeoutMs) {
        bool allFound = true;
        for (const QString& needle : needles) {
            if (!content.contains(needle)) {
                allFound = false;
                break;
            }
        }
        if (allFound) {
            break;
        }
        QTest::qWait(20);
        content = readFileAll(path);
    }
    return content;
}

}  // namespace

class DASlowLogTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testConfigDefaultsAndOverride();
    void testThresholdExceeded();
    void testBelowThresholdSilent();
    void testDetailLazy();
    void testNestedSelfMs();
    void testRuntimeDisabled();
    void testCallMacro();
    void testCounterAggregate();
    void testMainThreadScopePublish();
    void testFileRouting();  // 必须放最后：DALogger 会接管 message handler

private:
    QTemporaryDir mTempDir;
};

void DASlowLogTest::initTestCase()
{
    QVERIFY2(mTempDir.isValid(), "Failed to create temporary directory for log files");
    // 固定运行期配置，避免受外部环境变量干扰
    qputenv("DA_SLOW_LOG", "on");
    qputenv("DA_SLOW_LOG_MS", "50");
    qputenv("DA_SLOW_LOG_AGGREGATE_MS", "1000");
    qputenv("DA_SLOW_LOG_STALL_MS", "500");
    DA::DASlowLog::reloadConfig();
    static CaptureData sCapture;
    gCapture = &sCapture;
    qInstallMessageHandler(captureHandler);
    DA::DASlowLog::setMainThreadId(std::this_thread::get_id());
}

void DASlowLogTest::cleanupTestCase()
{
    gCapture = nullptr;
    qInstallMessageHandler(nullptr);
}

void DASlowLogTest::testConfigDefaultsAndOverride()
{
    QCOMPARE(DA::DASlowLog::isEnabled(), true);
    QCOMPARE(DA::DASlowLog::defaultThresholdMs(), 50);
    QCOMPARE(DA::DASlowLog::aggregateIntervalMs(), 1000);
    QCOMPARE(DA::DASlowLog::stallThresholdMs(), 500);

    // 按标签覆盖阈值
    qputenv("DA_SLOW_LOG_MS_test.cfg.override", "123");
    DA::DASlowLog::reloadConfig();
    QCOMPARE(DA::DASlowLog::thresholdMs("test.cfg.override"), 123);
    QCOMPARE(DA::DASlowLog::thresholdMs("test.cfg.none"), 50);
    QVERIFY(DA::DASlowLog::minThresholdMs() <= 50);

    // 非法值（非数字、负数）回退默认值
    qputenv("DA_SLOW_LOG_MS", "abc");
    DA::DASlowLog::reloadConfig();
    QCOMPARE(DA::DASlowLog::defaultThresholdMs(), 50);
    qputenv("DA_SLOW_LOG_MS", "-5");
    DA::DASlowLog::reloadConfig();
    QCOMPARE(DA::DASlowLog::defaultThresholdMs(), 50);

    // 恢复
    qputenv("DA_SLOW_LOG_MS", "50");
    qunsetenv("DA_SLOW_LOG_MS_test.cfg.override");
    DA::DASlowLog::reloadConfig();
    QCOMPARE(DA::DASlowLog::thresholdMs("test.cfg.override"), 50);
}

void DASlowLogTest::testThresholdExceeded()
{
#if defined(DA_ENABLE_SLOW_LOG)
    takeSlowLines();
    {
        DA_SLOW_LOG_SCOPE_MS("test.scope.exceeded", 10);
        QThread::msleep(30);
    }
    const QStringList lines = takeSlowLines();
    const int idx           = findLine(lines, "tag=test.scope.exceeded");
    QVERIFY2(idx >= 0, qPrintable(QStringLiteral("no record for test.scope.exceeded, lines=%1").arg(lines.join('|'))));
    const QString& l = lines[ idx ];
    // 单行自包含字段
    QVERIFY(l.startsWith("[slow]"));
    QVERIFY(l.contains("self_ms="));
    QVERIFY(l.contains("depth=0"));
    QVERIFY(l.contains("thread=main"));
    QVERIFY(l.contains("file=main.cpp"));
    QVERIFY(l.contains("func="));
    QVERIFY(QRegularExpression("(?:^|\\s)line=\\d+").match(l).hasMatch());
    QVERIFY(!l.contains("detail="));  // 未提供详情时不输出 detail 字段
    qint64 ms = 0;
    QVERIFY(parseField(l, "ms", &ms));
    QVERIFY2(ms >= 30, qPrintable(QStringLiteral("ms=%1 < 30").arg(ms)));
#else
    QSKIP("DA_ENABLE_SLOW_LOG not defined (macros compiled out)");
#endif
}

void DASlowLogTest::testBelowThresholdSilent()
{
#if defined(DA_ENABLE_SLOW_LOG)
    takeSlowLines();
    {
        DA_SLOW_LOG_SCOPE_MS("test.scope.silent", 5000);
        QThread::msleep(10);
    }
    const QStringList lines = takeSlowLines();
    QVERIFY2(lines.isEmpty(), qPrintable(QStringLiteral("unexpected records: %1").arg(lines.join('|'))));
#else
    QSKIP("DA_ENABLE_SLOW_LOG not defined (macros compiled out)");
#endif
}

void DASlowLogTest::testDetailLazy()
{
#if defined(DA_ENABLE_SLOW_LOG)
    int calls = 0;
    takeSlowLines();
    {
        // 默认阈值 50ms，10ms 不触发 → 详情 lambda 不应被调用
        DA_SLOW_LOG_SCOPE_DETAIL("test.detail.silent", [&calls]() {
            ++calls;
            return QStringLiteral("never");
        });
        QThread::msleep(10);
    }
    QCOMPARE(calls, 0);
    QVERIFY(takeSlowLines().isEmpty());

    {
        // 80ms > 默认阈值 50ms → 触发，详情 lambda 恰好调用一次
        DA_SLOW_LOG_SCOPE_DETAIL("test.detail.fired", [&calls]() {
            ++calls;
            return QStringLiteral("rows=42");
        });
        QThread::msleep(80);
    }
    QCOMPARE(calls, 1);
    const QStringList lines = takeSlowLines();
    const int idx           = findLine(lines, "tag=test.detail.fired");
    QVERIFY2(idx >= 0, qPrintable(QStringLiteral("no record for test.detail.fired, lines=%1").arg(lines.join('|'))));
    QVERIFY(lines[ idx ].contains("detail=\"rows=42\""));
#else
    QSKIP("DA_ENABLE_SLOW_LOG not defined (macros compiled out)");
#endif
}

void DASlowLogTest::testNestedSelfMs()
{
#if defined(DA_ENABLE_SLOW_LOG)
    takeSlowLines();
    {
        DA_SLOW_LOG_SCOPE_MS("test.nested.outer", 10);
        QThread::msleep(50);
        {
            DA_SLOW_LOG_SCOPE_MS("test.nested.inner", 10);
            QThread::msleep(250);
        }
    }
    const QStringList lines = takeSlowLines();
    const int outerIdx      = findLine(lines, "tag=test.nested.outer");
    const int innerIdx      = findLine(lines, "tag=test.nested.inner");
    QVERIFY2(outerIdx >= 0, qPrintable(QStringLiteral("no outer record, lines=%1").arg(lines.join('|'))));
    QVERIFY2(innerIdx >= 0, qPrintable(QStringLiteral("no inner record, lines=%1").arg(lines.join('|'))));

    QVERIFY(lines[ outerIdx ].contains("depth=0"));
    QVERIFY(lines[ innerIdx ].contains("depth=1"));

    qint64 outerMs = 0, outerSelf = 0, innerMs = 0, innerSelf = 0;
    QVERIFY(parseField(lines[ outerIdx ], "ms", &outerMs));
    QVERIFY(parseField(lines[ outerIdx ], "self_ms", &outerSelf));
    QVERIFY(parseField(lines[ innerIdx ], "ms", &innerMs));
    QVERIFY(parseField(lines[ innerIdx ], "self_ms", &innerSelf));

    QVERIFY2(outerMs >= 300, qPrintable(QStringLiteral("outerMs=%1 < 300").arg(outerMs)));
    QVERIFY2(innerMs >= 250, qPrintable(QStringLiteral("innerMs=%1 < 250").arg(innerMs)));
    // 外层自身耗时 ≈ 300-250 = 50ms（方案容差 ±30ms）；Windows 定时器粒度会带来正偏差
    QVERIFY2(outerSelf >= 20 && outerSelf <= 80,
             qPrintable(QStringLiteral("outerSelf=%1 not in [20,80]").arg(outerSelf)));
    // 内层无子作用域：self ≈ 总耗时
    QVERIFY2(qAbs(innerSelf - innerMs) <= 5,
             qPrintable(QStringLiteral("innerSelf=%1 innerMs=%2").arg(innerSelf).arg(innerMs)));
#else
    QSKIP("DA_ENABLE_SLOW_LOG not defined (macros compiled out)");
#endif
}

void DASlowLogTest::testRuntimeDisabled()
{
    // DA_SLOW_LOG=off 时：作用域与计数器都不产生输出（编译期开或关都适用）
    qputenv("DA_SLOW_LOG", "off");
    DA::DASlowLog::reloadConfig();
    QVERIFY(!DA::DASlowLog::isEnabled());

    takeSlowLines();
    {
        DA_SLOW_LOG_SCOPE_MS("test.disabled.scope", 1);
        QThread::msleep(20);
    }
    DA::DASlowLogCounter* c = DA::DASlowLog::counter("test.disabled.counter");
    QVERIFY(c);
    {
        DA::DASlowLogCounterScope sc(c);
        QThread::msleep(5);
    }
    DA::DASlowLog::flushCounters();
    const QStringList lines = takeSlowLines();
    QVERIFY2(lines.isEmpty(), qPrintable(QStringLiteral("unexpected records when disabled: %1").arg(lines.join('|'))));

    qputenv("DA_SLOW_LOG", "on");
    DA::DASlowLog::reloadConfig();
    QVERIFY(DA::DASlowLog::isEnabled());
}

void DASlowLogTest::testCallMacro()
{
    // 关闭态语义：DA_SLOW_LOG_CALL 必须展开为原表达式并保留返回值（任何编译开关下都成立）
    auto compute = []() {
        QThread::msleep(5);
        return 42;
    };
    int v = DA_SLOW_LOG_CALL("test.call.value", compute());
    QCOMPARE(v, 42);

#if defined(DA_ENABLE_SLOW_LOG)
    takeSlowLines();
    auto slowCompute = []() {
        QThread::msleep(80);
        return QStringLiteral("ret");
    };
    QString s = DA_SLOW_LOG_CALL("test.call.slow", slowCompute());
    QCOMPARE(s, QStringLiteral("ret"));
    const QStringList lines = takeSlowLines();
    const int idx           = findLine(lines, "tag=test.call.slow");
    QVERIFY2(idx >= 0, qPrintable(QStringLiteral("no record for test.call.slow, lines=%1").arg(lines.join('|'))));
    qint64 ms = 0;
    QVERIFY(parseField(lines[ idx ], "ms", &ms));
    QVERIFY2(ms >= 80, qPrintable(QStringLiteral("ms=%1 < 80").arg(ms)));
#endif
}

void DASlowLogTest::testCounterAggregate()
{
    // 计数器核心 API 与编译期开关无关，任何配置下都可测
    DA::DASlowLogCounter* c = DA::DASlowLog::counter("test.counter.aggregate");
    QVERIFY(c);
    // 同名标签返回同一计数器
    QCOMPARE(DA::DASlowLog::counter("test.counter.aggregate"), c);
    c->add(5000000);   // 5ms
    c->add(15000000);  // 15ms
    c->add(1000000);   // 1ms

    takeSlowLines();
    DA::DASlowLog::flushCounters();
    QStringList lines = takeSlowLines();
    int idx           = findLine(lines, "tag=test.counter.aggregate");
    QVERIFY2(idx >= 0, qPrintable(QStringLiteral("no counter record, lines=%1").arg(lines.join('|'))));
    QVERIFY(lines[ idx ].contains("count=3"));
    QVERIFY(lines[ idx ].contains("total_ms=21"));
    QVERIFY(lines[ idx ].contains("max_ms=15"));
    QVERIFY(lines[ idx ].contains("window_ms=1000"));

    // flush 后清零：无新增采样时不输出
    DA::DASlowLog::flushCounters();
    lines = takeSlowLines();
    QVERIFY(findLine(lines, "tag=test.counter.aggregate") < 0);

#if defined(DA_ENABLE_SLOW_LOG)
    // 宏路径：tag 以裸标识符书写，由宏字符串化；计数器指针缓存于函数局部静态
    auto fn = []() {
        DA_SLOW_LOG_COUNTER(test.counter.macro);
        QThread::msleep(15);
    };
    takeSlowLines();
    fn();
    fn();
    DA::DASlowLog::flushCounters();
    lines = takeSlowLines();
    idx   = findLine(lines, "tag=test.counter.macro");
    QVERIFY2(idx >= 0, qPrintable(QStringLiteral("no macro counter record, lines=%1").arg(lines.join('|'))));
    QVERIFY(lines[ idx ].contains("count=2"));
    qint64 maxMs = 0;
    QVERIFY(parseField(lines[ idx ], "max_ms", &maxMs));
    QVERIFY2(maxMs >= 10, qPrintable(QStringLiteral("max_ms=%1 < 10").arg(maxMs)));
#endif
}

void DASlowLogTest::testMainThreadScopePublish()
{
#if defined(DA_ENABLE_SLOW_LOG)
    // 无活动作用域时为空
    QVERIFY(DA::DASlowLog::currentMainThreadScope().tag == nullptr);
    {
        DA_SLOW_LOG_SCOPE("test.scope.publish");
        DA::DASlowLog::ScopeInfo info = DA::DASlowLog::currentMainThreadScope();
        QVERIFY(info.tag);
        QCOMPARE(QString::fromUtf8(info.tag ? info.tag : ""), QStringLiteral("test.scope.publish"));
        QVERIFY(info.file && std::strstr(info.file, "main.cpp"));
        QVERIFY(info.line > 0);
        QVERIFY(info.func);
        {
            // 嵌套进入内层：栈顶帧更新
            DA_SLOW_LOG_SCOPE_MS("test.scope.publish.inner", 999999);
            info = DA::DASlowLog::currentMainThreadScope();
            QCOMPARE(QString::fromUtf8(info.tag ? info.tag : ""), QStringLiteral("test.scope.publish.inner"));
        }
        // 内层弹栈后恢复为外层帧
        info = DA::DASlowLog::currentMainThreadScope();
        QCOMPARE(QString::fromUtf8(info.tag ? info.tag : ""), QStringLiteral("test.scope.publish"));
    }
    QVERIFY(DA::DASlowLog::currentMainThreadScope().tag == nullptr);
#else
    QSKIP("DA_ENABLE_SLOW_LOG not defined (macros compiled out)");
#endif
}

void DASlowLogTest::testFileRouting()
{
    // 本测试之后 message handler 由 DALogger 接管（单例，直到进程退出），故必须放在最后
    QVERIFY(mTempDir.isValid());
    const QString mainLog = mTempDir.filePath("da_log.log");
    const QString slowLog = mTempDir.filePath("slow.log");
    gCapture              = nullptr;
    DA::DALogger::instance().setupRotatingFile(mainLog, 10 * 1024 * 1024, 3, false);

    // 直接经 category 触发（任何编译开关下都验证分流逻辑）
    qCWarning(DA_SLOW).noquote() << "[slow] tag=test.route.direct ms=1 self_ms=1 depth=0 thread=main";
    daInfo << "business line for routing test";
    qInfo() << "system line for routing test";
#if defined(DA_ENABLE_SLOW_LOG)
    {
        DA_SLOW_LOG_SCOPE_MS("test.route.scope", 1);
        QThread::msleep(10);
    }
#endif
    DA::DALogger::instance().flush();

    const QString mainContent = readFileAllEventually(
        mainLog,
        { QStringLiteral("business line for routing test"), QStringLiteral("system line for routing test") });
#if defined(DA_ENABLE_SLOW_LOG)
    const QString slowContent = readFileAllEventually(
        slowLog,
        { QStringLiteral("tag=test.route.direct"), QStringLiteral("tag=test.route.scope") });
    QVERIFY2(slowContent.contains("tag=test.route.direct"),
             qPrintable(QStringLiteral("slow.log missing direct record:\n%1").arg(slowContent)));
    QVERIFY2(slowContent.contains("tag=test.route.scope"),
             qPrintable(QStringLiteral("slow.log missing scope record:\n%1").arg(slowContent)));
    // 慢日志文件不含业务/系统日志
    QVERIFY(!slowContent.contains("business line"));
    QVERIFY(!slowContent.contains("system line"));
#else
    // 编译期关闭：不创建慢日志 logger，slow.log 不应存在，daworkbench.slow 消息被丢弃
    Q_UNUSED(slowLog)
    QVERIFY2(!QFile::exists(slowLog), "slow.log must not be created when DA_ENABLE_SLOW_LOG is off");
#endif
    // 主日志不含慢日志
    QVERIFY2(mainContent.contains("business line for routing test"),
             qPrintable(QStringLiteral("da_log.log missing business record:\n%1").arg(mainContent)));
    QVERIFY(mainContent.contains("system line for routing test"));
    QVERIFY2(!mainContent.contains("[slow]"),
             qPrintable(QStringLiteral("da_log.log polluted by slow log:\n%1").arg(mainContent)));

    // UI 队列隔离：慢日志不进 DAMessageLogQueue
    DA::DAMessageLogQueue::instance().clear();
    qCWarning(DA_SLOW).noquote() << "[slow] tag=test.route.ui ms=1";
    daInfo << "ui visible line";
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < 5000 && DA::DAMessageLogQueue::instance().size() < 1) {
        QTest::qWait(50);
    }
    // 再等一段时间，确认慢日志不会迟到进入队列
    QTest::qWait(500);
    DA::DALogger::instance().flush();
    const int n = DA::DAMessageLogQueue::instance().size();
    QCOMPARE(n, 1);
    QCOMPARE(DA::DAMessageLogQueue::instance().at(0).getMsg(), QStringLiteral("ui visible line"));
}

QTEST_MAIN(DASlowLogTest)
#include "main.moc"
