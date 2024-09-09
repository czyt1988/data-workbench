#include "DAGraphicsScene.h"
#include <QGraphicsSceneMouseEvent>
#include "DACommandsForGraphics.h"
#include "DAGraphicsResizeableItem.h"
#include "DAGraphicsLinkItem.h"
#include "DAGraphicsItemGroup.h"
#include "DAGraphicsStandardTextItem.h"
#include "DAAbstractGraphicsSceneAction.h"
#include "DAGraphicsLayout.h"
#include <QPainter>
#include <QDebug>
#include <QApplication>
#include <QScreen>

namespace DA
{

class _DAGraphicsSceneItemMoveingInfos
{
public:
	QList< QGraphicsItem* > items;
	QList< QPointF > startsPos;
	QList< QPointF > endsPos;
	// 添加开始的位置
	void appendStartPos(QGraphicsItem* item, const QPointF& start);
	void appendStartPos(QGraphicsItem* item);
	// 刷新结束位置
	void updateEndPos();
	// 清除信息
	void clear();
};

//===================================================
// DAGraphicsSceneWithUndoStack::PrivateData
//===================================================

class DAGraphicsScene::PrivateData
{
	DA_DECLARE_PUBLIC(DAGraphicsScene)
public:
	PrivateData(DAGraphicsScene* p);
	// 绘制背景缓存
	void renderBackgroundCache();
	void renderBackgroundCache(QPainter* painter, const QRect& rect);

public:
	QUndoStack mUndoStack;
	bool mIsMovingItems { false };  /// 正在进行item的移动
	QPointF mLastMouseScenePos;
	QPointF mLastMousePressScenePos;                  ///< 记录点击时的位置
	_DAGraphicsSceneItemMoveingInfos mMovingInfos;    ///< 记录移动的信息
	bool mEnableSnapToGrid { false };                 ///< 允许对齐网格
	bool mShowGridLine { true };                      ///< 是否显示网格
	QSize mGridSize { 10, 10 };                       ///< 网格大小
	QPen mGridLinePen;                                ///< 绘制网格的画笔
	QPixmap mBackgroundCache;                         ///< 背景缓存，不用每次都绘制
	bool mIsPaintBackgroundInCache { false };         ///< 背景使用缓冲绘制
	std::unique_ptr< DAGraphicsLinkItem > mLinkItem;  ///< 连接线
	DAGraphicsScene::LinkMode mLinkMode { DAGraphicsScene::LinkModeAutoStartEndFollowMouseClick };  ///< 当前链接模式记录
	///< 标记连接线移动过，这个变量是在LinkModeAutoStartEndFollowMouseClick模式下，
	/// 用户调用beginLink函数后，有可能接下来就马上触发mousePressedEvent而结束链接，因此，在LinkModeAutoStartEndFollowMouseClick模式下
	/// beginLink函数调用时会把mLinkItemIsMoved设置为false，只有接收到mouseMove事件后，此变量变为true，在mousePressedEvent才会进行结束判断
	bool mLinkItemIsMoved { false };
	bool mIsIgnoreLinkEvent { false };  ///< 设置忽略链接事件的处理，主要忽略mousePressEvent，mouseMoveEvent的链接事件
	bool mIsReady { true };              ///< 场景是否就绪标记，此参数不保存
	QList< DAGraphicsLayout* > mLayout;  ///< 保存所有的图层
	std::unique_ptr< DAAbstractGraphicsSceneAction > mSceneAction;
	bool mIsReadOnlyMode { false };  ///< 是否为只读状态
};

////////////////////////////////////////////////
/// _DANodeGraphicsSceneItemMoveingInfos
////////////////////////////////////////////////

/**
 * @brief 添加开始的位置
 * @param item
 * @param start
 */
void _DAGraphicsSceneItemMoveingInfos::appendStartPos(QGraphicsItem* item, const QPointF& start)
{
	items.append(item);
	startsPos.append(start);
}

/**
 * @brief 刷新结束位置
 */
void _DAGraphicsSceneItemMoveingInfos::appendStartPos(QGraphicsItem* item)
{
	items.append(item);
	startsPos.append(item->pos());
}

void _DAGraphicsSceneItemMoveingInfos::updateEndPos()
{
	for (QGraphicsItem* i : qAsConst(items)) {
		endsPos.append(i->pos());
	}
}

/**
 * @brief 清除信息
 */
void _DAGraphicsSceneItemMoveingInfos::clear()
{
	items.clear();
	startsPos.clear();
	endsPos.clear();
}

////////////////////////////////////////////////
//==============================
// DAGraphicsSceneWithUndoStackPrivate
// 实现
//==============================
////////////////////////////////////////////////

DAGraphicsScene::PrivateData::PrivateData(DAGraphicsScene* p) : q_ptr(p)
{
	mGridLinePen.setStyle(Qt::SolidLine);
	mGridLinePen.setColor(QColor(219, 219, 219));
	mGridLinePen.setCapStyle(Qt::RoundCap);
	mGridLinePen.setWidthF(0.5);
}

void DAGraphicsScene::PrivateData::renderBackgroundCache()
{
	if (!mShowGridLine) {
		return;
	}
	QRectF sr = q_ptr->sceneRect();
	if (!sr.isValid()) {
		qDebug() << "sceneRect is invalid";
		return;
	}
	QRect scr = sr.toRect();
	QPixmap pixmap(scr.width(), scr.height());
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing, true);
	renderBackgroundCache(&painter, pixmap.rect());
	mBackgroundCache = std::move(pixmap);
}

/**
 * @brief 渲染背景缓存
 */
void DAGraphicsScene::PrivateData::renderBackgroundCache(QPainter* painter, const QRect& rect)
{
	painter->setPen(mGridLinePen);
	qreal left = int(rect.left()) - (int(rect.left()) % mGridSize.width());
	qreal top  = int(rect.top()) - (int(rect.top()) % mGridSize.height());

	for (qreal x = left; x < rect.right(); x += mGridSize.width()) {
		painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
	}
	// 绘制横线
	for (qreal y = top; y < rect.bottom(); y += mGridSize.height()) {
		painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
	}
}

//===============================================================
// DAGraphicsSceneWithUndoStack
//===============================================================

DAGraphicsScene::DAGraphicsScene(QObject* p) : QGraphicsScene(p), DA_PIMPL_CONSTRUCT
{
	init();
}

DAGraphicsScene::DAGraphicsScene(const QRectF& sceneRect, QObject* p) : QGraphicsScene(sceneRect, p), DA_PIMPL_CONSTRUCT
{
    init();
}

DAGraphicsScene::DAGraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject* p)
    : QGraphicsScene(x, y, width, height, p), DA_PIMPL_CONSTRUCT
{
    init();
}

void DAGraphicsScene::init()
{
    connect(this, &QGraphicsScene::selectionChanged, this, &DAGraphicsScene::onSelectionChanged);
}

DAGraphicsScene::~DAGraphicsScene()
{
}

/**
 * @brief 判断是否正在移动item
 * @return
 */
bool DAGraphicsScene::isMovingItems() const
{
    return d_ptr->mIsMovingItems;
}

/**
 * @brief 获取当前鼠标在scene的位置
 * @return
 */
QPointF DAGraphicsScene::getCurrentMouseScenePos() const
{
    return (d_ptr->mLastMouseScenePos);
}

/**
 * @brief 获取最后鼠标在scene点击的位置
 *
 * @note 如果子类重载了mousePressEvent，必须调用DAGraphicsSceneWithUndoStack::mousePressEvent(mouseEvent);此函数才会生效
 * @return
 */
QPointF DAGraphicsScene::getLastMousePressScenePos() const
{
    return (d_ptr->mLastMousePressScenePos);
}

/**
 * @brief 获取选中且能移动的item
 * @return
 */
QList< QGraphicsItem* > DAGraphicsScene::getSelectedMovableItems()
{
	QList< QGraphicsItem* > res;
	QList< QGraphicsItem* > its = selectedItems();
	for (QGraphicsItem* it : qAsConst(its)) {
		if (it->flags().testFlag(QGraphicsItem::ItemIsMovable)) {
			res.append(it);
		}
	}
	return res;
}

/**
 * @brief 等同additem，但使用redo/undo来添加，可以进行redo/undo操作
 * @param item
 * @return 返回执行的命令
 */
QUndoCommand* DAGraphicsScene::addItem_(QGraphicsItem* item)
{
	DA::DACommandsForGraphicsItemAdd* cmd = new DA::DACommandsForGraphicsItemAdd(item, this);
	push(cmd);
	emit itemsAdded({ item });
	return cmd;
}

/**
 * @brief 等同多次additem，但使用redo/undo来添加，可以进行redo/undo操作
 * @param item
 * @return 返回执行的命令
 */
QUndoCommand* DAGraphicsScene::addItems_(const QList< QGraphicsItem* >& its)
{
	DA::DACommandsForGraphicsItemsAdd* cmd = new DA::DACommandsForGraphicsItemsAdd(its, this);
	push(cmd);
	emit itemsAdded(its);
	return cmd;
}

/**
 * @brief 等同removeItem，但使用redo/undo来添加，可以进行redo/undo操作
 * @param item
 * @param autopush 自动推入redo/undo栈，对于无需操作返回的cmd，此值需要设置为true，否则需要手动调用push函数，把返回的cmd推入
 * @return
 */
QUndoCommand* DAGraphicsScene::removeItem_(QGraphicsItem* item)
{
	DA::DACommandsForGraphicsItemRemove* cmd = new DA::DACommandsForGraphicsItemRemove(item, this);
	push(cmd);
	emit itemsRemoved({ item });
	return cmd;
}

QUndoCommand* DAGraphicsScene::removeItems_(const QList< QGraphicsItem* >& its)
{
	DA::DACommandsForGraphicsItemsRemove* cmd = new DA::DACommandsForGraphicsItemsRemove(its, this);
	push(cmd);
	emit itemsRemoved(its);
	return cmd;
}

/**
 * @brief 导出为pixmap
 * @return
 */
QPixmap DAGraphicsScene::toPixamp(int dpi)
{
	QRectF br = itemsBoundingRect();
	if (dpi > 0) {
		br.setWidth(dpiToPx(dpi, br.width()));
		br.setHeight(dpiToPx(dpi, br.height()));
	}
	QPixmap res(br.size().toSize() + QSize(10, 10));
	res.fill(Qt::transparent);
	QPainter painter(&res);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	// 设置DPI，虽然这对最终的图像质量没有直接影响，但可能影响渲染过程中的一些细节

	render(&painter, QRectF(10, 10, br.width(), br.height()), br);
	return res;
}

/**
 * @brief 转换为设备相关的图片
 * @param dpi
 * @return
 */
QImage DAGraphicsScene::toImage(int dpi)
{
	QRectF br = itemsBoundingRect();
	if (dpi > 0) {
		br.setWidth(dpiToPx(dpi, br.width()));
		br.setHeight(dpiToPx(dpi, br.height()));
	}
	// 创建一个足够大的QImage对象来保存场景
	QImage image(br.width(), br.height(), QImage::Format_ARGB32);
	image.fill(Qt::transparent);  // 设置背景色为透明

	// 在QImage上创建一个QPainter对象
	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

	// 设置DPI
	if (dpi > 0) {
		image.setDotsPerMeterX(dpi);
		image.setDotsPerMeterY(dpi);
	}

	// 渲染场景到QImage上
	render(&painter);

	// 结束绘制
	painter.end();
	return image;
}

/**
 * @brief 开始链接模式
 *
 * 再开始链接模式，鼠标移动事件会改变当前链接线的末端位置，直到点击鼠标左键进行确认（endLink）,
 * 或者点击鼠标右键取消链接（cancelLink）
 * @param linkItem 连接线图元
 */
void DAGraphicsScene::beginLink(DAGraphicsLinkItem* linkItem, LinkMode lm)
{
	if (nullptr == linkItem) {
		cancelLink();
		return;
	}
	d_ptr->mLinkItem.reset(linkItem);
	if (linkItem->scene() != this) {
		addItem(linkItem);
	}
	switch (lm) {
	case LinkModeAutoStartEndFollowMouseClick: {  // 开端为当前鼠标位置，末端跟随鼠标移动，在下个鼠标左键点击时结束连线
		linkItem->setStartScenePosition(getCurrentMouseScenePos());
		d_ptr->mLinkItemIsMoved = false;
	} break;
	default:
		break;
	}

	linkItem->updateBoundingRect();
}

/**
 * @brief 判断当前是否是链接模式
 * @return
 */
bool DAGraphicsScene::isStartLink() const
{
    return (d_ptr->mLinkItem != nullptr);
}

/**
 * @brief 结束链接模式
 *
 * 结束链接模式会正常记录当前的连线
 *
 * 如果是要取消当前的连接线，使用@sa cancelLink
 *
 * @note 这里有个问题，endLink如果带有redo/undo动作的，但有些情况下，需要处理链接撤销前和后的动作，例如工作流，
 * 链接和不链接的逻辑是不一样的，因此，redo/undo的动作并不提供，而是由用户自己处理
 *
 */
void DAGraphicsScene::endLink()
{
	if (!isStartLink()) {
		return;
	}
	// 把item脱离智能指针管理
	if (!(d_ptr->mLinkItem->willCompleteLink())) {
		// 如果willCompleteLink返回false，endLink函数将中途推出，不接受
		return;
	}
	DAGraphicsLinkItem* linkItem = d_ptr->mLinkItem.release();
	linkItem->updateBoundingRect();
	// endLink如果带有redo/undo动作的，但有些情况下，需要处理链接撤销前和后的动作，例如工作流，
	// 链接和不链接的逻辑是不一样的，因此，redo/undo的动作并不提供，而是由用户自己处理
	//    DACommandsForGraphicsItemAdd* cmd = new DACommandsForGraphicsItemAdd(linkItem, this);
	//    push(cmd);
	emit linkCompleted(linkItem);
}

/**
 * @brief 取消链接模式
 */
void DAGraphicsScene::cancelLink()
{
	if (!isStartLink()) {
		return;
	}
	DAGraphicsLinkItem* linkItem = d_ptr->mLinkItem.get();
	removeItem(linkItem);
	d_ptr->mLinkItem.reset();
}

void DAGraphicsScene::cancel()
{
	cancelLink();
	clearSceneAction();
}

/**
 * @brief 获取当前正在进行连线的连接线item
 * @note 注意，此函数在@sa beginLink 调用之后才会有指针返回，在调用@sa endLink 或 @sa cancelLink 后都返回nullptr
 * @return 返回beginLink设置的指针
 */
DAGraphicsLinkItem* DAGraphicsScene::getCurrentLinkItem() const
{
    return d_ptr->mLinkItem.get();
}

/**
 * @brief 设置忽略链接事件的处理，主要忽略mousePressEvent，mouseMoveEvent的链接事件
 *
 * 这个函数一般是在子类中的重载函数中调用，用于进行一些特殊处理需要暂时屏蔽掉链接事件
 * @param on
 */
void DAGraphicsScene::setIgnoreLinkEvent(bool on)
{
    d_ptr->mIsIgnoreLinkEvent = on;
}

/**
 * @brief 判断当前是否忽略链接事件
 * @return
 */
bool DAGraphicsScene::isIgnoreLinkEvent() const
{
    return d_ptr->mIsIgnoreLinkEvent;
}

/**
   @brief 对选中item的分组
 */
void DAGraphicsScene::groupingSelectItems_()
{
	QList< QGraphicsItem* > selItems = selectedItems();
	if (selItems.empty()) {
		return;
	}
	if (1 == selItems.size()) {
		// 防止group套group
		if (QGraphicsItemGroup* g = dynamic_cast< QGraphicsItemGroup* >(selItems.first())) {
			// 选择了一个结果还是个group
			return;
		}
	}
	auto cmd = new DACommandsForGraphicsItemGrouping(this, selItems);
	push(cmd);
}

/**
   @brief 移除选中的分组
 */
void DAGraphicsScene::removeSelectItemGroup_()
{
	QList< QGraphicsItem* > si = selectedItems();
	for (QGraphicsItem* i : qAsConst(si)) {
		QGraphicsItemGroup* g = dynamic_cast< QGraphicsItemGroup* >(i);
		if (g) {
			auto cmd = new DACommandsForGraphicsItemUngrouping(this, g);
			push(cmd);
		}
	}
}

/**
 * @brief 是否允许对齐网格
 * @param on
 * @note 此操作对未对齐的item是不会起作用
 */
void DAGraphicsScene::setEnableSnapToGrid(bool on)
{
    d_ptr->mEnableSnapToGrid = on;
}
/**
 * @brief 是否允许对齐网格
 * @return
 */
bool DAGraphicsScene::isEnableSnapToGrid() const
{
    return d_ptr->mEnableSnapToGrid;
}
/**
 * @brief 设置网格尺寸
 * @param gs
 */
void DAGraphicsScene::setGridSize(const QSize& gs)
{
    d_ptr->mGridSize = gs;
}
/**
 * @brief 网格尺寸
 * @return
 */
QSize DAGraphicsScene::getGridSize() const
{
    return d_ptr->mGridSize;
}

/**
 * @brief 显示网格线
 * @param on
 */
void DAGraphicsScene::showGridLine(bool on)
{
    d_ptr->mShowGridLine = on;
}

/**
 * @brief 选中所有item
 *
 * @note 此函数会发射一次selectionChanged信号
 */
void DAGraphicsScene::selectAll()
{
	int selectCnt = changeAllSelection(true);
	if (selectCnt > 0) {
		emit selectionChanged();
	}
}

/**
 * @brief 取消选中item
 */
void DAGraphicsScene::clearSelection()
{
	int selectCnt = changeAllSelection(false);
	if (selectCnt > 0) {
		emit selectionChanged();
	}
}

/**
 * @brief 设置item的选中状态
 * @param its
 * @param isSelect
 */
int DAGraphicsScene::setSelectionState(const QList< QGraphicsItem* >& its, bool isSelect)
{
	int changeCnt = 0;
	for (QGraphicsItem* i : its) {
		if (i->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
			// 只有没有被选上，且是可选的才会执行选中动作
			i->setSelected(isSelect);
			++changeCnt;
		}
	}
	return changeCnt;
}

/**
 * @brief 设定为只读模式
 *
 * 锁定后，无法移动，无法编辑，进入只读模式
 */
void DAGraphicsScene::setReadOnly(bool on)
{
	DA_D(d);
	d->mIsReadOnlyMode = on;
	setIgnoreLinkEvent(on);
}

/**
 * @brief 是否显示网格线
 * @return
 */
bool DAGraphicsScene::isShowGridLine()
{
    return d_ptr->mShowGridLine;
}
/**
 * @brief 设置网格画笔
 * @param p
 */
void DAGraphicsScene::setGridLinePen(const QPen& p)
{
    d_ptr->mGridLinePen = p;
}
/**
 * @brief 获取网格画笔
 * @return
 */
QPen DAGraphicsScene::getGridLinePen() const
{
    return d_ptr->mGridLinePen;
}

/**
 * @brief 设置绘制背景使用缓冲
 * @param on
 */
void DAGraphicsScene::setPaintBackgroundInCache(bool on)
{
	d_ptr->mIsPaintBackgroundInCache = on;
	if (on) {
		d_ptr->renderBackgroundCache();
	}
}

/**
 * @brief 是否绘制背景使用缓冲
 * @return
 */
bool DAGraphicsScene::isPaintBackgroundInCache() const
{
    return d_ptr->mIsPaintBackgroundInCache;
}

/**
 * @brief 获取DANodeGraphicsScene内部维护的undoStack
 * @return
 */
QUndoStack& DAGraphicsScene::undoStack()
{
    return d_ptr->mUndoStack;
}

const QUndoStack& DAGraphicsScene::undoStack() const
{
    return d_ptr->mUndoStack;
}
/**
 * @brief 获取undostack指针
 * @return
 */
QUndoStack* DAGraphicsScene::getUndoStack() const
{
    return &(d_ptr->mUndoStack);
}

/**
 * @brief 在StackGroup中激活undoStack
 */
void DAGraphicsScene::setUndoStackActive()
{
	if (!d_ptr->mUndoStack.isActive()) {
		d_ptr->mUndoStack.setActive(true);
	}
}

/**
 * @brief 等同s->undoStack().push(cmd);
 * @param cmd
 */
void DAGraphicsScene::push(QUndoCommand* cmd)
{
    d_ptr->mUndoStack.push(cmd);
}

/**
   @brief 通过id查找item,此函数性能为O(n)
   @param id
   @param recursion 递归查找
   @return
 */
QGraphicsItem* DAGraphicsScene::findItemByID(uint64_t id, bool recursion) const
{
	QList< QGraphicsItem* > allItems = items();
	return findItemByID(allItems, id, recursion);
}

/**
   @brief 查找id对应的GraphicsItem*

   目前有@sa DAGraphicsItem，@sa DAGraphicsItemGroup，@sa DAGraphicsStandardTextItem 存在id
   @param its
   @param id
   @param recursion
   @return
 */
QGraphicsItem* DAGraphicsScene::findItemByID(const QList< QGraphicsItem* >& its, uint64_t id, bool recursion)
{
	for (auto i : its) {
		if (DAGraphicsItem* d = dynamic_cast< DAGraphicsItem* >(i)) {
			if (d->getItemID() == id) {
				return d;
			}
		} else if (DAGraphicsItemGroup* g = dynamic_cast< DAGraphicsItemGroup* >(i)) {
			if (g->getItemID() == id) {
				return g;
			}
		} else if (DAGraphicsStandardTextItem* s = dynamic_cast< DAGraphicsStandardTextItem* >(i)) {
			if (s->getItemID() == id) {
				return s;
			}
		}
		if (recursion) {
			QList< QGraphicsItem* > cs = i->childItems();
			if (!cs.empty()) {
				if (QGraphicsItem* c = findItemByID(cs, id, recursion)) {
					return c;
				}
			}
		}
	}
	return nullptr;
}

QList< QGraphicsItem* > DAGraphicsScene::topItems() const
{
	const QList< QGraphicsItem* > its = items();
	QList< QGraphicsItem* > r;
	for (QGraphicsItem* d : its) {
		if (nullptr == d->parentItem()) {
			r.append(d);
		}
	}
	return r;
}

QList< QGraphicsItem* > DAGraphicsScene::topItems(const QPointF& scenePos) const
{
	const QList< QGraphicsItem* > its = items(scenePos);
	QList< QGraphicsItem* > r;
	for (QGraphicsItem* d : its) {
		if (nullptr == d->parentItem()) {
			r.append(d);
		}
	}
	return r;
}

/**
 * @brief 获取选中的da item
 * @return
 */
QList< DAGraphicsItem* > DAGraphicsScene::selectedDAItems() const
{
	QList< DAGraphicsItem* > res;
	const QList< QGraphicsItem* > its = selectedItems();
	for (QGraphicsItem* item : its) {
		if (DAGraphicsItem* i = dynamic_cast< DAGraphicsItem* >(item)) {
			res.append(i);
		}
	}
	return res;
}

/**
 * @brief 设置场景就绪
 *
 * 如果场景还没加载完成，ready为false，一般这个函数在工程加载的时候应用
 * @param on
 */
void DAGraphicsScene::setReady(bool on)
{
    d_ptr->mIsReady = on;
}

/**
 * @brief 场景是否就绪
 * @return 如果场景还没加载完成，ready为false，一般这个函数在工程加载的时候应用
 */
bool DAGraphicsScene::isReady() const
{
    return d_ptr->mIsReady;
}

/**
 * @brief 获取默认的dpi
 * @return
 */
int DAGraphicsScene::getDefaultDPI()
{
	QScreen* screen = QApplication::primaryScreen();
	if (screen) {
		return screen->physicalDotsPerInch();
	}
	// 如果无法获取屏幕的DPI，则返回一个默认的DPI值，例如96
	return 96;
}

/**
 * @brief dpi转为像素
 * @param dpi
 * @param r
 * @return
 */
int DAGraphicsScene::dpiToPx(int dpi, int r)
{
    return r * dpi / 72;
}

/**
 * @brief 激活场景动作
 *
 * @note DAAbstractGraphicsSceneAction的内存归scene管理
 * @param act
 * @sa DAAbstractGraphicsSceneAction
 */
void DAGraphicsScene::setupSceneAction(DAAbstractGraphicsSceneAction* act)
{
	if (d_ptr->mSceneAction) {
		d_ptr->mSceneAction->endAction();
	}
	d_ptr->mSceneAction.reset(act);
	if (act) {
		act->beginActive();
	}
}

/**
 * @brief 判断当前是否存在场景动作
 * @return
 */
bool DAGraphicsScene::isHaveSceneAction() const
{
    return (d_ptr->mSceneAction != nullptr);
}

/**
 * @brief 清除场景动作
 */
void DAGraphicsScene::clearSceneAction()
{
	if (d_ptr->mSceneAction) {
		d_ptr->mSceneAction->endAction();
	}
	d_ptr->mSceneAction.reset(nullptr);
}

/**
 * @brief 获取所有图层
 * @return
 */
QList< DAGraphicsLayout* > DAGraphicsScene::getLayouts() const
{
    return d_ptr->mLayout;
}

bool DAGraphicsScene::isReadOnly() const
{
    return d_ptr->mIsReadOnlyMode;
}

/**
   @brief 通用的item分组，此操作和QGraphicsScene::createItemGroup逻辑一致
   @param group
   @param willGroupItems
 */
void DAGraphicsScene::addItemToGroup(QGraphicsItemGroup* group, const QList< QGraphicsItem* >& willGroupItems)
{
	QList< QGraphicsItem* > ancestors;
	int n = 0;
	if (!willGroupItems.isEmpty()) {
		QGraphicsItem* parent = willGroupItems.at(n++);
		while ((parent = parent->parentItem()))
			ancestors.append(parent);
	}

	// Find the common ancestor for all items
	QGraphicsItem* commonAncestor = 0;
	if (!ancestors.isEmpty()) {
		while (n < willGroupItems.size()) {
			int commonIndex       = -1;
			QGraphicsItem* parent = willGroupItems.at(n++);
			do {
				int index = ancestors.indexOf(parent, qMax(0, commonIndex));
				if (index != -1) {
					commonIndex = index;
					break;
				}
			} while ((parent = parent->parentItem()));

			if (commonIndex == -1) {
				commonAncestor = 0;
				break;
			}

			commonAncestor = ancestors.at(commonIndex);
		}
	}
	bool isNeedAcceptHoverEvents = false;
	group->setParentItem(commonAncestor);
	qreal minZ = std::numeric_limits< qreal >::max();
	// Create a new group at that level
	for (QGraphicsItem* item : qAsConst(willGroupItems)) {
		group->addToGroup(item);
		if (item->zValue() < minZ) {
			minZ = item->zValue();
		}
		isNeedAcceptHoverEvents |= item->acceptHoverEvents();
	}
	group->setAcceptHoverEvents(isNeedAcceptHoverEvents);
	group->setZValue(minZ - 1);
}

/**
 * @brief 判断点击的item是否可以移动
 *
 * 对于一些特殊的item，可以通过继承此函数来否决掉DAGraphicsSceneWithUndoStack对item移动的判断，否则会对item进行移动
 * 最简单的实现如下：
 * @code
 * bool DAGraphicsSceneWithUndoStack::isItemCanMove(const QGraphicsItem* positem, const QPointF& scenePos)
 * {
 *     //没选中
 *     if (positem == nullptr) {
 *         return false;
 *     }
 *     //选中了但不可移动，也不行
 *     if (!positem->flags().testFlag(QGraphicsItem::ItemIsMovable)) {
 *         return false;
 *     }
 *     return true;
 * }
 * @endcode
 * @param selitems
 * @return
 */
bool DAGraphicsScene::isItemCanMove(QGraphicsItem* positem, const QPointF& scenePos)
{
	// 没选中
	if (positem == nullptr) {
		return false;
	}
	// 选中了但不可移动，也不行
	if (!positem->flags().testFlag(QGraphicsItem::ItemIsMovable)) {
		return false;
	}
	// 还要确认一下是否点在了DAGraphicsResizeableItem的控制点上，点在控制点上是不能移动的
	DAGraphicsResizeableItem* resizeitem = qgraphicsitem_cast< DAGraphicsResizeableItem* >(positem);
	if (resizeitem) {
		DAGraphicsResizeableItem::ControlType t = resizeitem->getControlPointByPos(resizeitem->mapFromScene(scenePos));
		if (t != DAGraphicsResizeableItem::NotUnderAnyControlType) {
			// 说明点击在了控制点上，也取消移动
			return false;
		}
	}
	return true;
}

/**
 * @brief 调用此函数 主动触发itemsPositionChanged信号，这个函数用于 继承此类例如实现了键盘移动item，主动触发此信号
 * @param items
 * @param oldPos
 * @param newPos
 */
void DAGraphicsScene::emitItemsPositionChanged(const QList< QGraphicsItem* >& items,
                                               const QList< QPointF >& oldPos,
                                               const QList< QPointF >& newPos)
{
    emit itemsPositionChanged(items, oldPos, newPos);
}
/**
 * @brief 调用此函数 主动触发itemBodySizeChanged信号
 * @param item
 * @param oldSize
 * @param newSize
 */
void DAGraphicsScene::emitItemBodySizeChanged(DAGraphicsResizeableItem* item, const QSizeF& oldSize, const QSizeF& newSize)
{
    emit itemBodySizeChanged(item, oldSize, newSize);
}
/**
 * @brief 调用此函数 主动触发itemRotationed信号
 * @param item
 * @param rotation
 */
void DAGraphicsScene::emitItemRotationChanged(DAGraphicsResizeableItem* item, const qreal& rotation)
{
    emit itemRotationChanged(item, rotation);
}

/**
 * @brief 带信号的addItm
 * @param item
 */
void DAGraphicsScene::addItemWithSignal(QGraphicsItem* item)
{
	addItem(item);
	emit itemsAdded({ item });
}

void DAGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (d_ptr->mSceneAction) {
		// 存在场景动作
		if (d_ptr->mSceneAction->mousePressEvent(mouseEvent)) {
			// 场景动作返回true，直接返回此动作，代表场景动作劫持了此事件
			return;
		}
	}
	// 记录鼠标点击的位置
	d_ptr->mLastMousePressScenePos = mouseEvent->scenePos();
	// 先传递下去使得能处理选中状态
	QGraphicsScene::mousePressEvent(mouseEvent);
	//! 处理链接事件
	if (!isIgnoreLinkEvent()) {
		if (isStartLink()) {
			// 链接线模式，处理连接线
			if (mouseEvent->buttons().testFlag(Qt::RightButton)) {
				// 右键点击是取消
				cancelLink();
			} else if (mouseEvent->buttons().testFlag(Qt::LeftButton)) {
				// 左键点击
				switch (d_ptr->mLinkMode) {
				case LinkModeAutoStartEndFollowMouseClick: {
					// 结束链接
					if (d_ptr->mLinkItemIsMoved) {
						endLink();
					}
				} break;
				default:
					break;
				}
			}
			// 处理链接事件时，忽略掉移动事件的处理，所以这里要return掉
			return;
		}
	}
	//! 处理有移动事件
	if (mouseEvent->buttons().testFlag(Qt::LeftButton)) {
		//!  1.记录选中的所有图元，如果点击的是改变尺寸的点，这个就不执行记录
		QGraphicsItem* positem = itemAt(d_ptr->mLastMousePressScenePos, QTransform());
		if (!isItemCanMove(positem, d_ptr->mLastMousePressScenePos)) {
			d_ptr->mIsMovingItems = false;
			return;
		}
		// 说明这个是移动
		// 获取选中的可移动单元
		QList< QGraphicsItem* > mits = getSelectedMovableItems();
		if (mits.isEmpty()) {
			d_ptr->mIsMovingItems = false;
			return;
		}
		// todo.如果点击的是链接和control point，不属于移动
		for (QGraphicsItem* its : qAsConst(mits)) {
			DAGraphicsResizeableItem* ri = dynamic_cast< DAGraphicsResizeableItem* >(its);
			if (ri) {
				if (DAGraphicsResizeableItem::NotUnderAnyControlType
					!= ri->getControlPointByPos(ri->mapFromScene(mouseEvent->scenePos()))) {
					// 说明点击在了控制点上，需要跳过
					d_ptr->mIsMovingItems = false;
					return;
				}
			}
		}
		d_ptr->mIsMovingItems = true;
		//
		d_ptr->mMovingInfos.clear();
		for (QGraphicsItem* its : qAsConst(mits)) {
			d_ptr->mMovingInfos.appendStartPos(its);
		}
	}
}

void DAGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (d_ptr->mSceneAction) {
		// 存在场景动作
		if (d_ptr->mSceneAction->mouseMoveEvent(mouseEvent)) {
			// 场景动作返回true，直接返回此动作，代表场景动作劫持了此事件
			return;
		}
	}
	d_ptr->mLastMouseScenePos = mouseEvent->scenePos();
	if (!isIgnoreLinkEvent()) {
		if (isStartLink()) {
			switch (d_ptr->mLinkMode) {
			case LinkModeAutoStartEndFollowMouseClick: {  // 开端为当前鼠标位置，末端跟随鼠标移动，在下个鼠标左键点击时结束连线
				d_ptr->mLinkItem->setEndScenePosition(d_ptr->mLastMouseScenePos);
				d_ptr->mLinkItem->updateBoundingRect();
				d_ptr->mLinkItemIsMoved = true;
			} break;
			default:
				break;
			}
		}
	}
	QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void DAGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (d_ptr->mSceneAction) {
		// 存在场景动作
		if (d_ptr->mSceneAction->mouseReleaseEvent(mouseEvent)) {
			// 场景动作返回true，直接返回此动作，代表场景动作劫持了此事件
			return;
		}
	}
	QGraphicsScene::mouseReleaseEvent(mouseEvent);
	if (d_ptr->mIsMovingItems) {
		d_ptr->mIsMovingItems = false;
		QPointF releasePos    = mouseEvent->scenePos();
		if (qFuzzyCompare(releasePos.x(), d_ptr->mLastMousePressScenePos.x())
			&& qFuzzyCompare(releasePos.y(), d_ptr->mLastMousePressScenePos.y())) {
			// 位置相等，不做处理
			return;
		}
		// 位置不等，属于正常移动
		d_ptr->mMovingInfos.updateEndPos();
		DACommandsForGraphicsItemsMoved* cmd = new DACommandsForGraphicsItemsMoved(d_ptr->mMovingInfos.items,
																				   d_ptr->mMovingInfos.startsPos,
																				   d_ptr->mMovingInfos.endsPos,
																				   true);
		push(cmd);
		// 位置改变信号
		//         qDebug() << "emit itemsPositionChanged";
		// 如果 移动 过程 鼠标移出scene在释放，可能无法捕获
		emit itemsPositionChanged(d_ptr->mMovingInfos.items, d_ptr->mMovingInfos.startsPos, d_ptr->mMovingInfos.endsPos);
	}
}

void DAGraphicsScene::drawBackground(QPainter* painter, const QRectF& rect)
{
	QGraphicsScene::drawBackground(painter, rect);
	if (isShowGridLine()) {
		if (isPaintBackgroundInCache()) {
			if (d_ptr->mBackgroundCache.size() != sceneRect().toRect().size()) {
				d_ptr->renderBackgroundCache();
			}
			const QPointF& pixmapTopleft = rect.topLeft() - sceneRect().topLeft();
			qreal x                      = int(pixmapTopleft.x()) - (int(pixmapTopleft.x()) % d_ptr->mGridSize.width());
			qreal y = int(pixmapTopleft.y()) - (int(pixmapTopleft.y()) % d_ptr->mGridSize.height());
			painter->drawPixmap(rect, d_ptr->mBackgroundCache, QRectF(x, y, rect.width(), rect.height()));

		} else {
			d_ptr->renderBackgroundCache(painter, rect.toRect());
		}
	}
	// 直接绘制网格线在放大时很卡
}

void DAGraphicsScene::onSelectionChanged()
{
	QList< QGraphicsItem* > sits = selectedItems();
	if (sits.isEmpty()) {
		return;
	}
	checkSelectItem(sits.last());
}

void DAGraphicsScene::checkSelectItem(QGraphicsItem* item)
{
	if (DAGraphicsItem* gi = dynamic_cast< DAGraphicsItem* >(item)) {
		emit selectItemChanged(gi);
	} else if (DAGraphicsLinkItem* gi = dynamic_cast< DAGraphicsLinkItem* >(item)) {
		emit selectLinkChanged(gi);
	} else if (DAGraphicsItemGroup* gi = dynamic_cast< DAGraphicsItemGroup* >(item)) {
		// 选中的分组，要检查点击的是哪个
		QList< QGraphicsItem* > gites = gi->childItems();
		for (QGraphicsItem* i : qAsConst(gites)) {
			if (i->isUnderMouse() && i != item) {
				checkSelectItem(i);
			}
		}
	}
}

int DAGraphicsScene::changeAllSelection(bool setSelect)
{
	return setSelectionState(items(), setSelect);
}

}  // end DA
