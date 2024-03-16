#ifndef DAAPPPLUGINMANAGER_H
#define DAAPPPLUGINMANAGER_H

#include <QtCore/qglobal.h>
#include "DAGlobals.h"
#include "DACoreInterface.h"
#include <QObject>
#include <QList>
#include <QSet>

#include "DANodeMetaData.h"
namespace DA
{
class _DAPrivateWorkflowNodePluginData;
class DAAbstractNodePlugin;
class DAAbstractPlugin;
class DAAbstractNodeFactory;
/**
 * @brief 此app的插件管理类
 *
 */
class DAAppPluginManager : public QObject
{
    Q_OBJECT
    DAAppPluginManager(QObject* p = nullptr);
    ~DAAppPluginManager();

public:
    static DAAppPluginManager& instance();

    // 初始化加载所有插件
    void initLoadPlugins(DACoreInterface* c);

    // 获取所有的插件
    QList< DAAbstractPlugin* > getAllPlugins() const;

    // 获取所有的节点插件
    QList< DAAbstractNodePlugin* > getNodePlugins() const;

    // 获取所有的节点工厂
    QList< std::shared_ptr< DAAbstractNodeFactory > > createNodeFactorys() const;

    // 获取所有的元数据
    QList< DANodeMetaData > getAllNodeMetaDatas() const;

private:
    QList< DAAbstractPlugin* > mPlugins;
    QList< DAAbstractNodePlugin* > mNodePlugins;
    QList< DANodeMetaData > mNodeMetaDatas;
};
}  // namespace DA
#endif  // FCMETHODEDITORPLUGINMANAGER_H
