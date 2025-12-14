#include "DataAnalysisPlugin.h"
#include "DataAnalysisNodeFactory.h"
#include <QDebug>
#include "DataAnalysisUI.h"
#include "DACoreInterface.h"
#include "DataframeIOWorker.h"
#include "DataframeCleanerWorker.h"
DataAnalysisPlugin::DataAnalysisPlugin() : DA::DAAbstractNodePlugin()
{
}

DataAnalysisPlugin::~DataAnalysisPlugin()
{
}

bool DataAnalysisPlugin::initialize()
{
    loadSetting();
    DA::DACoreInterface* c = core();
    // 构建ui
    m_ui = new DataAnalysisUI(this);
    m_ui->initialize(c);

    // 创建工作者
    m_ioWorker = new DataframeIOWorker(this);
    m_ioWorker->initialize(c);
    m_cleanerWorker = new DataframeCleanerWorker(this);
    m_cleanerWorker->initialize(c);
    //

    // 绑定ui和工作者的信号槽
    m_ui->bind(m_ioWorker);
    m_ui->bind(m_cleanerWorker);
    return DA::DAAbstractNodePlugin::initialize();
}

QString DataAnalysisPlugin::getIID() const
{
    return "DA.Plugin.DataAnalysis";
}

QString DataAnalysisPlugin::getName() const
{
    return u8"DA DataAnalysis Plugin";
}

QString DataAnalysisPlugin::getVersion() const
{
    return "0.1.0";
}

QString DataAnalysisPlugin::getDescription() const
{
    return u8"This is the fundamental data analysis plugin of the DA project";
}

DA::DAAbstractNodeFactory* DataAnalysisPlugin::createNodeFactory()
{
    auto fac = new DataAnalysisNodeFactory();
    fac->setCore(core());
    connect(fac, &DataAnalysisNodeFactory::destroyed, this, &DataAnalysisPlugin::onFactoryDestroyed);
    return fac;
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

void DataAnalysisPlugin::retranslate()
{
    m_ui->retranslateUi();
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
