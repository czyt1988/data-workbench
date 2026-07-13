#ifndef DADIR_H
#define DADIR_H
#include "DAUtilsAPI.h"
#include <QTemporaryDir>
#include <QDebug>
namespace DA
{
/**
 * @brief 这个类定义了DA的固定路径，包括临时目录，配置文件目录等路径
 *
 * @note 此类所有路径获取函数都会确认路径是否存在，如果不存在会自动创建路径，
 * 因此，你如果在循环中用到这个类里的路径，你应该用个临时变量在循环外保存这个路径，
 * 这样就不会在循环中不停地重复地对路径是否存在进行检查
 */
class DAUTILS_API DADir
{
public:
	DADir();

	static QString getAPPName();                                    // app名，%APPDATA%路径下的文件夹以app名创建
	static QString getTempPath();                                   // 获取当前程序临时路径的根目录，程序结束时自动删除
	static QTemporaryDir& tempDir();                                // 获取当前程序临时路径的QTemporaryDir
	static QString getTempPath(const QString& folderName);          // 根据folderName在临时路径下创建文件夹，返回{RootTempPath}/{folderName}路径
	static QDir tempDir(const QString& folderName);                 // 根据folderName在临时路径下创建文件夹，返回{RootTempPath}/{folderName}的QDir
	static QString getTempFilePath(const QString& fileName);        // 获取一个临时目录下的文件
	static QString getConfigPath();                                 // 获取本应用的配置文件路径，不存在则创建
	static QString getConfigPath(const QString& folderName);        // 根据folderName在配置文件路径下创建文件夹
	static QString getExecutablePath();                             // 获取程序运行路径（exe所在文件夹）
	static std::string get_executable_path();                       // 获取程序运行路径
	static QString getAppDataPath();                                // 获取程序数据路径
	static QString getAppDataPath(const QString& folderName);       // 返回应用目录下的文件夹，不存在则创建
	// 一些常用的路径
	static QString getLogFileName();                                // 获取日志文件名字，默认"da_log.log"
	static QString getLogPath();                                    // 获取日志文件所在路径
	static QString getLogFilePath();                                // 获取日志文件的完整路径
	static QString getDumpFilePath();                               // 获取dump文件夹
};

/**
 * @brief 重载QDebug针对DADir的<<操作符，打印RootTempPath和RootConfigPath位置
 * @param debug
 * @param c
 * @return
 */
QDebug DAUTILS_API operator<<(QDebug debug, const DADir& c);

}

#endif  // DADIR_H
