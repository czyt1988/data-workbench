#ifndef DANODEGRAPHICSSCENE_H
#define DANODEGRAPHICSSCENE_H
#include <QtCore/qglobal.h>
#include "DANodeMetaData.h"
#include "DAWorkFlowGlobal.h"
#include "DAGraphicsScene.h"
#include "DAAbstractNodeGraphicsItem.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include "DAWorkFlow.h"

class QGraphicsSceneMouseEvent;
namespace DA
{
class DAGraphicsPixmapItem;
class DAGraphicsRectItem;
class DAGraphicsStandardTextItem;
class DAGraphicsTextItem;

/**
 * @brief DAAbstractNodeGraphicsItem对应的QGraphicsScene,通过此scene，管理DAWorkFlow内容
 *
 * @note 所有支持redo/undo的函数后面都会带有_下标
 */
class DAWORKFLOW_API DANodeGraphicsScene : public DAGraphicsScene
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DANodeGraphicsScene)
	friend class DAAbstractNodeGraphicsItem;
	friend class DAAbstractNodeLinkGraphicsItem;

public:
	DANodeGraphicsScene(QObject* p = nullptr);
	DANodeGraphicsScene(const QRectF& sceneRect, QObject* p = nullptr);
	DANodeGraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject* p = nullptr);
	~DANodeGraphicsScene();

	// 取消链接
	virtual void cancelLink() override;

	// 设置工作流
	void setWorkFlow(DAWorkFlow* wf);
	DAWorkFlow* getWorkflow();

	// 通过node找到item,时间复杂度<O(n)
	DAAbstractNodeGraphicsItem* findItemByNode(DAAbstractNode* n);

	// 获取选中的NodeGraphicsItem,如果没有返回nullptr，如果选中多个，返回第一个
	DAAbstractNodeGraphicsItem* getSelectedNodeGraphicsItem() const;

	// 获取所有的graphicsitem
	QList< DAAbstractNodeGraphicsItem* > getNodeGraphicsItems() const;

	// 获取所有的DAAbstractNodeGraphicsItem
	QList< DAGraphicsStandardTextItem* > getTextGraphicsItems() const;

	// 获取选中的NodeGraphicsItem,如果没有返回一个空list
	QList< DAAbstractNodeGraphicsItem* > getSelectedNodeGraphicsItems() const;

	// 获取选中的NodeLinkGraphicsItem,如果没有返回nullptr，如果选中多个，返回第一个
	DAAbstractNodeLinkGraphicsItem* getSelectedNodeLinkGraphicsItem() const;

	// 获取选中的NodeLinkGraphicsItem,如果没有返回一个空list
	QList< DAAbstractNodeLinkGraphicsItem* > getSelectedNodeLinkGraphicsItems() const;

	// 获取除了连接线以外的item
	QList< QGraphicsItem* > getGraphicsItemsWithoutLink() const;

	// 删除选中的item，此函数支持redo/undo,返回删除的数量
	int removeSelectedItems_();

	// 通过node元对象创建工作流节点
	DAAbstractNodeGraphicsItem* createNode(const DANodeMetaData& md, const QPointF& pos);
	DAAbstractNodeGraphicsItem* createNode_(const DANodeMetaData& md, const QPointF& pos);
	// 创建文本框
	DAGraphicsTextItem* createText_(const QString& str = QString());
	// 创建矩形
	DAGraphicsRectItem* createRect_(const QPointF& p = QPointF());
	// 通过位置获取DAAbstractNodeGraphicsItem，此函数是加强版的itemAt
	DAAbstractNodeGraphicsItem* nodeItemAt(const QPointF& scenePos) const;
signals:

	/**
	 * @brief 节点的连接点被选中触发的信号
	 * @param item 节点item
	 * @param lp 连接点
	 */
	void nodeItemLinkPointSelected(DA::DAAbstractNodeGraphicsItem* item,
								   const DA::DANodeLinkPoint& lp,
								   QGraphicsSceneMouseEvent* event);

	/**
	 * @brief 说明link已经为空，这时会自动remove
	 */
	void nodeLinkItemIsEmpty(DA::DAAbstractNodeLinkGraphicsItem* link);

	/**
	 * @brief 选中了某个节点的设置窗口
	 * @param w
	 */
	void selectNodeItemChanged(DA::DAAbstractNodeGraphicsItem* item);

	/**
	 * @brief 选中了某个节点的设置窗口
	 * @param w
	 */
	void selectNodeLinkChanged(DA::DAAbstractNodeLinkGraphicsItem* link);

	/**
	 * @brief 节点被移除
	 * @note 如果一次性删除了@sa DAAbstractNodeGraphicsItem 、@sa DAAbstractNodeLinkGraphicsItem、QGraphicsItem，那么信号的触发顺序是：
	 * - @sa nodeItemsRemoved
	 * - @sa nodeLinksRemoved
	 * - @sa itemRemoved
	 */
	void nodeItemsRemoved(const QList< DA::DAAbstractNodeGraphicsItem* >& items);

	/**
	 * @brief 连接线被移除
	 * @note 如果一次性删除了@sa DAAbstractNodeGraphicsItem 、@sa DAAbstractNodeLinkGraphicsItem、QGraphicsItem，那么信号的触发顺序是：
	 * - @sa nodeItemsRemoved
	 * - @sa nodeLinksRemoved
	 * - @sa itemRemoved
	 */
	void nodeLinksRemoved(const QList< DA::DAAbstractNodeLinkGraphicsItem* >& items);

	/**
	 * @brief QGraphicsItem的移除，注意，QGraphicsItem是不包含DAAbstractNodeGraphicsItem和DAAbstractNodeLinkGraphicsItem
	 *
	 * @sa DAAbstractNodeGraphicsItem 和@sa DAAbstractNodeLinkGraphicsItem 的移除触发@sa nodeItemsRemoved 和 @sa
	 * nodeLinksRemoved， 非@sa DAAbstractNodeGraphicsItem 和@sa DAAbstractNodeLinkGraphicsItem 才会触发此信号
	 * @param items 被移除的item
	 * @note 如果一次性删除了@sa DAAbstractNodeGraphicsItem 、@sa DAAbstractNodeLinkGraphicsItem、QGraphicsItem，那么信号的触发顺序是：
	 * - @sa nodeItemsRemoved
	 * - @sa nodeLinksRemoved
	 * - @sa itemRemoved
	 */
	void itemRemoved(const QList< QGraphicsItem* >& items);

protected slots:
	void onSelectItemChanged(DAGraphicsItem* item);
	void onSelectLinkChanged(DAGraphicsLinkItem* item);
	// item选择改变
	//    void onItemSelectionChanged();

	// node的名字改变
	void onNodeNameChanged(DAAbstractNode::SharedPointer node, const QString& oldName, const QString& newName);

protected:
	// 鼠标点击事件
	void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

	// 鼠标移动事件
	void mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

	// 鼠标释放
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

	// itemlink都没用节点连接时会调用这个函数，发出
	void callNodeItemLinkIsEmpty(DAAbstractNodeLinkGraphicsItem* link);

private:
	void initConnect();
};
}
#endif  // FCNODEGRAPHICSSCENE_H
