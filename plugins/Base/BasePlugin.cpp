#include "BasePlugin.h"
#include "BaseNodeFactory.h"
#include <QDebug>

BasePlugin::BasePlugin() : DA::DAAbstractNodePlugin()
{
}

BasePlugin::~BasePlugin()
{
}

bool BasePlugin::initialize()
{
	loadSetting();
	return DA::DAAbstractNodePlugin::initialize();
}

QString BasePlugin::getIID() const
{
	return "DA.Plugin.Base";
}

QString BasePlugin::getName() const
{
	return u8"DA Base Plugin";
}

QString BasePlugin::getVersion() const
{
	return "0.1.0";
}

QString BasePlugin::getDescription() const
{
	return u8"This is DA Base Plugin";
}

DA::DAAbstractNodeFactory* BasePlugin::createNodeFactory()
{
	auto fac = new BaseNodeFactory();
	fac->setCore(core());
	connect(fac, &BaseNodeFactory::destroyed, this, &BasePlugin::onFactoryDestroyed);
	return fac;
}

void BasePlugin::destoryNodeFactory(DA::DAAbstractNodeFactory* p)
{
	if (p) {
		p->deleteLater();
	}
}

DA::DAAbstractSettingPage* BasePlugin::createSettingPage()
{
	//! 对于插件的设置页，如果需要设置，这里要返回设置页面
	//! 返回nullptr代表没有设置页面
	return nullptr;
}

void BasePlugin::onFactoryDestroyed(QObject* obj)
{
	//! 每个插件会有多个工厂，每个工作流会产生一个工厂，工厂的删除会触发此槽函数，如果不需要，可以删除
	//! 槽函数的链接在createNodeFactory中进行
	qDebug() << "one factory have removred";
}

bool BasePlugin::loadSetting()
{
	//! 这里添加加载配置文件的内容，插件的配置信息从这里加载，此函数会在initialize中调用
	return true;
}
