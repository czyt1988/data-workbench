#ifndef {{PLUGIN-BASE-NAME}}NODEFACTORY_H
#define {{PLUGIN-BASE-NAME}}NODEFACTORY_H
#include "{{plugin-base-name}}Global.h"
#include "DAAbstractNodeFactory.h"
#include <QMap>
class QMainWindow;
namespace DA
{
class DACoreInterface;
class DAPyWorkFlowEditWidget;
class DAAbstractNodeLinkGraphicsItem;
class DAPyWorkFlowGraphicsScene;
}


class {{plugin-base-name}}NodeFactory : public DA::DAAbstractNodeFactory
{
    Q_OBJECT
public:
    using FpCreate      = std::function< DA::DAAbstractNode::SharedPointer(void) >;
public:
    {{plugin-base-name}}NodeFactory();
    virtual ~{{plugin-base-name}}NodeFactory() override;
    // 设置core
    void setCore(DA::DACoreInterface* c);

public:
    // 工厂设置了workflow，此函数设置为虚函数，在某些工厂可以通过此函数的重载来绑定DAPyWorkFlow的信号,以及注册回调
    //  此函数同样可以作为scene创建的回调
    virtual void registWorkflow(DA::DAPyWorkFlow* wf) override;

    // 工厂唯一标识
    virtual QString factoryPrototypes() const override;
    // 工厂名称
    virtual QString factoryName() const override;

    // 工厂描述
    virtual QString factoryDescribe() const override;

    // 创建节点
    virtual DA::DAAbstractNode::SharedPointer create(const DA::DANodeMetaData& meta) override;

    // 获取所有Prototypes
    virtual QStringList getPrototypes() const override;

    // 获取所有元数据
    virtual QList< DA::DANodeMetaData > getNodesMetaData() const override;

    // 节点加入workflow的回调
    virtual void nodeAddedToWorkflow(DA::DAAbstractNode::SharedPointer node) override;

    // 节点删除的工厂回调
    virtual void nodeStartRemove(DA::DAAbstractNode::SharedPointer node) override;
    // 节点连线删除的回调
    virtual void nodeLinkDetached(DA::DAAbstractNode::SharedPointer outNode,
                                  const QString& outKey,
                                  DA::DAAbstractNode::SharedPointer inNode,
                                  const QString& inkey) override;
    // 把扩展信息保存到xml上
    virtual void saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const override;
    // 从xml加载扩展信息
    virtual void loadExternInfoFromXml(const QDomElement* factoryExternElement) override;
public:
    // 窗口相关操作
    //  获取主体窗口
    QMainWindow* getMainWindow() const;

private:
    DA::DACoreInterface* mCore { nullptr };
    QMap< DA::DANodeMetaData, FpCreate > mPrototypeTpfp;
};

#endif  // {{PLUGIN-BASE-NAME}}NODEFACTORY_H
