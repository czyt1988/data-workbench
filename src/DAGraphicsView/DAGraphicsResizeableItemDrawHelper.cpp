#include "DAGraphicsResizeableItemDrawHelper.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include <QGraphicsSceneHoverEvent>
/**
 * @brief 控制点信息
 */
class __DAControlPointInfo
{
public:
    enum ConnerType
    {
        TopLeft = 0,
        TopMid,
        TopRight,
        RightMid,
        BottomRight,
        BottomMid,
        BottomLeft,
        LeftMid
    };
    __DAControlPointInfo(const QRectF& r, ConnerType t) : rect(r), isHighlight(false), connerType(t)
    {
    }
    QRectF rect;
    bool isHighlight;
    ConnerType connerType;
};

class DAGraphicsResizeableItemDrawHelperPrivate
{
    DA_IMPL_PUBLIC(DAGraphicsResizeableItemDrawHelper)
public:
    DAGraphicsResizeableItemDrawHelperPrivate(QGraphicsItem* i, DAGraphicsResizeableItemDrawHelper* p);
    bool _enable;  ///< 是否允许
    QGraphicsItem* _item;
    QSize _controlerSize;  ///< 调整尺寸用的控制器的大小，默认（8，8）
    std::unique_ptr< DANodePalette > _palette;
    QList< __DAControlPointInfo > _controlPointInfos;
    QRectF _lastBoundingRect;
};
DAGraphicsResizeableItemDrawHelperPrivate::DAGraphicsResizeableItemDrawHelperPrivate(QGraphicsItem* i,
                                                                                     DAGraphicsResizeableItemDrawHelper* p)
    : _enable(true), _item(i), q_ptr(p), _controlerSize(8, 8)
{
}

///////////////

DAGraphicsResizeableItemDrawHelper::DAGraphicsResizeableItemDrawHelper(QGraphicsItem* i, bool on)
    : d_ptr(new DAGraphicsResizeableItemDrawHelperPrivate(i, this))
{
    setEnable(on);
}

DAGraphicsResizeableItemDrawHelper::~DAGraphicsResizeableItemDrawHelper()
{
}

QRectF DAGraphicsResizeableItemDrawHelper::boundingRect(const QRectF& originBRect) const
{
    if (!isEnable()) {
        return originBRect;
    }
    int offset = getControlOffset();
    if (originBRect != d_ptr->_lastBoundingRect) {
        DAGraphicsResizeableItemDrawHelper* that = const_cast< DAGraphicsResizeableItemDrawHelper* >(this);
        that->graphicsItemBoundingRectChanged(originBRect);
    }
    d_ptr->_lastBoundingRect = originBRect;
    return originBRect.adjusted(-offset, -offset, offset, offset);
}

/**
 * @brief shape要添加对应的控制点
 * @return
 */
QPainterPath DAGraphicsResizeableItemDrawHelper::shape(const QPainterPath& originShape) const
{
    if (!isEnable()) {
        return originShape;
    }
    QPainterPath p = originShape;
    for (const __DAControlPointInfo& r : qAsConst(d_ptr->_controlPointInfos)) {
        p.addRect(r.rect);
    }
    return p;
}

void DAGraphicsResizeableItemDrawHelper::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    //绘制缩放用的
    if (!isEnable()) {
        return;
    }
    if (graphicsItem()->isSelected()) {
        painter->save();
        //绘制缩放的边框
        paintResizeBorder(painter, option, widget);
        //绘制缩放的点
        paintResizeControlPoints(painter, option, widget);

        painter->restore();
    }
}

void DAGraphicsResizeableItemDrawHelper::paintResizeBorder(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    int offset                   = getControlOffset();
    const DANodePalette& palette = getNodePalette();
    QPen pen(palette.getResizeEdgeColor());
    pen.setColor(Qt::red);
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    //绘制缩放边框
    painter->drawRect(option->rect.adjusted(offset, offset, -offset, -offset));
}

void DAGraphicsResizeableItemDrawHelper::paintResizeControlPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);
    const DANodePalette& palette = getNodePalette();
    QPen pen(palette.getResizeControlEdgeColor());
    pen.setStyle(Qt::SolidLine);
    painter->setBrush(palette.getResizeControlPointBrush());
    for (const __DAControlPointInfo& r : qAsConst(d_ptr->_controlPointInfos)) {
        if (r.isHighlight) {
            painter->fillRect(r.rect, palette.getResizeControlPointBrush().color().darker());
        } else {
            painter->drawRect(r.rect);
        }
    }
}

void DAGraphicsResizeableItemDrawHelper::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    if (!isEnable()) {
        return;
    }
    if (graphicsItem()->isSelected()) {
        //选中才判断
        qDebug() << "\n >> DAGraphicsResizeableItemDrawHelper::hoverEnterEvent pos=" << event->pos();
        QPointF p = event->pos();
        for (__DAControlPointInfo& r : d_ptr->_controlPointInfos) {
            r.isHighlight = r.rect.contains(p);
            if (r.isHighlight) {
                graphicsItem()->update(r.rect);
                event->accept();
                return;
            }
        }
    }
}

/**
 * @brief DAGraphicsResizeableItemDrawHelper::hoverMoveEvent
 * @param event
 */
void DAGraphicsResizeableItemDrawHelper::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (!isEnable()) {
        return;
    }

    if (graphicsItem()->isSelected()) {
        //选中才判断
        QPointF p         = event->pos();
        bool haveInConner = false;
        for (__DAControlPointInfo& r : d_ptr->_controlPointInfos) {
            r.isHighlight = r.rect.contains(p);
            if (r.isHighlight) {
                haveInConner = true;
                switch (r.connerType) {
                case __DAControlPointInfo::TopLeft:
                case __DAControlPointInfo::BottomRight:
                    graphicsItem()->setCursor(Qt::SizeFDiagCursor);
                    break;
                case __DAControlPointInfo::TopMid:
                case __DAControlPointInfo::BottomMid:
                    graphicsItem()->setCursor(Qt::SizeVerCursor);
                    break;
                case __DAControlPointInfo::TopRight:
                case __DAControlPointInfo::BottomLeft:
                    graphicsItem()->setCursor(Qt::SizeBDiagCursor);
                    break;
                case __DAControlPointInfo::RightMid:
                case __DAControlPointInfo::LeftMid:
                    graphicsItem()->setCursor(Qt::SizeHorCursor);
                    break;
                default:
                    break;
                }
                graphicsItem()->update(r.rect);
                event->accept();
                return;
            }
        }
        //这里说明都没在控制点上
        if (graphicsItem()->hasCursor()) {
            graphicsItem()->unsetCursor();
        }
    }
}

void DAGraphicsResizeableItemDrawHelper::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    if (!isEnable()) {
        return;
    }
    if (graphicsItem()->hasCursor()) {
        graphicsItem()->unsetCursor();
    }
    for (__DAControlPointInfo& r : d_ptr->_controlPointInfos) {
        //离开是把所有都刷新
        r.isHighlight = false;
        graphicsItem()->update(r.rect);
    }
}

/**
 * @brief 获取管理的QGraphicsItem
 * @return
 */
QGraphicsItem* DAGraphicsResizeableItemDrawHelper::graphicsItem()
{
    return d_ptr->_item;
}

/**
 * @brief 获取控制器的大小
 * @param s
 */
void DAGraphicsResizeableItemDrawHelper::setControlerSize(const QSize& s)
{
    d_ptr->_controlerSize = s;
}

QSize DAGraphicsResizeableItemDrawHelper::getControlerSize() const
{
    return d_ptr->_controlerSize;
}

int DAGraphicsResizeableItemDrawHelper::getControlOffset() const
{
    return d_ptr->_controlerSize.width() / 2 + 1;
}

void DAGraphicsResizeableItemDrawHelper::setNodePalette(const DANodePalette& pl)
{
    if (!(d_ptr->_palette)) {
        d_ptr->_palette.reset(new DANodePalette);
    }
    *(d_ptr->_palette) = pl;
}

const DANodePalette& DAGraphicsResizeableItemDrawHelper::getNodePalette() const
{
    if (d_ptr->_palette) {
        return *(d_ptr->_palette);
    }
    return DANodePalette::getGlobalPalette();
}

/**
 * @brief 初始化控制点
 */
void DAGraphicsResizeableItemDrawHelper::graphicsItemBoundingRectChanged(const QRectF& originBRect)
{
    d_ptr->_controlPointInfos.clear();
    //原始的控制点矩形
    QRectF cr        = QRectF(0, 0, d_ptr->_controlerSize.width(), d_ptr->_controlerSize.height());
    qreal halfWidth  = originBRect.width() / 2;
    qreal halfHeight = originBRect.height() / 2;
    //偏移
    QPointF offset(cr.width() / 2, cr.height() / 2);
    // topLeft
    cr.moveTo(originBRect.topLeft() - offset);
    d_ptr->_controlPointInfos.append(__DAControlPointInfo(cr, __DAControlPointInfo::TopLeft));
    // top-mid
    cr.moveTo(QPointF(originBRect.left() + halfWidth, originBRect.top()) - offset);
    d_ptr->_controlPointInfos.append(__DAControlPointInfo(cr, __DAControlPointInfo::TopMid));
    // top-right
    cr.moveTo(originBRect.topRight() - offset);
    d_ptr->_controlPointInfos.append(__DAControlPointInfo(cr, __DAControlPointInfo::TopRight));
    // mid-right
    cr.moveTo(QPointF(originBRect.right(), originBRect.top() + halfHeight) - offset);
    d_ptr->_controlPointInfos.append(__DAControlPointInfo(cr, __DAControlPointInfo::RightMid));
    // right-bottom
    cr.moveTo(originBRect.bottomRight() - offset);
    d_ptr->_controlPointInfos.append(__DAControlPointInfo(cr, __DAControlPointInfo::BottomRight));
    // bottom-mid
    cr.moveTo(QPointF(originBRect.left() + halfWidth, originBRect.bottom()) - offset);
    d_ptr->_controlPointInfos.append(__DAControlPointInfo(cr, __DAControlPointInfo::BottomMid));
    // bottom-left
    cr.moveTo(originBRect.bottomLeft() - offset);
    d_ptr->_controlPointInfos.append(__DAControlPointInfo(cr, __DAControlPointInfo::BottomLeft));
    // left-mid
    cr.moveTo(QPointF(originBRect.left(), originBRect.top() + halfHeight) - offset);
    d_ptr->_controlPointInfos.append(__DAControlPointInfo(cr, __DAControlPointInfo::LeftMid));
}

/**
 * @brief 设置是否helper可用
 * @param on
 */
void DAGraphicsResizeableItemDrawHelper::setEnable(bool on)
{
    d_ptr->_enable = on;
    if (on) {
        if (!graphicsItem()->acceptHoverEvents()) {
            graphicsItem()->setAcceptHoverEvents(on);
        }
    }
}

bool DAGraphicsResizeableItemDrawHelper::isEnable() const
{
    return d_ptr->_enable;
}
