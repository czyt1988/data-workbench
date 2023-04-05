#include "DACommandsForGraphics.h"
#include <QDebug>
#include <QGraphicsItem>
#include "DAGraphicsSceneWithUndoStack.h"
#include "DAGraphicsResizeableItem.h"
#include <QObject>
using namespace DA;

DACommandsForGraphicsItemAdd::DACommandsForGraphicsItemAdd(QGraphicsItem* item, QGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), _item(item), _scene(scene), _needDelete(false)
{
    setText(QObject::tr("Item Add"));
}

DACommandsForGraphicsItemAdd::~DACommandsForGraphicsItemAdd()
{
    if (_needDelete) {
        delete _item;
    }
}

void DACommandsForGraphicsItemAdd::redo()
{
    QUndoCommand::redo();
    _scene->addItem(_item);
    _needDelete = false;
}

void DACommandsForGraphicsItemAdd::undo()
{
    QUndoCommand::undo();
    _scene->removeItem(_item);
    _needDelete = true;
}

////////////////////////////////////////////////////////////

DACommandsForGraphicsItemRemove::DACommandsForGraphicsItemRemove(QGraphicsItem* item, QGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), _item(item), _scene(scene), _needDelete(false)
{
    setText(QObject::tr("Item Remove"));
}

DACommandsForGraphicsItemRemove::~DACommandsForGraphicsItemRemove()
{
    if (_needDelete) {
        delete _item;
    }
}

void DACommandsForGraphicsItemRemove::redo()
{
    QUndoCommand::redo();
    _scene->removeItem(_item);
    _needDelete = true;
}

void DACommandsForGraphicsItemRemove::undo()
{
    QUndoCommand::undo();
    _scene->addItem(_item);
    _needDelete = false;
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
    : QUndoCommand(parent), _items(items), _startsPos(starts), _endsPos(ends), _skipFirst(skipfirst)
{
    setText(QObject::tr("Items Move"));
}

void DACommandsForGraphicsItemsMoved::redo()
{
    QUndoCommand::redo();
    if (_skipFirst) {
        _skipFirst = false;
        return;
    }
    for (int i = 0; i < _items.size(); ++i) {
        _items[ i ]->setPos(_endsPos[ i ]);
    }
}

void DACommandsForGraphicsItemsMoved::undo()
{
    QUndoCommand::undo();
    for (int i = 0; i < _items.size(); ++i) {
        _items[ i ]->setPos(_startsPos[ i ]);
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
    if (other->_items != _items) {
        // items不一样不能合并
        return false;
    }
    _endsPos = other->_endsPos;
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
    : QUndoCommand(parent), _item(item), _startPos(start), _endPos(end), _skipFirst(skipfirst)
{
    setText(QObject::tr("Item Move"));
}

void DACommandsForGraphicsItemMoved::redo()
{
    QUndoCommand::redo();
    if (_skipFirst) {
        _skipFirst = false;
        return;
    }
    _item->setPos(_endPos);
}

void DACommandsForGraphicsItemMoved::undo()
{
    _item->setPos(_startPos);
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
    if (_item != other->_item) {
        return false;
    }
    //合并只改变最后的位置
    _endPos = other->_endPos;
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
    : QUndoCommand(parent), _item(item), _oldpos(oldpos), _oldSize(oldSize), _newpos(newpos), _newSize(newSize), _skipFirst(skipFirst)
{
    setText(QObject::tr("Item Resize"));
}

DACommandsForGraphicsItemResized::DACommandsForGraphicsItemResized(DAGraphicsResizeableItem* item,
                                                                   const QSizeF& oldSize,
                                                                   const QSizeF& newSize,
                                                                   bool skipFirst,
                                                                   QUndoCommand* parent)
    : QUndoCommand(parent), _item(item), _oldSize(oldSize), _newSize(newSize), _skipFirst(skipFirst)
{
    setText(QObject::tr("Item Resize"));
    _oldpos = _newpos = item->pos();
}

void DACommandsForGraphicsItemResized::redo()
{
    QUndoCommand::redo();
    if (_skipFirst) {
        _skipFirst = false;
        return;
    }
    if (_item) {
        if (_newSize.isValid()) {
            _item->setBodySize(_newSize);
        }
        if (!_newpos.isNull()) {
            _item->setPos(_newpos);
        }
    }
}

void DACommandsForGraphicsItemResized::undo()
{
    QUndoCommand::undo();
    if (_item) {
        if (_oldSize.isValid()) {
            _item->setBodySize(_oldSize);
        }
        if (!_oldpos.isNull()) {
            _item->setPos(_oldpos);
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
    if (_item != other->_item) {
        return false;
    }
    //合并只改变最后的位置
    _newpos  = other->_newpos;
    _newSize = other->_newSize;
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
    : QUndoCommand(parent), _item(item), _oldWidth(oldWidth), _newWidth(newWidth), _skipfirst(skipfirst)
{
    setText(QObject::tr("Item Resize Width"));
    _height = item->getBodySize().height();
}

void DACommandsForGraphicsItemResizeWidth::redo()
{
    if (_skipfirst) {
        _skipfirst = false;
        return;
    }
    _item->setBodySize(QSizeF(_newWidth, _height));
}

void DACommandsForGraphicsItemResizeWidth::undo()
{
    _item->setBodySize(QSizeF(_oldWidth, _height));
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
    if (_item != other->_item) {
        return false;
    }
    _newWidth = other->_newWidth;
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
    : QUndoCommand(parent), _item(item), _oldHeight(oldHeight), _newHeight(newHeight), _skipfirst(skipfirst)
{
    setText(QObject::tr("Item Resize Height"));
    _width = item->getBodySize().width();
}

void DACommandsForGraphicsItemResizeHeight::redo()
{
    if (_skipfirst) {
        _skipfirst = false;
        return;
    }
    _item->setBodySize(QSizeF(_width, _newHeight));
}

void DACommandsForGraphicsItemResizeHeight::undo()
{
    _item->setBodySize(QSizeF(_width, _oldHeight));
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
    if (_item != other->_item) {
        return false;
    }
    _newHeight = other->_newHeight;
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
    : QUndoCommand(parent), _item(item), _oldRotation(oldRotation), _newRotation(newRotation), _skipfirst(skipfirst)
{
    setText(QObject::tr("Item Rotation"));
}

void DACommandsForGraphicsItemRotation::redo()
{
    if (_skipfirst) {
        _skipfirst = false;
        return;
    }
    _item->setRotation(_newRotation);
}

void DACommandsForGraphicsItemRotation::undo()
{
    // qDebug() << "Item Reset Rotation " << _oldRotation;
    _item->setRotation(_oldRotation);
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
    if (_item != other->_item) {
        return false;
    }
    _newRotation = other->_newRotation;
    return true;
}
