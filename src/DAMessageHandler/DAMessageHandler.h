#ifndef DAMESSAGEHANDLER_H
#define DAMESSAGEHANDLER_H
#include "DAMessageHandlerGlobal.h"
#include <QObject>
namespace DA
{
/**
 * @brief 禁止MessageHandler，如果针对一些qDebug、qInfo不想被捕获的，可以调用此函数，但要保证使用daEnableQtMessageHandler恢复
 */
void DAMESSAGEHANDLER_API daDisableQtMessageHandler();
void DAMESSAGEHANDLER_API daEnableQtMessageHandler();

//注册控制台的消息处理
void DAMESSAGEHANDLER_API daRegisterConsolMessageHandler(int flush_every_sec = 1, bool async_logger = true);

//注册一个旋转文件的消息处理
void DAMESSAGEHANDLER_API daRegisterRotatingMessageHandler(const QString& filename,
                                                           int maxfile_size    = 1048576 * 10,
                                                           int maxfile_counts  = 5,
                                                           int flush_every_sec = 15,
                                                           bool output_stdout  = true,
                                                           bool async_logger   = true);

/**
 * @brief 设置消息的模板
 *
 * da消息占位符有如下：
 *
 * - {level} 代表是否打印消息等级，消息等级有debug/warn/critical/error/info五种
 * - {datetime} 代表日期
 * - {line} 代表打印行号
 * - {fun} 代表打印函数名
 * - {file} 代表打印文件名
 * - {msg} 代表消息主体
 * @param p patter字符串，默认为[{datetime}][{line}]{msg}
 */
void DAMESSAGEHANDLER_API daSetMessagePattern(const QString& p);

/**
 * @brief 设置记录进入全局消息队列的消息等级，默认为QtWarningMsg
 * @sa QtMsgType
 * @param type
 */
void DAMESSAGEHANDLER_API daSetMsgQueueRecordMsgType(QtMsgType type);

/**
 * @brief 注销
 * @note 最后需要调用此函数进行释放
 */
void DAMESSAGEHANDLER_API daUnregisterMessageHandler();

}  // namespace DA

#endif  // DAMESSAGEHANDLER_H
