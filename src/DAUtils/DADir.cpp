#include "DADir.h"
#include <QStandardPaths>
#include <QDir>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <filesystem>
#else
#include <unistd.h>
#include <limits.h>
#include <filesystem>
#endif

namespace DA
{
QTemporaryDir DADir::s_temp_dir;

DADir::DADir()
{
}

QString DADir::getRootTempPath()
{
    return s_temp_dir.path();
}

QTemporaryDir& DADir::tempDir()
{
    return s_temp_dir;
}

QString DADir::createTempPath(const QString& folderName)
{
	// 获取临时目录的根路径
	QString rootTempPath = getRootTempPath();
	// 拼接目标文件夹路径
	QString targetPath = QDir::toNativeSeparators(rootTempPath + QDir::separator() + folderName);

	// 创建一个 QDir 对象
	QDir targetDir(targetPath);
	// 如果目标文件夹不存在，则创建它
	if (!targetDir.exists()) {
		targetDir.mkpath(".");
	}

	// 返回目标文件夹路径
    return targetPath;
}

QDir DADir::createTempDir(const QString& folderName)
{
    return QDir(createTempPath(folderName));
}

QString DADir::getRootTempFile(const QString& fileName)
{
    return s_temp_dir.filePath(fileName);
}

QString DADir::getRootConfigPath()
{
	// 获取配置文件路径
	QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

	// 如果路径不存在，则创建它
	QDir configDir(configPath);
	if (!configDir.exists()) {
		configDir.mkpath(".");
	}

	return configPath;
}

QString DADir::createConfigPath(const QString& folderName)
{
	// 获取配置文件路径的根路径
	QString rootConfigPath = getRootConfigPath();
	// 拼接目标文件夹路径
	QString targetPath = QDir::toNativeSeparators(rootConfigPath + QDir::separator() + folderName);

	// 创建一个 QDir 对象
	QDir targetDir(targetPath);

	// 如果目标文件夹不存在，则创建它
	if (!targetDir.exists()) {
		targetDir.mkpath(".");
	}

	// 返回目标文件夹路径
	return targetPath;
}

QString DADir::getExecutablePath()
{
	std::string executablePath = get_executable_path();
	// 这时文本是系统编码的，要转换为utf-8
	return QString::fromLocal8Bit(executablePath.c_str());
}

std::string DADir::get_executable_path()
{
#if defined(_WIN32) || defined(_WIN64)
	char buffer[ MAX_PATH ];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::string fullPath(buffer);
	std::filesystem::path path(fullPath);
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
	return QString("da_log.log");
}

QString DADir::getLogPath()
{
	return QDir::toNativeSeparators(getExecutablePath() + QDir::separator() + "log");
}

QString DADir::getFullLogFilePath()
{
	return QDir::toNativeSeparators(getLogPath() + QDir::separator() + getLogFileName());
}

QString DADir::getAppDataPath()
{
	// 获取应用数据文件夹
	QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	// 如果路径不存在，则创建它
	QDir appDataDir(appDataPath);
	if (!appDataDir.exists()) {
		appDataDir.mkpath(".");
	}
	return appDataPath;
}

QDebug operator<<(QDebug debug, const DADir& c)
{
	QDebugStateSaver saver(debug);
	debug.noquote() << "Executable Dir:" << DADir::getExecutablePath();
	debug.noquote() << "\nLog File Path:" << DADir::getFullLogFilePath();
	debug.noquote() << "\nTemporary Dir:" << DADir::getRootTempPath();
	debug.noquote() << "\nConfig Dir:" << DADir::getRootConfigPath();
	debug.noquote() << "\nApp Data Dir:" << DADir::getAppDataPath();
	return debug;
}

}  // end DA
