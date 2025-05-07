#ifndef DAGRAPHICSCOMMANDSFACTORY_H
#define DAGRAPHICSCOMMANDSFACTORY_H
#include "DAGraphicsViewGlobal.h"
#include "DACommandsForGraphics.h"
namespace DA
{
class DAGraphicsScene;
class DAGraphicsResizeableItem;
/**
 * @brief 命令工厂
 *
 * 命令工厂有两个作用
 *
 * - 第一，可以根据字符串查找生成命令（这个暂时不实现）
 * - 第二，可以用户自定义命令，例如移动命令，用户实现的移动命令需要记录其它的特殊功能，需要继承原来的移动命令则用户可以定义一个自己的命令工厂，针对移动命令生成一个用户自己的移动命令
 */
class DAGRAPHICSVIEW_API DAGraphicsCommandsFactory
{
    friend class DAGraphicsScene;

public:
	DAGraphicsCommandsFactory();
	virtual ~DAGraphicsCommandsFactory();
	virtual DACommandsForGraphicsItemAdd* createItemAdd(QGraphicsItem* item, QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemsAdd* createItemsAdd(const QList< QGraphicsItem* > its, QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemRemove* createItemRemove(QGraphicsItem* item, QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemsRemove* createItemsRemove(const QList< QGraphicsItem* > its,
																QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemMoved* createItemMoved(QGraphicsItem* item,
															const QPointF& start,
															const QPointF& end,
															bool skipfirst,
															QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemMoved_Merge* createItemMoved_Merge(QGraphicsItem* item,
																		const QPointF& start,
																		const QPointF& end,
																		bool skipfirst,
																		QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemsMoved* createItemsMoved(const QList< QGraphicsItem* >& items,
															  const QList< QPointF >& starts,
															  const QList< QPointF >& ends,
															  bool skipfirst,
															  QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemsMoved_Merge* createItemsMoved_Merge(const QList< QGraphicsItem* >& items,
																		  const QList< QPointF >& starts,
																		  const QList< QPointF >& ends,
																		  bool skipfirst,
																		  QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemResized* createItemResized(DAGraphicsResizeableItem* item,
																const QPointF& oldpos,
																const QSizeF& oldSize,
																const QPointF& newpos,
																const QSizeF& newSize,
																bool skipfirst       = true,
																QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemResized* createItemResized(DAGraphicsResizeableItem* item,
																const QSizeF& oldSize,
																const QSizeF& newSize,
																QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemResizeWidth* createItemResizeWidth(DAGraphicsResizeableItem* item,
																		const qreal& oldWidth,
																		const qreal& newWidth,
																		QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemResizeHeight* createItemResizeHeight(DAGraphicsResizeableItem* item,
																		  const qreal& oldHeight,
																		  const qreal& newHeight,
																		  QUndoCommand* parent = nullptr);
	virtual DACommandsForGraphicsItemRotation* createItemRotation(DAGraphicsResizeableItem* item,
																  const qreal& oldRotation,
																  const qreal& newRotation,
																  QUndoCommand* parent = nullptr);

protected:
	void setScene(DAGraphicsScene* s);
	DAGraphicsScene* scene() const;

private:
	DAGraphicsScene* mScene { nullptr };
};
}  // end DA
#endif  // DAGRAPHICSVIEWCOMMANDSFACTORY_H
