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

DAMessageLogItem::DAMessageLogItem(const DAMessageLogItem& i)
{
    operator=(i);
}

DAMessageLogItem& DAMessageLogItem::operator=(const DAMessageLogItem& i)
{
    mValidFlag    = i.mValidFlag;
    mMsgType      = i.mMsgType;
    mMsg          = i.mMsg;
    mDatetime     = i.mDatetime;
    mFileName     = i.mFileName;
    mFunctionName = i.mFunctionName;
    mLine         = i.mLine;
    return *this;
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
