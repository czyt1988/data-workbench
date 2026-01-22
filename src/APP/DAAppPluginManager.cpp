#include "DAAppPluginManager.h"
#include "DAAbstractNodePlugin.h"
#include "DAAbstractPlugin.h"
#include "DAPluginManager.h"
#include "DAAbstractNodeFactory.h"

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAWorkFlowPluginManager
//===================================================
DAAppPluginManager::DAAppPluginManager(QObject* p) : DAPluginManager(p)
{
}

DAAppPluginManager::~DAAppPluginManager()
{
}

/**
 * @brief 初始化加载所有的插件
 * @param 核心接口
 */
void DAAppPluginManager::loadAllPlugins(DACoreInterface* c)
{
    // 加载插件

    if (!isLoaded()) {
        DAPluginManager::loadAllPlugins(c);
    }
    // 获取插件
    QList< DAPluginOption > plugins = getPluginOptions();
    mNodeMetaDatas.clear();
    for (int i = 0; i < plugins.size(); ++i) {
        const DAPluginOption& opt = plugins[ i ];
        if (!opt.isValid()) {
            continue;
        }
        DAAbstractPlugin* p = opt.plugin();
        if (p) {
            mPlugins.append(p);
            // 开始通过dynamic_cast判断插件的具体类型
            if (DAAbstractNodePlugin* np = dynamic_cast< DAAbstractNodePlugin* >(p)) {
                // 说明是节点插件
                // 这里工厂仅仅为了获取节点的meta数据
                // 此处未来看看是否优化，否则启动会相对较慢
                std::unique_ptr< DAAbstractNodeFactory > fac(np->createNodeFactory());
                if (nullptr == fac) {
                    // 创建工厂失败,是没有工作流的界面
                    continue;
                }
                // 此操作是为了获取所有节点metadata
                mNodeMetaDatas += fac->getNodesMetaData();
                mNodePlugins.append(np);
                qDebug() << tr("succeed load plugin %1").arg(np->getName());
            }
        }
    }
    // 最后对_nodeMetaDatas去重，此去重要保证原来的顺序
    QMap< DANodeMetaData, int > mapcnt;
    // 说明有重复项，需要去除
    for (auto i = mNodeMetaDatas.begin(); i != mNodeMetaDatas.end();) {
        if (!mapcnt.contains(*i)) {
            mapcnt.insert(*i, 1);
            ++i;
        } else {
            // 说明找到了重复项目
            i = mNodeMetaDatas.erase(i);
        }
    }
}

/**
 * @brief 获取所有的插件
 * @return
 */
QList< DAAbstractPlugin* > DAAppPluginManager::getAllPlugins() const
{
    return mPlugins;
}

/**
 * @brief 获取所有的节点插件
 * @return
 */
QList< DAAbstractNodePlugin* > DAAppPluginManager::getNodePlugins() const
{
    return (mNodePlugins);
}

/**
 * @brief 获取所有的节点工厂
 * @return
 */
QList< std::shared_ptr< DAAbstractNodeFactory > > DAAppPluginManager::createNodeFactorys() const
{
    QList< std::shared_ptr< DAAbstractNodeFactory > > res;

    for (DAAbstractNodePlugin* d : std::as_const(mNodePlugins)) {
        res.append(std::shared_ptr< DAAbstractNodeFactory >(d->createNodeFactory()));
    }
    return (res);
}

/**
 * @brief 获取所有的元数据
 * @return
 */
QList< DANodeMetaData > DAAppPluginManager::getAllNodeMetaDatas() const
{
    return mNodeMetaDatas;
}


}
