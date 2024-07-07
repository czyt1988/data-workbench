#ifndef DAPLUGINOPTION_H
#define DAPLUGINOPTION_H
#include "DAPluginSupportGlobal.h"
#include "DAAbstractPlugin.h"
#include <QDebug>
namespace DA
{

/**
 * @brief 用于插件的加载操作
 */
class DAPLUGINSUPPORT_API DAPluginOption
{
	DA_DECLARE_PRIVATE(DAPluginOption)
public:
	DAPluginOption();
	DAPluginOption(const DAPluginOption& other);
	DAPluginOption(DAPluginOption&& other);
	DAPluginOption& operator=(const DAPluginOption& other);

	~DAPluginOption();

public:
	// 判断是否是有效的
	bool isValid() const;

	// 加载插件
	bool load(const QString& pluginPath, DACoreInterface* c);
	bool unload();
	// 错误信息
	QString getErrorString() const;

	// 文件名
	QString getFileName() const;

	// 获取iid
	QString getIid() const;

	// 获取版本信息
	DAAbstractPlugin* plugin() const;

	// 获取插件名
	QString getPluginName() const;

	// 获取插件描述
	QString getPluginDescription() const;

	// 获取插件版本
	QString getPluginVersion() const;
};
// 格式化输出
DAPLUGINSUPPORT_API QDebug operator<<(QDebug debug, const DAPluginOption& po);
}  // namespace DA

#endif  // DAPLUGINOPTION_H
