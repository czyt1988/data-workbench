#include "DataAnalysisPlugin.h"
#include <QDebug>
#include "DataAnalysisUI.h"
#include "DACoreInterface.h"
#include "DataframeIOWorker.h"
#include "DataframeCleanerWorker.h"
#include "DataframeOperateWorker.h"
/**
 * @brief 构造函数
 */
DataAnalysisPlugin::DataAnalysisPlugin() : DA::DAAbstractNodePlugin()
{
}

/**
 * @brief 析构函数
 */
DataAnalysisPlugin::~DataAnalysisPlugin()
{
}

/**
 * @brief 初始化插件
 * @return 初始化是否成功
 */
bool DataAnalysisPlugin::initialize()
{
    loadSetting();
    DA::DACoreInterface* c = core();
    // 构建ui
    mUi = new DataAnalysisUI(this);
    mUi->initialize(c);

    // 创建工作者
    mIoWorker = new DataframeIOWorker(this);
    mIoWorker->initialize(c);
    mCleanerWorker = new DataframeCleanerWorker(this);
    mCleanerWorker->initialize(c);
    mOperateWorker = new DataframeOperateWorker(this);
    mOperateWorker->initialize(c);
    //

    // 绑定ui和工作者的信号槽
    mUi->bind(mIoWorker);
    mUi->bind(mCleanerWorker);
    mUi->bind(mOperateWorker);
    return DA::DAAbstractNodePlugin::initialize();
}

/**
 * @brief 获取插件IID
 * @return 插件IID字符串
 */
QString DataAnalysisPlugin::getIID() const
{
    return "DA.Plugin.DataAnalysis";
}

/**
 * @brief 获取插件名称
 * @return 插件名称
 */
QString DataAnalysisPlugin::getName() const
{
    return u8"DA DataAnalysis Plugin";
}

/**
 * @brief 获取插件版本
 * @return 版本号字符串
 */
QString DataAnalysisPlugin::getVersion() const
{
    return "0.1.0";
}

/**
 * @brief 获取插件描述
 * @return 描述信息
 */
QString DataAnalysisPlugin::getDescription() const
{
    return u8"This is the fundamental data analysis plugin of the DA project";
}

/**
 * @brief 创建节点工厂
 * @return 节点工厂指针
 */
DA::DAPyNodeFactory* DataAnalysisPlugin::createNodeFactory()
{
    return nullptr;
}

/**
 * @brief 销毁节点工厂
 * @param p 节点工厂指针
 */
void DataAnalysisPlugin::destroyNodeFactory(DA::DAPyNodeFactory* p)
{
}

/**
 * @brief 创建设置页面
 * @return 设置页面指针
 */
DA::DAAbstractSettingPage* DataAnalysisPlugin::createSettingPage()
{
    //! 对于插件的设置页，如果需要设置，这里要返回设置页面
    //! 返回nullptr代表没有设置页面
    return nullptr;
}

/**
 * @brief 翻译界面文本
 */
void DataAnalysisPlugin::retranslate()
{
    mUi->retranslateUi();
}

/**
 * @brief 工厂销毁时的槽函数
 * @param obj 被销毁的对象
 */
void DataAnalysisPlugin::onFactoryDestroyed(QObject* obj)
{
    //! 每个插件会有多个工厂，每个工作流会产生一个工厂，工厂的删除会触发此槽函数，如果不需要，可以删除
    //! 槽函数的链接在createNodeFactory中进行
    qDebug() << "one factory have removred";
}

/**
 * @brief 加载配置
 * @return 是否加载成功
 */
bool DataAnalysisPlugin::loadSetting()
{
    //! 这里添加加载配置文件的内容，插件的配置信息从这里加载，此函数会在initialize中调用
    return true;
}
