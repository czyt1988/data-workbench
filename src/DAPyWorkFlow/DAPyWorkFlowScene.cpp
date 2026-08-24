#include "DAPyWorkFlowScene.h"
#include "DAPybind11InQt.h"
#include <QGraphicsSceneMouseEvent>
#include <QPointer>
#include <QQueue>
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAPythonSignalHandler.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPyNode.h"
#include "DAPyWorkFlowManager.h"
#include "DAPyLinkPoint.h"
#include "DAGraphicsScene.h"
#include "DAPyWorkFlowSceneSerializer.h"
#include "DAPyWorkFlowCommandsFactory.h"
#include "DAPyWorkFlowUndoCommands.h"
#include "DAPyNodeMetaData.h"
#include "DAPyWorkFlow.h"
#include "DAPyBindQt/DAPybind11QtCaster.hpp"
#include "DALogCategory.h"
namespace DA
{

class DAPyWorkFlowScene::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlowScene)
public:
    PrivateData(DAPyWorkFlowScene* p);
    //
    void syncPyNodeLinkAdd(DAPyLinkGraphicsItem* linkItem);
    void syncPyNodeLinkRemove(DAPyLinkGraphicsItem* linkItem);
    // 工作流管理器（非拥有引用，通过setManager设置）
    QPointer< DAPyWorkFlowManager > mManager;
    // Python信号处理器
    QPointer< DAPythonSignalHandler > mSignalHandler;
    // 节点到连接线的映射表，维护DAPyNodeGraphicsItem→QList<DAPyLinkGraphicsItem*>的关联关系
    QMap< DAPyNodeGraphicsItem*, QList< DAPyLinkGraphicsItem* > > mNodeToLinksMap;
    // 节点到Python node_id的映射表，维护DAPyNodeGraphicsItem→QString的关联关系
    QMap< DAPyNodeGraphicsItem*, QString > mNodeIdMap;
    // 连接线到Python connection_id的映射表，维护DAPyLinkGraphicsItem→QString的关联关系
    QMap< DAPyLinkGraphicsItem*, QString > mLinkConnectionIdMap;
    // Python节点ID到图形项的反向索引，支持O(1)从nodeId查找DAPyNodeGraphicsItem*
    QHash< QString, DAPyNodeGraphicsItem* > mNodeIdToItemMap;

    // 多选拖拽状态
    bool mMultiMoveActive { false };            ///< 是否正在进行多选拖拽
    QList< QGraphicsItem* > mMultiMoveItems;    ///< 多选拖拽涉及的图元
    QList< QPointF > mMultiMoveStartPositions;  ///< 各图元的起始位置
    QPointF mMultiMoveLastScenePos;             ///< 上一帧鼠标场景坐标

    /**
     * @brief 注册节点到所有映射表（正向+反向索引统一维护）
     *
     * 同步更新mNodeIdMap、mNodeIdToItemMap两个映射表，
     * 确保节点创建时所有索引保持一致。
     *
     * @param[in] item 节点图形项指针
     * @param[in] nodeId Python侧节点唯一标识
     */
    void registerNode(DAPyNodeGraphicsItem* item, const QString& nodeId)
    {
        if (!item || nodeId.isEmpty()) {
            return;
        }
        // 正向映射: item → nodeId
        this->mNodeIdMap[ item ] = nodeId;
        // 反向映射: nodeId → item
        this->mNodeIdToItemMap[ nodeId ] = item;
    }

    /**
     * @brief 从所有映射表中注销节点（正向+反向索引+连接线映射统一清理）
     *
     * 同步清理mNodeIdMap、mNodeIdToItemMap、mNodeToLinksMap三个映射表，
     * 确保节点移除时所有索引保持一致。
     *
     * @param[in] item 要注销的节点图形项指针
     */
    void unregisterNode(DAPyNodeGraphicsItem* item)
    {
        if (!item) {
            return;
        }
        // 从未注册的item中静默返回
        if (!this->mNodeIdMap.contains(item)) {
            // 即使不在nodeIdMap，也尝试清理连接线映射
            this->mNodeToLinksMap.remove(item);
            return;
        }
        // 获取nodeId并清理反向索引
        QString nodeId = this->mNodeIdMap.value(item);
        this->mNodeIdToItemMap.remove(nodeId);
        // 清理正向索引
        this->mNodeIdMap.remove(item);
        // 清理节点到连接线的映射
        this->mNodeToLinksMap.remove(item);
    }

    /**
     * @brief 同步Python端节点注册（调用Manager::registerNode + 更新映射表）
     *
     * 将节点代理注册到Python workflow，获取nodeId，更新所有映射表。
     * 用于undo/redo恢复节点时重新注册到Python。
     *
     * @param[in] item 节点图形项指针
     * @return Python分配的nodeId，注册失败返回空字符串
     */
    QString syncPyNodeRegister(DAPyNodeGraphicsItem* item)
    {
        if (!item) {
            return { };
        }
        const DAPyNode& proxy = item->getProxy();
        if (proxy.isNone()) {
            return { };
        }
        // 调用Manager注册到Python workflow
        QString nodeId;
        if (this->mManager && this->mManager->isWorkflowValid()) {
            nodeId = this->mManager->registerNode(proxy);
            if (nodeId.isEmpty()) {
                daWarning << tr(
                    "DAPyWorkFlowScene::syncPyNodeRegister: registerNode failed");  // cn:同步Python节点注册失败：registerNode 返回空
                return { };
            }
        } else {
            // 无Manager时，从proxy读取nodeId（加载场景时已注册）
            nodeId = proxy.getNodeId();
        }
        // 更新映射表
        this->registerNode(item, nodeId);
        return nodeId;
    }

    /**
     * @brief 同步Python端节点注销（调用Manager::unregisterNode + 清理映射表）
     *
     * 从Python workflow移除节点，清理所有映射表。
     * 用于undo/redo移除节点时同步Python状态。
     *
     * @param[in] item 节点图形项指针
     */
    void syncPyNodeUnregister(DAPyNodeGraphicsItem* item)
    {
        if (!item) {
            return;
        }
        const DAPyNode& proxy = item->getProxy();
        // 调用Manager从Python workflow注销
        if (this->mManager && this->mManager->isWorkflowValid() && !proxy.isNone()) {
            this->mManager->unregisterNode(proxy);
        }
        // 清理映射表
        this->unregisterNode(item);
    }
};

/**
 * @brief 构造函数
 * @param[in] p 父对象指针
 */
DAPyWorkFlowScene::PrivateData::PrivateData(DAPyWorkFlowScene* p) : q_ptr(p)
{
}

/**
 * @brief 同步python端的链接添加
 * @param linkItem
 */
void DAPyWorkFlowScene::PrivateData::syncPyNodeLinkAdd(DAPyLinkGraphicsItem* linkItem)
{
    if (!linkItem) {
        return;
    }
    DAPyNodeGraphicsItem* fromItem = linkItem->getFromNode();
    DAPyNodeGraphicsItem* toItem   = linkItem->getToNode();
    QString fromOutput             = linkItem->getFromOutputName();
    QString toInput                = linkItem->getToInputName();
    // 同步Python侧连接
    if (this->mManager && this->mManager->isWorkflowValid()) {
        if (!fromItem->getProxy().isNone() && !toItem->getProxy().isNone()) {
            DAPyNodeConnection conn =
                this->mManager->connectNode(fromItem->getProxy(), fromOutput, toItem->getProxy(), toInput);
            if (conn) {
                this->mLinkConnectionIdMap[ linkItem ] = conn.getConnectionId();
            } else {
                daWarning << tr(
                    "DAPyWorkFlowScene::addPyNodeLink: connectNode failed, no valid connectionId");  // cn:添加节点连接线失败：connectNode 未返回有效连接 ID
            }
        }
    }
    // 维护节点到连接线的映射表
    this->mNodeToLinksMap[ fromItem ].append(linkItem);
    this->mNodeToLinksMap[ toItem ].append(linkItem);
}

/**
 * @brief 同步python端的链接移除
 * @param linkItem
 */
void DAPyWorkFlowScene::PrivateData::syncPyNodeLinkRemove(DAPyLinkGraphicsItem* linkItem)
{
    if (!linkItem) {
        return;
    }
    // 从映射表中移除连接线记录
    DAPyNodeGraphicsItem* fromNode = linkItem->getFromNode();
    DAPyNodeGraphicsItem* toNode   = linkItem->getToNode();
    if (fromNode && this->mNodeToLinksMap.contains(fromNode)) {
        this->mNodeToLinksMap[ fromNode ].removeOne(linkItem);
        if (this->mNodeToLinksMap[ fromNode ].isEmpty()) {
            this->mNodeToLinksMap.remove(fromNode);
        }
    }
    if (toNode && this->mNodeToLinksMap.contains(toNode)) {
        this->mNodeToLinksMap[ toNode ].removeOne(linkItem);
        if (this->mNodeToLinksMap[ toNode ].isEmpty()) {
            this->mNodeToLinksMap.remove(toNode);
        }
    }

    // 同步Python侧连接移除
    if (this->mManager && this->mManager->isWorkflowValid()) {
        QString connectionId = this->mLinkConnectionIdMap.value(linkItem);
        if (!connectionId.isEmpty()) {
            bool removed = this->mManager->disconnectNode(connectionId);
            if (!removed) {
                daWarning << tr("DAPyWorkFlowScene::removePyNodeLink: disconnectNode failed for connectionId: %1")
                                 .arg(connectionId);  // cn:移除节点连接线失败：断开连接 ID %1 失败
            }
        }
        this->mLinkConnectionIdMap.remove(linkItem);
    }
}

////////////////////////////////////////////////////
/// DAPyWorkFlowScene
////////////////////////////////////////////////////

/**
 * @brief 构造函数
 *
 * 创建Python工作流场景，继承DAGraphicsScene以获取undo/redo支持。
 * 在构造时初始化信号连接。
 *
 * @param parent 父对象
 */
DAPyWorkFlowScene::DAPyWorkFlowScene(QObject* parent) : DAGraphicsScene(parent), DA_PIMPL_CONSTRUCT
{
    initConnect();
    registCommandsFactory(new DAPyWorkFlowCommandsFactory());
}

/**
 * @brief 构造函数（指定场景矩形区域）
 *
 * @param sceneRect 场景矩形区域
 * @param parent 父对象
 */
DAPyWorkFlowScene::DAPyWorkFlowScene(const QRectF& sceneRect, QObject* parent)
    : DAGraphicsScene(sceneRect, parent), DA_PIMPL_CONSTRUCT
{
    initConnect();
}

/**
 * @brief 构造函数（指定场景矩形区域坐标参数）
 *
 * @param x 场景左上角x坐标
 * @param y 场景左上角y坐标
 * @param width 场景宽度
 * @param height 场景高度
 * @param parent 父对象
 */
DAPyWorkFlowScene::DAPyWorkFlowScene(qreal x, qreal y, qreal width, qreal height, QObject* parent)
    : DAGraphicsScene(x, y, width, height, parent), DA_PIMPL_CONSTRUCT
{
    initConnect();
}

/**
 * @brief 析构函数
 *
 * 析构时清空场景中的所有Python节点和连接线。
 */
DAPyWorkFlowScene::~DAPyWorkFlowScene()
{
    // 必须在 clearPyScene() 前断开 selectionChanged，
    // 否则清空节点时触发 selectionChanged → onSelectionChanged → checkSelectItem →
    // selectItemChanged → selectPyNodeItemChanged 信号链会尝试调用
    // 已析构的 DAPyWorkFlowEditWidget 槽，导致断言失败
    disconnectSelectionChanged();
    clearPyScene();
}

/**
 * @brief 设置工作流管理器
 *
 * Scene以非拥有引用方式持有Manager，所有Python操作通过Manager的方法完成。
 *
 * @param[in] manager 工作流管理器指针（Scene不管理其所有权）
 */
void DAPyWorkFlowScene::setManager(DAPyWorkFlowManager* manager)
{
    DA_D(d);
    if (d->mManager) {
        disconnect(d->mManager, &DAPyWorkFlowManager::nodeExecuted, this, &DAPyWorkFlowScene::onNodeExecuted);
    }
    d->mManager = manager;
    if (d->mManager) {
        connect(d->mManager, &DAPyWorkFlowManager::nodeExecuted, this, &DAPyWorkFlowScene::onNodeExecuted);
    }
}

/**
 * @brief 获取工作流管理器
 *
 * @return 当前设置的DAPyWorkFlowManager指针
 */
DAPyWorkFlowManager* DAPyWorkFlowScene::getManager() const
{
    DA_DC(d);
    return d->mManager;
}

/**
 * @brief 判断是否已设置有效的工作流管理器
 *
 * @return 如果Manager已设置且其内部workflow有效返回true
 */
bool DAPyWorkFlowScene::hasManager() const
{
    DA_DC(d);
    return d->mManager && d->mManager->isWorkflowValid();
}

/**
 * @brief 设置Python信号处理器
 *
 * 设置DAPythonSignalHandler用于接收Python侧的状态变更通知，
 * 如节点执行状态变化、数据流状态更新等。
 *
 * @param handler DAPythonSignalHandler实例指针
 * @note DAPyWorkFlowScene不管理DAPythonSignalHandler的所有权
 */
void DAPyWorkFlowScene::setSignalHandler(DAPythonSignalHandler* handler)
{
    DA_D(d);
    d->mSignalHandler = handler;
}

/**
 * @brief 获取Python信号处理器
 *
 * @return 当前设置的DAPythonSignalHandler指针
 */
DAPythonSignalHandler* DAPyWorkFlowScene::getSignalHandler() const
{
    DA_DC(d);
    return d->mSignalHandler.data();
}

/**
 * @brief 创建Python节点图形项（通过元数据，不添加到场景）
 *
 * 工厂创建代理时已获取Python侧完整属性，
 * DAPyNodeGraphicsItem构造函数自动从代理同步缓存字段和连接点，
 * 此方法仅设置元数据中的显示属性。
 *
 * @param[in] metaData 节点元数据，包含qualified_name、name、icon等
 * @param[in] pos 节点在场景中的初始位置
 * @return 创建的DAPyNodeGraphicsItem指针，创建失败返回nullptr
 * @note 返回的item未添加到场景，需要调用方自行添加
 */
DAPyNodeGraphicsItem* DAPyWorkFlowScene::createPyNode(const DAPyNodeMetaData& metaData, const QPointF& pos)
{
    DA_D(d);
    if (!d->mManager || !d->mManager->isWorkflowValid()) {
        daWarning << tr(
            "DAPyWorkFlowScene::createPyNode: Manager or workflow is not set");  // cn:创建 Python 节点失败：管理器或工作流未设置
        return nullptr;
    }

    if (!metaData.isValid()) {
        daWarning << tr("DAPyWorkFlowScene::createPyNode: invalid metadata (qualified_name: %1)")
                         .arg(metaData.qualifiedName);  // cn:创建 Python 节点失败：元数据无效（qualified_name: %1）
        return nullptr;
    }
    // 通过Manager创建DAPyNode
    DAPyNode proxy = d->mManager->createNodeProxy(metaData);
    if (proxy.isNone()) {
        // 节点创建失败
        daWarning << tr("DAPyWorkFlowScene::createPyNode: factory failed to create proxy for %1")
                         .arg(metaData.qualifiedName);  // cn:创建 Python 节点失败：工厂无法为 %1 创建代理
        return nullptr;
    }
    // 在Python侧注册节点到DAWorkflow
    QString nodeId = d->mManager->registerNode(proxy);
    if (nodeId.isEmpty()) {
        daWarning << tr("DAPyWorkFlowScene::createPyNode: addNode failed for %1")
                         .arg(metaData.qualifiedName);  // cn:创建 Python 节点失败：注册节点 %1 失败
        return nullptr;
    }

    // 构造函数已从proxy获取完整缓存字段（含inputKeys/outputKeys），
    DAPyNodeGraphicsItem* item = new DAPyNodeGraphicsItem(proxy);
    // 设置位置（未添加到场景）
    item->updateNodeBody();
    item->setPos(pos);

    // 记录nodeId映射
    d->registerNode(item, nodeId);

    return item;
}

/**
 * @brief 创建Python节点（通过元数据，带undo/redo）
 *
 * 通过QUndoStack记录创建操作，支持撤销和重做。
 * 先调用createPyNode()创建节点（含Python注册），
 * 然后通过专用命令将item添加到场景并推入undo栈。
 * 命令首次redo跳过Python同步（createPyNode已完成注册），
 * 后续redo会重新注册到Python。
 *
 * @param[in] metaData 节点元数据
 * @param[in] pos 节点在场景中的初始位置
 * @return 创建的DAPyNodeGraphicsItem指针，创建失败返回nullptr
 * @note 函数名后缀"_"表示支持undo/redo操作
 * @see createPyNode(const DAPyNodeMetaData&, const QPointF&)
 */
DAPyNodeGraphicsItem* DAPyWorkFlowScene::createPyNode_(const DAPyNodeMetaData& metaData, const QPointF& pos)
{
    DAPyNodeGraphicsItem* item = createPyNode(metaData, pos);
    if (!item) {
        return nullptr;
    }
    // 使用专用节点添加命令（带Python同步），首次redo跳过同步
    DAPyWorkFlowCommandsFactory* fac = dynamic_cast< DAPyWorkFlowCommandsFactory* >(commandsFactory());
    if (fac) {
        auto cmd = fac->createPyNodeItemAdd(item);
        push(cmd);
    } else {
        addItem_(item);
    }
    emit pyNodeItemCreated(item);
    return item;
}

/**
 * @brief 移除Python节点（不带undo/redo）
 *
 * 从场景中移除节点图形项及其所有连接线，
 * 并同步到Python DAWorkflow.remove_node()。
 *
 * @param item 要移除的DAPyNodeGraphicsItem指针
 * @return 移除成功返回true，失败返回false
 */
bool DAPyWorkFlowScene::removePyNodeItem(DAPyNodeGraphicsItem* item)
{
    if (!item) {
        return false;
    }
    DA_D(d);

    // 先移除所有关联的连接线（通过映射表直接获取）
    const QList< DAPyLinkGraphicsItem* > relatedLinks = getNodeLinkItems(item);
    for (DAPyLinkGraphicsItem* link : relatedLinks) {
        // 从映射表中移除连接线记录
        DAPyNodeGraphicsItem* linkFrom = link->getFromNode();
        DAPyNodeGraphicsItem* linkTo   = link->getToNode();
        if (linkFrom && d->mNodeToLinksMap.contains(linkFrom)) {
            d->mNodeToLinksMap[ linkFrom ].removeOne(link);
        }
        if (linkTo && d->mNodeToLinksMap.contains(linkTo)) {
            d->mNodeToLinksMap[ linkTo ].removeOne(link);
        }
        removeItem(link);
        // 同步Python侧连接移除
        if (d->mManager && d->mManager->isWorkflowValid()) {
            QString connectionId = d->mLinkConnectionIdMap.value(link);
            if (!connectionId.isEmpty()) {
                d->mManager->disconnectNode(connectionId);
            }
        }
        d->mLinkConnectionIdMap.remove(link);
        delete link;
    }

    // 从映射表中移除节点的记录
    d->mNodeToLinksMap.remove(item);

    // 获取节点代理
    const DAPyNode& proxy = item->getProxy();

    // 同步Python侧节点移除
    if (d->mManager && d->mManager->isWorkflowValid() && !proxy.isNone()) {
        d->mManager->unregisterNode(proxy);
    }
    d->unregisterNode(item);

    // 从场景移除图形项
    removeItem(item);

    // 销毁图形项（unique_ptr自动释放proxy，需GIL保护proxy析构中的Python引用释放）
    {
        DAPyGILGuard gil;
        delete item;
    }

    return true;
}

/**
 * @brief 移除Python节点（带undo/redo）
 *
 * 通过QUndoStack记录移除操作，支持撤销和重做。
 * 使用beginMacro/endMacro将节点及其关联连接线的移除合并为单个原子操作。
 * 所有命令在redo/undo时同时同步C++场景和Python workflow状态。
 *
 * @param item 要移除的DAPyNodeGraphicsItem指针
 * @note 函数名后缀"_"表示支持undo/redo操作
 */
void DAPyWorkFlowScene::removePyNodeItem_(DAPyNodeGraphicsItem* item)
{
    if (!item) {
        return;
    }
    DAPyWorkFlowCommandsFactory* fac = dynamic_cast< DAPyWorkFlowCommandsFactory* >(commandsFactory());
    if (!fac) {
        return;
    }

    // 使用宏命令将多个操作合并为单个undo步骤
    undoStack().beginMacro(tr("Remove Node"));  // cn:移除节点

    // 先移除所有关联的连接线（带undo，通过映射表获取）
    const QList< DAPyLinkGraphicsItem* > relatedLinks = getNodeLinkItems(item);
    for (DAPyLinkGraphicsItem* link : relatedLinks) {
        auto cmd = fac->createPyLinkItemRemove(link);
        push(cmd);
    }

    // 移除节点item（带Python同步的专用命令）
    auto nodeCmd = fac->createPyNodeItemRemove(item);
    push(nodeCmd);

    undoStack().endMacro();

    emit pyNodeItemsRemoved({ item });
}

/**
 * @brief 通过节点ID查找节点图形项
 *
 * @param nodeId Python节点的唯一标识字符串
 * @return 对应的DAPyNodeGraphicsItem指针，未找到返回nullptr
 */
DAPyNodeGraphicsItem* DAPyWorkFlowScene::findNodeItemById(const QString& nodeId) const
{
    DA_DC(dc);
    // 通过nodeId反向索引O(1)查找
    return dc->mNodeIdToItemMap.value(nodeId, nullptr);
}

/**
 * @brief 通过场景坐标获取Python节点图形项
 *
 * 加强版的itemAt，用于快速定位鼠标点击位置的节点。
 *
 * @param scenePos 场景坐标
 * @return 该坐标处的DAPyNodeGraphicsItem指针，未找到返回nullptr
 */
DAPyNodeGraphicsItem* DAPyWorkFlowScene::nodeItemAt(const QPointF& scenePos) const
{
    DAPyNodeGraphicsItem* nodeItem = dynamic_cast< DAPyNodeGraphicsItem* >(itemAt(scenePos, QTransform()));
    if (nodeItem) {
        return nodeItem;
    }
    // 如果没找到，检查此点下的所有item
    QList< QGraphicsItem* > its = topItems(scenePos);
    for (QGraphicsItem* i : std::as_const(its)) {
        if (DAPyNodeGraphicsItem* n = dynamic_cast< DAPyNodeGraphicsItem* >(i)) {
            return n;
        }
    }
    return nullptr;
}

/**
 * @brief 获取所有Python节点图形项
 *
 * @return 所有顶层DAPyNodeGraphicsItem列表
 */
QList< DAPyNodeGraphicsItem* > DAPyWorkFlowScene::getPyNodeItems() const
{
    QList< DAPyNodeGraphicsItem* > res;
    QList< QGraphicsItem* > its = topItems();
    for (QGraphicsItem* i : std::as_const(its)) {
        if (DAPyNodeGraphicsItem* ni = dynamic_cast< DAPyNodeGraphicsItem* >(i)) {
            res.append(ni);
        }
    }
    return res;
}

/**
 * @brief 获取选中的Python节点图形项
 *
 * @return 当前选中的DAPyNodeGraphicsItem列表
 */
QList< DAPyNodeGraphicsItem* > DAPyWorkFlowScene::getSelectedPyNodeItems() const
{
    QList< DAPyNodeGraphicsItem* > res;
    QList< QGraphicsItem* > sits = selectedItems();
    for (QGraphicsItem* i : std::as_const(sits)) {
        if (DAPyNodeGraphicsItem* gi = dynamic_cast< DAPyNodeGraphicsItem* >(i)) {
            res.append(gi);
        }
    }
    return res;
}

/**
 * @brief 添加Python节点连接线（不带undo/redo）
 *
 * 创建DAPyLinkGraphicsItem连接两个节点，
 * 并同步到Python DAWorkflow.add_connection()。
 * 此方法仅创建连接线但不添加到场景，由调用方决定添加方式。
 *
 * @param fromItem 源节点图形项
 * @param fromOutput 源节点的输出端口名称
 * @param toItem 目标节点图形项
 * @param toInput 目标节点的输入端口名称
 * @return 创建的DAPyLinkGraphicsItem指针，创建失败返回nullptr
 * @note 返回的link未添加到场景，需要调用方自行添加
 */
DAPyLinkGraphicsItem* DAPyWorkFlowScene::addPyNodeLink(DAPyNodeGraphicsItem* fromItem,
                                                       const QString& fromOutput,
                                                       DAPyNodeGraphicsItem* toItem,
                                                       const QString& toInput)
{
    if (!fromItem || !toItem) {
        return nullptr;
    }
    DA_D(d);

    // 创建连接线
    DAPyLinkGraphicsItem* link = createLinkItem(fromItem, fromOutput);
    link->setFromNode(fromItem, fromOutput);
    link->setToNode(toItem, toInput);

    // 设置连接线的起止位置
    const QList< DAPyLinkPoint > outputPoints = fromItem->getOutputLinkPoints();
    for (const DAPyLinkPoint& lp : outputPoints) {
        if (lp.name == fromOutput) {
            link->setStartScenePosition(fromItem->mapToScene(lp.position));
            break;
        }
    }
    const QList< DAPyLinkPoint > inputPoints = toItem->getInputLinkPoints();
    for (const DAPyLinkPoint& lp : inputPoints) {
        if (lp.name == toInput) {
            link->setEndScenePosition(toItem->mapToScene(lp.position));
            break;
        }
    }
    addPyNodeLink(link);
    return link;
}

/**
 * @brief 添加Python节点连接线（直接添加到场景，不带undo/redo）
 * @param[in] linkItem 连接线图形项
 */
void DAPyWorkFlowScene::addPyNodeLink(DAPyLinkGraphicsItem* linkItem)
{
    if (!linkItem) {
        return;
    }
    DA_D(d);
    d->syncPyNodeLinkAdd(linkItem);
    // 通过addItem_()添加到场景并推入undo栈
    if (linkItem->scene() != this) {  // 增加这个判断是避免重复添加
        addItem(linkItem);
    }
    Q_EMIT pyNodeLinkCreated(linkItem);
}

/**
 * @brief 添加Python节点连接线（带undo/redo）
 *
 * 通过QUndoStack记录连接操作，支持撤销和重做。
 * 仅创建连接线图形项并设置端点，不执行Python同步（由命令的redo统一处理）。
 *
 * @param fromItem 源节点图形项
 * @param fromOutput 源节点的输出端口名称
 * @param toItem 目标节点图形项
 * @param toInput 目标节点的输入端口名称
 * @return 创建的DAPyLinkGraphicsItem指针，创建失败返回nullptr
 * @note 函数名后缀"_"表示支持undo/redo操作
 */
DAPyLinkGraphicsItem* DAPyWorkFlowScene::addPyNodeLink_(DAPyNodeGraphicsItem* fromItem,
                                                        const QString& fromOutput,
                                                        DAPyNodeGraphicsItem* toItem,
                                                        const QString& toInput)
{
    if (!fromItem || !toItem) {
        return nullptr;
    }

    // 创建连接线（仅创建item，不添加场景，不同步Python）
    DAPyLinkGraphicsItem* link = createLinkItem(fromItem, fromOutput);
    link->setFromNode(fromItem, fromOutput);
    link->setToNode(toItem, toInput);

    // 设置连接线的起止场景位置
    const QList< DAPyLinkPoint > outputPoints = fromItem->getOutputLinkPoints();
    for (const DAPyLinkPoint& lp : outputPoints) {
        if (lp.name == fromOutput) {
            link->setStartScenePosition(fromItem->mapToScene(lp.position));
            break;
        }
    }
    const QList< DAPyLinkPoint > inputPoints = toItem->getInputLinkPoints();
    for (const DAPyLinkPoint& lp : inputPoints) {
        if (lp.name == toInput) {
            link->setEndScenePosition(toItem->mapToScene(lp.position));
            break;
        }
    }

    // 推入命令，redo()统一处理Python同步和场景添加
    addPyNodeLink_(link);
    return link;
}

/**
 * @brief 添加Python节点连接线（带undo/redo）
 * @param linkItem
 */
void DAPyWorkFlowScene::addPyNodeLink_(DAPyLinkGraphicsItem* linkItem)
{
    if (!linkItem) {
        return;
    }
    DAPyWorkFlowCommandsFactory* fac = dynamic_cast< DAPyWorkFlowCommandsFactory* >(commandsFactory());
    if (fac) {
        auto cmd = fac->createPyLinkItemAdd(linkItem);
        push(cmd);
    }
}

/**
 * @brief 移除Python节点连接线（不带undo/redo）
 *
 * 从场景中移除连接线，并同步到Python DAWorkflow.remove_connection()。
 *
 * @param linkItem 要移除的DAPyLinkGraphicsItem指针
 * @return 移除成功返回true，失败返回false
 */
bool DAPyWorkFlowScene::removePyNodeLink(DAPyLinkGraphicsItem* linkItem, bool autoDelete)
{
    if (!linkItem) {
        return false;
    }
    DA_D(d);

    // 从映射表中移除连接线记录
    d->syncPyNodeLinkRemove(linkItem);
    removeItem(linkItem);
    if (autoDelete) {
        delete linkItem;
    }
    return true;
}

/**
 * @brief 移除Python节点连接线（带undo/redo）
 *
 * 通过removeItem_()将移除操作推入undo栈，支持撤销和重做。
 *
 * @param linkItem 要移除的DAPyLinkGraphicsItem指针
 * @note 函数名后缀"_"表示支持undo/redo操作
 */
void DAPyWorkFlowScene::removePyNodeLink_(DAPyLinkGraphicsItem* linkItem)
{
    if (!linkItem) {
        return;
    }
    DAPyWorkFlowCommandsFactory* fac = dynamic_cast< DAPyWorkFlowCommandsFactory* >(commandsFactory());
    if (fac) {
        auto cmd = fac->createPyLinkItemRemove(linkItem);
        push(cmd);
    }
}

/**
 * @brief 获取所有Python连接线图形项
 *
 * @return 所有DAPyLinkGraphicsItem列表
 */
QList< DAPyLinkGraphicsItem* > DAPyWorkFlowScene::getPyNodeLinkItems() const
{
    QList< DAPyLinkGraphicsItem* > res;
    QList< QGraphicsItem* > its = topItems();
    for (QGraphicsItem* i : std::as_const(its)) {
        if (DAPyLinkGraphicsItem* li = dynamic_cast< DAPyLinkGraphicsItem* >(i)) {
            res.append(li);
        }
    }
    return res;
}

/**
 * @brief 获取选中的Python连接线图形项
 *
 * @return 当前选中的DAPyLinkGraphicsItem列表
 */
QList< DAPyLinkGraphicsItem* > DAPyWorkFlowScene::getSelectedPyNodeLinkItems() const
{
    QList< DAPyLinkGraphicsItem* > res;
    QList< QGraphicsItem* > sits = selectedItems();
    for (QGraphicsItem* i : std::as_const(sits)) {
        if (DAPyLinkGraphicsItem* gi = dynamic_cast< DAPyLinkGraphicsItem* >(i)) {
            res.append(gi);
        }
    }
    return res;
}

/**
 * @brief 获取节点的所有连接线
 *
 * 通过映射表直接查询，O(1)查找节点对应的连接线列表
 *
 * @param[in] nodeItem 节点图形项
 * @return 该节点关联的所有连接线列表，无关联时返回空列表
 */
QList< DAPyLinkGraphicsItem* > DAPyWorkFlowScene::getNodeLinkItems(DAPyNodeGraphicsItem* nodeItem) const
{
    DA_DC(dc);
    return dc->mNodeToLinksMap.value(nodeItem);
}

/**
 * @brief 获取节点的输入连接线
 *
 * 返回所有toNode为该节点的连接线，即数据流入该节点的连接
 *
 * @param[in] nodeItem 节点图形项
 * @return 该节点作为接收端的连接线列表
 */
QList< DAPyLinkGraphicsItem* > DAPyWorkFlowScene::getNodeInputLinkItems(DAPyNodeGraphicsItem* nodeItem) const
{
    QList< DAPyLinkGraphicsItem* > res;
    const QList< DAPyLinkGraphicsItem* > links = getNodeLinkItems(nodeItem);
    for (DAPyLinkGraphicsItem* link : links) {
        if (link->getToNode() == nodeItem) {
            res.append(link);
        }
    }
    return res;
}

/**
 * @brief 获取节点的输出连接线
 *
 * 返回所有fromNode为该节点的连接线，即数据从该节点流出的连接
 *
 * @param[in] nodeItem 节点图形项
 * @return 该节点作为发送端的连接线列表
 */
QList< DAPyLinkGraphicsItem* > DAPyWorkFlowScene::getNodeOutputLinkItems(DAPyNodeGraphicsItem* nodeItem) const
{
    QList< DAPyLinkGraphicsItem* > res;
    const QList< DAPyLinkGraphicsItem* > links = getNodeLinkItems(nodeItem);
    for (DAPyLinkGraphicsItem* link : links) {
        if (link->getFromNode() == nodeItem) {
            res.append(link);
        }
    }
    return res;
}

/**
 * @brief 获取节点沿输出方向的链路
 *
 * 从起始节点出发，沿输出连接线进行BFS遍历，收集所有下游可达节点
 *
 * @param[in] startNode 路起始节点
 * @return 所有下游可达节点列表（不含起始节点本身）
 */
QList< DAPyNodeGraphicsItem* > DAPyWorkFlowScene::getOutputLinkChain(DAPyNodeGraphicsItem* startNode) const
{
    if (!startNode) {
        return { };
    }
    QSet< DAPyNodeGraphicsItem* > visited;
    QQueue< DAPyNodeGraphicsItem* > queue;
    queue.enqueue(startNode);
    visited.insert(startNode);
    QList< DAPyNodeGraphicsItem* > result;
    while (!queue.isEmpty()) {
        DAPyNodeGraphicsItem* current                 = queue.dequeue();
        const QList< DAPyLinkGraphicsItem* > outLinks = getNodeOutputLinkItems(current);
        for (DAPyLinkGraphicsItem* link : outLinks) {
            DAPyNodeGraphicsItem* next = link->getToNode();
            if (next && !visited.contains(next)) {
                visited.insert(next);
                result.append(next);
                queue.enqueue(next);
            }
        }
    }
    return result;
}

/**
 * @brief 获取节点沿输入方向的链路
 *
 * 从起始节点出发，沿输入连接线进行BFS遍历，收集所有上游可达节点
 *
 * @param[in] startNode 路起始节点
 * @return 所有上游可达节点列表（不含起始节点本身）
 */
QList< DAPyNodeGraphicsItem* > DAPyWorkFlowScene::getInputLinkChain(DAPyNodeGraphicsItem* startNode) const
{
    if (!startNode) {
        return { };
    }
    QSet< DAPyNodeGraphicsItem* > visited;
    QQueue< DAPyNodeGraphicsItem* > queue;
    queue.enqueue(startNode);
    visited.insert(startNode);
    QList< DAPyNodeGraphicsItem* > result;
    while (!queue.isEmpty()) {
        DAPyNodeGraphicsItem* current                = queue.dequeue();
        const QList< DAPyLinkGraphicsItem* > inLinks = getNodeInputLinkItems(current);
        for (DAPyLinkGraphicsItem* link : inLinks) {
            DAPyNodeGraphicsItem* prev = link->getFromNode();
            if (prev && !visited.contains(prev)) {
                visited.insert(prev);
                result.append(prev);
                queue.enqueue(prev);
            }
        }
    }
    return result;
}

/**
 * @brief 更新节点对应的连接线端点位置
 *
 * 当节点移动后，需要更新其关联的所有连接线的起止端点位置，
 * 使连接线跟随节点端口位置变化。
 *
 * @param[in] nodeItem 移动后的节点图形项
 */
void DAPyWorkFlowScene::updateNodeLinkPositions(DAPyNodeGraphicsItem* nodeItem)
{
    if (!nodeItem) {
        return;
    }
    const QList< DAPyLinkGraphicsItem* > links = getNodeLinkItems(nodeItem);
    for (DAPyLinkGraphicsItem* link : links) {
        if (!link) {
            continue;
        }
        bool needUpdate = false;
        if (DAPyNodeGraphicsItem* fromNode = link->getFromNode()) {
            const QList< DAPyLinkPoint > outPoints = fromNode->getOutputLinkPoints();
            for (const DAPyLinkPoint& pt : outPoints) {
                if (pt.name == link->getFromOutputName()) {
                    link->setStartScenePosition(fromNode->mapToScene(pt.position));
                    needUpdate = true;
                    break;
                }
            }
        }
        if (DAPyNodeGraphicsItem* toNode = link->getToNode()) {
            const QList< DAPyLinkPoint > inPoints = toNode->getInputLinkPoints();
            for (const DAPyLinkPoint& pt : inPoints) {
                if (pt.name == link->getToInputName()) {
                    link->setEndScenePosition(toNode->mapToScene(pt.position));
                    needUpdate = true;
                    break;
                }
            }
        }
        if (needUpdate) {
            link->updateBoundingRect();
        }
    }
}

/**
 * @brief 删除选中项（支持undo/redo）
 *
 * 删除当前场景中选中的Python节点和连接线。
 * 删除节点时会连带删除其所有连接线。
 * 使用beginMacro/endMacro将所有删除操作合并为单个原子undo步骤。
 * 所有命令在redo/undo时同时同步C++场景和Python workflow状态。
 *
 * @return 删除的节点数量
 */
int DAPyWorkFlowScene::removeSelectedItems_()
{
    cancelLink();

    QList< QGraphicsItem* > sits = selectedItems();
    if (sits.isEmpty()) {
        return 0;
    }

    // 对选中项进行分类
    QList< DAPyNodeGraphicsItem* > nodeItems;
    QList< DAPyLinkGraphicsItem* > linkItems;
    QList< QGraphicsItem* > normalItems;
    classifyItems(sits, nodeItems, linkItems, normalItems);

    // 获取节点关联的所有连接线（包括未选中但关联的）
    QList< DAPyLinkGraphicsItem* > nodeLinks = getNodesAllLinkItems(nodeItems);
    // 合并选中的连接线和节点关联的连接线（去重）
    for (DAPyLinkGraphicsItem* link : std::as_const(linkItems)) {
        if (!nodeLinks.contains(link)) {
            nodeLinks.append(link);
        }
    }

    int removeCount = 0;

    DAPyWorkFlowCommandsFactory* fac = dynamic_cast< DAPyWorkFlowCommandsFactory* >(commandsFactory());

    // 使用宏命令将多个操作合并为单个undo步骤
    undoStack().beginMacro(tr("Remove Selected Items"));  // cn:移除选中项

    if (fac) {
        // 先移除连接线（带Python同步的专用命令）
        for (DAPyLinkGraphicsItem* link : std::as_const(nodeLinks)) {
            auto cmd = fac->createPyLinkItemRemove(link);
            push(cmd);
            ++removeCount;
        }

        // 再移除节点（带Python同步的专用命令）
        for (DAPyNodeGraphicsItem* node : std::as_const(nodeItems)) {
            auto cmd = fac->createPyNodeItemRemove(node);
            push(cmd);
            ++removeCount;
        }
    }

    // 移除普通图形项（使用基类命令，不涉及Python同步）
    for (QGraphicsItem* item : std::as_const(normalItems)) {
        removeItem_(item);
        ++removeCount;
    }

    undoStack().endMacro();

    // 发射移除信号
    if (!nodeItems.isEmpty()) {
        emit pyNodeItemsRemoved(nodeItems);
    }
    if (!nodeLinks.isEmpty()) {
        emit pyNodeLinksRemoved(nodeLinks);
    }

    return removeCount;
}

/**
 * @brief 清空场景中的所有Python节点和连接线
 *
 * 移除场景中所有DAPyNodeGraphicsItem和DAPyLinkGraphicsItem，
 * 同步清空Python侧的工作流节点。
 */
void DAPyWorkFlowScene::clearPyScene()
{
    DA_D(d);
    // 获取所有节点和连接线
    const QList< DAPyNodeGraphicsItem* > nodeItems = getPyNodeItems();
    const QList< DAPyLinkGraphicsItem* > linkItems = getPyNodeLinkItems();

    // 清空所有映射表
    d->mNodeToLinksMap.clear();
    d->mNodeIdMap.clear();
    d->mNodeIdToItemMap.clear();
    d->mLinkConnectionIdMap.clear();

    // 先移除所有连接线
    for (DAPyLinkGraphicsItem* link : linkItems) {
        removeItem(link);
        delete link;
    }

    // 同步Python侧清空
    if (d->mManager) {
        d->mManager->clearWorkflow();
    }

    // 移除所有节点（unique_ptr自动释放proxy，需GIL保护Python引用释放）
    {
        DAPyGILGuard gil;
        for (DAPyNodeGraphicsItem* node : nodeItems) {
            removeItem(node);
            delete node;
        }
    }

    // 清空undo栈
    undoStack().clear();
}

//===================================================
// 加载专用方法（Python数据已就绪，仅创建/恢复视图）
//===================================================

/**
 * @brief 包装已有的Python节点为图形项
 *
 * 用于加载流程：Python workflow中的节点已由反序列化创建完毕，
 * 此方法直接构造DAPyNodeGraphicsItem并注册到场景映射表，
 * 不经过工厂创建，也不调用Manager::registerNode()。
 *
 * @param[in] proxy 已有的Python节点代理
 * @param[in] pos 节点在场景中的位置
 * @return 创建的图形项指针，proxy无效时返回nullptr
 * @note 返回的item未添加到场景，需要调用方自行addItem()
 */
DAPyNodeGraphicsItem* DAPyWorkFlowScene::wrapPyNode(const DAPyNode& proxy, const QPointF& pos)
{
    if (proxy.isNone()) {
        return nullptr;
    }
    DA_D(d);

    DAPyNodeGraphicsItem* item = new DAPyNodeGraphicsItem(proxy);
    item->updateNodeBody();
    item->setPos(pos);

    // 注册到映射表（使 findNodeItemById 可查找）
    d->registerNode(item, proxy.getNodeId());

    return item;
}

/**
 * @brief 包装已有的Python连接为连线图形项
 *
 * 用于加载流程：Python workflow中的连接已由反序列化创建完毕，
 * 此方法仅创建连线图形项并维护C++侧映射表，
 * 不调用syncPyNodeLinkAdd()（避免Python端因端口对重复而ValueError）。
 * 连线自动添加到场景。
 *
 * @param[in] fromItem 源节点图形项
 * @param[in] fromOutput 源节点输出端口名称
 * @param[in] toItem 目标节点图形项
 * @param[in] toInput 目标节点输入端口名称
 * @return 创建的连线图形项指针，参数无效时返回nullptr
 */
DAPyLinkGraphicsItem* DAPyWorkFlowScene::wrapPyNodeLink(DAPyNodeGraphicsItem* fromItem,
                                                        const QString& fromOutput,
                                                        DAPyNodeGraphicsItem* toItem,
                                                        const QString& toInput)
{
    if (!fromItem || !toItem) {
        return nullptr;
    }
    DA_D(d);

    // 创建连线图形项
    DAPyLinkGraphicsItem* link = createLinkItem(fromItem, fromOutput);
    link->setFromNode(fromItem, fromOutput);
    link->setToNode(toItem, toInput);

    // 设置连接线的起止场景位置
    const QList< DAPyLinkPoint > outputPoints = fromItem->getOutputLinkPoints();
    for (const DAPyLinkPoint& lp : outputPoints) {
        if (lp.name == fromOutput) {
            link->setStartScenePosition(fromItem->mapToScene(lp.position));
            break;
        }
    }
    const QList< DAPyLinkPoint > inputPoints = toItem->getInputLinkPoints();
    for (const DAPyLinkPoint& lp : inputPoints) {
        if (lp.name == toInput) {
            link->setEndScenePosition(toItem->mapToScene(lp.position));
            break;
        }
    }

    // 仅维护C++侧映射表，不调用syncPyNodeLinkAdd（Python已有连接）
    d->mNodeToLinksMap[ fromItem ].append(link);
    d->mNodeToLinksMap[ toItem ].append(link);

    // 添加到场景
    if (link->scene() != this) {
        addItem(link);
    }

    return link;
}

/**
 * @brief 清空场景C++图元和映射表，但不清除Python workflow数据
 *
 * 与clearPyScene()的区别：此方法不调用Manager::clearWorkflow()，
 * 保留Python侧的节点和连接数据。用于加载流程中替换视图前的清理。
 */
void DAPyWorkFlowScene::clearSceneItems()
{
    DA_D(d);
    const QList< DAPyNodeGraphicsItem* > nodeItems = getPyNodeItems();
    const QList< DAPyLinkGraphicsItem* > linkItems = getPyNodeLinkItems();

    // 清空所有映射表
    d->mNodeToLinksMap.clear();
    d->mNodeIdMap.clear();
    d->mNodeIdToItemMap.clear();
    d->mLinkConnectionIdMap.clear();

    // 先移除所有连接线
    for (DAPyLinkGraphicsItem* link : linkItems) {
        removeItem(link);
        delete link;
    }

    // 移除所有节点（需GIL保护Python引用释放）
    {
        DAPyGILGuard gil;
        for (DAPyNodeGraphicsItem* node : nodeItems) {
            removeItem(node);
            delete node;
        }
    }

    // 清空undo栈
    undoStack().clear();
}

/**
 * @brief 从Python连接列表重建mLinkConnectionIdMap
 *
 * 加载流程中wrapPyNodeLink()不填充mLinkConnectionIdMap（因为Python连接已存在），
 * 此方法遍历Python workflow的所有连接，按四元组匹配C++连线图形项，
 * 填充connection_id映射，确保后续UI删除连线时能正确通知Python断开。
 */
void DAPyWorkFlowScene::rebuildLinkConnectionIdMap()
{
    DA_D(d);
    if (!d->mManager || !d->mManager->isWorkflowValid()) {
        return;
    }

    DAPyGILGuard gil;  // GIL保护：getWorkflow().getConnections() 调用 Python
    QList< DAPyNodeConnection > pyConns            = d->mManager->getWorkflow().getConnections();
    const QList< DAPyLinkGraphicsItem* > linkItems = getPyNodeLinkItems();

    for (const DAPyNodeConnection& conn : std::as_const(pyConns)) {
        if (conn.isNone()) {
            continue;
        }
        QString srcId  = conn.getSourceNodeId();
        QString srcCh  = conn.getSourceOutputChannel();
        QString dstId  = conn.getTargetNodeId();
        QString dstCh  = conn.getTargetInputChannel();
        QString connId = conn.getConnectionId();

        // 在C++连线中查找匹配项
        for (DAPyLinkGraphicsItem* link : linkItems) {
            DAPyNodeGraphicsItem* fromItem = link->getFromNode();
            DAPyNodeGraphicsItem* toItem   = link->getToNode();
            if (!fromItem || !toItem) {
                continue;
            }
            // 通过node_id匹配
            QString fromNodeId = d->mNodeIdMap.value(fromItem);
            QString toNodeId   = d->mNodeIdMap.value(toItem);
            if (fromNodeId == srcId && toNodeId == dstId && link->getFromOutputName() == srcCh
                && link->getToInputName() == dstCh) {
                d->mLinkConnectionIdMap[ link ] = connId;
                break;
            }
        }
    }
}

/**
 * @brief 保存场景到XML
 *
 * 通过DAPyWorkFlowSceneSerializer委托序列化操作。
 * 保存场景中所有节点位置、连接信息和参数值。
 *
 * @param doc XML文档指针
 * @param parentElement 父元素指针
 * @param ver 版本号
 * @return 保存成功返回true，失败返回false
 * @see DAPyWorkFlowSceneSerializer
 */
bool DAPyWorkFlowScene::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
    DAPyWorkFlowSceneSerializer serializer;
    // 先保存到临时doc
    if (!serializer.saveSceneToXml(this, doc, ver)) {
        daWarning << tr("DAPyWorkFlowScene::saveToXml failed: %1").arg(serializer.getLastErrorString());  // cn:保存场景到 XML 失败：%1
        return false;
    }
    // 将serializer创建的文档内容合并到parentElement
    QDomElement sceneEle = doc->documentElement();
    if (!sceneEle.isNull() && parentElement) {
        parentElement->appendChild(sceneEle);
    }
    return true;
}

/**
 * @brief 从XML加载场景
 *
 * 通过DAPyWorkFlowSceneSerializer委托反序列化操作。
 * 加载后不会自动执行工作流。
 *
 * @param parentElement XML父元素
 * @param ver 版本号
 * @return 加载成功返回true，失败返回false
 * @see DAPyWorkFlowSceneSerializer
 * @note 加载后不会自动执行工作流
 */
bool DAPyWorkFlowScene::loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver)
{
    DAPyWorkFlowSceneSerializer serializer;
    QDomElement sceneEle;
    if (parentElement) {
        // 查找DAPyWorkFlowScene子元素
        sceneEle = parentElement->firstChildElement("DAPyWorkFlowScene");
        if (sceneEle.isNull()) {
            // 如果parentElement本身就是DAPyWorkFlowScene元素
            sceneEle = *parentElement;
        }
    }
    if (sceneEle.isNull()) {
        daWarning << tr("DAPyWorkFlowScene::loadFromXml: DAPyWorkFlowScene element not found");  // cn:从 XML 加载场景失败：未找到 DAPyWorkFlowScene 元素
        return false;
    }
    if (!serializer.loadSceneFromXml(&sceneEle, this, ver)) {
        daWarning << tr("DAPyWorkFlowScene::loadFromXml failed: %1").arg(serializer.getLastErrorString());  // cn:从 XML 加载场景失败：%1
        return false;
    }
    // 加载后重建节点到连接线的映射表
    rebuildNodeLinksMap();
    return true;
}

/**
 * @brief 保存场景到文件
 *
 * 将场景序列化为XML并写入指定文件。
 *
 * @param filePath 文件路径
 * @param ver 版本号
 * @return 保存成功返回true，失败返回false
 */
bool DAPyWorkFlowScene::saveToFile(const QString& filePath, const QVersionNumber& ver)
{
    DAPyWorkFlowSceneSerializer serializer;
    if (!serializer.saveSceneToFile(this, filePath, ver)) {
        daWarning << tr("DAPyWorkFlowScene::saveToFile failed: %1").arg(serializer.getLastErrorString());  // cn:保存场景到文件失败：%1
        return false;
    }
    return true;
}

/**
 * @brief 从文件加载场景
 *
 * 从指定文件读取XML并恢复场景状态。
 * 加载后不会自动执行工作流。
 *
 * @param filePath 文件路径
 * @param ver 版本号
 * @return 加载成功返回true，失败返回false
 * @note 加载后不会自动执行工作流
 */
bool DAPyWorkFlowScene::loadFromFile(const QString& filePath, const QVersionNumber& ver)
{
    DAPyWorkFlowSceneSerializer serializer;
    if (!serializer.loadSceneFromFile(filePath, this, ver)) {
        daWarning << tr("DAPyWorkFlowScene::loadFromFile failed: %1").arg(serializer.getLastErrorString());  // cn:从文件加载场景失败：%1
        return false;
    }
    return true;
}

/**
 * @brief 取消链接模式
 *
 * 如果当前正在进行连线操作，取消连线并清理临时状态。
 */
void DAPyWorkFlowScene::cancelLink()
{
    DAPyLinkGraphicsItem* linkItem = dynamic_cast< DAPyLinkGraphicsItem* >(getCurrentLinkItem());
    if (linkItem) {
#ifdef DA_PYWORKFLOW_DEBUG
        DAPyNodeGraphicsItem* fromNode = linkItem->getFromNode();
        if (fromNode) {
            qDebug() << "DAPyWorkFlowScene::cancelLink: Disconnecting from node" << fromNode->getNodeName();
        }
        DAPyNodeGraphicsItem* toNode = linkItem->getToNode();
        if (toNode) {
            qDebug() << "DAPyWorkFlowScene::cancelLink: Disconnecting to node" << toNode->getNodeName();
        }
#endif
    }
    DAGraphicsScene::cancelLink();
}

/**
 * @brief 同步Python端节点连接添加
 * @param[in] linkItem 连接线图形项
 */
void DAPyWorkFlowScene::syncPyNodeLinkAdd(DAPyLinkGraphicsItem* linkItem)
{
    d_ptr->syncPyNodeLinkAdd(linkItem);
}

/**
 * @brief 同步Python端节点连接移除
 * @param[in] linkItem 连接线图形项
 */
void DAPyWorkFlowScene::syncPyNodeLinkRemove(DAPyLinkGraphicsItem* linkItem)
{
    d_ptr->syncPyNodeLinkRemove(linkItem);
}

/**
 * @brief 同步Python端节点注册
 * @param[in] nodeItem 节点图形项
 * @return Python分配的nodeId
 */
QString DAPyWorkFlowScene::syncPyNodeRegister(DAPyNodeGraphicsItem* nodeItem)
{
    return d_ptr->syncPyNodeRegister(nodeItem);
}

/**
 * @brief 同步Python端节点注销
 * @param[in] nodeItem 节点图形项
 */
void DAPyWorkFlowScene::syncPyNodeUnregister(DAPyNodeGraphicsItem* nodeItem)
{
    d_ptr->syncPyNodeUnregister(nodeItem);
}

/**
 * @brief 处理DAGraphicsItem选中变更
 *
 * @param item 选中的DAGraphicsItem
 */
void DAPyWorkFlowScene::onSelectItemChanged(DAGraphicsItem* item)
{
    if (DAPyNodeGraphicsItem* gi = dynamic_cast< DAPyNodeGraphicsItem* >(item)) {
        emit selectPyNodeItemChanged(gi);
    }
}

/**
 * @brief 处理连接线选中变更
 *
 * @param item 选中的DAGraphicsLinkItem
 */
void DAPyWorkFlowScene::onSelectLinkChanged(DAGraphicsLinkItem* item)
{
    if (DAPyLinkGraphicsItem* gi = dynamic_cast< DAPyLinkGraphicsItem* >(item)) {
        emit selectPyNodeLinkChanged(gi);
    }
}

/**
 * @brief 处理Python节点状态变更通知
 *
 * 由DAPythonSignalHandler::callInMainThread触发，
 * 当Python侧节点状态发生变化时更新对应的图形项状态颜色。
 *
 * @param nodeId Python节点的唯一标识
 * @param state 新的节点状态
 */
void DAPyWorkFlowScene::onPyNodeStateNotification(const QString& nodeId, DAPyNodeState state)
{
    DA_D(d);
    DAPyNodeGraphicsItem* item = d->mNodeIdToItemMap.value(nodeId);
    if (item) {
        item->setNodeState(state);
        emit pyNodeStateChanged(item, state);
    }
}

/**
 * @brief 处理节点执行完成通知
 *
 * 由 DAPyWorkFlowManager::nodeExecuted 信号触发，
 * 根据执行结果设置节点状态（Success/Error），
 * DAPyNodeGraphicsItem::setNodeState 内部会调用 update() 触发重绘，
 * 进而调用 paintBody() → Python paint() 回调刷新节点显示内容。
 *
 * @param[in] nodeId 节点唯一标识
 * @param[in] success 执行是否成功
 */
void DAPyWorkFlowScene::onNodeExecuted(const QString& nodeId, bool success)
{
    DA_D(d);
    DAPyNodeGraphicsItem* item = d->mNodeIdToItemMap.value(nodeId);
    if (item) {
        item->setNodeState(success ? DAPyNodeState::Success : DAPyNodeState::Error);
    }
}

/**
 * @brief 鼠标按下事件
 *
 * 处理节点连接点的交互：点击输出端口开始连线，点击输入端口完成连线。
 *
 * @param mouseEvent 鼠标事件
 */
void DAPyWorkFlowScene::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    DA_D(d);
    bool linkHandled = false;

    if (mouseEvent->isAccepted()) {
        // 如果被上游接受了鼠标事件，则需要取消链接
        if (isStartLink()) {
            cancelLink();
        }
    } else if (!isIgnoreLinkEvent() && mouseEvent->buttons().testFlag(Qt::LeftButton)) {
        DAPyNodeGraphicsItem* nodeItem = nodeItemAt(mouseEvent->scenePos());
        if (nodeItem) {
            if (isStartLink()) {
                // 正在连线状态，点击目标输入端口完成连线
                DAPyLinkGraphicsItem* linkItem = dynamic_cast< DAPyLinkGraphicsItem* >(getCurrentLinkItem());
                if (linkItem) {
                    // 查找输入端口
                    const QList< DAPyLinkPoint > inputPoints = nodeItem->getInputLinkPoints();
                    DAPyLinkPoint matchedPoint;
                    for (const DAPyLinkPoint& lp : inputPoints) {
                        QRectF hitRegion = nodeItem->mapRectToScene(lp.hitRegion());
                        if (hitRegion.contains(mouseEvent->scenePos())) {
                            matchedPoint = lp;
                            break;
                        }
                    }

                    if (!matchedPoint.isValid() || matchedPoint.isOutput()) {
                        setIgnoreLinkEvent(true);
                        DAGraphicsScene::mousePressEvent(mouseEvent);
                        setIgnoreLinkEvent(false);
                        linkHandled = true;
                    } else {
                        // 连接到目标节点
                        linkItem->setToNode(nodeItem, matchedPoint.name);
                        linkItem->setEndScenePosition(nodeItem->mapToScene(matchedPoint.position));
                        linkItem->updateBoundingRect();

                        // 带undo的添加连接线
                        addPyNodeLink_(linkItem);
                        endLink();
                    }
                }
            } else {
                // 非连线状态，点击输出端口开始连线
                const QList< DAPyLinkPoint > outputPoints = nodeItem->getOutputLinkPoints();
                DAPyLinkPoint matchedPoint;
                for (const DAPyLinkPoint& lp : outputPoints) {
                    QRectF hitRegion = nodeItem->mapRectToScene(lp.hitRegion());
                    if (hitRegion.contains(mouseEvent->scenePos())) {
                        matchedPoint = lp;
                        break;
                    }
                }

                if (matchedPoint.isValid() && matchedPoint.isOutput()) {
                    // 开始连线
                    DAPyLinkGraphicsItem* linkItem = new DAPyLinkGraphicsItem();
                    linkItem->setFromNode(nodeItem, matchedPoint.name);
                    linkItem->setStartScenePosition(nodeItem->mapToScene(matchedPoint.position));
                    beginLink(linkItem);
                }
            }
        }
    }

    // 调用基类处理选择状态和默认事件分发
    if (!linkHandled) {
        DAGraphicsScene::mousePressEvent(mouseEvent);
    }

    // 多选拖拽设置：左键按下、非连线模式、有2个以上可选图元时激活
    if (mouseEvent->button() == Qt::LeftButton && !isStartLink() && !isReadOnly()) {
        QList< QGraphicsItem* > movableItems = getSelectedMovableItems();
        if (movableItems.size() > 1) {
            d->mMultiMoveActive = true;
            d->mMultiMoveItems  = movableItems;
            d->mMultiMoveStartPositions.clear();
            for (QGraphicsItem* item : std::as_const(movableItems)) {
                d->mMultiMoveStartPositions.append(item->pos());
            }
            d->mMultiMoveLastScenePos = mouseEvent->scenePos();
            mouseEvent->accept();
        }
    }
}

/**
 * @brief 多选拖拽时的鼠标移动处理
 *
 * 当mMultiMoveActive为true时，手动移动所有选中图元（增量式），
 * 阻止QGraphicsScene的默认单项拖拽行为。
 */
void DAPyWorkFlowScene::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    DA_D(d);
    if (d->mMultiMoveActive) {
        QPointF currentPos = mouseEvent->scenePos();
        QPointF delta      = currentPos - d->mMultiMoveLastScenePos;

        if (!delta.isNull()) {
            for (QGraphicsItem* item : std::as_const(d->mMultiMoveItems)) {
                item->setPos(item->pos() + delta);
            }
            d->mMultiMoveLastScenePos = currentPos;
        }
        mouseEvent->accept();
        return;
    }
    DAGraphicsScene::mouseMoveEvent(mouseEvent);
}

/**
 * @brief 多选拖拽结束时的鼠标释放处理
 *
 * 当mMultiMoveActive为true时，创建DACommandsForGraphicsItemsMoved命令推入undo栈，
 * 确保多选拖拽支持撤销/重做。同时重置工厂的移动周期状态以避免重复命令。
 */
void DAPyWorkFlowScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    DA_D(d);
    if (d->mMultiMoveActive) {
        d->mMultiMoveActive = false;

        // 收集结束位置
        QList< QPointF > endPositions;
        bool hasMovement = false;
        for (int i = 0; i < d->mMultiMoveItems.size(); ++i) {
            QPointF endPos = d->mMultiMoveItems[ i ]->pos();
            endPositions.append(endPos);
            if (d->mMultiMoveItems[ i ]->pos() != d->mMultiMoveStartPositions[ i ]) {
                hasMovement = true;
            }
        }

        if (hasMovement && !d->mMultiMoveItems.isEmpty()) {
            // 创建多选移动命令（skipfirst=true，因为图元已经移动到结束位置）
            auto cmd =
                commandsFactory()->createItemsMoved(d->mMultiMoveItems, d->mMultiMoveStartPositions, endPositions, true);
            if (cmd) {
                // 提取信号数据（push后cmd可能被mergeWith合并导致悬空）
                QList< QGraphicsItem* > moveItems = cmd->getItems();
                QList< QPointF > startsPos        = cmd->getStartsPos();
                QList< QPointF > endsPos          = cmd->getEndsPos();
                push(cmd);
                Q_EMIT itemsPositionChanged(moveItems, startsPos, endsPos);
            }
        }

        d->mMultiMoveItems.clear();
        d->mMultiMoveStartPositions.clear();
        mouseEvent->accept();
        return;
    }
    DAGraphicsScene::mouseReleaseEvent(mouseEvent);
}

/**
 * @brief 对场景中的item进行分类
 *
 * 将item列表分为DAPyNodeGraphicsItem、DAPyLinkGraphicsItem和其他三类。
 *
 * @param sourceItems 原始item列表
 * @param nodeItems 分离出的节点item列表
 * @param linkItems 分离出的连接线item列表
 * @param normalItems 分离出的普通item列表
 */
void DAPyWorkFlowScene::classifyItems(const QList< QGraphicsItem* >& sourceItems,
                                      QList< DAPyNodeGraphicsItem* >& nodeItems,
                                      QList< DAPyLinkGraphicsItem* >& linkItems,
                                      QList< QGraphicsItem* >& normalItems)
{
    if (sourceItems.isEmpty()) {
        return;
    }
    for (QGraphicsItem* i : sourceItems) {
        if (DAPyNodeGraphicsItem* ni = dynamic_cast< DAPyNodeGraphicsItem* >(i)) {
            nodeItems.append(ni);
        } else if (DAPyLinkGraphicsItem* li = dynamic_cast< DAPyLinkGraphicsItem* >(i)) {
            linkItems.append(li);
        } else {
            normalItems.append(i);
        }
    }
}

/**
 * @brief 获取节点item的所有连接线
 *
 * 基于映射表高效查询，避免遍历全场景items
 *
 * @param nodeItems 节点item列表
 * @return 与节点关联的连接线列表（去重）
 */
QList< DAPyLinkGraphicsItem* > DAPyWorkFlowScene::getNodesAllLinkItems(const QList< DAPyNodeGraphicsItem* >& nodeItems) const
{
    DA_DC(dc);
    QSet< DAPyLinkGraphicsItem* > resultSet;
    for (DAPyNodeGraphicsItem* n : nodeItems) {
        const QList< DAPyLinkGraphicsItem* > links = dc->mNodeToLinksMap.value(n);
        for (DAPyLinkGraphicsItem* link : links) {
            resultSet.insert(link);
        }
    }
    return QList< DAPyLinkGraphicsItem* >(resultSet.begin(), resultSet.end());
}

/**
 * @brief 创建连接线的工厂函数，注意hintFromItem和hintFromOutput是一个试探性的输入，传入空值也可以，这两个参数是为了适配不同的连接点伸出不同连接线做准备的
 * @param hintFromItem 开始链接节点
 * @param hintFromOutput 开始节点名
 * @return 返回DAPyLinkGraphicsItem，注意这个函数只负责创建，不会进行链接绑定
 */
DAPyLinkGraphicsItem* DAPyWorkFlowScene::createLinkItem(DAPyNodeGraphicsItem* hintFromItem, const QString& hintFromOutput)
{
    Q_UNUSED(hintFromItem);
    Q_UNUSED(hintFromOutput)
    return new DAPyLinkGraphicsItem();
}

/**
 * @brief 初始化信号连接
 *
 * 连接DAGraphicsScene的选择变更信号到本类的处理槽。
 */
void DAPyWorkFlowScene::initConnect()
{
    connect(this, &DAGraphicsScene::selectItemChanged, this, &DAPyWorkFlowScene::onSelectItemChanged);
    connect(this, &DAGraphicsScene::selectLinkChanged, this, &DAPyWorkFlowScene::onSelectLinkChanged);
}

/**
 * @brief 重建节点到连接线的映射表
 *
 * 遍历场景中所有的节点和连接线，重新构建映射表。
 * 在undo/redo恢复连接线后或从文件加载场景后调用，
 * 确保映射表与场景实际状态一致。
 */
void DAPyWorkFlowScene::rebuildNodeLinksMap()
{
    DA_D(d);
    d->mNodeToLinksMap.clear();
    d->mNodeIdMap.clear();
    d->mNodeIdToItemMap.clear();
    d->mLinkConnectionIdMap.clear();
    const QList< DAPyNodeGraphicsItem* > nodeItems = getPyNodeItems();
    const QList< DAPyLinkGraphicsItem* > linkItems = getPyNodeLinkItems();
    // 重建节点nodeId映射（从Python节点对象提取node_id）
    for (DAPyNodeGraphicsItem* node : nodeItems) {
        const DAPyNode& proxy = node->getProxy();
        if (!proxy.isNone()) {
            QString nodeId = proxy.getNodeId();
            if (!nodeId.isEmpty()) {
                d->registerNode(node, nodeId);
            }
        }
    }
    // 重建连接线映射
    for (DAPyLinkGraphicsItem* link : linkItems) {
        DAPyNodeGraphicsItem* fromNode = link->getFromNode();
        DAPyNodeGraphicsItem* toNode   = link->getToNode();
        if (fromNode) {
            d->mNodeToLinksMap[ fromNode ].append(link);
        }
        if (toNode) {
            d->mNodeToLinksMap[ toNode ].append(link);
        }
    }
}

}  // namespace DA
