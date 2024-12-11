#ifndef DAMESSAGEHANDLER_H
#define DAMESSAGEHANDLER_H
#include "DAMessageHandlerGlobal.h"
#include <QObject>
#include <QTextStream>
#include <string>
namespace DA
{

/**
 * @brief QString转换为系统编码的wstring
 *
 * 此函数主要针对windows操作系统，把QString转换为系统编码的std::wstring
 */
std::wstring QStringToSystemWString(const QString& qstr);

/**
 * @brief 禁止MessageQueueCapture，如果针对一些qDebug、qInfo不想被捕获的，可以调用此函数，次数消息不会推入队列中，但会被spdlog写入文件中
 */

void DAMESSAGEHANDLER_API daDisableMessageQueueCapture();
/**
   @brief 允许MessageQueueCapture
 */
void DAMESSAGEHANDLER_API daEnableMessageQueueCapture();

/**
   @brief 判断当前是否运行消息捕获
   @return
 */
bool DAMESSAGEHANDLER_API daIsEnableMessageQueueCapture();

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
