#ifndef DACOMMANDSFORGRAPHICS_H
#define DACOMMANDSFORGRAPHICS_H
#include "DAGraphicsViewGlobal.h"
#include <QUndoCommand>
class QGraphicsScene;
class QGraphicsItem;

namespace DA
{
class DAGraphicsResizeableItem;
/**
 * @brief 添加item命令
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemAdd : public QUndoCommand
{
public:
    DACommandsForGraphicsItemAdd(QGraphicsItem* item, QGraphicsScene* scene, QUndoCommand* parent = nullptr);
    ~DACommandsForGraphicsItemAdd();
    void redo() override;
    void undo() override;

private:
    QGraphicsItem* mItem;
    QGraphicsScene* mScene;
    bool mNeedDelete;
};

/**
 * @brief 移除item命令
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemRemove : public QUndoCommand
{
public:
    DACommandsForGraphicsItemRemove(QGraphicsItem* item, QGraphicsScene* scene, QUndoCommand* parent = nullptr);
    ~DACommandsForGraphicsItemRemove();
    void redo() override;
    void undo() override;

private:
    QGraphicsItem* mItem;
    QGraphicsScene* mScene;
    bool mNeedDelete;
};

/**
 * @brief 移动item命令，必须先确保item是movable的
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemsMoved : public QUndoCommand
{
public:
    DACommandsForGraphicsItemsMoved(const QList< QGraphicsItem* >& items,
                                    const QList< QPointF >& starts,
                                    const QList< QPointF >& ends,
                                    bool skipfirst,
                                    QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* command);

private:
    QList< QGraphicsItem* > mItems;
    QList< QPointF > mStartsPos;
    QList< QPointF > mEndsPos;
    bool mSkipFirst;
};

/**
 * @brief 移动item命令，必须先确保item是movable的
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemMoved : public QUndoCommand
{
public:
    DACommandsForGraphicsItemMoved(QGraphicsItem* item, const QPointF& start, const QPointF& end, bool skipfirst, QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* command);

private:
    QGraphicsItem* mItem;
    QPointF mStartPos;
    QPointF mEndPos;
    bool mSkipFirst;
};
/**
 * @brief item改变尺寸
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemResized : public QUndoCommand
{
public:
    DACommandsForGraphicsItemResized(DAGraphicsResizeableItem* item,
                                     const QPointF& oldpos,
                                     const QSizeF& oldSize,
                                     const QPointF& newpos,
                                     const QSizeF& newSize,
                                     bool skipFirst       = false,
                                     QUndoCommand* parent = nullptr);
    DACommandsForGraphicsItemResized(DAGraphicsResizeableItem* item,
                                     const QSizeF& oldSize,
                                     const QSizeF& newSize,
                                     bool skipFirst       = false,
                                     QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* command);

private:
    DAGraphicsResizeableItem* mItem;
    QPointF mOldpos;
    QSizeF mOldSize;
    QPointF mNewPosition;
    QSizeF mNewSize;
    bool mSkipFirst;
};

/**
 * @brief 仅改变宽度
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemResizeWidth : public QUndoCommand
{
public:
    DACommandsForGraphicsItemResizeWidth(DAGraphicsResizeableItem* item,
                                         const qreal& oldWidth,
                                         const qreal& newWidth,
                                         bool skipfirst,
                                         QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* command);

private:
    DAGraphicsResizeableItem* mItem;
    qreal mOldWidth;
    qreal mNewWidth;
    qreal mHeight;
    bool mSkipfirst;
};

/**
 * @brief The DACommandsForGraphicsItemResizeWidth class
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemResizeHeight : public QUndoCommand
{
public:
    DACommandsForGraphicsItemResizeHeight(DAGraphicsResizeableItem* item,
                                          const qreal& oldHeight,
                                          const qreal& newHeight,
                                          bool skipfirst,
                                          QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* command);

private:
    DAGraphicsResizeableItem* mItem;
    qreal mOldHeight;
    qreal mNewHeight;
    qreal mWidth;
    bool mSkipfirst;
};

/**
 * @brief The DACommandsForGraphicsItemRotation class
 *
 * 旋转中心为bodysize的中心
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemRotation : public QUndoCommand
{
public:
    DACommandsForGraphicsItemRotation(DAGraphicsResizeableItem* item,
                                      const qreal& oldRotation,
                                      const qreal& newRotation,
                                      bool skipfirst,
                                      QUndoCommand* parent = nullptr);
    void redo() override;
    void undo() override;
    int id() const override;
    bool mergeWith(const QUndoCommand* command);

private:
    DAGraphicsResizeableItem* mItem;
    qreal mOldRotation;
    qreal mNewRotation;
    bool mSkipfirst;
};
}
#endif  // DACOMMANDSFORGRAPHICS_H
