#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <thread>
#include <vector>
#include "DALogCategory.h"
#include "DALogger.h"
#include "DAMessageLogQueue.h"

/**
 * @brief 测试消息处理
 *
 * 验证 DALogger + DAMessageLogQueue + DALogCategory 便捷宏的功能：
 * - 旋转文件日志写入
 * - 业务日志（daInfo/daWarning/daCritical，category=da.user）进入 UI 队列
 * - 系统日志（无 category 的 qDebug/qInfo 等）不进入 UI 队列
 * - daDebug 不进入 UI 队列（sink level=info），但写文件
 * - 多线程并发安全
 */
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 注册旋转文件日志
    DA::DALogger::instance().setupRotatingFile("./log/da_log.log");

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

    // 等待 spdlog 异步线程处理
    QTimer::singleShot(200, [ &app ]() {
        int queueSize = DA::DAMessageLogQueue::instance().size();
        // 预期：daInfo + daWarning + daCritical = 3 条（daDebug 不进，sink level=info）
        // 系统日志 4 条都不进 UI 队列
        qDebug() << "Queue size (expect 3 for info+warn+critical business):" << queueSize;

        // 多线程并发测试
        std::vector< std::thread > threads;
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([ i ]() {
                for (int j = 0; j < 100; ++j) {
                    daWarning << "thread" << i << "msg" << j;
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }

        // 等待异步处理完成后退出
        QTimer::singleShot(500, [ &app ]() {
            int finalSize = DA::DAMessageLogQueue::instance().size();
            qDebug() << "Final queue size:" << finalSize;
            app.quit();
        });
    });

    return app.exec();
}
