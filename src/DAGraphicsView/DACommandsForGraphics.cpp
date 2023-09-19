#include "DACommandsForGraphics.h"
#include <QDebug>
#include <QGraphicsItem>
#include "DAGraphicsSceneWithUndoStack.h"
#include "DAGraphicsResizeableItem.h"
#include <QObject>
using namespace DA;

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

////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////

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
    : QUndoCommand(parent), mItems(items), mStartsPos(starts), mEndsPos(ends), mSkipFirst(skipfirst)
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
/**
 * @brief 合并
 * @param command
 * @return
 */
bool DACommandsForGraphicsItemsMoved::mergeWith(const QUndoCommand* command)
{
    const DACommandsForGraphicsItemsMoved* other = dynamic_cast< const DACommandsForGraphicsItemsMoved* >(command);
    if (nullptr == other) {
        return false;
    }
    if (other->mItems != mItems) {
        // items不一样不能合并
        return false;
    }
    mEndsPos = other->mEndsPos;
    return true;
}
//==============================================================
// DACommandsForGraphicsItemMoved
//==============================================================

DACommandsForGraphicsItemMoved::DACommandsForGraphicsItemMoved(QGraphicsItem* item,
                                                               const QPointF& start,
                                                               const QPointF& end,
                                                               bool skipfirst,
                                                               QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mStartPos(start), mEndPos(end), mSkipFirst(skipfirst)
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
    if (mItem != other->mItem) {
        return false;
    }
    //合并只改变最后的位置
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
                                                                   bool skipFirst,
                                                                   QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldpos(oldpos), mOldSize(oldSize), mNewPosition(newpos), mNewSize(newSize), mSkipFirst(skipFirst)
{
    setText(QObject::tr("Item Resize"));
}

DACommandsForGraphicsItemResized::DACommandsForGraphicsItemResized(DAGraphicsResizeableItem* item,
                                                                   const QSizeF& oldSize,
                                                                   const QSizeF& newSize,
                                                                   bool skipFirst,
                                                                   QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldSize(oldSize), mNewSize(newSize), mSkipFirst(skipFirst)
{
    setText(QObject::tr("Item Resize"));
    mOldpos = mNewPosition = item->pos();
}

void DACommandsForGraphicsItemResized::redo()
{
    QUndoCommand::redo();
    if (mSkipFirst) {
        mSkipFirst = false;
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
    if (mItem != other->mItem) {
        return false;
    }
    //合并只改变最后的位置
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
                                                                           bool skipfirst,
                                                                           QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldWidth(oldWidth), mNewWidth(newWidth), mSkipfirst(skipfirst)
{
    setText(QObject::tr("Item Resize Width"));
    mHeight = item->getBodySize().height();
}

void DACommandsForGraphicsItemResizeWidth::redo()
{
    if (mSkipfirst) {
        mSkipfirst = false;
        return;
    }
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
                                                                             bool skipfirst,
                                                                             QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldHeight(oldHeight), mNewHeight(newHeight), mSkipfirst(skipfirst)
{
    setText(QObject::tr("Item Resize Height"));
    mWidth = item->getBodySize().width();
}

void DACommandsForGraphicsItemResizeHeight::redo()
{
    if (mSkipfirst) {
        mSkipfirst = false;
        return;
    }
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
    const DACommandsForGraphicsItemResizeHeight* other = dynamic_cast< const DACommandsForGraphicsItemResizeHeight* >(command);
    if (nullptr == other) {
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
                                                                     bool skipfirst,
                                                                     QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldRotation(oldRotation), mNewRotation(newRotation), mSkipfirst(skipfirst)
{
    setText(QObject::tr("Item Rotation"));
}

void DACommandsForGraphicsItemRotation::redo()
{
    if (mSkipfirst) {
        mSkipfirst = false;
        return;
    }
    mItem->setRotation(mNewRotation);
}

void DACommandsForGraphicsItemRotation::undo()
{
    // qDebug() << "Item Reset Rotation " << _oldRotation;
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
    mNewRotation = other->mNewRotation;
    return true;
}
