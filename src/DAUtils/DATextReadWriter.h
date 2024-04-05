#ifndef DATEXTREADWRITER_H
#define DATEXTREADWRITER_H
#include <QObject>
#include <QFile>
#include <QMutex>
#include <functional>
#include "DAUtilsAPI.h"
class QTextStream;
namespace DA
{

///
/// \brief 用于读写文件，但可以多线程调用，通过信号槽传递结果
/// \author czy
/// \date 20170408
///
class DAUTILS_API DATextReadWriter : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DATextReadWriter)
public:
    enum ErrorCode
    {
        NoError    = QFileDevice::NoError,           ///< No error occurred.
        ReadError  = QFileDevice::ReadError,         ///< An error occurred when reading from the file.
        WriteError = QFileDevice::WriteError,        ///< An error occurred when writing to the file.
        FatalError = QFileDevice::FatalError,        ///< A fatal error occurred.
        ResourceError = QFileDevice::ResourceError,  ///< Out of resources (e.g., too many open files, out of memory, etc.)
        OpenError        = QFileDevice::OpenError,         ///< The file could not be opened.
        AbortError       = QFileDevice::AbortError,        ///< The operation was aborted.
        TimeOutError     = QFileDevice::TimeOutError,      ///< A timeout occurred.
        UnspecifiedError = QFileDevice::UnspecifiedError,  ///< An unspecified error occurred.
        RemoveError      = QFileDevice::RemoveError,       ///< The file could not be removed.
        RenameError      = QFileDevice::RenameError,       ///< The file could not be renamed.
        PositionError    = QFileDevice::PositionError,     ///< The position in the file could not be changed.
        ResizeError      = QFileDevice::ResizeError,       ///< The file could not be resized.
        PermissionsError = QFileDevice::PermissionsError,  ///< The file could not be accessed.
        CopyError        = QFileDevice::CopyError,         ///< The file could not be copied.
        UserTerminate                                      ///< 用户中断
    };
    Q_ENUM(ErrorCode)
public:
    using StringFun = std::function< QString(const QString&) >;

public:
    DATextReadWriter(QObject* parent = nullptr);
    ~DATextReadWriter();
    void setFileName(const QString& name);
    // 设置总共读取的行数
    int getTotalReadLineCount() const;
    void setTotalReadLineCount(int totalReadLineCount);
    // 设置一次读取的行数
    int getReadOnceLineCount() const;
    void setReadOnceLineCount(int readOnceLineCount);
    // 设置总共读取的字符数
    int getTotalReadCharCount() const;
    void setTotalReadCharCount(int totalReadCharCount);
    // 设置一次读取的字符数
    int getReadOnceCharCount() const;
    void setReadOnceCharCount(int readOnceCharCount);
    // 设置跳过的行数
    void setSkipHeader(std::size_t v);
    int getSkipHeader() const;
    // 文件是否已经打开
    bool isOpen() const;
    // 打开文件
    bool open(QIODevice::OpenMode mode = QIODevice::ReadOnly | QIODevice::Text);
    // 最后一次错误信息
    QString getLastErrorString() const;
    // 编码
    QString getCodec() const;
    void setCodec(const QString& codec);
    // 处理每次读取的函数指针，默认为nullptr
    void setStringFun(const StringFun& fun);
    // 停止读取
    void setStopRead(bool stopRead);
    // 错误码转换为文字
    static QString errorCodeToString(ErrorCode c);
public slots:
    // 开始读取数据，读取完成会发射readComplete信号
    void startReadText();
    // 插入文本
    void appendText(const QString& text);
    // 插入一行
    void appendLine(const QString& text);
    // flush
    bool flush();
    // 传入一个操作QFile的函数指针，此函数指针用于操作内部保存的QFile
    void operatorFile(std::function< void(QFile*) > fun);
private slots:
    void onStartReadText();
signals:
    /**
     * @brief 完成一次读取会返回此信号
     * @param str 读取的文字
     * @param reachEnd 是否到达最后，为true，说明全文读取完毕
     */
    void readComplete(const QString& str, bool reachEnd);

    /**
     * @brief 开始读取发射的信号，读取完成会发射readComplete信号
     */
    void startedReadText();

    /**
     * @brief 完成信号
     * @param errcode 错误码，NoError代表正常完成
     */
    void finished(int errcode);
#if 0
public slots:
    //追加文本，完成会发射appendTextComplete信号
    void appendText(const QString & text);
signals:
    //appendText完成后返回的信号
    void appendComplete(bool isSuccess);
#endif
};

}

#endif  // DATEXTREADWRITETHREAD_H
