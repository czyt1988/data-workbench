#include "DAPyWorkFlowScene.h"
#include "DAPybind11InQt.h"
#include <QGraphicsSceneMouseEvent>
#include <QPointer>
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAPythonSignalHandler.h"
#include "DAPyModuleWorkflow.h"
#include "DAPyGILGuard.h"
#include "DAPyNodeProxy.h"
#include "DAPyNodeFactory.h"
#include "DAPyLinkPoint.h"
#include "DAGraphicsScene.h"
#include "DAPyWorkFlowSceneSerializer.h"

namespace DA
{
class DAPyWorkFlowScene::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyWorkFlowScene)
public:
    PrivateData(DAPyWorkFlowScene* p);

    // Python DAWorkflow对象引用
    DAPySafePyObjectHolder mPyWorkflow;
    // Python信号处理器
    QPointer< DAPythonSignalHandler > mSignalHandler;
    // Python节点工厂（用于创建DAPyNodeProxy实例）
    std::shared_ptr< DAPyNodeFactory > mPyNodeFactory;
};

DAPyWorkFlowScene::PrivateData::PrivateData(DAPyWorkFlowScene* p) : q_ptr(p)
{
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
    initPyWorkflow();
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
    initPyWorkflow();
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
    initPyWorkflow();
}

/**
 * @brief 析构函数
 *
 * 析构时清空场景中的所有Python节点和连接线。
 */
DAPyWorkFlowScene::~DAPyWorkFlowScene()
{
    clearPyScene();
}

/**
 * @brief 设置Python DAWorkflow实例引用
 *
 * 存储Python DAWorkflow对象的引用，用于同步节点和连接操作。
 * 使用DAPySafePyObjectHolder安全持有Python对象。
 *
 * @param workflow Python DAWorkflow实例的pybind11::object
 */
void DAPyWorkFlowScene::setPyWorkflow(const pybind11::object& workflow)
{
    DA_D(d);
    d->mPyWorkflow = workflow;
}

/**
 * @brief 获取Python DAWorkflow实例引用
 *
 * @return Python DAWorkflow对象的pybind11::object引用
 */
pybind11::object DAPyWorkFlowScene::getPyWorkflow() const
{
    DA_DC(d);
    return d->mPyWorkflow.object();
}

/**
 * @brief 判断是否已设置Python DAWorkflow实例
 *
 * @return 如果已设置返回true，否则返回false
 */
bool DAPyWorkFlowScene::hasPyWorkflow() const
{
    DA_DC(d);
    return !d->mPyWorkflow.isNone();
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
 * @brief 设置Python节点工厂
 *
 * 设置DAPyNodeFactory用于创建DAPyNodeProxy实例，
 * 工厂负责Python模块导入、节点实例创建和代理设置。
 * 如果未设置工厂，createPyNode()会回退到直接创建DAPyNodeProxy。
 *
 * @param[in] factory Python节点工厂的共享指针
 */
void DAPyWorkFlowScene::setPyNodeFactory(std::shared_ptr<DAPyNodeFactory> factory)
{
    DA_D(d);
    d->mPyNodeFactory = factory;
}

/**
 * @brief 获取Python节点工厂
 *
 * @return 当前设置的Python节点工厂共享指针，未设置时返回nullptr
 */
std::shared_ptr<DAPyNodeFactory> DAPyWorkFlowScene::getPyNodeFactory() const
{
    DA_DC(d);
    return d->mPyNodeFactory;
}

/**
 * @brief 创建Python节点图形项（不添加到场景）
 *
 * 根据描述符字典创建DAPyNodeGraphicsItem和DAPyNodeProxy，
 * 在Python侧调用DAWorkflow.add_node()同步创建节点。
 * 此方法仅创建图形项但不添加到场景，由调用方决定添加方式。
 *
 * @param descriptor 节点描述符，包含节点类型、名称、输入/输出端口等信息
 * @param pos 节点在场景中的初始位置（仅记录，不执行setPosition）
 * @return 创建的DAPyNodeGraphicsItem指针，创建失败返回nullptr
 * @note 返回的item未添加到场景，需要调用方自行添加
 */
DAPyNodeGraphicsItem* DAPyWorkFlowScene::createPyNode(const QJsonObject& descriptor, const QPointF& pos)
{
    DA_D(d);
    if (!d->mPyWorkflow) {
        qWarning() << tr("DAPyWorkFlowScene::createPyNode: Python workflow is not set");
        return nullptr;
    }

    // 从描述符提取节点信息
    QString qualifiedName = descriptor.value("qualified_name").toString();
    if (qualifiedName.isEmpty()) {
        qWarning() << tr("DAPyWorkFlowScene::createPyNode: descriptor missing qualified_name");
        return nullptr;
    }

    // 创建DAPyNodeProxy
    DAPyNodeProxy* proxy = nullptr;
    if (d->mPyNodeFactory) {
        // 通过工厂创建代理，工厂内部处理Python模块导入和setPyNodeRef
        proxy = d->mPyNodeFactory->createNodeProxy(qualifiedName);
        if (!proxy) {
            qWarning() << tr("DAPyWorkFlowScene::createPyNode: factory failed to create proxy for %1").arg(qualifiedName);
            return nullptr;
        }
    } else {
        // 未设置工厂时回退到直接创建（向后兼容）
        proxy = new DAPyNodeProxy();
        proxy->setQualifiedName(qualifiedName);
    }

    // 在Python侧注册节点到DAWorkflow
    {
        DAPyGILGuard gil;
        try {
            pybind11::object workflowObj = d->mPyWorkflow.object();
            if (!workflowObj) {
                qWarning() << tr("DAPyWorkFlowScene::createPyNode: workflow object is invalid");
                delete proxy;
                return nullptr;
            }
            if (d->mPyNodeFactory) {
                // 工厂已创建实例并设置setPyNodeRef，只需注册到workflow
                workflowObj.attr("add_node")(proxy->getPyNodeRef());
            } else {
                // 无工厂时，手动导入Python模块创建节点实例
                std::string qn = qualifiedName.toStdString();
                size_t dotPos  = qn.rfind('.');
                if (dotPos == std::string::npos) {
                    qWarning() << tr("DAPyWorkFlowScene::createPyNode: invalid qualified_name: %1").arg(qualifiedName);
                    delete proxy;
                    return nullptr;
                }
                std::string moduleName = qn.substr(0, dotPos);
                std::string className  = qn.substr(dotPos + 1);

                pybind11::module_ pyMod       = pybind11::module_::import(moduleName.c_str());
                pybind11::object nodeClassObj = pyMod.attr(className.c_str());

                // 创建Python节点实例
                pybind11::object pyNodeInstance = nodeClassObj();

                // 调用Python DAWorkflow.add_node()注册节点实例
                workflowObj.attr("add_node")(pyNodeInstance);
                proxy->setPyNodeRef(pyNodeInstance);
            }
        } catch (const pybind11::error_already_set& e) {
            qWarning() << tr("DAPyWorkFlowScene::createPyNode: Python error: %1").arg(e.what());
            delete proxy;
            return nullptr;
        }
    }

    // 创建图形项
    DAPyNodeGraphicsItem* item = new DAPyNodeGraphicsItem(proxy);
    item->setDescriptor(descriptor);
    item->setNodeName(descriptor.value("name").toString(qualifiedName));

    // 根据描述符设置渲染模板
    QString renderTemplate = descriptor.value("render_template").toString("rect");
    item->setRenderTemplate(renderTemplate);

    // 设置图标路径（如果有）
    QString iconPath = descriptor.value("icon").toString();
    if (!iconPath.isEmpty()) {
        if (renderTemplate == "svg") {
            item->setSvgPath(iconPath);
        } else {
            item->setIcon(QIcon(iconPath));
        }
    }

    // 更新连接点
    item->updateLinkPoints();

    // 设置描述符中的body尺寸（如果有）
    if (descriptor.contains("size")) {
        QJsonObject sizeObj = descriptor.value("size").toObject();
        double w            = sizeObj.value("width").toDouble(120);
        double h            = sizeObj.value("height").toDouble(80);
        item->setBodySize(QSizeF(w, h));
    }

    // 设置位置（未添加到场景）
    item->setPos(pos);

    return item;
}

/**
 * @brief 创建Python节点（带undo/redo）
 *
 * 通过QUndoStack记录创建操作，支持撤销和重做。
 * 先调用createPyNode()创建节点图形项（不添加到场景），
 * 然后通过addItem_()将item添加到场景并推入undo栈。
 *
 * @param descriptor 节点描述符
 * @param pos 节点在场景中的初始位置
 * @return 创建的DAPyNodeGraphicsItem指针，创建失败返回nullptr
 * @note 函数名后缀"_"表示支持undo/redo操作
 */
DAPyNodeGraphicsItem* DAPyWorkFlowScene::createPyNode_(const QJsonObject& descriptor, const QPointF& pos)
{
    DAPyNodeGraphicsItem* item = createPyNode(descriptor, pos);
    if (!item) {
        return nullptr;
    }
    // 通过addItem_()添加到场景并推入undo栈
    addItem_(item);
    emit pyNodeItemCreated(item);
    return item;
}

/**
 * @brief 创建Python节点图形项（通过元数据，不添加到场景）
 *
 * 推荐的节点创建路径，通过DAPyNodeMetaData创建节点，
 * 避免了QJsonObject路径中setDescriptor()覆盖完整描述符的数据丢失问题。
 * 工厂创建代理时已获取Python侧完整descriptor（含inputs/outputs），
 * DAPyNodeGraphicsItem构造函数自动从代理同步描述符和连接点，
 * 此方法不再调用setDescriptor()覆盖，仅设置元数据中的显示属性。
 *
 * @param[in] metaData 节点元数据，包含qualified_name、name、icon等
 * @param[in] pos 节点在场景中的初始位置
 * @return 创建的DAPyNodeGraphicsItem指针，创建失败返回nullptr
 * @note 返回的item未添加到场景，需要调用方自行添加
 * @note 不调用setDescriptor()（Bug 2修复），保留代理中的完整描述符
 * @see createPyNode(const QJsonObject&, const QPointF&)
 */
DAPyNodeGraphicsItem* DAPyWorkFlowScene::createPyNode(const DAPyNodeMetaData& metaData, const QPointF& pos)
{
    DA_D(d);
    if (!d->mPyWorkflow) {
        qWarning() << tr("DAPyWorkFlowScene::createPyNode: Python workflow is not set");
        return nullptr;
    }

    if (!metaData.isValid()) {
        qWarning() << tr("DAPyWorkFlowScene::createPyNode: invalid metadata (qualified_name: %1)")
                       .arg(metaData.qualifiedName);
        return nullptr;
    }

    // 创建DAPyNodeProxy
    DAPyNodeProxy* proxy = nullptr;
    if (d->mPyNodeFactory) {
        // 通过工厂创建代理，工厂内部处理Python模块导入、实例创建和setPyNodeRef
        proxy = d->mPyNodeFactory->createNodeProxy(metaData);
        if (!proxy) {
            qWarning() << tr("DAPyWorkFlowScene::createPyNode: factory failed to create proxy for %1")
                           .arg(metaData.qualifiedName);
            return nullptr;
        }
    } else {
        // 未设置工厂时回退到直接创建（向后兼容）
        proxy = new DAPyNodeProxy();
        proxy->setQualifiedName(metaData.qualifiedName);
    }

    // 在Python侧注册节点到DAWorkflow
    {
        DAPyGILGuard gil;
        try {
            pybind11::object workflowObj = d->mPyWorkflow.object();
            if (!workflowObj) {
                qWarning() << tr("DAPyWorkFlowScene::createPyNode: workflow object is invalid");
                delete proxy;
                return nullptr;
            }
            if (d->mPyNodeFactory) {
                // 工厂已创建实例并设置setPyNodeRef，只需注册到workflow
                workflowObj.attr("add_node")(proxy->getPyNodeRef());
            } else {
                // 无工厂时，手动导入Python模块创建节点实例
                std::string qn = metaData.qualifiedName.toStdString();
                size_t dotPos  = qn.rfind('.');
                if (dotPos == std::string::npos) {
                    qWarning() << tr("DAPyWorkFlowScene::createPyNode: invalid qualified_name: %1")
                                   .arg(metaData.qualifiedName);
                    delete proxy;
                    return nullptr;
                }
                std::string moduleName = qn.substr(0, dotPos);
                std::string className  = qn.substr(dotPos + 1);

                pybind11::module_ pyMod       = pybind11::module_::import(moduleName.c_str());
                pybind11::object nodeClassObj = pyMod.attr(className.c_str());

                // 创建Python节点实例
                pybind11::object pyNodeInstance = nodeClassObj();

                // 调用Python DAWorkflow.add_node()注册节点实例
                workflowObj.attr("add_node")(pyNodeInstance);
                proxy->setPyNodeRef(pyNodeInstance);
            }
        } catch (const pybind11::error_already_set& e) {
            qWarning() << tr("DAPyWorkFlowScene::createPyNode: Python error: %1").arg(e.what());
            delete proxy;
            return nullptr;
        }
    }

    // Bug 2修复：构造函数已从proxy获取完整descriptor（含inputs/outputs），
    // 不再调用setDescriptor()覆盖为薄描述符，保留代理中的完整数据
    DAPyNodeGraphicsItem* item = new DAPyNodeGraphicsItem(proxy);

    // 设置节点显示名称（metadata.name可能不同于Python默认名称）
    if (!metaData.name.isEmpty()) {
        item->setNodeName(metaData.name);
    }

    // 设置图标（如果有）
    if (!metaData.iconPath.isEmpty()) {
        item->setIcon(QIcon(metaData.iconPath));
    }

    // 设置位置（未添加到场景）
    item->setPos(pos);

    return item;
}

/**
 * @brief 创建Python节点（通过元数据，带undo/redo）
 *
 * 通过QUndoStack记录创建操作，支持撤销和重做。
 * 先调用createPyNode(DAPyNodeMetaData)创建节点图形项（不添加到场景），
 * 然后通过addItem_()将item添加到场景并推入undo栈。
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
    // 通过addItem_()添加到场景并推入undo栈
    addItem_(item);
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

    // 先移除所有关联的连接线
    QList< DAPyLinkGraphicsItem* > relatedLinks;
    QList< DAPyLinkGraphicsItem* > allLinks = getPyNodeLinkItems();
    for (DAPyLinkGraphicsItem* link : allLinks) {
        if (link->getFromNode() == item || link->getToNode() == item) {
            relatedLinks.append(link);
        }
    }
    for (DAPyLinkGraphicsItem* link : relatedLinks) {
        removeItem(link);
        // 同步Python侧连接移除
        if (d->mPyWorkflow) {
            DAPyGILGuard gil;
            try {
                pybind11::object workflowObj = d->mPyWorkflow.object();
                if (workflowObj) {
                    workflowObj.attr("remove_connection")(
                        link->getFromNode()->getProxy()->getPyNodeRef(),
                        link->getFromOutputName().toStdString(),
                        link->getToNode()->getProxy()->getPyNodeRef(),
                        link->getToInputName().toStdString()
                    );
                }
            } catch (const pybind11::error_already_set& e) {
                qWarning() << tr("DAPyWorkFlowScene::removePyNodeItem: Python error removing connection: %1").arg(e.what());
            }
        }
        delete link;
    }

    // 获取节点代理
    DAPyNodeProxy* proxy = item->getProxy();

    // 同步Python侧节点移除
    if (d->mPyWorkflow && proxy && proxy->hasPyNodeRef()) {
        DAPyGILGuard gil;
        try {
            pybind11::object workflowObj = d->mPyWorkflow.object();
            if (workflowObj) {
                workflowObj.attr("remove_node")(proxy->getPyNodeRef());
            }
        } catch (const pybind11::error_already_set& e) {
            qWarning() << tr("DAPyWorkFlowScene::removePyNodeItem: Python error removing node: %1").arg(e.what());
        }
    }

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
 * 先移除节点关联的所有连接线，再移除节点图形项，
 * 所有移除操作通过removeItem_()推入undo栈。
 *
 * @param item 要移除的DAPyNodeGraphicsItem指针
 * @note 函数名后缀"_"表示支持undo/redo操作
 */
void DAPyWorkFlowScene::removePyNodeItem_(DAPyNodeGraphicsItem* item)
{
    if (!item) {
        return;
    }
    // 先移除所有关联的连接线（带undo）
    QList< DAPyLinkGraphicsItem* > relatedLinks;
    QList< DAPyLinkGraphicsItem* > allLinks = getPyNodeLinkItems();
    for (DAPyLinkGraphicsItem* link : allLinks) {
        if (link->getFromNode() == item || link->getToNode() == item) {
            relatedLinks.append(link);
        }
    }
    for (DAPyLinkGraphicsItem* link : relatedLinks) {
        removeItem_(link);
    }
    // 移除节点item（带undo）
    removeItem_(item);
    emit pyNodeItemsRemoved({ item });
}

/**
 * @brief 通过DAPyNodeProxy查找节点图形项
 *
 * @param proxy DAPyNodeProxy指针
 * @return 对应的DAPyNodeGraphicsItem指针，未找到返回nullptr
 */
DAPyNodeGraphicsItem* DAPyWorkFlowScene::findNodeItemByProxy(DAPyNodeProxy* proxy) const
{
    if (!proxy) {
        return nullptr;
    }
    QList< QGraphicsItem* > its = items();
    for (QGraphicsItem* i : std::as_const(its)) {
        if (DAPyNodeGraphicsItem* ni = dynamic_cast< DAPyNodeGraphicsItem* >(i)) {
            if (ni->getProxy() == proxy) {
                return ni;
            }
        }
    }
    return nullptr;
}

/**
 * @brief 通过节点ID查找节点图形项
 *
 * @param nodeId Python节点的唯一标识字符串
 * @return 对应的DAPyNodeGraphicsItem指针，未找到返回nullptr
 */
DAPyNodeGraphicsItem* DAPyWorkFlowScene::findNodeItemById(const QString& nodeId) const
{
    QList< QGraphicsItem* > its = items();
    for (QGraphicsItem* i : std::as_const(its)) {
        if (DAPyNodeGraphicsItem* ni = dynamic_cast< DAPyNodeGraphicsItem* >(i)) {
            DAPyNodeProxy* proxy = ni->getProxy();
            if (proxy && proxy->hasPyNodeRef()) {
                DAPyGILGuard gil;
                try {
                    pybind11::object pyNode = proxy->getPyNodeRef();
                    if (pyNode && pybind11::hasattr(pyNode, "id")) {
                        std::string idStr = pybind11::str(pyNode.attr("id"));
                        if (QString::fromStdString(idStr) == nodeId) {
                            return ni;
                        }
                    }
                } catch (const pybind11::error_already_set&) {
                    qWarning() << "DAPyWorkFlowScene: Python exception ignored during search";
                }
            }
        }
    }
    return nullptr;
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
DAPyLinkGraphicsItem* DAPyWorkFlowScene::addPyNodeLink(
    DAPyNodeGraphicsItem* fromItem, const QString& fromOutput, DAPyNodeGraphicsItem* toItem, const QString& toInput
)
{
    if (!fromItem || !toItem) {
        return nullptr;
    }
    DA_D(d);

    // 创建连接线
    DAPyLinkGraphicsItem* link = new DAPyLinkGraphicsItem();
    link->setFromNode(fromItem, fromOutput);
    link->setToNode(toItem, toInput);

    // 设置连接线的起止位置
    QList< DAPyLinkPoint > outputPoints = fromItem->getOutputLinkPoints();
    for (const DAPyLinkPoint& lp : outputPoints) {
        if (lp.name == fromOutput) {
            link->setStartScenePosition(fromItem->mapToScene(lp.position));
            break;
        }
    }
    QList< DAPyLinkPoint > inputPoints = toItem->getInputLinkPoints();
    for (const DAPyLinkPoint& lp : inputPoints) {
        if (lp.name == toInput) {
            link->setEndScenePosition(toItem->mapToScene(lp.position));
            break;
        }
    }

    // 设置信号处理器
    if (d->mSignalHandler) {
        link->setSignalHandler(d->mSignalHandler);
    }

    // 同步Python侧连接
    if (d->mPyWorkflow) {
        DAPyGILGuard gil;
        try {
            pybind11::object workflowObj = d->mPyWorkflow.object();
            if (workflowObj && fromItem->getProxy() && toItem->getProxy()) {
                pybind11::object fromPyNode = fromItem->getProxy()->getPyNodeRef();
                pybind11::object toPyNode   = toItem->getProxy()->getPyNodeRef();
                if (fromPyNode && toPyNode) {
                    workflowObj.attr("add_connection")(fromPyNode, fromOutput.toStdString(), toPyNode, toInput.toStdString());
                }
            }
        } catch (const pybind11::error_already_set& e) {
            qWarning() << tr("DAPyWorkFlowScene::addPyNodeLink: Python error: %1").arg(e.what());
        }
    }

    return link;
}

/**
 * @brief 添加Python节点连接线（带undo/redo）
 *
 * 通过QUndoStack记录连接操作，支持撤销和重做。
 * 先调用addPyNodeLink()创建连接线（不添加到场景），
 * 然后通过addItem_()将link添加到场景并推入undo栈。
 *
 * @param fromItem 源节点图形项
 * @param fromOutput 源节点的输出端口名称
 * @param toItem 目标节点图形项
 * @param toInput 目标节点的输入端口名称
 * @return 创建的DAPyLinkGraphicsItem指针，创建失败返回nullptr
 * @note 函数名后缀"_"表示支持undo/redo操作
 */
DAPyLinkGraphicsItem* DAPyWorkFlowScene::addPyNodeLink_(
    DAPyNodeGraphicsItem* fromItem, const QString& fromOutput, DAPyNodeGraphicsItem* toItem, const QString& toInput
)
{
    DAPyLinkGraphicsItem* link = addPyNodeLink(fromItem, fromOutput, toItem, toInput);
    if (!link) {
        return nullptr;
    }
    // 通过addItem_()添加到场景并推入undo栈
    addItem_(link);
    emit pyNodeLinkCreated(link);
    return link;
}

/**
 * @brief 移除Python节点连接线（不带undo/redo）
 *
 * 从场景中移除连接线，并同步到Python DAWorkflow.remove_connection()。
 *
 * @param linkItem 要移除的DAPyLinkGraphicsItem指针
 * @return 移除成功返回true，失败返回false
 */
bool DAPyWorkFlowScene::removePyNodeLink(DAPyLinkGraphicsItem* linkItem)
{
    if (!linkItem) {
        return false;
    }
    DA_D(d);

    // 同步Python侧连接移除
    if (d->mPyWorkflow) {
        DAPyNodeGraphicsItem* fromNode = linkItem->getFromNode();
        DAPyNodeGraphicsItem* toNode   = linkItem->getToNode();
        if (fromNode && toNode && fromNode->getProxy() && toNode->getProxy()) {
            DAPyGILGuard gil;
            try {
                pybind11::object workflowObj = d->mPyWorkflow.object();
                if (workflowObj) {
                    workflowObj.attr("remove_connection")(
                        fromNode->getProxy()->getPyNodeRef(),
                        linkItem->getFromOutputName().toStdString(),
                        toNode->getProxy()->getPyNodeRef(),
                        linkItem->getToInputName().toStdString()
                    );
                }
            } catch (const pybind11::error_already_set& e) {
                qWarning() << tr("DAPyWorkFlowScene::removePyNodeLink: Python error: %1").arg(e.what());
            }
        }
    }

    removeItem(linkItem);
    delete linkItem;
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
    removeItem_(linkItem);
    emit pyNodeLinksRemoved({ linkItem });
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
 * @brief 删除选中项（支持undo/redo）
 *
 * 删除当前场景中选中的Python节点和连接线。
 * 删除节点时会连带删除其所有连接线。
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
    for (DAPyLinkGraphicsItem* link : linkItems) {
        if (!nodeLinks.contains(link)) {
            nodeLinks.append(link);
        }
    }

    int removeCount = 0;

    // 先移除连接线（带undo）
    for (DAPyLinkGraphicsItem* link : nodeLinks) {
        removeItem_(link);
        ++removeCount;
    }

    // 再移除节点（带undo）
    for (DAPyNodeGraphicsItem* node : nodeItems) {
        removeItem_(node);
        ++removeCount;
    }

    // 移除普通图形项（带undo）
    for (QGraphicsItem* item : normalItems) {
        removeItem_(item);
        ++removeCount;
    }

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
    QList< DAPyNodeGraphicsItem* > nodeItems = getPyNodeItems();
    QList< DAPyLinkGraphicsItem* > linkItems = getPyNodeLinkItems();

    // 先移除所有连接线
    for (DAPyLinkGraphicsItem* link : linkItems) {
        removeItem(link);
        delete link;
    }

    // 同步Python侧清空
    if (d->mPyWorkflow) {
        DAPyGILGuard gil;
        try {
            pybind11::object workflowObj = d->mPyWorkflow.object();
            if (workflowObj) {
                workflowObj.attr("clear")();
            }
        } catch (const pybind11::error_already_set& e) {
            qWarning() << tr("DAPyWorkFlowScene::clearPyScene: Python error: %1").arg(e.what());
        }
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
        qWarning() << tr("DAPyWorkFlowScene::saveToXml 失败: %1").arg(serializer.getLastErrorString());
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
        qWarning() << tr("DAPyWorkFlowScene::loadFromXml: 未找到DAPyWorkFlowScene元素");
        return false;
    }
    if (!serializer.loadSceneFromXml(&sceneEle, this, ver)) {
        qWarning() << tr("DAPyWorkFlowScene::loadFromXml 失败: %1").arg(serializer.getLastErrorString());
        return false;
    }
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
        qWarning() << tr("DAPyWorkFlowScene::saveToFile 失败: %1").arg(serializer.getLastErrorString());
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
        qWarning() << tr("DAPyWorkFlowScene::loadFromFile 失败: %1").arg(serializer.getLastErrorString());
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
    DAPyNodeGraphicsItem* item = findNodeItemById(nodeId);
    if (item) {
        item->setNodeState(state);
        emit pyNodeStateChanged(item, state);
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
    if (mouseEvent->isAccepted()) {
        if (isStartLink()) {
            cancelLink();
        }
        DAGraphicsScene::mousePressEvent(mouseEvent);
        return;
    }

    if (!isIgnoreLinkEvent()) {
        if (mouseEvent->buttons().testFlag(Qt::LeftButton)) {
            DAPyNodeGraphicsItem* nodeItem = nodeItemAt(mouseEvent->scenePos());
            if (nullptr == nodeItem) {
                DAGraphicsScene::mousePressEvent(mouseEvent);
                return;
            }

            if (isStartLink()) {
                // 正在连线状态，点击目标输入端口完成连线
                DAPyLinkGraphicsItem* linkItem = dynamic_cast< DAPyLinkGraphicsItem* >(getCurrentLinkItem());
                if (linkItem) {
                    // 查找输入端口
                    QList< DAPyLinkPoint > inputPoints = nodeItem->getInputLinkPoints();
                    DAPyLinkPoint matchedPoint;
                    for (const DAPyLinkPoint& lp : inputPoints) {
                        QPointF lpScenePos = nodeItem->mapToScene(lp.position);
                        double dist        = QLineF(mouseEvent->scenePos(), lpScenePos).length();
                        if (dist < 15) {  // 连接点命中阈值
                            matchedPoint = lp;
                            break;
                        }
                    }

                    if (!matchedPoint.isValid() || matchedPoint.isOutput()) {
                        setIgnoreLinkEvent(true);
                        DAGraphicsScene::mousePressEvent(mouseEvent);
                        setIgnoreLinkEvent(false);
                        return;
                    }

                    // 连接到目标节点
                    linkItem->setToNode(nodeItem, matchedPoint.name);
                    linkItem->setEndScenePosition(nodeItem->mapToScene(matchedPoint.position));
                    linkItem->updateBoundingRect();

                    // 带undo的添加连接线
                    addPyNodeLink_(linkItem->getFromNode(), linkItem->getFromOutputName(), nodeItem, matchedPoint.name);
                }
            } else {
                // 非连线状态，点击输出端口开始连线
                QList< DAPyLinkPoint > outputPoints = nodeItem->getOutputLinkPoints();
                DAPyLinkPoint matchedPoint;
                for (const DAPyLinkPoint& lp : outputPoints) {
                    QPointF lpScenePos = nodeItem->mapToScene(lp.position);
                    double dist        = QLineF(mouseEvent->scenePos(), lpScenePos).length();
                    if (dist < 15) {
                        matchedPoint = lp;
                        break;
                    }
                }

                if (matchedPoint.isValid() && matchedPoint.isOutput()) {
                    // 开始连线
                    DAPyLinkGraphicsItem* linkItem = new DAPyLinkGraphicsItem();
                    linkItem->setFromNode(nodeItem, matchedPoint.name);
                    linkItem->setStartScenePosition(nodeItem->mapToScene(matchedPoint.position));
                    if (d_ptr->mSignalHandler) {
                        linkItem->setSignalHandler(d_ptr->mSignalHandler);
                    }
                    beginLink(linkItem);
                }
            }
        }
    }
    DAGraphicsScene::mousePressEvent(mouseEvent);
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
void DAPyWorkFlowScene::classifyItems(
    const QList< QGraphicsItem* >& sourceItems,
    QList< DAPyNodeGraphicsItem* >& nodeItems,
    QList< DAPyLinkGraphicsItem* >& linkItems,
    QList< QGraphicsItem* >& normalItems
)
{
    if (sourceItems.isEmpty()) {
        return;
    }
    for (QGraphicsItem* i : std::as_const(sourceItems)) {
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
 * 遍历所有连接线，筛选出与给定节点列表关联的连接线。
 *
 * @param nodeItems 节点item列表
 * @return 与节点关联的连接线列表
 */
QList< DAPyLinkGraphicsItem* > DAPyWorkFlowScene::getNodesAllLinkItems(const QList< DAPyNodeGraphicsItem* >& nodeItems)
{
    QList< DAPyLinkGraphicsItem* > res;
    for (DAPyNodeGraphicsItem* n : std::as_const(nodeItems)) {
        // 遍历所有连接线检查关联
        QList< QGraphicsItem* > its = n->scene()->items();
        for (QGraphicsItem* i : std::as_const(its)) {
            if (DAPyLinkGraphicsItem* link = dynamic_cast< DAPyLinkGraphicsItem* >(i)) {
                if (link->getFromNode() == n || link->getToNode() == n) {
                    if (!res.contains(link)) {
                        res.append(link);
                    }
                }
            }
        }
    }
    return res;
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
 * @brief 初始化Python DAWorkflow实例
 *
 * 通过DAPyModuleWorkflow获取DAWorkflow类引用，创建实例并设置到场景中，
 * 使得createPyNode()等方法可以正常工作。
 *
 * @note 如果Python未初始化或创建失败，不会影响场景的其他功能，
 * 后续仍可通过setPyWorkflow()手动设置。
 */
void DAPyWorkFlowScene::initPyWorkflow()
{
    DA_D(d);
    if (!d->mPyWorkflow.isNone()) {
        // 已设置，无需重复初始化
        return;
    }
    DAPyGILGuard gil;
    try {
        DAPyModuleWorkflow& pyModule = DAPyModuleWorkflow::getInstance();
        if (!pyModule.isImport()) {
            if (!pyModule.import()) {
                qWarning() << "DAPyWorkFlowScene::initPyWorkflow: cannot import DAWorkbench.DAWorkFlowPy";
                return;
            }
        }
        pybind11::object workflowClass = pyModule.getWorkflowClass();
        if (workflowClass.is_none()) {
            qWarning() << "DAPyWorkFlowScene::initPyWorkflow: DAWorkflow class is not available";
            return;
        }
        pybind11::object workflowInstance = workflowClass();
        d->mPyWorkflow                    = DAPySafePyObjectHolder(workflowInstance);
        qDebug() << "DAPyWorkFlowScene::initPyWorkflow: Python DAWorkflow instance created";
    } catch (const pybind11::error_already_set& e) {
        qWarning() << "DAPyWorkFlowScene::initPyWorkflow: Python error:" << e.what();
    } catch (const std::exception& e) {
        qWarning() << "DAPyWorkFlowScene::initPyWorkflow: error:" << e.what();
    }
}

}  // namespace DA