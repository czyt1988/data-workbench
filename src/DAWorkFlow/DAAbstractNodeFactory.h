#ifndef DAABSTRACTNODEFACTORY_H
#define DAABSTRACTNODEFACTORY_H
#include "DAWorkFlowGlobal.h"
#include <QtCore/qglobal.h>
#include <QObject>
#include "DAAbstractNode.h"
class QDomDocument;
class QDomElement;

namespace DA
{

class DAWorkFlow;
class DANodeGraphicsScene;
/**
 * @brief FCAbstractNode的工厂基类，所有自定义的node集合最后都需要提供一个工厂
 *
 * 工厂将通过@sa DANodeMetaData 来创建一个DAAbstractNode,DAAbstractNode可以生成DAAbstractNodeGraphicsItem实现前端的渲染，
 * 因此，任何节点都需要实现一个DAAbstractNode和一个DAAbstractNodeGraphicsItem，一个实现逻辑节点的描述，
 * 一个实现前端的渲染,另外DAAbstractNodeGraphicsItem可以生成DAAbstractNodeWidget，用于设置DAAbstractNodeGraphicsItem
 */
class DAWORKFLOW_API DAAbstractNodeFactory : public QObject, public std::enable_shared_from_this< DAAbstractNodeFactory >
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAAbstractNodeFactory)
public:
	using SharedPointer = std::shared_ptr< DAAbstractNodeFactory >;
	using WeakPointer   = std::weak_ptr< DAAbstractNodeFactory >;

public:
	DAAbstractNodeFactory();
	virtual ~DAAbstractNodeFactory();

	/**
	 * @brief 工厂设置了workflow，此函数设置为虚函数，在某些工厂可以通过此函数的重载来绑定DAWorkFlow的信号
	 * @note 重载此函数一定要调用DAAbstractNodeFactory::registWorkflow,否则@sa getWorkFlow 一直返回空
	 */
	virtual void registWorkflow(DAWorkFlow* wf);

	/**
	 * @brief 获取工作流
	 * @return
	 */
	DAWorkFlow* getWorkFlow() const;

	/**
	 * @brief 返回自身的指针
	 * @return
	 */
	SharedPointer pointer();
	/**
	 * @brief 工厂的唯一标识
	 * @note 每个工厂需要保证有唯一的标识，工作流将通过标识查找工厂
	 * @note 此类型名字不能进行翻译
	 * @return
	 */
	virtual QString factoryPrototypes() const = 0;
	/**
	 * @brief 工厂名称
	 * @note 工厂名称可进行翻译
	 * @return
	 */
	virtual QString factoryName() const = 0;

	/**
	 * @brief 工厂具体描述
	 * @note 工厂具体描述可进行翻译
	 * @return
	 */
	virtual QString factoryDescribe() const = 0;

	/**
	 * @brief 工厂函数，创建一个DAAbstractNode，工厂不持有FCAbstractNode的管理权
	 * @param meta 元对象
	 * @return
	 * @note 此函数会在@sa DAWorkFlow::createNode 中调用，用户不要直接调用此函数，
	 * 因为@sa DAWorkFlow::createNode 中会有其他的操作
	 */
	virtual DAAbstractNode::SharedPointer create(const DANodeMetaData& meta) = 0;

	/**
	 * @brief 初始化节点
	 *
	 * @note 此函数需要用户在create之后调用
	 *
	 * @code
	 * DAAbstractNode::SharedPointer MyFactory::create(const DANodeMetaData& meta){
	 *	...
	 *  DAAbstractNode::SharedPointer node = new xxxNode();
	 *  initializNode(node);
	 *  ...
	 * }
	 *
	 * void MyFactory::initializNode(const DAAbstractNode::SharedPointer& node){
	 *   DAAbstractNodeFactory::initializNode(node);
	 *   ...
	 *   do you initializ
	 * }
	 * @endcode
	 * @param node
	 */
	virtual void initializNode(const DAAbstractNode::SharedPointer& node);
	/**
	 * @brief 获取所有注册的Prototypes
	 * @return
	 */
	virtual QStringList getPrototypes() const = 0;

	/**
	 * @brief 获取所有类型的元数据
	 * @return
	 */
	virtual QList< DANodeMetaData > getNodesMetaData() const = 0;

	/**
	 * @brief 节点加入workflow的回调
	 * 在调用DAWorkFlow::addNode会触发node对应工厂的此函数的回调
	 */
	virtual void nodeAddedToWorkflow(DAAbstractNode::SharedPointer node);

	/**
	 * @brief 节点删除的工厂回调
	 * @param node
	 * @note 此函数会在@sa DAWorkFlow::removeNode 调用时调用
	 */
	virtual void nodeStartRemove(DAAbstractNode::SharedPointer node);

	/**
	 * @brief 节点连线删除的回调
	 * @param outNode 输出节点
	 * @param outKey 输出key
	 * @param intNode 输入节点
	 * @param inkey 输入key
	 */
	virtual void nodeLinkDetached(DAAbstractNode::SharedPointer outNode,
								  const QString& outKey,
								  DAAbstractNode::SharedPointer inNode,
								  const QString& inkey);
	/**
	 * @brief 把扩展信息保存到xml上
	 * 	 * 此函数在工作流保存的过程中会调用，把工厂的附加信息保存到xml文件上
	 * 	 * @note 工作流保存过程如下：
	 * -# 保存工作流扩展信息
	 * -# 保存节点信息
	 * -# 保存链接信息
	 * -# 保存特殊item（非工作流的item）
	 * -# 保存工厂扩展信息
	 * -# 保存scene信息
	 * @param doc
	 * @param factoryExternElement
	 */
	virtual void saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const;
	/**
	 * @brief 加载扩展信息到工厂中
	 * 	 * 此函数会在工作流加载过程中调用，把工厂的特殊信息加载
	 * 	 * @note 工作流加载过程如下：
	 * -# 加载工作流扩展信息
	 * -# 加载节点信息
	 * -# 加载链接信息
	 * -# 加载特殊item（非工作流的item）
	 * -# 加载工厂扩展信息
	 * -# 加载scene信息
	 * 	 * @param factoryExternElement
	 */
	virtual void loadExternInfoFromXml(const QDomElement* factoryExternElement);
	/**
	 * @brief 界面初始化，这个回调发生在工作流和工厂被加入到场景中触发
	 * @param scene
	 * @note 此函数的回调发生DANodeGraphicsScene::setWorkFlow
	 */
	virtual void uiInitialization(DANodeGraphicsScene* scene);
	/**
	 * @brief 工作流准备完成回调
	 * 文件加载过程中不会触发nodeAdded信号，在整个文件加载完成后会触发workflowReady用来通知其他告知工作流加载完成
	 */
	virtual void workflowReady();
};

}  // end DA

#endif  // DAABSTRACTNODEFACTORY_H
