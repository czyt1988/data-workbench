#include "{{plugin-base-name}}NodeFactory.h"
//
#include <QMainWindow>
#include "DACoreInterface.h"
#include "DADockingAreaInterface.h"
#include "DAUIInterface.h"
#include "DAProjectInterface.h"

#ifndef REGISTE_CLASS
#define REGISTE_CLASS(className)                                                                                       \
    do {                                                                                                               \
        auto fp = []() -> DA::DAAbstractNode::SharedPointer {                                                          \
            DA::DAAbstractNode::SharedPointer node(new className());                                                   \
            return node;                                                                                               \
        };                                                                                                             \
        DA::DAAbstractNode::SharedPointer t = fp();                                                                    \
        mPrototypeTpfp[ t->metaData() ]    = fp;                                                                      \
    } while (0)
#endif

//! 注册节点在构造函数中执行REGISTE_CLASS宏，即可把节点类注册
//! REGISTE_CLASS(XXXXNode)


{{plugin-base-name}}NodeFactory::{{plugin-base-name}}NodeFactory() : DA::DAAbstractNodeFactory()
{

}

{{plugin-base-name}}NodeFactory::~{{plugin-base-name}}NodeFactory()
{
}

void {{plugin-base-name}}NodeFactory::setCore(DA::DACoreInterface* c)
{
    // 这里可以把一些信号关联，例如scene创建
    mCore = c;
}


void {{plugin-base-name}}NodeFactory::registWorkflow(DA::DAWorkFlow* wf)
{
    DA::DAAbstractNodeFactory::registWorkflow(wf);
}

QString {{plugin-base-name}}NodeFactory::factoryPrototypes() const
{
    return "{{factory-prototypes}}";
}

QString {{plugin-base-name}}NodeFactory::factoryName() const
{
    return u8"{{factory-name}}";
}

QString {{plugin-base-name}}NodeFactory::factoryDescribe() const
{
    return u8"{{factory-description}}";
}

DA::DAAbstractNode::SharedPointer {{plugin-base-name}}NodeFactory::create(const DA::DANodeMetaData& meta)
{
    auto fp = mPrototypeTpfp.value(meta, nullptr);
    if (fp) {
        return fp();
    }
    return nullptr;
}

QStringList {{plugin-base-name}}NodeFactory::getPrototypes() const
{
    QStringList res;

    res.reserve(mPrototypeTpfp.size());
    for (auto i = mPrototypeTpfp.begin(); i != mPrototypeTpfp.end(); ++i) {
        res.append(i.key().getNodePrototype());
    }
    return (res);
}

QList< DA::DANodeMetaData > {{plugin-base-name}}NodeFactory::getNodesMetaData() const
{
    return mPrototypeTpfp.keys();
}

void {{plugin-base-name}}NodeFactory::nodeAddedToWorkflow(DA::DAAbstractNode::SharedPointer node)
{
    DA::DAAbstractNodeFactory::nodeAddedToWorkflow(node);
}

void {{plugin-base-name}}NodeFactory::nodeStartRemove(DA::DAAbstractNode::SharedPointer node)
{
    DA::DAAbstractNodeFactory::nodeStartRemove(node);
}

void {{plugin-base-name}}NodeFactory::nodeLinkSucceed(DA::DAAbstractNode::SharedPointer outNode,
                                              const QString& outKey,
                                              DA::DAAbstractNode::SharedPointer inNode,
                                              const QString& inkey)
{
    DA::DAAbstractNodeFactory::nodeLinkSucceed(outNode,outKey,inNode,inkey);
}

void {{plugin-base-name}}NodeFactory::nodeLinkDetached(DA::DAAbstractNode::SharedPointer outNode,
                                               const QString& outKey,
                                               DA::DAAbstractNode::SharedPointer inNode,
                                               const QString& inkey)
{
    DA::DAAbstractNodeFactory::nodeLinkDetached(outNode,outKey,inNode,inkey);
}

void {{plugin-base-name}}NodeFactory::saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const
{
    DA::DAAbstractNodeFactory::saveExternInfoToXml(doc,factoryExternElement);
}

void {{plugin-base-name}}NodeFactory::loadExternInfoFromXml(const QDomElement* factoryExternElement)
{
    DA::DAAbstractNodeFactory::loadExternInfoFromXml(factoryExternElement);
}

DA::DANodeGraphicsSceneEventListener* {{plugin-base-name}}NodeFactory::createNodeGraphicsSceneEventListener()
{
    return nullptr;
}


QMainWindow* {{plugin-base-name}}NodeFactory::getMainWindow() const
{
    return mCore->getUiInterface()->getMainWindow();
}
