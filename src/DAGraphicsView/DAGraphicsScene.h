#ifndef DAGRAPHICSSCENE_H
#define DAGRAPHICSSCENE_H
#include "DAGraphicsViewGlobal.h"
#include <QGraphicsScene>
#include <QUndoStack>
#include <QImage>
#include <QTextDocument>
namespace DA
{
class DAGraphicsResizeableItem;
class DAGraphicsItem;
class DAGraphicsLinkItem;
class DAGraphicsItemGroup;
class DAAbstractGraphicsSceneAction;
class DAGraphicsLayout;
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
class DAGRAPHICSVIEW_API DAGraphicsScene : public QGraphicsScene
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAGraphicsScene)
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
	DAGraphicsScene(QObject* p = nullptr);
	DAGraphicsScene(const QRectF& sceneRect, QObject* p = nullptr);
	DAGraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject* p = nullptr);
	~DAGraphicsScene();
	// 判断是否正在移动item
	bool isMovingItems() const;
	// 获取当前鼠标在scene的位置
	QPointF getCurrentMouseScenePos() const;
	// 获取最后鼠标在scene点击的位置
	QPointF getLastMousePressScenePos() const;
	// 获取选中且能移动的item
	QList< QGraphicsItem* > getSelectedMovableItems();
	// 等同additem，但使用redo/undo来添加，可以进行redo/undo操作
	QUndoCommand* addItem_(QGraphicsItem* item);
	QUndoCommand* addItems_(const QList< QGraphicsItem* >& its);
	// 等同removeItem，但使用redo/undo来添加，可以进行redo/undo操作
	QUndoCommand* removeItem_(QGraphicsItem* item);
	QUndoCommand* removeItems_(const QList< QGraphicsItem* >& its);
	// 导出为pixmap,dpi=0代表不考虑dpi
	QPixmap toPixamp(int dpi = 0);
	// 保存为图片
	QImage toImage(int dpi = 0);
	// 链接模式
	void beginLink(DAGraphicsLinkItem* linkItem, LinkMode lm = LinkModeAutoStartEndFollowMouseClick);
	// 判断当前是否是链接模式
	bool isStartLink() const;
	// 结束链接模式
	void endLink();
	// 取消链接模式
	virtual void cancelLink();
	// 取消
	virtual void cancel();
	// 获取当前正在进行连线的连接线item
	DAGraphicsLinkItem* getCurrentLinkItem() const;
	// 设置忽略链接事件的处理，主要忽略mousePressEvent，mouseMoveEvent的链接事件，
	// 这个函数一般是在子类中的重载函数中调用，用于进行一些特殊处理需要暂时屏蔽掉链接事件
	void setIgnoreLinkEvent(bool on);
	bool isIgnoreLinkEvent() const;
	// 选中的item进行分组，支持redo/undo
	void groupingSelectItems_();
	// 移除选中的分组
	void removeSelectItemGroup_();
	// 是否允许对齐网格
	bool isEnableSnapToGrid() const;
	// 是否显示网格
	bool isShowGridLine();
	// 设置网格尺寸
	void setGridSize(const QSize& gs);
	QSize getGridSize() const;
	// 网格画笔
	void setGridLinePen(const QPen& p);
	QPen getGridLinePen() const;
	// 设置绘制背景使用缓冲
	void setPaintBackgroundInCache(bool on);
	bool isPaintBackgroundInCache() const;

	// 回退栈操作
	QUndoStack& undoStack();
	const QUndoStack& undoStack() const;
	QUndoStack* getUndoStack() const;
	void setUndoStackActive();
	void push(QUndoCommand* cmd);

	// 通过id查找item,此函数性能为O(n)
	QGraphicsItem* findItemByID(uint64_t id, bool recursion = false) const;
	static QGraphicsItem* findItemByID(const QList< QGraphicsItem* >& its, uint64_t id, bool recursion = false);

	// 返回所有顶层的item
	QList< QGraphicsItem* > topItems() const;
	QList< QGraphicsItem* > topItems(const QPointF& scenePos) const;
	// 选中的item
	QList< DAGraphicsItem* > selectedDAItems() const;
	// 设置场景就绪，如果场景还没加载完成，ready为false，一般这个函数在工程加载的时候应用
	void setReady(bool on);
	bool isReady() const;

	// 激活一个场景动作，DAAbstractGraphicsSceneAction的内存归scene管理
	void setupSceneAction(DAAbstractGraphicsSceneAction* act);
	// 是否当前存在场景动作
	bool isHaveSceneAction() const;
	// 清除场景动作
	void clearSceneAction();
	// 获取图层
	QList< DAGraphicsLayout* > getLayouts() const;
	// 是否为只读模式，只读模式不能编辑
	bool isReadOnly() const;

public:
	// 获取默认的dpi
	static int getDefaultDPI();
	// dpi转为像素
	static int dpiToPx(int dpi, int r);
	// 把item添加到分组
	static void addItemToGroup(QGraphicsItemGroup* group, const QList< QGraphicsItem* >& willGroupItems);
public slots:
	// 设置对齐网格
	void setEnableSnapToGrid(bool on = true);
	// 显示网格
	void showGridLine(bool on);
	// 选中所有item
	void selectAll();
	// 取消选中
	void clearSelection();
	// 设置item的选中状态
	int setSelectionState(const QList< QGraphicsItem* >& its, bool isSelect);
	// 锁定,具体的锁定，有item自行处理，scene只是持有只读的状态，一般item在itemChange中判断是否只读，然后进行动作判断
	void setReadOnly(bool on);
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
	void itemsPositionChanged(const QList< QGraphicsItem* >& items,
							  const QList< QPointF >& oldPos,
							  const QList< QPointF >& newPos);

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
	void linkCompleted(DAGraphicsLinkItem* linkItem);

	/**
	   @brief 选中的item发生了变化，注意，选中的如果是分组，会检查分组内部的item的点击，最终也是发送被点击的item
	   @param item
	 */
	void selectItemChanged(DAGraphicsItem* item);

	/**
	   @brief 选中的链接线发生了改变
	   @param item
	 */
	void selectLinkChanged(DAGraphicsLinkItem* item);

	/**
	 * @brief item添加的信号
	 *
	 * @note 此信号是通过@ref DAGraphicsScene::addItem_ 或是@ref DAGraphicsScene::addItems_ 函数才会触发，
	 * 直接调用@ref QGraphicsScene::addItem 函数不会触发此函数
	 *
	 * @param item
	 */
	void itemsAdded(const QList< QGraphicsItem* >& its);

	/**
	 * @brief item移除的信号
	 *
	 * @note 此信号是通过@ref DAGraphicsScene::removeItem_ 或是@ref DAGraphicsScene::removeItems_ 函数才会触发，
	 * 直接调用@ref QGraphicsScene::removeItem 函数不会触发此函数
	 *
	 * @param item
	 */
	void itemsRemoved(const QList< QGraphicsItem* >& its);

protected:
	// 判断点击的item是否可以移动
	virtual bool isItemCanMove(QGraphicsItem* positem, const QPointF& scenePos);
	// 调用此函数 主动触发itemsPositionChanged信号，这个函数用于 继承此类例如实现了键盘移动item，主动触发此信号
	void emitItemsPositionChanged(const QList< QGraphicsItem* >& items,
								  const QList< QPointF >& oldPos,
								  const QList< QPointF >& newPos);
	// 调用此函数 主动触发itemBodySizeChanged信号
	void emitItemBodySizeChanged(DAGraphicsResizeableItem* item, const QSizeF& oldSize, const QSizeF& newSize);
	// 调用此函数 主动触发itemRotationed信号
	void emitItemRotationChanged(DAGraphicsResizeableItem* item, const qreal& rotation);

protected:
	// 鼠标点击事件
	void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
	// 鼠标移动事件
	void mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
	// 鼠标释放
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
	// 绘制背景
	void drawBackground(QPainter* painter, const QRectF& rect) override;
private slots:
	//
	void onSelectionChanged();

private:
	void init();
	void checkSelectItem(QGraphicsItem* item);
	int changeAllSelection(bool setSelect);
};
}
#endif  // DAGRAPHICSSCENEWITHUNDOSTACK_H
