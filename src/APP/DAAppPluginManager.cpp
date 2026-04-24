#include "DAAppPluginManager.h"
#include "DAAbstractNodePlugin.h"
#include "DAAbstractPlugin.h"
#include "DAPluginManager.h"
#include "DAPyNodeFactory.h"
#include <QApplication>
#if DA_ENABLE_PYTHON
#include "DAPyNodeFactory.h"
#include "DAPyInterpreter.h"
#endif

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
 *
 * 加载C++插件节点后，如果Python环境已启用，还会初始化Python节点工厂，
 * 将Python节点元数据合并到节点列表中，实现Python和C++节点共存。
 *
 * @param[in] c 核心接口
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
                std::unique_ptr< DAPyNodeFactory > fac(np->createNodeFactory());
                if (nullptr == fac) {
                    // 创建工厂失败,是没有工作流的界面
                    continue;
                }
                // 此操作是为了获取所有节点metadata
                mNodeMetaDatas += fac->getNodeMetadataList();
                qDebug() << tr("succeed load plugin %1").arg(np->getName());
            }
        }
    }
#if DA_ENABLE_PYTHON
    // 初始化Python节点工厂
    initPyNodeFactory();
#endif
    // 最后对_nodeMetaDatas去重，此去重要保证原来的顺序
    QMap< DAPyNodeMetaData, int > mapcnt;
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
    QList< DAAbstractNodePlugin* > res;
    for (DAAbstractPlugin* p : std::as_const(mPlugins)) {
        if (DAAbstractNodePlugin* np = dynamic_cast< DAAbstractNodePlugin* >(p)) {
            res.append(np);
        }
    }
    return res;
}

/**
 * @brief 获取所有的节点工厂
 *
 * 返回C++插件节点工厂和Python节点工厂的合并列表
 *
 * @return 所有节点工厂的共享指针列表
 */
QList< std::shared_ptr< DAPyNodeFactory > > DAAppPluginManager::createNodeFactorys() const
{
    QList< std::shared_ptr< DAPyNodeFactory > > res;
    const QList< DAAbstractNodePlugin* > nodePlugins = getNodePlugins();
    for (DAAbstractNodePlugin* d : std::as_const(nodePlugins)) {
        res.append(std::shared_ptr< DAPyNodeFactory >(d->createNodeFactory()));
    }
#if DA_ENABLE_PYTHON
    if (mPyNodeFactory) {
        res.append(mPyNodeFactory);
    }
#endif
    return (res);
}

/**
 * @brief 获取所有的元数据
 * @return
 */
QList< DAPyNodeMetaData > DAAppPluginManager::getAllNodeMetaDatas() const
{
    return mNodeMetaDatas;
}

#if DA_ENABLE_PYTHON
/**
 * @brief 初始化Python节点工厂
 *
 * 创建DAPyNodeFactory实例，调用discoverNodes()发现Python节点，
 * 并将Python节点元数据合并到mNodeMetaDatas中。
 *
 * 发现流程：
 * 1. 检查Python解释器是否已初始化
 * 2. 将PyScripts目录添加到Python sys.path
 * 3. 创建DAPyNodeFactory并调用discoverNodes()
 * 4. 合并Python节点元数据到mNodeMetaDatas
 *
 * @note 如果Python未初始化或发现失败，不会影响C++节点的正常加载
 */
void DAAppPluginManager::initPyNodeFactory()
{
    if (!DAPyInterpreter::isPythonInitialized()) {
        qWarning() << tr("Python interpreter not initialized, skip Python node discovery");
        return;
    }
    try {
        // 将PyScripts目录添加到sys.path，确保DAWorkFlowPy可被导入
        DAPyInterpreter::appendSysPath(QApplication::applicationDirPath() + "/PyScripts");

        mPyNodeFactory = std::make_shared< DAPyNodeFactory >();
        if (!mPyNodeFactory->discoverNodes()) {
            qWarning() << tr("Python node discovery failed");
            mPyNodeFactory.reset();
            return;
        }
        // 合并Python节点元数据
        mNodeMetaDatas += mPyNodeFactory->getNodeMetadataList();
        qDebug() << tr("Python node factory initialized, discovered %1 nodes")
                     .arg(mPyNodeFactory->getNodeMetadataList().size());
    } catch (const std::exception& e) {
        qCritical() << tr("Python node factory initialization failed: %1").arg(e.what());
        mPyNodeFactory.reset();
    }
}
#endif

}
