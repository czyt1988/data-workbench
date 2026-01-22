#include "DACommandsForGraphics.h"
#include <QDebug>
#include <QGraphicsItem>
#include "DAGraphicsScene.h"
#include "DAGraphicsResizeableItem.h"
#include "DAGraphicsItem.h"
#include "DAGraphicsItemGroup.h"
#include "DAQtContainerUtil.hpp"
#include <QObject>
#include <QTextDocument>

#ifndef DACOMMANDSFORGRAPHICS_DEBUG_PRINT
#define DACOMMANDSFORGRAPHICS_DEBUG_PRINT 1
#endif

namespace DA
{
//----------------------------------------------------
// DACommandsForGraphicsItemAdd
//----------------------------------------------------
DACommandsForGraphicsItemAdd::DACommandsForGraphicsItemAdd(QGraphicsItem* item, QGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mScene(scene), mNeedDelete(false)
{
    setText(QObject::tr("Item Add"));
}

DACommandsForGraphicsItemAdd::~DACommandsForGraphicsItemAdd()
{
	if (mNeedDelete) {
		delete mItem;
	}
}

void DACommandsForGraphicsItemAdd::redo()
{
	QUndoCommand::redo();
	if (mItem->scene() != mScene) {
		mScene->addItem(mItem);
	}
	mNeedDelete = false;
}

void DACommandsForGraphicsItemAdd::undo()
{
	QUndoCommand::undo();
	mScene->removeItem(mItem);
	mNeedDelete = true;
}
//----------------------------------------------------
// DACommandsForGraphicsItemsAdd
//----------------------------------------------------

DACommandsForGraphicsItemsAdd::DACommandsForGraphicsItemsAdd(const QList< QGraphicsItem* > its,
                                                             QGraphicsScene* scene,
                                                             QUndoCommand* parent)
    : QUndoCommand(parent), mItems(its), mScene(scene), mNeedDelete(false)
{
    setText(QObject::tr("Items Add"));
}

DACommandsForGraphicsItemsAdd::~DACommandsForGraphicsItemsAdd()
{
	if (mNeedDelete) {
		for (QGraphicsItem* i : std::as_const(mItems)) {
			delete i;
		}
	}
}

void DACommandsForGraphicsItemsAdd::redo()
{
	QUndoCommand::redo();
	for (QGraphicsItem* item : std::as_const(mItems)) {
		if (item->scene() != mScene) {
			mScene->addItem(item);
		}
	}
	mNeedDelete = false;
}

void DACommandsForGraphicsItemsAdd::undo()
{
	QUndoCommand::undo();
	for (QGraphicsItem* item : std::as_const(mItems)) {
		mScene->removeItem(item);
	}

	mNeedDelete = true;
}

//----------------------------------------------------
//  DACommandsForGraphicsItemRemove
//----------------------------------------------------

DACommandsForGraphicsItemRemove::DACommandsForGraphicsItemRemove(QGraphicsItem* item, QGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mScene(scene), mNeedDelete(false)
{
    setText(QObject::tr("Item Remove"));
}

DACommandsForGraphicsItemRemove::~DACommandsForGraphicsItemRemove()
{
	if (mNeedDelete) {
		delete mItem;
	}
}

void DACommandsForGraphicsItemRemove::redo()
{
	QUndoCommand::redo();
	mScene->removeItem(mItem);
	mNeedDelete = true;
}

void DACommandsForGraphicsItemRemove::undo()
{
	QUndoCommand::undo();
	mScene->addItem(mItem);
	mNeedDelete = false;
}

//----------------------------------------------------
//  DACommandsForGraphicsItemsRemove
//----------------------------------------------------
DACommandsForGraphicsItemsRemove::DACommandsForGraphicsItemsRemove(const QList< QGraphicsItem* > its,
                                                                   QGraphicsScene* scene,
                                                                   QUndoCommand* parent)
    : QUndoCommand(parent), mItems(its), mScene(scene), mNeedDelete(false)
{
    setText(QObject::tr("Items Remove"));
}

DACommandsForGraphicsItemsRemove::~DACommandsForGraphicsItemsRemove()
{
	if (mNeedDelete) {
		for (QGraphicsItem* i : std::as_const(mItems)) {
			delete i;
		}
	}
}

void DACommandsForGraphicsItemsRemove::redo()
{
	QUndoCommand::redo();
	for (QGraphicsItem* item : std::as_const(mItems)) {
		if (item->scene() != mScene) {
			mScene->removeItem(item);
		}
	}
	mNeedDelete = true;
}

void DACommandsForGraphicsItemsRemove::undo()
{
	QUndoCommand::undo();
	for (QGraphicsItem* item : std::as_const(mItems)) {
		mScene->addItem(item);
	}

	mNeedDelete = false;
}
/**
 * @brief DACommandsForGraphicsItemMoved::DACommandsForGraphicsItemMoved
 * @param items 需要移动的items
 * @param starts 开始位置
 * @param ends 最终位置
 * @param skipfirst 是否跳过第一次执行，对于鼠标操作，移动已经执行，一般会让其跳过第一次执行，但对于通过窗体设置位置这种就不能跳过第一次
 * @param parent
 */
DACommandsForGraphicsItemsMoved::DACommandsForGraphicsItemsMoved(const QList< QGraphicsItem* >& items,
                                                                 const QList< QPointF >& starts,
                                                                 const QList< QPointF >& ends,
                                                                 bool skipfirst,
                                                                 QUndoCommand* parent)
    : QUndoCommand(parent)
    , mItems(items)
    , mStartsPos(starts)
    , mEndsPos(ends)
    , mCmdDatetime(QDateTime::currentDateTime())
    , mSkipFirst(skipfirst)
{
    setText(QObject::tr("Items Move"));
}

void DACommandsForGraphicsItemsMoved::redo()
{
	QUndoCommand::redo();
	if (mSkipFirst) {
		mSkipFirst = false;
		return;
	}
	for (int i = 0; i < mItems.size(); ++i) {
		mItems[ i ]->setPos(mEndsPos[ i ]);
	}
}

void DACommandsForGraphicsItemsMoved::undo()
{
	QUndoCommand::undo();
	for (int i = 0; i < mItems.size(); ++i) {
		mItems[ i ]->setPos(mStartsPos[ i ]);
	}
}

int DACommandsForGraphicsItemsMoved::id() const
{
	return CmdID_ItemsMove;
}

bool DACommandsForGraphicsItemsMoved::mergeWith(const QUndoCommand* command)
{
	// 说明都是移动，看看是否能合并
	const DACommandsForGraphicsItemsMoved* other = dynamic_cast< const DACommandsForGraphicsItemsMoved* >(command);
	if (nullptr == other) {
		return false;
	}
	if (qAbs(mCmdDatetime.secsTo(other->mCmdDatetime)) > 10) {
		// 如果两个命令间隔大于10s，则认为是一个新命令，这两个命令就不合并，在移动中这种较
		return false;
	}
	// 判断item是否都一样
	if (other->mItems != mItems) {
		return false;
	}
	// 到这里，基本符合合并条件，只要把mEndsPos赋值给当前命令即可
	mEndsPos = other->mEndsPos;
#if DACOMMANDSFORGRAPHICS_DEBUG_PRINT
	qDebug() << "ItemsMoved was merge";
#endif
	return true;
}

const QList< QGraphicsItem* >& DACommandsForGraphicsItemsMoved::getItems() const
{
	return mItems;
}

const QList< QPointF >& DACommandsForGraphicsItemsMoved::getStartsPos() const
{
	return mStartsPos;
}

const QList< QPointF >& DACommandsForGraphicsItemsMoved::getEndsPos() const
{
	return mEndsPos;
}

//==============================================================
// DACommandsForGraphicsItemMoved
//==============================================================

DACommandsForGraphicsItemMoved::DACommandsForGraphicsItemMoved(QGraphicsItem* item,
                                                               const QPointF& start,
                                                               const QPointF& end,
                                                               bool skipfirst,
                                                               QUndoCommand* parent)
    : QUndoCommand(parent)
    , mItem(item)
    , mStartPos(start)
    , mEndPos(end)
    , mSkipFirst(skipfirst)
    , mDatetime(QDateTime::currentDateTime())
{
    setText(QObject::tr("Item Move"));
}

void DACommandsForGraphicsItemMoved::redo()
{
	QUndoCommand::redo();
	if (mSkipFirst) {
		mSkipFirst = false;
		return;
	}
	mItem->setPos(mEndPos);
}

void DACommandsForGraphicsItemMoved::undo()
{
	mItem->setPos(mStartPos);
}

int DACommandsForGraphicsItemMoved::id() const
{
	return CmdID_ItemMove;
}

bool DACommandsForGraphicsItemMoved::mergeWith(const QUndoCommand* command)
{
	const DACommandsForGraphicsItemMoved* other = dynamic_cast< const DACommandsForGraphicsItemMoved* >(command);
	if (nullptr == other) {
		return false;
	}
	if (qAbs(mDatetime.secsTo(other->mDatetime)) > 10) {
		return false;
	}
	if (mItem != other->mItem) {
		return false;
	}
	// 合并只改变最后的位置
	mEndPos = other->mEndPos;
	return true;
}

//==============================================================
// DACommandsForGraphicsItemResized
//==============================================================

/**
 * @brief 改变尺寸
 * @param item
 * @param oldpos
 * @param oldSize
 * @param newpos
 * @param newSize
 * @param parent
 */
DACommandsForGraphicsItemResized::DACommandsForGraphicsItemResized(DAGraphicsResizeableItem* item,
                                                                   const QPointF& oldpos,
                                                                   const QSizeF& oldSize,
                                                                   const QPointF& newpos,
                                                                   const QSizeF& newSize,
                                                                   bool skipfirst,
                                                                   QUndoCommand* parent)
    : QUndoCommand(parent)
    , mItem(item)
    , mOldpos(oldpos)
    , mOldSize(oldSize)
    , mNewPosition(newpos)
    , mNewSize(newSize)
    , mSkipfirst(skipfirst)
{
    setText(QObject::tr("Item Resize"));
}

DACommandsForGraphicsItemResized::DACommandsForGraphicsItemResized(DAGraphicsResizeableItem* item,
                                                                   const QSizeF& oldSize,
                                                                   const QSizeF& newSize,
                                                                   QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldSize(oldSize), mNewSize(newSize), mDatetime(QDateTime::currentDateTime())
{
	setText(QObject::tr("Item Resize"));
	mOldpos = mNewPosition = item->pos();
}

void DACommandsForGraphicsItemResized::redo()
{
	QUndoCommand::redo();
	if (mSkipfirst) {
		mSkipfirst = false;
		return;
	}
	if (mItem) {
		if (mNewSize.isValid()) {
			mItem->setBodySize(mNewSize);
		}
		if (!mNewPosition.isNull()) {
			mItem->setPos(mNewPosition);
		}
	}
}

void DACommandsForGraphicsItemResized::undo()
{
	QUndoCommand::undo();
	if (mItem) {
		if (mOldSize.isValid()) {
			mItem->setBodySize(mOldSize);
		}
		if (!mOldpos.isNull()) {
			mItem->setPos(mOldpos);
		}
	}
}

int DACommandsForGraphicsItemResized::id() const
{
	return CmdID_ItemResize;
}

bool DACommandsForGraphicsItemResized::mergeWith(const QUndoCommand* command)
{
	const DACommandsForGraphicsItemResized* other = dynamic_cast< const DACommandsForGraphicsItemResized* >(command);
	if (nullptr == other) {
		return false;
	}
	if (qAbs(mDatetime.secsTo(other->mDatetime)) > 10) {
		return false;
	}
	if (mItem != other->mItem) {
		return false;
	}
	// 合并只改变最后的位置
	mNewPosition = other->mNewPosition;
	mNewSize     = other->mNewSize;
	return true;
}

//==============================================================
// DACommandsForGraphicsItemResizeWidth
//==============================================================
/**
 * @brief 仅改变宽度
 * @param item
 * @param oldWidth
 * @param newWidth
 * @param parent
 */
DACommandsForGraphicsItemResizeWidth::DACommandsForGraphicsItemResizeWidth(DAGraphicsResizeableItem* item,
                                                                           const qreal& oldWidth,
                                                                           const qreal& newWidth,
                                                                           QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldWidth(oldWidth), mNewWidth(newWidth), mDatetime(QDateTime::currentDateTime())
{
	setText(QObject::tr("Item Resize Width"));
	mHeight = item->getBodySize().height();
}

void DACommandsForGraphicsItemResizeWidth::redo()
{
	mItem->setBodySize(QSizeF(mNewWidth, mHeight));
}

void DACommandsForGraphicsItemResizeWidth::undo()
{
	mItem->setBodySize(QSizeF(mOldWidth, mHeight));
}

int DACommandsForGraphicsItemResizeWidth::id() const
{
	return CmdID_ItemResizeWidth;
}

bool DACommandsForGraphicsItemResizeWidth::mergeWith(const QUndoCommand* command)
{
	const DACommandsForGraphicsItemResizeWidth* other = dynamic_cast< const DACommandsForGraphicsItemResizeWidth* >(command);
	if (nullptr == other) {
		return false;
	}
	if (qAbs(mDatetime.secsTo(other->mDatetime)) > 10) {
		return false;
	}
	if (mItem != other->mItem) {
		return false;
	}
	mNewWidth = other->mNewWidth;
	return true;
}
//==============================================================
// DACommandsForGraphicsItemResizeHeight
//==============================================================
DACommandsForGraphicsItemResizeHeight::DACommandsForGraphicsItemResizeHeight(DAGraphicsResizeableItem* item,
                                                                             const qreal& oldHeight,
                                                                             const qreal& newHeight,
                                                                             QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldHeight(oldHeight), mNewHeight(newHeight), mDatetime(QDateTime::currentDateTime())
{
	setText(QObject::tr("Item Resize Height"));
	mWidth = item->getBodySize().width();
}

void DACommandsForGraphicsItemResizeHeight::redo()
{
	mItem->setBodySize(QSizeF(mWidth, mNewHeight));
}

void DACommandsForGraphicsItemResizeHeight::undo()
{
	mItem->setBodySize(QSizeF(mWidth, mOldHeight));
}

int DACommandsForGraphicsItemResizeHeight::id() const
{
	return CmdID_ItemResizeHeight;
}

bool DACommandsForGraphicsItemResizeHeight::mergeWith(const QUndoCommand* command)
{
	const DACommandsForGraphicsItemResizeHeight* other =
		dynamic_cast< const DACommandsForGraphicsItemResizeHeight* >(command);
	if (nullptr == other) {
		return false;
	}
	if (qAbs(mDatetime.secsTo(other->mDatetime)) > 10) {
		return false;
	}
	if (mItem != other->mItem) {
		return false;
	}
	mNewHeight = other->mNewHeight;
	return true;
}
//==============================================================
// DACommandsForGraphicsItemRotation
//==============================================================
DACommandsForGraphicsItemRotation::DACommandsForGraphicsItemRotation(DAGraphicsResizeableItem* item,
                                                                     const qreal& oldRotation,
                                                                     const qreal& newRotation,
                                                                     QUndoCommand* parent)
    : QUndoCommand(parent)
    , mItem(item)
    , mOldRotation(oldRotation)
    , mNewRotation(newRotation)
    , mDatetime(QDateTime::currentDateTime())
{
    setText(QObject::tr("Item Rotation"));
}

void DACommandsForGraphicsItemRotation::redo()
{
	mItem->setRotation(mNewRotation);
}

void DACommandsForGraphicsItemRotation::undo()
{
	mItem->setRotation(mOldRotation);
}

int DACommandsForGraphicsItemRotation::id() const
{
	return CmdID_ItemRotation;
}

bool DACommandsForGraphicsItemRotation::mergeWith(const QUndoCommand* command)
{
	const DACommandsForGraphicsItemRotation* other = dynamic_cast< const DACommandsForGraphicsItemRotation* >(command);
	if (nullptr == other) {
		return false;
	}
	if (mItem != other->mItem) {
		return false;
	}
	if (qAbs(mDatetime.secsTo(other->mDatetime)) > 10) {
		return false;
	}
	mNewRotation = other->mNewRotation;
	return true;
}

//==============================================================
// DACommandsForGraphicsItemGrouping
//==============================================================

DACommandsForGraphicsItemGrouping::DACommandsForGraphicsItemGrouping(DAGraphicsScene* sc,
                                                                     const QList< QGraphicsItem* >& groupingitems,
                                                                     QUndoCommand* parent)
    : QUndoCommand(parent), mScene(sc)
{
    mWillGroupItems = toSimple(groupingitems);
}

DACommandsForGraphicsItemGrouping::~DACommandsForGraphicsItemGrouping()
{
	if (mNeedDelete) {
		delete mGroupItem;
	}
}

void DACommandsForGraphicsItemGrouping::redo()
{
	if (!mGroupItem) {
		mGroupItem = new DAGraphicsItemGroup();
	}
	DAGraphicsScene::addItemToGroup(mGroupItem, mWillGroupItems);
	mScene->addItem(mGroupItem);
	mGroupItem->setSelected(true);
	mNeedDelete = false;
}

void DACommandsForGraphicsItemGrouping::undo()
{
	// 不能用destroyItemGroup，destroyItemGroup会删除mGroupItem，如果之前做过移动操作，mGroupItem会被保存在其它的cmd中，这时候就会触发异常
	const auto items = mGroupItem->childItems();
	for (QGraphicsItem* item : items) {
		mGroupItem->removeFromGroup(item);
		item->setSelected(false);
	}
	mScene->removeItem(mGroupItem);
	mNeedDelete = true;
}

/**
 * @brief 这是一个清洗，要分组的item里面，如果存在item的parent在分组的item里，就驱除，这样不会分组嵌套，形成单一的层级
 * @param groupingitems
 * @return
 */
QList< QGraphicsItem* > DACommandsForGraphicsItemGrouping::toSimple(const QList< QGraphicsItem* >& groupingitems)
{
	const QSet< QGraphicsItem* > willGroupItems = qlist_to_qset(groupingitems);
	QList< QGraphicsItem* > res;
	for (QGraphicsItem* i : groupingitems) {
		bool ancestorsInGroup = false;
		QGraphicsItem* par    = i->parentItem();
		if (par) {
			do {
				if (willGroupItems.contains(par)) {
					ancestorsInGroup = true;
					break;
				}
				par = par->parentItem();
			} while (par);
		}
		if (!ancestorsInGroup) {
			res.append(i);
		}
	}
	return res;
}

QList< QGraphicsItem* > DACommandsForGraphicsItemGrouping::getWillGroupItems() const
{
	return mWillGroupItems;
}

//==============================================================
// DACommandsForGraphicsItemUngrouping
//==============================================================

DACommandsForGraphicsItemUngrouping::DACommandsForGraphicsItemUngrouping(QGraphicsScene* sc,
                                                                         QGraphicsItemGroup* group,
                                                                         QUndoCommand* parent)
    : QUndoCommand(parent), mScene(sc), mGroupItem(group)
{
    mItems = mGroupItem->childItems();
}

DACommandsForGraphicsItemUngrouping::~DACommandsForGraphicsItemUngrouping()
{
	if (mNeedDelete) {
		delete mGroupItem;
	}
}

void DACommandsForGraphicsItemUngrouping::redo()
{
	for (QGraphicsItem* item : std::as_const(mItems)) {
		mGroupItem->removeFromGroup(item);
		item->setSelected(false);
	}
	mScene->removeItem(mGroupItem);
	mNeedDelete = true;
}

void DACommandsForGraphicsItemUngrouping::undo()
{
	DAGraphicsScene::addItemToGroup(mGroupItem, mItems);
	mScene->addItem(mGroupItem);
	mGroupItem->setSelected(true);
	mNeedDelete = false;
}
//===============================================================
// DACommandTextDocumentWrapper
//===============================================================
DACommandTextDocumentWrapper::DACommandTextDocumentWrapper(QTextDocument* doc, QUndoCommand* parent)
    : QUndoCommand(parent), mDoc(doc)
{
}

DACommandTextDocumentWrapper::~DACommandTextDocumentWrapper()
{
}

void DACommandTextDocumentWrapper::redo()
{
	if (mDoc) {
		if (mDoc->isRedoAvailable()) {
			qDebug() << "doc redo";
			mDoc->redo();
		}
	}
}

void DACommandTextDocumentWrapper::undo()
{
	if (mDoc) {
		if (mDoc->isUndoAvailable()) {
			qDebug() << "doc undo";
			mDoc->undo();
		}
	}
}
//===============================================================
// DACommandTextItemHtmlContentChanged
//===============================================================
DACommandTextItemHtmlContentChanged::DACommandTextItemHtmlContentChanged(QGraphicsTextItem* item,
                                                                         const QString& oldHtml,
                                                                         const QString& newHtml,
                                                                         QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldHtml(oldHtml), mNewHtml(newHtml), mDate(QDateTime::currentDateTime())
{
}

DACommandTextItemHtmlContentChanged::~DACommandTextItemHtmlContentChanged()
{
}

void DACommandTextItemHtmlContentChanged::redo()
{
	QUndoCommand::redo();
	if (mSkipFirst) {
		mSkipFirst = false;
		return;
	}
	QSignalBlocker b(mItem->document());
	mItem->setHtml(mNewHtml);
}

void DACommandTextItemHtmlContentChanged::undo()
{
	QUndoCommand::undo();
	QSignalBlocker b(mItem->document());
	mItem->setHtml(mOldHtml);
}

bool DACommandTextItemHtmlContentChanged::mergeWith(const QUndoCommand* command)
{
	if (id() != command->id()) {
		return false;
	}
	const DACommandTextItemHtmlContentChanged* other = static_cast< const DACommandTextItemHtmlContentChanged* >(command);
	if (mItem != other->mItem) {
		return false;
	}
	// 时间是否满足
	// 两次操作间隔超过1分钟就不合并了
	if (qAbs(mDate.secsTo(other->mDate)) > 60) {
		return false;
	}
	mNewHtml = other->mNewHtml;
	return true;
}

}
