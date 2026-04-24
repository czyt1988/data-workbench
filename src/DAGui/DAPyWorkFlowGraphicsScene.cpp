/*****************************************************************/ /**
  @file   DAPyWorkFlowGraphicsScene.cpp
  @brief

  @author czy
  @date   December 2023
  *********************************************************************/
#include "DAPyWorkFlowGraphicsScene.h"
// Qt
#include <QApplication>
#include <QKeyEvent>
#include <QDebug>
#include <QGraphicsSceneDragDropEvent>
#include <QJsonObject>
#include <QJsonArray>
#include "DAGraphicsStandardTextItem.h"
// workflow
#include "DAPyNodeProxy.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyLinkGraphicsItem.h"
#include "DAGraphicsCommandsFactory.h"
#include "DANodeMimeData.h"
#include "DAPyNodeFactory.h"
#include "DAGraphicsPixmapItem.h"
#include "DAGraphicsRectItem.h"
#include "Commands/DACommandsForWorkFlow.h"
#include "DACommandsForGraphics.h"
#include "DAGraphicsTextItem.h"
#include "DAGraphicsDrawRectSceneAction.h"
#include "DAGraphicsDrawTextItemSceneAction.h"
#include "DAPyNodeConfigDialog.h"
//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DAPyWorkFlowGraphicsScene
//===================================================
DAPyWorkFlowGraphicsScene::DAPyWorkFlowGraphicsScene(QObject* parent)
	: DAPyWorkFlowScene(parent), mBackgroundPixmapItem(nullptr), mEnableItemMoveWithBackground(false)
{
	mTextFont = QApplication::font();
	connect(this, &DAGraphicsScene::itemsPositionChanged, this, &DAPyWorkFlowGraphicsScene::onItemsPositionChanged);
	connect(this, &DAPyWorkFlowScene::pyNodeItemCreated, this, &DAPyWorkFlowGraphicsScene::onPyNodeItemCreated);
}

DAPyWorkFlowGraphicsScene::~DAPyWorkFlowGraphicsScene()
{
    qDebug() << "destroy DAPyWorkFlowGraphicsScene";
}

/**
 * @brief 添加一个背景图,如果多次调用，此函数返回的QGraphicsPixmapItem* 是一样的，也就是只会创建一个QGraphicsPixmapItem*
 * @param pixmap
 * @return
 */
DAGraphicsPixmapItem* DAPyWorkFlowGraphicsScene::setBackgroundPixmap(const QPixmap& pixmap)
{
	DACommandWorkFlowSceneAddBackgroundPixmap* cmd = new DACommandWorkFlowSceneAddBackgroundPixmap(this, pixmap);
	undoStack().push(cmd);
	return mBackgroundPixmapItem;
}
/**
 * @brief 获取背景图item
 * @return 如果没有设置返回一个nullptr
 */
DAGraphicsPixmapItem* DAPyWorkFlowGraphicsScene::getBackgroundPixmapItem() const
{
	return mBackgroundPixmapItem;
	//    if (nullptr == _backgroundPixmapItem) {
	//        return nullptr;
	//    }
	//    QList< QGraphicsItem* > its = items();
	//    for (const QGraphicsItem* i : std::as_const(its)) {
	//        if (i == _backgroundPixmapItem) {
	//            return _backgroundPixmapItem;
	//        }
	//    }
	//    //在所有item中没有找到，说明_backgroundPixmapItem已经被删除
	//    return nullptr;
}

/**
 * @brief 创建一个新的图片item
 *
 * @note 原来的并不会删除
 * @return
 */
DAGraphicsPixmapItem* DAPyWorkFlowGraphicsScene::createBackgroundPixmapItem()
{
	DAGraphicsPixmapItem* item = new DAGraphicsPixmapItem();
	setBackgroundPixmapItem(item);
	return item;
}

/**
 * @brief 设置鼠标动作
 *
 * 一旦设置鼠标动作，鼠标点击后就会触发此动作，continuous来标记动作结束后继续保持还是还原为无动作
 * @param mf 鼠标动作
 * @param continuous 是否连续执行
 */
void DAPyWorkFlowGraphicsScene::setPreDefineSceneAction(DAPyWorkFlowGraphicsScene::SceneActionFlag mf)
{
	switch (mf) {
	case AddRectItemAction:
		setupSceneAction(new DAGraphicsDrawRectSceneAction(this));
		break;
	case AddTextItemAction:
		setupSceneAction(new DAGraphicsDrawTextItemSceneAction(this));
		break;
	default:
		break;
	}
}

/**
 * @brief 设置背景item，如果外部调用getBackgroundPixmapItem并删除，需要通过此函数把保存的item设置为null
 * @note 如果之前有设置item，会对之前的背景进行移除，但不会删除，这样如果用户不remove就是set可能会导致内存泄漏
 * @param item 设置nullptr相当于移除背景
 */
void DAPyWorkFlowGraphicsScene::setBackgroundPixmapItem(DAGraphicsPixmapItem* item)
{
	removeBackgroundPixmapItem();
	mBackgroundPixmapItem = item;
	if (item) {
#if DA_USE_QGRAPHICSOBJECT
		connect(mBackgroundPixmapItem,
				&DAGraphicsPixmapItem::xChanged,
				this,
				&DAPyWorkFlowGraphicsScene::backgroundPixmapItemXChanged);
		connect(mBackgroundPixmapItem,
				&DAGraphicsPixmapItem::yChanged,
				this,
				&DAPyWorkFlowGraphicsScene::backgroundPixmapItemYChanged);
#endif
		item->setZValue(-9999);
		addItem(mBackgroundPixmapItem);
#if DA_USE_QGRAPHICSOBJECT
		mBackgroundPixmapItemLastPos = mBackgroundPixmapItem->pos();
#endif
	}
}

/**
 * @brief 移出当前的背景item
 * @note 不会对item进行删除操作
 * @note 如果没有设置背景，此函数返回nullptr
 * @return 返回pixmapitem
 */
DAGraphicsPixmapItem* DAPyWorkFlowGraphicsScene::removeBackgroundPixmapItem()
{
	if (mBackgroundPixmapItem) {
#if DA_USE_QGRAPHICSOBJECT
		disconnect(mBackgroundPixmapItem,
				   &DAGraphicsPixmapItem::xChanged,
				   this,
				   &DAPyWorkFlowGraphicsScene::backgroundPixmapItemXChanged);
		disconnect(mBackgroundPixmapItem,
				   &DAGraphicsPixmapItem::yChanged,
				   this,
				   &DAPyWorkFlowGraphicsScene::backgroundPixmapItemYChanged);
#endif
		removeItem(mBackgroundPixmapItem);
	}
	DAGraphicsPixmapItem* oldItem = mBackgroundPixmapItem;
	mBackgroundPixmapItem         = nullptr;
	return oldItem;
}

/**
 * @brief 允许item跟随背景图移动
 * @param on
 */
void DAPyWorkFlowGraphicsScene::enableItemMoveWithBackground(bool on)
{
    mEnableItemMoveWithBackground = on;
}

/**
  @brief 允许移动图元时，其它和此图元链接起来的图元跟随移动
  @return
 */
void DAPyWorkFlowGraphicsScene::setEnableItemLinkageMove(bool on)
{
    mEnableItemLinkageMove = on;
}

bool DAPyWorkFlowGraphicsScene::isEnableItemLinkageMove() const
{
    return mEnableItemLinkageMove;
}

/**
 * @brief DAPyWorkFlowGraphicsScene::isEnableItemMoveWithBackground
 * @return
 */
bool DAPyWorkFlowGraphicsScene::isEnableItemMoveWithBackground() const
{
    return mEnableItemMoveWithBackground;
}

DAGraphicsPixmapItem* DAPyWorkFlowGraphicsScene::ensureGetBackgroundPixmapItem()
{
	if (nullptr == mBackgroundPixmapItem) {
		createBackgroundPixmapItem();
	}
	return mBackgroundPixmapItem;
}
#if 0
void DAPyWorkFlowGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
	if (event->mimeData()->hasFormat(DANodeMimeData::formatString())) {
		// 说明有节点的meta数据拖入
		event->acceptProposedAction();
	} else {
		event->ignore();
	}
}

void DAPyWorkFlowGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
	event->acceptProposedAction();
}

void DAPyWorkFlowGraphicsScene::dragLeaveEvent(QGraphicsSceneDragDropEvent* event)
{
	event->acceptProposedAction();
}

void DAPyWorkFlowGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{
	if (event->mimeData()->hasFormat(DANodeMimeData::formatString())) {
		// 说明有节点的meta数据拖入
		const DANodeMimeData* nodemime = qobject_cast< const DANodeMimeData* >(event->mimeData());
		if (nullptr == nodemime) {
			return;
		}
		clearSelection();
		DAPyNodeMetaData nodemeta = nodemime->getNodeMetaData();
		createNode_(nodemeta, event->scenePos());
	}
}
#endif

#if DA_USE_QGRAPHICSOBJECT
void DAPyWorkFlowGraphicsScene::backgroundPixmapItemXChanged()
{
	if (!isEnableItemMoveWithBackground()) {
		mBackgroundPixmapItemLastPos = mBackgroundPixmapItem->pos();
		return;
	}
	QPointF newPos = mBackgroundPixmapItem->pos();
	QPointF sub(newPos - mBackgroundPixmapItemLastPos);
	//
	// TODO: reimplement getGraphicsItemsWithoutLink() as a DAPyWorkFlowScene method
	// The old DANodeGraphicsScene::getGraphicsItemsWithoutLink() returned all items
	// excluding link items. Here we filter inline.
	QList< QGraphicsItem* > allItems = items();
	QList< QGraphicsItem* > itemsWithoutLink;
	for (QGraphicsItem* item : std::as_const(allItems)) {
		if (dynamic_cast< DAGraphicsLinkItem* >(item) == nullptr) {
			itemsWithoutLink.append(item);
		}
	}

	for (QGraphicsItem* item : std::as_const(itemsWithoutLink)) {
		if (item == getBackgroundPixmapItem()) {
			// PixmapItem 也会获取到，避免递归
			continue;
		}
		if (nullptr != item->parentItem()) {
			continue;
		}
		QPointF tempPos = sub + item->pos();
		// 这里是用setPos不是setScenePos
		item->setPos(tempPos);
	}

	mBackgroundPixmapItemLastPos = newPos;
}

void DAPyWorkFlowGraphicsScene::backgroundPixmapItemYChanged()
{
	if (!isEnableItemMoveWithBackground()) {
		mBackgroundPixmapItemLastPos = mBackgroundPixmapItem->pos();
		return;
	}
	QPointF newPos = mBackgroundPixmapItem->pos();
	QPointF sub(newPos - mBackgroundPixmapItemLastPos);
	// TODO: reimplement getGraphicsItemsWithoutLink() as a DAPyWorkFlowScene method
	QList< QGraphicsItem* > allItems = items();
	QList< QGraphicsItem* > itemsWithoutLink;
	for (QGraphicsItem* item : std::as_const(allItems)) {
		if (dynamic_cast< DAGraphicsLinkItem* >(item) == nullptr) {
			itemsWithoutLink.append(item);
		}
	}

	for (QGraphicsItem* item : std::as_const(itemsWithoutLink)) {
		if (item == getBackgroundPixmapItem()) {
			// PixmapItem 也会获取到，避免递归
			continue;
		}
		if (nullptr != item->parentItem()) {
			continue;
		}
		QPointF tempPos = sub + item->pos();
		// 这里是用setPos不是setScenePos
		item->setPos(tempPos);
	}
	mBackgroundPixmapItemLastPos = newPos;
}

/**
  @brief itemsPositionChanged 是鼠标移动才会触发，因此，在此函数里面调用move函数是不会再次触发这个槽

  @param items
  @param oldPos
  @param newPos
  @return $RETURN
*/
void DAPyWorkFlowGraphicsScene::onItemsPositionChanged(const QList< QGraphicsItem* >& items,
                                                     const QList< QPointF >& oldPos,
                                                     const QList< QPointF >& newPos)
{
	qDebug() << "onItemsPositionChanged";
	if (items.empty() || oldPos.empty() || newPos.empty()) {
		return;
	}
	if (!isEnableItemLinkageMove()) {
		return;
	}
	// 要关联移动
	// 首先计算出移动的偏移量
	QPointF offset = newPos.first() - oldPos.first();
	// 查找所有关联的items
	// TODO: DAPyNodeGraphicsItem doesn't have getOutPutLinkChain() yet
	// For now, skip linked item discovery — linkage move will be reimplemented
	QSet< DAPyNodeGraphicsItem* > linkedItems;
	{
		QSet< DAPyNodeGraphicsItem* > willRemove;
		for (QGraphicsItem* i : std::as_const(items)) {
			DAPyNodeGraphicsItem* nitem = dynamic_cast< DAPyNodeGraphicsItem* >(i);
			if (!nitem) {
				continue;
			}
			willRemove.insert(nitem);
			// TODO: reimplement linkage chain lookup for DAPyNodeGraphicsItem
			// QList< DAPyNodeGraphicsItem* > chain = nitem->getOutPutLinkChain();
			// linkedItems += QSet(chain.begin(), chain.end());
		}
		// 获取到的链条要把items移除，因为已经移动过了
		linkedItems -= willRemove;
	}
	// 计算要移动的位置
	QList< QPointF > startPos;
	QList< QPointF > endPos;
	QList< QGraphicsItem* > willMoveItems;
	for (DAPyNodeGraphicsItem* i : std::as_const(linkedItems)) {
		willMoveItems.append(i);
		startPos.push_back(i->pos());
		endPos.push_back(i->pos() + offset);
	}
	qDebug() << "will move " << willMoveItems.size();

	// 进行同步移动,这里是处理关联移动的item
	DACommandsForGraphicsItemsMoved* cmd = commandsFactory()->createItemsMoved(willMoveItems, startPos, endPos, false);
	getUndoStack()->push(cmd);
}

QColor DAPyWorkFlowGraphicsScene::getDefaultTextColor() const
{
	return mTextColor;
}

void DAPyWorkFlowGraphicsScene::setDefaultTextColor(const QColor& c)
{
	mTextColor = c;
}

QFont DAPyWorkFlowGraphicsScene::getDefaultTextFont() const
{
	return mTextFont;
}

void DAPyWorkFlowGraphicsScene::setDefaultTextFont(const QFont& f)
{
	mTextFont = f;
}
#endif

/**
 * @brief 节点双击处理槽，弹出参数配置对话框
 *
 * 当DAPyNodeGraphicsItem发射nodeDoubleClicked信号时，
 * 此槽被触发，创建DAPyNodeConfigDialog并显示给用户。
 * 仅当节点有参数时才弹出对话框。
 *
 * @param[in] proxy 双击的节点代理
 */
void DAPyWorkFlowGraphicsScene::onNodeDoubleClicked(DAPyNodeProxy* proxy)
{
	if (!proxy) {
		return;
	}
	// 查找对应的节点图形项，获取描述符检查是否有参数
	DAPyNodeGraphicsItem* item = findNodeItemByProxy(proxy);
	if (item) {
		QJsonObject descriptor = item->getDescriptor();
		QJsonArray parameters  = descriptor.value("parameters").toArray();
		if (parameters.isEmpty()) {
			// 无参数，不显示对话框
			return;
		}
	}
	// 创建并显示参数配置对话框
	DAPyNodeConfigDialog dialog(proxy);
	dialog.exec();
}

/**
 * @brief 节点创建时连接其双击信号
 *
 * 当DAPyWorkFlowScene发射pyNodeItemCreated信号时，
 * 此槽被触发，将新创建的DAPyNodeGraphicsItem的nodeDoubleClicked信号
 * 连接到DAPyWorkFlowGraphicsScene的onNodeDoubleClicked槽。
 *
 * @param[in] item 新创建的节点图形项
 */
void DAPyWorkFlowGraphicsScene::onPyNodeItemCreated(DAPyNodeGraphicsItem* item)
{
	if (!item) {
		return;
	}
	connect(item, &DAPyNodeGraphicsItem::nodeDoubleClicked, this, &DAPyWorkFlowGraphicsScene::onNodeDoubleClicked);
}
