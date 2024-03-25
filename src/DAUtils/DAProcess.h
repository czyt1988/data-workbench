#ifndef DAPROCESS_H
#define DAPROCESS_H
#include <QProcess>
#include "DAUtilsAPI.h"
class QThread;
namespace DA
{

/**
 * @brief 对QProcess的封装
 */
class DAUTILS_API DAProcess : public QProcess
{
    Q_OBJECT
public:
    DAProcess(QObject* par = nullptr);
public slots:
    // 对start函数的槽映射，可以通过object-with-thread模式跨线程运行
    void run();
    void run(QIODevice::OpenMode mode);
    void run(const QString& command, QIODevice::OpenMode mode);
    void run(const QString& program, const QStringList& arguments, QIODevice::OpenMode mode);
signals:
    /**
     * @brief 标准输出
     *
     * 此标准输出以行的形式发出
     * @param str
     */
    void processStarandOutput(const QString& str);
    /**
     * @brief 标准错误输出
     *
     * 此标准输出以行的形式发出
     * @param str
     */
    void processErrorOutput(const QString& str);
private slots:
    // 标准输出
    void onReadyReadStandardOutput();
    // 标准错误
    void onReadyReadStandardError();
};

/**
 * @brief 带线程的Process
 *
 * 用法：
 * @code
 * //header
 * class xx{
 *  private:
 *      DA::DAProcessWithThread mProcess;
 * }
 *
 * //cpp
 * mProcess.setProgram(xx);
 * mProcess.setArguments({"--value","12"});
 * @endcode
 */
class DAUTILS_API DAProcessWithThread : public QObject
{
    Q_OBJECT
public:
    DAProcessWithThread(QObject* par = nullptr);
    ~DAProcessWithThread();
    // 参数
    void setArguments(const QStringList& arguments);
    QStringList getArguments() const;
    // 设置程序
    void setProgram(const QString& program);
    QString getProgram() const;

public slots:
    // 运行进程，发射beginRunProcess
    void runProcess();
signals:
    /**
     * @brief 发生错误
     * @param error
     */
    void errorOccurred(QProcess::ProcessError error, const QString& errString);
    /**
     * @brief 标准输出
     *
     * 此标准输出以行的形式发出
     * @param str
     */
    void processStarandOutput(const QString& str);
    /**
     * @brief 标准错误输出
     *
     * 此标准输出以行的形式发出
     * @param str
     */
    void processErrorOutput(const QString& str);
    /**
     * @brief 开始运行进程的信号
     */
    void beginRunProcess();
    /**
     * @brief 进程已经开始
     */
    void processStarted();
    /**
     * @brief 进程结束
     * @param code 结束返回的代码
     */
    void processFinished(int code);

private:
    DAProcess* mProcess { nullptr };
    QThread* mThread { nullptr };
    //
    QString mProgram;
    QStringList mArguments;
    // 记录最后的错误
    QString mLastError;
};

}

#endif  // DAPROCESS_H
