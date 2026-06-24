#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <thread>
#include <vector>
#include "DALogger.h"
#include "DAMessageLogQueue.h"

/**
 * @brief 测试消息处理
 *
 * 验证 DALogger + DAMessageLogQueue 的基本功能：
 * - 旋转文件日志写入
 * - UI 队列消息捕获
 * - 多线程并发安全
 */
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 注册旋转文件日志
    DA::DALogger::instance().setupRotatingFile("./log/da_log.log");

    // 基本消息测试
    qDebug() << QStringLiteral(u"数据工作流-版本=v0.0.1");
    qInfo() << QStringLiteral(u"qInfo test");
    qWarning() << QStringLiteral(u"qWarning test");
    qCritical() << QStringLiteral(u"qCritical test");

    // 验证队列中消息数量（UI 队列默认 warn 级别，应只有 2 条）
    // 需要短暂等待 spdlog 异步线程处理
    QTimer::singleShot(200, [&app]() {
        int queueSize = DA::DAMessageLogQueue::instance().size();
        qDebug() << "Queue size (expect >= 2 for warn+critical):" << queueSize;

        // 多线程并发测试
        std::vector< std::thread > threads;
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([i]() {
                for (int j = 0; j < 100; ++j) {
                    qWarning() << "thread" << i << "msg" << j;
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }

        // 等待异步处理完成后退出
        QTimer::singleShot(500, [&app]() {
            int finalSize = DA::DAMessageLogQueue::instance().size();
            qDebug() << "Final queue size:" << finalSize;
            app.quit();
        });
    });

    return app.exec();
}
