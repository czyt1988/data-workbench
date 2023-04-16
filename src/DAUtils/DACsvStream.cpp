#include "DACsvStream.h"
#include <QTextStream>
#include <QFile>
namespace DA
{
class DACsvStream::PrivateData
{
    DA_DECLARE_PUBLIC(DACsvStream)
private:
    QTextStream* mTxt { nullptr };
    bool mIsStartLine { true };
    QScopedPointer< QTextStream > mTextStream;  ///< 这个是用来临时生成的stream，之所以要两个stream是因为这个是用于内部生成的临时stream，而mTxt是QTextStream的代理

public:
    PrivateData(DACsvStream* p) : q_ptr(p)
    {
    }
    void setTextStream(QTextStream* st)
    {
        mTxt = st;
    }
    QTextStream* streamPtr()
    {
        return mTxt;
    }
    QTextStream& stream()
    {
        return (*mTxt);
    }
    const QTextStream& stream() const
    {
        return (*mTxt);
    }
    bool isStartLine() const
    {
        return mIsStartLine;
    }
    void setStartLine(bool on)
    {
        mIsStartLine = on;
    }

    void setFile(QFile* txt)
    {
        mTextStream.reset(new QTextStream(txt));
        mTxt = mTextStream.data();
    }

    QTextStream& formatTextStream()
    {
        if (!isStartLine()) {
            stream() << ',';
        } else {
            setStartLine(false);
        }
        return stream();
    }
};

//==============================================================
// DACsvStream
//==============================================================

DACsvStream::DACsvStream(QTextStream* txt) : DA_PIMPL_CONSTRUCT
{
    d_ptr->setTextStream(txt);
}

DACsvStream::DACsvStream(QFile* txt) : DA_PIMPL_CONSTRUCT
{
    d_ptr->setFile(txt);
}

DACsvStream::~DACsvStream()
{
}
///
/// \brief 把字符串装换为标准csv一个单元得字符串，对应字符串原有的逗号会进行装换
///
/// csv的原则是：
///
/// - 如果字符串有逗号，把整个字符串前后用引号括起来
/// - 如果字符串有引号",引号要用两个引号表示转义""
/// \param rawStr 原有数据
/// \return 标准的csv单元字符串
///
QString DACsvStream::toCsvString(const QString& rawStr)
{
    //首先查找有没有引号,如果有，则把引号替换为两个引号
    QString res = rawStr;
    res.replace("\"", "\"\"");
    if (res.contains(',')) {
        //如果有逗号，在前后把整个句子用引号包含
        res = ('\"' + res + '\"');
    }
    return res;
}
///
/// \brief 把一行要用逗号分隔的字符串转换为一行标准csv字符串
/// \param sectionLine 如：xxx,xxxx,xxxxx 传入{'xxx','xxxx','xxxxx'}
/// \return 标准的csv字符串不带换行符
///
QString DACsvStream::toCsvStringLine(const QStringList& sectionLine)
{
    QString res;
    int size = sectionLine.size();
    for (int i = 0; i < size; ++i) {
        if (0 == i) {
            res += DACsvStream::toCsvString(sectionLine[ i ]);
        } else {
            res += ("," + DACsvStream::toCsvString(sectionLine[ i ]));
        }
    }
    return res;
}
///
/// \brief 把一句csv格式的内容解析
/// \param lineStr
/// \return
///
QStringList DACsvStream::fromCsvLine(const QString& lineStr)
{
    if (lineStr.isEmpty()) {
        return QStringList();
    }
    QStringList res;
    QString str;
    int i, j = 0;
    int col = 0;
    i       = 0;
    do {
        if (i < lineStr.length() && lineStr[ i ] == '\"')
            j = advquoted(lineStr, str, ++i);  // skip quote
        else
            j = advplain(lineStr, str, i);
        res.push_back(str);
        ++col;
        i = j + 1;
    } while (j < lineStr.length());
    return res;
}
///
/// \brief 写csv文件内容，字符会自动转义为csv文件支持的字符串，不需要转义
///
/// 例如csv文件：
/// \par
/// 12,txt,23,34
/// 45,num,56,56
/// 写入的方法为：
/// \code
/// .....
/// QCsvWriter csv(&textStream);
/// csv<<"12"<<"txt"<<"23";
/// csv.endLine("34");
/// csv<<"45"<<"num"<<"56";
/// csv.endLine("56");
/// \endcode
///
/// \param str 需要写入的csv文件一个单元得字符串
/// \return
///
DACsvStream& DACsvStream::operator<<(const QString& str)
{
    d_ptr->formatTextStream() << toCsvString(str);
    return *this;
}

DACsvStream& DACsvStream::operator<<(int d)
{
    d_ptr->formatTextStream() << d;
    return *this;
}

DACsvStream& DACsvStream::operator<<(double d)
{
    d_ptr->formatTextStream() << d;
    return *this;
}

DACsvStream& DACsvStream::operator<<(float d)
{
    d_ptr->formatTextStream() << d;
    return *this;
}

///
/// \brief 换行
///
void DACsvStream::newLine()
{
    d_ptr->setStartLine(true);
    d_ptr->stream() << endl;
}

QTextStream* DACsvStream::streamPtr() const
{
    return d_ptr->streamPtr();
}

QTextStream& DACsvStream::stream()
{
    return d_ptr->stream();
}

const QTextStream& DACsvStream::stream() const
{
    return d_ptr->stream();
}
///
/// \brief 读取一行csv文件
/// \return
///
QStringList DACsvStream::readCsvLine()
{
    return fromCsvLine(stream().readLine());
}

bool DACsvStream::atEnd() const
{
    return stream().atEnd();
}

void DACsvStream::flush()
{
    stream().flush();
}

int DACsvStream::advquoted(const QString& s, QString& fld, int i)
{
    int j;

    fld = "";
    for (j = i; j < s.length() - 1; j++) {
        if (s[ j ] == '\"' && s[ ++j ] != '\"') {
            int k = s.indexOf(',', j);
            if (k < 0)  // 没找到逗号
                k = s.length();
            for (k -= j; k-- > 0;)
                fld += s[ j++ ];
            break;
        }
        fld += s[ j ];
    }
    return j;
}

int DACsvStream::advplain(const QString& s, QString& fld, int i)
{
    int j;

    j = s.indexOf(',', i);  // 查找,
    if (j < 0)              // 没找到
        j = s.length();
    fld = s.mid(i, j - i);  //提取内容
    return j;
}

}

///
/// \brief endl
/// \param s
/// \return
///
DA::DACsvStream& endl(DA::DACsvStream& s)
{
    s.newLine();
    return s;
}
