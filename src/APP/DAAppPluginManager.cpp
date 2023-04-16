#include "DAAppPluginManager.h"
#include "DAAbstractNodePlugin.h"
#include "DAAbstractPlugin.h"
#include "DAPluginManager.h"
#include "DAAbstractNodeFactory.h"

namespace DA
{
class _DAPrivateWorkflowNodePluginData
{
public:
    _DAPrivateWorkflowNodePluginData();
    ~_DAPrivateWorkflowNodePluginData();
    DAAbstractNodePlugin* plugin   = { nullptr };
    DAAbstractNodeFactory* factory = { nullptr };
};
}  // namespace DA
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// _DAPrivateNodePluginData
//===================================================

_DAPrivateWorkflowNodePluginData::_DAPrivateWorkflowNodePluginData()
{
}

_DAPrivateWorkflowNodePluginData::~_DAPrivateWorkflowNodePluginData()
{
    if (plugin && factory) {
        plugin->destoryNodeFactory(factory);
    }
}
//===================================================
// DAWorkFlowPluginManager
//===================================================
DAAppPluginManager::DAAppPluginManager(QObject* p) : QObject(p)
{
}

DAAppPluginManager::~DAAppPluginManager()
{
    for (_DAPrivateWorkflowNodePluginData* d : qAsConst(mNnodePlugins)) {
        delete d;
    }
}

DAAppPluginManager& DAAppPluginManager::instance()
{
    static DAAppPluginManager s_mgr;

    return (s_mgr);
}

/**
 * @brief 初始化加载所有的插件
 * @param 核心接口
 */
void DAAppPluginManager::initLoadPlugins(DACoreInterface* c)
{
    //加载插件
    DAPluginManager& plugin = DAPluginManager::instance();

    if (!plugin.isLoaded()) {
        plugin.load(c);
    }
    //获取插件
    QList< DAPluginOption > plugins = plugin.getPluginOptions();
    mNodeMetaDatas.clear();
    for (int i = 0; i < plugins.size(); ++i) {
        const DAPluginOption& opt = plugins[ i ];
        if (!opt.isValid()) {
            continue;
        }
        DAAbstractPlugin* p = opt.plugin();
        if (p) {
            mPlugins.append(p);
            //开始通过dynamic_cast判断插件的具体类型
            if (DAAbstractNodePlugin* np = dynamic_cast< DAAbstractNodePlugin* >(p)) {
                //说明是节点插件
                _DAPrivateWorkflowNodePluginData* data = new _DAPrivateWorkflowNodePluginData();
                data->plugin                           = np;
                data->factory                          = np->createNodeFactory();
                if (nullptr == data->factory) {
                    //创建工厂失败
                    qCritical() << tr("%1 plugin create a null node factory").arg(opt.getFileName());
                    continue;
                }
                mNodeMetaDatas += data->factory->getNodesMetaData();
                mNnodePlugins.append(data);
                qDebug() << tr("succeed load plugin %1").arg(np->getName());
            }
        }
    }
    //最后对_nodeMetaDatas去重，此去重要保证原来的顺序
    QMap< DANodeMetaData, int > mapcnt;
    //说明有重复项，需要去除
    for (auto i = mNodeMetaDatas.begin(); i != mNodeMetaDatas.end();) {
        if (!mapcnt.contains(*i)) {
            mapcnt.insert(*i, 1);
            ++i;
        } else {
            //说明找到了重复项目
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
    QList< DAAbstractNodePlugin* > res;

    for (_DAPrivateWorkflowNodePluginData* d : qAsConst(mNnodePlugins)) {
        res.append(d->plugin);
    }
    return (res);
}

/**
 * @brief 获取所有的节点工厂
 * @return
 */
QList< DA::DAAbstractNodeFactory* > DAAppPluginManager::getNodeFactorys() const
{
    QList< DA::DAAbstractNodeFactory* > res;

    for (_DAPrivateWorkflowNodePluginData* d : qAsConst(mNnodePlugins)) {
        res.append(d->factory);
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
