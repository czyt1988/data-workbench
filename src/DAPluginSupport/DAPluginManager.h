#ifndef FCPLUGINMANAGER_H
#define FCPLUGINMANAGER_H
#include <QObject>
#include "DAPluginSupportGlobal.h"
#include "DAAbstractPlugin.h"
#include "DAPluginOption.h"
#include <QDebug>
namespace DA
{

/**
 * @brief 此类为插件管理类，作为单例，管理整个程序的插件加载和释放
 */
class DAPLUGINSUPPORT_API DAPluginManager : public QObject
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAPluginManager)
	DAPluginManager(QObject* p = nullptr);

public:
	~DAPluginManager();
	// 调用实例函数
	static DAPluginManager& instance();

	// 设置忽略列表
	void setIgnoreList(const QStringList ignorePluginsName);

	// 加载所有插件
	void load(DACoreInterface* c);

	// 是否已经加载
	bool isLoaded() const;

	// 设置插件路径
	void setPluginPath(const QString& path);

	// 插件数
	int getPluginCount() const;

	// 获取加载的插件名
	QList< QString > getPluginNames() const;

	// 获取所有插件信息
	QList< DAPluginOption > getPluginOptions() const;
signals:
	/**
	 * @brief 开始加载插件信号
	 *
	 * 此信号可以给到启动画面窗口使用
	 * @param pluginPath
	 */
	void beginLoadPlugin(const QString& pluginPath);
};

// 格式化输出
DAPLUGINSUPPORT_API QDebug operator<<(QDebug debug, const DAPluginManager& fmg);
}  // namespace DA

#endif  // FCPLUGINMANAGER_H
