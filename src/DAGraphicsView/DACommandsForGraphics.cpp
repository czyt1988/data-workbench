#include "DACommandsForGraphics.h"
#include <QDebug>
#include <QGraphicsItem>
#include "DAGraphicsScene.h"
#include "DAIResizableGraphicsItem.h"
#include "DAGraphicsItem.h"
#include "DAGraphicsItemGroup.h"
#include "DAQtContainerUtil.hpp"
#include <QObject>
#include <QTextDocument>

#ifndef DACOMMANDSFORGRAPHICS_DEBUG_PRINT
#define DACOMMANDSFORGRAPHICS_DEBUG_PRINT 0
#endif

namespace DA
{
//----------------------------------------------------
// DACommandsForGraphicsItemAdd
//----------------------------------------------------

/**
 * @brief 构造函数，添加图元命令
 * @param item 要添加的图元
 * @param scene 目标场景
 * @param parent 父命令
 */
DACommandsForGraphicsItemAdd::DACommandsForGraphicsItemAdd(QGraphicsItem* item, QGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mScene(scene), mNeedDelete(false)
{
    setText(QObject::tr("Item Add"));  // cn:添加图元
}

/**
 * @brief 析构函数，如果图元已被移除且需要删除则释放内存
 */
DACommandsForGraphicsItemAdd::~DACommandsForGraphicsItemAdd()
{
	if (mNeedDelete) {
		delete mItem;
	}
}

/**
 * @brief 重做：将图元添加到场景中
 */
void DACommandsForGraphicsItemAdd::redo()
{
	QUndoCommand::redo();
	if (mItem->scene() != mScene) {
		mScene->addItem(mItem);
	}
	mNeedDelete = false;
}

/**
 * @brief 撤销：从场景中移除图元
 */
void DACommandsForGraphicsItemAdd::undo()
{
	QUndoCommand::undo();
	mScene->removeItem(mItem);
	mNeedDelete = true;
}
//----------------------------------------------------
// DACommandsForGraphicsItemsAdd
//----------------------------------------------------

/**
 * @brief 构造函数，添加多个图元命令
 * @param its 要添加的图元列表
 * @param scene 目标场景
 * @param parent 父命令
 */
DACommandsForGraphicsItemsAdd::DACommandsForGraphicsItemsAdd(const QList< QGraphicsItem* > its,
                                                             QGraphicsScene* scene,
                                                             QUndoCommand* parent)
    : QUndoCommand(parent), mItems(its), mScene(scene), mNeedDelete(false)
{
    setText(QObject::tr("Items Add"));  // cn:添加多个图元
}

/**
 * @brief 析构函数，如果图元已被移除且需要删除则释放所有图元内存
 */
DACommandsForGraphicsItemsAdd::~DACommandsForGraphicsItemsAdd()
{
	if (mNeedDelete) {
		for (QGraphicsItem* i : std::as_const(mItems)) {
			delete i;
		}
	}
}

/**
 * @brief 重做：将所有图元添加到场景中
 */
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

/**
 * @brief 撤销：从场景中移除所有图元
 */
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

/**
 * @brief 构造函数，移除图元命令
 * @param item 要移除的图元
 * @param scene 所在场景
 * @param parent 父命令
 */
DACommandsForGraphicsItemRemove::DACommandsForGraphicsItemRemove(QGraphicsItem* item, QGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mScene(scene), mNeedDelete(false)
{
    setText(QObject::tr("Item Remove"));  // cn:移除图元
}

/**
 * @brief 析构函数，如果图元需要删除则释放内存
 */
DACommandsForGraphicsItemRemove::~DACommandsForGraphicsItemRemove()
{
	if (mNeedDelete) {
		delete mItem;
	}
}

/**
 * @brief 重做：从场景中移除图元
 */
void DACommandsForGraphicsItemRemove::redo()
{
	QUndoCommand::redo();
	mScene->removeItem(mItem);
	mNeedDelete = true;
}

/**
 * @brief 撤销：将图元重新添加到场景中
 */
void DACommandsForGraphicsItemRemove::undo()
{
	QUndoCommand::undo();
	mScene->addItem(mItem);
	mNeedDelete = false;
}

//----------------------------------------------------
//  DACommandsForGraphicsItemsRemove
//----------------------------------------------------

/**
 * @brief 构造函数，移除多个图元命令
 * @param its 要移除的图元列表
 * @param scene 所在场景
 * @param parent 父命令
 */
DACommandsForGraphicsItemsRemove::DACommandsForGraphicsItemsRemove(const QList< QGraphicsItem* > its,
                                                                   QGraphicsScene* scene,
                                                                   QUndoCommand* parent)
    : QUndoCommand(parent), mItems(its), mScene(scene), mNeedDelete(false)
{
    setText(QObject::tr("Items Remove"));  // cn:移除多个图元
}

/**
 * @brief 析构函数，如果图元需要删除则释放所有图元内存
 */
DACommandsForGraphicsItemsRemove::~DACommandsForGraphicsItemsRemove()
{
	if (mNeedDelete) {
		for (QGraphicsItem* i : std::as_const(mItems)) {
			delete i;
		}
	}
}

/**
 * @brief 重做：从场景中移除所有图元
 */
void DACommandsForGraphicsItemsRemove::redo()
{
	QUndoCommand::redo();
	for (QGraphicsItem* item : std::as_const(mItems)) {
		if (item->scene() == mScene) {
			mScene->removeItem(item);
		}
	}
	mNeedDelete = true;
}

/**
 * @brief 撤销：将所有图元重新添加到场景中
 */
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
    Q_ASSERT_X(mItems.size() == mStartsPos.size() && mItems.size() == mEndsPos.size(),
               "DACommandsForGraphicsItemsMoved", "items, starts, ends size mismatch");
    setText(QObject::tr("Items Move"));  // cn:移动多个图元
}

/**
 * @brief 重做：将所有图元移动到最终位置
 */
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

/**
 * @brief 撤销：将所有图元恢复到起始位置
 */
void DACommandsForGraphicsItemsMoved::undo()
{
	QUndoCommand::undo();
	for (int i = 0; i < mItems.size(); ++i) {
		mItems[ i ]->setPos(mStartsPos[ i ]);
	}
}

/**
 * @brief 返回命令的唯一标识ID，用于命令合并判断
 * @return 命令ID
 */
int DACommandsForGraphicsItemsMoved::id() const
{
	return CmdID_ItemsMove;
}

/**
 * @brief 尝试与另一个移动命令合并
 * @param command 待合并的命令
 * @return 合并成功返回true，否则返回false
 */
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
	return true;
}

/**
 * @brief 获取参与移动的图元列表
 * @return 图元列表的常引用
 */
const QList< QGraphicsItem* >& DACommandsForGraphicsItemsMoved::getItems() const
{
	return mItems;
}

/**
 * @brief 获取图元的起始位置列表
 * @return 起始位置列表的常引用
 */
const QList< QPointF >& DACommandsForGraphicsItemsMoved::getStartsPos() const
{
	return mStartsPos;
}

/**
 * @brief 获取图元的最终位置列表
 * @return 最终位置列表的常引用
 */
const QList< QPointF >& DACommandsForGraphicsItemsMoved::getEndsPos() const
{
	return mEndsPos;
}

//==============================================================
// DACommandsForGraphicsItemMoved
//==============================================================

/**
 * @brief 构造函数，移动单个图元命令
 * @param item 要移动的图元
 * @param start 起始位置
 * @param end 最终位置
 * @param skipfirst 是否跳过第一次执行
 * @param parent 父命令
 */
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
    setText(QObject::tr("Item Move"));  // cn:移动图元
}

/**
 * @brief 重做：将图元移动到最终位置
 */
void DACommandsForGraphicsItemMoved::redo()
{
	QUndoCommand::redo();
	if (mSkipFirst) {
		mSkipFirst = false;
		return;
	}
	if (mItem) {
		mItem->setPos(mEndPos);
	}
}

/**
 * @brief 撤销：将图元恢复到起始位置
 */
void DACommandsForGraphicsItemMoved::undo()
{
	if (mItem) {
		mItem->setPos(mStartPos);
	}
}

/**
 * @brief 返回命令的唯一标识ID，用于命令合并判断
 * @return 命令ID
 */
int DACommandsForGraphicsItemMoved::id() const
{
	return CmdID_ItemMove;
}

/**
 * @brief 尝试与另一个移动命令合并
 * @param command 待合并的命令
 * @return 合并成功返回true，否则返回false
 */
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
DACommandsForGraphicsItemResized::DACommandsForGraphicsItemResized(DAIResizableGraphicsItem* item,
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
    , mDatetime(QDateTime::currentDateTime())
{
    setText(QObject::tr("Item Resize"));  // cn:调整图元尺寸
}

DACommandsForGraphicsItemResized::DACommandsForGraphicsItemResized(DAIResizableGraphicsItem* item,
                                                                   const QSizeF& oldSize,
                                                                   const QSizeF& newSize,
                                                                   QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldSize(oldSize), mNewSize(newSize), mHasPosition(false), mDatetime(QDateTime::currentDateTime())
{
	setText(QObject::tr("Item Resize"));  // cn:调整图元尺寸
	mOldpos = mNewPosition = item->graphicsItem()->pos();
}

/**
 * @brief 重做：将图元设置为新尺寸和新位置
 */
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
		if (mHasPosition) {
			mItem->graphicsItem()->setPos(mNewPosition);
		}
	}
}

/**
 * @brief 撤销：将图元恢复到旧尺寸和旧位置
 */
void DACommandsForGraphicsItemResized::undo()
{
	QUndoCommand::undo();
	if (mItem) {
		if (mOldSize.isValid()) {
			mItem->setBodySize(mOldSize);
		}
		if (mHasPosition) {
			mItem->graphicsItem()->setPos(mOldpos);
		}
	}
}

/**
 * @brief 返回命令的唯一标识ID，用于命令合并判断
 * @return 命令ID
 */
int DACommandsForGraphicsItemResized::id() const
{
	return CmdID_ItemResize;
}

/**
 * @brief 尝试与另一个改变尺寸命令合并
 * @param command 待合并的命令
 * @return 合并成功返回true，否则返回false
 */
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
DACommandsForGraphicsItemResizeWidth::DACommandsForGraphicsItemResizeWidth(DAIResizableGraphicsItem* item,
                                                                           const qreal& oldWidth,
                                                                           const qreal& newWidth,
                                                                           QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldWidth(oldWidth), mNewWidth(newWidth), mDatetime(QDateTime::currentDateTime())
{
	setText(QObject::tr("Item Resize Width"));  // cn:调整图元宽度
}

/**
 * @brief 重做：将图元宽度设置为新宽度
 */
void DACommandsForGraphicsItemResizeWidth::redo()
{
	QUndoCommand::redo();
	if (mItem) {
		mItem->setBodySize(QSizeF(mNewWidth, mItem->getBodySize().height()));
	}
}

/**
 * @brief 撤销：将图元宽度恢复到旧宽度
 */
void DACommandsForGraphicsItemResizeWidth::undo()
{
	QUndoCommand::undo();
	if (mItem) {
		mItem->setBodySize(QSizeF(mOldWidth, mItem->getBodySize().height()));
	}
}

/**
 * @brief 返回命令的唯一标识ID，用于命令合并判断
 * @return 命令ID
 */
int DACommandsForGraphicsItemResizeWidth::id() const
{
	return CmdID_ItemResizeWidth;
}

/**
 * @brief 尝试与另一个改变宽度命令合并
 * @param command 待合并的命令
 * @return 合并成功返回true，否则返回false
 */
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
DACommandsForGraphicsItemResizeHeight::DACommandsForGraphicsItemResizeHeight(DAIResizableGraphicsItem* item,
                                                                             const qreal& oldHeight,
                                                                             const qreal& newHeight,
                                                                             QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldHeight(oldHeight), mNewHeight(newHeight), mDatetime(QDateTime::currentDateTime())
{
	setText(QObject::tr("Item Resize Height"));  // cn:调整图元高度
}

/**
 * @brief 重做：将图元高度设置为新高度
 */
void DACommandsForGraphicsItemResizeHeight::redo()
{
	QUndoCommand::redo();
	if (mItem) {
		mItem->setBodySize(QSizeF(mItem->getBodySize().width(), mNewHeight));
	}
}

/**
 * @brief 撤销：将图元高度恢复到旧高度
 */
void DACommandsForGraphicsItemResizeHeight::undo()
{
	QUndoCommand::undo();
	if (mItem) {
		mItem->setBodySize(QSizeF(mItem->getBodySize().width(), mOldHeight));
	}
}

/**
 * @brief 返回命令的唯一标识ID，用于命令合并判断
 * @return 命令ID
 */
int DACommandsForGraphicsItemResizeHeight::id() const
{
	return CmdID_ItemResizeHeight;
}

/**
 * @brief 尝试与另一个改变高度命令合并
 * @param command 待合并的命令
 * @return 合并成功返回true，否则返回false
 */
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
DACommandsForGraphicsItemRotation::DACommandsForGraphicsItemRotation(DAIResizableGraphicsItem* item,
                                                                     const qreal& oldRotation,
                                                                     const qreal& newRotation,
                                                                     bool skipfirst,
                                                                     QUndoCommand* parent)
    : QUndoCommand(parent)
    , mItem(item)
    , mOldRotation(oldRotation)
    , mNewRotation(newRotation)
    , mSkipFirst(skipfirst)
    , mDatetime(QDateTime::currentDateTime())
{
    setText(QObject::tr("Item Rotation"));  // cn:旋转图元
}

/**
 * @brief 重做：将图元旋转到新角度
 */
void DACommandsForGraphicsItemRotation::redo()
{
	QUndoCommand::redo();
	if (mSkipFirst) {
		mSkipFirst = false;
		return;
	}
	if (mItem) {
		mItem->graphicsItem()->setRotation(mNewRotation);
	}
}

/**
 * @brief 撤销：将图元旋转恢复到旧角度
 */
void DACommandsForGraphicsItemRotation::undo()
{
	QUndoCommand::undo();
	if (mItem) {
		mItem->graphicsItem()->setRotation(mOldRotation);
	}
}

/**
 * @brief 返回命令的唯一标识ID，用于命令合并判断
 * @return 命令ID
 */
int DACommandsForGraphicsItemRotation::id() const
{
	return CmdID_ItemRotation;
}

/**
 * @brief 尝试与另一个旋转命令合并
 * @param command 待合并的命令
 * @return 合并成功返回true，否则返回false
 */
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

/**
 * @brief 构造函数，图元分组命令
 * @param sc 目标场景
 * @param groupingitems 待分组的图元列表
 * @param parent 父命令
 */
DACommandsForGraphicsItemGrouping::DACommandsForGraphicsItemGrouping(DAGraphicsScene* sc,
                                                                     const QList< QGraphicsItem* >& groupingitems,
                                                                     QUndoCommand* parent)
    : QUndoCommand(parent), mScene(sc)
{
    mWillGroupItems = toSimple(groupingitems);
}

/**
 * @brief 析构函数，如果分组项需要删除则释放内存
 */
DACommandsForGraphicsItemGrouping::~DACommandsForGraphicsItemGrouping()
{
	if (mNeedDelete) {
		delete mGroupItem;
	}
}

/**
 * @brief 重做：创建分组并将图元添加到分组中
 */
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

/**
 * @brief 撤销：从分组中移除所有子图元并从场景中移除分组项
 */
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

/**
 * @brief 获取将要分组的图元列表
 * @return 将要分组的图元列表
 */
QList< QGraphicsItem* > DACommandsForGraphicsItemGrouping::getWillGroupItems() const
{
	return mWillGroupItems;
}

//==============================================================
// DACommandsForGraphicsItemUngrouping
//==============================================================

/**
 * @brief 构造函数，取消图元分组命令
 * @param sc 目标场景
 * @param group 待取消分组的图元组
 * @param parent 父命令
 */
DACommandsForGraphicsItemUngrouping::DACommandsForGraphicsItemUngrouping(QGraphicsScene* sc,
                                                                         QGraphicsItemGroup* group,
                                                                         QUndoCommand* parent)
    : QUndoCommand(parent), mScene(sc), mGroupItem(group)
{
    mItems = mGroupItem->childItems();
}

/**
 * @brief 析构函数，如果分组项需要删除则释放内存
 */
DACommandsForGraphicsItemUngrouping::~DACommandsForGraphicsItemUngrouping()
{
	if (mNeedDelete) {
		delete mGroupItem;
	}
}

/**
 * @brief 重做：从分组中移除所有子图元并从场景中移除分组项
 */
void DACommandsForGraphicsItemUngrouping::redo()
{
	for (QGraphicsItem* item : std::as_const(mItems)) {
		mGroupItem->removeFromGroup(item);
		item->setSelected(false);
	}
	mScene->removeItem(mGroupItem);
	mNeedDelete = true;
}

/**
 * @brief 撤销：将子图元重新添加到分组中并将分组添加到场景
 */
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
/**
 * @brief 构造函数，QTextDocument撤销命令包装器
 * @param doc 被包装的文本文档
 * @param parent 父命令
 */
DACommandTextDocumentWrapper::DACommandTextDocumentWrapper(QTextDocument* doc, QUndoCommand* parent)
    : QUndoCommand(parent), mDoc(doc)
{
}

/**
 * @brief 析构函数
 */
DACommandTextDocumentWrapper::~DACommandTextDocumentWrapper()
{
}

/**
 * @brief 重做：调用文本文档的redo操作
 */
void DACommandTextDocumentWrapper::redo()
{
	if (mDoc) {
		if (mDoc->isRedoAvailable()) {
			mDoc->redo();
		}
	}
}

/**
 * @brief 撤销：调用文本文档的undo操作
 */
void DACommandTextDocumentWrapper::undo()
{
	if (mDoc) {
		if (mDoc->isUndoAvailable()) {
			mDoc->undo();
		}
	}
}
//===============================================================
// DACommandTextItemHtmlContentChanged
//===============================================================
/**
 * @brief 构造函数，文本图元HTML内容变更命令
 * @param item 目标文本图元
 * @param oldHtml 旧HTML内容
 * @param newHtml 新HTML内容
 * @param parent 父命令
 */
DACommandTextItemHtmlContentChanged::DACommandTextItemHtmlContentChanged(QGraphicsTextItem* item,
                                                                         const QString& oldHtml,
                                                                         const QString& newHtml,
                                                                         QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldHtml(oldHtml), mNewHtml(newHtml), mDate(QDateTime::currentDateTime())
{
}

/**
 * @brief 析构函数
 */
DACommandTextItemHtmlContentChanged::~DACommandTextItemHtmlContentChanged()
{
}

/**
 * @brief 重做：将文本图元设置为新HTML内容
 */
void DACommandTextItemHtmlContentChanged::redo()
{
	QUndoCommand::redo();
	if (mSkipFirst) {
		mSkipFirst = false;
		return;
	}
	if (mItem) {
		QSignalBlocker b(mItem->document());
		mItem->setHtml(mNewHtml);
	}
}

/**
 * @brief 撤销：将文本图元恢复到旧HTML内容
 */
void DACommandTextItemHtmlContentChanged::undo()
{
	QUndoCommand::undo();
	if (mItem) {
		QSignalBlocker b(mItem->document());
		mItem->setHtml(mOldHtml);
	}
}

/**
 * @brief 尝试与另一个HTML内容变更命令合并
 * @param command 待合并的命令
 * @return 合并成功返回true，否则返回false
 */
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
