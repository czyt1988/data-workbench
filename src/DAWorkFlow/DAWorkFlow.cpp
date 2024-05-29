#include "DAWorkFlow.h"
#include <QPointer>
#include <QMap>
#include <QDebug>
#include <QThread>
#include "DAWorkFlowExecuter.h"
#include "DAAbstractNodeFactory.h"
#include "DAQtContainerUtil.hpp"

namespace DA
{
class DAWorkFlow::PrivateData
{
	DA_DECLARE_PUBLIC(DAWorkFlow)
public:
	PrivateData(DAWorkFlow* p);
	void createExecuter();
	//
	void recordNode(DAAbstractNode::SharedPointer& node);

public:
	QHash< DANodeMetaData, std::shared_ptr< DAAbstractNodeFactory > > mMetaToFactory;  ///< 记录prototype对应的工厂
	QHash< QString, std::shared_ptr< DAAbstractNodeFactory > > mFactorys;              ///< 记录所有的工厂
	QList< DAAbstractNode::SharedPointer > mNodes;                                     ///< 节点相关信息列表
	QMap< DAAbstractNode::IdType, DAAbstractNode::SharedPointer > mIdToNode;           ///< 文本信息列表
	DAAbstractNode::WeakPointer mStartNode;                                            ///< 开始执行的节点
	QThread* mExecuterThread { nullptr };                                              ///< 执行器线程
	DA::DAWorkFlowExecuter* mExecuter { nullptr };                                     ///< 执行器
	bool mIsExecuting { false };                                                       ///< 正在执行
	QString mLastErr;                                                                  ///< 记录最后的错误
	QList< DAWorkFlow::CallbackPrepareStartExecute > mPrepareStartCallback;
	QList< DAWorkFlow::CallbackPrepareEndExecute > mPrepareEndCallback;
};

//===================================================
// DAWorkFlowPrivate
//===================================================

DAWorkFlow::PrivateData::PrivateData(DAWorkFlow* p) : q_ptr(p)
{
}

/**
 * @brief 创建执行器
 */
void DAWorkFlow::PrivateData::createExecuter()
{
	if (mExecuterThread) {
		mExecuterThread->quit();  // quit会触发finished，从而进行内存清除
	}
	mExecuterThread = new QThread();
	mExecuter       = new DA::DAWorkFlowExecuter();
	// 设置开始节点
	mExecuter->setStartNode(mStartNode.lock());
	mExecuter->setWorkFlow(q_ptr);
	mExecuter->moveToThread(mExecuterThread);
	QObject::connect(mExecuterThread, &QThread::finished, mExecuter, &QObject::deleteLater);
	QObject::connect(mExecuterThread, &QThread::finished, mExecuterThread, &QObject::deleteLater);
	//
	QObject::connect(q_ptr, &DAWorkFlow::startExecute, mExecuter, &DA::DAWorkFlowExecuter::startExecute);
	QObject::connect(q_ptr, &DAWorkFlow::terminateExecute, mExecuter, &DA::DAWorkFlowExecuter::terminateRequest);
	QObject::connect(mExecuter, &DA::DAWorkFlowExecuter::nodeExecuteFinished, q_ptr, &DAWorkFlow::nodeExecuteFinished);
	QObject::connect(mExecuter, &DA::DAWorkFlowExecuter::finished, q_ptr, &DAWorkFlow::onExecuteFinished);
	// 结束后，线程也结束
	QObject::connect(mExecuter, &DA::DAWorkFlowExecuter::finished, mExecuterThread, &QThread::quit);
	mExecuterThread->start();
}

void DAWorkFlow::PrivateData::recordNode(DAAbstractNode::SharedPointer& node)
{
	mNodes.append(node);
	mIdToNode[ node->getID() ] = node;
}

//==============================================================
// DAWorkFlow
//==============================================================

DAWorkFlow::DAWorkFlow(QObject* p) : QObject(p), DA_PIMPL_CONSTRUCT
{
    qRegisterMetaType< DAAbstractNode::SharedPointer >("DAAbstractNode::SharedPointer");
}

DAWorkFlow::~DAWorkFlow()
{
}

/**
 * @brief 注册工厂，工厂的内存管理根据实例情况管理，目前工作流和执行器会持有工厂的共享指针，如果这两个同时销毁，工厂也会随之销毁
 * @note 最初设计时工作流不保留工厂的所有权，但在实际的使用过程中发现，许多工作流的工厂需要保存这个工厂的全局变量，
 * 如果程序支持多工作流，那么每个工作流需要持有不同的工厂实例，否则会产生冲突，此操作有点违背单一职责原则，但能很好的降低复杂度
 *
 * @note 传入的工厂指针不应该在其他地方进行删除，也不应该在工作流之外的地方保存
 *
 * @param factory
 */
void DAWorkFlow::registFactory(std::shared_ptr< DAAbstractNodeFactory > factory)
{
	QList< DANodeMetaData > mds = factory->getNodesMetaData();

	for (const DANodeMetaData& m : qAsConst(mds)) {
		// qDebug() << "registFactory m=" << m.getNodePrototype();
		d_ptr->mMetaToFactory[ m ] = factory;
	}
	d_ptr->mFactorys[ factory->factoryPrototypes() ] = factory;
	factory->registWorkflow(this);
}

/**
 * @brief 注册工厂群，工作流不保留工程的内存管理权
 * @param factory
 */
void DAWorkFlow::registFactorys(const QList< std::shared_ptr< DAAbstractNodeFactory > > factorys)
{
	for (std::shared_ptr< DAAbstractNodeFactory > f : qAsConst(factorys)) {
		registFactory(f);
	}
}

/**
 * @brief 删除工厂，此函数一般在插件卸载时调用
 * @param fac
 */
void DAWorkFlow::removeFactory(std::shared_ptr< DAAbstractNodeFactory > fac)
{
	// 清除_metaToFactory信息
	auto i = d_ptr->mMetaToFactory.begin();
	while (i != d_ptr->mMetaToFactory.end()) {
		if (i.value() == fac) {
			i = d_ptr->mMetaToFactory.erase(i);
		} else {
			++i;
		}
	}
	// 清除_factorys信息
	auto k = d_ptr->mFactorys.begin();
	while (k != d_ptr->mFactorys.end()) {
		if (k.value() == fac) {
			k = d_ptr->mFactorys.erase(k);
		} else {
			++k;
		}
	}
}

/**
 * @brief 获取所有的工厂
 * @return
 */
QList< std::shared_ptr< DAAbstractNodeFactory > > DAWorkFlow::getAllFactorys() const
{
    return d_ptr->mFactorys.values();
}

/**
 * @brief 获取使用到的工厂
 * @return
 */
QList< std::shared_ptr< DAAbstractNodeFactory > > DAWorkFlow::usedFactorys() const
{
	QList< DAAbstractNode::SharedPointer > ns = nodes();
	// 获取到涉及的工厂
	QSet< std::shared_ptr< DAAbstractNodeFactory > > factorys;
	for (auto n : qAsConst(ns)) {
		factorys.insert(n->factory());
	}
	return qset_to_qlist(factorys);
}

/**
 * @brief 获取工厂的数量
 * @return
 */
int DAWorkFlow::getFactoryCount() const
{
    return d_ptr->mFactorys.size();
}

/**
 * @brief 通过工厂名字获取工厂
 * @param name
 * @return 如果找不到，返回nullptr
 */
std::shared_ptr< DAAbstractNodeFactory > DAWorkFlow::getFactory(const QString& factoryPrototypes)
{
	for (auto f : qAsConst(d_ptr->mFactorys)) {
		if (0 == f->factoryPrototypes().compare(factoryPrototypes, Qt::CaseInsensitive)) {
			return f;
		}
	}
	return nullptr;
}

/**
 * @brief 通过protoType获取DANodeMetaData
 * @param protoType
 * @return
 */
DANodeMetaData DAWorkFlow::getNodeMetaData(const QString& protoType) const
{
	DANodeMetaData data;
	for (auto iter = d_ptr->mMetaToFactory.constBegin(); iter != d_ptr->mMetaToFactory.constEnd(); ++iter) {
		DANodeMetaData data = iter.key();
		if (!data.getNodePrototype().compare(protoType)) {
			return (data);
		}
	}
	return (data);
}

/**
 * @brief 工作流创建节点，FCWorkFlow保留节点的内存管理权
 *
 * 此函数仅仅创建节点，不会添加到工作流中
 * @param md
 * @return
 *
 */
DAAbstractNode::SharedPointer DAWorkFlow::createNode(const DANodeMetaData& md)
{
	DAAbstractNodeFactory::SharedPointer factory = d_ptr->mMetaToFactory.value(md, nullptr);
	if (factory == nullptr) {
		return (nullptr);
	}
	DAAbstractNode::SharedPointer node = factory->create(md);
	if (nullptr == node) {
		return nullptr;
	}
	node->registFactory(factory);
	node->registWorkflow(this);
	// 单一职责原则，不添加
	//     addNode(node);
	return (node);
}

/**
 * @brief 添加节点，会触发nodeAdded信号
 * @param n
 */
void DAWorkFlow::addNode(DAAbstractNode::SharedPointer n)
{
	if (nullptr == n) {
		return;
	}
	if (n->workflow() != this) {
		n->registWorkflow(this);
	}
	DAAbstractNodeFactory::SharedPointer f = n->factory();
	if (f) {
		f->nodeAddedToWorkflow(n);
	}
	d_ptr->recordNode(n);
	emit nodeAdded(n);
}

/**
 * @brief 获取所有节点
 * @return
 */
QList< DAAbstractNode::SharedPointer > DAWorkFlow::nodes() const
{
    return d_ptr->mNodes;
}

/**
 * @brief 清空节点
 * 此函数会触发信号 @sa workflowCleared
 * @note 此函数不会触发信号@sa nodeStartRemove
 */
void DAWorkFlow::clear()
{
	QList< DAAbstractNode::SharedPointer > ns = nodes();
	for (DAAbstractNode::SharedPointer& node : ns) {
		node->unregistWorkflow();
		node->detachAll();
	}
	d_ptr->mNodes.clear();
	d_ptr->mIdToNode.clear();
	emit workflowCleared();
}

/**
 * @brief 删除工作流节点，节点删除会触发@sa nodeStartRemove 信号
 *
 * @note node会调用detachAll函数移除所有依赖
 * @note 此函数会触发@sa DAAbstractNodeFactory::nodeStartRemove ，然后触发@sa DAWorkFlow::nodeStartRemove 信号
 * @param n
 */
void DAWorkFlow::removeNode(const DAAbstractNode::SharedPointer& n)
{
	// 先判断是否有node，没有就跳过
	if (!d_ptr->mNodes.contains(n)) {
		return;
	}
	DAAbstractNodeFactory::SharedPointer f = n->factory();
	if (f) {
		f->nodeStartRemove(n);
	}
	emit nodeStartRemove(n);
	n->detachAll();
	n->unregistWorkflow();
	d_ptr->mNodes.removeAll(n);
	d_ptr->mIdToNode.remove(n->getID());
}

/**
 * @brief 在工作流中是否存在id
 * @param id
 * @return
 */
bool DAWorkFlow::hasNodeID(const DAAbstractNode::IdType id)
{
    return d_ptr->mIdToNode.contains(id);
}

/**
 * @brief 通过id获取节点
 * @param id
 * @return
 */
DAAbstractNode::SharedPointer DAWorkFlow::getNode(const DAAbstractNode::IdType id)
{
    return d_ptr->mIdToNode.value(id, nullptr);
}

/**
 * @brief DAWorkFlow::getNodeGraphicsItem
 * @param id
 * @return
 */
DAAbstractNodeGraphicsItem* DAWorkFlow::getNodeGraphicsItem(const DAAbstractNode::IdType id)
{
	DAAbstractNode::SharedPointer n = getNode(id);
	if (n) {
		return n->graphicsItem();
	}
	return (nullptr);
}

/**
 * @brief 保存工作流的扩展信息
 *
 * @note 工作流保存过程如下：
 * -# 保存工作流扩展信息
 * -# 保存节点信息
 * -# 保存链接信息
 * -# 保存特殊item（非工作流的item）
 * -# 保存工厂扩展信息
 * -# 保存scene信息
 * @param doc
 * @param nodeElement
 */
void DAWorkFlow::saveExternInfoToXml(QDomDocument* doc, QDomElement* nodeElement,const QVersionNumber& ver) const
{
	Q_UNUSED(doc);
	Q_UNUSED(nodeElement);
    Q_UNUSED(ver);
}

/**
 * @brief 加载工作流的扩展信息
 * @note 工作流加载过程如下：
 * -# 加载工作流扩展信息
 * -# 加载节点信息
 * -# 加载链接信息
 * -# 加载特殊item（非工作流的item）
 * -# 加载工厂扩展信息
 * -# 加载scene信息
 * @param nodeElement
 */
void DAWorkFlow::loadExternInfoFromXml(const QDomElement* nodeElement,const QVersionNumber& ver)
{
    Q_UNUSED(nodeElement);
    Q_UNUSED(ver);
}

/**
 * @brief 设置开始执行的节点
 * @param p
 * @note start node并不是必须要指定的，start node的指定可能有利于快速找到开始节点
 */
void DAWorkFlow::setStartNode(DAAbstractNode::SharedPointer p)
{
    d_ptr->mStartNode = p;
}

/**
 * @brief 获取开始执行的节点
 * @return
 */
DAAbstractNode::SharedPointer DAWorkFlow::getStartNode() const
{
    return d_ptr->mStartNode.lock();
}

/**
 * @brief 开始执行
 * 此函数会触发信号@sa startExecuteNode
 */
void DAWorkFlow::exec()
{
	qDebug() << tr("begin exec workflow");
	if (isEmpty()) {
		qWarning() << tr("empty workflow can not exec");  // 无法执行一个空的工作流
		emit finished(false);
		return;
	}

	if (nullptr == d_ptr->mExecuter) {
		d_ptr->createExecuter();
	}
	d_ptr->mIsExecuting = true;
	qDebug() << tr("workflow start run");
	emit startExecute();
}

/**
 * @brief 终止
 */
void DAWorkFlow::terminate()
{
	if (isRunning()) {
		emit terminateExecute();
	}
}

/**
 * @brief 判断是否正在运行
 * @return
 */
bool DAWorkFlow::isRunning() const
{
    return d_ptr->mIsExecuting;
}

/**
 * @brief 工作流中节点的数量
 * @return
 */
int DAWorkFlow::size() const
{
    return d_ptr->mNodes.size();
}

/**
 * @brief 判断是否为空
 * @return
 */
bool DAWorkFlow::isEmpty() const
{
    return d_ptr->mNodes.isEmpty();
}

/**
 * @brief 获取最后发生的错误信息
 * @return
 */
QString DAWorkFlow::getLastErrorString() const
{
    return d_ptr->mLastErr;
}

/**
 * @brief 注册开始执行工作流的回调
 *
 * @note 此回调函数会在DAWorkFlowExecuter里执行，DAWorkFlowExecuter是在其它线程中运行，因此此回调函数是和workflow不在一个线程
 * @param fn
 */
void DAWorkFlow::registStartWorkflowCallback(DAWorkFlow::CallbackPrepareStartExecute fn)
{
    d_ptr->mPrepareStartCallback.append(fn);
}
/**
 * @brief 注册结束执行工作流的回调
 *
 * @note 此回调函数会在DAWorkFlowExecuter里执行，DAWorkFlowExecuter是在其它线程中运行，因此此回调函数是和workflow不在一个线程
 * @param fn
 */
void DAWorkFlow::registEndWorkflowCallback(DAWorkFlow::CallbackPrepareEndExecute fn)
{
    d_ptr->mPrepareEndCallback.append(fn);
}

/**
 * @brief 获取所有的开始回调函数
 * @return
 */
QList< DAWorkFlow::CallbackPrepareStartExecute > DAWorkFlow::getStartWorkflowCallback() const
{
    return d_ptr->mPrepareStartCallback;
}
/**
 * @brief 获取所有的结束回调函数
 * @return
 */
QList< DAWorkFlow::CallbackPrepareEndExecute > DAWorkFlow::getEndWorkflowCallback() const
{
    return d_ptr->mPrepareEndCallback;
}

void DAWorkFlow::emitNodeNameChanged(DAAbstractNode::SharedPointer node, const QString& oldName, const QString& newName)
{
    emit nodeNameChanged(node, oldName, newName);
}

void DAWorkFlow::onExecuteFinished(bool success)
{
	d_ptr->mIsExecuting = false;
	// 无需quit，已经结束了
	//  d_ptr->_executerThread->quit();
	d_ptr->mExecuterThread = nullptr;
	d_ptr->mExecuter       = nullptr;
	emit finished(success);
}
}  // end of namespace DA
