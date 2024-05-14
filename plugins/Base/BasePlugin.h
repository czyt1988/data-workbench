#ifndef BASEPLUGIN_H
#define BASEPLUGIN_H
#include <QtCore/qglobal.h>
#include <QObject>
#include <QAction>
#include "BaseGlobal.h"
#include "DAAbstractNodePlugin.h"

namespace DA
{
class DAAbstractNodeFactory;
}

class BasePlugin : public QObject, public DA::DAAbstractNodePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)
	Q_INTERFACES(DA::DAAbstractNodePlugin)
public:
	BasePlugin();
	virtual ~BasePlugin() override;
	// 初始化
	virtual bool initialize() override;

	// 插件id
	virtual QString getIID() const override;

	/**
	 * @brief 插件名
	 * @return
	 */
	virtual QString getName() const override;

	/**
	 * @brief 插件版本
	 * @return
	 */
	virtual QString getVersion() const override;

	/**
	 * @brief 插件描述
	 * @return
	 */
	virtual QString getDescription() const override;

	/**
	 * @brief 创建一个节点工厂
	 * @return
	 */
	virtual DA::DAAbstractNodeFactory* createNodeFactory() override;

	/**
	 * @brief 删除一个节点工厂(谁创建谁删除原则)
	 * @param p
	 */
	virtual void destoryNodeFactory(DA::DAAbstractNodeFactory* p) override;
	/**
	 * @brief 获取设置页，默认返回nullptr，代表没有设置页
	 * @return
	 */
	virtual DA::DAAbstractSettingPage* createSettingPage() override;

private slots:
	void onFactoryDestroyed(QObject* obj);

private:
	bool loadSetting();
};

#endif  // BASEPLUGIN_H
