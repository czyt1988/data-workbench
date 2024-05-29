#include "DAAbstractNode.h"
#include "DAAbstractNodeGraphicsItem.h"
#include "DANodeMetaData.h"
#include "DAWorkFlow.h"
#include "DAAbstractNodeFactory.h"
#include "DAQtContainerUtil.hpp"
#include "DAGlobals.h"
// Qxml
#include <QPointer>
#include <QDomComment>
#include <QDomElement>
#include <QDateTime>

/**
 * @def 此打印开启将会打印辅助信息
 */
#define DA_DAABSTRACTNODE_DEBUG_PRINT 0

namespace DA
{
class DAAbstractNode::PrivateData
{
	DA_DECLARE_PUBLIC(DAAbstractNode)
public:
	class LinkData
	{
	public:
		using NodeSharedPtr = DAAbstractNode::SharedPointer;
		using NodeWeakPtr   = std::weak_ptr< DAAbstractNode >;
		LinkData();
		LinkData(const NodeSharedPtr& inNode, const QString& inKey);
		LinkData(const NodeSharedPtr& outNode, const QString& outKey, const NodeSharedPtr& inNode, const QString& inKey);
		bool operator==(const LinkData& d) const;
		NodeWeakPtr inputNode;
		QString inputKey;
		NodeWeakPtr outputNode;
		QString outputKey;
	};
	QList< LinkData > getOutputLinkData(const QString& k);
	QList< LinkData > getInputLinkData(const QString& k);

public:
	PrivateData(DAAbstractNode* p);
	DANodeMetaData mMetaData;
	QList< LinkData > mLinksInfo;  ///< 记录所有连接信息
	QList< QString > mInputKeys;   ///< 记录输入节点，原来使用mInputData的keys，但这样无法保证有序
	QHash< QString, QVariant > mInputData;  ///< 节点输入数据
	QList< QString > mOutputKeys;  ///< 记录输出节点，原来使用mOutputData的keys，但这样无法保证有序
	QHash< QString, QVariant > mOutputData;  ///< 节点输出数据
	QHash< QString, QVariant > mPropertys;   ///< 属性数据
	DAAbstractNode::IdType mId;
	QPointer< DAWorkFlow > mWorkflow;               ///< 持有的workflow
	DAAbstractNodeGraphicsItem* mItem { nullptr };  ///< node 对应的item
	DAAbstractNodeFactory::WeakPointer mFactory;  ///< 保存节点的工厂，工厂的设置在DAWorkFlow::createNode中
};
}

using namespace DA;
//================================================
// DAAbstractNodePrivate::LinkData
//================================================

DAAbstractNode::PrivateData::LinkData::LinkData()
{
}

DAAbstractNode::PrivateData::LinkData::LinkData(const NodeSharedPtr& inNode, const QString& inKey)
    : inputNode(inNode), inputKey(inKey)
{
}

DAAbstractNode::PrivateData::LinkData::LinkData(const DAAbstractNode::PrivateData::LinkData::NodeSharedPtr& outNode,
                                                const QString& outKey,
                                                const DAAbstractNode::PrivateData::LinkData::NodeSharedPtr& inNode,
                                                const QString& inKey)
    : outputNode(outNode), outputKey(outKey), inputNode(inNode), inputKey(inKey)
{
}

bool DAAbstractNode::PrivateData::LinkData::operator==(const DAAbstractNode::PrivateData::LinkData& d) const
{
	return (inputNode.lock() == d.inputNode.lock()) && (inputKey == d.inputKey)
           && (outputNode.lock() == d.outputNode.lock()) && (outputKey == d.outputKey);
}

/**
 * @brief 获取out节点名字下的所有LinkData
 * @param k
 * @return
 */
QList< DAAbstractNode::PrivateData::LinkData > DAAbstractNode::PrivateData::getOutputLinkData(const QString& k)
{
	QList< DAAbstractNode::PrivateData::LinkData > res;
	for (const LinkData& d : qAsConst(mLinksInfo)) {
		if (d.outputNode.lock().get() == q_ptr && d.outputKey == k) {
			res.append(d);
		}
	}
	return res;
}

/**
 * @brief DAAbstractNodePrivate::getInputLinkData
 * @param k
 * @return
 */
QList< DAAbstractNode::PrivateData::LinkData > DAAbstractNode::PrivateData::getInputLinkData(const QString& k)
{
	QList< DAAbstractNode::PrivateData::LinkData > res;
	for (const LinkData& d : qAsConst(mLinksInfo)) {
		if (d.inputNode.lock().get() == q_ptr && d.inputKey == k) {
			res.append(d);
		}
	}
	return res;
}

//==============================================================
// DAAbstractNode::PrivateData
//==============================================================

DAAbstractNode::PrivateData::PrivateData(DAAbstractNode* p) : q_ptr(p)
{
	mId = p->generateID();
}

//================================================
// DAAbstractNode::LinkInfo
//================================================

DAAbstractNode::LinkInfo::LinkInfo()
{
}

//================================================
// DAAbstractNode
//================================================

DAAbstractNode::DAAbstractNode() : d_ptr(new DAAbstractNode::PrivateData(this))
{
}

/**
 * @brief 节点的销毁，节点销毁过程会通知相关联的节点把自己信息解除
 */
DAAbstractNode::~DAAbstractNode()
{
    detachAll();
}

/**
 * @brief 获取节点的名字
 * @return
 */
QString DAAbstractNode::getNodeName() const
{
    return (d_ptr->mMetaData.getNodeName());
}

/**
 * @brief 设置节点名
 * @param name
 * @note 此函数会导致关联的DAWorkFlow发射nodeNameChanged信号@sa DAWorkFlow::nodeNameChanged
 * @note 此函数为虚函数，在一些场合需要对名字进行校验的特殊节点可继承此函数来进行校验，但记得调用DAAbstractNode::setNodeName使之生效
 */
void DAAbstractNode::setNodeName(const QString& name)
{
	QString oldname = d_ptr->mMetaData.getNodeName();
	d_ptr->mMetaData.setNodeName(name);
	if (d_ptr->mWorkflow) {
		d_ptr->mWorkflow->emitNodeNameChanged(shared_from_this(), oldname, name);
	}
}

/**
 * @brief FCAbstractNode::getNodePrototype
 * @return
 */
QString DAAbstractNode::getNodePrototype() const
{
    return (d_ptr->mMetaData.getNodePrototype());
}

/**
 * @brief 获取分组
 * @return
 */
QString DAAbstractNode::getNodeGroup() const
{
    return d_ptr->mMetaData.getGroup();
}

/**
 * @brief 获取节点图标
 * @return 图标
 */
QIcon DAAbstractNode::getIcon() const
{
    return (d_ptr->mMetaData.getIcon());
}

/**
 * @brief 设置图标
 * @param icon 设置的图标
 */
void DAAbstractNode::setIcon(const QIcon& icon)
{
    d_ptr->mMetaData.setIcon(icon);
}

/**
 * @brief 获取节点元数据
 * @return
 */
const DANodeMetaData& DAAbstractNode::metaData() const
{
    return (d_ptr->mMetaData);
}

/**
 * @brief 获取节点元数据
 * @return
 */
DANodeMetaData& DAAbstractNode::metaData()
{
    return (d_ptr->mMetaData);
}

/**
 * @brief 获取节点的说明
 * @return 返回说明字符串
 * @sa setNodeTooltip
 */
QString DAAbstractNode::getNodeTooltip() const
{
    return (d_ptr->mMetaData.getNodeTooltip());
}

/**
 * @brief 设置节点的说明
 * @param tp 说明文本
 * @sa getNodeTooltip
 */
void DAAbstractNode::setNodeTooltip(const QString& tp)
{
    d_ptr->mMetaData.setNodeTooltip(tp);
}

/**
 * @brief 设置元数据
 * @param metadata
 */
void DAAbstractNode::setMetaData(const DANodeMetaData& metadata)
{
    d_ptr->mMetaData = metadata;
}

/**
 * @brief 返回自身的引用
 * @return
 */
DAAbstractNode::SharedPointer DAAbstractNode::pointer()
{
    return (shared_from_this());
}

/**
 * @brief 获取id
 * @see setID generateID
 * @return
 */
DAAbstractNode::IdType DAAbstractNode::getID() const
{
    return d_ptr->mId;
}

/**
 * @brief 设置id
 * @see getID generateID
 * @param d
 */
void DAAbstractNode::setID(const IdType& d)
{
    d_ptr->mId = d;
}

/**
 * @brief 判断是否存在属性
 * @param k
 * @return
 */
bool DAAbstractNode::hasProperty(const QString& k) const
{
    return d_ptr->mPropertys.contains(k);
}

/**
 * @brief 设置属性
 * @note 如果节点有一些额外属性，通过此函数保存才能存入文件系统中
 * @param k 属性键值
 * @param v 属性值
 */
void DAAbstractNode::setProperty(const QString& k, const QVariant& v)
{
    d_ptr->mPropertys[ k ] = v;
}

/**
 * @brief 读取属性
 * @note 如果节点有一些额外属性，通过此函数保存才能存入文件系统中
 * @param k
 * @param defaultVal
 * @return
 */
QVariant DAAbstractNode::getProperty(const QString& k, const QVariant& defaultVal) const
{
    return d_ptr->mPropertys.value(k, defaultVal);
}

/**
 * @brief 移除属性
 * @param k
 */
bool DAAbstractNode::removeProperty(const QString& k)
{
    return d_ptr->mPropertys.remove(k);
}

/**
 * @brief 获取所以得属性关键字
 * @return
 */
QList< QString > DAAbstractNode::getPropertyKeys() const
{
    return d_ptr->mPropertys.keys();
}

/**
 * @brief 把信息保存到xml上
 *
 * DAAbstractNode的此函数不会实现任何功能，继承的node要保存一些参数可以通过继承此函数实现
 * @param doc
 * @param parentElement
 */
void DAAbstractNode::saveExternInfoToXml(QDomDocument* doc, QDomElement* nodeElement,const QVersionNumber& ver) const
{
	Q_UNUSED(doc);
	Q_UNUSED(nodeElement);
    Q_UNUSED(ver);
}

/**
 * @brief 从xml加载扩展信息
 *
 * DAAbstractNode的此函数不会实现任何功能，继承的node要加载一些参数可以通过继承此函数实现
 * @param parentElement
 */
void DAAbstractNode::loadExternInfoFromXml(const QDomElement* nodeElement,const QVersionNumber& ver)
{
    Q_UNUSED(nodeElement);
    Q_UNUSED(ver);
}

/**
 * @brief 节点类型默认都为NormalNode
 * @return
 */
DAAbstractNode::NodeType DAAbstractNode::nodeType() const
{
    return NormalNode;
}

/**
 * @brief 获取说有的输入参数
 * @return
 */
QList< QString > DAAbstractNode::getInputKeys() const
{
    return d_ptr->mInputKeys;
}

/**
 * @brief 获取输入节点的数量
 * @return
 */
int DAAbstractNode::getInputKeysConut() const
{
    return d_ptr->mInputKeys.size();
}

/**
 * @brief 获取所有已经链接上的输入节点
 * @return 结果已经去重
 */
QList< QString > DAAbstractNode::getLinkedInputKeys() const
{
	QSet< QString > res;
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
		SharedPointer toNode = d.inputNode.lock();
		if (toNode.get() == this) {
			// 说明这个是其他节点连接到自身的item
			res.insert(d.inputKey);
		}
	}
	return qset_to_qlist(res);
}

/**
 * @brief 获取所有的输出参数
 * @return
 */
QList< QString > DAAbstractNode::getOutputKeys() const
{
    return d_ptr->mOutputKeys;
}

/**
 * @brief 获取输出节点的数量
 * @return
 */
int DAAbstractNode::getOutputKeysConut() const
{
    return d_ptr->mOutputKeys.size();
}

/**
 * @brief 获取所有已经链接上的输出的参数名
 * @return 结果已经去重
 */
QList< QString > DAAbstractNode::getLinkedOutputKeys() const
{
	QSet< QString > res;
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
		SharedPointer fromNode = d.outputNode.lock();
		if (fromNode.get() == this) {
			// 说明这个是其他节点连接到自身的item
			res.insert(d.outputKey);
		}
	}
	return qset_to_qlist(res);
}

/**
 * @brief 添加一个输入参数
 * @param k
 */
void DAAbstractNode::addInputKey(const QString& k)
{
	if (d_ptr->mInputKeys.contains(k)) {
		return;
	}
	d_ptr->mInputKeys.append(k);
}
/**
 * @brief 添加一个输出参数
 * @param k
 */
void DAAbstractNode::addOutputKey(const QString& k)
{
	if (d_ptr->mOutputKeys.contains(k)) {
		return;
	}
	d_ptr->mOutputKeys.append(k);
}

/**
 * @brief 建立连接,从out到另外一个item的in
 *
 * 建立连接,如果基础的对象需要校验，可继承此函数
 *
 * 注意，继承的方法中一定要调用父类方法DAAbstractNode::linkTo，否则信息会异常
 *
 * @param outpt 输出点
 * @param toItem 输入的item
 * @param topt 输入点
 * @return 成功返回true
 */
bool DAAbstractNode::linkTo(const QString& outKey, DAAbstractNode::SharedPointer inNode, const QString& inKey)
{
	if (!linkTo_(outKey, inNode, inKey)) {
		return false;
	}
	if (auto f = factory()) {
		f->nodeLinkSucceed(pointer(), outKey, inNode, inKey);
	}
#if DA_DAABSTRACTNODE_DEBUG_PRINT
	qDebug() << getNodeName() << "->linkTo(outKey=" << outKey << ",inNode=" << inNode->getNodeName()
             << ",inKey=" << inKey << ")";
#endif
	return (true);
}

/**
 * @brief detachToLink会对_toNode进行删除操作，因此不允许在_toNode迭代环境中调用此函数
 * @param outpt
 * @return
 */
bool DAAbstractNode::detachLink(const QString& key)
{
	QList< DAAbstractNode::PrivateData::LinkData > outs = d_ptr->getOutputLinkData(key);
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(outs)) {
		d_ptr->mLinksInfo.removeAll(d);
		SharedPointer inputNode = d.inputNode.lock();
		if (inputNode && inputNode.get() != this) {
			// 说明这个是从本身连接到其他，则其他节点也需要删除这个链接
			inputNode->d_func()->mLinksInfo.removeAll(d);
			// 通知工厂的回调函数
			if (auto f = factory()) {
				f->nodeLinkDetached(d.outputNode.lock(), d.outputKey, inputNode, d.inputKey);
			}
		}
	}
	QList< DAAbstractNode::PrivateData::LinkData > ins = d_ptr->getInputLinkData(key);
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(ins)) {
		d_ptr->mLinksInfo.removeAll(d);
		SharedPointer outputNode = d.outputNode.lock();
		if (outputNode && outputNode.get() != this) {
			// 说明这个是从本身连接到其他，则其他节点也需要删除这个链接
			outputNode->d_func()->mLinksInfo.removeAll(d);
			// 通知工厂的回调函数
			if (auto f = factory()) {
				f->nodeLinkDetached(outputNode, d.outputKey, pointer(), d.inputKey);
			}
		}
	}
	return (ins.size() + outs.size()) > 0;
}

/**
 * @brief 移除所有依赖，一般是节点被删除时会调用此函数
 */
void DAAbstractNode::detachAll()
{
#if DA_DAABSTRACTNODE_DEBUG_PRINT
	qDebug() << "---------------------------";
	qDebug() << "-start detachAll"
             << "\n- node name:" << getNodeName() << ",prototype:" << metaData().getNodePrototype() << "\n- link infos:";
	for (const DAAbstractNodePrivate::LinkData& d : qAsConst(d_ptr->_linksInfo)) {
		SharedPointer from = d.outputNode.lock();
		SharedPointer to   = d.inputNode.lock();
		if (from && to) {
			qDebug().noquote() << from->getNodeName() << "[" << d.outputKey << "] -> " << to->getNodeName() << "["
                               << d.inputKey << "]";
		} else if (from && to == nullptr) {
			qDebug().noquote() << from->getNodeName() << "[" << d.outputKey << "] -> "
                               << "null";
		} else if (from == nullptr && to) {
			qDebug().noquote() << "null -> " << to->getNodeName() << "[" << d.inputKey << "]";
		} else {
			qDebug().noquote() << "null -> null";
		}
	}
#endif
	//! 不能直接迭代_toNode过程调用detachToLink，会导致迭代器失效
	// 清空关系
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
		SharedPointer toItem = d.inputNode.lock();
		if (toItem && toItem.get() != this) {
			// 说明这个是从本身连接到其他，则其他节点也需要删除这个链接
			toItem->d_func()->mLinksInfo.removeAll(d);
			// 通知工厂的回调函数
			if (auto f = factory()) {
				f->nodeLinkDetached(d.outputNode.lock(), d.outputKey, d.inputNode.lock(), d.inputKey);
			}
		}
	}
	d_ptr->mLinksInfo.clear();
}

/**
 * @brief 获取所有连接了输入keys的节点
 *              input       Output
 * ┌───┐              ┌───┐                 ┌───┐
 * │in │━━━━━━━━━━━━━━│ * │━━━━━━━━━━━━━━━━━│out│
 * └───┘              └───┘                 └───┘
 *         LinkData
 * outputkey        inputkey
 * outputNode       inputNode
 * @return 返回连接了node的input keys的节点
 */
QList< DAAbstractNode::SharedPointer > DAAbstractNode::getInputNodes() const
{
	QSet< DAAbstractNode::SharedPointer > res;
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
		SharedPointer toNode = d.inputNode.lock();
		if (toNode.get() == this) {
			// 说明这个是其他节点连接到自身的item
			SharedPointer fromNode = d.outputNode.lock();
			if (fromNode) {
				res.insert(fromNode);
			}
		}
	}
	return qset_to_qlist(res);
}
/**
 * @brief 获取所有连接了inputkey的节点
 *              input       Output
 * ┌───┐              ┌───┐                 ┌───┐
 * │in │━━━━━━━━━━━━━━│ * │━━━━━━━━━━━━━━━━━│out│
 * └───┘              └───┘                 └───┘
 *         LinkData
 * outputkey        inputkey
 * outputNode       inputNode
 * @return 返回连接了node的inputkey的节点
 */
QList< DAAbstractNode::SharedPointer > DAAbstractNode::getInputNodes(const QString inputkey) const
{
	QSet< DAAbstractNode::SharedPointer > res;
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
		if (d.inputKey != inputkey) {
			continue;
		}
		SharedPointer toNode = d.inputNode.lock();
		if (toNode.get() == this) {
			// 说明这个是其他节点连接到自身的item
			SharedPointer fromNode = d.outputNode.lock();
			if (fromNode) {
				res.insert(fromNode);
			}
		}
	}
	return qset_to_qlist(res);
}

/**
 * @brief 获取此节点输出到其他的节点
 *              input       Output
 * ┌───┐              ┌───┐                 ┌───┐
 * │in │━━━━━━━━━━━━━━│ * │━━━━━━━━━━━━━━━━━│out│
 * └───┘              └───┘                 └───┘
 *                              LinkData
 *                      outputkey        inputkey
 *                      outputNode       inputNode
 *
 * @return
 */
QList< DAAbstractNode::SharedPointer > DAAbstractNode::getOutputNodes() const
{
	QSet< DAAbstractNode::SharedPointer > res;
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
		SharedPointer fromNode = d.outputNode.lock();
		if (fromNode.get() == this) {
			// 说明这个是此节点连接到其他的node
			SharedPointer toNode = d.inputNode.lock();
			if (toNode) {
				res.insert(toNode);
			}
		}
	}
	return qset_to_qlist(res);
}
/**
 * @brief 获取此节点输出到其他的节点
 *              input       Output
 * ┌───┐              ┌───┐                 ┌───┐
 * │in │━━━━━━━━━━━━━━│ * │━━━━━━━━━━━━━━━━━│out│
 * └───┘              └───┘                 └───┘
 *                              LinkData
 *                      outputkey        inputkey
 *                      outputNode       inputNode
 *
 * @return
 */
QList< DAAbstractNode::SharedPointer > DAAbstractNode::getOutputNodes(const QString outputkey) const
{
	QSet< DAAbstractNode::SharedPointer > res;
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
		if (d.outputKey != outputkey) {
			continue;
		}
		SharedPointer fromNode = d.outputNode.lock();
		if (fromNode.get() == this) {
			// 说明这个是此节点连接到其他的node
			SharedPointer toNode = d.inputNode.lock();
			if (toNode) {
				res.insert(toNode);
			}
		}
	}
	return qset_to_qlist(res);
}

/**
 * @brief 获取输入节点的数量
 * @return
 */
int DAAbstractNode::getInputNodesCount() const
{
	int res = 0;
#if DA_DAABSTRACTNODE_DEBUG_PRINT
	qDebug() << getNodeName()
             << "-> getInputNodesCount()\n"
                "    _linksInfo:";
#endif
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
#if DA_DAABSTRACTNODE_DEBUG_PRINT
		qDebug() << "    outputKey=" << d.outputKey << "(" << d.outputNode.lock()->getNodeName()
                 << ")--->inputKey=" << d.inputKey << "(" << d.inputNode.lock()->getNodeName() << ")";
#endif
		SharedPointer toNode = d.inputNode.lock();
		if (toNode.get() == this) {
			// 说明这个是其他节点连接到自身的item
			++res;
		}
	}
	return res;
}

/**
 * @brief 获取输出节点的数量
 * @return
 */
int DAAbstractNode::getOutputNodesCount() const
{
	int res = 0;
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
		SharedPointer fromNode = d.outputNode.lock();
		if (fromNode.get() == this) {
			// 说明这个是此节点连接到其他的node
			++res;
		}
	}
	return res;
}

/**
 * @brief 输入参数
 * @param inputName
 * @param dp 数据会发生拷贝操作，不会影响原来数据包
 */
void DAAbstractNode::setInputData(const QString& key, const QVariant& dp)
{
	if (!d_ptr->mInputKeys.contains(key)) {
		return;
	}
	d_ptr->mInputData[ key ] = dp;
}

/**
 * @brief FCAbstractNode::inputData
 * @param key
 * @return
 */
QVariant DAAbstractNode::getInputData(const QString& key) const
{
    return (d_ptr->mInputData.value(key));
}

QVariant DAAbstractNode::getOutputData(const QString& key) const
{
    return (d_ptr->mOutputData.value(key));
}

/**
 * @brief 获取所有输入（入度）的信息
 * @return
 */
QList< DAAbstractNode::LinkInfo > DAAbstractNode::getAllInputLinkInfo() const
{
	QHash< QString, DAAbstractNode::LinkInfo > res;
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
		SharedPointer n = d.inputNode.lock();
		if (n.get() == this) {
			// 说明输出节点是自己
			SharedPointer outputNode = d.outputNode.lock();
			if (!outputNode) {
				continue;
			}
			auto ite = res.find(d.inputKey);
			if (ite == res.end()) {
				ite             = res.insert(d.inputKey, DAAbstractNode::LinkInfo());
				ite.value().key = d.inputKey;
			}
			ite.value().nodes.append(qMakePair(d.outputKey, outputNode));
		}
	}
	return res.values();
}

/**
 * @brief 获取输出（出度）的链接信息
 * @return
 */
QList< DAAbstractNode::LinkInfo > DAAbstractNode::getAllOutputLinkInfo() const
{
	QHash< QString, DAAbstractNode::LinkInfo > res;
	for (const DAAbstractNode::PrivateData::LinkData& d : qAsConst(d_ptr->mLinksInfo)) {
		SharedPointer n = d.outputNode.lock();
		if (n.get() == this) {  // 说明输出节点是自己
			// 首先找到输出节点对应的节点
			SharedPointer inputnode = d.inputNode.lock();
			if (!inputnode) {
				continue;
			}
			// 找到这个输出节点的连接信息，有可能有多个输出对应着多个输出节点
			auto ite = res.find(d.outputKey);
			if (ite == res.end()) {
				ite             = res.insert(d.outputKey, DAAbstractNode::LinkInfo());
				ite.value().key = d.outputKey;
			}
			ite.value().nodes.append(qMakePair(d.inputKey, inputnode));
		}
	}
	return res.values();
}
/**
 * @brief 生成一个唯一id
 * @return
 */
DAAbstractNode::IdType DAAbstractNode::generateID() const
{
	union {
		IdType id;
		uint32_t raw[ 2 ];
	} mem;
	QDateTime dt = QDateTime::currentDateTime();

	mem.raw[ 0 ] = uint32_t(dt.toMSecsSinceEpoch());
	mem.raw[ 1 ] = uintptr_t(this);
	return mem.id;
}

/**
 * @brief 获取工作流
 * @return
 */
DAWorkFlow* DAAbstractNode::workflow() const
{
    return d_ptr->mWorkflow;
}

/**
 * @brief 获取工厂
 * @return
 */
std::shared_ptr< DAAbstractNodeFactory > DAAbstractNode::factory() const
{
    return d_ptr->mFactory.lock();
}

/**
 * @brief 获取graphicsItem
 * @return
 */
DA::DAAbstractNodeGraphicsItem* DAAbstractNode::graphicsItem() const
{
    return d_ptr->mItem;
}

/**
 * @brief 记录item，此函数在DAAbstractNodeGraphicsItem构造函数中调用
 * @param it
 * @note 在createGraphicsItem时，构造DAAbstractNodeGraphicsItem就会在
 * DAAbstractNodeGraphicsItem的构造函数中调用此函数记录DAAbstractNodeGraphicsItem
 */
void DAAbstractNode::registItem(DAAbstractNodeGraphicsItem* it)
{
	if (d_ptr->mItem) {
		// 这种情况说明用户误操作，一般是调用了两次createGraphicsItem
		qCritical() << "node regist item more than one";
	}
	d_ptr->mItem = it;
}

/**
 * @brief 解除对item的记录
 * 在某些环境下，删除item就会调用此函数解除item的记录
 */
void DAAbstractNode::unregistItem()
{
    d_ptr->mItem = nullptr;
}

/**
   @brief DAAbstractNode::linkToNoCallback
   @param outKey
   @param inNode
   @param inKey
   @return
 */
bool DAAbstractNode::linkTo_(const QString& outKey, DAAbstractNode::SharedPointer inNode, const QString& inKey)
{
	if (!getOutputKeys().contains(outKey)) {
		qDebug() << "invalid link [" << getNodeName() << "] can not find out key " << outKey;
		return (false);
	}
	if (!(inNode->getInputKeys().contains(inKey))) {
		qDebug() << "invalid link [" << inNode->getNodeName() << "] can not find in Key " << inKey;
		return (false);
	}
	DAAbstractNode::PrivateData::LinkData ld(pointer(), outKey, inNode, inKey);
	d_ptr->mLinksInfo.append(ld);             // 当前记录连接信息
	inNode->d_func()->mLinksInfo.append(ld);  // 被连接的节点也记录下连接信息
	return (true);
}

/**
 * @brief 设置输出的参数
 * @param key
 * @param dp
 */
void DAAbstractNode::setOutputData(const QString& key, const QVariant& dp)
{
    d_ptr->mOutputData[ key ] = dp;
}
/**
 * @brief 移除输入,如果有数据，数据也会移除
 * @param key
 */
void DAAbstractNode::removeInputKey(const QString& key)
{
	d_ptr->mInputKeys.removeAll(key);
	d_ptr->mInputData.remove(key);
}
/**
 * @brief 移除输出,如果有数据，数据也会移除
 * @param key
 */
void DAAbstractNode::removeOutputKey(const QString& key)
{
	d_ptr->mOutputKeys.removeAll(key);
	d_ptr->mOutputData.remove(key);
}

/**
 * @brief 记录DAWorkFlow
 * @param wf
 */
void DAAbstractNode::registWorkflow(DAWorkFlow* wf)
{
    d_ptr->mWorkflow = wf;
}

/**
 * @brief 记录工厂
 * @param fc
 */
void DAAbstractNode::registFactory(std::shared_ptr< DAAbstractNodeFactory > fc)
{
    d_ptr->mFactory = fc;
}

/**
 * @brief 把当前注册的工作流取消
 */
void DAAbstractNode::unregistWorkflow()
{
    d_ptr->mWorkflow = nullptr;
}
