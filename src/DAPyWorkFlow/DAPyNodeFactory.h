#ifndef DAPYNODEFACTORY_H
#define DAPYNODEFACTORY_H
#include "DAPyWorkFlowAPI.h"
#include "DAAbstractNodeFactory.h"
#include <QList>
#include <QString>
#include <QStringList>

namespace DA
{
class DAPyNodeProxy;
class DAPyModuleWorkflow;
class DANodeMetaData;

/**
 * @brief Python节点工厂，继承DAAbstractNodeFactory
 *
 * 通过DAPyModuleWorkflow调用Python侧的DANodeRegistry发现节点，
 * 将Python的DANodeDescriptor转换为C++的DANodeMetaData，
 * 并通过DAPyNodeProxy创建节点实例。
 *
 * @code
 * auto factory = std::make_shared<DA::DAPyNodeFactory>();
 * factory->discoverNodes(); // 发现并注册Python节点
 * auto metaList = factory->getNodesMetaData();
 * @endcode
 * @see DAAbstractNodeFactory, DAPyModuleWorkflow, DAPyNodeProxy
 */
class DAPYWORKFLOW_API DAPyNodeFactory : public DAAbstractNodeFactory
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyNodeFactory)
public:
    DAPyNodeFactory();
    ~DAPyNodeFactory();

    // 发现Python节点（调用DANodeRegistry.discover）
    bool discoverNodes(const QStringList& scanPaths = QStringList(), bool useEntryPoints = false);

    // 工厂唯一标识
    virtual QString factoryPrototypes() const override;
    // 工厂名称
    virtual QString factoryName() const override;
    // 工厂描述
    virtual QString factoryDescribe() const override;
    // 创建节点实例
    virtual DAAbstractNode::SharedPointer create(const DANodeMetaData& meta) override;
    // 获取所有节点原型标识
    virtual QStringList getPrototypes() const override;
    // 获取所有节点元数据
    virtual QList< DANodeMetaData > getNodesMetaData() const override;
};
}  // namespace DA

#endif  // DAPYNODEFACTORY_H