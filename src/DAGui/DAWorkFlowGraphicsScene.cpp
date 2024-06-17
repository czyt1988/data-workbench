/*****************************************************************/ /**
  @file   DAWorkFlowGraphicsScene.cpp
  @brief

  @author czy
  @date   December 2023
  *********************************************************************/
#include "DAWorkFlowGraphicsScene.h"
// Qt
#include <QApplication>
#include <QKeyEvent>
#include <QDebug>
#include <QGraphicsSceneDragDropEvent>
#include "DAGraphicsStandardTextItem.h"
// workflow
#include "DAAbstractNode.h"
#include "DANodeMimeData.h"
#include "DANodeMetaData.h"
#include "DAGraphicsPixmapItem.h"
#include "DAGraphicsRectItem.h"
#include "Commands/DACommandsForWorkFlow.h"
#include "DACommandsForGraphics.h"
#include "DAGraphicsTextItem.h"
//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
// DAWorkFlowGraphicsScene
//===================================================
DAWorkFlowGraphicsScene::DAWorkFlowGraphicsScene(QObject* parent)
    : DANodeGraphicsScene(parent)
    , mBackgroundPixmapItem(nullptr)
    , mMouseAction(NoMouseAction)
    , mIsMouseActionContinuoue(false)
    , mEnableItemMoveWithBackground(false)
{
	mTextFont = QApplication::font();
	connect(this, &DAGraphicsScene::itemsPositionChanged, this, &DAWorkFlowGraphicsScene::onItemsPositionChanged);
}

DAWorkFlowGraphicsScene::~DAWorkFlowGraphicsScene()
{
    qDebug() << "destory DAWorkFlowGraphicsScene";
}

/**
 * @brief 添加一个背景图,如果多次调用，此函数返回的QGraphicsPixmapItem* 是一样的，也就是只会创建一个QGraphicsPixmapItem*
 * @param pixmap
 * @return
 */
DAGraphicsPixmapItem* DAWorkFlowGraphicsScene::setBackgroundPixmap(const QPixmap& pixmap)
{
	DACommandWorkFlowSceneAddBackgroundPixmap* cmd = new DACommandWorkFlowSceneAddBackgroundPixmap(this, pixmap);
	undoStack().push(cmd);
	return mBackgroundPixmapItem;
}
/**
 * @brief 获取背景图item
 * @return 如果没有设置返回一个nullptr
 */
DAGraphicsPixmapItem* DAWorkFlowGraphicsScene::getBackgroundPixmapItem() const
{
	return mBackgroundPixmapItem;
	//    if (nullptr == _backgroundPixmapItem) {
	//        return nullptr;
	//    }
	//    QList< QGraphicsItem* > its = items();
	//    for (const QGraphicsItem* i : qAsConst(its)) {
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
DAGraphicsPixmapItem* DAWorkFlowGraphicsScene::createBackgroundPixmapItem()
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
void DAWorkFlowGraphicsScene::setMouseAction(DAWorkFlowGraphicsScene::MouseActionFlag mf, bool continuous)
{
	mMouseAction             = mf;
	mIsMouseActionContinuoue = continuous;
}
/**
 * @brief 获取当前的鼠标动作标记
 * @return
 */
DAWorkFlowGraphicsScene::MouseActionFlag DAWorkFlowGraphicsScene::getMouseAction() const
{
    return mMouseAction;
}
/**
 * @brief 鼠标动作是否连续执行
 * @return
 */
bool DAWorkFlowGraphicsScene::isMouseActionContinuoue() const
{
    return mIsMouseActionContinuoue;
}

/**
 * @brief 设置背景item，如果外部调用getBackgroundPixmapItem并删除，需要通过此函数把保存的item设置为null
 * @note 如果之前有设置item，会对之前的背景进行移除，但不会删除，这样如果用户不remove就是set可能会导致内存泄漏
 * @param item 设置nullptr相当于移除背景
 */
void DAWorkFlowGraphicsScene::setBackgroundPixmapItem(DAGraphicsPixmapItem* item)
{
	removeBackgroundPixmapItem();
	mBackgroundPixmapItem = item;
	if (item) {
#if DA_USE_QGRAPHICSOBJECT
		connect(mBackgroundPixmapItem,
                &DAGraphicsPixmapItem::xChanged,
                this,
                &DAWorkFlowGraphicsScene::backgroundPixmapItemXChanged);
		connect(mBackgroundPixmapItem,
                &DAGraphicsPixmapItem::yChanged,
                this,
                &DAWorkFlowGraphicsScene::backgroundPixmapItemYChanged);
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
DAGraphicsPixmapItem* DAWorkFlowGraphicsScene::removeBackgroundPixmapItem()
{
	if (mBackgroundPixmapItem) {
#if DA_USE_QGRAPHICSOBJECT
		disconnect(mBackgroundPixmapItem,
                   &DAGraphicsPixmapItem::xChanged,
                   this,
                   &DAWorkFlowGraphicsScene::backgroundPixmapItemXChanged);
		disconnect(mBackgroundPixmapItem,
                   &DAGraphicsPixmapItem::yChanged,
                   this,
                   &DAWorkFlowGraphicsScene::backgroundPixmapItemYChanged);
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
void DAWorkFlowGraphicsScene::enableItemMoveWithBackground(bool on)
{
    mEnableItemMoveWithBackground = on;
}

/**
  @brief 允许移动图元时，其它和此图元链接起来的图元跟随移动
  @return
 */
void DAWorkFlowGraphicsScene::setEnableItemLinkageMove(bool on)
{
    mEnableItemLinkageMove = on;
}

bool DAWorkFlowGraphicsScene::isEnableItemLinkageMove() const
{
    return mEnableItemLinkageMove;
}

/**
 * @brief DAWorkFlowGraphicsScene::isEnableItemMoveWithBackground
 * @return
 */
bool DAWorkFlowGraphicsScene::isEnableItemMoveWithBackground() const
{
    return mEnableItemMoveWithBackground;
}

DAGraphicsPixmapItem* DAWorkFlowGraphicsScene::ensureGetBackgroundPixmapItem()
{
	if (nullptr == mBackgroundPixmapItem) {
		createBackgroundPixmapItem();
	}
	return mBackgroundPixmapItem;
}
#if 0
void DAWorkFlowGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
	if (event->mimeData()->hasFormat(DANodeMimeData::formatString())) {
		// 说明有节点的meta数据拖入
		event->acceptProposedAction();
	} else {
		event->ignore();
	}
}

void DAWorkFlowGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
	event->acceptProposedAction();
}

void DAWorkFlowGraphicsScene::dragLeaveEvent(QGraphicsSceneDragDropEvent* event)
{
	event->acceptProposedAction();
}

void DAWorkFlowGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{
	if (event->mimeData()->hasFormat(DANodeMimeData::formatString())) {
		// 说明有节点的meta数据拖入
		const DANodeMimeData* nodemime = qobject_cast< const DANodeMimeData* >(event->mimeData());
		if (nullptr == nodemime) {
			return;
		}
		clearSelection();
		DANodeMetaData nodemeta = nodemime->getNodeMetaData();
		createNode_(nodemeta, event->scenePos());
	}
}
#endif
void DAWorkFlowGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (mMouseAction == NoMouseAction) {
		// 说明没有鼠标动作要执行，正常传递鼠标事件
		DANodeGraphicsScene::mousePressEvent(mouseEvent);
		return;
	}
	// 说明有鼠标事件要执行
	QPointF pos = mouseEvent->scenePos();
	switch (mMouseAction) {
	case StartAddText: {
		auto item = createText_();
		item->setScenePos(pos);
		item->setSelectTextFont(mTextFont);
		item->setSelectTextColor(mTextColor);
		item->setEditable(true);
		item->setFocus();
		emit mouseActionFinished(mMouseAction);
	} break;
	case StartAddRect: {
		DAGraphicsRectItem* item = createRect_(pos);
		item->setSelected(true);
		item->setZValue(-10);
		emit mouseActionFinished(mMouseAction);
	}
	default:
		break;
	}
	if (!mIsMouseActionContinuoue) {
		mMouseAction = NoMouseAction;
	}
	mouseEvent->accept();  // 接受事件，通知下面的mousePressEvent函数事件已经接收，无需执行动作
}

void DAWorkFlowGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	DANodeGraphicsScene::mouseReleaseEvent(mouseEvent);
}

#if DA_USE_QGRAPHICSOBJECT
void DAWorkFlowGraphicsScene::backgroundPixmapItemXChanged()
{
	if (!isEnableItemMoveWithBackground()) {
		mBackgroundPixmapItemLastPos = mBackgroundPixmapItem->pos();
		return;
	}
	QPointF newPos = mBackgroundPixmapItem->pos();
	QPointF sub(newPos - mBackgroundPixmapItemLastPos);
	//
	QList< QGraphicsItem* > itemsWithoutLink = getGraphicsItemsWithoutLink();

	for (QGraphicsItem* item : qAsConst(itemsWithoutLink)) {
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

void DAWorkFlowGraphicsScene::backgroundPixmapItemYChanged()
{
	if (!isEnableItemMoveWithBackground()) {
		mBackgroundPixmapItemLastPos = mBackgroundPixmapItem->pos();
		return;
	}
	QPointF newPos = mBackgroundPixmapItem->pos();
	QPointF sub(newPos - mBackgroundPixmapItemLastPos);
	QList< QGraphicsItem* > itemsWithoutLink = getGraphicsItemsWithoutLink();

	for (QGraphicsItem* item : qAsConst(itemsWithoutLink)) {
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
void DAWorkFlowGraphicsScene::onItemsPositionChanged(const QList< QGraphicsItem* >& items,
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
	QSet< DAAbstractNodeGraphicsItem* > linkedItems;
	{
		QSet< DAAbstractNodeGraphicsItem* > willRemove;
		for (QGraphicsItem* i : qAsConst(items)) {
			DAAbstractNodeGraphicsItem* nitem = dynamic_cast< DAAbstractNodeGraphicsItem* >(i);
			if (!nitem) {
				continue;
			}
			willRemove.insert(nitem);
            QList< DAAbstractNodeGraphicsItem* > chain = nitem->getOutPutLinkChain();
			linkedItems += QSet(chain.begin(), chain.end());
		}
		// 获取到的链条要把items移除，因为已经移动过了
		linkedItems -= willRemove;
	}
	// 计算要移动的位置
	QList< QPointF > startPos;
	QList< QPointF > endPos;
	QList< QGraphicsItem* > willMoveItems;
	for (DAAbstractNodeGraphicsItem* i : qAsConst(linkedItems)) {
		willMoveItems.append(i);
		startPos.push_back(i->pos());
		endPos.push_back(i->pos() + offset);
	}
	qDebug() << "will move " << willMoveItems.size();

	// 进行同步移动
	DACommandsForGraphicsItemsMoved* cmd = new DACommandsForGraphicsItemsMoved(willMoveItems, startPos, endPos, false);
	getUndoStack()->push(cmd);
}

QColor DAWorkFlowGraphicsScene::getDefaultTextColor() const
{
	return mTextColor;
}

void DAWorkFlowGraphicsScene::setDefaultTextColor(const QColor& c)
{
	mTextColor = c;
}

QFont DAWorkFlowGraphicsScene::getDefaultTextFont() const
{
	return mTextFont;
}

void DAWorkFlowGraphicsScene::setDefaultTextFont(const QFont& f)
{
	mTextFont = f;
}
#endif
