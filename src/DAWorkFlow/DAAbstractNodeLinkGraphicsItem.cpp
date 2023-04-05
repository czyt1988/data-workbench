#include "DAAbstractNodeLinkGraphicsItem.h"
#include <QPainter>
#include <QDebug>
#include "DAAbstractNodeGraphicsItem.h"
#include "DANodeGraphicsScene.h"
#include "DANodePalette.h"
#include <math.h>
#include "DAAbstractNode.h"
#include <QGraphicsSimpleTextItem>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsTextItem>
//是否在连线中间加点
/**
 *
 * @def 在链接线中间点加个圆
 */
#define ADD_POINT_ON_LINK_LINE_CENTER 0

namespace DA
{
class DAAbstractNodeLinkGraphicsItemPrivate
{
    DA_IMPL_PUBLIC(DAAbstractNodeLinkGraphicsItem)
public:
public:
    DAAbstractNodeLinkGraphicsItemPrivate(DAAbstractNodeLinkGraphicsItem* p);
    DANodeGraphicsScene* nodeScene() const;
    void setLinkPointNameVisible(bool on, DAAbstractNodeLinkGraphicsItem::Orientations o);
    bool isLinkPointNameVisible(DAAbstractNodeLinkGraphicsItem::Orientations o) const;
    void updateLinkPointNameText(QGraphicsSimpleTextItem* item, const QPointF& p, const DANodeLinkPoint& pl, int offset);
    void updateLinkPointNameText();
    void setPointTextColor(const QColor& c, DAAbstractNodeLinkGraphicsItem::Orientations o);
    QColor getPointTextColor(DAAbstractNodeLinkGraphicsItem::Orientations o) const;
    void setPointTextPositionOffset(int offset, DAAbstractNodeLinkGraphicsItem::Orientations o);
    int getPointTextPositionOffset(DAAbstractNodeLinkGraphicsItem::Orientations o) const;
    bool isStartLinking() const;
    bool isLinked() const;
    void updateTextPos();

public:
    DAAbstractNodeLinkGraphicsItem::LinkLineStyle _linkLineStyle;
    DAAbstractNodeGraphicsItem* _fromItem;
    DAAbstractNodeGraphicsItem* _toItem;
    DANodeLinkPoint _fromPoint;
    DANodeLinkPoint _toPoint;
    QPointF _fromPos;
    QPointF _toPos;
    QRectF _boundingRect;         ///< 记录boundingRect
    qreal _bezierControlScale;    ///<贝塞尔曲线的控制点的缩放比例
    QPainterPath _linePath;       ///< 通过点得到的绘图线段
    QPainterPath _lineShapePath;  ///_linePath的轮廓，用于shape函数
    QPen _linePen;                ///< 线的画笔
    QGraphicsSimpleTextItem* _fromTextItem;
    QGraphicsSimpleTextItem* _toTextItem;
    QPair< int, int > _pointTextPositionOffset;  ///< 记录文本和连接点的偏移量，默认为10
    QGraphicsSimpleTextItem* _textItem;          ///< 文本item，文本item默认为false
    QPointF _textPosProportion;                  ///< 文本位置占比
    bool _autoDetachLink;                        ///< 默认为true，在析构时自动detach link
    QPointF _linkCenterPoint;                    ///< 记录中心点
    int _textItemSpace;                          ///< 文字离中心点的距离
    //端点样式
    DAAbstractNodeLinkGraphicsItem::EndPointType _fromEndPointType;  ///< from的端点样式
    DAAbstractNodeLinkGraphicsItem::EndPointType _toEndPointType;    ///< to的端点样式
    QPainterPath _fromEndPointPainterPath;                           ///< 记录from的端点样式
    QPainterPath _toEndPointPainterPath;                             ///< 记录to的端点样式
    int _endPointSize;                                               ///< 记录端点大小
};
}  // end of namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAAbstractNodeLinkGraphicsItemPrivate
//===================================================
DAAbstractNodeLinkGraphicsItemPrivate::DAAbstractNodeLinkGraphicsItemPrivate(DAAbstractNodeLinkGraphicsItem* p)
    : q_ptr(p)
    , _linkLineStyle(DAAbstractNodeLinkGraphicsItem::LinkLineKnuckle)
    , _fromItem(nullptr)
    , _toItem(nullptr)
    , _fromPos(0, 0)
    , _toPos(100, 100)
    , _boundingRect(0, 0, 100, 100)
    , _bezierControlScale(0.25)
    , _pointTextPositionOffset(10, 10)
    , _textItem(nullptr)
    , _textPosProportion(0.5, 0.5)
    , _autoDetachLink(true)
    , _linkCenterPoint(50, 50)
    , _textItemSpace(5)
    , _fromEndPointType(DAAbstractNodeLinkGraphicsItem::EndPointNone)
    , _toEndPointType(DAAbstractNodeLinkGraphicsItem::EndPointNone)
    , _endPointSize(12)
{
    _linePen      = QPen(DANodePalette::getGlobalLinkLineColor());
    _fromTextItem = new QGraphicsSimpleTextItem(p);
    _toTextItem   = new QGraphicsSimpleTextItem(p);
    setLinkPointNameVisible(false, DAAbstractNodeLinkGraphicsItem::OrientationBoth);
}

DANodeGraphicsScene* DAAbstractNodeLinkGraphicsItemPrivate::nodeScene() const
{
    return (qobject_cast< DANodeGraphicsScene* >(q_ptr->scene()));
}

void DAAbstractNodeLinkGraphicsItemPrivate::setLinkPointNameVisible(bool on, DAAbstractNodeLinkGraphicsItem::Orientations o)
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationFrom:
        _fromTextItem->setVisible(on);
        break;

    case DAAbstractNodeLinkGraphicsItem::OrientationTo:
        _toTextItem->setVisible(on);
        break;

    default:
        _fromTextItem->setVisible(on);
        _toTextItem->setVisible(on);
        break;
    }
}

bool DAAbstractNodeLinkGraphicsItemPrivate::isLinkPointNameVisible(DAAbstractNodeLinkGraphicsItem::Orientations o) const
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationFrom:
        return (_fromTextItem->isVisible());

    case DAAbstractNodeLinkGraphicsItem::OrientationTo:
        return (_toTextItem->isVisible());

    default:
        break;
    }
    return (_fromTextItem->isVisible() && _toTextItem->isVisible());
}

void DAAbstractNodeLinkGraphicsItemPrivate::updateLinkPointNameText(QGraphicsSimpleTextItem* item,
                                                                    const QPointF& p,
                                                                    const DANodeLinkPoint& pl,
                                                                    int offset)
{
    item->setText(pl.name);
    int hoff = item->boundingRect().height();

    hoff /= 2;
    int w = item->boundingRect().width();

    switch (pl.direction) {
    case DANodeLinkPoint::East:
        item->setRotation(0);
        item->setPos(p.x() + offset, p.y() - hoff);
        break;

    case DANodeLinkPoint::South:
        item->setRotation(90);
        item->setPos(p.x() + hoff, p.y() + offset);
        break;

    case DANodeLinkPoint::West:
        item->setRotation(0);
        item->setPos(p.x() - w - offset, p.y() - hoff);
        break;

    case DANodeLinkPoint::North:
        item->setRotation(90);
        item->setPos(p.x() + hoff, p.y() - w - offset);
        break;

    default:
        break;
    }
}

void DAAbstractNodeLinkGraphicsItemPrivate::updateLinkPointNameText()
{
    updateLinkPointNameText(_fromTextItem, _fromPos, _fromPoint, _pointTextPositionOffset.first);
    updateLinkPointNameText(_toTextItem, _toPos, _toPoint, _pointTextPositionOffset.second);
}

void DAAbstractNodeLinkGraphicsItemPrivate::setPointTextColor(const QColor& c, DAAbstractNodeLinkGraphicsItem::Orientations o)
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationFrom:
        _fromTextItem->setBrush(c);
        break;

    case DAAbstractNodeLinkGraphicsItem::OrientationTo:
        _toTextItem->setBrush(c);
        break;

    default:
        _fromTextItem->setBrush(c);
        _toTextItem->setBrush(c);
        break;
    }
}

QColor DAAbstractNodeLinkGraphicsItemPrivate::getPointTextColor(DAAbstractNodeLinkGraphicsItem::Orientations o) const
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationFrom:
        return (_fromTextItem->brush().color());

    case DAAbstractNodeLinkGraphicsItem::OrientationTo:
        return (_toTextItem->brush().color());

    default:
        break;
    }
    return (QColor());
}

void DAAbstractNodeLinkGraphicsItemPrivate::setPointTextPositionOffset(int offset, DAAbstractNodeLinkGraphicsItem::Orientations o)
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationFrom:
        _pointTextPositionOffset.first = offset;
        break;

    case DAAbstractNodeLinkGraphicsItem::OrientationTo:
        _pointTextPositionOffset.second = offset;
        break;

    default:
        _pointTextPositionOffset.first  = offset;
        _pointTextPositionOffset.second = offset;
        break;
    }
}

int DAAbstractNodeLinkGraphicsItemPrivate::getPointTextPositionOffset(DAAbstractNodeLinkGraphicsItem::Orientations o) const
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationFrom:
        return (_pointTextPositionOffset.first);

    case DAAbstractNodeLinkGraphicsItem::OrientationTo:
        return (_pointTextPositionOffset.second);

    default:
        break;
    }
    return (0);
}

/**
 * @brief 处于连接状态中，开始有，结束还未有
 * @return
 */
bool DAAbstractNodeLinkGraphicsItemPrivate::isStartLinking() const
{
    return ((_fromItem != nullptr) && (_toItem == nullptr));
}

/**
 * @brief 已经完成连接
 * @return
 */
bool DAAbstractNodeLinkGraphicsItemPrivate::isLinked() const
{
    return ((_fromItem != nullptr) && (_toItem != nullptr));
}
/**
 * @brief 更新文本位置
 */
void DAAbstractNodeLinkGraphicsItemPrivate::updateTextPos()
{
    if (nullptr == _textItem) {
        return;
    }
    _textItem->setPos(_linkCenterPoint.x() + _textItemSpace, _linkCenterPoint.y());
}

//////////////////////////////////////////////////////////////
/// DAAbstractNodeLinkGraphicsItem
//////////////////////////////////////////////////////////////

DAAbstractNodeLinkGraphicsItem::DAAbstractNodeLinkGraphicsItem(QGraphicsItem* p)
    : QGraphicsItem(p), d_ptr(new DAAbstractNodeLinkGraphicsItemPrivate(this))
{
    setFlags(flags() | ItemIsSelectable);
    setEndPointType(OrientationFrom, EndPointNone);
    setEndPointType(OrientationTo, EndPointTriangType);
    setZValue(-1);  //连接线在-1层，这样避免在节点上面
}

DAAbstractNodeLinkGraphicsItem::DAAbstractNodeLinkGraphicsItem(DAAbstractNodeGraphicsItem* from,
                                                               const DA::DANodeLinkPoint& pl,
                                                               QGraphicsItem* p)
    : QGraphicsItem(p), d_ptr(new DAAbstractNodeLinkGraphicsItemPrivate(this))
{
    setFlags(flags() | ItemIsSelectable);
    attachFrom(from, pl);
    setEndPointType(OrientationBoth, EndPointTriangType);
    setZValue(-1);  //连接线在-1层，这样避免在节点上面
}

DAAbstractNodeLinkGraphicsItem::~DAAbstractNodeLinkGraphicsItem()
{
    //析构时d调用FCAbstractNodeGraphicsItem::callItemLinkIsDestroying移除item对应记录的link
    if (d_ptr->_autoDetachLink) {
        detachFrom();
        detachTo();
    }
}

/**
 * @brief 设置连线样式
 * @param s
 */
void DAAbstractNodeLinkGraphicsItem::setLinkLineStyle(LinkLineStyle s)
{
    d_ptr->_linkLineStyle = s;
    updateBoundingRect();
}

/**
 * @brief 获取连线样式
 * @return
 */
DAAbstractNodeLinkGraphicsItem::LinkLineStyle DAAbstractNodeLinkGraphicsItem::getLinkLineStyle() const
{
    return d_ptr->_linkLineStyle;
}

/**
 * @brief 自动根据fromitem来更新位置
 * @note 如果设置了toitem，会调用@sa updateBoundingRect 来更新boundingRect
 */
void DAAbstractNodeLinkGraphicsItem::updatePos()
{
    DANodeGraphicsScene* sc = d_ptr->nodeScene();

    if ((nullptr == d_ptr->_fromItem) || (nullptr == sc)) {
        return;
    }
    setPos(d_ptr->_fromItem->mapToScene(d_ptr->_fromPoint.position));
    if (d_ptr->_toItem) {
        updateBoundingRect();
    }
}

void DAAbstractNodeLinkGraphicsItem::updateBoundingRect()
{
    DANodeGraphicsScene* sc = d_ptr->nodeScene();

    if (nullptr == sc) {
        return;
    }
    //! 通过调用prepareGeometryChange()通知范围变更，避免出现残影
    prepareGeometryChange();
    d_ptr->_fromPos = QPointF(0, 0);
    d_ptr->_toPos   = QPointF(100, 100);
    if ((d_ptr->_fromItem == nullptr) && (d_ptr->_toItem == nullptr)) {
        //都是空退出
        d_ptr->_fromPoint.direction = DANodeLinkPoint::East;
        d_ptr->_toPoint.direction   = DANodeLinkPoint::West;
        generatePainterPath();
        d_ptr->_boundingRect = d_ptr->_linePath.boundingRect().adjusted(-2, -2, 2, 2);  //留足选中后画笔变宽的绘制余量
        return;
    } else if ((d_ptr->_fromItem != nullptr) && (d_ptr->_toItem == nullptr)) {
        //只设定了一个from
        // to要根据scene的鼠标位置实时刷新
        d_ptr->_toPos = mapFromScene(sc->getCurrentMouseScenePos());
        // 为了不覆盖点击，d_ptr->_toPos要做2像素偏移
        if (d_ptr->_toPos.x() > d_ptr->_fromPos.x()) {
            d_ptr->_toPos.rx() -= 2;
        } else {
            d_ptr->_toPos.rx() += 2;
        }
        if (d_ptr->_toPos.y() > d_ptr->_fromPos.y()) {
            d_ptr->_toPos.ry() -= 2;
        } else {
            d_ptr->_toPos.ry() += 2;
        }
        d_ptr->_toPoint.direction = DANodeLinkPoint::oppositeDirection(d_ptr->_fromPoint.direction);
        generatePainterPath();
        d_ptr->_boundingRect = d_ptr->_linePath.boundingRect().adjusted(-2, -2, 2, 2);  //留足选中后画笔变宽的绘制余量
    } else if ((d_ptr->_fromItem != nullptr) && (d_ptr->_toItem != nullptr)) {
        //两个都不为空
        d_ptr->_toPos = mapFromItem(d_ptr->_toItem, d_ptr->_toPoint.position);
        generatePainterPath();
        d_ptr->_boundingRect = d_ptr->_linePath.boundingRect().adjusted(-2, -2, 2, 2);  //留足选中后画笔变宽的绘制余量
    } else {
        generatePainterPath();
        d_ptr->_boundingRect = d_ptr->_linePath.boundingRect().adjusted(-2, -2, 2, 2);  //留足选中后画笔变宽的绘制余量
        //        qDebug() << "occ unknow link type,please check!, from item:" << d_ptr->_fromItem << " to item:" << d_ptr->_toItem;
    }
    d_ptr->updateLinkPointNameText();
    d_ptr->updateTextPos();
}

/**
 * @brief 通过两个点形成一个矩形，两个点总能形成一个矩形，如果重合，返回一个空矩形
 * @param p0
 * @param p1
 * @return
 */
QRectF DAAbstractNodeLinkGraphicsItem::rectFromTwoPoint(const QPointF& p0, const QPointF& p1)
{
    return (QRectF(QPointF(qMin(p0.x(), p1.x()), qMin(p0.y(), p1.y())), QPointF(qMax(p0.x(), p1.x()), qMax(p0.y(), p1.y()))));
}

/**
 * @brief 延长线，以一个方向和距离延伸
 * @param orgPoint 原始点
 * @param d 伸出方向
 * @param externLen 伸出长度
 * @return 得到控制点
 */
QPointF DAAbstractNodeLinkGraphicsItem::elongation(const QPointF& orgPoint, DANodeLinkPoint::Direction d, qreal externLen)
{
    switch (d) {
    case DANodeLinkPoint::East:
        return (QPointF(orgPoint.x() + externLen, orgPoint.y()));

    case DANodeLinkPoint::South:
        return (QPointF(orgPoint.x(), orgPoint.y() + externLen));

    case DANodeLinkPoint::West:
        return (QPointF(orgPoint.x() - externLen, orgPoint.y()));

    case DANodeLinkPoint::North:
        return (QPointF(orgPoint.x(), orgPoint.y() - externLen));
    }
    return (orgPoint);
}

/**
 * @brief 计算两个点的距离
 * @param a
 * @param b
 * @return
 */
qreal DAAbstractNodeLinkGraphicsItem::pointLength(const QPointF& a, const QPointF& b)
{
    return (pow((a.x() - b.x()) * (a.x() - b.x()) + (a.y() - b.y()) * (a.y() - b.y()), 0.5));
}

/**
 * @brief 设置贝塞尔曲线的控制点的缩放比例
 *
 * FCAbstractNodeLinkGraphicsItem在连线时按照两个连接点的方向延伸出贝塞尔曲线的控制点，贝塞尔曲线的控制点的长度w = length * bezierControlScale，
 * 其中length是两个连接点的绝对距离，可以通过@sa pointLength 计算得到，bezierControlScale，就是这个延伸的比例，如果越大，曲率越明显
 * @param rate 默认为0.25
 */
void DAAbstractNodeLinkGraphicsItem::setBezierControlScale(qreal rate)
{
    d_ptr->_bezierControlScale = rate;
}

/**
 * @brief 获取贝塞尔曲线的控制点的缩放比例
 * @return
 */
qreal DAAbstractNodeLinkGraphicsItem::getBezierControlScale() const
{
    return (d_ptr->_bezierControlScale);
}

/**
 * @brief 设置线的画笔
 * @param p
 */
void DAAbstractNodeLinkGraphicsItem::setLinePen(const QPen& p)
{
    d_ptr->_linePen = p;
}

/**
 * @brief 返回当前画笔
 * @return
 */
QPen DAAbstractNodeLinkGraphicsItem::getLinePen() const
{
    return (d_ptr->_linePen);
}

/**
 * @brief 设置是否显示连接点的文本
 * @param on
 */
void DAAbstractNodeLinkGraphicsItem::setLinkPointNameVisible(bool on, Orientations o)
{
    d_ptr->setLinkPointNameVisible(on, o);
}

/**
 * @brief 是否显示连接点的文本
 * @return
 */
bool DAAbstractNodeLinkGraphicsItem::isLinkPointNameVisible(Orientations o) const
{
    return (d_ptr->isLinkPointNameVisible(o));
}

/**
 * @brief 设置连接点显示的颜色
 * @param c
 * @param o
 */
void DAAbstractNodeLinkGraphicsItem::setLinkPointNameTextColor(const QColor& c, DAAbstractNodeLinkGraphicsItem::Orientations o)
{
    d_ptr->setPointTextColor(c, o);
}

/**
 * @brief 获取连接点显示的颜色
 * @param o 不能指定OrientationBoth，指定OrientationBoth返回QColor()
 * @return
 */
QColor DAAbstractNodeLinkGraphicsItem::getLinkPointNameTextColor(DAAbstractNodeLinkGraphicsItem::Orientations o) const
{
    return (d_ptr->getPointTextColor(o));
}

/**
 * @brief 设置文本和连接点的偏移量，默认为10
 * @param offset
 * @param o
 */
void DAAbstractNodeLinkGraphicsItem::setLinkPointNamePositionOffset(int offset, DAAbstractNodeLinkGraphicsItem::Orientations o)
{
    d_ptr->setPointTextPositionOffset(offset, o);
}

/**
 * @brief 文本和连接点的偏移量
 * @param o 不能指定OrientationBoth，指定OrientationBoth返回0
 * @return 指定OrientationBoth返回0
 */
int DAAbstractNodeLinkGraphicsItem::getLinkPointNamePositionOffset(DAAbstractNodeLinkGraphicsItem::Orientations o) const
{
    return (d_ptr->getPointTextPositionOffset(o));
}

QGraphicsSimpleTextItem* DAAbstractNodeLinkGraphicsItem::getFromTextItem() const
{
    return (d_ptr->_fromTextItem);
}

QGraphicsSimpleTextItem* DAAbstractNodeLinkGraphicsItem::getToTextItem() const
{
    return (d_ptr->_toTextItem);
}

QRectF DAAbstractNodeLinkGraphicsItem::boundingRect() const
{
    return (d_ptr->_boundingRect);
}

QPainterPath DAAbstractNodeLinkGraphicsItem::shape() const
{
    return (d_ptr->_lineShapePath);
}

void DAAbstractNodeLinkGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->save();
    paintLinkLine(painter, option, widget, d_ptr->_linePath);
    paintEndPoint(painter, option, d_ptr->_fromPos, d_ptr->_fromPoint);
    paintEndPoint(painter, option, d_ptr->_toPos, d_ptr->_toPoint);
    painter->restore();
}

/**
 * @brief 绘制连接线
 * @param painter
 * @param option
 * @param widget
 * @param linkPath
 */
void DAAbstractNodeLinkGraphicsItem::paintLinkLine(QPainter* painter,
                                                   const QStyleOptionGraphicsItem* option,
                                                   QWidget* widget,
                                                   const QPainterPath& linkPath)
{
    Q_UNUSED(widget);
    QPen pen = d_ptr->_linePen;

    if (d_ptr->isStartLinking()) {
        pen.setStyle(Qt::DashLine);
    }
    if (option->state.testFlag(QStyle::State_Selected)) {
        //说明选中了
        pen.setWidth(pen.width() + 2);
        pen.setColor(pen.color().darker(150));
    }
    painter->setPen(pen);
    painter->drawPath(linkPath);
#if ADD_POINT_ON_LINK_LINE_CENTER
    painter->setBrush(QBrush(pen.color()));
    painter->drawEllipse(d_ptr->_linkCenterPoint, d_ptr->_textItemSpace, d_ptr->_textItemSpace);
#endif
}

/**
 * @brief DAAbstractNodeLinkGraphicsItem::paintArrow
 * @param painter
 * @param arrowType 箭头类型
 * @param p 箭头尖端的点
 * @param rotate 箭头的旋转角度，所有箭头默认是箭头都是尖朝上（↑）
 * @sa getLinkEndPointType
 * @sa getEndPointSize
 */
void DAAbstractNodeLinkGraphicsItem::paintEndPoint(QPainter* painter,
                                                   const QStyleOptionGraphicsItem* option,
                                                   const QPointF& p,
                                                   const DANodeLinkPoint& pl)
{
    if (d_ptr->_fromEndPointType == EndPointNone && d_ptr->_toEndPointType == EndPointNone) {
        return;
    }
    //根据DANodeLinkPoint计算旋转的角度
    painter->save();
    QPen pen = d_ptr->_linePen;
    if (option->state.testFlag(QStyle::State_Selected)) {
        //说明选中了
        pen.setWidth(pen.width() + 2);
        pen.setColor(pen.color().darker(150));
    }
    painter->setPen(pen);
    painter->setBrush(pen.color());
    painter->translate(p);
    qreal rotate = 0;
    if (d_ptr->_linkLineStyle == LinkLineStraight) {
        //直线的角度需要计算
        QLineF lf(d_ptr->_fromPos, d_ptr->_toPos);
        if (pl.isOutput()) {
            rotate = 270 - lf.angle();
        } else {
            rotate = 90 - lf.angle();
        }
    } else {
        switch (pl.direction) {
        case DANodeLinkPoint::East:
            rotate = 270;
            break;
        case DANodeLinkPoint::West:
            rotate = 90;
            break;
        case DANodeLinkPoint::South:
            rotate = 0;
            break;
        case DANodeLinkPoint::North:
            rotate = 180;
            break;
        default:
            break;
        }
    }
    painter->rotate(rotate);
    if (pl.isOutput() && d_ptr->_fromEndPointType != EndPointNone) {
        painter->drawPath(d_ptr->_fromEndPointPainterPath);
    } else if (pl.isInput() && d_ptr->_toEndPointType != EndPointNone) {
        painter->drawPath(d_ptr->_toEndPointPainterPath);
    }
    painter->restore();
}

/**
 * @brief 生成箭头，所有生成的箭头都是尖朝上（↑），绘制的时候需要根据情况进行旋转
 * @param arrowType
 * @param size 箭头↑的高度
 * @return
 */
QPainterPath DAAbstractNodeLinkGraphicsItem::generateEndPointPainterPath(DAAbstractNodeLinkGraphicsItem::EndPointType epType, int size)
{
    QPainterPath path;
    switch (epType) {
    case EndPointTriangType:  //三角形
        path.moveTo(0, 0);
        path.lineTo(-size / 2, size);
        path.lineTo(size / 2, size);
        path.lineTo(0, 0);
        break;
    default:
        break;
    }
    return path;
}

/**
 * @brief 从item的出口开始进行连接
 * @param item
 * @param name
 * @return
 */
bool DAAbstractNodeLinkGraphicsItem::attachFrom(DAAbstractNodeGraphicsItem* item, const QString& name)
{
    DANodeLinkPoint pl = item->getOutputLinkPoint(name);
    return attachFrom(item, pl);
}

bool DAAbstractNodeLinkGraphicsItem::attachFrom(DAAbstractNodeGraphicsItem* item, const DANodeLinkPoint& pl)
{
    if (!item->isHaveLinkPoint(pl)) {
        qDebug() << QObject::tr("Item have not out put link point:") << pl;
        item->prepareLinkOutputFailed(pl);
        return (false);
    }
    if (!pl.isOutput()) {
        // from必须从out出发
        qDebug() << QObject::tr("Link from must attach an output point");
        item->prepareLinkOutputFailed(pl);
        return (false);
    }
    d_ptr->_fromItem  = item;
    d_ptr->_fromPoint = pl;
    d_ptr->updateLinkPointNameText();
    item->linked(this, pl);
    item->prepareLinkOutputSucceed(pl);
    return (true);
}

/**
 * @brief 清空from节点
 *
 * 在nodeitem删除时会触发
 *
 * 断开时，node的连接就已经断开，因此，对workflow来讲，断开连接是无需等到detachto调用
 */
void DAAbstractNodeLinkGraphicsItem::detachFrom()
{
    if (d_ptr->_fromItem) {
        d_ptr->_fromItem->linkItemRemoved(this, d_ptr->_fromPoint);
        d_ptr->_fromItem->node()->detachLink(d_ptr->_fromPoint.name);  //断开连接
        d_ptr->_fromItem = nullptr;
    }
    d_ptr->_fromPoint = DANodeLinkPoint();
}

bool DAAbstractNodeLinkGraphicsItem::attachTo(DAAbstractNodeGraphicsItem* item, const QString& name)
{
    DANodeLinkPoint pl = item->getInputLinkPoint(name);
    return attachTo(item, pl);
}

bool DAAbstractNodeLinkGraphicsItem::attachTo(DAAbstractNodeGraphicsItem* item, const DANodeLinkPoint& pl)
{
    if (!item->isHaveLinkPoint(pl)) {
        qDebug() << QObject::tr("Item have not in put link point:") << pl;
        item->prepareLinkInputFailed(pl, this);
        return (false);
    }
    if (!pl.isInput()) {
        // to必须到in
        qDebug() << QObject::tr("Link to must attach an input point");
        item->prepareLinkInputFailed(pl, this);
        return (false);
    }
    //这个函数才完成一个节点的连接
    DAAbstractNode::SharedPointer fnode = d_ptr->_fromItem->node();
    DAAbstractNode::SharedPointer tnode = item->node();
    if (!fnode->linkTo(d_ptr->_fromPoint.name, tnode, pl.name)) {
        item->prepareLinkInputFailed(pl, this);
        return (false);
    }
    d_ptr->_toItem  = item;
    d_ptr->_toPoint = pl;
    d_ptr->updateLinkPointNameText();
    item->linked(this, pl);
    item->prepareLinkInputSucceed(pl, this);
    return (true);
}

/**
 * @brief 断开to点
 *
 * 断开时，node的连接就已经断开，因此，对workflow来讲，断开连接是无需等到detachfrom,detachTo两个一起调用，
 * 任意一个detach操作都会对workflow的node断开连接
 */
void DAAbstractNodeLinkGraphicsItem::detachTo()
{
    if (d_ptr->_toItem) {
        d_ptr->_toItem->linkItemRemoved(this, d_ptr->_toPoint);
        d_ptr->_toItem->node()->detachLink(d_ptr->_toPoint.name);  //断开连接
        d_ptr->_toItem = nullptr;
    }
    d_ptr->_toPoint = DANodeLinkPoint();
}

/**
 * @brief 已经连接完成，在from和to都有节点时，返回true
 * @return
 */
bool DAAbstractNodeLinkGraphicsItem::isLinked() const
{
    return d_ptr->isLinked();
}
/**
 * @brief 更新连接点信息
 * @param pl
 */
void DAAbstractNodeLinkGraphicsItem::updateFromLinkPointInfo(const DANodeLinkPoint& pl)
{
    d_ptr->_fromPoint = pl;
    updatePos();
    updateBoundingRect();
}
/**
 * @brief 更新连接点信息
 * @param pl
 */
void DAAbstractNodeLinkGraphicsItem::updateToLinkPointInfo(const DANodeLinkPoint& pl)
{
    d_ptr->_toPoint = pl;
    updatePos();
    updateBoundingRect();
}

/**
 * @brief 生成painterpath,默认会根据连接点生成一个贝塞尔曲线
 * 此函数在updateBoundingRect里调用
 */
void DAAbstractNodeLinkGraphicsItem::generatePainterPath()
{
    switch (getLinkLineStyle()) {
    case LinkLineBezier:
        d_ptr->_linePath = generateLinkLineBezierPainterPath(d_ptr->_fromPos,
                                                             d_ptr->_fromPoint.direction,
                                                             d_ptr->_toPos,
                                                             d_ptr->_toPoint.direction);
        break;
    case LinkLineStraight:
        d_ptr->_linePath = generateLinkLineStraightPainterPath(d_ptr->_fromPos,
                                                               d_ptr->_fromPoint.direction,
                                                               d_ptr->_toPos,
                                                               d_ptr->_toPoint.direction);
        break;
    case LinkLineKnuckle:
        d_ptr->_linePath = generateLinkLineKnucklePainterPath(d_ptr->_fromPos,
                                                              d_ptr->_fromPoint.direction,
                                                              d_ptr->_toPos,
                                                              d_ptr->_toPoint.direction);
        break;
    default:
        break;
    }
    int w = d_ptr->_linePen.width() + 2;
    QPainterPathStroker stroker;
    stroker.setWidth((w < 6) ? 6 : w);
    d_ptr->_lineShapePath = stroker.createStroke(d_ptr->_linePath);
}

/**
 * @brief 生成贝塞尔曲线
 * @param fromPos
 * @param fromDirect
 * @param toPos
 * @param toDirect
 * @return
 */
QPainterPath DAAbstractNodeLinkGraphicsItem::generateLinkLineBezierPainterPath(const QPointF& fromPos,
                                                                               DANodeLinkPoint::Direction fromDirect,
                                                                               const QPointF& toPos,
                                                                               DANodeLinkPoint::Direction toDirect)
{

    //贝塞尔的引导线根据伸出点的方向偏移两个点距离的1/5
    //! 1 先求出两个点距离
    qreal length = pointLength(fromPos, toPos);

    length *= getBezierControlScale();
    //! 2.通过伸出方向，得到两个控制点的位置
    QPointF fromcCrtlPoint = elongation(fromPos, fromDirect, length);
    QPointF toCrtlPoint    = elongation(toPos, toDirect, length);

    //! 3.生成贝塞尔曲线
    QPainterPath path;
    path.moveTo(fromPos);
    path.cubicTo(fromcCrtlPoint, toCrtlPoint, toPos);
    d_ptr->_linkCenterPoint = path.pointAtPercent(0.5);
    return path;
}

QPainterPath DAAbstractNodeLinkGraphicsItem::generateLinkLineStraightPainterPath(const QPointF& fromPos,
                                                                                 DANodeLinkPoint::Direction fromDirect,
                                                                                 const QPointF& toPos,
                                                                                 DANodeLinkPoint::Direction toDirect)
{
    Q_UNUSED(fromDirect);
    Q_UNUSED(toDirect);
    QPainterPath path;
    path.moveTo(fromPos);
    path.lineTo(toPos);
    d_ptr->_linkCenterPoint = path.pointAtPercent(0.5);
    return path;
}

/**
 * @brief 生成直角连线
 * @param fromPos
 * @param fromDirect
 * @param toPos
 * @param toDirect
 * @return
 */
QPainterPath DAAbstractNodeLinkGraphicsItem::generateLinkLineKnucklePainterPath(const QPointF& fromPos,
                                                                                DANodeLinkPoint::Direction fromDirect,
                                                                                const QPointF& toPos,
                                                                                DANodeLinkPoint::Direction toDirect)
{
    const int extendLength = 15;  //延长线的长度
    QPointF extendFrom     = elongation(fromPos, fromDirect, extendLength);
    QPointF extendTo       = elongation(toPos, toDirect, extendLength);
    QPainterPath path;
    path.moveTo(fromPos);
    path.lineTo(extendFrom);
    qreal dx = extendTo.x() - extendFrom.x();
    qreal dy = extendTo.y() - extendFrom.y();
    if ((fromDirect == DANodeLinkPoint::East) || (fromDirect == DANodeLinkPoint::West)) {
        // from沿着x方向
        if (DANodeLinkPoint::isDirectionParallel(fromDirect, toDirect)) {
            //平行
            if (DANodeLinkPoint::isDirectionOpposite(fromDirect, toDirect)) {
                //反向
                if (DANodeLinkPoint::isParallelPointApproachInDirection(extendFrom, fromDirect, extendTo, toDirect)) {
                    // from沿着x方向、平行，反向接近
                    path.lineTo(extendFrom.x() + dx / 2, extendFrom.y());
                    path.lineTo(extendFrom.x() + dx / 2, extendTo.y());
                } else {
                    // from沿着x方向、平行，反向远离
                    path.lineTo(extendFrom.x(), extendFrom.y() + dy / 2);
                    path.lineTo(extendTo.x(), extendFrom.y() + dy / 2);
                }
            } else {
                //同向
                if (DANodeLinkPoint::isPointInFront(extendFrom, fromDirect, extendTo)) {
                    // from沿着x方向，平行，同向靠前
                    path.lineTo(extendTo.x(), extendFrom.y());
                } else {
                    // from沿着x方向，平行，同向靠后
                    path.lineTo(extendFrom.x(), extendTo.y());
                }
            }
        } else {
            //垂直
            if (DANodeLinkPoint::isPointCanMeet(extendFrom, fromDirect, extendTo, toDirect)) {
                // from沿着x方向，垂直,能相遇使用
                path.lineTo(extendTo.x(), extendFrom.y());
            } else if (DANodeLinkPoint::isPointCanMeet(extendFrom, fromDirect, extendTo, DANodeLinkPoint::oppositeDirection(toDirect))) {
                // from沿着x方向，垂直,to调转后能相遇使用
                path.lineTo(extendFrom.x() + dx / 2, extendFrom.y());
                path.lineTo(extendFrom.x() + dx / 2, extendTo.y());
            } else if (DANodeLinkPoint::isPointCanMeet(extendFrom, DANodeLinkPoint::oppositeDirection(fromDirect), extendTo, toDirect)) {
                // from沿着x方向，垂直,from调转后能相遇使用
                path.lineTo(extendFrom.x(), extendFrom.y() + dy / 2);
                path.lineTo(extendTo.x(), extendFrom.y() + dy / 2);
            } else {
                // from沿着x方向，垂直,两边同时调转后能相遇使用
                path.lineTo(extendFrom.x(), extendTo.y());
            }
        }
    } else {
        // from沿着y方向
        if (DANodeLinkPoint::isDirectionParallel(fromDirect, toDirect)) {
            //平行
            if (DANodeLinkPoint::isDirectionOpposite(fromDirect, toDirect)) {
                //反向
                if (DANodeLinkPoint::isParallelPointApproachInDirection(extendFrom, fromDirect, extendTo, toDirect)) {
                    // from沿着y方向、平行，反向接近
                    path.lineTo(extendFrom.x(), extendFrom.y() + dy / 2);
                    path.lineTo(extendTo.x(), extendFrom.y() + dy / 2);
                } else {
                    // from沿着y方向、平行，反向远离

                    path.lineTo(extendFrom.x() + dx / 2, extendFrom.y());
                    path.lineTo(extendFrom.x() + dx / 2, extendTo.y());
                }
            } else {
                //同向
                if (DANodeLinkPoint::isPointInFront(extendFrom, fromDirect, extendTo)) {
                    // from沿着y方向，平行，同向靠前
                    path.lineTo(extendFrom.x(), extendTo.y());
                } else {
                    // from沿着y方向，平行，同向靠后
                    path.lineTo(extendTo.x(), extendFrom.y());
                }
            }
        } else {
            //垂直
            if (DANodeLinkPoint::isPointCanMeet(extendFrom, fromDirect, extendTo, toDirect)) {
                // from沿着y方向，垂直,能相遇使用
                path.lineTo(extendFrom.x(), extendTo.y());
            } else if (DANodeLinkPoint::isPointCanMeet(extendFrom, fromDirect, extendTo, DANodeLinkPoint::oppositeDirection(toDirect))) {
                // from沿着y方向，垂直,to调转后能相遇使用
                path.lineTo(extendFrom.x(), extendFrom.y() + dy / 2);
                path.lineTo(extendTo.x(), extendFrom.y() + dy / 2);
            } else if (DANodeLinkPoint::isPointCanMeet(extendFrom, DANodeLinkPoint::oppositeDirection(fromDirect), extendTo, toDirect)) {
                // from沿着y方向，垂直,from调转后能相遇使用
                path.lineTo(extendFrom.x() + dx / 2, extendFrom.y());
                path.lineTo(extendFrom.x() + dx / 2, extendTo.y());
            } else {
                // from沿着y方向，垂直,两边同时调转后能相遇使用
                path.lineTo(extendTo.x(), extendFrom.y());
            }
        }
    }
    path.lineTo(extendTo);
    path.lineTo(toPos);
    d_ptr->_linkCenterPoint = path.pointAtPercent(0.5);
    return path;
}

/**
 * @brief 设置文本
 * @param t
 */
void DAAbstractNodeLinkGraphicsItem::setText(const QString& t)
{
    //设置null字符就销毁item
    if (t.isNull()) {
        if (d_ptr->_textItem) {
            delete d_ptr->_textItem;
            d_ptr->_textItem = nullptr;
            update();
        }
        return;
    }
    if (d_ptr->_textItem == nullptr) {
        d_ptr->_textItem = new QGraphicsSimpleTextItem(this);
        d_ptr->_textItem->setFlag(ItemIsSelectable, true);
        d_ptr->_textItem->setFlag(ItemIsMovable, false);
        //默认位置在中间
        d_ptr->updateTextPos();
    }
    d_ptr->_textItem->setText(t);
}

/**
 * @brief 获取文本
 * @return
 */
QString DAAbstractNodeLinkGraphicsItem::getText() const
{
    if (d_ptr->_textItem == nullptr) {
        return QString();
    }
    return d_ptr->_textItem->text();
}

/**
 * @brief 获取文本对应的item
 * @return
 */
QGraphicsSimpleTextItem* DAAbstractNodeLinkGraphicsItem::getTextItem()
{
    return d_ptr->_textItem;
}

/**
 * @brief 获取from node item，如果没有返回nullptr
 * @return
 */
DAAbstractNodeGraphicsItem* DAAbstractNodeLinkGraphicsItem::fromNodeItem() const
{
    return d_ptr->_fromItem;
}

/**
 * @brief 获取to node item，如果没有返回nullptr
 * @return
 */
DAAbstractNodeGraphicsItem* DAAbstractNodeLinkGraphicsItem::toNodeItem() const
{
    return d_ptr->_toItem;
}
/**
 * @brief from的连接点
 * @return
 */
DANodeLinkPoint DAAbstractNodeLinkGraphicsItem::fromNodeLinkPoint() const
{
    return d_ptr->_fromPoint;
}
/**
 * @brief to的连接点
 * @return
 */
DANodeLinkPoint DAAbstractNodeLinkGraphicsItem::toNodeLinkPoint() const
{
    return d_ptr->_toPoint;
}

/**
 * @brief 获取from的节点，如果没有返回nullptr
 * @return
 */
DAAbstractNode::SharedPointer DAAbstractNodeLinkGraphicsItem::fromNode() const
{
    if (nullptr == d_ptr->_fromItem) {
        return nullptr;
    }
    return d_ptr->_fromItem->node();
}

/**
 * @brief 获取to的节点，如果没有返回nullptr
 * @return
 */
DAAbstractNode::SharedPointer DAAbstractNodeLinkGraphicsItem::toNode() const
{
    if (nullptr == d_ptr->_toItem) {
        return nullptr;
    }
    return d_ptr->_toItem->node();
}

/**
 * @brief 设置连接点端点的样式
 * @param o 连接点方向
 * @param epType 样式
 */
void DAAbstractNodeLinkGraphicsItem::setEndPointType(DAAbstractNodeLinkGraphicsItem::Orientations o,
                                                     DAAbstractNodeLinkGraphicsItem::EndPointType epType)
{
    switch (o) {
    case OrientationFrom: {
        if (d_ptr->_fromEndPointType == epType) {
            return;
        }
        d_ptr->_fromEndPointType        = epType;
        d_ptr->_fromEndPointPainterPath = generateEndPointPainterPath(epType, d_ptr->_endPointSize);
    } break;
    case OrientationTo: {
        if (d_ptr->_toEndPointType == epType) {
            return;
        }
        d_ptr->_toEndPointType        = epType;
        d_ptr->_toEndPointPainterPath = generateEndPointPainterPath(epType, d_ptr->_endPointSize);
    } break;
    case OrientationBoth: {
        setEndPointType(OrientationFrom, epType);
        setEndPointType(OrientationTo, epType);
    }
    default:
        break;
    }
}

/**
 * @brief 获取端点的样式
 * @param o 端点的方向，如果为both，若两个端点样式一样，返回端点样式，若不一样，返回EndPointNone
 * @return
 */
DAAbstractNodeLinkGraphicsItem::EndPointType DAAbstractNodeLinkGraphicsItem::getEndPointType(DAAbstractNodeLinkGraphicsItem::Orientations o) const
{
    switch (o) {
    case OrientationFrom:
        return d_ptr->_fromEndPointType;
    case OrientationTo:
        return d_ptr->_toEndPointType;
    default:
        break;
    }
    return (d_ptr->_fromEndPointType == d_ptr->_toEndPointType) ? d_ptr->_fromEndPointType : EndPointNone;
}

/**
 * @brief 设置连接点尺寸
 * @param size
 */
void DAAbstractNodeLinkGraphicsItem::setEndPointSize(int size)
{
    if (d_ptr->_endPointSize == size) {
        return;
    }
    d_ptr->_endPointSize            = size;
    d_ptr->_fromEndPointPainterPath = generateEndPointPainterPath(d_ptr->_fromEndPointType, size);
    d_ptr->_toEndPointPainterPath   = generateEndPointPainterPath(d_ptr->_toEndPointType, size);
}

/**
 * @brief 获取端点的大小
 * @return
 */
int DAAbstractNodeLinkGraphicsItem::getEndPointSize() const
{
    return d_ptr->_endPointSize;
}

QVariant DAAbstractNodeLinkGraphicsItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case QGraphicsItem::ItemSelectedHasChanged:
        setLinkPointNameVisible(value.toBool());
        break;

    case QGraphicsItem::ItemSelectedChange:
        //在连接状态中不允许选中
        if (d_ptr->isStartLinking()) {
            return (false);
        }
        break;

    default:
        break;
    }
    return (QGraphicsItem::itemChange(change, value));
}

void DAAbstractNodeLinkGraphicsItem::callItemIsDestroying(DAAbstractNodeGraphicsItem* item, const DA::DANodeLinkPoint& pl)
{
    if ((d_ptr->_fromItem == item) && (d_ptr->_fromPoint == pl)) {
        //说明from要取消
        d_ptr->_fromItem  = nullptr;
        d_ptr->_fromPoint = DANodeLinkPoint();
    } else if ((d_ptr->_toItem == item) && (d_ptr->_toPoint == pl)) {
        //说明to要取消
        qDebug() << "d_ptr->_toItem = nullptr";
        d_ptr->_toItem  = nullptr;
        d_ptr->_toPoint = DANodeLinkPoint();
    }
    //如果from和to都为空，这时就需要自动销毁
    DANodeGraphicsScene* sc = d_ptr->nodeScene();

    if (sc) {
        sc->callNodeItemLinkIsEmpty(this);
    }
}

namespace DA
{

/**
 * @brief DAAbstractNodeLinkGraphicsItem::EndPointType的枚举转换
 * @param e
 * @return
 */
QString enumToString(DAAbstractNodeLinkGraphicsItem::EndPointType e)
{

    switch (e) {
    case DAAbstractNodeLinkGraphicsItem::EndPointNone:
        return "none";
    case DAAbstractNodeLinkGraphicsItem::EndPointTriangType:
        return "triang";
    default:
        break;
    }
    return "none";
}

/**
 * @brief DAAbstractNodeLinkGraphicsItem::EndPointType的枚举转换
 * @param s
 * @return
 */
DAAbstractNodeLinkGraphicsItem::EndPointType stringToEnum(const QString& s, DAAbstractNodeLinkGraphicsItem::EndPointType defaultEnum)
{
    if (0 == s.compare("none", Qt::CaseInsensitive)) {
        return DAAbstractNodeLinkGraphicsItem::EndPointNone;
    } else if (0 == s.compare("triang", Qt::CaseInsensitive)) {
        return DAAbstractNodeLinkGraphicsItem::EndPointTriangType;
    }
    return defaultEnum;
}
/**
 * @brief DAAbstractNodeLinkGraphicsItem::LinkLineStyle的枚举转换
 * @param e
 * @return
 */
QString enumToString(DAAbstractNodeLinkGraphicsItem::LinkLineStyle e)
{
    switch (e) {
    case DAAbstractNodeLinkGraphicsItem::LinkLineBezier:
        return "bezier";
    case DAAbstractNodeLinkGraphicsItem::LinkLineStraight:
        return "straight";
    case DAAbstractNodeLinkGraphicsItem::LinkLineKnuckle:
        return "knuckle";
    default:
        break;
    }
    return "knuckle";
}
/**
 * @brief DAAbstractNodeLinkGraphicsItem::LinkLineStyle的枚举转换
 * @param s
 * @return
 */
DAAbstractNodeLinkGraphicsItem::LinkLineStyle stringToEnum(const QString& s, DAAbstractNodeLinkGraphicsItem::LinkLineStyle defaultEnum)
{
    if (0 == s.compare("knuckle", Qt::CaseInsensitive)) {
        return DAAbstractNodeLinkGraphicsItem::LinkLineKnuckle;
    } else if (0 == s.compare("bezier", Qt::CaseInsensitive)) {
        return DAAbstractNodeLinkGraphicsItem::LinkLineBezier;
    } else if (0 == s.compare("straight", Qt::CaseInsensitive)) {
        return DAAbstractNodeLinkGraphicsItem::LinkLineStraight;
    }
    return defaultEnum;
}

}  // namespace DA
