#include "DataAnalysisPlugin.h"
#include <QDebug>

DataAnalysisPlugin::DataAnalysisPlugin() : DA::DAAbstractNodePlugin()
{
}

DataAnalysisPlugin::~DataAnalysisPlugin()
{
}

bool DataAnalysisPlugin::initialize()
{
	loadSetting();
	return DA::DAAbstractNodePlugin::initialize();
}

QString DataAnalysisPlugin::getIID() const
{
	return "Plugin.DataAnalysis";
}

QString DataAnalysisPlugin::getName() const
{
	return u8"DataAnalysis";
}

QString DataAnalysisPlugin::getVersion() const
{
	return "0.1.0";
}

QString DataAnalysisPlugin::getDescription() const
{
	return u8"DataAnalysis Plugins";
}

DA::DAAbstractNodeFactory* DataAnalysisPlugin::createNodeFactory()
{
	// auto fac = new PipeDesignerNodeFactory;
	// fac->setCore(core());
	// fac->setPipeDesignerPlugin(this);
	// mFactorys.append(fac);
	// connect(fac, &PipeDesignerNodeFactory::destroyed, this, &PipeDesignerPlugin::onFactoryDestroyed);
	// return fac;
	return nullptr;
}

void DataAnalysisPlugin::destoryNodeFactory(DA::DAAbstractNodeFactory* p)
{
	if (p) {
		p->deleteLater();
	}
}

DA::DAAbstractSettingPage* DataAnalysisPlugin::createSettingPage()
{
	//! 对于插件的设置页，如果需要设置，这里要返回设置页面
	//! 返回nullptr代表没有设置页面
	return nullptr;
}

void DataAnalysisPlugin::onFactoryDestroyed(QObject* obj)
{
	//! 每个插件会有多个工厂，每个工作流会产生一个工厂，工厂的删除会触发此槽函数，如果不需要，可以删除
	//! 槽函数的链接在createNodeFactory中进行
	qDebug() << "one factory have removred";
}

bool DataAnalysisPlugin::loadSetting()
{
	//! 这里添加加载配置文件的内容，插件的配置信息从这里加载，此函数会在initialize中调用
	return true;
}
