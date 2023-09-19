#ifndef DAGRAPHICSSCENEWITHUNDOSTACK_H
#define DAGRAPHICSSCENEWITHUNDOSTACK_H
#include "DAGraphicsViewGlobal.h"
#include <QGraphicsScene>
#include <QUndoStack>
#include "DAGraphicsLinkItem.h"
namespace DA
{
class DAGraphicsResizeableItem;
/**
 * @brief 这是带着undostack的GraphicsScene
 * 此QGraphicsScene支持：
 *
 * - item移动的undo/redo
 * - item缩放的undo/redo item需要继承@sa DAGraphicsResizeableItem
 * - item添加删除的undo/redo
 *
 * @note 能支持redo/undo操作的函数后面以“_”结尾
 */
class DAGRAPHICSVIEW_API DAGraphicsSceneWithUndoStack : public QGraphicsScene
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAGraphicsSceneWithUndoStack)
    friend class DAGraphicsResizeableItem;

public:
    /**
     * @brief 链接模式
     */
    enum LinkMode
    {
        ///< The beginning is the current mouse position, the end follows the mouse movement,
        /// and the connection ends when the next left mouse button is clicked
        /// 开端为当前鼠标位置，末端跟随鼠标移动，在下个鼠标左键点击时结束连线
        LinkModeAutoStartEndFollowMouseClick
    };

public:
    DAGraphicsSceneWithUndoStack(QObject* p = nullptr);
    DAGraphicsSceneWithUndoStack(const QRectF& sceneRect, QObject* p = nullptr);
    DAGraphicsSceneWithUndoStack(qreal x, qreal y, qreal width, qreal height, QObject* p = nullptr);
    ~DAGraphicsSceneWithUndoStack();
    //判断是否正在移动item
    bool isMovingItems() const;
    // 获取当前鼠标在scene的位置
    QPointF getCurrentMouseScenePos() const;
    //获取选中且能移动的item
    QList< QGraphicsItem* > getSelectedMovableItems();
    //等同additem，但使用redo/undo来添加，可以进行redo/undo操作
    QUndoCommand* addItem_(QGraphicsItem* item, bool autopush = true);
    //等同removeItem，但使用redo/undo来添加，可以进行redo/undo操作
    QUndoCommand* removeItem_(QGraphicsItem* item, bool autopush = true);
    //导出为pixmap
    QPixmap toPixamp();
    //链接模式
    void beginLink(DAGraphicsLinkItem* linkItem, LinkMode lm = LinkModeAutoStartEndFollowMouseClick);
    //判断当前是否是链接模式
    bool isStartLink() const;
    //结束链接模式
    void endLink();
    //取消链接模式
    void cancelLink();
    //获取当前正在进行连线的连接线item
    DAGraphicsLinkItem* getCurrentLinkItem() const;

public:
    //是否允许对齐网格
    bool isEnableSnapToGrid() const;
    //是否显示网格
    bool isShowGridLine();
    //设置网格尺寸
    void setGridSize(const QSize& gs);
    QSize getGridSize() const;
    //网格画笔
    void setGridLinePen(const QPen& p);
    QPen getGridLinePen() const;
    //设置绘制背景使用缓冲
    void setPaintBackgroundInCache(bool on);
    bool isPaintBackgroundInCache() const;

public slots:
    //设置对齐网格
    void setEnableSnapToGrid(bool on = true);
    //显示网格
    void showGridLine(bool on);
    //选中所有item
    void selectAll();
signals:
    /**
     * @brief item移动发射的信号
     *
     * 此信号只针对鼠标移动导致的item位置改变
     *
     * @note 这个信号相比objectitem的信号发送频率低很多，只有在鼠标释放的时候会发出
     * @param item
     * @param oldPos
     * @param newPos
     */
    void itemsPositionChanged(const QList< QGraphicsItem* >& items, const QList< QPointF >& oldPos, const QList< QPointF >& newPos);

    /**
     * @brief 条目bodysize改变触发的信号
     * @param item
     * @param rotation
     */
    void itemBodySizeChanged(DAGraphicsResizeableItem* item, const QSizeF& oldSize, const QSizeF& newSize);

    /**
     * @brief item旋转发出的信号
     * @param item
     * @param rotation
     */
    void itemRotationChanged(DAGraphicsResizeableItem* item, const qreal& rotation);

    /**
     * @brief 完成了一次链接
     * @param linkItem 通过此指针可以获取两个连接点
     */
    void completeLink(DAGraphicsLinkItem* linkItem);

public:
    //回退栈操作
    QUndoStack& undoStack();
    const QUndoStack& undoStack() const;
    QUndoStack* getUndoStack() const;
    void setUndoStackActive();
    void push(QUndoCommand* cmd);

protected:
    //判断点击的item是否可以移动
    virtual bool isItemCanMove(QGraphicsItem* positem, const QPointF& scenePos);
    //调用此函数 主动触发itemsPositionChanged信号，这个函数用于 继承此类例如实现了键盘移动item，主动触发此信号
    void emitItemsPositionChanged(const QList< QGraphicsItem* >& items, const QList< QPointF >& oldPos, const QList< QPointF >& newPos);
    //调用此函数 主动触发itemBodySizeChanged信号
    void emitItemBodySizeChanged(DAGraphicsResizeableItem* item, const QSizeF& oldSize, const QSizeF& newSize);
    //调用此函数 主动触发itemRotationed信号
    void emitItemRotationChanged(DAGraphicsResizeableItem* item, const qreal& rotation);

protected:
    //鼠标点击事件
    void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
    //鼠标移动事件
    void mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
    //鼠标释放
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
    //绘制背景
    void drawBackground(QPainter* painter, const QRectF& rect) override;
};
}
#endif  // DAGRAPHICSSCENEWITHUNDOSTACK_H
