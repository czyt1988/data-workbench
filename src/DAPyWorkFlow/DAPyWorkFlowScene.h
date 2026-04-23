#ifndef DAPYWORKFLOWSCENE_H
#define DAPYWORKFLOWSCENE_H
#include "DAPyWorkFlowAPI.h"
#include "DAGraphicsScene.h"
#include "DAPyNodeProxy.h"
#include "DAPyNodeState.h"
#include <QPointF>
#include <QJsonObject>
class QDomDocument;
class QDomElement;

namespace DA
{
class DAPyNodeGraphicsItem;
class DAPyLinkGraphicsItem;
class DAPythonSignalHandler;

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

    // 节点管理
    DAPyNodeGraphicsItem* createPyNode(const QJsonObject& descriptor, const QPointF& pos);
    DAPyNodeGraphicsItem* createPyNode_(const QJsonObject& descriptor, const QPointF& pos);
    bool removePyNodeItem(DAPyNodeGraphicsItem* item);
    void removePyNodeItem_(DAPyNodeGraphicsItem* item);

    // 查找节点
    DAPyNodeGraphicsItem* findNodeItemByProxy(DAPyNodeProxy* proxy) const;
    DAPyNodeGraphicsItem* findNodeItemById(const QString& nodeId) const;
    DAPyNodeGraphicsItem* nodeItemAt(const QPointF& scenePos) const;
    QList<DAPyNodeGraphicsItem*> getPyNodeItems() const;
    QList<DAPyNodeGraphicsItem*> getSelectedPyNodeItems() const;

    // 连接管理
    DAPyLinkGraphicsItem* addPyNodeLink(DAPyNodeGraphicsItem* fromItem,
                                        const QString& fromOutput,
                                        DAPyNodeGraphicsItem* toItem,
                                        const QString& toInput);
    DAPyLinkGraphicsItem* addPyNodeLink_(DAPyNodeGraphicsItem* fromItem,
                                         const QString& fromOutput,
                                         DAPyNodeGraphicsItem* toItem,
                                         const QString& toInput);
    bool removePyNodeLink(DAPyLinkGraphicsItem* linkItem);
    void removePyNodeLink_(DAPyLinkGraphicsItem* linkItem);
    QList<DAPyLinkGraphicsItem*> getPyNodeLinkItems() const;
    QList<DAPyLinkGraphicsItem*> getSelectedPyNodeLinkItems() const;

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

Q_SIGNALS:
    // Python节点item被创建
    void pyNodeItemCreated(DAPyNodeGraphicsItem* item);

    // Python节点item被移除
    void pyNodeItemsRemoved(const QList<DAPyNodeGraphicsItem*>& items);

    // Python连接线被创建
    void pyNodeLinkCreated(DAPyLinkGraphicsItem* link);

    // Python连接线被移除
    void pyNodeLinksRemoved(const QList<DAPyLinkGraphicsItem*>& links);

    // Python节点选中变更
    void selectPyNodeItemChanged(DAPyNodeGraphicsItem* item);

    // Python连接线选中变更
    void selectPyNodeLinkChanged(DAPyLinkGraphicsItem* link);

    // Python节点状态变更（由DAPythonSignalHandler触发）
    void pyNodeStateChanged(DAPyNodeGraphicsItem* item, DAPyNodeState state);

protected Q_SLOTS:
    void onSelectItemChanged(DAGraphicsItem* item);
    void onSelectLinkChanged(DAGraphicsLinkItem* item);

    // 处理Python侧节点状态变更通知
    void onPyNodeStateNotification(const QString& nodeId, DAPyNodeState state);

protected:
    // 鼠标事件（处理节点连接点交互）
    void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

    // 对scene中的item进行分类
    static void classifyItems(const QList<QGraphicsItem*>& sourceItems,
                              QList<DAPyNodeGraphicsItem*>& nodeItems,
                              QList<DAPyLinkGraphicsItem*>& linkItems,
                              QList<QGraphicsItem*>& normalItems);

    // 获取节点item的所有连接线
    static QList<DAPyLinkGraphicsItem*> getNodesAllLinkItems(const QList<DAPyNodeGraphicsItem*>& nodeItems);

private:
    void initConnect();
};

}  // namespace DA

#endif  // DAPYWORKFLOWSCENE_H