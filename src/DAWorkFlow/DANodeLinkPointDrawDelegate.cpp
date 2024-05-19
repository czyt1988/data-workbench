#include "DANodeLinkPointDrawDelegate.h"
#include <QGraphicsItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include "DAAbstractNodeGraphicsItem.h"
#include "DANodePalette.h"
const int c_lpHWidth  = 14;  // 连接点水平时的宽度
const int c_lpHHeight = 10;  // 连接点水平时的高度
namespace DA
{
class DANodeLinkPointDrawDelegate::PrivateData
{
	DA_DECLARE_PUBLIC(DANodeLinkPointDrawDelegate)
public:
	PrivateData(DANodeLinkPointDrawDelegate* p);
	DAAbstractNodeGraphicsItem* mItem;  ///< 绑定的item
	bool mShowText { true };
};

//===================================================
//
//===================================================
DANodeLinkPointDrawDelegate::PrivateData::PrivateData(DANodeLinkPointDrawDelegate* p) : q_ptr(p)
{
}

////////////////////////////////////

DANodeLinkPointDrawDelegate::DANodeLinkPointDrawDelegate(DAAbstractNodeGraphicsItem* i) : DA_PIMPL_CONSTRUCT
{
    setItem(i);
}

DANodeLinkPointDrawDelegate::~DANodeLinkPointDrawDelegate()
{
}

/**
 * @brief 设置item
 * @param i
 */
void DANodeLinkPointDrawDelegate::setItem(DAAbstractNodeGraphicsItem* i)
{
    d_ptr->mItem = i;
}

/**
 * @brief 获取item
 * @return
 */
DAAbstractNodeGraphicsItem* DANodeLinkPointDrawDelegate::getItem() const
{
    return d_ptr->mItem;
}

/**
 * @brief 绘制连接点
 * @param painter
 * @param option
 * @param widget
 */
void DANodeLinkPointDrawDelegate::paintLinkPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QList< DANodeLinkPoint > pls = getLinkPoints();
	paintLinkPoints(pls, painter, option, widget);
	if (d_ptr->mShowText) {
		paintLinkPointTexts(pls, painter, option, widget);
	}
}

/**
 * @brief 显示连接点的文字
 * @param on
 */
void DANodeLinkPointDrawDelegate::showLinkPointText(bool on)
{
    d_ptr->mShowText = on;
}

bool DANodeLinkPointDrawDelegate::isShowLinkPointText() const
{
    return d_ptr->mShowText;
}

/**
 * @brief 重新计算连接点的信息，此函数用来布局连接点
 * @param lps
 * @param bodyRect
 */
void DANodeLinkPointDrawDelegate::layoutLinkPoints(QList< DANodeLinkPoint >& lps, const QRectF& bodyRect)
{
	int inputCnt                     = 0;
	int outputCnt                    = 0;
	DAAbstractNodeGraphicsItem* item = getItem();
	if (!item) {
		return;
	}
	for (const DANodeLinkPoint& lp : lps) {
		if (lp.way == DANodeLinkPoint::Output) {
			++outputCnt;
		} else {
			++inputCnt;
		}
	}
	// 离开边界的距离
	const qreal spaceSide = 2;
	// 获取出入口位置
	const DAAbstractNodeGraphicsItem::LinkPointLocation inLoc  = item->getLinkPointLocation(DANodeLinkPoint::Input);
	const DAAbstractNodeGraphicsItem::LinkPointLocation outLoc = item->getLinkPointLocation(DANodeLinkPoint::Output);
	// 计算出入口的每次偏移量
	qreal dtIn  = 0;
	qreal dtOut = 0;
	switch (inLoc) {
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
		dtIn = (bodyRect.height() - 2 * spaceSide) / (inputCnt + 1.0);
		break;
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
		dtIn = (bodyRect.width() - 2 * spaceSide) / (inputCnt + 1.0);
	default:
		break;
	}
	switch (outLoc) {
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
		dtOut = (bodyRect.height() - 2 * spaceSide) / (outputCnt + 1.0);
		break;
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
	case DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
		dtOut = (bodyRect.width() - 2 * spaceSide) / (outputCnt + 1.0);
	default:
		break;
	}
	// 计数清零
	inputCnt  = 0;
	outputCnt = 0;
	for (DANodeLinkPoint& lp : lps) {
		if (lp.isInput()) {
			++inputCnt;
			switch (inLoc) {
			case DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
				lp.direction = AspectDirection::West;
				lp.position  = QPointF(bodyRect.left() + spaceSide, bodyRect.top() + spaceSide + (inputCnt * dtIn));
				break;
			case DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
				lp.direction = AspectDirection::North;
				lp.position  = QPointF(bodyRect.left() + spaceSide + (inputCnt * dtIn), bodyRect.top() + spaceSide);
				break;
			case DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
				lp.direction = AspectDirection::East;
				lp.position  = QPointF(bodyRect.right() - spaceSide, bodyRect.top() + spaceSide + (inputCnt * dtIn));
				break;
			case DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
				lp.direction = AspectDirection::South;
				lp.position  = QPointF(bodyRect.left() + spaceSide + (inputCnt * dtIn), bodyRect.bottom() - spaceSide);
				break;
			default:
				break;
			}
		} else {
			// 出口
			++outputCnt;
			switch (outLoc) {
			case DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
				lp.direction = AspectDirection::West;
				lp.position  = QPointF(bodyRect.left() + spaceSide, bodyRect.top() + spaceSide + (outputCnt * dtOut));
				break;
			case DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
				lp.direction = AspectDirection::North;
				lp.position  = QPointF(bodyRect.left() + spaceSide + (outputCnt * dtOut), bodyRect.top() + spaceSide);
				break;
			case DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
				lp.direction = AspectDirection::East;
				lp.position  = QPointF(bodyRect.right() - spaceSide, bodyRect.top() + spaceSide + (outputCnt * dtOut));
				break;
			case DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
				lp.direction = AspectDirection::South;
				lp.position = QPointF(bodyRect.left() + spaceSide + (outputCnt * dtOut), bodyRect.bottom() - spaceSide);
				break;
			default:
				break;
			}
		}
	}
}

/**
 * @brief 等同DAAbstractNodeGraphicsItem::getLinkPoints,正常情况，不需要继承此函数，此函数只有比较特殊的情况继承
 *
 * 如果要做一些特殊的调整，可以继承此函数，从而使得绘图获取的LinkPoints和Item获取的LinkPoints不一样，
 * 例如有些特殊需求，在链接成功后就不显示linkpoint，此函数就可以返回一个假的QList< DANodeLinkPoint >，
 * 让绘图时把不想绘制的LinkPoints排除
 * @return
 */
QList< DANodeLinkPoint > DANodeLinkPointDrawDelegate::getLinkPoints() const
{
    return d_ptr->mItem->getLinkPoints();
}

/**
 * @brief 绘制连接点
 * @param pl 连接点
 * @param painter
 * @param option
 * @param widget
 */
void DANodeLinkPointDrawDelegate::paintLinkPoints(const QList< DANodeLinkPoint >& pls,
                                                  QPainter* painter,
                                                  const QStyleOptionGraphicsItem* option,
                                                  QWidget* widget)
{
	for (const DANodeLinkPoint& pl : pls) {
		paintLinkPoint(pl, painter, option, widget);
	}
}
/**
 * @brief 绘制连接点文本
 * @param pl
 * @param painter
 * @param option
 * @param widget
 */
void DANodeLinkPointDrawDelegate::paintLinkPointTexts(const QList< DANodeLinkPoint >& pls,
                                                      QPainter* painter,
                                                      const QStyleOptionGraphicsItem* option,
                                                      QWidget* widget)
{
	for (const DANodeLinkPoint& pl : pls) {
		paintLinkPointText(pl, painter, option, widget);
	}
}

/**
 * @brief 获取连接点的绘图区域
 *
 * DAAbstractNodeGraphicsItem::getLinkPointByPos函数是通过此函数获取到的路径来进行判断
 *
 * 此函数会影响到场景链接过程选中的状态，比较关键，决定了DANodeGraphicsScene::nodeItemLinkPointSelected能否发射
 *
 * @sa DAAbstractNodeGraphicsItem::getLinkPointByPos DANodeGraphicsScene::nodeItemLinkPointSelected
 * @param pl
 * @return
 */
QPainterPath DANodeLinkPointDrawDelegate::getlinkPointPainterRegion(const DANodeLinkPoint& pl) const
{
	QPainterPath region;
	switch (pl.direction) {
	case AspectDirection::East:
	case AspectDirection::West:
		region.addRect(QRectF(pl.position.x() - c_lpHWidth / 2, pl.position.y() - c_lpHHeight / 2, c_lpHWidth, c_lpHHeight));
		break;
	case AspectDirection::North:
	case AspectDirection::South:
		region.addRect(QRectF(pl.position.x() - c_lpHHeight / 2, pl.position.y() - c_lpHWidth / 2, c_lpHHeight, c_lpHWidth));
		break;
	default:
		region.addRect(QRectF(pl.position.x() - c_lpHWidth / 2, pl.position.y() - c_lpHWidth / 2, c_lpHWidth, c_lpHWidth));
		break;
	}
	return region;
}
/**
 * @brief 绘制连接点
 * @param pl 连接点
 * @param painter
 * @param option
 * @param widget
 */
void DANodeLinkPointDrawDelegate::paintLinkPoint(const DANodeLinkPoint& pl,
                                                 QPainter* painter,
                                                 const QStyleOptionGraphicsItem* option,
                                                 QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	painter->save();
	// 连接点是一个长方形，6X8,点中心是长方形中心
	// 先把painter坐标变换到点处
	const DANodePalette& palette = d_ptr->mItem->getNodePalette();
	QPainterPath pointrange      = getlinkPointPainterRegion(pl);  // 横版矩形，对应East，West
	if (!pointrange.isEmpty()) {
		// 绘制连接点
		painter->setPen(palette.getGlobalLinkPointBorderColor());
		if (DANodeLinkPoint::Input == pl.way) {
			painter->setBrush(palette.getInLinkPointBrush());
		} else {
			painter->setBrush(palette.getOutLinkPointBrush());
		}
		painter->drawPath(pointrange);
	}
	// 绘制文本

	painter->restore();
}

/**
 * @brief 绘制连接点文本，会先绘制连接点，再绘制连接点文本
 * @param pl
 * @param painter
 * @param option
 * @param widget
 */
void DANodeLinkPointDrawDelegate::paintLinkPointText(const DANodeLinkPoint& pl,
                                                     QPainter* painter,
                                                     const QStyleOptionGraphicsItem* option,
                                                     QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	painter->save();
	//! 对于左右位置文本处于伸出朝向的对立位置
	//! ←◁文本
	//!
	//! 文本▷→
	//!
	//!   ↑
	//!   △
	//! 文本
	//!
	//! 文本
	//!  ▽
	//!  ↓
	//!
	const DANodePalette& palette = d_ptr->mItem->getNodePalette();
	QFontMetrics fm              = painter->fontMetrics();
	const int spaceing           = 2;
	const int offset             = c_lpHWidth / 2;
	painter->setPen(QPen(palette.getTextColor()));
	// 文字高度
	QRect textRect = fm.boundingRect(pl.name);
	textRect.adjust(0, 0, spaceing, spaceing);  // 上下生长1px
	switch (pl.direction) {
	case AspectDirection::East: {
		//! 文本▷→
		textRect.moveTopLeft(QPoint(pl.position.x() - textRect.width() - offset, pl.position.y() - textRect.height() / 2));
		painter->drawText(textRect, Qt::AlignCenter, pl.name);
	} break;
	case AspectDirection::West: {
		//! ←◁文本
		textRect.moveTopLeft(QPoint(pl.position.x() + offset, pl.position.y() - textRect.height() / 2));
		painter->drawText(textRect, Qt::AlignCenter, pl.name);
	} break;
	case AspectDirection::North: {
		//!   ↑
		//!   △
		//! 文本
		textRect.moveTopLeft(QPoint(pl.position.x() - textRect.width() / 2, pl.position.y() + offset));
		painter->drawText(textRect, Qt::AlignCenter, pl.name);
	} break;
	case AspectDirection::South: {
		//! 文本
		//!  ▽
		//!  ↓
		textRect.moveTopLeft(QPoint(pl.position.x() - textRect.width() / 2, pl.position.y() - textRect.height() - offset));
		painter->drawText(textRect, Qt::AlignCenter, pl.name);
	} break;
	default:
		break;
	}

	painter->restore();
}
}  // end of namespace DA
