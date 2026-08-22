#include "DAGraphicsCommandsFactory.h"
#include "DAGraphicsScene.h"
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
namespace DA
{

class DAGraphicsCommandsFactory::PrivateData
{
public:
	DA_DECLARE_PUBLIC(DAGraphicsCommandsFactory)
public:
	PrivateData(DAGraphicsCommandsFactory* p);
	// 清空鼠标移动状态特征的标记信息
	void clearMouseMovementCycleState();
	DAGraphicsScene* scene { nullptr };
	QPointF sceneMousePressPos;         ///< 鼠标按下的场景位置
	QPointF sceneMouseReleasePos;       ///< 鼠标释放的场景位置
	bool isBeginMovingItems { false };  ///< 标记是否在移动
	bool isMouseMovementCycleComplete {
		false
	};  ///< 标记是否完成了一个完整的鼠标移动元件周期，所谓完整的鼠标移动元件周期，是指鼠标按下并选择了元件，同时鼠标拖动让元件形成位移，最后再松开鼠标
	QList< std::pair< QGraphicsItem*, QPointF > > movingItemsStartPos;
	QList< std::pair< QGraphicsItem*, QPointF > > movingItemsEndPos;  ///< 记录移动结束的位置
};

/**
 * @brief PrivateData 构造函数
 * @param p 指向公开类的指针
 */
DAGraphicsCommandsFactory::PrivateData::PrivateData(DAGraphicsCommandsFactory* p) : q_ptr(p)
{
}

/**
 * @brief 清空鼠标移动状态特征的标记信息
 *
 * 清空移动起始和结束位置列表，并重置移动状态标记
 */
void DAGraphicsCommandsFactory::PrivateData::clearMouseMovementCycleState()
{
	if (!movingItemsStartPos.empty()) {
		movingItemsStartPos.clear();
	}
	if (!movingItemsEndPos.empty()) {
		movingItemsEndPos.clear();
	}
	isBeginMovingItems           = false;
	isMouseMovementCycleComplete = false;
}

//----------------------------------------------------
// DAGraphicsCommandsFactory
//----------------------------------------------------
/**
 * @brief 构造函数
 */
DAGraphicsCommandsFactory::DAGraphicsCommandsFactory() : DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构函数
 */
DAGraphicsCommandsFactory::~DAGraphicsCommandsFactory()
{
}

/**
 * @brief 创建添加图元的撤销命令
 * @param item 要添加的图元
 * @return 添加图元的撤销命令指针
 */
DACommandsForGraphicsItemAdd* DAGraphicsCommandsFactory::createItemAdd(QGraphicsItem* item)
{
	return new DACommandsForGraphicsItemAdd(item, scene());
}

/**
 * @brief 创建批量添加图元的撤销命令
 * @param its 要添加的图元列表
 * @return 批量添加图元的撤销命令指针
 */
DACommandsForGraphicsItemsAdd* DAGraphicsCommandsFactory::createItemsAdd(const QList< QGraphicsItem* > its)
{
	return new DACommandsForGraphicsItemsAdd(its, scene());
}

/**
 * @brief 创建移除图元的撤销命令
 * @param item 要移除的图元
 * @param parent 父撤销命令
 * @return 移除图元的撤销命令指针
 */
DACommandsForGraphicsItemRemove* DAGraphicsCommandsFactory::createItemRemove(QGraphicsItem* item, QUndoCommand* parent)
{
	return new DACommandsForGraphicsItemRemove(item, scene(), parent);
}

/**
 * @brief 创建批量移除图元的撤销命令
 * @param its 要移除的图元列表
 * @return 批量移除图元的撤销命令指针
 */
DACommandsForGraphicsItemsRemove* DAGraphicsCommandsFactory::createItemsRemove(const QList< QGraphicsItem* > its)
{
	return new DACommandsForGraphicsItemsRemove(its, scene());
}

/**
 * @brief 创建图元移动的撤销命令
 * @param item 要移动的图元
 * @param start 移动起始位置
 * @param end 移动结束位置
 * @param skipfirst 是否跳过第一次执行
 * @return 图元移动的撤销命令指针
 */
DACommandsForGraphicsItemMoved*
DAGraphicsCommandsFactory::createItemMoved(QGraphicsItem* item, const QPointF& start, const QPointF& end, bool skipfirst)
{
    return new DACommandsForGraphicsItemMoved(item, start, end, skipfirst);
}

/**
 * @brief 创建批量图元移动的撤销命令
 * @param items 要移动的图元列表
 * @param starts 移动起始位置列表
 * @param ends 移动结束位置列表
 * @param skipfirst 是否跳过第一次执行
 * @return 批量图元移动的撤销命令指针
 */
DACommandsForGraphicsItemsMoved* DAGraphicsCommandsFactory::createItemsMoved(const QList< QGraphicsItem* >& items,
                                                                             const QList< QPointF >& starts,
                                                                             const QList< QPointF >& ends,
                                                                             bool skipfirst)
{
    return new DACommandsForGraphicsItemsMoved(items, starts, ends, skipfirst);
}

/**
 * @brief 针对鼠标移动的移动命令
 * @param mouseReleaseEEvent
 * @return
 * @note 注意，此函数会返回nullptr，代表没有需要推入的命令
 */
DACommandsForGraphicsItemsMoved* DAGraphicsCommandsFactory::createItemsMoved()
{
	DA_D(d);
	if (!d->isMouseMovementCycleComplete) {
		// 没有完成一个完整的鼠标移动生命周期
		return nullptr;
	}
	// 说明完成了一个完整的鼠标移动生命周期，创建命令
	if (d->movingItemsStartPos.empty()) {
		// 异常情况，记录
		qDebug() << "Mouse Movement Cycle Complete,but not record any items";
		d->clearMouseMovementCycleState();
		return nullptr;
	}
	if (d->movingItemsEndPos.empty()) {
		// 特殊情况，说明触发了鼠标点击和移动，但没有触发鼠标释放，一般是鼠标点击item，然后鼠标移出scene范围再释放导致的，这时补全末端位置
        for (const auto& pair : std::as_const(d->movingItemsStartPos)) {
			d->movingItemsEndPos.append(std::make_pair(pair.first, pair.first->pos()));
		}
	}

	if (d->movingItemsEndPos.size() != d->movingItemsStartPos.size()) {
		// 异常，不对其
		qDebug() << "Mouse Movement Cycle Complete,but record pressed items and release items is not same";
		d->clearMouseMovementCycleState();
		return nullptr;
	}
	QList< QGraphicsItem* > items;
	QList< QPointF > startPos;
	QList< QPointF > endsPos;

	for (int i = 0; i < d->movingItemsStartPos.size(); ++i) {
		items.push_back(d->movingItemsStartPos[ i ].first);
		startPos.push_back(d->movingItemsStartPos[ i ].second);
		endsPos.push_back(d->movingItemsEndPos[ i ].second);
	}
	return createItemsMoved(items, startPos, endsPos, true);
}

DACommandsForGraphicsItemResized* DAGraphicsCommandsFactory::createItemResized(DAIResizableGraphicsItem* item,
                                                                               const QPointF& oldpos,
                                                                               const QSizeF& oldSize,
                                                                               const QPointF& newpos,
                                                                               const QSizeF& newSize,
                                                                               bool skipfirst)
{
    return new DACommandsForGraphicsItemResized(item, oldpos, oldSize, newpos, newSize, skipfirst);
}

DACommandsForGraphicsItemResized* DAGraphicsCommandsFactory::createItemResized(DAIResizableGraphicsItem* item,
                                                                               const QSizeF& oldSize,
                                                                               const QSizeF& newSize)
{
    return new DACommandsForGraphicsItemResized(item, oldSize, newSize);
}

DACommandsForGraphicsItemResizeWidth* DAGraphicsCommandsFactory::createItemResizeWidth(DAIResizableGraphicsItem* item,
                                                                                       const qreal& oldWidth,
                                                                                       const qreal& newWidth)
{
    return new DACommandsForGraphicsItemResizeWidth(item, oldWidth, newWidth);
}

DACommandsForGraphicsItemResizeHeight* DAGraphicsCommandsFactory::createItemResizeHeight(DAIResizableGraphicsItem* item,
                                                                                         const qreal& oldHeight,
                                                                                         const qreal& newHeight)
{
    return new DACommandsForGraphicsItemResizeHeight(item, oldHeight, newHeight);
}

DACommandsForGraphicsItemRotation* DAGraphicsCommandsFactory::createItemRotation(DAIResizableGraphicsItem* item,
                                                                                 const qreal& oldRotation,
                                                                                 const qreal& newRotation,
                                                                                 bool skipfirst)
{
    return new DACommandsForGraphicsItemRotation(item, oldRotation, newRotation, skipfirst);
}

/**
 * @brief 创建图元组合的撤销命令
 * @param groupingitems 要组合的图元列表
 * @return 图元组合的撤销命令指针
 */
DACommandsForGraphicsItemGrouping* DAGraphicsCommandsFactory::createItemGrouping(const QList< QGraphicsItem* >& groupingitems)
{
    return new DACommandsForGraphicsItemGrouping(scene(), groupingitems);
}

/**
 * @brief 创建图元取消组合的撤销命令
 * @param group 要取消组合的图元组
 * @return 图元取消组合的撤销命令指针
 */
DACommandsForGraphicsItemUngrouping* DAGraphicsCommandsFactory::createItemUngrouping(QGraphicsItemGroup* group)
{
    return new DACommandsForGraphicsItemUngrouping(scene(), group);
}

/**
 * @brief 处理场景鼠标按下事件
 *
 * 记录鼠标按下位置，判断是否开始移动图元，并记录选中可移动图元的起始位置
 * @param mouseEvent 鼠标事件
 */
void DAGraphicsCommandsFactory::sceneMousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (!mouseEvent) {
		return;
	}
	DA_D(d);
	// 鼠标点击后清空
	d->clearMouseMovementCycleState();
	d->sceneMousePressPos = mouseEvent->scenePos();
	if (mouseEvent->buttons().testFlag(Qt::LeftButton)) {
		// 要判断当前点击下去之后是否是移动状态，有些情况，虽然选中了item，但鼠标点击下去并不是移动状态，例如拉一个选择框的过程
		//!  1.记录选中的所有图元，如果点击的是改变尺寸的点，这个就不执行记录
		DAGraphicsScene* sc    = scene();
		if (!sc) {
			return;
		}
		QGraphicsItem* positem = sc->itemAt(d->sceneMousePressPos, QTransform());
		if (!sc->isItemCanMove(positem, d->sceneMousePressPos)) {
			d->isBeginMovingItems = false;
			return;
		}
		QList< QGraphicsItem* > mits = d->scene->getSelectedMovableItems();
        for (QGraphicsItem* its : std::as_const(mits)) {
			d->movingItemsStartPos.append(std::make_pair(its, its->pos()));
		}
		// 只要mits不为空，说明开始移动
		d->isBeginMovingItems = !mits.empty();
	}
}

/**
 * @brief 处理场景鼠标移动事件
 *
 * 根据是否开始移动图元来标记鼠标移动周期是否完成
 * @param mouseEvent 鼠标事件
 */
void DAGraphicsCommandsFactory::sceneMouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	Q_UNUSED(mouseEvent);
	if (d_ptr->isBeginMovingItems) {
		d_ptr->isMouseMovementCycleComplete = true;
	} else {
		d_ptr->isMouseMovementCycleComplete = false;
	}
}

/**
 * @brief 处理场景鼠标释放事件
 *
 * 记录鼠标释放位置，判断是否形成完整的鼠标移动周期，并记录图元结束位置
 * @param mouseEvent 鼠标事件
 */
void DAGraphicsCommandsFactory::sceneMouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (!mouseEvent) {
		return;
	}
	DA_D(d);
	d->sceneMouseReleasePos = mouseEvent->scenePos();
	if (d->isBeginMovingItems) {
		// 此时形成了一个完整的鼠标移动命令周期,但还得看看是否是点击下去后又抬起这种没有任何位移的情况
		d->isBeginMovingItems = false;
		if (d->movingItemsStartPos.isEmpty()) {
			// 正常不会进入
			d->clearMouseMovementCycleState();
			return;
		}
		if (qFuzzyCompare(d->sceneMouseReleasePos.x(), d->sceneMousePressPos.x())
			&& qFuzzyCompare(d->sceneMouseReleasePos.y(), d->sceneMousePressPos.y())) {
			// 位置相等，说明是点击了元件后又松开鼠标
			d->clearMouseMovementCycleState();
			return;
		}
		// 说明有位移，开始记录结束的位置信息
		d->movingItemsEndPos.clear();
        for (const auto& pair : std::as_const(d->movingItemsStartPos)) {
			d->movingItemsEndPos.push_back(std::make_pair(pair.first, pair.first->pos()));
		}
		d->isMouseMovementCycleComplete = true;
	}
}

/**
 * @brief 记录场景鼠标左键按下的位置
 * @return
 */
QPointF DAGraphicsCommandsFactory::sceneMousePressPos() const
{
    return d_ptr->sceneMousePressPos;
}

/**
 * @brief 这是记录scene移动鼠标时记录的移动的item
 *
 * 这个函数仅仅在DACommandsForGraphicsItemsMoved* createItemsMoved(QGraphicsSceneMouseEvent* mouseReleaseEEvent)使用
 *
 * 用来对鼠标移动的item进行回退
 * @return
 */
const QList< std::pair< QGraphicsItem*, QPointF > >& DAGraphicsCommandsFactory::movingItemsStartPos() const
{
    return d_ptr->movingItemsStartPos;
}

/**
 * @brief 这是记录scene移动item时鼠标释放时记录的item信息
 * @return
 */
const QList< std::pair< QGraphicsItem*, QPointF > >& DAGraphicsCommandsFactory::movingItemsEndPos() const
{
    return d_ptr->movingItemsEndPos;
}

/**
 * @brief 标记是否完成了一个完整的鼠标移动元件周期，所谓完整的鼠标移动元件周期，是指鼠标按下并选择了元件，同时鼠标拖动让元件形成位移，最后再松开鼠标
 *
 * 只有完整的鼠标移动元件周期，才应该产生元件移动命令
 * @return
 */
bool DAGraphicsCommandsFactory::isMouseMovementCycleComplete() const
{
    return d_ptr->isMouseMovementCycleComplete;
}

/**
 * @brief 这个函数用来判断是否开始进入移动状态
 * @return
 */
bool DAGraphicsCommandsFactory::isBeginMovingItems() const
{
    return d_ptr->isBeginMovingItems;
}

/**
 * @brief 重置鼠标移动的循环状态，这个函数是重写sceneMouseReleaseEvent等函数时要重置状态用
 */
void DAGraphicsCommandsFactory::resetMouseMovementCycleState()
{
    d_ptr->clearMouseMovementCycleState();
}

/**
 * @brief 设置关联的场景
 * @param s 关联的场景指针
 * @sa scene()
 */
void DAGraphicsCommandsFactory::setScene(DAGraphicsScene* s)
{
    d_ptr->scene = s;
}

/**
 * @brief 获取关联的场景
 * @return 关联的场景指针
 * @sa setScene()
 */
DAGraphicsScene* DAGraphicsCommandsFactory::scene() const
{
    return d_ptr->scene;
}

}  // end DA
