#include "{{plugin-base-name}}Plugin.h"
#include "{{plugin-base-name}}NodeFactory.h"
#include <QDebug>
#include "{{plugin-base-name}}UI.h"

/**
 * @brief 构造函数
 */
{{plugin-base-name}}Plugin::{{plugin-base-name}}Plugin() : DA::DAAbstractNodePlugin()
{
}

/**
 * @brief 析构函数
 */
{{plugin-base-name}}Plugin::~{{plugin-base-name}}Plugin()
{
}

/**
 * @brief 初始化插件
 * @return 初始化成功返回true，否则返回false
 */
bool {{plugin-base-name}}Plugin::initialize()
{
	loadSetting();
	//构建ui
	mUi = new {{plugin-base-name}}UI(this);
	mUi->initialize(core());
	return DA::DAAbstractNodePlugin::initialize();
}

/**
 * @brief 卸载清理
 *
 * 插件热卸载前由插件管理器调用，必须清理initialize中向宿主注册/创建的全部资源：
 * 1. 移除加入宿主ribbon的panel与action（action的parent是宿主DAActionsInterface，
 *    不会随插件库卸载销毁，必须显式经DAActionsInterface::removeAction移除）
 * 2. agent工具与系统提示词由宿主在卸载前按provider自动注销，工具需以插件对象为parent
 * 3. 返回false可拒绝卸载（插件保持加载状态）
 * @return 允许卸载返回true
 */
bool {{plugin-base-name}}Plugin::finalize()
{
	if (mUi) {
		mUi->finalize();
		mUi->deleteLater();
		mUi = nullptr;
	}
	return true;
}

/**
 * @brief 获取插件的IID
 * @return 返回插件的IID字符串
 */
QString {{plugin-base-name}}Plugin::getIID() const
{
	return "{{plugin-iid}}";
}

/**
 * @brief 获取插件名称
 * @return 返回插件名称字符串
 */
QString {{plugin-base-name}}Plugin::getName() const
{
	return u8"{{plugin-display-name}}";
}

/**
 * @brief 获取插件版本号
 * @return 返回插件版本号字符串
 */
QString {{plugin-base-name}}Plugin::getVersion() const
{
	return "0.1.0";
}

/**
 * @brief 获取插件描述信息
 * @return 返回插件描述字符串
 */
QString {{plugin-base-name}}Plugin::getDescription() const
{
	return u8"{{plugin-description}}";
}

/**
 * @brief 创建节点工厂
 * @return 返回创建的节点工厂指针
 */
DA::DAAbstractNodeFactory* {{plugin-base-name}}Plugin::createNodeFactory()
{
	auto fac = new {{plugin-base-name}}NodeFactory();
	fac->setCore(core());
	connect(fac, &{{plugin-base-name}}NodeFactory::destroyed, this, &{{plugin-base-name}}Plugin::onFactoryDestroyed);
	return fac;
}

/**
 * @brief 销毁节点工厂
 * @param p 待销毁的节点工厂指针
 */
void {{plugin-base-name}}Plugin::destoryNodeFactory(DA::DAAbstractNodeFactory* p)
{
	if (p) {
		p->deleteLater();
	}
}

/**
 * @brief 创建设置页面
 * @return 返回设置页面指针，若无设置页面则返回nullptr
 */
DA::DAAbstractSettingPage* {{plugin-base-name}}Plugin::createSettingPage()
{
	//! 对于插件的设置页，如果需要设置，这里要返回设置页面
	//! 返回nullptr代表没有设置页面
	return nullptr;
}

/**
 * @brief 工厂被销毁时触发的槽函数
 * @param obj 被销毁的工厂对象指针
 */
void {{plugin-base-name}}Plugin::onFactoryDestroyed(QObject* obj)
{
	//! 每个插件会有多个工厂，每个工作流会产生一个工厂，工厂的删除会触发此槽函数，如果不需要，可以删除
	//! 槽函数的链接在createNodeFactory中进行
	qDebug() << "one factory have removred";
}

/**
 * @brief 加载插件配置
 * @return 加载成功返回true，否则返回false
 */
bool {{plugin-base-name}}Plugin::loadSetting()
{
	//! 这里添加加载配置文件的内容，插件的配置信息从这里加载，此函数会在initialize中调用
	return true;
}
