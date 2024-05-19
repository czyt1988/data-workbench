#include "DataAnalysisNodeFactory.h"
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


DataAnalysisNodeFactory::DataAnalysisNodeFactory() : DA::DAAbstractNodeFactory()
{

}

DataAnalysisNodeFactory::~DataAnalysisNodeFactory()
{
}

void DataAnalysisNodeFactory::setCore(DA::DACoreInterface* c)
{
    // 这里可以把一些信号关联，例如scene创建
    mCore = c;
}


void DataAnalysisNodeFactory::registWorkflow(DA::DAWorkFlow* wf)
{
    DA::DAAbstractNodeFactory::registWorkflow(wf);
}

QString DataAnalysisNodeFactory::factoryPrototypes() const
{
    return "DA.DataAnalysisNodeFactory";
}

QString DataAnalysisNodeFactory::factoryName() const
{
    return u8"DataAnalysis";
}

QString DataAnalysisNodeFactory::factoryDescribe() const
{
    return u8"DataAnalysis";
}

DA::DAAbstractNode::SharedPointer DataAnalysisNodeFactory::create(const DA::DANodeMetaData& meta)
{
    auto fp = mPrototypeTpfp.value(meta, nullptr);
    if (fp) {
        return fp();
    }
    return nullptr;
}

QStringList DataAnalysisNodeFactory::getPrototypes() const
{
    QStringList res;

    res.reserve(mPrototypeTpfp.size());
    for (auto i = mPrototypeTpfp.begin(); i != mPrototypeTpfp.end(); ++i) {
        res.append(i.key().getNodePrototype());
    }
    return (res);
}

QList< DA::DANodeMetaData > DataAnalysisNodeFactory::getNodesMetaData() const
{
    return mPrototypeTpfp.keys();
}

void DataAnalysisNodeFactory::nodeAddedToWorkflow(DA::DAAbstractNode::SharedPointer node)
{
    DA::DAAbstractNodeFactory::nodeAddedToWorkflow(node);
}

void DataAnalysisNodeFactory::nodeStartRemove(DA::DAAbstractNode::SharedPointer node)
{
    DA::DAAbstractNodeFactory::nodeStartRemove(node);
}

void DataAnalysisNodeFactory::nodeLinkSucceed(DA::DAAbstractNode::SharedPointer outNode,
                                              const QString& outKey,
                                              DA::DAAbstractNode::SharedPointer inNode,
                                              const QString& inkey)
{
    DA::DAAbstractNodeFactory::nodeLinkSucceed(outNode,outKey,inNode,inkey);
}

void DataAnalysisNodeFactory::nodeLinkDetached(DA::DAAbstractNode::SharedPointer outNode,
                                               const QString& outKey,
                                               DA::DAAbstractNode::SharedPointer inNode,
                                               const QString& inkey)
{
    DA::DAAbstractNodeFactory::nodeLinkDetached(outNode,outKey,inNode,inkey);
}

void DataAnalysisNodeFactory::saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const
{
    DA::DAAbstractNodeFactory::saveExternInfoToXml(doc,factoryExternElement);
}

void DataAnalysisNodeFactory::loadExternInfoFromXml(const QDomElement* factoryExternElement)
{
    DA::DAAbstractNodeFactory::loadExternInfoFromXml(factoryExternElement);
}

DA::DANodeGraphicsSceneEventListener* DataAnalysisNodeFactory::createNodeGraphicsSceneEventListener()
{
    return nullptr;
}


QMainWindow* DataAnalysisNodeFactory::getMainWindow() const
{
    return mCore->getUiInterface()->getMainWindow();
}
