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
#include "DAQtEnumTypeStringUtils.h"
#include "DAGraphicsViewEnumStringUtils.h"
#include "DAWorkFlowEnumStringUtils.h"
#include "DAGuiEnumStringUtils.h"
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
#include "DAChartOperateWidget.h"
#include "DAFigureWidget.h"
#include "DAChartWidget.h"
#include "DAChartItemsManager.h"
#include "DAChartAxisRangeBinder.h"
// qwt
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_draw.h"
#include "qwt_date_scale_draw.h"
#include "qwt_date_scale_engine.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_layout.h"
#include "qwt_scale_widget.h"
namespace DA
{
//==============================================================
// DAXmlHelperPrivate
//==============================================================
class DAXmlHelper::PrivateData
{
    DA_DECLARE_PUBLIC(DAXmlHelper)
public:
    PrivateData(DAXmlHelper* p);

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
    QDomElement
    makeNodesElement(const QList< DAAbstractNode::SharedPointer >& nodes, const QString& tagName, QDomDocument& doc);
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

DAXmlHelper::PrivateData::PrivateData(DAXmlHelper* p) : q_ptr(p)
{
}

void DAXmlHelper::PrivateData::saveWorkflow(DAWorkFlowEditWidget* wfe, QDomDocument& doc, QDomElement& workflowEle)
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
bool DAXmlHelper::PrivateData::loadWorkflow(DAWorkFlowEditWidget* wfe, const QDomElement& workflowEle)
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

void DAXmlHelper::PrivateData::saveWorkflowFromClipBoard(const QList< DAGraphicsItem* > its,
                                                         QDomDocument& doc,
                                                         QDomElement& workflowEle)
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
    for (DAAbstractNodeGraphicsItem* i : std::as_const(nodeItems)) {
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
    for (auto li : std::as_const(linkItems)) {
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
bool DAXmlHelper::PrivateData::loadWorkflowFromClipBoard(DAWorkFlowGraphicsScene* scene,
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
void DAXmlHelper::PrivateData::saveFactoryInfo(const DAWorkFlow* workflow, QDomDocument& doc, QDomElement& workflowEle)
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
bool DAXmlHelper::PrivateData::loadFactoryInfo(DAWorkFlow* workflow, const QDomElement& workflowEle)
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

void DAXmlHelper::PrivateData::saveNodes(const DAWorkFlowEditWidget* wfe, QDomDocument& doc, QDomElement& workflowEle)
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
QDomElement DAXmlHelper::PrivateData::makeNodesElement(const QList< DAAbstractNode::SharedPointer >& nodes,
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
QDomElement DAXmlHelper::PrivateData::makeNodeElement(const DAAbstractNode::SharedPointer& node,
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
bool DAXmlHelper::PrivateData::loadNodes(DAWorkFlow* workflow,
                                         DAWorkFlowGraphicsScene* workFlowScene,
                                         const QDomElement& workflowEle)
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
bool DAXmlHelper::PrivateData::loadNodesClipBoard(DAWorkFlowGraphicsScene* scene,
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
DAAbstractNode::SharedPointer DAXmlHelper::PrivateData::loadNode(const QDomElement& nodeEle, DAWorkFlow* workflow, bool isLoadID)
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

DAAbstractNodeGraphicsItem* DAXmlHelper::PrivateData::loadNodeAndItem(const QDomElement& nodeEle,
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
    DANodeMetaData metadata            = workflow->getNodeMetaData(protoType);
    DAAbstractNode::SharedPointer node = workflow->createNode(metadata);
    if (!node) {
        qWarning() << QObject::tr("Unable to create node by metadata(prototype=%1,name=%2,group=%3)-2")
                          .arg(metadata.getNodePrototype(),
                               metadata.getNodeName(),
                               metadata.getGroup());  // cn:无法通过元数据创建节点(类型=%1,名称=%2,分组=%3)创建节点-2
        return nullptr;
    }
    node->setID(id);
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
    workflow->addNode(node);
    // 加载item信息
    // 注意，这里时createNode，不是createNode_
    std::unique_ptr< DAAbstractNodeGraphicsItem > item(node->createGraphicsItem());
    if (!item) {
        qWarning() << QObject::tr("Unable to create node by metadata(prototype=%1,name=%2,group=%3)-1")
                          .arg(metadata.getNodePrototype(),
                               metadata.getNodeName(),
                               metadata.getGroup());  // cn:无法通过元数据创建节点(类型=%1,名称=%2,分组=%3)创建节点-1
        return nullptr;
    }

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
DAAbstractNodeGraphicsItem* DAXmlHelper::PrivateData::loadNodeAndItemWithUndo(const QDomElement& nodeEle,
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
void DAXmlHelper::PrivateData::saveNodeInputOutput(const DAAbstractNode::SharedPointer& node,
                                                   QDomDocument& doc,
                                                   QDomElement& nodeEle)
{
    QDomElement inputsEle      = doc.createElement("inputs");
    QList< QString > inputKeys = node->getInputKeys();
    for (const auto& key : std::as_const(inputKeys)) {
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
    for (const auto& key : std::as_const(outputKeys)) {
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

bool DAXmlHelper::PrivateData::loadNodeInPutOutputKey(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode)
{
    if (mLoadedVersion.majorVersion() == 1 && mLoadedVersion.minorVersion() < 3) {
        return loadNodeInPutOutputKey_v110(node, eleNode);
    } else if (mLoadedVersion.majorVersion() >= 1 && mLoadedVersion.minorVersion() > 3) {
        return loadNodeInPutOutputKey_v130(node, eleNode);
    }
    return true;
}

bool DAXmlHelper::PrivateData::loadNodeInPutOutputKey_v110(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode)
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

bool DAXmlHelper::PrivateData::loadNodeInPutOutputKey_v130(DAAbstractNode::SharedPointer& node, const QDomElement& eleNode)
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
void DAXmlHelper::PrivateData::saveNodePropertys(const DAAbstractNode::SharedPointer& node,
                                                 QDomDocument& doc,
                                                 QDomElement& nodeEle)
{
    const QHash< QString, QVariant >& props = node->propertys();
    savePropertys(props, doc, nodeEle);
    //    QDomElement propertysEle      = doc.createElement("props");
    //	QList< QString > propertyKeys = node->getPropertyKeys();
    //	for (const QString& k : std::as_const(propertyKeys)) {
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

bool DAXmlHelper::PrivateData::loadNodePropertys(DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle)
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

bool DAXmlHelper::PrivateData::loadNodePropertys_v110(DAAbstractNode::SharedPointer& node, const QDomElement& nodeEle)
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

void DAXmlHelper::PrivateData::saveNodeItem(const DAAbstractNode::SharedPointer& node, QDomDocument& doc, QDomElement& nodeEle)
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
DAAbstractNodeGraphicsItem* DAXmlHelper::PrivateData::loadNodeItem(DAAbstractNode::SharedPointer& node,
                                                                   const QDomElement& nodeEle)
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

void DAXmlHelper::PrivateData::saveNodeLinks(const DAWorkFlow* workflow, QDomDocument& doc, QDomElement& workflowEle)
{
    const QList< DAAbstractNode::SharedPointer >& nodes = workflow->nodes();
    //
    QDomElement nodeLinkEle = doc.createElement("links");

    QSet< DAAbstractNodeLinkGraphicsItem* > itemSet;
    for (const auto& node : nodes) {
        auto links = node->graphicsItem()->getLinkItems();
        for (auto link : std::as_const(links)) {
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
QDomElement DAXmlHelper::PrivateData::makeNodeLinkElement(DAAbstractNodeLinkGraphicsItem* link,
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

bool DAXmlHelper::PrivateData::loadNodeLinks(DAWorkFlowGraphicsScene* scene, DAWorkFlow* wf, const QDomElement& workflowEle)
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

bool DAXmlHelper::PrivateData::loadNodeLinksClipBoardCopy(DAWorkFlowGraphicsScene* scene,
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
        scene->addNodeLink_(linkitem);
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
void DAXmlHelper::PrivateData::saveCommonItems(const DAWorkFlowGraphicsScene* scene, QDomDocument& doc, QDomElement& workflowEle)
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
QDomElement DAXmlHelper::PrivateData::makeCommonItemsElement(const QList< QGraphicsItem* >& items,
                                                             const QString& tagName,
                                                             QDomDocument& doc)
{
    QDomElement itemsElement = doc.createElement(tagName);
    // 背景不作为items保存
    for (const QGraphicsItem* i : std::as_const(items)) {
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
bool DAXmlHelper::PrivateData::loadCommonItems(DAWorkFlowGraphicsScene* scene, const QDomElement& workflowEle, bool isRedoUndo)
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
bool DAXmlHelper::PrivateData::saveItem(const QGraphicsItem* i, QDomDocument& doc, QDomElement& parentElement)
{
    if (const DAGraphicsItemGroup* gi = dynamic_cast< const DAGraphicsItemGroup* >(i)) {
        saveItemGroup(gi, doc, parentElement);
    } else {
        auto itemEle = DAXmlHelper::makeElement(i, QStringLiteral("item"), &doc);
        if (itemEle.isNull()) {
            qDebug() << QObject::tr("Unable to generate graphics item element during the saveing");  // cn:保存过程中，无法生成图元元素
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
QGraphicsItem* DAXmlHelper::PrivateData::loadItem(const QDomElement& itemElement)
{
    QGraphicsItem* item = DAXmlHelper::loadItemElement(&itemElement, mLoadedVersion);
    if (!item) {
        return nullptr;
    }
    return item;
}

bool DAXmlHelper::PrivateData::loadItem(QGraphicsItem* item, const QDomElement& itemElement)
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
QDomElement DAXmlHelper::PrivateData::findItemElement(const QDomElement& parentElement)
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
void DAXmlHelper::PrivateData::saveItemGroup(const DAGraphicsItemGroup* itemGroup, QDomDocument& doc, QDomElement& parentElement)
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

DAGraphicsItemGroup* DAXmlHelper::PrivateData::loadItemGroup(DAWorkFlowGraphicsScene* scene, const QDomElement& groupElement)
{
    std::unique_ptr< DAGraphicsItemGroup > group = std::make_unique< DAGraphicsItemGroup >();
    if (!DAXmlHelper::loadElement(scene, group.get(), &groupElement)) {
        return nullptr;
    }
    scene->addItem(group.get());
    return group.release();
}

void DAXmlHelper::PrivateData::saveSecenInfo(DAWorkFlowGraphicsScene* scene, QDomDocument& doc, QDomElement& workflowEle)
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

bool DAXmlHelper::PrivateData::loadSecenInfo(DAWorkFlowGraphicsScene* scene, const QDomElement& workflowEle)
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

void DAXmlHelper::PrivateData::savePropertys(const QHash< QString, QVariant >& props, QDomDocument& doc, QDomElement& parentEle)
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

bool DAXmlHelper::PrivateData::loadPropertys(QHash< QString, QVariant >& props, const QDomElement& parentEle)
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

void DAXmlHelper::PrivateData::clearDealItemSet()
{
    mHaveBeenDealNodeItem.clear();
}

void DAXmlHelper::PrivateData::recordDealItem(QGraphicsItem* i)
{
    mHaveBeenDealNodeItem.insert(i);
}

void DAXmlHelper::PrivateData::recordDealItem(const QGraphicsItem* i)
{
    mHaveBeenDealNodeItem.insert(const_cast< QGraphicsItem* >(i));
}

bool DAXmlHelper::PrivateData::isItemHaveDeal(QGraphicsItem* i) const
{
    return mHaveBeenDealNodeItem.contains(i);
}

bool DAXmlHelper::PrivateData::isItemHaveDeal(const QGraphicsItem* i) const
{
    return mHaveBeenDealNodeItem.contains(const_cast< QGraphicsItem* >(i));
}

//==============================================================
// DAXmlHelper
//==============================================================
DAXmlHelper::DAXmlHelper() : DA_PIMPL_CONSTRUCT
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
    Q_UNUSED(v);
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
 * @brief DAColorTheme
 * @param ct
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXmlHelper::makeElement(const DAColorTheme* ct, const QString& tagName, QDomDocument* doc)
{
    QDomElement ele = doc->createElement(tagName);
    ele.setAttribute("style", enumToString(ct->getColorThemeStyle()));
    if (ct->getColorThemeStyle() == DAColorTheme::Style_UserDefine) {
        const int size = ct->size();
        for (int i = 0; i < size; ++i) {
            QDomElement eleColor = doc->createElement("li");
            eleColor.appendChild(doc->createTextNode(ct->at(i).name()));
            ele.appendChild(eleColor);
        }
    }
    return ele;
}

bool DAXmlHelper::loadElement(DAColorTheme* ct, const QDomElement* tag, const QVersionNumber& v)
{
    Q_UNUSED(v);
    QString styleStr                    = tag->attribute("style");
    DAColorTheme::ColorThemeStyle style = stringToEnum(styleStr, DAColorTheme::Style_Archambault);
    ct->setColorThemeStyle(style);
    DAColorTheme::ColorList clrVec;
    if (style == DAColorTheme::Style_UserDefine) {
        auto childNodes = tag->childNodes();
        for (int i = 0; i < childNodes.size(); ++i) {
            QDomElement eleColor = childNodes.at(i).toElement();
            QColor c(eleColor.text());
            if (c.isValid()) {
                clrVec.push_back(c);
            }
        }
        ct->setUserDefineColorList(clrVec, DAColorTheme::Style_UserDefine);
    }
    return true;
}

/**
 * @brief DAChartOperateWidget的序列化
 * @param chartOpt
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXmlHelper::makeElement(DAChartOperateWidget* chartOpt,
                                     const QString& tagName,
                                     QDomDocument* doc,
                                     DAChartItemsManager* itemsMgr)
{
    int figCnt                = chartOpt->getFigureCount();
    QDomElement chartsElement = doc->createElement(tagName);
    for (int i = 0; i < figCnt; ++i) {
        DAFigureWidget* fig = chartOpt->getFigure(i);
        if (!fig) {
            qCritical() << QObject::tr("unknow except:get null figure widget at %1").arg(i);
            continue;
        }
        QDomElement figEle = makeElement(fig, QStringLiteral("figure"), doc, itemsMgr);
        figEle.setAttribute(QStringLiteral("figure-name"), chartOpt->getFigureName(i));
        chartsElement.appendChild(figEle);
    }
    return chartsElement;
}

bool DAXmlHelper::loadElement(DAChartOperateWidget* chartOpt,
                              const QDomElement* tag,
                              const DAChartItemsManager* itemsMgr,
                              const QVersionNumber& v)
{
    auto childs = tag->childNodes();
    for (int i = 0; i < childs.size(); ++i) {
        QDomElement figEle = childs.at(i).toElement();
        if (figEle.isNull()) {
            continue;
        }
        DAFigureWidget* fig = chartOpt->createFigure();
        if (!loadElement(fig, &figEle, itemsMgr)) {
            // 加载失败，删除窗口
            qDebug() << "load figure error";
            chartOpt->removeFigure(fig, true);
        }
        // 获取窗口名字
        QString figName = figEle.attribute(QStringLiteral("figure-name"));
        chartOpt->setFigureName(fig, figName);
    }
    return true;
}

/**
 * @brief DAFigureWidget的序列化
 * @param fig
 * @param tagName
 * @param doc
 * @return
 */
QDomElement
DAXmlHelper::makeElement(const DAFigureWidget* fig, const QString& tagName, QDomDocument* doc, DAChartItemsManager* itemsMgr)
{
    QwtFigure* qwtFig = fig->figure();
    if (!qwtFig) {
        return QDomElement();
    }
    QDomElement eleFig = doc->createElement(tagName);
    eleFig.setAttribute(QStringLiteral("id"), fig->getFigureId());
    // background
    QDomElement eleBKBrush = DAXMLFileInterface::makeElement(fig->getBackgroundColor(), QStringLiteral("background"), doc);
    eleFig.appendChild(eleBKBrush);
    // colortheme
    const DAColorTheme cth  = fig->getColorTheme();
    QDomElement eleClrTheme = makeElement(&cth, QStringLiteral("colortheme"), doc);
    eleFig.appendChild(eleClrTheme);
    // 记录chart
    QDomElement chartsEle                = doc->createElement(QStringLiteral("charts"));
    const QList< DAChartWidget* > charts = fig->getCharts();
    for (DAChartWidget* chart : charts) {
        QDomElement chartEle = makeElement(chart, QStringLiteral("chart"), doc, itemsMgr);

        QRectF pos = fig->axesNormRect(chart);
        chartEle.setAttribute(QStringLiteral("x"), pos.x());
        chartEle.setAttribute(QStringLiteral("y"), pos.y());
        chartEle.setAttribute(QStringLiteral("w"), pos.width());
        chartEle.setAttribute(QStringLiteral("h"), pos.height());

        chartsEle.appendChild(chartEle);
    }
    eleFig.appendChild(chartsEle);
    // 记录绑定关系
    const QList< DAChartAxisRangeBinder* > axisRangeBinders = fig->getBindAxisRangeInfos();
    if (axisRangeBinders.size() > 0) {
        QDomElement axisRangeBindersEle = doc->createElement(QStringLiteral("axisRangeBinders"));
        for (DAChartAxisRangeBinder* b : axisRangeBinders) {
            QDomElement axisRangeBinderEle = makeElement(b, QStringLiteral("bind"), doc);
            axisRangeBindersEle.appendChild(axisRangeBinderEle);
        }
        eleFig.appendChild(axisRangeBindersEle);
    }
    // 记录轴对齐
    const int alimentCnt = qwtFig->axisAligmentCount();
    if (alimentCnt > 0) {
        QDomElement axisAlimentEle = doc->createElement(QStringLiteral("axisAliment"));
        for (int i = 0; i < alimentCnt; ++i) {
            QPair< QList< QwtPlot* >, int > alimentInfo = qwtFig->axisAligmentInfo(i);
            if (alimentInfo.first.size() <= 0) {
                continue;
            }
            QDomElement alimentInfoEle = doc->createElement(QStringLiteral("ali"));
            alimentInfoEle.setAttribute("axisid", alimentInfo.second);
            for (QwtPlot* p : std::as_const(alimentInfo.first)) {
                QDomElement plotEle = doc->createElement(QStringLiteral("p"));
                plotEle.setAttribute("id", p->plotId());
                alimentInfoEle.appendChild(plotEle);
            }
            axisAlimentEle.appendChild(alimentInfoEle);
        }
        eleFig.appendChild(axisAlimentEle);
    }
    return eleFig;
}

bool DAXmlHelper::loadElement(DAFigureWidget* fig,
                              const QDomElement* tag,
                              const DAChartItemsManager* itemsMgr,
                              const QVersionNumber& v)
{
    Q_UNUSED(v);
    QwtFigure* qwtFig = fig->figure();
    if (!qwtFig) {
        return false;
    }
    QString id = tag->attribute(QStringLiteral("id"));
    if (!id.isEmpty()) {
        fig->setFigureId(id);
    }
    auto eleBKBrush = tag->firstChildElement(QStringLiteral("background"));
    // background
    QBrush brush;
    if (DAXMLFileInterface::loadElement(brush, &eleBKBrush)) {
        fig->setBackgroundColor(brush);
    }
    // colortheme
    auto eleClrTheme = tag->firstChildElement(QStringLiteral("colortheme"));
    DAColorTheme cth;
    if (loadElement(&cth, &eleClrTheme)) {
        fig->setColorTheme(cth);
    }
    // chart
    QDomElement chartsEle = tag->firstChildElement(QStringLiteral("charts"));
    auto chartListEle     = chartsEle.childNodes();
    for (int i = 0; i < chartListEle.size(); ++i) {
        QDomElement chartEle = chartListEle.at(i).toElement();
        if (chartEle.isNull()) {
            continue;
        }
        if (chartEle.tagName() != QStringLiteral("chart")) {
            continue;
        }

        std::unique_ptr< DAChartWidget > chart(fig->getChartFactory()->createChart());

        // 获取位置
        QRectF pos;
        pos.setX(chartEle.attribute(QStringLiteral("x")).toDouble());
        pos.setY(chartEle.attribute(QStringLiteral("y")).toDouble());
        pos.setWidth(chartEle.attribute(QStringLiteral("w")).toDouble());
        pos.setHeight(chartEle.attribute(QStringLiteral("h")).toDouble());
        if (!loadElement(chart.get(), &chartEle, itemsMgr)) {
            continue;
        }
        fig->addChart(chart.get(), pos);
        chart.release();
    }
    // 恢复绑定关系
    QDomElement axisRangeBindersEle = tag->firstChildElement(QStringLiteral("axisRangeBinders"));
    auto axisRangeBindersNodes      = axisRangeBindersEle.childNodes();
    for (int i = 0; i < axisRangeBindersNodes.size(); ++i) {
        QDomElement axisRangeEle = axisRangeBindersNodes.at(i).toElement();
        if (axisRangeEle.isNull()) {
            continue;
        }
        loadChartAxisRangeElement(fig, &axisRangeEle, v);
    }
    // 设置对齐信息
    QDomElement axisAlimentEle = tag->firstChildElement(QStringLiteral("axisAliment"));
    if (!axisAlimentEle.isNull()) {
        // 对齐信息
        auto alimentInfoNodes = axisAlimentEle.childNodes();
        for (int i = 0; i < alimentInfoNodes.size(); ++i) {
            QDomElement alimentInfoEle = alimentInfoNodes.at(i).toElement();
            if (alimentInfoEle.isNull()) {
                continue;
            }
            int axisid = alimentInfoEle.attribute("axisid", "4").toInt();
            QList< QwtPlot* > plots;
            if (!QwtAxis::isValid(axisid)) {
                continue;
            }
            auto plotNodes = alimentInfoEle.childNodes();
            for (int j = 0; j < plotNodes.size(); ++j) {
                QDomElement plotEle = plotNodes.at(j).toElement();
                if (plotEle.isNull()) {
                    continue;
                }
                QString id    = plotEle.attribute(QStringLiteral("id"));
                QwtPlot* plot = fig->findPlotById(id);
                if (plot) {
                    plots.push_back(plot);
                }
            }
            if (plots.size() > 0) {
                qwtFig->addAxisAlignment(plots, axisid);
            }
        }
    }
    return true;
}

/**
 * @brief DAChartWidget
 * @param chart
 * @param tagName
 * @param doc
 * @param itemsMgr 这是一个输出变量，会把chart里的item记录到DAChartItemsManager中，xml中只记录key
 * @return
 */
QDomElement DAXmlHelper::makeElement(const DAChartWidget* chart,
                                     const QString& tagName,
                                     QDomDocument* doc,
                                     DAChartItemsManager* itemsMgr)
{
    QDomElement chartEle = doc->createElement(tagName);
    // 记录id
    chartEle.setAttribute(QStringLiteral("id"), chart->plotId());
    //!====================
    //! plotLayout 不需要保存信息，根据窗口自动调整
    //!====================
    /*
    const QwtPlotLayout* layout = chart->plotLayout();
    if (layout) {
        QDomElement layoutEle = makeElement(layout, QStringLiteral("layout"), doc);
        chartEle.appendChild(layoutEle);
    }
    */

    //!====================
    //! title
    //!====================
    QwtText title = chart->title();
    if (!title.isNull() && !title.isEmpty()) {
        QDomElement titleEle = makeElement(&title, QStringLiteral("title"), doc);
        chartEle.appendChild(titleEle);
    }

    //!====================
    //!  Footer
    //!====================
    QwtText footer = chart->footer();
    if (!footer.isNull() && !footer.isEmpty()) {
        QDomElement footerEle = makeElement(&footer, QStringLiteral("footer"), doc);
        chartEle.appendChild(footerEle);
    }

    //!====================
    //! canvasBackground
    //!====================
    QDomElement canvasBackgroundEle =
        DAXMLFileInterface::makeElement(chart->canvasBackground(), QStringLiteral("canvasBackground"), doc);
    chartEle.appendChild(canvasBackgroundEle);

    //!====================
    //! axis
    //!====================
    // QwtAxis::YLeft
    QDomElement axisEle = makeQwtPlotAxisElement(chart, QwtAxis::YLeft, QStringLiteral("axis"), doc);
    chartEle.appendChild(axisEle);
    // QwtAxis::YRight
    axisEle = makeQwtPlotAxisElement(chart, QwtAxis::YRight, QStringLiteral("axis"), doc);
    chartEle.appendChild(axisEle);
    // QwtAxis::XBottom
    axisEle = makeQwtPlotAxisElement(chart, QwtAxis::XBottom, QStringLiteral("axis"), doc);
    chartEle.appendChild(axisEle);
    // QwtAxis::XTop
    axisEle = makeQwtPlotAxisElement(chart, QwtAxis::XTop, QStringLiteral("axis"), doc);
    chartEle.appendChild(axisEle);

    //!====================
    //! plot item
    //! 注意，这里仅仅保存plotitem的指针和映射关系，真实的保存将在线程中
    //!====================
    QDomElement itemsEle        = doc->createElement(QStringLiteral("items"));
    const QwtPlotItemList items = chart->itemList();
    for (QwtPlotItem* item : items) {
        QString key         = itemsMgr->recordItem(item);
        QDomElement itemEle = doc->createElement(QStringLiteral("item"));
        itemEle.setAttribute("key", key);
        itemsEle.appendChild(itemEle);
    }
    chartEle.appendChild(itemsEle);
    return chartEle;
}

bool DAXmlHelper::loadElement(DAChartWidget* chart,
                              const QDomElement* tag,
                              const DAChartItemsManager* itemsMgr,
                              const QVersionNumber& v)
{
    Q_UNUSED(v);
    QString id = tag->attribute(QStringLiteral("id"), QString());
    if (!id.isEmpty()) {
        chart->setPlotId(id);
    }
    // plotLayout
    QDomElement layoutEle = tag->firstChildElement(QStringLiteral("layout"));
    if (!layoutEle.isNull()) {
        QwtPlotLayout* layout = chart->plotLayout();
        if (!layout) {
            layout = new QwtPlotLayout();
            chart->setPlotLayout(layout);
        }
        loadElement(layout, &layoutEle);
    }
    // title
    QDomElement titleEle = tag->firstChildElement(QStringLiteral("title"));
    if (!titleEle.isNull()) {
        QwtText title;
        loadElement(&title, &titleEle);
        if (!title.isNull() && !title.isEmpty()) {
            chart->setTitle(title);
        }
    }
    // footer
    QDomElement footerEle = tag->firstChildElement(QStringLiteral("footer"));
    if (!footerEle.isNull()) {
        QwtText footer;
        loadElement(&footer, &footerEle);
        if (!footer.isNull() && !footer.isEmpty()) {
            chart->setFooter(footer);
        }
    }
    // canvasBackground
    QDomElement canvasBackgroundEle = tag->firstChildElement(QStringLiteral("canvasBackground"));
    if (!canvasBackgroundEle.isNull()) {
        QBrush brush;
        if (DAXMLFileInterface::loadElement(brush, &canvasBackgroundEle)) {
            chart->setCanvasBackground(brush);
        }
    }
    // axis
    QDomElement axisEle = tag->firstChildElement(QStringLiteral("axis"));
    while (!axisEle.isNull()) {
        loadQwtPlotAxisElement(chart, &axisEle);
        axisEle = axisEle.nextSiblingElement(QStringLiteral("axis"));
    }
    // item
    bool oldreplot = chart->autoReplot();
    chart->setAutoReplot(false);
    QDomElement itemsEle         = tag->firstChildElement(QStringLiteral("items"));
    QDomNodeList itemsChildNodes = itemsEle.childNodes();
    for (int i = 0; i < itemsChildNodes.size(); ++i) {
        QDomElement itemEle = itemsChildNodes.at(i).toElement();
        if (itemEle.isNull()) {
            continue;
        }
        QString key = itemEle.attribute(QStringLiteral("key"));
        if (key.isEmpty()) {
            continue;
        }
        QwtPlotItem* plotitem = itemsMgr->keyToItem(key);
        if (!plotitem) {
            continue;
        }
        plotitem->attach(chart);
        qDebug() << QString("plotitem(%1) attach chart(%2)").arg(plotitem->title().text()).arg(chart->title().text());
        if (plotitem->rtti() == QwtPlotItem::Rtti_PlotCurve) {
            QwtPlotCurve* cur = static_cast< QwtPlotCurve* >(plotitem);
            qDebug() << "plotitem isVisible=" << plotitem->isVisible() << ",cur size=" << cur->dataSize();
        }
    }
    chart->autoRefresh();
    chart->setAutoReplot(oldreplot);
    return true;
}

QDomElement DAXmlHelper::makeElement(const DAChartAxisRangeBinder* axisBinder, const QString& tagName, QDomDocument* doc)
{
    if (!axisBinder->isValid()) {
        // 异常
        return QDomElement();
    }
    QDomElement binderEle = doc->createElement(tagName);
    binderEle.setAttribute(QStringLiteral("source"), axisBinder->getSourcePlot()->plotId());
    binderEle.setAttribute(QStringLiteral("sourceAxis"), axisBinder->getSourceAxis());
    binderEle.setAttribute(QStringLiteral("follower"), axisBinder->getFollowerPlot()->plotId());
    binderEle.setAttribute(QStringLiteral("followerAxis"), axisBinder->getFollowerAxis());
    return binderEle;
}

bool DAXmlHelper::loadChartAxisRangeElement(DAFigureWidget* fig, const QDomElement* tag, const QVersionNumber& v)
{
    Q_UNUSED(v);
    QString sourcePlotId   = tag->attribute(QStringLiteral("source"));
    int sourceAxisId       = tag->attribute(QStringLiteral("sourceAxis"), "4").toInt();  // 4是无效
    QString followerPlotId = tag->attribute(QStringLiteral("follower"));
    int followerAxisId     = tag->attribute(QStringLiteral("followerAxis"), "4").toInt();
    // 找到QwtPlot
    QwtPlot* sourcePlot   = fig->findPlotById(sourcePlotId);
    QwtPlot* followerPlot = fig->findPlotById(followerPlotId);
    return fig->bindAxisRange(sourcePlot, sourceAxisId, followerPlot, followerAxisId);
}

QDomElement
DAXmlHelper::makeQwtPlotAxisElement(const DAChartWidget* chart, int axisID, const QString& tagName, QDomDocument* doc)
{
    QDomElement axisEle = doc->createElement(tagName);
    axisEle.setAttribute(QStringLiteral("axisID"), axisID);
    axisEle.setAttribute(QStringLiteral("axisType"), enumToString(static_cast< QwtAxis::Position >(axisID)));
    axisEle.setAttribute(QStringLiteral("visible"), chart->isAxisVisible(axisID));
    axisEle.setAttribute(QStringLiteral("autoScale"), chart->axisAutoScale(axisID));
    axisEle.setAttribute(QStringLiteral("maxMinor"), chart->axisMaxMajor(axisID));
    axisEle.setAttribute(QStringLiteral("maxMajor"), chart->axisMaxMajor(axisID));
    axisEle.setAttribute(QStringLiteral("stepSize"), chart->axisStepSize(axisID));
    QwtInterval axisInterval = chart->axisInterval(axisID);
    axisEle.setAttribute(QStringLiteral("min"), axisInterval.minValue());
    axisEle.setAttribute(QStringLiteral("max"), axisInterval.maxValue());

    //!====================
    //! font
    //!====================
    QFont f             = chart->axisFont(axisID);
    QDomElement fontEle = DAXMLFileInterface::makeElement(f, QStringLiteral("font"), doc);
    axisEle.appendChild(fontEle);

    //!====================
    //! title
    //!====================
    QwtText title = chart->axisTitle(axisID);
    if (!title.isNull() && !title.isEmpty()) {
        QDomElement titleEle = makeElement(&title, QStringLiteral("title"), doc);
        axisEle.appendChild(titleEle);
    }
    //!====================
    //! scaleWidget
    //!====================
    const QwtScaleWidget* scale = chart->axisWidget(axisID);
    QDomElement scaleWidgetEle  = makeElement(scale, "scaleWidget", doc);
    axisEle.appendChild(scaleWidgetEle);

    //!====================
    //! scaleDraw 这里主要处理日期时间轴和对数时间轴
    //!====================
    QDomElement scaleDrawEle      = doc->createElement(QStringLiteral("scaleDraw"));
    const QwtScaleDraw* scaleDraw = chart->axisScaleDraw(axisID);
    // 判断是否是时间坐标轴
    // QwtDateScaleEngine	生成刻度的位置和间隔 决定“刻度在哪里显示”
    // QwtDateScaleDraw	格式化刻度标签	决定“刻度如何显示为日期文本”
    if (const QwtDateScaleDraw* dateScaleDraw = dynamic_cast< const QwtDateScaleDraw* >(scaleDraw)) {
        // 时间坐标轴
        scaleDrawEle.setAttribute(QStringLiteral("type"), QStringLiteral("datetime"));
        QDomElement datescaleDrawEle = doc->createElement("datescale");
        datescaleDrawEle.setAttribute(QStringLiteral("timeSpec"), enumToString(dateScaleDraw->timeSpec()));
        datescaleDrawEle.setAttribute(QStringLiteral("utcOffset"), dateScaleDraw->utcOffset());
        datescaleDrawEle.setAttribute(QStringLiteral("week0Type"), enumToString(dateScaleDraw->week0Type()));
        // 保存时间坐标轴的其它设置
        QDomElement dateformatEle = doc->createElement(QStringLiteral("dateformat"));
        dateformatEle.appendChild(DAXMLFileInterface::makeElement(
            dateScaleDraw->dateFormat(QwtDate::Millisecond), QStringLiteral("msec"), doc));
        dateformatEle.appendChild(
            DAXMLFileInterface::makeElement(dateScaleDraw->dateFormat(QwtDate::Second), QStringLiteral("sec"), doc));
        dateformatEle.appendChild(
            DAXMLFileInterface::makeElement(dateScaleDraw->dateFormat(QwtDate::Minute), QStringLiteral("min"), doc));
        dateformatEle.appendChild(
            DAXMLFileInterface::makeElement(dateScaleDraw->dateFormat(QwtDate::Hour), QStringLiteral("hour"), doc));
        dateformatEle.appendChild(
            DAXMLFileInterface::makeElement(dateScaleDraw->dateFormat(QwtDate::Day), QStringLiteral("day"), doc));
        dateformatEle.appendChild(
            DAXMLFileInterface::makeElement(dateScaleDraw->dateFormat(QwtDate::Week), QStringLiteral("week"), doc));
        dateformatEle.appendChild(
            DAXMLFileInterface::makeElement(dateScaleDraw->dateFormat(QwtDate::Month), QStringLiteral("month"), doc));
        dateformatEle.appendChild(
            DAXMLFileInterface::makeElement(dateScaleDraw->dateFormat(QwtDate::Year), QStringLiteral("year"), doc));
        datescaleDrawEle.appendChild(dateformatEle);
        // 把date独有的添加
        scaleDrawEle.appendChild(datescaleDrawEle);
    } else {
        scaleDrawEle.setAttribute(QStringLiteral("type"), QStringLiteral("normal"));
    }
    // 普通坐标轴的属性
    scaleDrawEle.setAttribute(QStringLiteral("alignment"), enumToString(scaleDraw->alignment()));
    scaleDrawEle.setAttribute(QStringLiteral("labelAlignment"),
                              static_cast< int >(scaleDraw->labelAlignment()));  // 这里的对其是复合对其，无法转换为字符串
    scaleDrawEle.setAttribute(QStringLiteral("labelRotation"), scaleDraw->labelRotation());
    axisEle.appendChild(scaleDrawEle);
    //!====================
    //! QwtScaleEngine 这里主要处理日期时间刻度和对数刻度
    //!====================
    QDomElement scaleEngineEle        = doc->createElement(QStringLiteral("scaleEngine"));
    const QwtScaleEngine* scaleEngine = chart->axisScaleEngine(axisID);
    if (const QwtLinearScaleEngine* lineEngine = dynamic_cast< const QwtLinearScaleEngine* >(scaleEngine)) {
        // 说明是普通坐标
        scaleEngineEle.setAttribute(QStringLiteral("type"), QStringLiteral("line"));
    } else if (const QwtLogScaleEngine* logEngine = dynamic_cast< const QwtLogScaleEngine* >(scaleEngine)) {
        // 说明是对数坐标
        scaleEngineEle.setAttribute(QStringLiteral("type"), QStringLiteral("log"));
    } else if (const QwtDateScaleEngine* dateEngine = dynamic_cast< const QwtDateScaleEngine* >(scaleEngine)) {
        // 说明是对数坐标
        scaleEngineEle.setAttribute(QStringLiteral("type"), QStringLiteral("date"));
    }
    axisEle.appendChild(scaleEngineEle);
    return axisEle;
}

bool DAXmlHelper::loadQwtPlotAxisElement(DAChartWidget* chart, const QDomElement* qwtplotTag, const QVersionNumber& v)
{
    Q_UNUSED(v);
    QwtAxisId axisID = qwtplotTag->attribute(QStringLiteral("axisID")).toInt();
    //! 注意
    //! 这里的加载顺序稍微调整
    //! 1、线把scaleDraw加载，也就是先设置日期轴还是普通轴，在开始配置文本参数，否则会丢失

    //!====================
    //! scaleDraw 这里主要处理日期时间轴和对数时间轴
    //!====================
    // 判断是否是时间坐标轴
    // QwtDateScaleEngine	生成刻度的位置和间隔 决定“刻度在哪里显示”
    // QwtDateScaleDraw	格式化刻度标签	决定“刻度如何显示为日期文本”
    QDomElement scaleDrawEle = qwtplotTag->firstChildElement(QStringLiteral("scaleDraw"));
    if (!scaleDrawEle.isNull()) {
        QwtScaleDraw* scaleDraw         = chart->axisScaleDraw(axisID);
        QwtDateScaleDraw* dateScaleDraw = dynamic_cast< QwtDateScaleDraw* >(scaleDraw);
        if (scaleDrawEle.attribute(QStringLiteral("type")).toLower() == QStringLiteral("datetime")) {
            // 说明是时间坐标系
            if (!dateScaleDraw) {
                // 说明需要新建一个datescaleDraw
                dateScaleDraw = new QwtDateScaleDraw();
                chart->setAxisScaleDraw(axisID, dateScaleDraw);
                // 更新scaleDraw
                scaleDraw = dateScaleDraw;
            }
            QDomElement datescaleDrawEle = scaleDrawEle.firstChildElement(QStringLiteral("datescale"));
            if (!datescaleDrawEle.isNull()) {
                dateScaleDraw->setTimeSpec(
                    stringToEnum(datescaleDrawEle.attribute(QStringLiteral("timeSpec")), Qt::LocalTime));
                dateScaleDraw->setUtcOffset(datescaleDrawEle.attribute(QStringLiteral("utcOffset")).toInt());
                dateScaleDraw->setWeek0Type(
                    stringToEnum(datescaleDrawEle.attribute(QStringLiteral("week0Type")), QwtDate::FirstThursday));
                // 设置dateformat
                QDomElement dateformatEle = datescaleDrawEle.firstChildElement(QStringLiteral("dateformat"));
                if (!dateformatEle.isNull()) {
                    auto safeSetDateformat = [ &dateformatEle, dateScaleDraw ](const QString& tagName) {
                        QString fm = dateformatEle.firstChildElement(tagName).text();
                        if (!fm.isEmpty()) {
                            dateScaleDraw->setDateFormat(QwtDate::Millisecond, fm);
                        }
                    };
                    safeSetDateformat(QStringLiteral("msec"));
                    safeSetDateformat(QStringLiteral("sec"));
                    safeSetDateformat(QStringLiteral("min"));
                    safeSetDateformat(QStringLiteral("hour"));
                    safeSetDateformat(QStringLiteral("day"));
                    safeSetDateformat(QStringLiteral("week"));
                    safeSetDateformat(QStringLiteral("month"));
                    safeSetDateformat(QStringLiteral("year"));
                }
            }
        } else {
            // 进入这里说明坐标轴非datetime
            //  说明没有scaleDraw
            if (!scaleDraw) {
                scaleDraw = new QwtScaleDraw();
                chart->setAxisScaleDraw(axisID, scaleDraw);
            }
            // 说明是普通坐标，但原来是时间坐标
            if (dateScaleDraw) {
                // 正常不会进入这里
                //  说明需要新建一个QwtScaleDraw,还原为普通坐标
                scaleDraw = new QwtScaleDraw();
                chart->setAxisScaleDraw(axisID, scaleDraw);
            }
        }
        // QwtScaleDraw的属性设置
        scaleDraw->setAlignment(stringToEnum(scaleDrawEle.attribute(QStringLiteral("alignment")), QwtScaleDraw::BottomScale));
        scaleDraw->setLabelAlignment(
            static_cast< Qt::Alignment >(scaleDrawEle.attribute(QStringLiteral("labelAlignment")).toInt()));
        scaleDraw->setLabelRotation(scaleDrawEle.attribute(QStringLiteral("labelRotation")).toDouble());
    }
    //!====================
    //! QwtScaleEngine 这里主要处理日期时间刻度和对数刻度
    //!====================
    QDomElement scaleEngineEle = qwtplotTag->firstChildElement(QStringLiteral("scaleEngine"));
    if (!scaleEngineEle.isNull()) {
        QwtScaleEngine* scaleEngine = chart->axisScaleEngine(axisID);
        QString scaleEngineType     = scaleEngineEle.attribute(QStringLiteral("type")).toLower();

        if (scaleEngineType == QStringLiteral("line")) {
            if (!scaleEngine || dynamic_cast< QwtLinearScaleEngine* >(scaleEngine) == nullptr
                // || dynamic_cast< QwtDateScaleEngine* >(scaleEngine)
            ) {  // QwtDateScaleEngine是为了确保如果原来如果是QwtDateScaleEngine，把它变回QwtLinearScaleEngine，但实际不会出现这种情况，省略
                scaleEngine = new QwtLinearScaleEngine();
                chart->setAxisScaleEngine(axisID, scaleEngine);
            }
        } else if (scaleEngineType == QStringLiteral("date")) {
            if (!scaleEngine || dynamic_cast< QwtDateScaleEngine* >(scaleEngine) == nullptr) {
                scaleEngine = new QwtDateScaleEngine();
                chart->setAxisScaleEngine(axisID, scaleEngine);
            }
        } else if (scaleEngineType == QStringLiteral("log")) {
            if (!scaleEngine || dynamic_cast< QwtLogScaleEngine* >(scaleEngine) == nullptr) {
                scaleEngine = new QwtLogScaleEngine();
                chart->setAxisScaleEngine(axisID, scaleEngine);
            }
        }
    }
    //!====================
    //! 通用设置
    //!====================

    chart->setAxisVisible(axisID, qwtplotTag->attribute(QStringLiteral("visible")).toInt());
    chart->setAxisAutoScale(axisID, qwtplotTag->attribute(QStringLiteral("autoScale")).toInt());
    chart->setAxisMaxMinor(axisID, qwtplotTag->attribute(QStringLiteral("maxMinor")).toInt());
    chart->setAxisMaxMajor(axisID, qwtplotTag->attribute(QStringLiteral("maxMajor")).toInt());
    bool isok0 = false, isok1 = false, isok2 = false;
    double stepSize = qwtplotTag->attribute(QStringLiteral("stepSize")).toDouble(&isok0);
    double minValue = qwtplotTag->attribute(QStringLiteral("min")).toDouble(&isok1);
    double maxValue = qwtplotTag->attribute(QStringLiteral("max")).toDouble(&isok2);
    if (isok0 && isok1 && isok2) {
        chart->setAxisScale(axisID, minValue, maxValue, stepSize);
    }
    // font
    QDomElement fontEle = qwtplotTag->firstChildElement(QStringLiteral("font"));
    if (!fontEle.isNull()) {
        QFont f = chart->axisFont(axisID);
        if (DAXMLFileInterface::loadElement(f, &fontEle)) {
            chart->setAxisFont(axisID, f);
        }
    }
    // title
    QDomElement titleEle = qwtplotTag->firstChildElement(QStringLiteral("title"));
    if (!titleEle.isNull()) {
        QwtText title = chart->axisTitle(axisID);
        if (loadElement(&title, &titleEle)) {
            chart->setAxisTitle(axisID, title);
        }
    }
    //!====================
    //! scaleWidget
    //!====================
    QDomElement scaleWidgetEle = qwtplotTag->firstChildElement(QStringLiteral("scaleWidget"));
    if (!scaleWidgetEle.isNull()) {
        QwtScaleWidget* scaleWidget = chart->axisWidget(axisID);
        loadElement(scaleWidget, &scaleWidgetEle);
    }
    return true;
}

/**
 * @brief QwtPlotLayout
 * @param value
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXmlHelper::makeElement(const QwtPlotLayout* value, const QString& tagName, QDomDocument* doc)
{
    QDomElement rootEle = doc->createElement(tagName);
    // setCanvasMargin
    QDomElement canvasMarginEle = doc->createElement(QStringLiteral("margin"));
    canvasMarginEle.setAttribute(QStringLiteral("YLeft"), value->canvasMargin(QwtAxis::YLeft));
    canvasMarginEle.setAttribute(QStringLiteral("YRight"), value->canvasMargin(QwtAxis::YRight));
    canvasMarginEle.setAttribute(QStringLiteral("XBottom"), value->canvasMargin(QwtAxis::XBottom));
    canvasMarginEle.setAttribute(QStringLiteral("XTop"), value->canvasMargin(QwtAxis::XTop));
    rootEle.appendChild(canvasMarginEle);

    // alignCanvasToScale
    QDomElement alignCanvasToScaleEle = doc->createElement(QStringLiteral("alignToScale"));
    alignCanvasToScaleEle.setAttribute(QStringLiteral("YLeft"), value->alignCanvasToScale(QwtAxis::YLeft));
    alignCanvasToScaleEle.setAttribute(QStringLiteral("YRight"), value->alignCanvasToScale(QwtAxis::YRight));
    alignCanvasToScaleEle.setAttribute(QStringLiteral("XBottom"), value->alignCanvasToScale(QwtAxis::XBottom));
    alignCanvasToScaleEle.setAttribute(QStringLiteral("XTop"), value->alignCanvasToScale(QwtAxis::XTop));
    rootEle.appendChild(alignCanvasToScaleEle);

    // 其它属性
    rootEle.setAttribute(QStringLiteral("spacing"), value->spacing());
    rootEle.setAttribute(QStringLiteral("legendRatio"), value->legendRatio());
    rootEle.setAttribute(QStringLiteral("legendPosition"), enumToString(value->legendPosition()));
    return rootEle;
}

bool DAXmlHelper::loadElement(QwtPlotLayout* value, const QDomElement* tag, const QVersionNumber& version)
{
    QDomElement canvasMarginEle = tag->firstChildElement(QStringLiteral("margin"));
    if (!canvasMarginEle.isNull()) {
        value->setCanvasMargin(canvasMarginEle.attribute("YLeft").toInt(), QwtAxis::YLeft);
        value->setCanvasMargin(canvasMarginEle.attribute("YRight").toInt(), QwtAxis::YRight);
        value->setCanvasMargin(canvasMarginEle.attribute("XBottom").toInt(), QwtAxis::XBottom);
        value->setCanvasMargin(canvasMarginEle.attribute("XTop").toInt(), QwtAxis::XTop);
    }

    QDomElement alignCanvasToScaleEle = tag->firstChildElement(QStringLiteral("alignToScale"));
    if (!alignCanvasToScaleEle.isNull()) {
        value->setAlignCanvasToScale(QwtAxis::YLeft, alignCanvasToScaleEle.attribute("YLeft").toInt());
        value->setAlignCanvasToScale(QwtAxis::YRight, alignCanvasToScaleEle.attribute("YRight").toInt());
        value->setAlignCanvasToScale(QwtAxis::XBottom, alignCanvasToScaleEle.attribute("XBottom").toInt());
        value->setAlignCanvasToScale(QwtAxis::XTop, alignCanvasToScaleEle.attribute("XTop").toInt());
    }

    // 其它属性
    value->setSpacing(tag->attribute("spacing").toInt());
    value->setLegendRatio(tag->attribute("legendRatio").toDouble());
    value->setLegendPosition(stringToEnum(tag->attribute("legendPosition"), QwtPlot::RightLegend));
    return true;
}

QDomElement DAXmlHelper::makeElement(const QwtScaleWidget* value, const QString& tagName, QDomDocument* doc)
{
    int dist1, dist2;
    value->getBorderDistHint(dist1, dist2);
    QDomElement scaleWidgetEle = doc->createElement("scaleWidget");
    scaleWidgetEle.setAttribute("dist1", dist1);
    scaleWidgetEle.setAttribute("dist2", dist2);
    scaleWidgetEle.setAttribute("margin", value->margin());
    scaleWidgetEle.setAttribute("spacing", value->spacing());
    scaleWidgetEle.setAttribute("colorBarEnabled", static_cast< int >(value->isColorBarEnabled()));
    scaleWidgetEle.setAttribute("colorBarWidth", value->colorBarWidth());
    return scaleWidgetEle;
}

bool DAXmlHelper::loadElement(QwtScaleWidget* value, const QDomElement* tag, const QVersionNumber& version)
{
    int dist1, dist2;
    dist1 = tag->attribute("dist1").toInt();
    dist2 = tag->attribute("dist2").toInt();
    value->setBorderDist(dist1, dist2);
    value->setMargin(tag->attribute("margin").toInt());
    value->setSpacing(tag->attribute("spacing").toInt());
    value->setColorBarEnabled(static_cast< bool >(tag->attribute("colorBarEnabled").toInt()));
    value->setColorBarWidth(tag->attribute("colorBarWidth").toInt());
    return true;
}

/**
 * @brief QwtText
 * @param value
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXmlHelper::makeElement(const QwtText* value, const QString& tagName, QDomDocument* doc)
{
    QDomElement rootEle = doc->createElement(tagName);
    // text
    QDomElement textEle = doc->createElement(QStringLiteral("text"));
    textEle.setAttribute(QStringLiteral("format"), enumToString(value->format()));
    textEle.appendChild(doc->createTextNode(value->text()));
    // font
    QDomElement fontEle = DAXMLFileInterface::makeElement(value->font(), QStringLiteral("font"), doc);
    rootEle.appendChild(fontEle);
    // color
    QDomElement colorEle = DAXMLFileInterface::makeElement(value->color(), QStringLiteral("color"), doc);
    rootEle.appendChild(colorEle);
    // borderPen
    QDomElement borderPenEle = DAXMLFileInterface::makeElement(value->borderPen(), QStringLiteral("borderPen"), doc);
    rootEle.appendChild(borderPenEle);
    // backgroundBrush
    QDomElement backgroundBrushEle =
        DAXMLFileInterface::makeElement(value->backgroundBrush(), QStringLiteral("backgroundBrush"), doc);
    rootEle.appendChild(backgroundBrushEle);
    // backgroundBrush

    return rootEle;
}

bool DAXmlHelper::loadElement(QwtText* value, const QDomElement* tag, const QVersionNumber& version)
{
    // text
    QDomElement textEle = tag->firstChildElement(QStringLiteral("text"));
    if (!textEle.isNull()) {
        QwtText::TextFormat fm = stringToEnum(textEle.attribute(QStringLiteral("format")), QwtText::AutoText);
        value->setText(textEle.text(), fm);
    }
    // font
    QDomElement fontEle = tag->firstChildElement(QStringLiteral("font"));
    if (!fontEle.isNull()) {
        QFont v;
        if (DAXMLFileInterface::loadElement(v, &fontEle)) {
            value->setFont(v);
        }
    }
    // color
    QDomElement colorEle = tag->firstChildElement(QStringLiteral("color"));
    if (!colorEle.isNull()) {
        QColor v;
        if (DAXMLFileInterface::loadElement(v, &colorEle)) {
            value->setColor(v);
        }
    }
    // borderPen
    QDomElement borderPenEle = tag->firstChildElement(QStringLiteral("borderPen"));
    if (!borderPenEle.isNull()) {
        QPen v;
        if (DAXMLFileInterface::loadElement(v, &borderPenEle)) {
            value->setBorderPen(v);
        }
    }
    // backgroundBrush
    QDomElement backgroundBrushEle = tag->firstChildElement(QStringLiteral("backgroundBrush"));
    if (!backgroundBrushEle.isNull()) {
        QBrush v;
        if (DAXMLFileInterface::loadElement(v, &backgroundBrushEle)) {
            value->setBackgroundBrush(v);
        }
    }
    return true;
}

/**
 * @brief 创建 QwtPlotItems
 * @param value
 * @param tagName
 * @param doc
 * @return
 */
QDomElement
DAXmlHelper::makeElement(unsigned int plotitemID, const QwtPlotItem* value, const QString& tagName, QDomDocument* doc)
{
    QDomElement rootEle = doc->createElement(tagName);
    rootEle.setAttribute(QStringLiteral("rtti"), value->rtti());
    rootEle.setAttribute(QStringLiteral("isVisible"), value->isVisible());
    rootEle.setAttribute(QStringLiteral("xAxis"), value->xAxis());
    rootEle.setAttribute(QStringLiteral("yAxis"), value->yAxis());
    rootEle.setAttribute(QStringLiteral("legendIcon-width"), value->legendIconSize().width());
    rootEle.setAttribute(QStringLiteral("legendIcon-height"), value->legendIconSize().height());
    rootEle.setAttribute(QStringLiteral("z"), value->z());
    rootEle.setAttribute(QStringLiteral("renderThreadCount"), value->renderThreadCount());
    // ItemAttribute
    QDomElement itemAttributeEle = doc->createElement(QStringLiteral("itemAttribute"));
    itemAttributeEle.setAttribute(QStringLiteral("legend"), value->testItemAttribute(QwtPlotItem::Legend));
    itemAttributeEle.setAttribute(QStringLiteral("autoScale"), value->testItemAttribute(QwtPlotItem::AutoScale));
    itemAttributeEle.setAttribute(QStringLiteral("margins"), value->testItemAttribute(QwtPlotItem::Margins));
    rootEle.appendChild(itemAttributeEle);
    // ItemInterest
    QDomElement itemInterestEle = doc->createElement(QStringLiteral("itemInterest"));
    itemInterestEle.setAttribute(QStringLiteral("scaleInterest"), value->testItemInterest(QwtPlotItem::ScaleInterest));
    itemInterestEle.setAttribute(QStringLiteral("legendInterest"), value->testItemInterest(QwtPlotItem::LegendInterest));
    rootEle.appendChild(itemInterestEle);
    // title
    QDomElement titleEle = makeElement(&(value->title()), QStringLiteral("title"), doc);
    rootEle.appendChild(titleEle);
    // qwt的实例内容在线程中存储

    return rootEle;
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
