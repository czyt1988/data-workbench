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

	/**
	 * @brief app名，%APPDATA%路径下的文件夹以app名创建
	 * @return
	 */
	static QString getAPPName();

	/**
	 * @brief 获取当前程序临时路径的根目录
	 * @return 此函数返回的路径是临时路径的绝对路径
	 * @note 注意此路径在程序结束时会自动删除
	 */
	static QString getTempPath();

	/**
	 * @brief 获取当前程序临时路径的QTemporaryDir
	 * @return
	 */
	static QTemporaryDir& tempDir();

	/**
	 * @brief 根据folderName，在临时路径下创建一个文件夹，返回{RootTempPath}/{folderName}路径
	 * @param 临时路径下文件夹的名字
	 * @return 如果路径已经存在将直接返回路径，如果不存在则创建
	 */
	static QString getTempPath(const QString& folderName);

	/**
	 * @brief 根据folderName，在临时路径下创建一个文件夹，返回{RootTempPath}/{folderName}的QDir
	 * @param folderName 临时路径下文件夹的名字
	 * @return 如果路径已经存在将直接返回路径，如果不存在则创建
	 */
	static QDir tempDir(const QString& folderName);

	/**
	 * @brief 获取一个临时目录下的文件
	 *
	 * 此函数等效@a DADir::getRootTempDir.filePath(fileName)
	 * @param fileName
	 * @return
	 */
	static QString getTempFilePath(const QString& fileName);

	/**
	 * @brief 获取本应用的配置文件路径
	 *
	 * 如果配置文件路径不存在，将创建此路径，并返回
	 * 配置文件路径等同于：
	 * @code
	 * getAppDataPath(QStringLiteral("config"));
	 * @endcode
	 *
	 * @return
	 */
	static QString getConfigPath();

	/**
	 * @brief 根据folderName，在配置文件路径下创建一个文件夹，返回{getConfigPath}/{folderName}路径
	 * @param 配置文件路径下文件夹的名字
	 * @return 如果路径已经存在将直接返回路径，如果不存在则创建
	 */
	static QString getConfigPath(const QString& folderName);

	/**
	 * @brief 获取程序运行路径
	 * @note 这个路径不会应该工作路径的改变而改变，就是exe所在文件夹
	 * @return
	 */
	static QString getExecutablePath();

	/**
	 * @brief 获取程序运行路径
	 * @return 这个路径不会应该工作路径的改变而改变，就是exe所在文件夹
	 */
	static std::string get_executable_path();

	/**
	 * @brief 获取程序数据路径
	 *
	 * 此函数等价于：
	 * @code
	 * return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	 * @endcode
	 * @return
	 */
	static QString getAppDataPath();

	/**
	 * @brief 返回应用目录下的文件夹，如果文件没有，此函数会保证创建好文件夹
	 * @param folderName
	 * @return
	 */
	static QString getAppDataPath(const QString& folderName);

	/// @group 一些常用的路径
	/// @{

	/**
	 * @brief 获取日志文件名字
	 * @return 默认返回"da_log.log"
	 */
	static QString getLogFileName();

	/**
	 * @brief 获取日志文件所在路径
	 * @return 默认返回"{ExecutablePath}/log"
	 */
	static QString getLogPath();

	/**
	 * @brief 获取日志文件的完整路径
	 * @return 默认返回"{getLogPath()}/log/da_log.log"
	 */
	static QString getLogFilePath();

	/**
	 * @brief 获取dump文件夹
	 * @return
	 */
	static QString getDumpFilePath();

	/// @}
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
