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
   //注册节点创建的函数指针,create函数会使用mPrototypeTpfp进行查询函数指针
   // REGISTE_CLASS(MyNode1);
   // REGISTE_CLASS(MyNode2);
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

/**
 * @brief 工厂的唯一id
 */
QString {{plugin-base-name}}NodeFactory::factoryPrototypes() const
{
    return "{{factory-prototypes}}";
}

/**
 * @brief 工厂名称
 *
 * 目前暂时无用
 */
QString {{plugin-base-name}}NodeFactory::factoryName() const
{
    return u8"{{factory-name}}";
}

/**
 * @brief 工厂描述
 *
 * 目前暂时无用
 */
QString {{plugin-base-name}}NodeFactory::factoryDescribe() const
{
    return u8"{{factory-description}}";
}

/**
 * @brief 工厂创建节点的函数
 *
 * 此函数是工厂最核心的函数，通过元对象信息创建节点，模板使用了一个map管理了元对象和创建的函数指针的关系，
 * map应在构造函数中赋值
 *
 * @param meta 节点的元对象信息
 * @return 节点指针
 */
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
