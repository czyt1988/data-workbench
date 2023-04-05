#include "DAMessageLogItem.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAMessageLogItem
//===================================================
DAMessageLogItem::DAMessageLogItem()
    : m_validFlag(false), m_msgType(QtDebugMsg), m_msg(""), m_datetime(QDateTime::currentDateTime())
{
    m_fileName     = "";
    m_functionName = "";
    m_line         = -1;
}

DAMessageLogItem::DAMessageLogItem(QtMsgType type, const QMessageLogContext& context, const QString& m)
    : m_validFlag(true), m_msgType(type), m_msg(m), m_datetime(QDateTime::currentDateTime())
{
    m_fileName     = (context.file ? context.file : "");
    m_functionName = (context.function ? context.function : "");
    m_line         = context.line;
}

DAMessageLogItem::DAMessageLogItem(const DAMessageLogItem& i)
{
    operator=(i);
}

DAMessageLogItem& DAMessageLogItem::operator=(const DAMessageLogItem& i)
{
    m_validFlag    = i.m_validFlag;
    m_msgType      = i.m_msgType;
    m_msg          = i.m_msg;
    m_datetime     = i.m_datetime;
    m_fileName     = i.m_fileName;
    m_functionName = i.m_functionName;
    m_line         = i.m_line;
    return *this;
}

bool DAMessageLogItem::isValid() const
{
    return m_validFlag;
}

QString DAMessageLogItem::datetimeToString(bool showms) const
{
    return m_datetime.toString(showms ? "yyyy-MM-dd HH:mm:ss.zzz" : "yyyy-MM-dd HH:mm:ss");
}

const QString& DAMessageLogItem::getMsg() const
{
    return m_msg;
}

const QString& DAMessageLogItem::getFileName() const
{
    return m_fileName;
}

const QString& DAMessageLogItem::getFunctionName() const
{
    return m_functionName;
}

QtMsgType DAMessageLogItem::getMsgType() const
{
    return m_msgType;
}

int DAMessageLogItem::getLine() const
{
    return m_line;
}

const QDateTime& DAMessageLogItem::getDateTime() const
{
    return m_datetime;
}
