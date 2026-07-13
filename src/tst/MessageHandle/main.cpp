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

QTEST_MAIN(MessageHandleTest)
#include "main.moc"
