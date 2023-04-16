#ifndef DACSVSTREAM_H
#define DACSVSTREAM_H
class QTextStream;
class QFile;
#include "DAUtilsAPI.h"
#include <QString>
namespace DA
{
///
/// \brief 写csv文件类支持
/// \author czy
/// \date 2016-08-10
///
class DAUTILS_API DACsvStream
{
    DA_DECLARE_PRIVATE(DACsvStream)
public:
    DACsvStream(QTextStream* txt);
    DACsvStream(QFile* txt);
    virtual ~DACsvStream();
    //转换为标识csv字符
    static QString toCsvString(const QString& rawStr);
    //把一行要用逗号分隔的字符串转换为一行标准csv字符串
    static QString toCsvStringLine(const QStringList& sectionLine);
    //解析一行csv字符
    static QStringList fromCsvLine(const QString& lineStr);
    DACsvStream& operator<<(const QString& str);
    DACsvStream& operator<<(int d);
    DACsvStream& operator<<(double d);
    DACsvStream& operator<<(float d);
    //另起一行
    void newLine();
    //获取输入输出流
    QTextStream* streamPtr() const;
    QTextStream& stream();
    const QTextStream& stream() const;
    //读取并解析一行csv字符串
    QStringList readCsvLine();
    //判断是否到文件末端
    bool atEnd() const;
    void flush();

private:
    static int advquoted(const QString& s, QString& fld, int i);
    static int advplain(const QString& s, QString& fld, int i);
};
}

typedef DA::DACsvStream& (*SACsvWriterFunction)(DA::DACsvStream&);

inline DA::DACsvStream& operator<<(DA::DACsvStream& s, SACsvWriterFunction f)
{
    return (*f)(s);
}
DAUTILS_API DA::DACsvStream& endl(DA::DACsvStream& s);

#endif  // QCSVSTREAM_H
