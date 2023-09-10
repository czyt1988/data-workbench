#include "DANodeLinkPointDrawDelegate.h"
#include <QGraphicsItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include "DAAbstractNodeGraphicsItem.h"
#include "DANodePalette.h"

namespace DA
{
class DANodeLinkPointDrawDelegate::PrivateData
{
    DA_DECLARE_PUBLIC(DANodeLinkPointDrawDelegate)
public:
    PrivateData(DANodeLinkPointDrawDelegate* p);
    DAAbstractNodeGraphicsItem* mItem;  ///< 绑定的item
};
}  // end of namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

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
    for (const DANodeLinkPoint& pl : qAsConst(pls)) {
        paintLinkPoint(pl, painter, option, widget);
    }
}

/**
 * @brief 等同DAAbstractNodeGraphicsItem::getLinkPoints
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
 * @brief 获取连接点的绘图区域
 *
 * DAAbstractNodeGraphicsItem::changeLinkPointPos函数用于定义DANodeLinkPoint的信息，此函数根据定义的位置信息获取连接点的绘图区域
 * @sa DAAbstractNodeGraphicsItem::changeLinkPointPos
 * @param pl
 * @return
 */
QPainterPath DANodeLinkPointDrawDelegate::getlinkPointPainterRegion(const DANodeLinkPoint& pl) const
{
    int lpHWidth  = 14;  //连接点水平时的宽度
    int lpHHeight = 10;  //连接点水平时的高度
    QPainterPath region;
    switch (pl.direction) {
    case DANodeLinkPoint::East:
    case DANodeLinkPoint::West:
        region.addRect(QRectF(pl.position.x() - lpHWidth / 2, pl.position.y() - lpHHeight / 2, lpHWidth, lpHHeight));
        break;
    case DANodeLinkPoint::North:
    case DANodeLinkPoint::South:
        region.addRect(QRectF(pl.position.x() - lpHHeight / 2, pl.position.y() - lpHWidth / 2, lpHHeight, lpHWidth));
        break;
    default:
        region.addRect(QRectF(pl.position.x() - lpHWidth / 2, pl.position.y() - lpHWidth / 2, lpHWidth, lpHWidth));
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
    //连接点是一个长方形，6X8,点中心是长方形中心
    //先把painter坐标变换到点处
    const DANodePalette& palette = d_ptr->mItem->getNodePalette();
    QPainterPath pointrange      = getlinkPointPainterRegion(pl);  // 横版矩形，对应East，West
    painter->setPen(palette.getGlobalLinkPointBorderColor());
    if (DANodeLinkPoint::Input == pl.way) {
        painter->setBrush(palette.getInLinkPointBrush());
    } else {
        painter->setBrush(palette.getOutLinkPointBrush());
    }
    painter->drawPath(pointrange);
    painter->restore();
}
