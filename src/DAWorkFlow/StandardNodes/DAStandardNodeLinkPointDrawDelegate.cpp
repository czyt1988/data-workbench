#include "DAStandardNodeLinkPointDrawDelegate.h"
#include <QGraphicsItem>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include "DAAbstractNodeGraphicsItem.h"
namespace DA
{
/**
 * @brief 连接点的宽度，针对东西方向的，如果变为南北方向，宽高要调换
 */
const int linkpoint_width = 10;
/**
 * @brief 连接点的高度，针对东西方向的，如果变为南北方向，宽高要调换
 */
const int linkpoint_height = 15;

DAStandardNodeLinkPointDrawDelegate::DAStandardNodeLinkPointDrawDelegate(DA::DAAbstractNodeGraphicsItem* i)
    : DA::DANodeLinkPointDrawDelegate(i)
{
}

DAStandardNodeLinkPointDrawDelegate::~DAStandardNodeLinkPointDrawDelegate()
{
}

void DAStandardNodeLinkPointDrawDelegate::layoutLinkPoints(QList< DA::DANodeLinkPoint >& lps, const QRectF& bodyRect)
{
	int inputCnt  = 0;
	int outputCnt = 0;
	// 连接点尺寸
	for (const DA::DANodeLinkPoint& lp : lps) {
		if (lp.isOutput()) {
			++outputCnt;
		} else {
			++inputCnt;
		}
	}
	// 离开边界的距离
	const qreal spaceSide = 1;
	// 获取出入口位置
	const DA::DAAbstractNodeGraphicsItem::LinkPointLocation inLoc = getItem()->getLinkPointLocation(
		DA::DANodeLinkPoint::Input);
	const DA::DAAbstractNodeGraphicsItem::LinkPointLocation outLoc = getItem()->getLinkPointLocation(
		DA::DANodeLinkPoint::Output);
	// 计算出入口的每次偏移量
	qreal dtIn  = 0;
	qreal dtOut = 0;
	switch (inLoc) {
	case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
	case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
		dtIn = (bodyRect.height() - 2 * spaceSide) / (inputCnt + 1.0);
		break;
	case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
	case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
		dtIn = (bodyRect.width() - 2 * spaceSide) / (inputCnt + 1.0);
	default:
		break;
	}
	switch (outLoc) {
	case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
	case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
		dtOut = (bodyRect.height() - 2 * spaceSide) / (outputCnt + 1.0);
		break;
	case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
	case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
		dtOut = (bodyRect.width() - 2 * spaceSide) / (outputCnt + 1.0);
	default:
		break;
	}
	// 计数清零
	inputCnt  = 0;
	outputCnt = 0;
	for (DA::DANodeLinkPoint& lp : lps) {
		if (lp.isInput()) {
			++inputCnt;
			switch (inLoc) {
			case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide: {
				//! 输入在左边，朝向西
				//! ←|▷
				lp.direction = DA::AspectDirection::West;
				lp.position  = QPointF(bodyRect.left() + spaceSide, bodyRect.top() + spaceSide + (inputCnt * dtIn));
			} break;
			case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide: {
				lp.direction = DA::AspectDirection::North;
				lp.position  = QPointF(bodyRect.left() + spaceSide + (inputCnt * dtIn), bodyRect.top() + spaceSide);
			} break;
			case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide: {
				lp.direction = DA::AspectDirection::East;
				lp.position  = QPointF(bodyRect.right() - spaceSide, bodyRect.top() + spaceSide + (inputCnt * dtIn));
			} break;
			case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide: {
				lp.direction = DA::AspectDirection::South;
				lp.position  = QPointF(bodyRect.left() + spaceSide + (inputCnt * dtIn), bodyRect.bottom() - spaceSide);
			} break;
			default:
				break;
			}
		} else {
			// 出口
			// 输出的位置要偏移outputOffsetLen距离
			++outputCnt;
			switch (outLoc) {
			case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide: {
				lp.direction = DA::AspectDirection::West;
				lp.position  = QPointF(bodyRect.left() + spaceSide, bodyRect.top() + spaceSide + (outputCnt * dtOut));
			} break;
			case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide: {
				lp.direction = DA::AspectDirection::North;
				lp.position  = QPointF(bodyRect.left() + spaceSide + (outputCnt * dtOut), bodyRect.top() + spaceSide);
			} break;
			case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide: {
				lp.direction = DA::AspectDirection::East;
				lp.position  = QPointF(bodyRect.right() - spaceSide, bodyRect.top() + spaceSide + (outputCnt * dtOut));
			} break;
			case DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide: {
				lp.direction = DA::AspectDirection::South;
				lp.position = QPointF(bodyRect.left() + spaceSide + (outputCnt * dtOut), bodyRect.bottom() - spaceSide);
			} break;
			default:
				break;
			}
		}
	}
}

/**
 * @brief 连接点
 *
 * 连接点规则,都是三角形△◁▷▽
 *
 * 入口：
 * -------
 * |  ▽  |
 * |▷   ◁|
 * |  △  |
 * -------
 *
 * 出口：
 *     △
 *  -------
 *  |     |
 * ◁|     |▷
 *  |     |
 *  -------
 *     ▽
 * 统一为指向右边的三角
 * @param pl
 * @return
 */
QPainterPath DAStandardNodeLinkPointDrawDelegate::getlinkPointPainterRegion(const DA::DANodeLinkPoint& pl) const
{
	if (!mEnableMultLink) {
		if (getItem()->isLinkPointLinked(pl)) {
			// 已经链接就不显示连接点
			return QPainterPath();
		}
	}
	if (pl.isInput()) {
		switch (pl.direction) {
		case DA::AspectDirection::East:
			//! ◁|→
			return closePainterPath(pl.position,
									QPointF(pl.position.x(), pl.position.y() - linkpoint_height / 2),
									QPointF(pl.position.x() - linkpoint_width, pl.position.y()),
									QPointF(pl.position.x(), pl.position.y() + linkpoint_height / 2));
		case DA::AspectDirection::West:
			//! ←|▷
			return closePainterPath(pl.position,
									QPointF(pl.position.x(), pl.position.y() - linkpoint_height / 2),
									QPointF(pl.position.x() + linkpoint_width, pl.position.y()),
									QPointF(pl.position.x(), pl.position.y() + linkpoint_height / 2));
		case DA::AspectDirection::North:  // 南北方向，高和宽调换
			//! ↑
			//! -
			//! ▽
			return closePainterPath(pl.position,
									QPointF(pl.position.x() + linkpoint_height / 2, pl.position.y()),
									QPointF(pl.position.x(), pl.position.y() + linkpoint_width),
									QPointF(pl.position.x() - linkpoint_height / 2, pl.position.y()));
		case DA::AspectDirection::South:  // 南北方向，高和宽调换
			//! △
			//! -
			//! ↓
			return closePainterPath(pl.position,
									QPointF(pl.position.x() + linkpoint_height / 2, pl.position.y()),
									QPointF(pl.position.x(), pl.position.y() - linkpoint_width),
									QPointF(pl.position.x() - linkpoint_height / 2, pl.position.y()));
			break;
		default:
			break;
		}
	} else {
		switch (pl.direction) {
		case DA::AspectDirection::East:
			//! |▷→
			return closePainterPath(pl.position,
									QPointF(pl.position.x(), pl.position.y() - linkpoint_height / 2),
									QPointF(pl.position.x() + linkpoint_width, pl.position.y()),
									QPointF(pl.position.x(), pl.position.y() + linkpoint_height / 2));
		case DA::AspectDirection::West:
			//! ←◁|
			return closePainterPath(pl.position,
									QPointF(pl.position.x(), pl.position.y() - linkpoint_height / 2),
									QPointF(pl.position.x() - linkpoint_width, pl.position.y()),
									QPointF(pl.position.x(), pl.position.y() + linkpoint_height / 2));
		case DA::AspectDirection::North:  // 南北方向，高和宽调换
			//! ↑
			//! △
			//! -
			return closePainterPath(pl.position,
									QPointF(pl.position.x() + linkpoint_height / 2, pl.position.y()),
									QPointF(pl.position.x(), pl.position.y() - linkpoint_width),
									QPointF(pl.position.x() - linkpoint_height / 2, pl.position.y()));
		case DA::AspectDirection::South:  // 南北方向，高和宽调换
			//! -
			//! ▽
			//! ↓
			return closePainterPath(pl.position,
									QPointF(pl.position.x() + linkpoint_height / 2, pl.position.y()),
									QPointF(pl.position.x(), pl.position.y() + linkpoint_width),
									QPointF(pl.position.x() - linkpoint_height / 2, pl.position.y()));
			break;
		default:
			break;
		}
	}
	return closePainterPath(pl.position,
							QPointF(pl.position.x(), pl.position.y() - linkpoint_height / 2),
							QPointF(pl.position.x() + linkpoint_width, pl.position.y()),
							QPointF(pl.position.x(), pl.position.y() + linkpoint_height / 2));
}

/**
 * @brief 获取连接点的序号
 * @param pl
 * @return
 */
int DAStandardNodeLinkPointDrawDelegate::getLinkPointIndex(const DA::DANodeLinkPoint& pl) const
{
	int index = -1;
	auto item = getItem();
	if (!item) {
		return index;
	}
	if (pl.isInput()) {
		index = item->getInputLinkPoints().indexOf(pl);
	} else {
		index = item->getOutputLinkPoints().indexOf(pl);
	}
	return index;
}

/**
 * @brief 画板
 * @return
 */
DA::DANodePalette& DAStandardNodeLinkPointDrawDelegate::palette() const
{
    return DA::DANodePalette::getGlobalPalette();
}

void DAStandardNodeLinkPointDrawDelegate::setEnableMultLink(bool on)
{
    mEnableMultLink = on;
}

bool DAStandardNodeLinkPointDrawDelegate::isEnableMultLink() const
{
    return mEnableMultLink;
}

/**
 * @brief 生成一个三角形
 * @param a
 * @param b
 * @param c
 * @return
 */
QPainterPath DAStandardNodeLinkPointDrawDelegate::makeTriangle(const QPoint& a, const QPoint& b, const QPoint& c)
{
    return closePainterPath(a, b, c);
}

QPainterPath DAStandardNodeLinkPointDrawDelegate::closePainterPath(const QPoint& a, const QPoint& b, const QPoint& c)
{
	QPainterPath res;
	res.moveTo(a);
	res.lineTo(b);
	res.lineTo(c);
	res.closeSubpath();
	return res;
}

QPainterPath DAStandardNodeLinkPointDrawDelegate::closePainterPath(const QPoint& a,
                                                                   const QPoint& b,
                                                                   const QPoint& c,
                                                                   const QPoint& d)
{
	QPainterPath res;
	res.moveTo(a);
	res.lineTo(b);
	res.lineTo(c);
	res.lineTo(d);
	res.closeSubpath();
	return res;
}

QPainterPath DAStandardNodeLinkPointDrawDelegate::closePainterPath(const QPointF& a,
                                                                   const QPointF& b,
                                                                   const QPointF& c,
                                                                   const QPointF& d)
{
	QPainterPath res;
	res.moveTo(a);
	res.lineTo(b);
	res.lineTo(c);
	res.lineTo(d);
	res.closeSubpath();
	return res;
}

/**
 * @brief 获取连接点的尺寸
 * @return
 */
QSize DAStandardNodeLinkPointDrawDelegate::getLinkPointSize() const
{
    return QSize(linkpoint_width, linkpoint_height);
}
}
