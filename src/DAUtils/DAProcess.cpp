#include "DAProcess.h"
#include <QTextStream>
#include <QTextCodec>
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

void DAProcess::setEncoding(const char* codecName)
{
	mCodecName = codecName;
}

void DAProcess::run(const QString& command, QIODevice::OpenMode mode)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	startCommand(command, mode);
#else
	// Qt5 当前构建未提供 startCommand，只能用单字符串 start 重载，局部屏蔽弃用警告
	QT_WARNING_PUSH
	QT_WARNING_DISABLE_DEPRECATED
	start(command, mode);
	QT_WARNING_POP
#endif
}

void DAProcess::onReadyReadStandardOutput()
{
	QByteArray allout = readAllStandardOutput();
	QTextStream ss(&allout);
	setEncoding(&ss, mCodecName);
	while (!ss.atEnd()) {
		QString line = ss.readLine();
		emit processStarandOutput(line);
	}
}

void DAProcess::onReadyReadStandardError()
{
	QByteArray allout = readAllStandardError();
	QTextStream ss(&allout);
	setEncoding(&ss, mCodecName);
	while (!ss.atEnd()) {
		QString line = ss.readLine();
		emit processErrorOutput(line);
	}
}

void DAProcess::setEncoding(QTextStream* ss, const QString& codec)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	if (mCodecName.isEmpty()) {
		ss->setCodec(QTextCodec::codecForLocale());
	} else {
		ss->setCodec(mCodecName.toStdString().c_str());
	}
#else
	if (codec.isEmpty()) {
		ss->setEncoding(QStringConverter::System);
	} else {
		auto e = QStringConverter::encodingForName(codec.toStdString().c_str());
		if (e.has_value()) {
			ss->setEncoding(e.value());
		} else {
			ss->setEncoding(QStringConverter::System);
		}
	}
#endif
}

//----------------------------------------------------
// DAProcessWithThread
//----------------------------------------------------

DAProcessWithThread::DAProcessWithThread(QObject* par) : QObject(par)
{
}

DAProcessWithThread::~DAProcessWithThread()
{
	if (mProcess) {
		// 断开所有信号连接，避免回调到已销毁的 this
		disconnect(this, nullptr, nullptr, nullptr);
		mProcess->kill();
	}
	if (mThread && mThread->isRunning()) {
		mThread->quit();
		mThread->wait(3000);  // 等待线程退出，最多 3 秒
	}
	// mProcess 和 mThread 在线程运行时通过 finished 信号的 deleteLater 自动销毁，
	// 若线程未启动（mThread 非 null 但未 running），需手动清理
	if (mThread && !mThread->isRunning()) {
		delete mThread;
		mThread = nullptr;
	}
	if (mProcess) {
		delete mProcess;
		mProcess = nullptr;
	}
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
	if (!mProcess) {
		mProcess = new DAProcess();
		mThread  = new QThread();
		mProcess->moveToThread(mThread);
		// 双参 finished 重载自 Qt 5.13 前即可用（Qt6 中为唯一形式），Qt5/Qt6 均可直接使用
		connect(mProcess, QOverload< int, QProcess::ExitStatus >::of(&DA::DAProcess::finished), mThread, &QThread::quit);  // 进程结束，线程退出
		connect(mThread, &QThread::finished, mProcess, &DA::DAProcess::deleteLater);  // 线程结束了，实例销毁
		connect(mThread, &QThread::finished, mThread, &QThread::deleteLater);         // 线程结束了，线程自毁
		connect(mThread, &QThread::finished, this, [ this ]() {
			mProcess = nullptr;
			mThread  = nullptr;
		});  // 线程结束了，指针清空
		// 把beginRunProcess 和DAProcess::run的槽绑定
		connect(this, &DAProcessWithThread::beginRunProcess, mProcess, QOverload<>::of(&DA::DAProcess::run));
		connect(this, &DAProcessWithThread::beginKillProcess, mProcess, &DAProcess::kill);
		connect(this, &DAProcessWithThread::beginTerminateProcess, mProcess, &DAProcess::terminate);
		// 错误发生
		connect(mProcess, &DA::DAProcess::errorOccurred, this, [ this ](QProcess::ProcessError error) {
			QString errstr;
			if (this->mProcess) {
				errstr = this->mProcess->errorString();
			}
			this->mLastError = errstr;
			emit errorOccurred(error, errstr);
		});
		connect(mProcess, &DA::DAProcess::started, this, &DAProcessWithThread::processStarted);
		connect(mProcess, QOverload< int, QProcess::ExitStatus >::of(&DA::DAProcess::finished), this, &DAProcessWithThread::processFinished);
		connect(mProcess, &DA::DAProcess::processStarandOutput, this, &DAProcessWithThread::processStarandOutput);
		connect(mProcess, &DA::DAProcess::processErrorOutput, this, &DAProcessWithThread::processErrorOutput);
		mProcess->setProgram(mProgram);
		mProcess->setArguments(mArguments);
		mThread->start();
	}
	//
	emit beginRunProcess();
}

void DAProcessWithThread::kill()
{
	emit beginKillProcess();
}

void DAProcessWithThread::terminate()
{
	emit beginTerminateProcess();
}

QString DAProcessWithThread::getProgram() const
{
	return mProgram;
}

}  // end DA
