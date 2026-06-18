#ifndef DAPYNODEFACTORY_H
#define DAPYNODEFACTORY_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyObjectWrapper.h"
#include "DAPyNodeMetaData.h"
#include "DAPyNode.h"
namespace DA
{

/**
 * @brief Python节点工厂，继承DAPyObjectWrapper，代理Python侧DANodeFactory
 *
 * 通过DAPyModuleWorkflow调用Python侧的DANodeRegistry发现节点，
 * 将Python的DANodeDescriptor转换为C++的DAPyNodeMetaData，
 * 并通过DAPyNode创建节点实例。
 *
 * @code
 * DA::DAPyNodeFactory factory;
 * factory.discoverNodes(); // 发现Python节点
 * auto metaList = factory.getNodeMetadataList();
 * auto proxy = factory.createNodeProxy("pkg.module.MyNode");
 * @endcode
 * @see DAPyModuleWorkflow DAPyNode DAPyNodeMetaData
 */
class DAPYWORKFLOW_API DAPyNodeFactory : public DAPyObjectWrapper
{
public:
    explicit DAPyNodeFactory();
    explicit DAPyNodeFactory(const pybind11::object& obj);
    explicit DAPyNodeFactory(pybind11::object&& obj);
    explicit DAPyNodeFactory(const DAPyObjectWrapper& obj);
    explicit DAPyNodeFactory(const DAPyNode& obj);
    ~DAPyNodeFactory();

    // 发现Python节点（调用DANodeRegistry.discover）
    bool discoverNodes(const QStringList& scanPaths = QStringList(), bool useEntryPoints = false);

    // 通过限定名创建DAPyNode实例
    DAPyNode createNode(const QString& qualifiedName);
    // 通过节点元数据创建DAPyNode实例
    DAPyNode createNode(const DAPyNodeMetaData& metaData);

    // 获取所有已发现节点的元数据列表
    QList< DAPyNodeMetaData > getNodeMetadataList() const;

    // 获取所有已发现节点的原型标识列表
    QStringList getNodePrototypes() const;

    // 工厂名称
    QString factoryName() const;

    // 工厂描述
    QString factoryDescribe() const;

private:
    // 已发现的节点元数据列表
    QList< DAPyNodeMetaData > mNodeMetaDataList;
};

}  // namespace DA

#endif  // DAPYNODEFACTORY_H
