#ifndef DADUMPCAPTURE_H
#define DADUMPCAPTURE_H
// stl
#include <functional>
// qt
#include <QtGlobal>
#include <QObject>
#include <QMessageBox>
#include <QDir>
#include <QApplication>
#include <QDateTime>
#include <QTextStream>
#include <QSysInfo>

/**
 * @brief QTextStream 对QSysInfo的重载
 * @param st
 * @param c
 * @return
 */
QTextStream& operator<<(QTextStream& st, const QSysInfo& c)
{
	st << "buildAbi=" << QSysInfo::buildAbi()                                  // buildAbi
	   << "\r\nbuildCpuArchitecture=" << QSysInfo::buildCpuArchitecture()      // buildCpuArchitecture
	   << "\r\ncurrentCpuArchitecture=" << QSysInfo::currentCpuArchitecture()  // currentCpuArchitecture
	   << "\r\nkernelType=" << QSysInfo::kernelType()                          // kernelType
	   << "\r\nkernelVersion=" << QSysInfo::kernelVersion()                    // kernelVersion
	   << "\r\nmachineHostName=" << QSysInfo::machineHostName()                // machineHostName
	   << "\r\nprettyProductName=" << QSysInfo::prettyProductName()            // prettyProductName
	   << "\r\nproductType=" << QSysInfo::productType()                        // productType
	   << "\r\nproductVersion=" << QSysInfo::productVersion()                  // productVersion
	   << Qt::endl;
	return st;
}
// win32
#ifdef Q_OS_WIN
#ifdef Q_CC_MSVC

// msvc下才有用
#include <Windows.h>
#include <DbgHelp.h>
#endif
#endif

namespace DA
{
/**
 * @brief 函数指针，这个函数指针会在形成dump文件之前执行，要求返回形成dump文件的完整路径，如果没有会赋予一个默认的
 */
using FpPreposeDump     = std::function< QString() >;
FpPreposeDump g_fp_dump = nullptr;

// win32
#ifdef Q_OS_WIN
#ifdef Q_CC_MSVC

LONG applicationCrashHandler(_EXCEPTION_POINTERS* pException);
// 程式异常捕获
LONG applicationCrashHandler(_EXCEPTION_POINTERS* pException)
{
	QString createPath = g_fp_dump();
	qDebug() << "application have been dump,write dump file at " << createPath;
	std::wstring wlpstr = createPath.toStdWString();
	LPCWSTR lpcwStr     = wlpstr.c_str();

	HANDLE hDumpFile = CreateFile(lpcwStr, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile != INVALID_HANDLE_VALUE) {
		// Dump信息
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ExceptionPointers = pException;
		dumpInfo.ThreadId          = GetCurrentThreadId();
		dumpInfo.ClientPointers    = FALSE;
		// 写入Dump文件内容
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

#endif
#endif

/**
 * @brief The DADumpCapture class
 */
class DADumpCapture
{
public:
public:
	/**
	 * @brief 初始化dump
	 * @param fp 前置处理函数指针，返回dump文件的路径
	 */
	static void initDump(FpPreposeDump fp = nullptr)
	{
#ifdef Q_OS_WIN
#ifdef Q_CC_MSVC
		if (nullptr == fp) {
			fp = []() -> QString {
				QString dumppath = QDir::toNativeSeparators(QApplication::applicationDirPath() + "/dumps");
				QDir dir;
				// 确保目录存在
				dir.mkpath(dumppath);
				dumppath = QString("%1/dump%2.dmp").arg(dumppath).arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss.zzz"));
				return QDir::toNativeSeparators(dumppath);
			};
		}
		g_fp_dump = fp;
		SetUnhandledExceptionFilter(applicationCrashHandler);  // 注冊异常捕获函数
#endif
#endif
	}
};

}  // end DA
#endif  // DADUMPCAPTURE_H
