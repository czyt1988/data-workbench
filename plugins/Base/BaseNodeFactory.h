#ifndef BASENODEFACTORY_H
#define BASENODEFACTORY_H
#include "BaseGlobal.h"
#include "DAAbstractNodeFactory.h"
#include <QMap>
class QMainWindow;
namespace DA
{
class DACoreInterface;
class DAWorkFlowEditWidget;
class DAAbstractNodeLinkGraphicsItem;
class DAWorkFlowGraphicsScene;
}

class BaseNodeFactory : public DA::DAAbstractNodeFactory
{
	Q_OBJECT
public:
	using FpCreate = std::function< DA::DAAbstractNode::SharedPointer(void) >;

public:
	BaseNodeFactory();
	virtual ~BaseNodeFactory() override;
	// 设置core
	void setCore(DA::DACoreInterface* c);

public:
	// 工厂设置了workflow，此函数设置为虚函数，在某些工厂可以通过此函数的重载来绑定DAWorkFlow的信号,以及注册回调
	//  此函数同样可以作为scene创建的回调
	virtual void registWorkflow(DA::DAWorkFlow* wf) override;

	/**
	 * @brief 工厂的唯一标识
	 * @note 每个工厂需要保证有唯一的标识，工作流将通过标识查找工厂
	 * @note 此类型名字不能进行翻译
	 * @return
	 */
	virtual QString factoryPrototypes() const override;
	/**
	 * @brief 工厂名称
	 * @note 工厂名称可进行翻译
	 * @return
	 */
	virtual QString factoryName() const override;

	/**
	 * @brief 工厂具体描述
	 * @note 工厂具体描述可进行翻译
	 * @return
	 */
	virtual QString factoryDescribe() const override;

	/**
	 * @brief 工厂函数，创建一个DAAbstractNode，工厂不持有FCAbstractNode的管理权
	 * @param meta 元对象
	 * @return
	 * @note 此函数会在@sa DAWorkFlow::createNode 中调用，用户不要直接调用此函数，
	 * 因为@sa DAWorkFlow::createNode 中会有其他的操作
	 */
	virtual DA::DAAbstractNode::SharedPointer create(const DA::DANodeMetaData& meta) override;

	/**
	 * @brief 获取所有注册的Prototypes
	 * @return
	 */
	virtual QStringList getPrototypes() const override;

	/**
	 * @brief 获取所有类型的元数据
	 * @return
	 */
	virtual QList< DA::DANodeMetaData > getNodesMetaData() const override;

	// 节点加入workflow的回调
	virtual void nodeAddedToWorkflow(DA::DAAbstractNode::SharedPointer node) override;

	// 节点删除的工厂回调
	virtual void nodeStartRemove(DA::DAAbstractNode::SharedPointer node) override;

	// 节点连线删除的回调
	virtual void nodeLinkDetached(DA::DAAbstractNode::SharedPointer outNode,
								  const QString& outKey,
								  DA::DAAbstractNode::SharedPointer inNode,
								  const QString& inkey) override;
	// 把扩展信息保存到xml上
	virtual void saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const override;
	// 从xml加载扩展信息
	virtual void loadExternInfoFromXml(const QDomElement* factoryExternElement) override;

public:
	// 窗口相关操作
	//  获取主体窗口
	QMainWindow* getMainWindow() const;

private:
	DA::DACoreInterface* mCore { nullptr };
	QMap< DA::DANodeMetaData, FpCreate > mPrototypeTpfp;
};

#endif  // BASENODEFACTORY_H
