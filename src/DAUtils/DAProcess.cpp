#include "DAProcess.h"
#include <QTextStream>
#include <QThread>
namespace DA
{

DAProcess::DAProcess(QObject* par) : QProcess(par)
{
    connect(this, &DAProcess::readyReadStandardError, this, &DAProcess::onReadyReadStandardError);
    connect(this, &DAProcess::readyReadStandardOutput, this, &DAProcess::onReadyReadStandardOutput);
}

void DAProcess::run()
{
    start(ReadWrite);
}

void DAProcess::run(QIODevice::OpenMode mode)
{
    start(mode);
}

void DAProcess::run(const QString& program, const QStringList& arguments, QIODevice::OpenMode mode)
{
    start(program, arguments, mode);
}

void DAProcess::run(const QString& command, QIODevice::OpenMode mode)
{
    start(command, mode);
}

void DAProcess::onReadyReadStandardOutput()
{
    QByteArray allout = readAll();
    QTextStream ss(&allout);
    while (!ss.atEnd()) {
        QString line = ss.readLine();
        emit processStarandOutput(line);
    }
}

void DAProcess::onReadyReadStandardError()
{
    QByteArray allout = readAll();
    QTextStream ss(&allout);
    while (!ss.atEnd()) {
        QString line = ss.readLine();
        emit processErrorOutput(line);
    }
}

//----------------------------------------------------
// DAProcessWithThread
//----------------------------------------------------

DAProcessWithThread::DAProcessWithThread(QObject* par) : QObject(par)
{
}

DAProcessWithThread::~DAProcessWithThread()
{
}

/**
 * @brief 设置参数
 * @param arguments
 */
void DAProcessWithThread::setArguments(const QStringList& arguments)
{
    mArguments = arguments;
}

/**
 * @brief 参数
 * @return
 */
QStringList DAProcessWithThread::getArguments() const
{
    return mArguments;
}

void DAProcessWithThread::setProgram(const QString& program)
{
    mProgram = program;
}

void DAProcessWithThread::runProcess()
{
    mLastError = QString();
    mProcess   = new DAProcess();
    mThread    = new QThread();
    mProcess->moveToThread(mThread);
    connect(mProcess, QOverload< int >::of(&DA::DAProcess::finished), mThread, &QThread::quit);  //进程结束，线程退出
    connect(mThread, &QThread::finished, mProcess, &DA::DAProcess::deleteLater);  //线程结束了，实例销毁
    connect(mThread, &QThread::finished, mThread, &QThread::deleteLater);         //线程结束了，线程自毁
    connect(mThread, &QThread::finished, this, [ this ]() {
        mProcess = nullptr;
        mThread  = nullptr;
    });  //线程结束了，指针清空
    //把beginRunProcess 和DAProcess::run的槽绑定
    connect(this, &DAProcessWithThread::beginRunProcess, mProcess, QOverload< void >::of(&DA::DAProcess::run));
    //错误发生
    connect(mProcess, &DA::DAProcess::errorOccurred, this, [ this ](QProcess::ProcessError error) {
        QString errstr;
        if (this->mProcess) {
            errstr = this->mProcess->errorString();
        }
        this->mLastError = errstr;
        emit errorOccurred(error, errstr);
    });
    connect(mProcess, &DA::DAProcess::started, this, &DAProcessWithThread::processStarted);
    connect(mProcess, QOverload< int >::of(&DA::DAProcess::finished), this, &DAProcessWithThread::processFinished);
    connect(mProcess, &DA::DAProcess::processStarandOutput, this, &DAProcessWithThread::processStarandOutput);
    connect(mProcess, &DA::DAProcess::processErrorOutput, this, &DAProcessWithThread::processErrorOutput);
    mProcess->setProgram(mProgram);
    mProcess->setArguments(mArguments);
    mThread->start();
    //
    emit beginRunProcess();
}

QString DAProcessWithThread::getProgram() const
{
    return mProgram;
}

}  // end DA
