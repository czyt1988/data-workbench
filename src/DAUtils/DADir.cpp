#include "DADir.h"
#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

// qt5.14.2的默认带的mingw是mingw730_64，这里的写法是为了兼容mingw730_64没有#include <filesystem>，
#if __has_include(<filesystem>)
#include <filesystem>
namespace std_fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace std_fs = std::experimental::filesystem;
#else
#error "No filesystem support"
#endif

#else
#include <unistd.h>
#include <limits.h>
#include <filesystem>
#endif

namespace DA
{

DADir::DADir()
{
}

QString DADir::getAPPName()
{
	const static QString cs_app_name = QStringLiteral("DAWorkBench");
	return cs_app_name;
}

QString DADir::getTempPath()
{
	return tempDir().path();
}

QTemporaryDir& DADir::tempDir()
{
	static QTemporaryDir s_temp_dir;  ///< 本应用的临时目录，程序销毁时会自动删除
	// 确保临时目录有效
	if (!s_temp_dir.isValid()) {
		qCritical() << "Failed to create temporary directory:" << s_temp_dir.errorString();
	}
	return s_temp_dir;
}

QString DADir::getTempPath(const QString& folderName)
{
	if (folderName.isEmpty()) {
		return getTempPath();  // 处理空文件夹名的情况
	}
	// 构建完整路径
	const static QString cs_fullPath = QDir::cleanPath(getTempPath() + QDir::separator() + folderName);
	// 创建一个 QDir 对象
	if (QDir().mkpath(cs_fullPath)) {
		qWarning() << "Failed to create directory:" << cs_fullPath;
	}
	// 返回目标文件夹路径
	return cs_fullPath;
}

QDir DADir::tempDir(const QString& folderName)
{
	return QDir(getTempPath(folderName));
}

QString DADir::getTempFilePath(const QString& fileName)
{
	return tempDir().filePath(fileName);
}

QString DADir::getConfigPath()
{
	// 获取配置文件路径
	const static QString cs_configPath = getAppDataPath(QStringLiteral("config"));

	// 如果路径不存在，则创建它
	if (!QDir().mkpath(cs_configPath)) {
		qCritical() << "Failed to create application config directory:" << cs_configPath;
	}
	return cs_configPath;
}

QString DADir::getConfigPath(const QString& folderName)
{
	if (folderName.isEmpty()) {
		return getConfigPath();  // 处理空文件夹名的情况
	}
	// 构建完整路径
	QString fullPath = QDir::cleanPath(getConfigPath() + QDir::separator() + folderName);
	// 创建目录（如果不存在）
	if (!QDir().mkpath(fullPath)) {
		qWarning() << "Failed to create directory:" << fullPath;
	}

	return fullPath;
}

QString DADir::getExecutablePath()
{
#if 0
    return QApplication::applicationDirPath();
#else
    const static std::string cs_executablePath = get_executable_path();
    // 这时文本是系统编码的，要转换为utf-8
    return QString::fromLocal8Bit(cs_executablePath.c_str());
#endif
}

std::string DADir::get_executable_path()
{
#if defined(_WIN32) || defined(_WIN64)
	char buffer[ MAX_PATH ];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);  // 显式 A 版
	std::string fullPath(buffer);
    std_fs::path path(fullPath);
	return path.parent_path().string();
#else
	char buffer[ PATH_MAX ];
	ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
	if (count == -1) {
		return "";  // Error occurred
	}
	std::string fullPath(buffer, count);
	std::filesystem::path path(fullPath);
	return path.parent_path().string();
#endif
}

QString DADir::getLogFileName()
{
	const static QString cs_da_log_file = QStringLiteral("da_log.log");
	return cs_da_log_file;
}

QString DADir::getLogPath()
{
	const static QString cs_logPath = getAppDataPath(QStringLiteral("log"));
	return cs_logPath;
}

QString DADir::getDumpFilePath()
{
	const static QString cs_dumpPath = getAppDataPath(QStringLiteral("dumps"));
	return cs_dumpPath;
}

QString DADir::getLogFilePath()
{
	const static QString cs_logFilePath = QDir::toNativeSeparators(getLogPath() + QDir::separator() + getLogFileName());
	return cs_logFilePath;
}

QString DADir::getAppDataPath()
{
	// 获取应用数据文件夹
	const static QString cs_appDataPath = QDir::toNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + getAPPName());
	// 使用单次操作创建路径（如果不存在）
	if (!QDir().mkpath(cs_appDataPath)) {
		qCritical() << "Failed to create application data directory:" << cs_appDataPath;
	}
	return cs_appDataPath;
}

QString DADir::getAppDataPath(const QString& folderName)
{
	if (folderName.isEmpty()) {
		return getAppDataPath();  // 处理空文件夹名的情况
	}
	// 构建完整路径
	QString fullPath = QDir::cleanPath(getAppDataPath() + QDir::separator() + folderName);
	// 创建目录（如果不存在）
	if (!QDir().mkpath(fullPath)) {
		qWarning() << "Failed to create directory:" << fullPath;
	}

	return fullPath;
}

QDebug operator<<(QDebug debug, const DADir& c)
{
	QDebugStateSaver saver(debug);
	debug.noquote() << "Executable Dir:" << DADir::getExecutablePath();
	debug.noquote() << "\nApp Data Dir:" << DADir::getAppDataPath();
	debug.noquote() << "\nTemporary Dir:" << DADir::getTempPath();
	debug.noquote() << "\nLog File Path:" << DADir::getLogFilePath();
	debug.noquote() << "\nConfig Dir:" << DADir::getConfigPath();
	debug.noquote() << "\nApp Data Dir:" << DADir::getDumpFilePath();
	return debug;
}

}  // end DA
