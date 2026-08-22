#include "DAGraphicsResizeableItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtMath>
#include <QDomDocument>
#include <QDomElement>
#include "DAGraphicsScene.h"

namespace DA
{
//===================================================
// DAGraphicsResizeableItem::PrivateData
//===================================================
class DAGraphicsResizeableItem::PrivateData
{
	DA_DECLARE_PUBLIC(DAGraphicsResizeableItem)
public:
	PrivateData(DAGraphicsResizeableItem* p);
	// 计算合理的尺寸
	QSizeF testBodySize(const QSizeF& ts) const;
	bool testBodySize(QSizeF& ts) const;
	// 位置坐标匹配网格
	void adjustPosToGrid(QPointF& pos);

public:
	bool mEnableResize { true };                    ///< 是否允许调整大小
	bool mAutoCenterTransformOriginPoint { true };  ///< 自动更新TransformOriginPoint
	DAGraphicsScene* mSceneUndo { nullptr };        ///< 保存scene
	QSizeF mSize { 30, 30 };                        ///< 尺寸
	QSizeF mMinSize { 5, 5 };                       ///< 最小尺寸
	QSizeF mMaxSize { 9999, 9999 };                 ///< 最大尺寸
	QPointF mPainterRectStartPos { 0, 0 };          ///< 绘图范围的开始位置
	QSizeF mControlPointSize { 10, 10 };            ///< 控制点的大小（deprecated，保留以维持 XML 兼容）
};

DAGraphicsResizeableItem::PrivateData::PrivateData(DAGraphicsResizeableItem* p) : q_ptr(p)
{
}
/**
 * @brief 检测尺寸，返回检测后的结果，如果合格，返回的和传入的是一致的
 * @param ts
 * @return
 */
QSizeF DAGraphicsResizeableItem::PrivateData::testBodySize(const QSizeF& ts) const
{
	QSizeF s = ts;
	if (s.width() < mMinSize.width()) {
		s.setWidth(mMinSize.width());
	}
	if (s.height() < mMinSize.height()) {
		s.setHeight(mMinSize.height());
	}
	if (s.width() > mMaxSize.width()) {
		s.setWidth(mMaxSize.width());
	}
	if (s.height() > mMaxSize.height()) {
		s.setHeight(mMaxSize.height());
	}
	return s;
}
/**
 * @brief 检测尺寸，如果尺寸改变了，返回false，如果尺寸无需改变返回true
 * @param ts
 * @return
 */
bool DAGraphicsResizeableItem::PrivateData::testBodySize(QSizeF& ts) const
{
	bool res = true;
	if (ts.width() < mMinSize.width()) {
		ts.setWidth(mMinSize.width());
		res = false;
	}
	if (ts.height() < mMinSize.height()) {
		ts.setHeight(mMinSize.height());
		res = false;
	}
	if (ts.width() > mMaxSize.width()) {
		ts.setWidth(mMaxSize.width());
		res = false;
	}
	if (ts.height() > mMaxSize.height()) {
		ts.setHeight(mMaxSize.height());
		res = false;
	}
	return res;
}

/**
 * @brief 位置坐标匹配网格
 * @param pos
 */
void DAGraphicsResizeableItem::PrivateData::adjustPosToGrid(QPointF& pos)
{
	if (!q_ptr->isSnapToGrid()) {
		return;
	}
	QSize gridsize = q_ptr->getGridSize();
	if (gridsize.isValid()) {
		// If it is rotated 90 or 270 degrees and the difference between
		// the height and width is odd then the position needs to be
		// offset by half a grid unit vertically and horizontally.
		if ((qFuzzyCompare(qAbs(q_ptr->rotation()), 90) || qFuzzyCompare(qAbs(q_ptr->rotation()), 270))
			&& (fmod(mSize.width() / gridsize.width() - mSize.height() / gridsize.height(), 2) != 0)) {
			pos.setX(qCeil(pos.x() / gridsize.width()) * gridsize.width());
			pos.setY(qCeil(pos.y() / gridsize.height()) * gridsize.height());
			pos -= QPointF(gridsize.width() / 2, gridsize.height() / 2);
		} else {
			pos.setX(qRound(pos.x() / gridsize.width()) * gridsize.width());
			pos.setY(qRound(pos.y() / gridsize.height()) * gridsize.height());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// DAGraphicsResizeableItem
//////////////////////////////////////////////////////////////////////////////

DAGraphicsResizeableItem::DAGraphicsResizeableItem(QGraphicsItem* parent) : DAGraphicsItem(parent), DA_PIMPL_CONSTRUCT
{
	setFlags(flags() | ItemIsSelectable | ItemIsMovable
			 | ItemSendsGeometryChanges  // 确保位置改变时能发出QGraphicsItem::ItemPositionHasChanged
	);
	// setAcceptHoverEvents 已移除，hover 事件由 Overlay 处理
}

DAGraphicsResizeableItem::~DAGraphicsResizeableItem()
{
}

/**
 * @brief DAGraphicsResizeableItem的boundingRect会根据getSize尺寸进行计算
 * @return
 */
QRectF DAGraphicsResizeableItem::boundingRect() const
{
	return getBodyRect();  // 不再膨胀控制点区域
}

/**
 * @brief 对setPos的封装
 * @param p
 */
void DAGraphicsResizeableItem::setBodyPos(const QPointF& p)
{
	setPos(p);
}

void DAGraphicsResizeableItem::setBodyScenePos(const QPointF& p)
{
	setScenePos(p);
}

/**
 * @brief 返回body的中心点，此坐标系为item坐标系
 * @return item坐标系
 * @sa getBodyCenterPosition
 */
QPointF DAGraphicsResizeableItem::getBodyCenterPoint() const
{
	QSizeF s = getBodySize();
	return QPointF(d_ptr->mPainterRectStartPos.x() + s.width() / 2, d_ptr->mPainterRectStartPos.y() + s.height() / 2);
}

/**
 * @brief 获取body中心的位置
 * @return scene坐标系
 * @sa getBodyCenterPoint
 */
QPointF DAGraphicsResizeableItem::getBodyCenterPos() const
{
	QPointF p = getBodyCenterPoint();
	return mapToScene(p);
}

/**
  @brief setBodyCenterPos

  @param p scene的点
*/
void DAGraphicsResizeableItem::setBodyCenterPos(const QPointF& p)
{
	QPointF cp = getBodyCenterPoint();
	setPos(p.x() - cp.x(), p.y() - cp.y());
}

/**
 * @brief 设置TransformOriginPoint自动设置为bodysize的中心,否则为用户自己指定
 * @param on
 */
void DAGraphicsResizeableItem::setAutoCenterTransformOriginPoint(bool on)
{
	if (!d_ptr->mAutoCenterTransformOriginPoint && on) {
		// 从否转为true，需要立即重写计算一下
		d_ptr->mAutoCenterTransformOriginPoint = true;
		updateTransformOriginPoint();
	}
	d_ptr->mAutoCenterTransformOriginPoint = on;
}

/**
 * @brief 更新TransformOriginPoint
 */
void DAGraphicsResizeableItem::updateTransformOriginPoint()
{
	if (d_ptr->mAutoCenterTransformOriginPoint) {
		setTransformOriginPoint(d_ptr->mPainterRectStartPos.x() + (d_ptr->mSize.width() / 2),
								d_ptr->mPainterRectStartPos.y() + (d_ptr->mSize.height() / 2));
	}
}

/**
 * @brief DAGraphicsResizeableItem::itemChange
 * @param change
 * @param value
 * @return
 */
QVariant DAGraphicsResizeableItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
	switch (change) {
	case QGraphicsItem::ItemPositionChange: {
		if (isSceneReadOnly()) {
			return pos();
		}
		QPointF newPos = value.toPointF();
		// 网格对齐逻辑保留
		d_ptr->adjustPosToGrid(newPos);
		return newPos;
	}
	case QGraphicsItem::ItemRotationHasChanged: {
		if (d_ptr->mSceneUndo) {
			d_ptr->mSceneUndo->emitItemRotationChanged(this, rotation());
		}
		break;
	}
	case QGraphicsItem::ItemSceneHasChanged: {
		// 记录scene
		d_ptr->mSceneUndo = qobject_cast< DAGraphicsScene* >(scene());
		break;
	}
	default:
		break;
	}
	return DAGraphicsItem::itemChange(change, value);
}

/**
 * @brief 用户不要继承此shape函数，而是继承bodyShape函数
 * @return
 */
QPainterPath DAGraphicsResizeableItem::shape() const
{
	return getBodyShape();
}

/**
 * @brief 获取绘图的shape
 * @return
 */
QPainterPath DAGraphicsResizeableItem::getBodyShape() const
{
	QPainterPath p;
	p.addRect(getBodyRect());  // 不再用 getBodyControlRect()
	return p;
}

/**
 * @brief 用户不要继承此paint函数，而是继承paintBody函数
 *
 * 几个虚函数的绘制顺序：
 *
 * @param painter
 * @param option
 * @param widget
 */
void DAGraphicsResizeableItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QRectF bodyrect = getBodyRect();
	paintBackground(painter, option, widget, bodyrect);
	paintBorder(painter, option, widget, bodyrect);
	paintBody(painter, option, widget, bodyrect);
	// 选中边框始终由图元自身绘制（单选/多选均显示），Overlay 仅负责控制点
	if (isSelected()) {
		QPen pen(QColor(32, 128, 240));
		pen.setStyle(Qt::DashLine);
		painter->setPen(pen);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(bodyrect);
	}
	// 不再绘制控制点 — 由 Overlay 负责（仅单选可缩放图元）
}

bool DAGraphicsResizeableItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
	DAGraphicsItem::saveToXml(doc, parentElement, ver);
	QDomElement rsinfoEle = doc->createElement("resize-info");
	rsinfoEle.setAttribute("enableResize", isResizable());
	QSizeF bs = getBodySize();
	rsinfoEle.setAttribute("width", bs.width());
	rsinfoEle.setAttribute("height", bs.height());
	bs = getBodyMaximumSize();
	rsinfoEle.setAttribute("maxWidth", bs.width());
	rsinfoEle.setAttribute("maxHeight", bs.height());
	bs = getBodyMinimumSize();
	rsinfoEle.setAttribute("minWidth", bs.width());
	rsinfoEle.setAttribute("minHeight", bs.height());
	parentElement->appendChild(rsinfoEle);

	QDomElement conEle = doc->createElement("controler");
	bs                 = d_ptr->mControlPointSize;
	conEle.setAttribute("width", bs.width());
	conEle.setAttribute("height", bs.height());

	rsinfoEle.appendChild(conEle);
	parentElement->appendChild(rsinfoEle);
	return true;
}

bool DAGraphicsResizeableItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
	if (!DAGraphicsItem::loadFromXml(itemElement, ver)) {
		return false;
	}
	QDomElement rsinfoEle = itemElement->firstChildElement("resize-info");
	if (rsinfoEle.isNull()) {
		return false;
	}
	setEnableResize(getStringBoolValue(rsinfoEle.attribute("enableResize")));
	qreal v1, v2;
	if (getStringRealValue(rsinfoEle.attribute("width"), v1) && getStringRealValue(rsinfoEle.attribute("height"), v2)) {
		setBodySize(QSizeF(v1, v2));
	}
	if (getStringRealValue(rsinfoEle.attribute("maxWidth"), v1) && getStringRealValue(rsinfoEle.attribute("maxHeight"), v2)) {
		setBodyMaximumSize(QSizeF(v1, v2));
	}
	if (getStringRealValue(rsinfoEle.attribute("minWidth"), v1) && getStringRealValue(rsinfoEle.attribute("minHeight"), v2)) {
		setBodyMinimumSize(QSizeF(v1, v2));
	}
	QDomElement conEle = rsinfoEle.firstChildElement("controler");
	if (!conEle.isNull()) {
		if (getStringRealValue(conEle.attribute("width"), v1) && getStringRealValue(conEle.attribute("height"), v2)) {
			d_ptr->mControlPointSize = QSizeF(v1, v2);
			update();
		}
	}
	return true;
}

/**
 * @brief 设置尺寸
 * @note setBodySize是虚函数，在scene鼠标动作的时候会触发此函数，
 * 如果仅仅想改变bodysize的尺寸，可以调用@sa changeBodySize
 * @param s
 */
void DAGraphicsResizeableItem::setBodySize(const QSizeF& s)
{
	QSizeF cs = d_ptr->testBodySize(s);
	if (cs != d_ptr->mSize) {
		QSizeF oldsize = d_ptr->mSize;
		prepareGeometryChange();
		changeBodySize(cs);
		if (d_ptr->mSceneUndo) {
			d_ptr->mSceneUndo->emitItemBodySizeChanged(this, oldsize, d_ptr->mSize);
		}
	}
}

/**
 * @brief 绘图的区域
 * @return
 */
QRectF DAGraphicsResizeableItem::getBodyRect() const
{
	return QRectF(d_ptr->mPainterRectStartPos, d_ptr->mSize);
}

/**
 * @brief 获取尺寸
 * @return
 */
QSizeF DAGraphicsResizeableItem::getBodySize() const
{
	return d_ptr->mSize;
}

/**
 * @brief 获取body控制矩形，已废弃，等价于 getBodyRect()
 * @return
 */
QRectF DAGraphicsResizeableItem::getBodyControlRect() const
{
	return getBodyRect();  // 不再膨胀，等价于 getBodyRect()
}

/**
 * @brief 设置最小尺寸
 * @param s
 */
void DAGraphicsResizeableItem::setBodyMinimumSize(const QSizeF& s)
{
	d_ptr->mMinSize = s;
	setBodySize(d_ptr->mSize);
}

/**
 * @brief 设置最大尺寸
 * @param s
 */
void DAGraphicsResizeableItem::setBodyMaximumSize(const QSizeF& s)
{
	d_ptr->mMaxSize = s;
	setBodySize(d_ptr->mSize);
}
/**
 * @brief 获取最小尺寸
 */
QSizeF DAGraphicsResizeableItem::getBodyMinimumSize() const
{
	return d_ptr->mMinSize;
}
/**
 * @brief 获取最大尺寸
 */
QSizeF DAGraphicsResizeableItem::getBodyMaximumSize() const
{
	return d_ptr->mMaxSize;
}

/**
 * @brief 设置控制器的大小（已废弃，控制点由 Overlay 管理）
 * @param s
 */
void DAGraphicsResizeableItem::setControlerSize(const QSizeF& s)
{
	d_ptr->mControlPointSize = s;
	update();
}

QSizeF DAGraphicsResizeableItem::getControlerSize() const
{
	return d_ptr->mControlPointSize;
}

/**
 * @brief 控制点的大小（非虚，仅返回 mControlPointSize）
 * @return
 */
QSizeF DAGraphicsResizeableItem::controlPointSize() const
{
	return d_ptr->mControlPointSize;
}

/**
 * @brief 设置是否允许缩放
 * @param on
 */
void DAGraphicsResizeableItem::setEnableResize(bool on)
{
	d_ptr->mEnableResize = on;
	update();  // 刷新选中边框显示
	// setAcceptHoverEvents(on) 已移除，hover 事件由 Overlay 处理
}
/**
 * @brief 判断是否允许
 * @return
 */
bool DAGraphicsResizeableItem::isResizable() const
{
	return d_ptr->mEnableResize;
}

/**
 * @brief 绘制背景
 * @param painter
 * @param option
 * @param widget
 * @param bodyRect
 */
void DAGraphicsResizeableItem::paintBackground(QPainter* painter,
                                               const QStyleOptionGraphicsItem* option,
                                               QWidget* widget,
                                               const QRectF& bodyRect)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (isShowBackground()) {
		painter->save();
		painter->setPen(Qt::NoPen);
		painter->fillRect(bodyRect, getBackgroundBrush());
		painter->restore();
	}
}

/**
 * @brief 绘制边框
 * @param painter
 * @param option
 * @param widget
 * @param bodyRect body的尺寸
 */
void DAGraphicsResizeableItem::paintBorder(QPainter* painter,
                                           const QStyleOptionGraphicsItem* option,
                                           QWidget* widget,
                                           const QRectF& bodyRect)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (isShowBorder()) {
		painter->save();
		painter->setPen(getBorderPen());
		painter->drawRect(bodyRect);
		painter->restore();
	}
}

/**
 * @brief 是否允许对齐网格
 * @return
 */
bool DAGraphicsResizeableItem::isSnapToGrid() const
{
	DAGraphicsScene* sc = d_ptr->mSceneUndo;
	if (sc) {
		return sc->isEnableSnapToGrid();
	}
	return false;
}
/**
 * @brief 获取网格尺寸
 * @return
 */
QSize DAGraphicsResizeableItem::getGridSize() const
{
	DAGraphicsScene* sc = qobject_cast< DAGraphicsScene* >(scene());
	if (sc) {
		return sc->getGridSize();
	}
	return QSize();
}

/**
 * @brief 测试尺寸是否在最大最小范围内，返回修正后的尺寸
 * @param s
 * @return
 */
QSizeF DAGraphicsResizeableItem::testBodySize(const QSizeF& s) const
{
	return d_ptr->testBodySize(s);
}

/**
 * @brief 此函数和setBodySize不同，setBodySize是虚函数，且会校验尺寸的最大最小范围，此函数不进行校验
 *
 * 此函数适合继承的类在构造函数中调用，应为理论上构造函数不应该调用虚函数
 * @param s
 */
void DAGraphicsResizeableItem::changeBodySize(const QSizeF& s)
{
	if (s != d_ptr->mSize) {
		d_ptr->mSize = s;
		updateTransformOriginPoint();
		// prepareControlInfoChange() 已移除
	}
}

//======================================================================
// DAIResizableGraphicsItem 接口实现
//======================================================================

QPointF DAGraphicsResizeableItem::getBodyPainterStartPos() const
{
	return d_ptr->mPainterRectStartPos;
}

QPointF DAGraphicsResizeableItem::getBodyTransformOriginPoint() const
{
	return transformOriginPoint();
}

QGraphicsItem* DAGraphicsResizeableItem::graphicsItem()
{
	return this;
}

const QGraphicsItem* DAGraphicsResizeableItem::graphicsItem() const
{
	return this;
}

}  // end namespace DA
