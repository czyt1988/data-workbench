#ifndef DAGRAPHICSCOMMANDSFACTORY_H
#define DAGRAPHICSCOMMANDSFACTORY_H
#include "DAGraphicsViewGlobal.h"
#include "DACommandsForGraphics.h"
class QGraphicsSceneMouseEvent;
class sceneMouseMoveEvent;
class sceneMouseReleaseEvent;
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
    DA_DECLARE_PRIVATE(DAGraphicsCommandsFactory)
public:
	DAGraphicsCommandsFactory();
	virtual ~DAGraphicsCommandsFactory();
    virtual DACommandsForGraphicsItemAdd* createItemAdd(QGraphicsItem* item);
    virtual DACommandsForGraphicsItemsAdd* createItemsAdd(const QList< QGraphicsItem* > its);
	virtual DACommandsForGraphicsItemRemove* createItemRemove(QGraphicsItem* item, QUndoCommand* parent = nullptr);
    virtual DACommandsForGraphicsItemsRemove* createItemsRemove(const QList< QGraphicsItem* > its);
	virtual DACommandsForGraphicsItemMoved* createItemMoved(QGraphicsItem* item,
                                                            const QPointF& start,
                                                            const QPointF& end,
                                                            bool skipfirst);
	virtual DACommandsForGraphicsItemMoved_Merge* createItemMoved_Merge(QGraphicsItem* item,
                                                                        const QPointF& start,
                                                                        const QPointF& end,
                                                                        bool skipfirst);
	virtual DACommandsForGraphicsItemsMoved* createItemsMoved(const QList< QGraphicsItem* >& items,
                                                              const QList< QPointF >& starts,
                                                              const QList< QPointF >& ends,
                                                              bool skipfirst);
    // 这个命令主要针对场景鼠标移动item进行设置
    virtual DACommandsForGraphicsItemsMoved* createItemsMoved(QGraphicsSceneMouseEvent* mouseReleaseEEvent);
	virtual DACommandsForGraphicsItemsMoved_Merge* createItemsMoved_Merge(const QList< QGraphicsItem* >& items,
                                                                          const QList< QPointF >& starts,
                                                                          const QList< QPointF >& ends,
                                                                          bool skipfirst);
	virtual DACommandsForGraphicsItemResized* createItemResized(DAGraphicsResizeableItem* item,
                                                                const QPointF& oldpos,
                                                                const QSizeF& oldSize,
                                                                const QPointF& newpos,
                                                                const QSizeF& newSize,
                                                                bool skipfirst = true);
	virtual DACommandsForGraphicsItemResized* createItemResized(DAGraphicsResizeableItem* item,
                                                                const QSizeF& oldSize,
                                                                const QSizeF& newSize);
	virtual DACommandsForGraphicsItemResizeWidth* createItemResizeWidth(DAGraphicsResizeableItem* item,
                                                                        const qreal& oldWidth,
                                                                        const qreal& newWidth);
	virtual DACommandsForGraphicsItemResizeHeight* createItemResizeHeight(DAGraphicsResizeableItem* item,
                                                                          const qreal& oldHeight,
                                                                          const qreal& newHeight);
	virtual DACommandsForGraphicsItemRotation* createItemRotation(DAGraphicsResizeableItem* item,
                                                                  const qreal& oldRotation,
                                                                  const qreal& newRotation);
    virtual DACommandsForGraphicsItemGrouping* createItemGrouping(const QList< QGraphicsItem* >& groupingitems);
    virtual DACommandsForGraphicsItemUngrouping* createItemUngrouping(QGraphicsItemGroup* group);
    DAGraphicsScene* scene() const;

protected:
    //=========================
    //! 工厂为何会存在如下函数：
    //! 原因是createItemMoved命令是针对鼠标移动元件产生的，
    //! DAGraphicsScene 场景在按下鼠标时记录元件的初始位置，场景释放鼠标时记录元件的最终位置,在场景的mouseReleaseEvent中，
    //! 生成DACommandsForGraphicsItemMoved命令，记录了这两个位置,形成命令，这样回退的时候，能回退到鼠标按下的位置。
    //! 但有些插件，需要记录除了这两个位置以外的信息，例如有个插件是作3d管道设计的，场景里除了元件的位置信息，还要记录元件的真实具体3d位置，
    //! 这样回退时，除了还原能还原出原来元件所在场景的位置，还要把真实具体3d位置的数据信息还原。
    //! 这时候，在mousePressEvent时就要介入，记录下具体元件的其它信息，原来这些操作是在scene的mousePressEvent实现的，用户也可以重写mousePressEvent，
    //! 但mousePressEvent除了处理回退命令，还要处理很多其它的操作，如果让用户为了这个命令的记录重写mousePressEvent，会多出很多没有用的工作，
    //! 因此把scene的mousePressEvent针对移动命令的操作移动到命令工厂中实现，如果用户需要针对这个命令进行特殊处理，可以重写sceneMousePressEvent和sceneMouseReleaseEvent即可
    //==========================
    // 鼠标点击事件
    virtual void sceneMousePressEvent(QGraphicsSceneMouseEvent* mouseEvent);
    // 鼠标移动事件
    virtual void sceneMouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent);
    // 鼠标释放
    virtual void sceneMouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent);

protected:
	void setScene(DAGraphicsScene* s);
};
}  // end DA
#endif  // DAGRAPHICSVIEWCOMMANDSFACTORY_H
