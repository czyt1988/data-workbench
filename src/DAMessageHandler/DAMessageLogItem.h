#ifndef DAMESSAGELOGITEM_H
#define DAMESSAGELOGITEM_H
#include <QMetaType>
#include <QDateTime>
#include <memory>
#include "DAMessageHandlerGlobal.h"
namespace DA
{
/**
 * @brief 包含了一个信息的所有内容
 */
class DAMESSAGEHANDLER_API DAMessageLogItem
{
public:
    //构造一个无效的
    DAMessageLogItem();
    //构造一个有效的
    DAMessageLogItem(QtMsgType type, const QMessageLogContext& context, const QString& m);
    //拷贝构造
    DAMessageLogItem(const DAMessageLogItem& i);
    //赋值操作
    DAMessageLogItem& operator=(const DAMessageLogItem& i);

public:
    bool isValid() const;
    QString datetimeToString(bool showms = false) const;
    const QString& getMsg() const;
    const QString& getFileName() const;
    const QString& getFunctionName() const;
    const QDateTime& getDateTime() const;
    QtMsgType getMsgType() const;
    int getLine() const;

private:
    bool m_validFlag;
    QtMsgType m_msgType;
    QString m_msg;
    QDateTime m_datetime;
    QString m_fileName;
    QString m_functionName;
    int m_line;
};
}  // namespace DA
Q_DECLARE_METATYPE(DA::DAMessageLogItem)
#endif  // DAMESSAGELOGITEM_H
