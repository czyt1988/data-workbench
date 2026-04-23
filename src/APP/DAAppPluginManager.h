#ifndef DAAPPPLUGINMANAGER_H
#define DAAPPPLUGINMANAGER_H

#include <QtCore/qglobal.h>
#include "DAGlobals.h"
#include "DACoreInterface.h"
#include <QObject>
#include <QList>
#include <QSet>
#include <memory>

#include "DANodeMetaData.h"
#include "DAPluginManager.h"
namespace DA
{
class _DAPrivateWorkflowNodePluginData;
class DAAbstractNodePlugin;
class DAAbstractPlugin;
class DAAbstractNodeFactory;
#if DA_ENABLE_PYTHON
class DAPyNodeFactory;
#endif
/**
 * @brief 此app的插件管理类
 *
 */
class DAAppPluginManager : public DAPluginManager
{
    Q_OBJECT
public:
    DAAppPluginManager(QObject* p = nullptr);
    ~DAAppPluginManager();

    // 初始化加载所有插件
    virtual void loadAllPlugins(DACoreInterface* c) override;

    // 获取所有的插件
    QList< DAAbstractPlugin* > getAllPlugins() const;

    // 获取所有的节点插件
    QList< DAAbstractNodePlugin* > getNodePlugins() const;

    // 获取所有的节点工厂
    QList< std::shared_ptr< DAAbstractNodeFactory > > createNodeFactorys() const;

    // 获取所有的元数据
    QList< DANodeMetaData > getAllNodeMetaDatas() const;

private:
#if DA_ENABLE_PYTHON
    // 初始化Python节点工厂
    void initPyNodeFactory();
#endif
    QList< DAAbstractPlugin* > mPlugins;
    QList< DANodeMetaData > mNodeMetaDatas;
#if DA_ENABLE_PYTHON
    std::shared_ptr< DAPyNodeFactory > mPyNodeFactory;  // Python节点工厂
#endif
};
}  // namespace DA
#endif  // FCMETHODEDITORPLUGINMANAGER_H
