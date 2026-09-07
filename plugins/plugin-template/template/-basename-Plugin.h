#ifndef {{PLUGIN-BASE-NAME}}PLUGIN_H
#define {{PLUGIN-BASE-NAME}}PLUGIN_H
#include <QtCore/qglobal.h>
#include <QObject>
#include <QAction>
#include "{{plugin-base-name}}Global.h"
#include "DAAbstractNodePlugin.h"
//
class {{plugin-base-name}}UI;
namespace DA
{
class DAAbstractNodeFactory;
}

class {{plugin-base-name}}Plugin : public QObject, public DA::DAAbstractNodePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID DAABSTRACTNODEPLUGIN_IID)
	Q_INTERFACES(DA::DAAbstractNodePlugin)
public:
	{{plugin-base-name}}Plugin();
	virtual ~{{plugin-base-name}}Plugin() override;
	// 初始化
	virtual bool initialize() override;

	// 卸载清理（插件热卸载前调用，移除本插件加入宿主的UI等资源）
	virtual bool finalize() override;

	// 插件id
	virtual QString getIID() const override;

	// 插件名
	virtual QString getName() const override;

	// 插件版本
	virtual QString getVersion() const override;

	// 插件描述
	virtual QString getDescription() const override;

	// 创建节点工厂
	virtual DA::DAAbstractNodeFactory* createNodeFactory() override;

	// 删除节点工厂
	virtual void destoryNodeFactory(DA::DAAbstractNodeFactory* p) override;
	// 获取设置页
	virtual DA::DAAbstractSettingPage* createSettingPage() override;

private Q_SLOTS:
	void onFactoryDestroyed(QObject* obj);

private:
	bool loadSetting();
private:
	{{plugin-base-name}}UI* mUi{nullptr};
};

#endif  // {{PLUGIN-BASE-NAME}}PLUGIN_H
