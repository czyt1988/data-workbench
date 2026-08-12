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


/**
 * @brief 构造函数
 */
{{plugin-base-name}}NodeFactory::{{plugin-base-name}}NodeFactory() : DA::DAAbstractNodeFactory()
{
   //注册节点创建的函数指针,create函数会使用mPrototypeTpfp进行查询函数指针
   // REGISTE_CLASS(MyNode1);
   // REGISTE_CLASS(MyNode2);
}

/**
 * @brief 析构函数
 */
{{plugin-base-name}}NodeFactory::~{{plugin-base-name}}NodeFactory()
{
}

/**
 * @brief 设置核心接口
 * @param c 核心接口指针
 */
void {{plugin-base-name}}NodeFactory::setCore(DA::DACoreInterface* c)
{
    // 这里可以把一些信号关联，例如scene创建
    mCore = c;
}


/**
 * @brief 注册工作流
 * @param wf 工作流指针
 */
void {{plugin-base-name}}NodeFactory::registWorkflow(DA::DAPyWorkFlow* wf)
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

/**
 * @brief 获取所有节点的原型字符串列表
 * @return 节点原型字符串列表
 */
QStringList {{plugin-base-name}}NodeFactory::getPrototypes() const
{
    QStringList res;

    res.reserve(mPrototypeTpfp.size());
    for (auto i = mPrototypeTpfp.begin(); i != mPrototypeTpfp.end(); ++i) {
        res.append(i.key().getNodePrototype());
    }
    return (res);
}

/**
 * @brief 获取所有节点的元数据列表
 * @return 节点元数据列表
 */
QList< DA::DANodeMetaData > {{plugin-base-name}}NodeFactory::getNodesMetaData() const
{
    return mPrototypeTpfp.keys();
}

/**
 * @brief 节点被添加到工作流时的回调
 * @param node 被添加的节点
 */
void {{plugin-base-name}}NodeFactory::nodeAddedToWorkflow(DA::DAAbstractNode::SharedPointer node)
{
    DA::DAAbstractNodeFactory::nodeAddedToWorkflow(node);
}

/**
 * @brief 节点开始移除时的回调
 * @param node 开始移除的节点
 */
void {{plugin-base-name}}NodeFactory::nodeStartRemove(DA::DAAbstractNode::SharedPointer node)
{
    DA::DAAbstractNodeFactory::nodeStartRemove(node);
}

/**
 * @brief 节点连接断开时的回调
 * @param outNode 输出节点
 * @param outKey 输出节点的连接键
 * @param inNode 输入节点
 * @param inkey 输入节点的连接键
 */
void {{plugin-base-name}}NodeFactory::nodeLinkDetached(DA::DAAbstractNode::SharedPointer outNode,
                                               const QString& outKey,
                                               DA::DAAbstractNode::SharedPointer inNode,
                                               const QString& inkey)
{
    DA::DAAbstractNodeFactory::nodeLinkDetached(outNode,outKey,inNode,inkey);
}

/**
 * @brief 保存工厂扩展信息到XML
 * @param doc XML文档对象
 * @param factoryExternElement 工厂扩展信息对应的XML元素
 */
void {{plugin-base-name}}NodeFactory::saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const
{
    DA::DAAbstractNodeFactory::saveExternInfoToXml(doc,factoryExternElement);
}

/**
 * @brief 从XML加载工厂扩展信息
 * @param factoryExternElement 工厂扩展信息对应的XML元素
 */
void {{plugin-base-name}}NodeFactory::loadExternInfoFromXml(const QDomElement* factoryExternElement)
{
    DA::DAAbstractNodeFactory::loadExternInfoFromXml(factoryExternElement);
}

/**
 * @brief 获取主窗口
 * @return 主窗口指针
 */
QMainWindow* {{plugin-base-name}}NodeFactory::getMainWindow() const
{
    return mCore->getUiInterface()->getMainWindow();
}
