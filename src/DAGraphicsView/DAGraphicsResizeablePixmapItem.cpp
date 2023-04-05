#include "DAGraphicsResizeablePixmapItem.h"
#include <QPainter>
#include <QDebug>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
using namespace DA;
/**
 * @brief 因为要显示调整尺寸的8个点，因此需要调整boundingRect
 * @return
 */
DAGraphicsResizeablePixmapItem::DAGraphicsResizeablePixmapItem(QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), _aspectRatioMode(Qt::IgnoreAspectRatio), _transformationMode(Qt::FastTransformation)
{
}

DAGraphicsResizeablePixmapItem::DAGraphicsResizeablePixmapItem(const QPixmap& pixmap, QGraphicsItem* parent)
    : DAGraphicsResizeableItem(parent), _aspectRatioMode(Qt::IgnoreAspectRatio), _transformationMode(Qt::FastTransformation)
{

    _pixmap       = pixmap;
    _pixmapOrigin = pixmap;
    changeBodySize(pixmap.size());
}

/**
 * @brief 设置是否可移动
 * @param on
 */
void DAGraphicsResizeablePixmapItem::setMoveable(bool on)
{
    setFlag(QGraphicsItem::ItemIsMovable, on);
}

bool DAGraphicsResizeablePixmapItem::isMoveable() const
{
    return flags().testFlag(QGraphicsItem::ItemIsMovable);
}

void DAGraphicsResizeablePixmapItem::setSelectable(bool on)
{
    setFlag(QGraphicsItem::ItemIsSelectable, on);
}

bool DAGraphicsResizeablePixmapItem::isSelectable() const
{
    return flags().testFlag(QGraphicsItem::ItemIsSelectable);
}

void DAGraphicsResizeablePixmapItem::setPixmap(const QPixmap& pixmap)
{
    _pixmap       = pixmap;
    _pixmapOrigin = pixmap;
    setBodySize(pixmap.size());
}

const QPixmap& DAGraphicsResizeablePixmapItem::getPixmap() const
{
    return _pixmap;
}

/**
 * @brief 获取原来的尺寸的图片
 *
 * @sa DAGraphicsResizeablePixmapItem 会保留原来尺寸的图片，以便能进行缩放
 * @return
 */
const QPixmap& DAGraphicsResizeablePixmapItem::getOriginPixmap() const
{
    return _pixmapOrigin;
}

/**
 * @brief DAGraphicsResizeablePixmapItem::setTransformationMode
 * @param t
 */
void DAGraphicsResizeablePixmapItem::setTransformationMode(Qt::TransformationMode t)
{
    _transformationMode = t;
}

Qt::TransformationMode DAGraphicsResizeablePixmapItem::getTransformationMode() const
{
    return _transformationMode;
}

void DAGraphicsResizeablePixmapItem::setAspectRatioMode(Qt::AspectRatioMode t)
{
    _aspectRatioMode = t;
}

Qt::AspectRatioMode DAGraphicsResizeablePixmapItem::getAspectRatioMode() const
{
    return _aspectRatioMode;
}

void DAGraphicsResizeablePixmapItem::setBodySize(const QSizeF& s)
{
    //设置尺寸
    QSizeF ss = testBodySize(s);
    _pixmap   = _pixmapOrigin.scaled(ss.toSize(), _aspectRatioMode, _transformationMode);
    DAGraphicsResizeableItem::setBodySize(_pixmap.size());
}

void DAGraphicsResizeablePixmapItem::paintBody(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, (_transformationMode == Qt::SmoothTransformation));

    painter->drawPixmap(bodyRect.topLeft(), _pixmap);
}
