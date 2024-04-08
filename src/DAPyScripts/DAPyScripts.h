#ifndef DAPYSCRIPTS_H
#define DAPYSCRIPTS_H
#include <QObject>
#include <QtCore/qglobal.h>
#include "DAPyScriptsGlobal.h"
#include "DAPyScriptsIO.h"
#include "DAPyScriptsDataFrame.h"
#include "DAPyScriptsDataProcess.h"
namespace DA
{
/**
 * @brief 这个是da外部脚本的总调度，此类设计为单例，会加载软件安装目录下的PyScripts文件夹的固定脚本
 * 此类在初始化的时候就会加载对应的脚本，da的业务逻辑将使用这些脚本进行
 *
 * 调用示例：
 *
 * @code
 * DAPyScripts& scripts = DAPyScripts::getInstance();
 * QString err;
 * QString scriptPath = getPythonScriptsPath(appabsPath);
 * qInfo() << QObject::tr("python scripts path is ") << scriptPath;
 * scripts.appendSysPath(scriptPath);
 * if (!scripts.initScripts(&err)) {
 *     qCritical() << QObject::tr("scripts initialize error:") << err;
 *     return false;
 * }
 * @endcode
 *
 * @note 不建议在插件中创建DAPy相关的类，因为插件析构的顺序目前没有做保证，有可能python在插件之前析构而导致问题
 */
class DAPYSCRIPTS_API DAPyScripts
{
	DA_DECLARE_PRIVATE(DAPyScripts)
	DAPyScripts();

public:
	~DAPyScripts();
	// 初始化脚本
	static void appendSysPath(const QString& path);
	bool isInitScripts() const;
	static DAPyScripts& getInstance();
	DAPyScriptsIO& getIO();
	DAPyScriptsDataFrame& getDataFrame();
	DAPyScriptsDataProcess& getDataProcess();

protected:
	// import sys
	bool loadSysModule();
};
}  // namespace DA
#endif  // DAPYSCRIPTS_H
