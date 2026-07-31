#ifndef DADUMPCAPTURE_H
#define DADUMPCAPTURE_H

// stl
#include <functional>
#include <fstream>
// qt
#include <QDebug>
#include <QtGlobal>
#include <QObject>
#include <QMessageBox>
#include <QDir>
#include <QApplication>
#include <QDateTime>
#include <QTextStream>
#include <QSysInfo>
#include <QVersionNumber>
#include "DADir.h"
// win32
#ifdef Q_OS_WIN
#ifdef Q_CC_MSVC

// msvc下才有用
#include <Windows.h>
#include <DbgHelp.h>
#include <tchar.h>
#pragma comment(lib, "dbghelp.lib")

#endif
#endif

namespace DA
{
/**
 * @brief 函数指针，这个函数指针会在形成dump文件之前执行，要求返回形成dump文件的完整路径，如果没有会赋予一个默认的
 */
using FpPreposeDump = std::function< QString() >;
using FpPostDump    = std::function< void(const QString& dumpPath, bool success) >;

// win32
#ifdef Q_OS_WIN
#ifdef Q_CC_MSVC

LONG applicationCrashHandler(_EXCEPTION_POINTERS* pException);
#endif  // Q_CC_MSVC
#endif  // Q_OS_WIN

/**
 * @brief Dump文件捕获类
 *
 * 用于捕获应用程序崩溃异常，生成dump文件和相关信息文件。
 * 所有方法仅在 Windows/MSVC 下有效。
 */
class DADumpCapture
{
private:
    static FpPreposeDump s_fp_prepose_dump;  ///< 前置处理函数指针
    static FpPostDump s_fp_post_dump;        ///< 后置处理函数指针
    static bool s_initialized;               ///< 初始化状态标志

public:
    static FpPreposeDump getPreposeDumpFunc()  // 获取前置处理函数指针
    {
        return s_fp_prepose_dump;
    }

    static FpPostDump getPostDumpFunc()  // 获取后置处理函数指针
    {
        return s_fp_post_dump;
    }

    static void initDump(FpPreposeDump fpPre = nullptr, FpPostDump fpPost = nullptr)  // 初始化dump捕获
    {
#ifdef Q_OS_WIN
#ifdef Q_CC_MSVC
        if (s_initialized) {
            qDebug() << "Dump capture already initialized";
            return;
        }

        // 设置默认前置处理函数
        if (nullptr == fpPre) {
            fpPre = []() -> QString {
                QString dumppath = getDefaultDumpDirectory();
                QDir dir;
                if (!dir.exists(dumppath)) {
                    if (!dir.mkpath(dumppath)) {
                        qDebug() << "Failed to create dump directory:" << dumppath;
                        // 如果创建失败，使用临时目录
                        dumppath = QDir::toNativeSeparators(QDir::tempPath() + "/app_dumps");
                        dir.mkpath(dumppath);
                    }
                }
                QString filename = QString("dump_%1_%2.dmp")
                                       .arg(QApplication::applicationName())
                                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz"));
                QString fullPath = QString("%1/%2").arg(dumppath).arg(filename);
                return QDir::toNativeSeparators(fullPath);
            };
        }

        s_fp_prepose_dump = fpPre;
        s_fp_post_dump    = fpPost;
        s_initialized     = true;

        // 注册异常捕获函数
        LPTOP_LEVEL_EXCEPTION_FILTER oldFilter = SetUnhandledExceptionFilter(applicationCrashHandler);
        if (oldFilter) {
            qDebug() << "Previous exception filter was replaced";
        }

        qDebug() << "Dump capture initialized successfully";
#endif
#endif
    }

    static bool isInitialized()  // 检查是否已初始化
    {
        return s_initialized;
    }

    static QString getDefaultDumpDirectory()  // 获取默认dump目录
    {
        QString dumppath = DADir::getDumpFilePath();
        QDir dir;
        if (!dir.exists(dumppath)) {
            dir.mkpath(dumppath);
        }
        return dumppath;
    }

    static void cleanupOldDumps(int daysToKeep = 7, const QString& directory = QString())  // 清理旧的dump文件
    {
        QString dumpDir = directory.isEmpty() ? getDefaultDumpDirectory() : directory;
        QDir dir(dumpDir);

        if (!dir.exists()) {
            return;
        }

        QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-daysToKeep);
        QFileInfoList files =
            dir.entryInfoList(QStringList() << "*.dmp" << "*_info.txt", QDir::Files | QDir::NoDotAndDotDot);

        int removedCount = 0;
        for (const QFileInfo& file : files) {
            if (file.lastModified() < cutoffDate) {
                if (QFile::remove(file.absoluteFilePath())) {
                    qDebug() << "Removed old dump file:" << file.fileName();
                    removedCount++;
                }
            }
        }

        if (removedCount > 0) {
            qDebug() << "总共清理了" << removedCount << "个旧文件";
        }
    }
};

// 定义静态成员变量
#ifdef Q_OS_WIN
#ifdef Q_CC_MSVC
inline FpPreposeDump DADumpCapture::s_fp_prepose_dump = nullptr;
inline FpPostDump DADumpCapture::s_fp_post_dump       = nullptr;
inline bool DADumpCapture::s_initialized              = false;

// 程式异常捕获
inline LONG applicationCrashHandler(_EXCEPTION_POINTERS* pException)
{
    QString createPath;
    bool dumpSuccess = false;
    LPCWSTR lpcwStr  = NULL;
    try {
        // 访问DADumpCapture的静态成员变量
        FpPreposeDump fpDump = DADumpCapture::getPreposeDumpFunc();
        if (fpDump) {
            createPath = fpDump();
        } else {
            // 默认路径生成
            QString dumppath = DADumpCapture::getDefaultDumpDirectory();
            QDir dir;
            if (!dir.exists(dumppath)) {
                dir.mkpath(dumppath);
            }
            createPath =
                QString("%1/dump_%2.dmp").arg(dumppath).arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz"));
        }

        qDebug() << "Application crashed, writing dump file at:" << createPath;

        // 记录系统信息到文本文件
        QString infoPath = createPath;
        infoPath.replace(".dmp", "_info.txt");

        // 使用C风格文件操作避免C++异常
        FILE* infoFile = nullptr;
#ifdef _MSC_VER
        fopen_s(&infoFile, infoPath.toLocal8Bit().constData(), "w");
#else
        infoFile = fopen(infoPath.toLocal8Bit().constData(), "w");
#endif

        if (infoFile) {
            // 应用程序信息
            fprintf(infoFile, "=== Application Information ===\n");
            fprintf(infoFile, "Application: %s\n", QApplication::applicationName().toLocal8Bit().constData());
            fprintf(infoFile, "Version: %s\n", QApplication::applicationVersion().toLocal8Bit().constData());
            fprintf(infoFile, "Organization: %s\n", QApplication::organizationName().toLocal8Bit().constData());
            fprintf(infoFile, "Organization Domain: %s\n", QApplication::organizationDomain().toLocal8Bit().constData());

            // 崩溃时间信息
            fprintf(infoFile, "\n=== Crash Information ===\n");
            fprintf(
                infoFile,
                "Crash Time: %s\n",
                QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toLocal8Bit().constData()
            );
            fprintf(infoFile, "Process ID: %lu\n", GetCurrentProcessId());
            fprintf(infoFile, "Thread ID: %lu\n", GetCurrentThreadId());
            fprintf(infoFile, "Exception Code: 0x%08lX\n", pException->ExceptionRecord->ExceptionCode);
            fprintf(infoFile, "Exception Address: 0x%p\n", pException->ExceptionRecord->ExceptionAddress);

            // 系统信息
            fprintf(infoFile, "\n=== System Information ===\n");
            fprintf(infoFile, "Build ABI: %s\n", QSysInfo::buildAbi().toLocal8Bit().constData());
            fprintf(infoFile, "Build CPU Architecture: %s\n", QSysInfo::buildCpuArchitecture().toLocal8Bit().constData());
            fprintf(infoFile, "Current CPU Architecture: %s\n", QSysInfo::currentCpuArchitecture().toLocal8Bit().constData());
            fprintf(infoFile, "Kernel Type: %s\n", QSysInfo::kernelType().toLocal8Bit().constData());
            fprintf(infoFile, "Kernel Version: %s\n", QSysInfo::kernelVersion().toLocal8Bit().constData());
            fprintf(infoFile, "Machine Host Name: %s\n", QSysInfo::machineHostName().toLocal8Bit().constData());
            fprintf(infoFile, "Pretty Product Name: %s\n", QSysInfo::prettyProductName().toLocal8Bit().constData());
            fprintf(infoFile, "Product Type: %s\n", QSysInfo::productType().toLocal8Bit().constData());
            fprintf(infoFile, "Product Version: %s\n", QSysInfo::productVersion().toLocal8Bit().constData());

            fclose(infoFile);
        }

        std::wstring wlpstr = createPath.toStdWString();
        lpcwStr             = wlpstr.c_str();
        HANDLE hDumpFile    = CreateFileW(lpcwStr, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hDumpFile != INVALID_HANDLE_VALUE) {
            // Dump信息
            MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
            dumpInfo.ExceptionPointers = pException;
            dumpInfo.ThreadId          = GetCurrentThreadId();
            dumpInfo.ClientPointers    = TRUE;  // 设置为TRUE以获取更多信息

            // 使用更详细的dump类型
            MINIDUMP_TYPE dumpType = static_cast< MINIDUMP_TYPE >(
                MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithUnloadedModules
                | MiniDumpWithProcessThreadData
            );

            // 写入Dump文件内容
            BOOL result =
                MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, dumpType, &dumpInfo, NULL, NULL);

            if (result) {
                qDebug() << "Dump file created successfully:" << createPath;
                dumpSuccess = true;
            } else {
                DWORD error = GetLastError();
                qDebug() << "Failed to create dump file, error code:" << error;
            }

            CloseHandle(hDumpFile);
        } else {
            DWORD error = GetLastError();
            qDebug() << "Failed to create dump file handle, error code:" << error;
        }

        FpPostDump fpPostDump = DADumpCapture::getPostDumpFunc();
        if (fpPostDump) {
            fpPostDump(createPath, dumpSuccess);
        }
    } catch (const std::exception& e) {
        qDebug() << "Exception in create dump:" << e.what();
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

#endif  // Q_CC_MSVC
#endif  // Q_OS_WIN
}  // end DA
#endif  // DADUMPCAPTURE_H
