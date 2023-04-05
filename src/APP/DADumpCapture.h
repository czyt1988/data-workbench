#ifndef DADUMPCAPTURE_H
#define DADUMPCAPTURE_H
#include <QtGlobal>
#include <QObject>
#include <QMessageBox>
#include <QDir>
#include <QApplication>
#include <QDateTime>
// win32
#ifdef Q_OS_WIN
#ifdef Q_CC_MSVC
// msvc下才有用
#include <Windows.h>
#include <DbgHelp.h>
namespace DA
{

//程式异常捕获
LONG applicationCrashHandler(EXCEPTION_POINTERS* pException)
{
    QString createPath = QApplication::applicationDirPath() + "/Dumps";
    QDir dir;
    dir.mkpath(createPath);
    createPath = QString("%1/dump%2.dmp").arg(createPath).arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss.zzz"));
    std::wstring wlpstr = createPath.toStdWString();
    LPCWSTR lpcwStr     = wlpstr.c_str();

    HANDLE hDumpFile = CreateFile(lpcwStr, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDumpFile != INVALID_HANDLE_VALUE) {
        // Dump信息
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = pException;
        dumpInfo.ThreadId          = GetCurrentThreadId();
        dumpInfo.ClientPointers    = FALSE;
        //写入Dump文件内容
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
    static void initDump()
    {
#ifdef Q_OS_WIN
#ifdef Q_CC_MSVC
        SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)applicationCrashHandler);  //注冊异常捕获函数
#endif
#endif
    }
};

}  // end DA
#endif  // DADUMPCAPTURE_H
