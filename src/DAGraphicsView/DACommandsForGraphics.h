#ifndef DACOMMANDSFORGRAPHICS_H
#define DACOMMANDSFORGRAPHICS_H
#include "DAGraphicsViewGlobal.h"
#include <QUndoCommand>
#include <QPointer>
class QGraphicsScene;
class QGraphicsItem;
class QTextDocument;

/**
  @note command一定要注意创建移动陷阱，所谓创建移动陷阱，是指一个创建命令，会new出一个item，移动命令，会把new出的item的位置记录，
  这时，创建命令在析构时，如果做了delete创建item的处理，那么就会造成野指针
 */

namespace DA
{
class DAGraphicsScene;
class DAGraphicsResizeableItem;
/**
 * @brief 添加item命令
 *
 * @note 如果item已经调用Scene::addItem函数，此命令不会重复addItem
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

protected:
	QList< QGraphicsItem* > mItems;
	QList< QPointF > mStartsPos;
	QList< QPointF > mEndsPos;
	bool mSkipFirst;
};

class DAGRAPHICSVIEW_API DACommandsForGraphicsItemsMoved_Merge : public DACommandsForGraphicsItemsMoved
{
public:
	DACommandsForGraphicsItemsMoved_Merge(const QList< QGraphicsItem* >& items,
										  const QList< QPointF >& starts,
										  const QList< QPointF >& ends,
										  bool skipfirst,
										  QUndoCommand* parent = nullptr);
	int id() const override;
	bool mergeWith(const QUndoCommand* command) override;
};

/**
 * @brief 移动item命令，必须先确保item是movable的
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemMoved : public QUndoCommand
{
public:
	DACommandsForGraphicsItemMoved(QGraphicsItem* item,
								   const QPointF& start,
								   const QPointF& end,
								   bool skipfirst,
								   QUndoCommand* parent = nullptr);
	void redo() override;
	void undo() override;
	int id() const override;

protected:
	QGraphicsItem* mItem;
	QPointF mStartPos;
	QPointF mEndPos;
	bool mSkipFirst;
};

/**
 * @brief 带合并的移动item命令
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemMoved_Merge : public DACommandsForGraphicsItemMoved
{
public:
	DACommandsForGraphicsItemMoved_Merge(QGraphicsItem* item,
										 const QPointF& start,
										 const QPointF& end,
										 bool skipfirst,
										 QUndoCommand* parent = nullptr);
	int id() const override;
	bool mergeWith(const QUndoCommand* command) override;
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
	bool mergeWith(const QUndoCommand* command) override;

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
	bool mergeWith(const QUndoCommand* command) override;

private:
	DAGraphicsResizeableItem* mItem;
	qreal mOldWidth;
	qreal mNewWidth;
	qreal mHeight;
	bool mSkipfirst;
};

/**
 * @brief 改变高度
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
	bool mergeWith(const QUndoCommand* command) override;

private:
	DAGraphicsResizeableItem* mItem;
	qreal mOldHeight;
	qreal mNewHeight;
	qreal mWidth;
	bool mSkipfirst;
};

/**
 * @brief 旋转
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
	bool mergeWith(const QUndoCommand* command) override;

private:
	DAGraphicsResizeableItem* mItem;
	qreal mOldRotation;
	qreal mNewRotation;
	bool mSkipfirst;
};

/**
   @brief 分组命令
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemGrouping : public QUndoCommand
{
public:
	DACommandsForGraphicsItemGrouping(DAGraphicsScene* sc,
									  const QList< QGraphicsItem* >& groupingitems,
									  QUndoCommand* parent = nullptr);
	~DACommandsForGraphicsItemGrouping();
	void redo() override;
	void undo() override;
	// 这是一个清洗，要分组的item里面，如果存在item的parent在分组的item里，就驱除，这样不会分组嵌套，形成单一的层级
	static QList< QGraphicsItem* > toSimple(const QList< QGraphicsItem* >& groupingitems);
	QList< QGraphicsItem* > getWillGroupItems() const;

private:
	DAGraphicsScene* mScene;
	QList< QGraphicsItem* > mWillGroupItems;
	QGraphicsItemGroup* mGroupItem { nullptr };
	bool mNeedDelete { false };
};

/**
   @brief 解除分组命令

   @note 注意，QGraphicsItemGroup指针不要保存
 */
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemUngrouping : public QUndoCommand
{
public:
	DACommandsForGraphicsItemUngrouping(QGraphicsScene* sc, QGraphicsItemGroup* group, QUndoCommand* parent = nullptr);
	~DACommandsForGraphicsItemUngrouping();
	void redo() override;
	void undo() override;

private:
	QGraphicsScene* mScene;
	QList< QGraphicsItem* > mItems;
	QGraphicsItemGroup* mGroupItem { nullptr };
	bool mNeedDelete { false };
};

/**
 * @brief 改变GraphicsTextItem内容
 */
class DAGRAPHICSVIEW_API DACommandTextDocumentWrapper : public QUndoCommand
{
public:
	DACommandTextDocumentWrapper(QTextDocument* doc, QUndoCommand* parent = nullptr);
	~DACommandTextDocumentWrapper();
	virtual void redo() override;
	virtual void undo() override;

private:
	QPointer< QTextDocument > mDoc;
};
}
#endif  // DACOMMANDSFORGRAPHICS_H
