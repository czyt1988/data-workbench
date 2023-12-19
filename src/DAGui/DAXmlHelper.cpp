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
#include "DAWorkFlowGraphicsScene.h"
#include "DAGraphicsResizeablePixmapItem.h"
#include "DAGraphicsItemFactory.h"
#include "DAAbstractNodeFactory.h"
#include "DAGraphicsItem.h"
#include "DAXMLFileInterface.h"
#include "DAStringUtil.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAWorkFlowEditWidget.h"
#include "DAQtContainerUtil.h"
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
    // 保存工厂相关的扩展信息
    void saveFactoryInfo(const DAWorkFlow* workflow, QDomDocument& doc, QDomElement& workflowEle);
    bool loadFactoryInfo(DAWorkFlow* workflow, const QDomElement& workflowEle);
    // 保存工作流的节点
    void saveNodes(const DAWorkFlowEditWidget* wfe, QDomDocument& doc, QDomElement& workflowEle);
    bool loadNodes(DAWorkFlowEditWidget* wfe, const QDomElement& workflowEle);
    // 保存输入输出
    void saveNodeInputOutput(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle);
    bool loadNodeInPutOutputKey(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode);
    // 保存属性
    void saveNodePropertys(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle);
    bool loadNodePropertys(DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle);
    // 保存item
    void saveNodeItem(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle);
    bool loadNodeItem(DAWorkFlowGraphicsScene* scene, DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle);
    // 保存链接
    void saveNodeLinks(const DAWorkFlow* workflow, QDomDocument& doc, QDomElement& workflowEle);
    bool loadNodeLinks(DAWorkFlowGraphicsScene* scene, DAWorkFlow* wf, const QDomElement& workflowEle);
    // 保存特殊的item，主要为文本
    void saveSpecialItem(const DAWorkFlowGraphicsScene* scene, QDomDocument& doc, QDomElement& workflowEle);
    bool loadSpecialItem(DAWorkFlowGraphicsScene* scene, const QDomElement& workflowEle);

    // SecenInfo
    void saveSecenInfo(DAWorkFlowGraphicsScene* scene, QDomDocument& doc, QDomElement& workflowEle);
    bool loadSecenInfo(DAWorkFlowGraphicsScene* scene, const QDomElement& workflowEle);
    //
public:
    QSet< QGraphicsItem* > _haveBeenSaveNodeItem;  ///< 记录已经被保存过的node item
};

DAXmlHelperPrivate::DAXmlHelperPrivate(DAXmlHelper* p) : q_ptr(p)
{
}

void DAXmlHelperPrivate::saveWorkflow(DAWorkFlowEditWidget* wfe, QDomDocument& doc, QDomElement& workflowEle)
{
    // 保存工作流的扩展信息
    QElapsedTimer tes;
    tes.start();
    _haveBeenSaveNodeItem.clear();  // 清空保存过的item的记录
    QDomElement externEle                  = doc.createElement("extern");
    DAWorkFlow* workflow                   = wfe->getWorkflow();
    DAWorkFlowGraphicsScene* workFlowScene = wfe->getWorkFlowGraphicsScene();
    workflow->saveExternInfoToXml(&doc, &externEle);
    workflowEle.appendChild(externEle);
    qDebug() << QObject::tr("save workflow extern info cost: %1 ms").arg(tes.restart());
    // 先获取所有的item，在保存过程中，如果item保存了，就删掉
    QList< QGraphicsItem* > allItems = workFlowScene->items();
    // 保存所有节点
    saveNodes(wfe, doc, workflowEle);
    qDebug() << QObject::tr("save workflow nodes cost: %1 ms").arg(tes.restart());
    // 保存所有连接
    saveNodeLinks(workflow, doc, workflowEle);
    qDebug() << QObject::tr("save workflow links cost: %1 ms").arg(tes.restart());
    // 保存特殊的item。例如文本
    saveSpecialItem(workFlowScene, doc, workflowEle);
    qDebug() << QObject::tr("save special item cost: %1 ms").arg(tes.restart());
    // 保存工厂相关信息，包括工厂的扩展信息，工厂的信息一般在node和link之后保存
    saveFactoryInfo(workflow, doc, workflowEle);
    qDebug() << QObject::tr("save workflow factory info cost: %1 ms").arg(tes.restart());
    // 保存scene信息
    saveSecenInfo(workFlowScene, doc, workflowEle);
    qDebug() << QObject::tr("save secen info cost: %1 ms").arg(tes.restart());
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
    //

    QDomElement externEle = workflowEle.firstChildElement("extern");
    workflow->loadExternInfoFromXml(&externEle);
    qDebug() << QObject::tr("load workflow extern info cost: %1 ms").arg(tes.restart());

    if (!loadNodes(wfe, workflowEle)) {
        qCritical() << QObject::tr("load nodes occurce error");
    }
    qDebug() << QObject::tr("load workflow nodes cost: %1 ms").arg(tes.restart());

    if (!loadNodeLinks(workFlowScene, workflow, workflowEle)) {
        qCritical() << QObject::tr("load nodes link occurce error");
    }
    qDebug() << QObject::tr("load workflow links cost: %1 ms").arg(tes.restart());

    if (!loadSpecialItem(workFlowScene, workflowEle)) {
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
    QList< DAAbstractNodeFactory* > factorys = workflow->getAllFactorys();
    // 创建节点
    QDomElement factorysEle = doc.createElement("factorys");
    for (const DAAbstractNodeFactory* fac : qAsConst(factorys)) {
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
        QString factoryPrototypes  = factoryEle.attribute("prototypes");
        DAAbstractNodeFactory* fac = workflow->getFactory(factoryPrototypes);
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

    QDomElement nodesEle = doc.createElement("nodes");
    for (const DAAbstractNode::SharedPointer& node : qAsConst(nodes)) {
        qDebug() << "save node,id=" << node->getID() << ",protoType=" << node->getNodePrototype();
        QDomElement nodeEle = doc.createElement("node");
        nodeEle.setAttribute("id", node->getID());
        nodeEle.setAttribute("name", node->getNodeName());
        nodeEle.setAttribute("protoType", node->getNodePrototype());

        // 保存节点input和output的key和propertys
        saveNodeInputOutput(node, doc, nodeEle);
        // 保存节点属性
        saveNodePropertys(node, doc, nodeEle);
        // 保存额外信息
        node->saveExternInfoToXml(&doc, &nodeEle);
        // 添加节点Item信息
        saveNodeItem(node, doc, nodeEle);
        nodesEle.appendChild(nodeEle);
    }
    workflowEle.appendChild(nodesEle);
}

bool DAXmlHelperPrivate::loadNodes(DAWorkFlowEditWidget* wfe, const QDomElement& workflowEle)
{
    DAWorkFlow* workflow                   = wfe->getWorkflow();
    DAWorkFlowGraphicsScene* workFlowScene = wfe->getWorkFlowGraphicsScene();
    QDomElement nodesEle                   = workflowEle.firstChildElement("nodes");
    QDomNodeList nodeslist                 = nodesEle.childNodes();

    for (int i = 0; i < nodeslist.size(); ++i) {
        QDomElement nodeEle = nodeslist.at(i).toElement();
        if (nodeEle.tagName() != "node") {
            qDebug() << "nodeEle.tagName()=" << nodeEle.tagName() << ",skip and continue";
            continue;
        }
        bool isok     = false;
        qulonglong id = nodeEle.attribute("id").toULongLong(&isok);
        if (!isok) {
            qWarning() << QObject::tr("node's id=%1 can not conver to qulonglong type ,will skip this node").arg(nodeEle.attribute("id"));
            continue;
        }
        if (workflow->hasNodeID(id)) {
            qDebug() << "duplicate node id";
            continue;
        }
        QString name      = nodeEle.attribute("name");
        QString protoType = nodeEle.attribute("protoType");
        // 创建节点
        DANodeMetaData metadata            = workflow->getNodeMetaData(protoType);
        DAAbstractNode::SharedPointer node = workflow->createNode(metadata);
        if (nullptr == node) {
            qWarning() << QObject::tr("workflow can not create note by "
                                      "metadata(prototype=%1,name=%2,group=%3),will skip this node")
                                  .arg(metadata.getNodePrototype(), metadata.getNodeName(), metadata.getGroup());
            continue;
        }

        node->setID(id);
        node->setNodeName(name);
        // 加载节点的输入输出
        loadNodeInPutOutputKey(node, nodeEle);
        // 加载节点的属性
        loadNodePropertys(node, nodeEle);
        // 加载额外信息
        node->loadExternInfoFromXml(&nodeEle);
        // 加载item
        loadNodeItem(workFlowScene, node, nodeEle);
        // 最后再添加
        workflow->addNode(node);
    }
    return true;
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
        QDomElement inputEle = doc.createElement("input");
        QDomElement nameEle  = doc.createElement("name");
        QDomText nameText    = doc.createTextNode(key);
        nameEle.appendChild(nameText);
        inputEle.appendChild(nameEle);
        QVariant v = node->getInputData(key);
        if (v.isValid()) {
            QDomElement varEle = DAXmlHelper::createVariantValueElement(doc, v);
            inputEle.appendChild(varEle);
        }
        inputsEle.appendChild(inputEle);
    }

    QDomElement outputsEle      = doc.createElement("outputs");
    QList< QString > outputKeys = node->getOutputKeys();
    for (const auto& key : qAsConst(outputKeys)) {
        // 添加output
        QDomElement outputEle = doc.createElement("output");
        QDomElement nameEle   = doc.createElement("name");
        QDomText nameText     = doc.createTextNode(key);
        nameEle.appendChild(nameText);
        outputEle.appendChild(nameEle);
        QVariant v = node->getOutputData(key);
        if (v.isValid()) {
            QDomElement varEle = DAXmlHelper::createVariantValueElement(doc, v);
            outputEle.appendChild(varEle);
        }
        outputsEle.appendChild(outputEle);
    }
    nodeEle.appendChild(inputsEle);
    nodeEle.appendChild(outputsEle);
}

bool DAXmlHelperPrivate::loadNodeInPutOutputKey(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode)
{
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
                                      .arg(node->getNodePrototype(), node->getNodeName(), node->getNodeGroup(), ks.at(i).nodeName());
                continue;
            }
            QString key = nameEle.text();
            node->addInputKey(key);
            QDomElement variantEle = inputEle.firstChildElement("variant");
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
                                      .arg(node->getNodePrototype(), node->getNodeName(), node->getNodeGroup(), ks.at(i).nodeName());
                continue;
            }
            QString key = nameEle.text();
            node->addOutputKey(key);
            QDomElement variantEle = outputEle.firstChildElement("variant");
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
 * @param node
 * @param doc
 * @param nodeEle
 */
void DAXmlHelperPrivate::saveNodePropertys(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle)
{
    QDomElement propertysEle      = doc.createElement("propertys");
    QList< QString > propertyKeys = node->getPropertyKeys();
    for (const QString& k : qAsConst(propertyKeys)) {
        QVariant v = node->getProperty(k);
        if (!v.isValid()) {
            continue;
        }
        QDomElement nameEle = doc.createElement("name");
        QDomText nameVal    = doc.createTextNode(k);
        nameEle.appendChild(nameVal);
        QDomElement varEle      = DAXmlHelper::createVariantValueElement(doc, v);
        QDomElement propertyEle = doc.createElement("property");
        propertyEle.appendChild(nameEle);
        propertyEle.appendChild(varEle);
        propertysEle.appendChild(propertyEle);
    }
    nodeEle.appendChild(propertysEle);
}

bool DAXmlHelperPrivate::loadNodePropertys(DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle)
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
    QDomElement itemEle              = doc.createElement("item");
    item->saveToXml(&doc, &itemEle);
    nodeEle.appendChild(itemEle);
    _haveBeenSaveNodeItem.insert(item);
}

bool DAXmlHelperPrivate::loadNodeItem(DAWorkFlowGraphicsScene* scene, DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle)
{
    QDomElement ele = nodeEle.firstChildElement("item");
    if (ele.isNull()) {
        return false;
    }
    DAAbstractNodeGraphicsItem* item = node->createGraphicsItem();
    if (nullptr == item) {
        qWarning() << QObject::tr("node metadata(prototype=%1,name=%2,group=%3) can not create graphics item")
                              .arg(node->getNodePrototype(), node->getNodeName(), node->getNodeGroup());
        return false;
    }
    item->loadFromXml(&ele);
    scene->addItem(item);
    return true;
}

void DAXmlHelperPrivate::saveNodeLinks(const DAWorkFlow* workflow, QDomDocument& doc, QDomElement& workflowEle)
{
    const QList< DAAbstractNode::SharedPointer >& nodes = workflow->nodes();
    //
    QDomElement nodeLinkEle = doc.createElement("links");

    QSet< DAAbstractNodeLinkGraphicsItem* > itemSet;
    for (auto node : nodes) {
        auto links = node->graphicsItem()->getLinkItems();
        for (auto link : qAsConst(links)) {
            itemSet.insert(link);
        }
    }

    for (auto link : itemSet) {
        QDomElement linkEle = doc.createElement("link");
        QDomElement fromEle = doc.createElement("from");
        fromEle.setAttribute("id", link->fromNode()->getID());
        fromEle.setAttribute("name", link->fromNodeLinkPoint().name);
        QDomElement toEle = doc.createElement("to");
        toEle.setAttribute("id", link->toNode()->getID());
        toEle.setAttribute("name", link->toNodeLinkPoint().name);

        linkEle.appendChild(fromEle);
        linkEle.appendChild(toEle);

        if (!link->saveToXml(&doc, &linkEle)) {
            qWarning() << QObject::tr("linkitem save to xml return false");  // cn:链接线从xml加载信息返回了false
        }

        nodeLinkEle.appendChild(linkEle);
        _haveBeenSaveNodeItem.insert(link);  // 加入以保存列表避免重复保存
    }

    workflowEle.appendChild(nodeLinkEle);
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

        qulonglong id                          = fromEle.attribute("id").toULongLong(&ok);
        QString fromKey                        = fromEle.attribute("name");
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
        DAAbstractNodeLinkGraphicsItem* linkitem = fromItem->linkTo(fromKey, toItem, toKey);
        if (nullptr == linkitem) {
            qWarning() << QObject::tr("Unable to link to node %3's link point %4 through link point %2 of node %1")  // cn:节点%1无法通过连接点%2链接到节点%3的连接点%4
                                  .arg(fromItem->getNodeName(), fromKey, toItem->getNodeName(), toKey);
            continue;
        }
        if (!linkitem->loadFromXml(&linkEle)) {
            qWarning() << QObject::tr("linkitem load from xml return false")  // cn:链接线从xml加载信息返回了false
                    ;
        }
        scene->addItem(linkitem);
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
void DAXmlHelperPrivate::saveSpecialItem(const DAWorkFlowGraphicsScene* scene, QDomDocument& doc, QDomElement& workflowEle)
{  // 保存文本
    QList< QGraphicsItem* > items = scene->items();
    QDomElement itemsElement      = doc.createElement("items");
    // 背景不作为items保存
    DAGraphicsResizeablePixmapItem* bkItem = scene->getBackgroundPixmapItem();
    for (const QGraphicsItem* i : qAsConst(items)) {
        if (i == bkItem) {
            // 背景图片这个特殊的item不在items里保存
            continue;
        }
        if (_haveBeenSaveNodeItem.contains(const_cast< QGraphicsItem* >(i))) {
            // 已经保存过的不进行保存
            continue;
        }
        const DAXMLFileInterface* xml = dynamic_cast< const DAXMLFileInterface* >(i);
        if (nullptr == xml) {
            // 非xml文件系统
            continue;
        }
        switch (i->type()) {
        case DA::ItemType_GraphicsStandardTextItem: {
            // 文本
            QDomElement itemElement = doc.createElement("item");
            itemElement.setAttribute("className", "DA::DAStandardGraphicsTextItem");
            const DAStandardGraphicsTextItem* ti = static_cast< const DAStandardGraphicsTextItem* >(i);
            ti->saveToXml(&doc, &itemElement);
            itemsElement.appendChild(itemElement);
        } break;
        default: {
            const DAGraphicsItem* obj = dynamic_cast< const DAGraphicsItem* >(i);
            if (nullptr == obj) {
                qWarning() << QObject::tr("There is a item that is not a DA Graphics Item system and cannot be saved");  // 存在不是DA Graphics Item系统的元件，无法对此元件进行保存
                continue;
            }
            QDomElement itemElement = doc.createElement("item");
            itemElement.setAttribute("className", obj->metaObject()->className());
            xml->saveToXml(&doc, &itemElement);
            itemsElement.appendChild(itemElement);
        }
        }
    }
    workflowEle.appendChild(itemsElement);
}
/**
 * @brief 加载特殊的item
 * @param scene
 * @param workflowEle
 * @return
 */
bool DAXmlHelperPrivate::loadSpecialItem(DAWorkFlowGraphicsScene* scene, const QDomElement& workflowEle)
{
    QDomElement specialItemsElement = workflowEle.firstChildElement("items");
    DAGraphicsItemFactory itemFactory;
    QDomNodeList childNodes = specialItemsElement.childNodes();
    for (int i = 0; i < childNodes.size(); ++i) {
        QDomElement itemEle = childNodes.at(i).toElement();
        if (itemEle.tagName() == "item") {
            QString className = itemEle.attribute("className");
            if (className.isEmpty()) {
                continue;
            }
            if (className == "DA::DAStandardGraphicsTextItem") {
                DAStandardGraphicsTextItem* item = new DAStandardGraphicsTextItem();
                if (!item->loadFromXml(&itemEle)) {
                    qWarning() << QObject::tr("Unable to load item information from file").arg(className);  // 无法通过文件加载元件信息
                } else {
                    scene->addItem(item);
                }
            } else {
                DAGraphicsItem* item = itemFactory.createItem(className);
                if (nullptr == item) {
                    qWarning() << QObject::tr("Cannot create item by class name:%1").arg(className);  // 无法通过类名:%1创建元件
                    continue;
                } else {
                    if (!item->loadFromXml(&itemEle)) {
                        qWarning() << QObject::tr("Unable to load item information from file").arg(className);  // 无法通过文件加载元件信息
                        itemFactory.destoryItem(item);
                    } else {
                        scene->addItem(item);
                    }
                }
            }
        }
    }
    return true;
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
        item->saveToXml(&doc, &imageEle);
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
        QScopedPointer< DAGraphicsResizeablePixmapItem > item(new DAGraphicsResizeablePixmapItem());
        if (item->loadFromXml(&imageEle)) {
            scene->setBackgroundPixmapItem(item.take());
        }
    }
    return true;
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
 * @brief ResizeableItem的通用保存
 * @param item
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXmlHelper::makeElement(DAGraphicsResizeableItem* item, const QString& tagName, QDomDocument* doc)
{
    QDomElement tag = doc->createElement(tagName);
    item->saveToXml(doc, &tag);
    return tag;
}

/**
 * @brief ResizeableItem的通用保存
 * @param item
 * @param tag
 * @return
 */
bool DAXmlHelper::loadElement(DAGraphicsResizeableItem* item, const QDomElement* tag)
{
    return item->loadFromXml(tag);
}

/**
 * @brief 生成一个qvariant element
 * @param doc
 * @param v
 * @return
 */
QDomElement DAXmlHelper::createVariantValueElement(QDomDocument& doc, const QVariant& var)
{
    return DAXMLFileInterface::makeElement(var, "variant", &doc);
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
