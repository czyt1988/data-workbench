#ifndef DAPYWORKFLOWSCENE_H
#define DAPYWORKFLOWSCENE_H
#include "DAPyWorkFlowAPI.h"
#include "DAGraphicsScene.h"
#include "DAPyNodeProxy.h"
#include "DAPyNodeState.h"
#include <QPointF>
#include <QJsonObject>
#include <QVersionNumber>
#include <memory>
class QDomDocument;
class QDomElement;

namespace DA
{
class DAPyNodeGraphicsItem;
class DAPyLinkGraphicsItem;
class DAPythonSignalHandler;
class DAPyNodeFactory;
struct DAPyNodeMetaData;
struct DANodeDescriptor;

/**
 * @brief Python工作流场景管理类
 *
 * 继承DAGraphicsScene，管理Python工作流节点的可视化渲染和交互。
 * 独立于DANodeGraphicsScene，专门用于Python DAWorkflow的可视化。
 * 支持undo/redo操作（通过DAGraphicsScene的QUndoStack），
 * Python↔C++状态同步通过DAPythonSignalHandler::callInMainThread实现。
 *
 * @see DAGraphicsScene DAPyNodeGraphicsItem DAPyLinkGraphicsItem DAPythonSignalHandler
 */
class DAPYWORKFLOW_API DAPyWorkFlowScene : public DAGraphicsScene
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyWorkFlowScene)
    friend class DAPyNodeGraphicsItem;
    friend class DAPyLinkGraphicsItem;

public:
    DAPyWorkFlowScene(QObject* parent = nullptr);
    DAPyWorkFlowScene(const QRectF& sceneRect, QObject* parent = nullptr);
    DAPyWorkFlowScene(qreal x, qreal y, qreal width, qreal height, QObject* parent = nullptr);
    ~DAPyWorkFlowScene();

    // 设置Python DAWorkflow实例引用
    void setPyWorkflow(const pybind11::object& workflow);
    pybind11::object getPyWorkflow() const;
    bool hasPyWorkflow() const;

    // 设置Python信号处理器（用于接收Python侧的状态变更通知）
    void setSignalHandler(DAPythonSignalHandler* handler);
    DAPythonSignalHandler* getSignalHandler() const;

    // 设置Python节点工厂（用于创建DAPyNodeProxy实例）
    void setPyNodeFactory(std::shared_ptr< DAPyNodeFactory > factory);
    std::shared_ptr< DAPyNodeFactory > getPyNodeFactory() const;

    // 节点管理
    // @deprecated 使用 createPyNode(const DANodeDescriptor&, const QPointF&) 代替
    DAPyNodeGraphicsItem* createPyNode(const QJsonObject& descriptor, const QPointF& pos);
    // @deprecated 使用 createPyNode_(const DANodeDescriptor&, const QPointF&) 代替
    DAPyNodeGraphicsItem* createPyNode_(const QJsonObject& descriptor, const QPointF& pos);
    // 通过节点元数据创建Python工作流节点（推荐路径，避免数据丢失）
    DAPyNodeGraphicsItem* createPyNode(const DAPyNodeMetaData& metaData, const QPointF& pos);
    // 通过节点元数据创建Python工作流节点（undo版本）
    DAPyNodeGraphicsItem* createPyNode_(const DAPyNodeMetaData& metaData, const QPointF& pos);
    // 通过节点描述符创建Python工作流节点（结构体路径，推荐）
    DAPyNodeGraphicsItem* createPyNode(const DANodeDescriptor& descriptor, const QPointF& pos);
    // 通过节点描述符创建Python工作流节点（undo版本）
    DAPyNodeGraphicsItem* createPyNode_(const DANodeDescriptor& descriptor, const QPointF& pos);
    bool removePyNodeItem(DAPyNodeGraphicsItem* item);
    void removePyNodeItem_(DAPyNodeGraphicsItem* item);

    // 查找节点
    DAPyNodeGraphicsItem* findNodeItemByProxy(DAPyNodeProxy* proxy) const;
    DAPyNodeGraphicsItem* findNodeItemById(const QString& nodeId) const;
    DAPyNodeGraphicsItem* nodeItemAt(const QPointF& scenePos) const;
    QList< DAPyNodeGraphicsItem* > getPyNodeItems() const;
    QList< DAPyNodeGraphicsItem* > getSelectedPyNodeItems() const;

    // 连接管理
    DAPyLinkGraphicsItem* addPyNodeLink(
        DAPyNodeGraphicsItem* fromItem, const QString& fromOutput, DAPyNodeGraphicsItem* toItem, const QString& toInput
    );
    void addPyNodeLink(DAPyLinkGraphicsItem* linkItem);
    DAPyLinkGraphicsItem* addPyNodeLink_(
        DAPyNodeGraphicsItem* fromItem, const QString& fromOutput, DAPyNodeGraphicsItem* toItem, const QString& toInput
    );
    void addPyNodeLink_(DAPyLinkGraphicsItem* linkItem);
    bool removePyNodeLink(DAPyLinkGraphicsItem* linkItem, bool autoDelete = true);
    void removePyNodeLink_(DAPyLinkGraphicsItem* linkItem);
    QList< DAPyLinkGraphicsItem* > getPyNodeLinkItems() const;
    QList< DAPyLinkGraphicsItem* > getSelectedPyNodeLinkItems() const;

    // 获取节点的所有连接线（通过映射表直接查询，O(1)查找）
    QList< DAPyLinkGraphicsItem* > getNodeLinkItems(DAPyNodeGraphicsItem* nodeItem) const;
    // 获取节点的输入连接线（toNode为该节点的连接线）
    QList< DAPyLinkGraphicsItem* > getNodeInputLinkItems(DAPyNodeGraphicsItem* nodeItem) const;
    // 获取节点的输出连接线（fromNode为该节点的连接线）
    QList< DAPyLinkGraphicsItem* > getNodeOutputLinkItems(DAPyNodeGraphicsItem* nodeItem) const;

    // 获取节点沿输出方向的链路（BFS遍历所有下游可达节点）
    QList< DAPyNodeGraphicsItem* > getOutputLinkChain(DAPyNodeGraphicsItem* startNode) const;
    // 获取节点沿输入方向的链路（BFS遍历所有上游可达节点）
    QList< DAPyNodeGraphicsItem* > getInputLinkChain(DAPyNodeGraphicsItem* startNode) const;

    // 更新节点对应的连接线端点位置（节点移动后调用）
    void updateNodeLinkPositions(DAPyNodeGraphicsItem* nodeItem);

    // 删除选中项（支持undo/redo）
    int removeSelectedItems_();

    // 清空场景
    void clearPyScene();

    // 场景序列化（通过DAPyWorkFlowSceneSerializer实现）
    bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const;
    bool loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver);

    // 保存场景到文件
    bool saveToFile(const QString& filePath, const QVersionNumber& ver = QVersionNumber(1, 0, 0));

    // 从文件加载场景
    bool loadFromFile(const QString& filePath, const QVersionNumber& ver = QVersionNumber(1, 0, 0));

    // 取消链接模式
    virtual void cancelLink() override;
    // 同步python端的链接添加
    void syncPyNodeLinkAdd(DAPyLinkGraphicsItem* linkItem);
    // 同步python端的链接移除
    void syncPyNodeLinkRemove(DAPyLinkGraphicsItem* linkItem);
Q_SIGNALS:
    // Python节点item被创建
    void pyNodeItemCreated(DA::DAPyNodeGraphicsItem* item);

    // Python节点item被移除
    void pyNodeItemsRemoved(const QList< DA::DAPyNodeGraphicsItem* >& items);

    // Python连接线被创建
    void pyNodeLinkCreated(DA::DAPyLinkGraphicsItem* link);

    // Python连接线被移除
    void pyNodeLinksRemoved(const QList< DAPyLinkGraphicsItem* >& links);

    // Python节点选中变更
    void selectPyNodeItemChanged(DA::DAPyNodeGraphicsItem* item);

    // Python连接线选中变更
    void selectPyNodeLinkChanged(DA::DAPyLinkGraphicsItem* link);

    // Python节点状态变更（由DAPythonSignalHandler触发）
    void pyNodeStateChanged(DA::DAPyNodeGraphicsItem* item, DA::DAPyNodeState state);

protected Q_SLOTS:
    void onSelectItemChanged(DA::DAGraphicsItem* item);
    void onSelectLinkChanged(DA::DAGraphicsLinkItem* item);

    // 处理Python侧节点状态变更通知
    void onPyNodeStateNotification(const QString& nodeId, DA::DAPyNodeState state);

protected:
    // 鼠标事件（处理节点连接点交互）
    void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

    // 对scene中的item进行分类
    static void classifyItems(
        const QList< QGraphicsItem* >& sourceItems,
        QList< DAPyNodeGraphicsItem* >& nodeItems,
        QList< DAPyLinkGraphicsItem* >& linkItems,
        QList< QGraphicsItem* >& normalItems
    );

    // 获取节点item的所有连接线（基于映射表高效查询）
    QList< DAPyLinkGraphicsItem* > getNodesAllLinkItems(const QList< DAPyNodeGraphicsItem* >& nodeItems) const;

    // 创建连接线的工厂函数，注意hintFromItem和hintFromOutput是一个试探性的输入，传入空值也可以，这两个参数是为了适配不同的连接点伸出不同连接线做准备的
    virtual DAPyLinkGraphicsItem* createLinkItem(
        DAPyNodeGraphicsItem* hintFromItem = nullptr, const QString& hintFromOutput = QString()
    );

private:
    void initConnect();
    // 初始化Python DAWorkflow实例
    void initPyWorkflow();
    // 重建节点到连接线的映射表（undo/redo恢复后或从文件加载后调用）
    void rebuildNodeLinksMap();
};

}  // namespace DA

#endif  // DAPYWORKFLOWSCENE_H
