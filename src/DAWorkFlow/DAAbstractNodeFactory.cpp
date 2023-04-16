#include "DAAbstractNodeFactory.h"
#include <QPointer>
#include <DAWorkFlow.h>

namespace DA
{
class DAAbstractNodeFactory::PrivateData
{
    DA_DECLARE_PUBLIC(DAAbstractNodeFactory)
public:
    DAAbstractNodeFactory::PrivateData(DAAbstractNodeFactory* p);

public:
    QPointer< DAWorkFlow > mWorkflow;
};

//////////////////////////////////////////////
/// DAAbstractNodeFactoryPrivate
//////////////////////////////////////////////

DAAbstractNodeFactory::PrivateData::PrivateData(DAAbstractNodeFactory* p) : q_ptr(p)
{
}

//////////////////////////////////////////////
/// DAAbstractNodeFactory
//////////////////////////////////////////////

DAAbstractNodeFactory::DAAbstractNodeFactory(QObject* p) : QObject(p), DA_PIMPL_CONSTRUCT
{
}

DAAbstractNodeFactory::~DAAbstractNodeFactory()
{
}
/**
 * @brief 工厂设置了workflow，此函数设置为虚函数，在某些工厂可以通过此函数的重载来绑定DAWorkFlow的信号
 *
 * @note 重载此函数一定要调用DAAbstractNodeFactory::registWorkflow,否则@sa getWorkFlow 一直返回空
 */
void DAAbstractNodeFactory::registWorkflow(DAWorkFlow* wf)
{
    d_ptr->mWorkflow = wf;
}

/**
 * @brief 获取工作流
 * @return
 */
DAWorkFlow* DAAbstractNodeFactory::getWorkFlow() const
{
    return d_ptr->mWorkflow.data();
}
/**
 * @brief 节点加入workflow的回调
 * 在调用DAWorkFlow::addNode会触发node对应工厂的此函数的回调
 */
void DAAbstractNodeFactory::nodeAddedToWorkflow(DAAbstractNode::SharedPointer node)
{
    Q_UNUSED(node);
}
/**
 * @brief 节点删除的工厂回调
 * @param node
 * @note 此函数会在@sa DAWorkFlow::removeNode 调用时调用
 */
void DAAbstractNodeFactory::nodeStartRemove(DAAbstractNode::SharedPointer node)
{
    Q_UNUSED(node);
}
/**
 * @brief 节点连接成功的回调
 * @param outNode
 * @param outKey
 * @param inNode
 * @param inkey
 */
void DAAbstractNodeFactory::nodeLinkSucceed(DAAbstractNode::SharedPointer outNode,
                                            const QString& outKey,
                                            DAAbstractNode::SharedPointer inNode,
                                            const QString& inkey)
{
    Q_UNUSED(outNode);
    Q_UNUSED(outKey);
    Q_UNUSED(inNode);
    Q_UNUSED(inkey);
}
/**
 * @brief 节点连线删除的回调
 * @param outNode 输出节点
 * @param outKey 输出key
 * @param intNode 输入节点
 * @param inkey 输入key
 */
void DAAbstractNodeFactory::nodeLinkDetached(DAAbstractNode::SharedPointer outNode,
                                             const QString& outKey,
                                             DAAbstractNode::SharedPointer inNode,
                                             const QString& inkey)
{
    Q_UNUSED(outNode);
    Q_UNUSED(outKey);
    Q_UNUSED(inNode);
    Q_UNUSED(inkey);
}

/**
 * @brief 把扩展信息保存到xml上
 *
 * 此函数在工作流保存的过程中会调用，把工厂的附加信息保存到xml文件上
 *
 * @note 工作流保存过程如下：
 * -# 保存工作流扩展信息
 * -# 保存节点信息
 * -# 保存链接信息
 * -# 保存特殊item（非工作流的item）
 * -# 保存工厂扩展信息
 * -# 保存scene信息
 * @param doc
 * @param factoryExternElement
 */
void DAAbstractNodeFactory::saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const
{
    Q_UNUSED(doc);
    Q_UNUSED(factoryExternElement);
}

/**
 * @brief 加载扩展信息到工厂中
 *
 * 此函数会在工作流加载过程中调用，把工厂的特殊信息加载
 *
 * @note 工作流加载过程如下：
 * -# 加载工作流扩展信息
 * -# 加载节点信息
 * -# 加载链接信息
 * -# 加载特殊item（非工作流的item）
 * -# 加载工厂扩展信息
 * -# 加载scene信息
 *
 * @param factoryExternElement
 */
void DAAbstractNodeFactory::loadExternInfoFromXml(const QDomElement* factoryExternElement)
{
    Q_UNUSED(factoryExternElement);
}
}  // end DA
