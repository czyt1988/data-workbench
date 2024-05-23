#ifndef DAABSTRACTNODE_H
#define DAABSTRACTNODE_H
#include <memory>
#include <QMetaEnum>
#include "DAWorkFlowGlobal.h"
#include "DANodeMetaData.h"
class QDomElement;
class QDomDocument;

namespace DA
{

class DAAbstractNodeGraphicsItem;
class DAWorkFlow;
class DAAbstractNodeFactory;
class DAAbstractNodeLinkGraphicsItem;
/**
 * @brief 节点对应的基类
 *
 * 节点可以理解为一个函数，函数有多个输入，但只会有一个return或者多个分支的return，
 * 反映到节点中就是可以存在多个输入，但也应该有多个输出（参考流程中的菱形控制（if-else））。
 * 因此节点的输入点是0~n，节点的输出点是0~n。
 *
 * 此节点作为专业数据结构讲，就是图结构（实际workflow是个网结构）的节点，为了适应workflow的称呼，这里做如下约定：
 *
 * - in-degree入度定义为inputKey，inputCount就是入度的权
 * - out-degree出度定义为outputKeyy，outputCount就是出度的权
 * - 每个度都可以赋予不同的值，作为数据投递
 * - 邻接（Adjacent）标示为link，linkto就建立起两个节点的邻接
 *
 * 需要用户实现以下2个函数
 * @code
 * //节点对应的item显示接口，所有node都需要提供一个供前端的显示接口
 * virtual DAAbstractNodeGraphicsItem *createGraphicsItem();
 * //运行
 * virtual bool exec() = 0;
 * @endcode
 *
 * 这个节点和有向无环图（DAG）的不一样，有向无环图是没有节点名称的说法的，这个节点是更通用的节点，
 * 当然，通过限制节点的输入输出名称，也可以实现通用的有向无环图，一个简单的workflow应该使用DAG，
 * 使用带节点名称的图结构会使得workflow过于复杂，就变成类似labview的模式
 *
 * @note 每个节点对应一个@sa DAAbstractNodeGraphicsItem ,可以通过createGraphicsItem创建一个item
 *
 * DAAbstractNode不一定会持有DAAbstractNodeGraphicsItem，但DAAbstractNodeGraphicsItem一定会有对应的node
 *
 */
class DAWORKFLOW_API DAAbstractNode : public std::enable_shared_from_this< DAAbstractNode >
{
	DA_DECLARE_PRIVATE(DAAbstractNode)
	friend class DAAbstractNodeLinkGraphicsItem;
	friend class DAAbstractNodeGraphicsItem;
	friend class DAAbstractNodeFactory;
	friend class DAWorkFlow;

public:
	using SharedPointer = std::shared_ptr< DAAbstractNode >;
	using WeakPointer   = std::weak_ptr< DAAbstractNode >;
	using IdType        = uint64_t;

public:
	/**
	 * @brief 链接信息
	 */
	class LinkInfo
	{
	public:
		LinkInfo();
		QString key;                                                     ///< 针对本节点的链接key
		QList< QPair< QString, DAAbstractNode::SharedPointer > > nodes;  ///< 链接的对象节点，内容为pair，first为对象的key，second为对象节点指针
	};

	/**
	 * @brief 节点类型
	 */
	enum NodeType
	{
		NormalNode,  ///< 普通节点，默认为普通节点
		GlobalNode   ///< 全局节点
	};

public:
	DAAbstractNode();
	virtual ~DAAbstractNode();

	// 获取node的名字
	QString getNodeName() const;
	virtual void setNodeName(const QString& name);

	// 获取node的类型，这个类型可以表征同一类型的node 这个不会进行翻译
	QString getNodePrototype() const;
	// 获取分组
	QString getNodeGroup() const;

	// 获取图标，图标是节点对应的图标
	QIcon getIcon() const;
	void setIcon(const QIcon& icon);

	// 获取节点的元数据
	const DANodeMetaData& metaData() const;
	DANodeMetaData& metaData();

	// 说明
	QString getNodeTooltip() const;
	void setNodeTooltip(const QString& tp);

	// 设置元数据
	void setMetaData(const DANodeMetaData& metadata);

	// 返回自身智能指针
	SharedPointer pointer();
	// id操作
	IdType getID() const;
	void setID(const IdType& d);

	// 属性操作，如果节点有一些额外属性，通过此函数保存才能存入文件系统中
	bool hasProperty(const QString& k) const;
	void setProperty(const QString& k, const QVariant& v);
	QVariant getProperty(const QString& k, const QVariant& defaultVal = QVariant()) const;
	bool removeProperty(const QString& k);
	QList< QString > getPropertyKeys() const;
	// 把扩展信息保存到xml上
	virtual void saveExternInfoToXml(QDomDocument* doc, QDomElement* nodeElement) const;
	// 从xml加载扩展信息
	virtual void loadExternInfoFromXml(const QDomElement* nodeElement);
	// 节点类型默认都为NormalNode
	virtual NodeType nodeType() const;

public:  // 连接相关
	// 获取所有输入的参数名
	QList< QString > getInputKeys() const;
	// 获取输入节点的数量
	int getInputKeysConut() const;
	// 获取所有已经链接上的输入节点
	QList< QString > getLinkedInputKeys() const;
	// 获取所有输出的参数名
	QList< QString > getOutputKeys() const;
	// 获取输出节点的数量
	int getOutputKeysConut() const;
	// 获取所有已经链接上的输出的参数名
	QList< QString > getLinkedOutputKeys() const;
	// 添加一个输入参数
	void addInputKey(const QString& k);
	// 添加一个输出参数
	void addOutputKey(const QString& k);
	// 建立连接,如果基础的对象需要校验，可继承此函数
	virtual bool linkTo(const QString& outKey, SharedPointer inNode, const QString& inKey);
	// 移除连接,节点对应的连接全部解除
	bool detachLink(const QString& key);
	// 移除所有依赖，一般是节点被删除时会调用此函数
	void detachAll();
	// 获取所有连接了输入keys的节点
	QList< SharedPointer > getInputNodes() const;
	QList< SharedPointer > getInputNodes(const QString inputkey) const;
	// 获取此节点输出到其他的节点
	QList< SharedPointer > getOutputNodes() const;
	QList< SharedPointer > getOutputNodes(const QString outputkey) const;
	// 获取输入节点的数量
	int getInputNodesCount() const;
	// 获取输出节点的数量
	int getOutputNodesCount() const;
	// 数据操作
	// 输入参数，in-degree 属性
	void setInputData(const QString& key, const QVariant& dp);
	// 设置输出的参数 out-degree 属性
	void setOutputData(const QString& key, const QVariant& dp);
	// 移除输入
	void removeInputKey(const QString& key);
	// 移除输出
	void removeOutputKey(const QString& key);
	// 获取input的数据包,此函数返回的FCDataPackage是引用，不发生拷贝，修改将直接改变input所维护的FCDataPackage内容
	QVariant getInputData(const QString& key) const;
	// 输出参数
	QVariant getOutputData(const QString& key) const;
	// 获取输入的链接信息
	QList< LinkInfo > getAllInputLinkInfo() const;
	// 获取输出的链接信息
	QList< LinkInfo > getAllOutputLinkInfo() const;
	// 生成一个uint64_t的唯一id
	IdType generateID() const;
	// 获取工作流
	DAWorkFlow* workflow() const;
	// 获取工厂
	std::shared_ptr< DAAbstractNodeFactory > factory() const;
	//

public:
	// 执行
	virtual bool exec() = 0;

public:
	// 节点对应的item显示接口，所有node都需要提供一个供前端的显示接口
	virtual DAAbstractNodeGraphicsItem* createGraphicsItem() = 0;

	// 获取item
	DAAbstractNodeGraphicsItem* graphicsItem() const;

protected:
	// 记录item，此函数在DAAbstractNodeGraphicsItem构造函数中调用
	void registItem(DAAbstractNodeGraphicsItem* it);
	// 解除对item的记录
	void unregistItem();
	// linkTo的实现
	bool linkTo_(const QString& outKey, SharedPointer inNode, const QString& inKey);

protected:
	// 注册工作流
	void registWorkflow(DAWorkFlow* wf);
	void registFactory(std::shared_ptr< DAAbstractNodeFactory > fc);
	void unregistWorkflow();
};

}  // end DA
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
/**
 * @brief qHash
 * @param ptr
 * @param seed
 * @return
 */
uint qHash(const std::shared_ptr< DA::DAAbstractNode >& ptr, uint seed)
{
    return qHash(ptr.get(), seed);
}
#endif
#endif  // DAABSTRACTNODE_H
