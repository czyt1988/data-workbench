#include "DAMessageLogItem.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAMessageLogItem
//===================================================
DAMessageLogItem::DAMessageLogItem()
    : mValidFlag(false), mMsgType(QtDebugMsg), mMsg(""), mDatetime(QDateTime::currentDateTime())
{
    mFileName     = "";
    mFunctionName = "";
    mLine         = -1;
}

DAMessageLogItem::DAMessageLogItem(QtMsgType type, const QMessageLogContext& context, const QString& m)
    : mValidFlag(true), mMsgType(type), mMsg(m), mDatetime(QDateTime::currentDateTime())
{
    mFileName     = (context.file ? context.file : "");
    mFunctionName = (context.function ? context.function : "");
    mLine         = context.line;
}

/**
 * @brief 从基本类型构造
 *
 * 供 DAMessageLogSink 从 spdlog::details::log_msg 重建 DAMessageLogItem 使用
 * @param type 消息类型
 * @param fileName 文件名
 * @param functionName 函数名
 * @param line 行号
 * @param msg 消息内容
 * @param dt 时间戳
 */
DAMessageLogItem::DAMessageLogItem(QtMsgType type, const QString& fileName, const QString& functionName,
                                   int line, const QString& msg, const QDateTime& dt)
    : mValidFlag(true), mMsgType(type), mMsg(msg), mDatetime(dt), mFileName(fileName),
      mFunctionName(functionName), mLine(line)
{
}

bool DAMessageLogItem::isValid() const
{
    return mValidFlag;
}

QString DAMessageLogItem::datetimeToString(bool showms) const
{
    return mDatetime.toString(showms ? "yyyy-MM-dd HH:mm:ss.zzz" : "yyyy-MM-dd HH:mm:ss");
}

const QString& DAMessageLogItem::getMsg() const
{
    return mMsg;
}

const QString& DAMessageLogItem::getFileName() const
{
    return mFileName;
}

const QString& DAMessageLogItem::getFunctionName() const
{
    return mFunctionName;
}

QtMsgType DAMessageLogItem::getMsgType() const
{
    return mMsgType;
}

int DAMessageLogItem::getLine() const
{
    return mLine;
}

const QDateTime& DAMessageLogItem::getDateTime() const
{
    return mDatetime;
}
