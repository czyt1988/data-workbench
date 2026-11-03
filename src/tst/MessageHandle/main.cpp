#include <QtTest/QtTest>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QDebug>
#include <thread>
#include <vector>
#include "DALogCategory.h"
#include "DALogger.h"
#include "DAMessageLogQueue.h"

class MessageHandleTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testBusinessLogEntersUiQueue();
    void testSystemLogNotInUiQueue();
    void testMultiThreadConcurrent();
    void testTotalPushedMonotonic();
    void testTotalPushedSurvivesRingOverwrite();
    void testSeverityCounters();

private:
    // 轮询等待队列大小稳定（连续 stableCount 次不变），最多等待 timeoutMs
    int waitForStableQueueSize(int stableCount = 4, int intervalMs = 50, int timeoutMs = 5000);
    // 清理日志文件
    void cleanupLogFiles();

private:
    QTemporaryDir m_tempDir;
    QString m_logPath;
};

void MessageHandleTest::initTestCase()
{
    QVERIFY2(m_tempDir.isValid(), "Failed to create temporary directory for log files");
    m_logPath = m_tempDir.filePath("da_log.log");
    DA::DALogger::instance().setupRotatingFile(m_logPath);
}

void MessageHandleTest::cleanupTestCase()
{
    cleanupLogFiles();
}

void MessageHandleTest::cleanupLogFiles()
{
    // 临时目录会在 QTemporaryDir 析构时自动清理，此处无需额外操作
}

int MessageHandleTest::waitForStableQueueSize(int stableCount, int intervalMs, int timeoutMs)
{
    int lastSize    = -1;
    int stable      = 0;
    int currentSize = 0;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        currentSize = DA::DAMessageLogQueue::instance().size();
        if (currentSize == lastSize) {
            ++stable;
            if (stable >= stableCount) {
                break;
            }
        } else {
            stable   = 0;
            lastSize = currentSize;
        }
        QTest::qWait(intervalMs);
    }
    return currentSize;
}

void MessageHandleTest::testBusinessLogEntersUiQueue()
{
    // 清空队列，确保从干净状态开始
    DA::DAMessageLogQueue::instance().clear();

    // 业务日志（da.user category via 便捷宏）——应进入 UI 队列（除 daDebug）
    daDebug << "business debug (not in UI, sink level=info)";
    daInfo << "business info";
    daWarning << "business warning";
    daCritical << "business critical";

    // 系统日志（无 category）——不应进入 UI 队列
    qDebug() << "system debug (not in UI)";
    qInfo() << "system info (not in UI)";
    qWarning() << "system warning (not in UI)";
    qCritical() << "system critical (not in UI)";

    // 轮询等待 spdlog 异步线程处理完成（队列大小稳定）
    int queueSize = waitForStableQueueSize();
    // 预期：daInfo + daWarning + daCritical = 3 条（daDebug 不进，sink level=info）
    // 系统日志 4 条都不进 UI 队列
    qDebug() << "Queue size (expect 3 for info+warn+critical business):" << queueSize;
    QCOMPARE(queueSize, 3);
}

void MessageHandleTest::testSystemLogNotInUiQueue()
{
    // 此测试已在上一个测试中合并验证（系统日志不进入 UI 队列）
    // 此处单独验证：仅系统日志不应改变队列大小
    DA::DAMessageLogQueue::instance().clear();

    qDebug() << "system debug only";
    qInfo() << "system info only";
    qWarning() << "system warning only";
    qCritical() << "system critical only";

    int queueSize = waitForStableQueueSize();
    qDebug() << "Queue size after system-only logs (expect 0):" << queueSize;
    QCOMPARE(queueSize, 0);
}

void MessageHandleTest::testMultiThreadConcurrent()
{
    // 清空队列
    DA::DAMessageLogQueue::instance().clear();

    // 先记录业务日志的基线
    daInfo << "baseline info before multithread test";
    daWarning << "baseline warning";
    daCritical << "baseline critical";
    // 等待基线稳定
    int baselineSize = waitForStableQueueSize();
    QCOMPARE(baselineSize, 3);

    // 多线程并发测试
    const int threadCount   = 4;
    const int msgsPerThread = 100;
    const int expectedTotal = 3 + threadCount * msgsPerThread;

    std::vector< std::thread > threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([ i, msgsPerThread ]() {
            for (int j = 0; j < msgsPerThread; ++j) {
                daWarning << "thread" << i << "msg" << j;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    // 轮询等待异步处理完成
    int finalSize = waitForStableQueueSize();
    qDebug() << "Final queue size (expect" << expectedTotal << "):" << finalSize;
    QCOMPARE(finalSize, expectedTotal);
}

/**
 * 累计推送总数单调递增：业务日志进入后递增，clear 不回退
 */
void MessageHandleTest::testTotalPushedMonotonic()
{
    DA::DAMessageLogQueue& queue = DA::DAMessageLogQueue::instance();
    queue.clear();
    const qint64 base = queue.totalPushed();

    daInfo << "totalPushed test message 1";
    daWarning << "totalPushed test message 2";
    // 等待 spdlog 异步 sink 推送完成（不校验队列 size）
    waitForStableQueueSize();

    QVERIFY(queue.totalPushed() >= base + 2);
    const qint64 afterPush = queue.totalPushed();

    // clear 只清空缓冲，不回退累计总数（未读水位依赖此语义）
    queue.clear();
    QCOMPARE(queue.totalPushed(), afterPush);
}

/**
 * 环形缓冲写满后：size 封顶不再变化，totalPushed 依然精确递增
 * （未读数 = totalPushed - 已读水位 依赖此语义）
 */
void MessageHandleTest::testTotalPushedSurvivesRingOverwrite()
{
    DA::DAMessageLogQueue& queue = DA::DAMessageLogQueue::instance();
    const int oldCapacity = queue.capacity();
    queue.setCapacity(3);
    const qint64 base = queue.totalPushed();

    const DA::DAMessageLogItem item(QtInfoMsg, "totalPushedTest.cpp", "testTotalPushedSurvivesRingOverwrite", 1,
                                    "ring overwrite message", QDateTime::currentDateTime());
    for (int i = 0; i < 5; ++i) {
        queue.push(item);  // 直接 push（主线程），不经 spdlog
    }
    QCOMPARE(queue.size(), 3);  // 环形覆写：尺寸封顶
    QCOMPARE(queue.totalPushed(), base + 5);  // 累计数不受覆写影响
    queue.setCapacity(oldCapacity);
}

/**
 * 按严重级别累计：仅警告计入警告计数，critical/fatal 计入错误计数，info/debug 不计
 */
void MessageHandleTest::testSeverityCounters()
{
    DA::DAMessageLogQueue& queue = DA::DAMessageLogQueue::instance();
    // 等待上一用例通过 spdlog 异步推送的日志落地，避免计数快照漂移
    waitForStableQueueSize();
    queue.clear();
    const qint64 w0 = queue.totalWarningPushed();
    const qint64 c0 = queue.totalCriticalPushed();
    const qint64 t0 = queue.totalPushed();

    const QDateTime now = QDateTime::currentDateTime();
    auto push           = [ &queue, &now ](QtMsgType type) {
        queue.push(DA::DAMessageLogItem(type, QStringLiteral("severityTest.cpp"),
                                        QStringLiteral("testSeverityCounters"), 1,
                                        QStringLiteral("severity message"), now));
    };
    push(QtInfoMsg);
    push(QtDebugMsg);
    push(QtWarningMsg);
    push(QtCriticalMsg);
    push(QtFatalMsg);
    push(QtWarningMsg);

    QCOMPARE(queue.totalPushed() - t0, qint64(6));
    QCOMPARE(queue.totalWarningPushed() - w0, qint64(2));
    // critical + fatal 均计入错误
    QCOMPARE(queue.totalCriticalPushed() - c0, qint64(2));
}

QTEST_MAIN(MessageHandleTest)
#include "main.moc"
