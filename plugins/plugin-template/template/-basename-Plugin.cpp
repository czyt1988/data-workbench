#include "{{plugin-base-name}}Plugin.h"
#include <QDebug>

{{plugin-base-name}}Plugin::{{plugin-base-name}}Plugin() : DA::DAAbstractNodePlugin()
{
}

{{plugin-base-name}}Plugin::~{{plugin-base-name}}Plugin()
{
}

bool {{plugin-base-name}}Plugin::initialize()
{
	loadSetting();
	return DA::DAAbstractNodePlugin::initialize();
}

QString {{plugin-base-name}}Plugin::getIID() const
{
	return "{{plugin-iid}}";
}

QString {{plugin-base-name}}Plugin::getName() const
{
	return u8"{{plugin-display-name}}";
}

QString {{plugin-base-name}}Plugin::getVersion() const
{
	return "0.1.0";
}

QString {{plugin-base-name}}Plugin::getDescription() const
{
	return u8"{{plugin-description}}";
}

DA::DAAbstractNodeFactory* {{plugin-base-name}}Plugin::createNodeFactory()
{
	auto fac = new {{plugin-base-name}}NodeFactory;
	fac->setCore(core());
	connect(fac, &{{plugin-base-name}}NodeFactory::destroyed, this, &{{plugin-base-name}}Plugin::onFactoryDestroyed);
	return fac;
}

void {{plugin-base-name}}Plugin::destoryNodeFactory(DA::DAAbstractNodeFactory* p)
{
	if (p) {
		p->deleteLater();
	}
}

DA::DAAbstractSettingPage* {{plugin-base-name}}Plugin::createSettingPage()
{
	//! 对于插件的设置页，如果需要设置，这里要返回设置页面
	//! 返回nullptr代表没有设置页面
	return nullptr;
}

void {{plugin-base-name}}Plugin::onFactoryDestroyed(QObject* obj)
{
	//! 每个插件会有多个工厂，每个工作流会产生一个工厂，工厂的删除会触发此槽函数，如果不需要，可以删除
	//! 槽函数的链接在createNodeFactory中进行
	qDebug() << "one factory have removred";
}

bool {{plugin-base-name}}Plugin::loadSetting()
{
	//! 这里添加加载配置文件的内容，插件的配置信息从这里加载，此函数会在initialize中调用
	return true;
}
