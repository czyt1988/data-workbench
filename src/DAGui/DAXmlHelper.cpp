#include "DAXmlHelper.h"
#include <QElapsedTimer>
#include <QObject>
#include <QDebug>
// Qt
#include <QBuffer>
#include <QDomDocument>
#include <QFile>
#include <QScopedPointer>
#include <QVariant>
#include <QPen>
#include <QSet>
// DA
#include "DACommandsForWorkFlowNodeGraphics.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DAGraphicsPixmapItem.h"
#include "DAGraphicsItemFactory.h"
#include "DAAbstractNodeFactory.h"
#include "DAGraphicsItem.h"
#include "DAXMLFileInterface.h"
#include "DAStringUtil.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAWorkFlowEditWidget.h"
#include "DAQtContainerUtil.hpp"
namespace DA
{
//==============================================================
// DAXmlHelperPrivate
//==============================================================
class DAXmlHelperPrivate
{
	DA_IMPL_PUBLIC(DAXmlHelper)
public:
	DAXmlHelperPrivate(DAXmlHelper* p);

public:
	// 保存工作流
	void saveWorkflow(DAWorkFlowEditWidget* wfe, QDomDocument& doc, QDomElement& workflowEle);
	bool loadWorkflow(DAWorkFlowEditWidget* wfe, const QDomElement& workflowEle);
	// copy type类型
	void saveWorkflowFromClipBoard(const QList< DAGraphicsItem* > its, QDomDocument& doc, QDomElement& workflowEle);
	bool loadWorkflowFromClipBoard(DAWorkFlowGraphicsScene* scene, const QDomElement& workflowEle, bool isCreateNewId = true);
	// 保存工厂相关的扩展信息
	void saveFactoryInfo(const DAWorkFlow* workflow, QDomDocument& doc, QDomElement& workflowEle);
	bool loadFactoryInfo(DAWorkFlow* workflow, const QDomElement& workflowEle);
	// 保存工作流的节点
	void saveNodes(const DAWorkFlowEditWidget* wfe, QDomDocument& doc, QDomElement& workflowEle);
	QDomElement makeNodesElement(const QList< DAAbstractNode::SharedPointer >& nodes,
                                 const QString& tagName,
                                 QDomDocument& doc);
	QDomElement makeNodeElement(const DAAbstractNode::SharedPointer& node, const QString& tagName, QDomDocument& doc);
	bool loadNodes(DAWorkFlow* workflow, DAWorkFlowGraphicsScene* workFlowScene, const QDomElement& workflowEle);
	bool loadNodesClipBoard(DAWorkFlowGraphicsScene* scene,
                            const QDomElement& workflowEle,
                            QMap< qulonglong, qulonglong >* idMap);

	[[deprecated("This function is deprecated. Use loadNodeAndItem() instead.")]]
	DAAbstractNode::SharedPointer loadNode(const QDomElement& nodeEle, DAWorkFlow* workflow, bool isLoadID = true);
	DAAbstractNodeGraphicsItem* loadNodeAndItem(const QDomElement& nodeEle, DAWorkFlowGraphicsScene* workFlowScene);
	// 这种是针对需要redo/undo的加载节点
	DAAbstractNodeGraphicsItem* loadNodeAndItemWithUndo(const QDomElement& nodeEle,
                                                        DAWorkFlowGraphicsScene* workFlowScene,
                                                        QMap< qulonglong, qulonglong >* idMap);
	// 保存输入输出
	void saveNodeInputOutput(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle);
	bool loadNodeInPutOutputKey(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode);
	bool loadNodeInPutOutputKey_v110(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode);
	bool loadNodeInPutOutputKey_v130(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode);
	// 保存属性
	void saveNodePropertys(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle);
	bool loadNodePropertys(DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle);
	bool loadNodePropertys_v110(DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle);
	// 保存item
	void saveNodeItem(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle);
	[[deprecated("This function is deprecated. Use loadNodeAndItem() instead.")]]
	DAAbstractNodeGraphicsItem* loadNodeItem(DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle);
	// 保存链接
	void saveNodeLinks(const DAWorkFlow* workflow, QDomDocument& doc, QDomElement& workflowEle);
	QDomElement makeNodeLinkElement(DAAbstractNodeLinkGraphicsItem* link, const QString& tagName, QDomDocument& doc);
	bool loadNodeLinks(DAWorkFlowGraphicsScene* scene, DAWorkFlow* wf, const QDomElement& workflowEle);
	bool loadNodeLinksClipBoardCopy(DAWorkFlowGraphicsScene* scene,
                                    const QDomElement& workflowEle,
                                    const QMap< qulonglong, qulonglong >* idMap);
	// 保存特殊的item，主要为文本
	void saveCommonItems(const DAWorkFlowGraphicsScene* scene, QDomDocument& doc, QDomElement& workflowEle);
	QDomElement makeCommonItemsElement(const QList< QGraphicsItem* >& items, const QString& tagName, QDomDocument& doc);
	bool loadCommonItems(DAWorkFlowGraphicsScene* scene, const QDomElement& workflowEle, bool isRedoUndo = false);
	// 保存一个item
	bool saveItem(const QGraphicsItem* i, QDomDocument& doc, QDomElement& parentElement);
	QGraphicsItem* loadItem(const QDomElement& itemElement);
	bool loadItem(QGraphicsItem* item, const QDomElement& itemElement);
	QDomElement findItemElement(const QDomElement& parentElement);
	// 保存和加载分组
	void saveItemGroup(const DAGraphicsItemGroup* itemGroup, QDomDocument& doc, QDomElement& parentElement);
	DAGraphicsItemGroup* loadItemGroup(DAWorkFlowGraphicsScene* scene, const QDomElement& groupElement);
	// SecenInfo
	void saveSecenInfo(DAWorkFlowGraphicsScene* scene, QDomDocument& doc, QDomElement& workflowEle);
	bool loadSecenInfo(DAWorkFlowGraphicsScene* scene, const QDomElement& workflowEle);
	// 保存属性
	void savePropertys(const QHash< QString, QVariant >& props, QDomDocument& doc, QDomElement& parentEle);
	bool loadPropertys(QHash< QString, QVariant >& props, const QDomElement& parentEle);
	// 清空处理列表
	void clearDealItemSet();
	void recordDealItem(QGraphicsItem* i);
	void recordDealItem(const QGraphicsItem* i);
	bool isItemHaveDeal(QGraphicsItem* i) const;
	bool isItemHaveDeal(const QGraphicsItem* i) const;

public:
	QSet< QGraphicsItem* > mHaveBeenDealNodeItem;    ///< 记录已经被处理过的item
	QVersionNumber mLoadedVersion;                   ///< 不同版本号有不同解析方式
	QMap< qulonglong, qulonglong > mPasteNodeIdMap;  ///< 记录粘贴时，旧id和新id的映射
};

DAXmlHelperPrivate::DAXmlHelperPrivate(DAXmlHelper* p) : q_ptr(p)
{
}

void DAXmlHelperPrivate::saveWorkflow(DAWorkFlowEditWidget* wfe, QDomDocument& doc, QDomElement& workflowEle)
{
	// 保存工作流的扩展信息
	QElapsedTimer tes;
	tes.start();
	clearDealItemSet();  // 清空保存过的item的记录
	QDomElement externEle                  = doc.createElement("extern");
	DAWorkFlow* workflow                   = wfe->getWorkflow();
	DAWorkFlowGraphicsScene* workFlowScene = wfe->getWorkFlowGraphicsScene();
	// 保存开始，设置场景没有就绪
	workFlowScene->setReady(false);
	workflow->saveExternInfoToXml(&doc, &externEle, DAXmlHelper::getCurrentVersionNumber());
	workflowEle.appendChild(externEle);
	qDebug() << QObject::tr("save workflow extern info cost: %1 ms").arg(tes.restart());
	// 保存所有节点
	saveNodes(wfe, doc, workflowEle);
	qDebug() << QObject::tr("save workflow nodes cost: %1 ms").arg(tes.restart());
	// 保存所有连接
	saveNodeLinks(workflow, doc, workflowEle);
	qDebug() << QObject::tr("save workflow links cost: %1 ms").arg(tes.restart());
	// 保存特殊的item。例如文本
	saveCommonItems(workFlowScene, doc, workflowEle);
	qDebug() << QObject::tr("save special item cost: %1 ms").arg(tes.restart());
	// 保存工厂相关信息，包括工厂的扩展信息，工厂的信息一般在node和link之后保存
	saveFactoryInfo(workflow, doc, workflowEle);
	qDebug() << QObject::tr("save workflow factory info cost: %1 ms").arg(tes.restart());
	// 保存scene信息
	saveSecenInfo(workFlowScene, doc, workflowEle);
	qDebug() << QObject::tr("save secen info cost: %1 ms").arg(tes.restart());
	// 保存完成，设置场景没有就绪
	workFlowScene->setReady(true);
}

/**
 * @brief 加载工作流相关信息
 * @param workflow
 * @param workflowEle
 * @return
 */
bool DAXmlHelperPrivate::loadWorkflow(DAWorkFlowEditWidget* wfe, const QDomElement& workflowEle)
{  // 加载扩展信息
	QElapsedTimer tes;
	tes.start();
	DAWorkFlow* workflow                   = wfe->getWorkflow();
	DAWorkFlowGraphicsScene* workFlowScene = wfe->getWorkFlowGraphicsScene();
	// 加载开始，设置场景没有就绪
    // 一定要设置disableFactoryCallBack，否则在加载过程会 一直触发回调，加载过程会很慢，加载过程通过最后的ready信号进行触发
	workflow->disableFactoryCallBack();
	workFlowScene->setReady(false);
	//
	clearDealItemSet();  // 清空保存过的item的记录
	QDomElement externEle = workflowEle.firstChildElement("extern");
	if (!externEle.isNull()) {
		workflow->loadExternInfoFromXml(&externEle, mLoadedVersion);
		qDebug() << QObject::tr("load workflow extern info cost: %1 ms").arg(tes.restart());
	}
	// 从文件加载，必须第四个参数为true
	if (!loadNodes(workflow, workFlowScene, workflowEle)) {
		qCritical() << QObject::tr("load nodes occurce error");
	}
	qDebug() << QObject::tr("load workflow nodes cost: %1 ms").arg(tes.restart());

	if (!loadNodeLinks(workFlowScene, workflow, workflowEle)) {
		qCritical() << QObject::tr("load nodes link occurce error");
	}
	qDebug() << QObject::tr("load workflow links cost: %1 ms").arg(tes.restart());
	// false代表不进行回退操作
	if (!loadCommonItems(workFlowScene, workflowEle, false)) {
		qCritical() << QObject::tr("load special item occurce error");
	}
	qDebug() << QObject::tr("load special item cost: %1 ms").arg(tes.restart());
	// 加载工厂
	//! 注意，工厂的加载要在节点和连接之后，工厂一般维护着全局的数据，因此
	//! 额外信息一般是在节点和连接都完成后进行加载
	if (!loadFactoryInfo(workflow, workflowEle)) {
		qCritical() << QObject::tr("load factorys occurce error");
	}
	qDebug() << QObject::tr("load workflow factory info cost: %1 ms").arg(tes.restart());

	if (!loadSecenInfo(workFlowScene, workflowEle)) {
		qCritical() << QObject::tr("load scene info occurce error");
	}
	qDebug() << QObject::tr("load secen info cost: %1 ms").arg(tes.restart());

	// 加载完成，设置场景没有就绪
	workFlowScene->setReady(true);
    // 加载完成后开启回调
	workflow->enableFactoryCallBack();
	workflow->callWorkflowReady();
	return true;
}

void DAXmlHelperPrivate::saveWorkflowFromClipBoard(const QList< DAGraphicsItem* > its, QDomDocument& doc, QDomElement& workflowEle)
{
	//! 1. 对图元节点进行分类
	QList< DAAbstractNodeGraphicsItem* > nodeItems;
	QList< DAAbstractNodeLinkGraphicsItem* > linkItems;
	QList< QGraphicsItem* > otherItems;  // 分组等其它的items
	for (DAGraphicsItem* i : its) {
		if (DAAbstractNodeGraphicsItem* ni = dynamic_cast< DAAbstractNodeGraphicsItem* >(i)) {
			nodeItems.append(ni);
		} else if (DAAbstractNodeLinkGraphicsItem* li = dynamic_cast< DAAbstractNodeLinkGraphicsItem* >(i)) {
			linkItems.append(li);
		} else {
			otherItems.append(i);
		}
	}
	//! 2.把工作流节点提取出来
	QList< DAAbstractNode::SharedPointer > nodes;
	for (DAAbstractNodeGraphicsItem* i : qAsConst(nodeItems)) {
		auto n = i->node();
		if (n) {
			nodes.append(n);
		}
	}
	//! 3.把涉及到的链接保存，有些情况，选中的链接不完全，
	//! 例如只选中了入口节点或者只选中了出口节点，这种链接是不能要的，不进行复制
	QSet< DAAbstractNodeGraphicsItem* > nodeItemsSet(nodeItems.begin(), nodeItems.end());
	for (auto i = linkItems.begin(); i != linkItems.end();) {
		if (nodeItemsSet.contains((*i)->fromNodeItem()) && nodeItemsSet.contains((*i)->toNodeItem())) {
			// 完整链接，要保留
			++i;
		} else {
			i = linkItems.erase(i);
		}
	}
	//! 4.先把记忆清空
	clearDealItemSet();  // 清空保存过的item的记录
                         //! 把工作流的总体范围保存
	QRectF sceneRange = DAWorkFlowEditWidget::calcAllItemsSceneRange(DAWorkFlowEditWidget::cast(its));
	workflowEle.setAttribute("sc-x", sceneRange.x());
	workflowEle.setAttribute("sc-y", sceneRange.y());
	workflowEle.setAttribute("sc-w", sceneRange.width());
	workflowEle.setAttribute("sc-h", sceneRange.height());
	//! 5.先保存工作流
	QDomElement nodesEle = makeNodesElement(nodes, QStringLiteral("nodes"), doc);
	workflowEle.appendChild(nodesEle);
	//! 6.保存链接
	QDomElement linksEle = doc.createElement(QStringLiteral("links"));
	for (auto li : qAsConst(linkItems)) {
		QDomElement linkEle = makeNodeLinkElement(li, QStringLiteral("link"), doc);
		linksEle.appendChild(linkEle);
	}
	workflowEle.appendChild(linksEle);
	//!7.保存其它
	QDomElement itemsElement = makeCommonItemsElement(otherItems, QStringLiteral("items"), doc);
	workflowEle.appendChild(itemsElement);
}

/**
 * @brief 加载剪切板的内容
 * @param wfe
 * @param workflowEle
 * @param isCreateNewId 是否创建新id，对于复制(copy),这个值应该为true，
 * 因为复制过程原来的元素还在，对于剪切(cut)，这个值应该为false,因为剪切原来的节点已经删除，
 * 需要加载回原来的节点内容
 * @return
 */
bool DAXmlHelperPrivate::loadWorkflowFromClipBoard(DAWorkFlowGraphicsScene* scene,
                                                   const QDomElement& workflowEle,
                                                   bool isCreateNewId)
{
	clearDealItemSet();  // 清空保存过的item的记录
	// 计算当前view的中心点
	// 加载开始，设置场景没有就绪
	scene->setReady(false);
	auto wf = scene->getWorkflow();
    // 一定要设置disableFactoryCallBack，否则在加载过程会 一直触发回调，加载过程会很慢，加载过程通过最后的ready信号进行触发
	wf->disableFactoryCallBack();
	scene->getUndoStack()->beginMacro(QObject::tr("Load Nodes"));  // cn:加载节点
	std::unique_ptr< QMap< qulonglong, qulonglong > > idMaps;
	if (isCreateNewId) {
		idMaps = std::make_unique< QMap< qulonglong, qulonglong > >();
	}
	if (!loadNodesClipBoard(scene, workflowEle, idMaps.get())) {
		qCritical() << QObject::tr("load nodes occurce error");
	}
	if (!loadNodeLinksClipBoardCopy(scene, workflowEle, idMaps.get())) {
		qCritical() << QObject::tr("load nodes link occurce error");
	}
	// 加载其它
	// 第三个参数为true代表可以回退
	if (!loadCommonItems(scene, workflowEle, true)) {
		qCritical() << QObject::tr("load items occurce error");
	}
	scene->getUndoStack()->endMacro();
	scene->setReady(true);
	wf->enableFactoryCallBack();
	wf->callWorkflowReady();
	return true;
}

/**
 * @brief 保存工厂相关的扩展信息
 * @param workflow
 * @param doc
 * @param workflowEle
 */
void DAXmlHelperPrivate::saveFactoryInfo(const DAWorkFlow* workflow, QDomDocument& doc, QDomElement& workflowEle)
{
	const QList< std::shared_ptr< DAAbstractNodeFactory > > factorys = workflow->getAllFactorys();
	// 创建节点
	QDomElement factorysEle = doc.createElement("factorys");
	for (const auto& fac : factorys) {
		QDomElement factoryEle = doc.createElement("factory");
		factoryEle.setAttribute("prototypes", fac->factoryPrototypes());
		QDomElement externEle = doc.createElement("extern");
		fac->saveExternInfoToXml(&doc, &externEle);  // 保存扩展信息
		factoryEle.appendChild(externEle);
		factorysEle.appendChild(factoryEle);
	}
	workflowEle.appendChild(factorysEle);
}
/**
 * @brief 加载工厂相关的扩展信息
 * @param workflow
 * @param workflowEle
 * @return
 */
bool DAXmlHelperPrivate::loadFactoryInfo(DAWorkFlow* workflow, const QDomElement& workflowEle)
{
	QDomElement factorysEle     = workflowEle.firstChildElement("factorys");
	QDomNodeList factoryEleList = factorysEle.childNodes();

	for (int i = 0; i < factoryEleList.size(); ++i) {
		QDomElement factoryEle = factoryEleList.at(i).toElement();
		if (factoryEle.tagName() != "factory") {
			qWarning() << QObject::tr("find unknow tag <%1> under <factorys> element").arg(factoryEle.tagName());
			continue;
		}
		QString factoryPrototypes                    = factoryEle.attribute("prototypes");
		std::shared_ptr< DAAbstractNodeFactory > fac = workflow->getFactory(factoryPrototypes);
		if (fac == nullptr) {
			qCritical() << QObject::tr("can not find factory prototypes = %1").arg(factoryPrototypes);
			continue;
		}
		QDomElement externEle = factoryEle.firstChildElement("extern");
		fac->loadExternInfoFromXml(&externEle);  // 加载工厂的信息
	}
	return true;
}

void DAXmlHelperPrivate::saveNodes(const DAWorkFlowEditWidget* wfe, QDomDocument& doc, QDomElement& workflowEle)
{
	DAWorkFlow* workflow                                = wfe->getWorkflow();
	const QList< DAAbstractNode::SharedPointer >& nodes = workflow->nodes();
	QDomElement nodesEle                                = makeNodesElement(nodes, QStringLiteral("nodes"), doc);
	workflowEle.appendChild(nodesEle);
}

/**
 * @brief 创建nodes节点
 *
 * @note 此函数会调用makeNodeElement(含saveItem,会把nodeitem写入mHaveBeenSaveNodeItem中)
 * @param nodes
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXmlHelperPrivate::makeNodesElement(const QList< DAAbstractNode::SharedPointer >& nodes,
                                                 const QString& tagName,
                                                 QDomDocument& doc)
{
	QDomElement nodesEle = doc.createElement(tagName);
	for (const DAAbstractNode::SharedPointer& node : nodes) {
		QDomElement nodeEle = makeNodeElement(node, QStringLiteral("node"), doc);
		nodesEle.appendChild(nodeEle);
	}
	return nodesEle;
}

/**
 * @brief 创建node节点
 *
 * @note 此函数会调用saveItem,会把linkitem写入mHaveBeenSaveNodeItem中
 * @param node
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXmlHelperPrivate::makeNodeElement(const DAAbstractNode::SharedPointer& node,
                                                const QString& tagName,
                                                QDomDocument& doc)
{
	QDomElement nodeEle = doc.createElement(tagName);
	nodeEle.setAttribute("id", node->getID());
	nodeEle.setAttribute("name", node->getNodeName());
	nodeEle.setAttribute("protoType", node->getNodePrototype());

	// 保存节点input和output的key和propertys
	saveNodeInputOutput(node, doc, nodeEle);
	// 保存节点属性
	saveNodePropertys(node, doc, nodeEle);
	// 保存额外信息
	node->saveExternInfoToXml(&doc, &nodeEle, DAXmlHelper::getCurrentVersionNumber());
	// 添加节点Item信息
	saveNodeItem(node, doc, nodeEle);
	return nodeEle;
}

/**
 * @brief 加载节点
 * @param workflow
 * @param workFlowScene
 * @param workflowEle
 * @param isLoadID 是否要用保存的id覆盖新节点的id，对于文件加载，这个是true，对于复制粘贴，这个应该为false
 * @param isNeedUndo 是否需要undo,对于文件加载，这个是false，对于复制粘贴，这个是ture
 * @return
 */
bool DAXmlHelperPrivate::loadNodes(DAWorkFlow* workflow, DAWorkFlowGraphicsScene* workFlowScene, const QDomElement& workflowEle)
{
	QDomElement nodesEle   = workflowEle.firstChildElement("nodes");
	QDomNodeList nodeslist = nodesEle.childNodes();

	for (int i = 0; i < nodeslist.size(); ++i) {
		QDomElement nodeEle = nodeslist.at(i).toElement();
		if (nodeEle.tagName() != "node") {
			qDebug() << "nodeEle.tagName()=" << nodeEle.tagName() << ",skip and continue";
			continue;
		}
#if 1
		loadNodeAndItem(nodeEle, workFlowScene);
#else
		DAAbstractNode::SharedPointer node = loadNode(nodeEle, workflow, true);
		if (!node) {
			// 异常提示在loadNode函数中已经有，不用重复提示
			continue;
		}
		// 加载item
		DAAbstractNodeGraphicsItem* item = loadNodeItem(node, nodeEle);
		if (!item) {
			continue;
		}
		workFlowScene->addItem(item);
		workflow->addNode(node);
		// 最后再添加
#endif
	}
	return true;
}

/**
 * @brief 加载剪切板的节点
 * @param workflow
 * @param workFlowScene
 * @param workflowEle
 * @param idMap
 * @param currentViewsceneRect 当前视图的显示区域，用来进行偏移
 * @return
 */
bool DAXmlHelperPrivate::loadNodesClipBoard(DAWorkFlowGraphicsScene* scene,
                                            const QDomElement& workflowEle,
                                            QMap< qulonglong, qulonglong >* idMap)
{
	QDomElement nodesEle = workflowEle.firstChildElement("nodes");
	//! 加载原来的scenet区域
	QRectF sceneRange;
	sceneRange.setX(workflowEle.attribute("sc-x").toDouble());
	sceneRange.setY(workflowEle.attribute("sc-y").toDouble());
	sceneRange.setWidth(workflowEle.attribute("sc-w").toDouble());
	sceneRange.setHeight(workflowEle.attribute("sc-h").toDouble());
	//! 计算sceneRange和currentViewsceneRect的中心偏移
	QDomNodeList nodeslist = nodesEle.childNodes();

	for (int i = 0; i < nodeslist.size(); ++i) {
		QDomElement nodeEle = nodeslist.at(i).toElement();
		if (nodeEle.tagName() != "node") {
			qDebug() << "nodeEle.tagName()=" << nodeEle.tagName() << ",skip and continue";
			continue;
		}
		loadNodeAndItemWithUndo(nodeEle, scene, idMap);
	}
	return true;
}

/**
 * @brief 加载节点
 * @note 此函数workflow不会把节点添加进去，需要手动添加
 * @param nodeEle
 * @param workflow
 * @param isLoadID 是否把保存的id也设置回来，有些情况是不要设置id的，如复制粘贴
 * @return
 */
DAAbstractNode::SharedPointer DAXmlHelperPrivate::loadNode(const QDomElement& nodeEle, DAWorkFlow* workflow, bool isLoadID)
{
	bool isok     = false;
	qulonglong id = nodeEle.attribute("id").toULongLong(&isok);
	if (!isok) {
		qWarning() << QObject::tr("node's id=%1 can not conver to qulonglong type ,will skip this node")
                          .arg(nodeEle.attribute("id"));
		return nullptr;
	}
	if (workflow->hasNodeID(id)) {
		qDebug() << "duplicate node id";
		return nullptr;
	}
	QString name      = nodeEle.attribute("name");
	QString protoType = nodeEle.attribute("protoType");
	// 创建节点
	DANodeMetaData metadata            = workflow->getNodeMetaData(protoType);
	DAAbstractNode::SharedPointer node = workflow->createNode(metadata);
	if (nullptr == node) {
		qWarning() << QObject::tr("Unable to create node by metadata(prototype=%1,name=%2,group=%3)-0")
                          .arg(metadata.getNodePrototype(),
                               metadata.getNodeName(),
                               metadata.getGroup());  // cn:无法通过元数据创建节点(类型=%1,名称=%2,分组=%3)创建节点-0
		return nullptr;
	}
	// 这里是要把保存的id加载，有些情况是不要设置id的，如复制粘贴
	if (isLoadID) {
		node->setID(id);
	}
	node->setNodeName(name);
	// 加载节点的输入输出
	loadNodeInPutOutputKey(node, nodeEle);
	// 加载节点的属性
	if (mLoadedVersion.majorVersion() == 1 && mLoadedVersion.minorVersion() < 4) {
		loadNodePropertys_v110(node, nodeEle);
	} else {
		loadNodePropertys(node, nodeEle);
	}
	// 加载额外信息
	node->loadExternInfoFromXml(&nodeEle, mLoadedVersion);
	return node;
}

DAAbstractNodeGraphicsItem* DAXmlHelperPrivate::loadNodeAndItem(const QDomElement& nodeEle,
                                                                DAWorkFlowGraphicsScene* workFlowScene)
{
	DAWorkFlow* workflow = workFlowScene->getWorkflow();
	if (!workflow) {
		return nullptr;
	}
	bool isok = false;
	// 这个id暂时要记录下来
	qulonglong id = nodeEle.attribute("id").toULongLong(&isok);
	if (!isok) {
		qWarning() << QObject::tr("node's id=%1 can not conver to qulonglong type ,will skip this node")
                          .arg(nodeEle.attribute("id"));
		return nullptr;
	}
	if (workflow->hasNodeID(id)) {
		qDebug() << "duplicate node id";
		return nullptr;
	}
	QString name      = nodeEle.attribute("name");
	QString protoType = nodeEle.attribute("protoType");
	// 创建节点
	DANodeMetaData metadata = workflow->getNodeMetaData(protoType);
	// 注意，这里时createNode，不是createNode_
	std::unique_ptr< DAAbstractNodeGraphicsItem > item(workFlowScene->createNode(metadata, QPoint(0, 0)));
	if (!item) {
		qWarning() << QObject::tr("Unable to create node by metadata(prototype=%1,name=%2,group=%3)-1")
                          .arg(metadata.getNodePrototype(),
                               metadata.getNodeName(),
                               metadata.getGroup());  // cn:无法通过元数据创建节点(类型=%1,名称=%2,分组=%3)创建节点-1
		return nullptr;
	}
	DAAbstractNode::SharedPointer node = item->node();
	node->setID(id);
	if (nullptr == node) {
		qWarning() << QObject::tr("Unable to create node by metadata(prototype=%1,name=%2,group=%3)-2")
                          .arg(metadata.getNodePrototype(),
                               metadata.getNodeName(),
                               metadata.getGroup());  // cn:无法通过元数据创建节点(类型=%1,名称=%2,分组=%3)创建节点-2
		return nullptr;
	}
	node->setNodeName(name);
	// 加载节点的输入输出
	loadNodeInPutOutputKey(node, nodeEle);
	// 加载节点的属性
	if (mLoadedVersion.majorVersion() == 1 && mLoadedVersion.minorVersion() < 4) {
		loadNodePropertys_v110(node, nodeEle);
	} else {
		loadNodePropertys(node, nodeEle);
	}
	// 加载额外信息
	node->loadExternInfoFromXml(&nodeEle, mLoadedVersion);
	// 加载item信息
	QDomElement itemEle = nodeEle.firstChildElement("item");
	if (itemEle.isNull()) {
		qWarning() << QObject::tr("can not find <item> tag under <node> tag");  // cn:无法在<node>标签下查询到<item>标签
		return nullptr;
	}
	workFlowScene->addItem(item.get());
	// 由于node和item的信息加载后，node又加载了其他信息，例如input、output这些信息，加载后需要通知item进行刷新，否则item和node不同步
	item->resetLinkPoint();
	loadItem(item.get(), itemEle);
	return item.release();
}

/**
 * @brief 加载节点，此过程是可以回退的
 *
 * @note 此过程加载的节点将赋予新的id，并且把旧id和新id的关系保存入idMap中
 * @param nodeEle
 * @param workFlowScene
 * @param idMap 如果idmap为空，则代表使用原来的id，如果不为空，代表使用新id
 * @return
 */
DAAbstractNodeGraphicsItem* DAXmlHelperPrivate::loadNodeAndItemWithUndo(const QDomElement& nodeEle,
                                                                        DAWorkFlowGraphicsScene* workFlowScene,
                                                                        QMap< qulonglong, qulonglong >* idMap)
{
	DAWorkFlow* workflow = workFlowScene->getWorkflow();
	if (!workflow) {
		return nullptr;
	}
	bool isok = false;
	// 这个id暂时要记录下来
	qulonglong id = nodeEle.attribute("id").toULongLong(&isok);
	if (!isok) {
		qWarning() << QObject::tr("node's id=%1 can not conver to qulonglong type ,will skip this node")
                          .arg(nodeEle.attribute("id"));
		return nullptr;
	}
	QString name      = nodeEle.attribute("name");
	QString protoType = nodeEle.attribute("protoType");
	// 创建节点
	DANodeMetaData metadata          = workflow->getNodeMetaData(protoType);
	DAAbstractNodeGraphicsItem* item = workFlowScene->createNode_(metadata, QPoint(0, 0));
	if (!item) {
		qWarning() << QObject::tr("Unable to create node by metadata(prototype=%1,name=%2,group=%3)-1")
                          .arg(metadata.getNodePrototype(),
                               metadata.getNodeName(),
                               metadata.getGroup());  // cn:无法通过元数据创建节点(类型=%1,名称=%2,分组=%3)创建节点-1
		return nullptr;
	}
	DAAbstractNode::SharedPointer node = item->node();
	if (nullptr == node) {
		qWarning() << QObject::tr("Unable to create node by metadata(prototype=%1,name=%2,group=%3)-2")
                          .arg(metadata.getNodePrototype(),
                               metadata.getNodeName(),
                               metadata.getGroup());  // cn:无法通过元数据创建节点(类型=%1,名称=%2,分组=%3)创建节点-2
		return nullptr;
	}
	if (!idMap) {
		// idMap为空，此时使用原来的id
		node->setID(id);
	}
	node->setNodeName(name);
	// 加载节点的输入输出
	loadNodeInPutOutputKey(node, nodeEle);
	// 加载节点的属性
	if (mLoadedVersion.majorVersion() == 1 && mLoadedVersion.minorVersion() < 4) {
		loadNodePropertys_v110(node, nodeEle);
	} else {
		loadNodePropertys(node, nodeEle);
	}
	// 加载额外信息
	node->loadExternInfoFromXml(&nodeEle, mLoadedVersion);
	// 加载item信息
	QDomElement itemEle = nodeEle.firstChildElement("item");
	if (itemEle.isNull()) {
		qWarning() << QObject::tr("can not find <item> tag under <node> tag");  // cn:无法在<node>标签下查询到<item>标签
		return nullptr;
	}
	// 由于node和item的信息加载后，node又加载了其他信息，例如input、output这些信息，加载后需要通知item进行刷新，否则item和node不同步
	item->resetLinkPoint();
	qDebug() << "resetLinkPoint=" << item->getLinkPoints() << ",node getInputKeys=" << node->getInputKeys();
	loadItem(item, itemEle);
	if (idMap) {
		(*idMap)[ id ] = node->getID();
	}
	return item;
}
/**
 * @brief DAProjectPrivate::saveNodeInputOutput
 * @param node
 * @param doc
 * @param nodeEle
 */
void DAXmlHelperPrivate::saveNodeInputOutput(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle)
{
	QDomElement inputsEle      = doc.createElement("inputs");
	QList< QString > inputKeys = node->getInputKeys();
	for (const auto& key : qAsConst(inputKeys)) {
		// 添加input
		QDomElement inputEle = doc.createElement("li");
		inputEle.setAttribute("name", key);
		QVariant v = node->getInputData(key);
		if (v.isValid()) {
			QDomElement varEle = DAXmlHelper::createVariantValueElement(doc, "value", v);
			inputEle.appendChild(varEle);
		}
		inputsEle.appendChild(inputEle);
	}

	QDomElement outputsEle      = doc.createElement("outputs");
	QList< QString > outputKeys = node->getOutputKeys();
	for (const auto& key : qAsConst(outputKeys)) {
		// 添加output
		QDomElement outputEle = doc.createElement("li");
		outputEle.setAttribute("name", key);
		QVariant v = node->getOutputData(key);
		if (v.isValid()) {
			QDomElement varEle = DAXmlHelper::createVariantValueElement(doc, "data", v);
			outputEle.appendChild(varEle);
		}
		outputsEle.appendChild(outputEle);
	}
	nodeEle.appendChild(inputsEle);
	nodeEle.appendChild(outputsEle);
}

bool DAXmlHelperPrivate::loadNodeInPutOutputKey(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode)
{
	if (mLoadedVersion.majorVersion() == 1 && mLoadedVersion.minorVersion() < 3) {
		return loadNodeInPutOutputKey_v110(node, eleNode);
	} else if (mLoadedVersion.majorVersion() >= 1 && mLoadedVersion.minorVersion() > 3) {
		return loadNodeInPutOutputKey_v130(node, eleNode);
	}
	return true;
}

bool DAXmlHelperPrivate::loadNodeInPutOutputKey_v110(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode)
{
	//	qDebug() << "loadNodeInPutOutputKey_v110";
	// v1.1.0以下解析方法
	QDomElement e = eleNode.firstChildElement("inputs");
	if (!e.isNull()) {
		QDomNodeList ks = e.childNodes();
		for (int i = 0; i < ks.size(); ++i) {
			QDomElement inputEle = ks.at(i).toElement();
			if (!inputEle.isNull() && inputEle.tagName() != "input") {
				continue;
			}
			QDomElement nameEle = inputEle.firstChildElement("name");
			if (nameEle.isNull()) {
				qWarning() << QObject::tr("node(prototype=%1,name=%2,group=%3) %4 tag loss child tag <name>")
                                  .arg(node->getNodePrototype(),
                                       node->getNodeName(),
                                       node->getNodeGroup(),
                                       ks.at(i).nodeName());
				continue;
			}
			QString key = nameEle.text();
			node->addInputKey(key);
			QDomElement variantEle = inputEle.firstChildElement("data");
			if (!variantEle.isNull()) {
				QVariant d = DAXmlHelper::loadVariantValueElement(variantEle, QVariant());
				node->setInputData(key, d);
			}
		}
	}

	e = eleNode.firstChildElement("outputs");
	if (!e.isNull()) {
		QDomNodeList ks = e.childNodes();
		for (int i = 0; i < ks.size(); ++i) {
			QDomElement outputEle = ks.at(i).toElement();
			if (!outputEle.isNull() && outputEle.tagName() != "output") {
				continue;
			}
			QDomElement nameEle = outputEle.firstChildElement("name");
			if (nameEle.isNull()) {
				qWarning() << QObject::tr("node(prototype=%1,name=%2,group=%3) %4 tag loss child tag <name>")
                                  .arg(node->getNodePrototype(),
                                       node->getNodeName(),
                                       node->getNodeGroup(),
                                       ks.at(i).nodeName());
				continue;
			}
			QString key = nameEle.text();
			node->addOutputKey(key);
			QDomElement variantEle = outputEle.firstChildElement("data");
			if (!variantEle.isNull()) {
				QVariant d = DAXmlHelper::loadVariantValueElement(variantEle, QVariant());
				node->setOutputData(key, d);
			}
		}
	}
	return true;
}

bool DAXmlHelperPrivate::loadNodeInPutOutputKey_v130(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode)
{
	//	qDebug() << "loadNodeInPutOutputKey_v130";
	// v1.3.0解析方法
	QDomElement e = eleNode.firstChildElement("inputs");
	if (!e.isNull()) {
		QDomNodeList ks = e.childNodes();
		for (int i = 0; i < ks.size(); ++i) {
			QDomElement inputEle = ks.at(i).toElement();
			if (!inputEle.isNull() && inputEle.tagName() != "li") {
				continue;
			}
			QString key = inputEle.attribute("name");
			node->addInputKey(key);
			QDomElement variantEle = inputEle.firstChildElement("data");
			if (!variantEle.isNull()) {
				QVariant d = DAXmlHelper::loadVariantValueElement(variantEle, QVariant());
				node->setInputData(key, d);
			}
		}
	}

	e = eleNode.firstChildElement("outputs");
	if (!e.isNull()) {
		QDomNodeList ks = e.childNodes();
		for (int i = 0; i < ks.size(); ++i) {
			QDomElement outputEle = ks.at(i).toElement();
			if (!outputEle.isNull() && outputEle.tagName() != "li") {
				continue;
			}
			QString key = outputEle.attribute("name");
			node->addOutputKey(key);
			QDomElement variantEle = outputEle.firstChildElement("data");
			if (!variantEle.isNull()) {
				QVariant d = DAXmlHelper::loadVariantValueElement(variantEle, QVariant());
				node->setOutputData(key, d);
			}
		}
	}
	return true;
}

/**
 * @brief 保存节点的附加属性
 *
 *
 * @param node
 * @param doc
 * @param nodeEle
 */
void DAXmlHelperPrivate::saveNodePropertys(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle)
{
	const QHash< QString, QVariant >& props = node->propertys();
	savePropertys(props, doc, nodeEle);
	//    QDomElement propertysEle      = doc.createElement("props");
	//	QList< QString > propertyKeys = node->getPropertyKeys();
	//	for (const QString& k : qAsConst(propertyKeys)) {
	//		QVariant v = node->getProperty(k);
	//		if (!v.isValid()) {
	//			continue;
	//		}
	//        QDomElement nameEle = doc.createElement("key");
	//		QDomText nameVal    = doc.createTextNode(k);
	//		nameEle.appendChild(nameVal);
	//        QDomElement varEle      = DAXmlHelper::createVariantValueElement(doc, "value", v);
	//        QDomElement propertyEle = doc.createElement("prop");
	//		propertyEle.appendChild(nameEle);
	//		propertyEle.appendChild(varEle);
	//		propertysEle.appendChild(propertyEle);
	//	}
	//	nodeEle.appendChild(propertysEle);
}

bool DAXmlHelperPrivate::loadNodePropertys(DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle)
{
	QHash< QString, QVariant > props;
	if (loadPropertys(props, nodeEle)) {
		node->propertys() = props;
		return true;
	}
	return false;
	//	QDomElement propertysEle = nodeEle.firstChildElement("propertys");
	//	if (propertysEle.isNull()) {
	//		return false;
	//	}
	//	QDomNodeList propertyslist = propertysEle.childNodes();
	//	for (int i = 0; i < propertyslist.size(); ++i) {
	//		QDomElement propertyEle = propertyslist.at(i).toElement();
	//		if (propertyEle.tagName() != "property") {
	//			continue;
	//		}
	//		QDomElement nameEle = propertyEle.firstChildElement("name");
	//		if (nameEle.isNull()) {
	//			continue;
	//		}
	//		QDomElement variantEle = propertyEle.firstChildElement("variant");
	//		if (variantEle.isNull()) {
	//			continue;
	//		}
	//		QVariant v = DAXmlHelper::loadVariantValueElement(variantEle, QVariant());
	//		node->setProperty(nameEle.text(), v);
	//	}
	//    return true;
}

bool DAXmlHelperPrivate::loadNodePropertys_v110(DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle)
{
	QDomElement propertysEle = nodeEle.firstChildElement("propertys");
	if (propertysEle.isNull()) {
		return false;
	}
	QDomNodeList propertyslist = propertysEle.childNodes();
	for (int i = 0; i < propertyslist.size(); ++i) {
		QDomElement propertyEle = propertyslist.at(i).toElement();
		if (propertyEle.tagName() != "property") {
			continue;
		}
		QDomElement nameEle = propertyEle.firstChildElement("name");
		if (nameEle.isNull()) {
			continue;
		}
		QDomElement variantEle = propertyEle.firstChildElement("variant");
		if (variantEle.isNull()) {
			continue;
		}
		QVariant v = DAXmlHelper::loadVariantValueElement(variantEle, QVariant());
		node->setProperty(nameEle.text(), v);
	}
	return true;
}

void DAXmlHelperPrivate::saveNodeItem(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle)
{
	DAAbstractNodeGraphicsItem* item = node->graphicsItem();
	saveItem(item, doc, nodeEle);
}

/**
 * @brief 加载<node>节点下的<node-item>节点
 * @note 此函数不会把加载的item添加到scene
 * @param scene
 * @param node
 * @param nodeEle <node>节点
 * @return
 */
DAAbstractNodeGraphicsItem* DAXmlHelperPrivate::loadNodeItem(DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle)
{

	QDomElement itemEle = nodeEle.firstChildElement("item");
	if (itemEle.isNull()) {
		qWarning() << QObject::tr("can not find <item> tag under <node> tag");  // cn:无法在<node>标签下查询到<item>标签
		return nullptr;
	}
	DAAbstractNodeGraphicsItem* item = node->createGraphicsItem();
	if (nullptr == item) {
		qWarning() << QObject::tr("node metadata(prototype=%1,name=%2,group=%3) can not create graphics item")
                          .arg(node->getNodePrototype(), node->getNodeName(), node->getNodeGroup());
		return nullptr;
	}
	loadItem(item, itemEle);
	return item;
}

void DAXmlHelperPrivate::saveNodeLinks(const DAWorkFlow* workflow, QDomDocument& doc, QDomElement& workflowEle)
{
	const QList< DAAbstractNode::SharedPointer >& nodes = workflow->nodes();
	//
	QDomElement nodeLinkEle = doc.createElement("links");

	QSet< DAAbstractNodeLinkGraphicsItem* > itemSet;
	for (const auto& node : nodes) {
		auto links = node->graphicsItem()->getLinkItems();
		for (auto link : qAsConst(links)) {
			itemSet.insert(link);
		}
	}

	for (auto link : itemSet) {
		QDomElement linkEle = makeNodeLinkElement(link, QStringLiteral("link"), doc);
		nodeLinkEle.appendChild(linkEle);
	}

	workflowEle.appendChild(nodeLinkEle);
}

/**
 * @brief 创建一个链接节点，这个链接节点虽然传入的是DAAbstractNodeLinkGraphicsItem
 * 但它会保存链接的必要信息，不仅仅保存图元的信息
 *
 * @note 此函数会调用saveItem,会把linkitem写入mHaveBeenSaveNodeItem中
 * @param link
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXmlHelperPrivate::makeNodeLinkElement(DAAbstractNodeLinkGraphicsItem* link,
                                                    const QString& tagName,
                                                    QDomDocument& doc)
{
	QDomElement linkEle = doc.createElement(tagName);
	QDomElement fromEle = doc.createElement("from");
	QDomElement toEle   = doc.createElement("to");
	fromEle.setAttribute("id", link->fromNode()->getID());
	fromEle.setAttribute("name", link->fromNodeLinkPoint().name);
	toEle.setAttribute("id", link->toNode()->getID());
	toEle.setAttribute("name", link->toNodeLinkPoint().name);

	linkEle.appendChild(fromEle);
	linkEle.appendChild(toEle);
	if (!saveItem(link, doc, linkEle)) {
		qWarning() << QObject::tr("linkitem save to xml return false");  // cn:链接线从xml加载信息返回了false
	}
	return linkEle;
}

bool DAXmlHelperPrivate::loadNodeLinks(DAWorkFlowGraphicsScene* scene, DAWorkFlow* wf, const QDomElement& workflowEle)
{
	QDomElement linksEle = workflowEle.firstChildElement("links");
	QDomNodeList list    = linksEle.childNodes();
	for (int i = 0; i < list.size(); ++i) {
		QDomElement linkEle = list.at(i).toElement();
		if (linkEle.tagName() != "link") {
			continue;
		}
		QDomElement fromEle = linkEle.firstChildElement("from");
		QDomElement toEle   = linkEle.firstChildElement("to");
		if (fromEle.isNull() || toEle.isNull()) {
			continue;
		}
		bool ok = false;

		qulonglong id   = fromEle.attribute("id").toULongLong(&ok);
		QString fromKey = fromEle.attribute("name");

		DAAbstractNode::SharedPointer fromNode = wf->getNode(id);
		if (!ok || nullptr == fromNode) {
			qWarning() << QObject::tr("link info can not find node in workflow,id = %1").arg(fromEle.attribute("id"));
			continue;
		}
		id                                   = toEle.attribute("id").toULongLong(&ok);
		QString toKey                        = toEle.attribute("name");
		DAAbstractNode::SharedPointer toNode = wf->getNode(id);
		if (!ok || nullptr == toNode) {
			qWarning() << QObject::tr("link info can not find node in workflow,id = %1").arg(toEle.attribute("id"));
			continue;
		}
		DAAbstractNodeGraphicsItem* fromItem = fromNode->graphicsItem();
		DAAbstractNodeGraphicsItem* toItem   = toNode->graphicsItem();
		if (nullptr == fromItem || nullptr == toItem) {
			qWarning() << QObject::tr("can not get item by node");
			continue;
		}
		// 建立链接线
        DAAbstractNodeLinkGraphicsItem* linkitem = fromItem->linkToByName(fromKey, toItem, toKey);
		if (nullptr == linkitem) {
			qWarning() << QObject::tr("Unable to link to node %3's link point %4 through link point %2 of node %1")  // cn:节点%1无法通过连接点%2链接到节点%3的连接点%4
                              .arg(fromItem->getNodeName(), fromKey, toItem->getNodeName(), toKey);
			continue;
		}
		if (mLoadedVersion.majorVersion() == 1 && mLoadedVersion.minorVersion() <= 3) {
			//! v1.3.0版本
			//!
			//! <link>
			//! <from id="2797356937958297592" name="out"/>
			//! <to id="6288358926546712460" name="input-1"/>
			//! <linkPoint visible="0" fromTextColor="#000000" toTextColor="#000000" fromPositionOffset="10" toPositionOffset="10"/>
			//! <endPoint fromType="none" size="12" toType="triang"/>
			//! <linkLine style="bezier"/>
			//! <linePen width="1" style="1" color="#808093"/>
			//! </link>
			if (!loadItem(linkitem, linkEle)) {
				qWarning() << QObject::tr("linkitem load from xml return false")  // cn:链接线从xml加载信息返回了false
                    ;
			}
		} else {
			//!
			//! v1.4.0版本
			//!
			//! <link>
			//!     <from id="16440979847065875286" name="nz"/>
			//!     <to id="16420698186860452117" name="z"/>
			//!     <item tg="DAGraphics" className="PipeLineNodeLinkGraphicsItem" tid="66039">
			//!         <info z="-1" y="207.5" rotation="0" id="16440979091151641689" x="-137.5" enable="1" opacity="1" acceptDrops="0" scale="1">
			//!             <shape-info show-border="0" show-bk="0"/>
			//!         </info>
			//!         <pos BezierControlScale="0.34999999999999998">
			//!             <startPos y="0" x="0" class="QPointF"/>
			//!             <endPos y="150" x="0" class="QPointF"/>
			//!             <boundingRect y="48" h="104" w="4" x="-2" class="QRectF"/>
			//!         </pos>
			//!         <linkLine style="bezier"/>
			//!         <linePen color="#df593f" style="SolidLine" width="2"/>
			//!         <endPoint fromType="none" size="12" toType="triang"/>
			//!         <linkPoint fromTextColor="#000000" toTextColor="#000000" visible="0" fromPositionOffset="10" toPositionOffset="10"/>
			//!         <pipe is-orth="1" pipeID="19" is-main="1" radiusInner="1" thickness="1" length="100"/>
			//!     </item>
			//! </link>
			QDomElement itemEle = findItemElement(linkEle);
			if (!loadItem(linkitem, itemEle)) {
				qWarning() << QObject::tr("linkitem load from xml return false")  // cn:链接线从xml加载信息返回了false
                    ;
			}
		}
		scene->addItem(linkitem);
		linkitem->updatePos();
	}
	return true;
}

bool DAXmlHelperPrivate::loadNodeLinksClipBoardCopy(DAWorkFlowGraphicsScene* scene,
                                                    const QDomElement& workflowEle,
                                                    const QMap< qulonglong, qulonglong >* idMap)
{
	DAWorkFlow* wf       = scene->getWorkflow();
	QDomElement linksEle = workflowEle.firstChildElement("links");
	QDomNodeList list    = linksEle.childNodes();
	for (int i = 0; i < list.size(); ++i) {
		QDomElement linkEle = list.at(i).toElement();
		if (linkEle.tagName() != "link") {
			continue;
		}
		QDomElement fromEle = linkEle.firstChildElement("from");
		QDomElement toEle   = linkEle.firstChildElement("to");
		if (fromEle.isNull() || toEle.isNull()) {
			continue;
		}
		bool ok = false;

		qulonglong id   = fromEle.attribute("id").toULongLong(&ok);
		QString fromKey = fromEle.attribute("name");
		DAAbstractNode::SharedPointer fromNode;
		// 找到实际的id
		if (idMap) {
			qulonglong realId = idMap->value(id, 0);
			if (0 == realId) {
				qWarning()
                    << QObject::tr("During the pasting process, the mapping corresponding to ID(%1) cannot be found").arg(id);
				continue;
			}
			fromNode = wf->getNode(realId);
		} else {
			fromNode = wf->getNode(id);
		}
		if (nullptr == fromNode) {
			qWarning() << QObject::tr("link info can not find node in workflow,id = %1").arg(fromEle.attribute("id"));
			continue;
		}

		id            = toEle.attribute("id").toULongLong(&ok);
		QString toKey = toEle.attribute("name");
		DAAbstractNode::SharedPointer toNode;
		if (idMap) {
			qulonglong realId = idMap->value(id, 0);
			if (0 == realId) {
				qWarning()
                    << QObject::tr("During the pasting process, the mapping corresponding to ID(%1) cannot be found").arg(id);
				continue;
			}
			toNode = wf->getNode(realId);
		} else {
			toNode = wf->getNode(id);
		}
		if (nullptr == toNode) {
			qWarning() << QObject::tr("link info can not find node in workflow,id = %1").arg(toEle.attribute("id"));
			continue;
		}

		DAAbstractNodeGraphicsItem* fromItem = fromNode->graphicsItem();
		DAAbstractNodeGraphicsItem* toItem   = toNode->graphicsItem();
		if (nullptr == fromItem || nullptr == toItem) {
			qWarning() << QObject::tr("can not get item by node");
			continue;
		}
		// 建立链接线
        DAAbstractNodeLinkGraphicsItem* linkitem = fromItem->linkToByName(fromKey, toItem, toKey);
		if (nullptr == linkitem) {
			qWarning() << QObject::tr("Unable to link to node %3's link point %4 through link point %2 of node %1")  // cn:节点%1无法通过连接点%2链接到节点%3的连接点%4
                              .arg(fromItem->getNodeName(), fromKey, toItem->getNodeName(), toKey);
			continue;
		}
		if (mLoadedVersion.majorVersion() == 1 && mLoadedVersion.minorVersion() <= 3) {
			//! v1.3.0版本
			//!
			//! <link>
			//! <from id="2797356937958297592" name="out"/>
			//! <to id="6288358926546712460" name="input-1"/>
			//! <linkPoint visible="0" fromTextColor="#000000" toTextColor="#000000" fromPositionOffset="10" toPositionOffset="10"/>
			//! <endPoint fromType="none" size="12" toType="triang"/>
			//! <linkLine style="bezier"/>
			//! <linePen width="1" style="1" color="#808093"/>
			//! </link>
			if (!loadItem(linkitem, linkEle)) {
				qWarning() << QObject::tr("linkitem load from xml return false")  // cn:链接线从xml加载信息返回了false
                    ;
			}
		} else {
			//!
			//! v1.4.0版本
			//!
			//! <link>
			//!     <from id="16440979847065875286" name="nz"/>
			//!     <to id="16420698186860452117" name="z"/>
			//!     <item tg="DAGraphics" className="PipeLineNodeLinkGraphicsItem" tid="66039">
			//!         <info z="-1" y="207.5" rotation="0" id="16440979091151641689" x="-137.5" enable="1" opacity="1" acceptDrops="0" scale="1">
			//!             <shape-info show-border="0" show-bk="0"/>
			//!         </info>
			//!         <pos BezierControlScale="0.34999999999999998">
			//!             <startPos y="0" x="0" class="QPointF"/>
			//!             <endPos y="150" x="0" class="QPointF"/>
			//!             <boundingRect y="48" h="104" w="4" x="-2" class="QRectF"/>
			//!         </pos>
			//!         <linkLine style="bezier"/>
			//!         <linePen color="#df593f" style="SolidLine" width="2"/>
			//!         <endPoint fromType="none" size="12" toType="triang"/>
			//!         <linkPoint fromTextColor="#000000" toTextColor="#000000" visible="0" fromPositionOffset="10" toPositionOffset="10"/>
			//!         <pipe is-orth="1" pipeID="19" is-main="1" radiusInner="1" thickness="1" length="100"/>
			//!     </item>
			//! </link>
			QDomElement itemEle = findItemElement(linkEle);
			if (!loadItem(linkitem, itemEle)) {
				qWarning() << QObject::tr("linkitem load from xml return false")  // cn:链接线从xml加载信息返回了false
                    ;
			}
		}
		DACommandsForWorkFlowCreateLink* cmd = new DACommandsForWorkFlowCreateLink(linkitem, scene);
		scene->push(cmd);
		linkitem->updatePos();
	}
	return true;
}
/**
 * @brief 保存特殊的item
 * @param scene
 * @param doc
 * @param workflowEle
 */
void DAXmlHelperPrivate::saveCommonItems(const DAWorkFlowGraphicsScene* scene, QDomDocument& doc, QDomElement& workflowEle)
{
	// 保存文本
	QList< QGraphicsItem* > items = scene->topItems();
	DAGraphicsPixmapItem* bkItem  = scene->getBackgroundPixmapItem();
	items.removeAll(bkItem);
	QDomElement itemsElement = makeCommonItemsElement(items, QStringLiteral("items"), doc);
	workflowEle.appendChild(itemsElement);
}

/**
 * @brief 创建通用图元节点
 *
 * @note 注意，这个必须传入的是非节点图元
 * @param items 非节点图元
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXmlHelperPrivate::makeCommonItemsElement(const QList< QGraphicsItem* >& items,
                                                       const QString& tagName,
                                                       QDomDocument& doc)
{
	QDomElement itemsElement = doc.createElement(tagName);
	// 背景不作为items保存
	for (const QGraphicsItem* i : qAsConst(items)) {
		if (isItemHaveDeal(i)) {
			// 已经保存过的不进行保存
			continue;
		}
		saveItem(i, doc, itemsElement);
	}
	return itemsElement;
}
/**
 * @brief 加载特殊的item
 * @param scene
 * @param workflowEle
 * @param isRedoUndo 是否回退
 * @return
 */
bool DAXmlHelperPrivate::loadCommonItems(DAWorkFlowGraphicsScene* scene, const QDomElement& workflowEle, bool isRedoUndo)
{
	QDomElement itemsElement = workflowEle.firstChildElement(QStringLiteral("items"));
	QDomNodeList childNodes  = itemsElement.childNodes();
	for (int i = 0; i < childNodes.size(); ++i) {
		QDomElement itemEle = childNodes.at(i).toElement();
		if (itemEle.tagName() == QStringLiteral("item")) {
			QGraphicsItem* item = loadItem(itemEle);
			if (item) {
				if (isRedoUndo) {
					scene->addItem_(item);
				} else {
					scene->addItem(item);
				}
			}
		}
	}
	for (int i = 0; i < childNodes.size(); ++i) {
		QDomElement itemEle = childNodes.at(i).toElement();
		if (itemEle.tagName() == QStringLiteral("group")) {
			QGraphicsItem* item = loadItemGroup(scene, itemEle);
			if (item) {
				if (isRedoUndo) {
					scene->addItem_(item);
				} else {
					scene->addItem(item);
				}
			}
		}
	}
	return true;
}

/**
   @brief 保存DAGraphicsItem
   @param i
   @param doc
   @param itemsElement
 */
bool DAXmlHelperPrivate::saveItem(const QGraphicsItem* i, QDomDocument& doc, QDomElement& parentElement)
{
	if (const DAGraphicsItemGroup* gi = dynamic_cast< const DAGraphicsItemGroup* >(i)) {
		saveItemGroup(gi, doc, parentElement);
	} else {
		auto itemEle = DAXmlHelper::makeElement(i, QStringLiteral("item"), &doc);
		if (itemEle.isNull()) {
			qWarning() << QObject::tr("Unable to generate graphics item element during the saveing");  // cn:保存过程中，无法生成图元元素
			return false;
		}
		parentElement.appendChild(itemEle);
		recordDealItem(i);
	}
	return true;
}

/**
   @brief 加载da item
   @param itemElement
   @return
 */
QGraphicsItem* DAXmlHelperPrivate::loadItem(const QDomElement& itemElement)
{
	QGraphicsItem* item = DAXmlHelper::loadItemElement(&itemElement, mLoadedVersion);
	if (!item) {
		return nullptr;
	}
	return item;
}

bool DAXmlHelperPrivate::loadItem(QGraphicsItem* item, const QDomElement& itemElement)
{
	if (!DAXmlHelper::loadElement(item, &itemElement, mLoadedVersion)) {
		qWarning() << QObject::tr("Unable to load item information from <%1>").arg(itemElement.tagName());  // 无法通过<%1>加载元件信息
		return false;
	}
	recordDealItem(item);
	return true;
}

/**
 * @brief 查找item标签
 * @param parentElement
 * @return
 */
QDomElement DAXmlHelperPrivate::findItemElement(const QDomElement& parentElement)
{
    return parentElement.firstChildElement("item");
}
/**
   @brief 保存分组信息

   这个分组是放到最后保存，前面已经把item保存了，分组里面可能还会有item还需要保存
   @param scene
   @param doc
   @param workflowEle
 */
void DAXmlHelperPrivate::saveItemGroup(const DAGraphicsItemGroup* itemGroup, QDomDocument& doc, QDomElement& parentElement)
{
	qDebug() << "saveItemGroup";
	const QList< QGraphicsItem* > childItems = itemGroup->childItems();
	// 先保存实体信息
	for (auto i : childItems) {
		qDebug() << "group:" << i->type();
		// 保证子分组先保存
		// 如果保存过就不保存
		if (isItemHaveDeal(i)) {
			continue;
		}
		saveItem(i, doc, parentElement);
	}
	// 再记录分组信息
	QDomElement gEle = DAXmlHelper::makeElement(itemGroup, "group", &doc);
	parentElement.appendChild(gEle);
}

DAGraphicsItemGroup* DAXmlHelperPrivate::loadItemGroup(DAWorkFlowGraphicsScene* scene, const QDomElement& groupElement)
{
	std::unique_ptr< DAGraphicsItemGroup > group = std::make_unique< DAGraphicsItemGroup >();
	if (!DAXmlHelper::loadElement(scene, group.get(), &groupElement)) {
		return nullptr;
	}
	scene->addItem(group.get());
	return group.release();
}

void DAXmlHelperPrivate::saveSecenInfo(DAWorkFlowGraphicsScene* scene, QDomDocument& doc, QDomElement& workflowEle)
{
	QDomElement sceneEle = doc.createElement("scene");
	QRectF sceneRect     = scene->sceneRect();
	sceneEle.setAttribute("x", sceneRect.x());
	sceneEle.setAttribute("y", sceneRect.y());
	sceneEle.setAttribute("width", sceneRect.width());
	sceneEle.setAttribute("height", sceneRect.height());
	// 保存背景图
	auto item = scene->getBackgroundPixmapItem();
	if (item) {
		QDomElement imageEle = doc.createElement("background");
		item->saveToXml(&doc, &imageEle, DAXmlHelper::getCurrentVersionNumber());
		sceneEle.appendChild(imageEle);
	}

	workflowEle.appendChild(sceneEle);
}

bool DAXmlHelperPrivate::loadSecenInfo(DAWorkFlowGraphicsScene* scene, const QDomElement& workflowEle)
{
	QDomElement sceneEle = workflowEle.firstChildElement("scene");
	if (sceneEle.isNull()) {
		return false;
	}
	// 暂时不对scene的尺寸做限制
	//    qreal x      = attributeToDouble(sceneEle, "x");
	//    qreal y      = attributeToDouble(sceneEle, "y");
	//    qreal width  = attributeToDouble(sceneEle, "width");
	//    qreal height = attributeToDouble(sceneEle, "height");
	//    scene->setSceneRect(x, y, width, height);

	QDomElement imageEle = sceneEle.firstChildElement("background");
	if (!imageEle.isNull()) {
		std::unique_ptr< DAGraphicsPixmapItem > item = std::make_unique< DAGraphicsPixmapItem >();
		if (item->loadFromXml(&imageEle, mLoadedVersion)) {
			scene->setBackgroundPixmapItem(item.release());
		}
	}
	return true;
}

void DAXmlHelperPrivate::savePropertys(const QHash< QString, QVariant >& props, QDomDocument& doc, QDomElement& parentEle)
{
	QDomElement propertysEle = doc.createElement("props");
	for (auto i = props.begin(); i != props.end(); ++i) {
		QVariant v = i.value();
		if (!v.isValid()) {
			continue;
		}
		QDomElement keyEle = doc.createElement("key");
		keyEle.appendChild(doc.createTextNode(i.key()));
		QDomElement valueEle    = DAXmlHelper::createVariantValueElement(doc, "value", v);
		QDomElement propertyEle = doc.createElement("prop");
		propertyEle.appendChild(keyEle);
		propertyEle.appendChild(valueEle);
		propertysEle.appendChild(propertyEle);
	}
	parentEle.appendChild(propertysEle);
}

bool DAXmlHelperPrivate::loadPropertys(QHash< QString, QVariant >& props, const QDomElement& parentEle)
{
	QDomElement propertysEle = parentEle.firstChildElement("props");
	if (propertysEle.isNull()) {
		return false;
	}
	QDomNodeList propertyslist = propertysEle.childNodes();
	for (int i = 0; i < propertyslist.size(); ++i) {
		QDomElement propertyEle = propertyslist.at(i).toElement();
		if (propertyEle.tagName() != "prop") {
			continue;
		}
		QDomElement nameEle = propertyEle.firstChildElement("key");
		if (nameEle.isNull()) {
			continue;
		}
		QDomElement variantEle = propertyEle.firstChildElement("value");
		if (variantEle.isNull()) {
			continue;
		}
		QVariant v              = DAXmlHelper::loadVariantValueElement(variantEle, QVariant());
		props[ nameEle.text() ] = v;
	}
	return true;
}

void DAXmlHelperPrivate::clearDealItemSet()
{
	mHaveBeenDealNodeItem.clear();
}

void DAXmlHelperPrivate::recordDealItem(QGraphicsItem* i)
{
	mHaveBeenDealNodeItem.insert(i);
}

void DAXmlHelperPrivate::recordDealItem(const QGraphicsItem* i)
{
	mHaveBeenDealNodeItem.insert(const_cast< QGraphicsItem* >(i));
}

bool DAXmlHelperPrivate::isItemHaveDeal(QGraphicsItem* i) const
{
	return mHaveBeenDealNodeItem.contains(i);
}

bool DAXmlHelperPrivate::isItemHaveDeal(const QGraphicsItem* i) const
{
	return mHaveBeenDealNodeItem.contains(const_cast< QGraphicsItem* >(i));
}

//==============================================================
// DAXmlHelper
//==============================================================
DAXmlHelper::DAXmlHelper() : d_ptr(new DAXmlHelperPrivate(this))
{
}

DAXmlHelper::~DAXmlHelper()
{
}

void DAXmlHelper::setLoadedVersionNumber(const QVersionNumber& v)
{
	d_ptr->mLoadedVersion = v;
}

QVersionNumber DAXmlHelper::getLoaderVersionNumber() const
{
	return d_ptr->mLoadedVersion;
}

QVersionNumber DAXmlHelper::getCurrentVersionNumber()
{
	static QVersionNumber s_version(1, 4, 0);
	return s_version;
}

QDomElement DAXmlHelper::makeElement(DAWorkFlowEditWidget* wfe, const QString& tagName, QDomDocument* doc)
{
	QDomElement workflowEle = doc->createElement(tagName);
	d_ptr->saveWorkflow(wfe, *doc, workflowEle);
	return workflowEle;
}

bool DAXmlHelper::loadElement(DAWorkFlowEditWidget* wfe, const QDomElement* ele)
{
	return d_ptr->loadWorkflow(wfe, *ele);
}

QDomElement DAXmlHelper::makeElement(DAWorkFlowOperateWidget* wfo, const QString& tagName, QDomDocument* doc)
{
	QDomElement workflowsElement = doc->createElement(tagName);
	int wfcount                  = wfo->count();
	for (int i = 0; i < wfcount; ++i) {
		DAWorkFlowEditWidget* wfe = wfo->getWorkFlowWidget(i);
		QDomElement workflowEle   = makeElement(wfe, "workflow", doc);
		QString name              = wfo->getWorkFlowWidgetName(i);
		workflowEle.setAttribute("name", name);
		workflowsElement.appendChild(workflowEle);
	}
	int currentIndex = wfo->getCurrentWorkflowIndex();
	workflowsElement.setAttribute("currentIndex", currentIndex);
	workflowsElement.setAttribute("ver", getCurrentVersionNumber().toString());
	return workflowsElement;
}

/**
 * @brief 从xml中加载DAWorkFlowOperateWidget
 * @param wfo
 * @param ele
 * @return
 */
bool DAXmlHelper::loadElement(DAWorkFlowOperateWidget* wfo, const QDomElement* workflowsEle)
{
	Q_CHECK_PTR(wfo);
	bool isok = true;
	// 先获取当前的窗口名字，避免重名
	QSet< QString > names    = qlist_to_qset(wfo->getAllWorkflowNames());
	QDomNodeList wfListNodes = workflowsEle->childNodes();
	QString verString        = workflowsEle->attribute("ver");
	if (!verString.isEmpty()) {
		QVersionNumber ver = QVersionNumber::fromString(verString);
		if (!ver.isNull()) {
			setLoadedVersionNumber(ver);
		}
	} else {
		// 说明是较低版本，设置为v1.1
		setLoadedVersionNumber(QVersionNumber(1, 1, 0));
	}
	qInfo() << QObject::tr("current workflow file version:").arg(getLoaderVersionNumber().toString());
	for (int i = 0; i < wfListNodes.size(); ++i) {
		QDomElement workflowEle = wfListNodes.at(i).toElement();
		if (workflowEle.tagName() != "workflow") {
			continue;
		}
		QString name = workflowEle.attribute("name");
		// 生成一个唯一名字
		name = DA::makeUniqueString(names, name);
		// 建立工作流窗口
		DAWorkFlowEditWidget* wfe = wfo->appendWorkflow(name);
		isok &= loadElement(wfe, &workflowEle);
	}
	return isok;
}

/**
 * @brief 创建剪切板描述xml
 * @param its
 * @param doc
 * @return
 */
QDomElement DAXmlHelper::makeClipBoardElement(const QList< DAGraphicsItem* > its,
                                              const QString& tagName,
                                              QDomDocument* doc,
                                              bool isCopyType)
{
	QDomElement rootEle = doc->createElement(tagName);
	if (isCopyType) {
		rootEle.setAttribute("type", "copy");  // copy 和 cut之分
	} else {
		rootEle.setAttribute("type", "cut");  // copy 和 cut之分
	}
	QDomElement workflowEle = doc->createElement(QStringLiteral("workflow"));
	d_ptr->saveWorkflowFromClipBoard(its, *doc, workflowEle);
	rootEle.appendChild(workflowEle);
	return rootEle;
}

bool DAXmlHelper::loadClipBoardElement(const QDomElement* clipBoardElement, DAWorkFlowGraphicsScene* sc)
{
	setLoadedVersionNumber(getCurrentVersionNumber());
	//! 先识别类型
	QString typestr = clipBoardElement->attribute("type");
	if (typestr.isEmpty()) {
		qDebug() << QObject::tr("An exception occurred during the process of processing pasted content XML, with the "
                                "root node missing the type attribute");  // cn:在处理粘贴内容xml过程出现异常，根节点缺失type属性
		return false;
	}
	QDomElement workflowEle = clipBoardElement->firstChildElement("workflow");
	//! 首先找到workflow节点
	if (workflowEle.isNull()) {
		qWarning() << QObject::tr(
            "An exception occurred during the process of parsing and pasting content,miss workflow tag");  // cn:解析粘贴内容过程出现异常,缺失workflow标签
		return false;
	}
	if (typestr == "copy") {
		d_ptr->loadWorkflowFromClipBoard(sc, workflowEle, true);
	} else if (typestr == "cut") {
		d_ptr->loadWorkflowFromClipBoard(sc, workflowEle, false);
	}
	return true;
}

QDomElement DAXmlHelper::makeElement(const DAGraphicsItem* item, const QString& tagName, QDomDocument* doc)
{
	QDomElement ele = doc->createElement(tagName);
	ele.setAttribute("className", item->metaObject()->className());
	ele.setAttribute("tid", item->type());
	item->saveToXml(doc, &ele, getCurrentVersionNumber());
	return ele;
}

bool DAXmlHelper::loadElement(DAGraphicsItem* item, const QDomElement* tag, const QVersionNumber& v)
{
    return item->loadFromXml(tag, v);
}

/**
 * @brief 加载item
 *
 * 这是一个工厂函数，返回的指针如果不使用需要销毁
 * @param itemEle
 * @param v
 * @return
 */
QGraphicsItem* DAXmlHelper::loadItemElement(const QDomElement* itemEle, const QVersionNumber& v)
{
	QString className = itemEle->attribute("className");
	if (className.isEmpty()) {
		return nullptr;
	}
	std::unique_ptr< QGraphicsItem > item(DAGraphicsItemFactory::createItem(className));
	if (nullptr == item) {
		qWarning() << QObject::tr("Cannot create item by class name:%1,maybe unregist to DAGraphicsItemFactory")
                          .arg(className);  // 无法通过类名:%1创建元件,类名没有注册到DAGraphicsItemFactory
		return nullptr;
	}
	if (!loadElement(item.get(), itemEle, v)) {
		return nullptr;
	}
	return item.release();
}

/**
 * @brief DAGraphicsItemGroup的通用保存
 *
 * @note DAGraphicsItemGroup并不会把子item的信息保存，仅仅记录子item的id
 * @param item
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXmlHelper::makeElement(const DAGraphicsItemGroup* itemGroup, const QString& tagName, QDomDocument* doc)
{
	QDomElement gEle = doc->createElement(tagName);
	gEle.setAttribute("id", itemGroup->getItemID());
	const QList< QGraphicsItem* > childItems = itemGroup->childItems();
	if (!childItems.empty()) {
		QDomElement ciEle = doc->createElement("childs");
		for (auto i : childItems) {
			if (DAGraphicsItem* daItem = DAGraphicsItem::cast(i)) {
				QDomElement liEle = doc->createElement("li");
				liEle.appendChild(doc->createTextNode(QString::number(daItem->getItemID())));
				ciEle.appendChild(liEle);
			} else if (DAGraphicsStandardTextItem* si = dynamic_cast< DAGraphicsStandardTextItem* >(i)) {
				QDomElement liEle = doc->createElement("li");
				liEle.appendChild(doc->createTextNode(QString::number(si->getItemID())));
				ciEle.appendChild(liEle);
			} else if (DAGraphicsItemGroup* gi = dynamic_cast< DAGraphicsItemGroup* >(i)) {
				QDomElement liEle = doc->createElement("li");
				liEle.appendChild(doc->createTextNode(QString::number(gi->getItemID())));
				ciEle.appendChild(liEle);
			}
		}
		gEle.appendChild(ciEle);
	}
	return gEle;
}
/**
 * @brief 加载分组，一般分组要最后加载
 * @param scene 场景，主要用于搜索item，注意此函数不会把group加入场景中
 * @param group
 * @param groupElement
 * @param v
 * @return
 */
bool DAXmlHelper::loadElement(DAGraphicsScene* scene,
                              DAGraphicsItemGroup* group,
                              const QDomElement* groupElement,
                              const QVersionNumber& v)
{
	uint64_t id;
	if (!getStringULongLongValue(groupElement->attribute("id"), id)) {
		return false;
	}
	group->setItemID(id);
	// 先查看是否有group
	auto cgEle    = groupElement->firstChildElement("childs");
	auto cgChilds = cgEle.childNodes();
	for (int i = 0; i < cgChilds.size(); ++i) {
		auto liEle = cgChilds.at(i).toElement();
		if (liEle.isNull()) {
			continue;
		}
		id                  = liEle.text().toULongLong();
		QGraphicsItem* item = scene->findItemByID(id);
		if (item) {
			group->addToGroup(item);
		}
	}
	return true;
}

QDomElement DAXmlHelper::makeElement(const QGraphicsItem* item, const QString& tagName, QDomDocument* doc)
{
	if (const DAGraphicsItem* daItem = dynamic_cast< const DAGraphicsItem* >(item)) {
		return makeElement(daItem, tagName, doc);
	} else if (const DAGraphicsStandardTextItem* si = dynamic_cast< const DAGraphicsStandardTextItem* >(item)) {
		QDomElement itemElement = doc->createElement(tagName);
		itemElement.setAttribute("className", "DA::DAGraphicsStandardTextItem");
		itemElement.setAttribute("tid", item->type());
		si->saveToXml(doc, &itemElement, DAXmlHelper::getCurrentVersionNumber());
		return itemElement;
	} else if (const DAGraphicsItemGroup* gi = dynamic_cast< const DAGraphicsItemGroup* >(item)) {
		return makeElement(gi, tagName, doc);
	}
	return QDomElement();
}

/**
 * @brief 加载
 * @param item
 * @param tag
 * @param v
 * @return
 */
bool DAXmlHelper::loadElement(QGraphicsItem* item, const QDomElement* tag, const QVersionNumber& v)
{
	DAXMLFileInterface* xml = dynamic_cast< DAXMLFileInterface* >(item);
	if (xml) {
		if (!xml->loadFromXml(tag, v)) {
			return false;
		}
	}
	return true;
}

/**
 * @brief 获取所有处理过的item
 * @return
 */
QList< QGraphicsItem* > DAXmlHelper::getAllDealItems() const
{
	QList< QGraphicsItem* > res(d_ptr->mHaveBeenDealNodeItem.begin(), d_ptr->mHaveBeenDealNodeItem.end());
	return res;
}

/**
 * @brief 生成一个qvariant element
 * @param doc
 * @param v
 * @return
 */
QDomElement DAXmlHelper::createVariantValueElement(QDomDocument& doc, const QString& tagName, const QVariant& var)
{
    return DAXMLFileInterface::makeElement(var, tagName, &doc);
}

QVariant DAXmlHelper::loadVariantValueElement(const QDomElement& item, const QVariant& defaultVal)
{
	QVariant res;
	if (DAXMLFileInterface::loadElement(res, &item)) {
		return res;
	}
	return defaultVal;
}

/**
 * @brief 带警告的attribute转double
 * @param item
 * @param att
 * @return
 */
qreal DAXmlHelper::attributeToDouble(const QDomElement& item, const QString& att)
{
	bool isok = false;
	qreal r   = item.attribute(att).toDouble(&isok);
	if (!isok) {
		qWarning() << QObject::tr("The attribute %1=%2 under the tag %3 cannot be converted to double ")
                          .arg(att, item.attribute(att), item.tagName());
	}
	return r;
}
}
