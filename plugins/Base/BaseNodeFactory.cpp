#include "BaseNodeFactory.h"
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


BaseNodeFactory::BaseNodeFactory() : DA::DAAbstractNodeFactory()
{

}

BaseNodeFactory::~BaseNodeFactory()
{
}

void BaseNodeFactory::setCore(DA::DACoreInterface* c)
{
    // 这里可以把一些信号关联，例如scene创建
    mCore = c;
}


void BaseNodeFactory::registWorkflow(DA::DAWorkFlow* wf)
{
    DA::DAAbstractNodeFactory::registWorkflow(wf);
}

QString BaseNodeFactory::factoryPrototypes() const
{
    return "DA.Base";
}

QString BaseNodeFactory::factoryName() const
{
    return u8"Base Factory";
}

QString BaseNodeFactory::factoryDescribe() const
{
    return u8"Base Plugin Node Factory";
}

DA::DAAbstractNode::SharedPointer BaseNodeFactory::create(const DA::DANodeMetaData& meta)
{
    auto fp = mPrototypeTpfp.value(meta, nullptr);
    if (fp) {
        return fp();
    }
    return nullptr;
}

QStringList BaseNodeFactory::getPrototypes() const
{
    QStringList res;

    res.reserve(mPrototypeTpfp.size());
    for (auto i = mPrototypeTpfp.begin(); i != mPrototypeTpfp.end(); ++i) {
        res.append(i.key().getNodePrototype());
    }
    return (res);
}

QList< DA::DANodeMetaData > BaseNodeFactory::getNodesMetaData() const
{
    return mPrototypeTpfp.keys();
}

void BaseNodeFactory::nodeAddedToWorkflow(DA::DAAbstractNode::SharedPointer node)
{
    DA::DAAbstractNodeFactory::nodeAddedToWorkflow(node);
}

void BaseNodeFactory::nodeStartRemove(DA::DAAbstractNode::SharedPointer node)
{
    DA::DAAbstractNodeFactory::nodeStartRemove(node);
}

void BaseNodeFactory::nodeLinkSucceed(DA::DAAbstractNode::SharedPointer outNode,
                                              const QString& outKey,
                                              DA::DAAbstractNode::SharedPointer inNode,
                                              const QString& inkey)
{
    DA::DAAbstractNodeFactory::nodeLinkSucceed(outNode,outKey,inNode,inkey);
}

void BaseNodeFactory::nodeLinkDetached(DA::DAAbstractNode::SharedPointer outNode,
                                               const QString& outKey,
                                               DA::DAAbstractNode::SharedPointer inNode,
                                               const QString& inkey)
{
    DA::DAAbstractNodeFactory::nodeLinkDetached(outNode,outKey,inNode,inkey);
}

void BaseNodeFactory::saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const
{
    DA::DAAbstractNodeFactory::saveExternInfoToXml(doc,factoryExternElement);
}

void BaseNodeFactory::loadExternInfoFromXml(const QDomElement* factoryExternElement)
{
    DA::DAAbstractNodeFactory::loadExternInfoFromXml(factoryExternElement);
}

DA::DANodeGraphicsSceneEventListener* BaseNodeFactory::createNodeGraphicsSceneEventListener()
{
    return nullptr;
}


QMainWindow* BaseNodeFactory::getMainWindow() const
{
    return mCore->getUiInterface()->getMainWindow();
}
