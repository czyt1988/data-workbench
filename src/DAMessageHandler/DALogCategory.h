#ifndef DALOGCATEGORY_H
#define DALOGCATEGORY_H
#include "DAMessageHandlerGlobal.h"
#include <QLoggingCategory>

/**
 * @brief 用户可见业务日志的默认 category
 *
 * 字符串为 "da.user"，DALogger::dispatchMessage 会将 "da." 前缀的 category
 * 路由到带 UI sink 的业务 logger，从而进入 DAMessageLogQueue → UI 日志窗口。
 *
 * 业务代码应通过 daInfo/daWarning/daCritical/daDebug 便捷宏使用此 category，
 * 无需自行声明 Q_LOGGING_CATEGORY。
 *
 * @note DAMessageHandler 模块内部不应使用 da.* 前缀的 category，
 *       以避免日志循环（日志系统自身的日志不需要进入 UI）。
 *
 * @see DALogger
 */
DAMESSAGEHANDLER_API Q_DECLARE_LOGGING_CATEGORY(DA_USER)

/**
 * @brief 便捷日志宏，使用 DA_USER category，日志会进入 UI 日志窗口
 *
 * 用法与 qDebug/qInfo/qWarning/qCritical 一致，支持 << 链式输出：
 * @code
 * daInfo << "kernel initialized, version =" << DA_VERSION;
 * daWarning << "config file not found:" << path;
 * daCritical << "failed to open project:" << err;
 * @endcode
 *
 * 这些宏内部展开为 qCInfo(DA_USER)/qCWarning(DA_USER)/qCCritical(DA_USER)/qCDebug(DA_USER)，
 * category 为 "da.user"，会被 DALogger 识别为业务日志并推送到 UI 队列。
 *
 * @note daDebug 默认不进入 UI 队列（UI sink level 为 info），但会写入日志文件。
 *
 * @see DALogger::dispatchMessage
 */
#define daInfo     qCInfo(DA_USER)
#define daWarning  qCWarning(DA_USER)
#define daCritical qCCritical(DA_USER)
#define daDebug    qCDebug(DA_USER)

#endif  // DALOGCATEGORY_H
