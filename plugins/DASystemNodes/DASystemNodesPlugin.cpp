#include "DASystemNodesPlugin.h"
#include <QDebug>

/**
 * @brief 构造函数
 */
DASystemNodesPlugin::DASystemNodesPlugin() : DA::DAAbstractNodePlugin()
{
}

/**
 * @brief 析构函数
 */
DASystemNodesPlugin::~DASystemNodesPlugin()
{
}

/**
 * @brief 初始化插件
 * @return 初始化成功返回 true，否则返回 false
 */
bool DASystemNodesPlugin::initialize()
{
    return DA::DAAbstractNodePlugin::initialize();
}

/**
 * @brief 获取插件的IID
 * @return 返回插件的唯一标识符
 */
QString DASystemNodesPlugin::getIID() const
{
    return "DA.Plugin.DASystemNodes";
}

/**
 * @brief 获取插件名称
 * @return 返回插件名称
 */
QString DASystemNodesPlugin::getName() const
{
    return u8"DA System Nodes Plugin";
}

/**
 * @brief 获取插件版本号
 * @return 返回插件版本字符串
 */
QString DASystemNodesPlugin::getVersion() const
{
    return "1.0.0";
}

/**
 * @brief 获取插件描述信息
 * @return 返回插件的描述文本
 */
QString DASystemNodesPlugin::getDescription() const
{
    return u8"Provides system-level workflow nodes for UI interaction, flow control and data display";
}

/**
 * @brief 创建节点工厂
 * @return 节点工厂指针，本插件节点由 Python 包自动注册，返回 nullptr
 */
DA::DAPyNodeFactory* DASystemNodesPlugin::createNodeFactory()
{
    // 节点由 Python 包 DASystemNodes 通过 pyplugins 目录扫描自动注册
    return nullptr;
}

/**
 * @brief 销毁节点工厂
 * @param p 待销毁的节点工厂指针
 */
void DASystemNodesPlugin::destroyNodeFactory(DA::DAPyNodeFactory* p)
{
    Q_UNUSED(p)
}

/**
 * @brief 创建设置页面
 * @return 设置页面指针，本插件无设置页面，返回 nullptr
 */
DA::DAAbstractSettingPage* DASystemNodesPlugin::createSettingPage()
{
    return nullptr;
}

/**
 * @brief 重新翻译UI字符串
 */
void DASystemNodesPlugin::retranslate()
{
}
