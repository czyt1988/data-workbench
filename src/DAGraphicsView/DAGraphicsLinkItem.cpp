#include "DAGraphicsLinkItem.h"
#include <QPainter>
#include <QDebug>
#include <QGraphicsScene>
#include <QEvent>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QLineF>
#include <math.h>

namespace DA
{
//===============================================================
// DAGraphicsLinkItem::PrivateData
//===============================================================
class DAGraphicsLinkItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAGraphicsLinkItem)
public:
    PrivateData(DAGraphicsLinkItem* p);

public:
    DAGraphicsLinkItem::LinkLineStyle mLinkLineStyle { DAGraphicsLinkItem::LinkLineKnuckle };
    QPointF mStartPos { 0, 0 };
    QPointF mEndPos { 100, 100 };
    QRectF mBoundingRect { 0, 0, 100, 100 };  ///< 记录boundingRect
    qreal mBezierControlScale { 0.35 };       ///<贝塞尔曲线的控制点的缩放比例
    QPainterPath mLinePath;                   ///< 通过点得到的绘图线段
    QPainterPath mLineShapePath;              ///_linePath的轮廓，用于shape函数
    QPen mLinePen { QColor(101, 103, 106) };  ///< 线的画笔(默认灰色)
    //端点样式
    DAGraphicsLinkItem::EndPointType mStartEndPointType { DAGraphicsLinkItem::EndPointNone };  ///< from的端点样式
    DAGraphicsLinkItem::EndPointType mEndEndPointType { DAGraphicsLinkItem::EndPointNone };    ///< to的端点样式
    QPainterPath mSrartEndPointPainterPath;    ///< 记录from的端点样式
    QPainterPath mEndEndPointPainterPath;      ///< 记录to的端点样式
    int mEndPointSize { 12 };                  ///< 记录端点大小
    bool mIsEndPositionFollowMouse { false };  ///< 结束节点跟随鼠标移动而移动
};

DAGraphicsLinkItem::PrivateData::PrivateData(DAGraphicsLinkItem* p) : q_ptr(p)
{
}

//===============================================================
// DAGraphicsLinkItem
//===============================================================
DAGraphicsLinkItem::DAGraphicsLinkItem(QGraphicsItem* p) : QGraphicsItem(p), DA_PIMPL_CONSTRUCT
{
    setFlags(flags() | ItemIsSelectable);
    setEndPointType(OrientationFrom, EndPointNone);
    setEndPointType(OrientationTo, EndPointTriangType);
    setLinkLineStyle(LinkLineBezier);
    setZValue(-1);  //连接线在-1层，这样避免在节点上面
}

DAGraphicsLinkItem::~DAGraphicsLinkItem()
{
}

/**
 * @brief 设置连接点端点的样式
 * @param o 连接点方向
 * @param epType 样式
 */
void DAGraphicsLinkItem::setEndPointType(DAGraphicsLinkItem::Orientations o, DAGraphicsLinkItem::EndPointType epType)
{
    switch (o) {
    case OrientationFrom: {
        if (d_ptr->mStartEndPointType == epType) {
            return;
        }
        d_ptr->mStartEndPointType        = epType;
        d_ptr->mSrartEndPointPainterPath = generateEndPointPainterPath(epType, d_ptr->mEndPointSize);
    } break;
    case OrientationTo: {
        if (d_ptr->mEndEndPointType == epType) {
            return;
        }
        d_ptr->mEndEndPointType        = epType;
        d_ptr->mEndEndPointPainterPath = generateEndPointPainterPath(epType, d_ptr->mEndPointSize);
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
DAGraphicsLinkItem::EndPointType DAGraphicsLinkItem::getEndPointType(DAGraphicsLinkItem::Orientations o) const
{
    switch (o) {
    case OrientationFrom:
        return d_ptr->mStartEndPointType;
    case OrientationTo:
        return d_ptr->mEndEndPointType;
    default:
        break;
    }
    return (d_ptr->mStartEndPointType == d_ptr->mEndEndPointType) ? d_ptr->mStartEndPointType : EndPointNone;
}

/**
 * @brief 设置连线样式
 * @param s
 */
void DAGraphicsLinkItem::setLinkLineStyle(DAGraphicsLinkItem::LinkLineStyle s)
{
    d_ptr->mLinkLineStyle = s;
    updateBoundingRect();
}

/**
 * @brief 获取连线样式
 * @return
 */
DAGraphicsLinkItem::LinkLineStyle DAGraphicsLinkItem::getLinkLineStyle() const
{
    return d_ptr->mLinkLineStyle;
}

/**
 * @brief 设置线的画笔
 * @param p
 */
void DAGraphicsLinkItem::setLinePen(const QPen& p)
{
    d_ptr->mLinePen = p;
}

/**
 * @brief 返回当前画笔
 * @return
 */
QPen DAGraphicsLinkItem::getLinePen() const
{
    return (d_ptr->mLinePen);
}

/**
 * @brief 更新范围
 *
 * @note 争对只有一个起始连接点的情况下，此函数的终止链接点将更新为场景鼠标所在
 */
QRectF DAGraphicsLinkItem::updateBoundingRect()
{
    //! 通过调用prepareGeometryChange()通知范围变更，避免出现残影
    prepareGeometryChange();
#if 0
    QPointF fromPoint = d_ptr->mStartPos;
    QPointF toPoint   = d_ptr->mEndPos;
    // 关键！！！
    // 为了不覆盖点击，d_ptr->_toPos要做2像素偏移
    if (toPoint.x() > fromPoint.x()) {
        toPoint.rx() -= 4;
    } else {
        toPoint.rx() += 4;
    }
    if (toPoint.y() > fromPoint.y()) {
        toPoint.ry() -= 4;
    } else {
        toPoint.ry() += 4;
    }
    d_ptr->mLinePath = generateLinePainterPath(fromPoint, toPoint, getLinkLineStyle());
#else
    d_ptr->mLinePath = generateLinePainterPath(d_ptr->mStartPos, d_ptr->mEndPos, getLinkLineStyle());
#endif
    QPainterPathStroker stroker;
    int w = d_ptr->mLinePen.width() + 2;
    stroker.setWidth((w < 6) ? 6 : w);
    d_ptr->mLineShapePath = stroker.createStroke(d_ptr->mLinePath);
    d_ptr->mBoundingRect = d_ptr->mLinePath.boundingRect().adjusted(-2, -2, 2, 2);  //留足选中后画笔变宽的绘制余量
    return d_ptr->mBoundingRect;
}

/**
 * @brief 生成一个连接线
 * @param fromPoint
 * @param toPoint
 * @param strokerWidth 轮廓扩展，此轮廓扩展会生成一个
 * @return
 */
QPainterPath DAGraphicsLinkItem::generateLinePainterPath(const QPointF& fromPoint, const QPointF& toPoint, LinkLineStyle linestyle)
{
    QPainterPath res;
    switch (linestyle) {
    case LinkLineBezier:
        res = generateLinkLineBezierPainterPath(fromPoint,
                                                relativeDirectionOfPoint(toPoint, fromPoint),
                                                toPoint,
                                                relativeDirectionOfPoint(fromPoint, toPoint));
        break;
    case LinkLineStraight:
        res = generateLinkLineStraightPainterPath(fromPoint, toPoint);
        break;
    case LinkLineKnuckle:
        res = generateLinkLineKnucklePainterPath(fromPoint,
                                                 relativeDirectionOfPoint(toPoint, fromPoint),
                                                 toPoint,
                                                 relativeDirectionOfPoint(fromPoint, toPoint));
        break;
    default:
        break;
    }
    return res;
}

/**
 * @brief 设置贝塞尔曲线的控制点的缩放比例
 *
 * 在连线时按照两个连接点的方向延伸出贝塞尔曲线的控制点，贝塞尔曲线的控制点的长度w = length * bezierControlScale，
 * 其中length是两个连接点的绝对距离，可以通过@sa pointLength 计算得到，bezierControlScale，就是这个延伸的比例，如果越大，曲率越明显
 * @param rate 默认为0.25
 */
void DAGraphicsLinkItem::setBezierControlScale(qreal rate)
{
    d_ptr->mBezierControlScale = rate;
}

/**
 * @brief 获取贝塞尔曲线的控制点的缩放比例
 * @return
 */
qreal DAGraphicsLinkItem::getBezierControlScale() const
{
    return (d_ptr->mBezierControlScale);
}

/**
 * @brief 开始连接点的位置
 * @param p 相对item自身的位置
 */
void DAGraphicsLinkItem::setStartPosition(const QPointF& p)
{
    d_ptr->mStartPos = p;
}

/**
 * @brief 开始连接点的位置
 * @return 相对item自身的位置
 */
const QPointF& DAGraphicsLinkItem::getStartPosition() const
{
    return d_ptr->mStartPos;
}

/**
 * @brief 结束连接点的位置
 * @param p 相对item自身的位置
 */
void DAGraphicsLinkItem::setEndPosition(const QPointF& p)
{
    d_ptr->mEndPos = p;
}

/**
 * @brief 结束连接点的位置
 * @return 相对item自身的位置
 */
const QPointF& DAGraphicsLinkItem::getEndPosition() const
{
    return d_ptr->mEndPos;
}

/**
 * @brief 开始连接点的位置,位置基于scene
 *
 * 默认情况下，开始点就是相对于item的原点（0，0）
 * @param p 相对scene的位置
 */
void DAGraphicsLinkItem::setStartScenePosition(const QPointF& p)
{
    setPos(p);
    d_ptr->mStartPos = mapFromScene(p);
}

/**
 * @brief 开始连接点的位置,位置基于scene
 * @return
 */
QPointF DAGraphicsLinkItem::getStartScenePosition() const
{
    return mapToScene(d_ptr->mStartPos);
}

/**
 * @brief 结束连接点的位置,位置基于scene
 * @param p 相对scene的位置
 */
void DAGraphicsLinkItem::setEndScenePosition(const QPointF& p)
{
    d_ptr->mEndPos = mapFromScene(p);
}

/**
 * @brief 结束连接点的位置,位置基于scene
 * @return 相对scene的位置
 */
QPointF DAGraphicsLinkItem::getEndScenePosition() const
{
    return mapToScene(d_ptr->mEndPos);
}

/**
 * @brief 结束节点跟随鼠标，此函数前提是from节点已经确定
 *
 * 设置此函数后，DAGraphicsLinkItem的结束点会跟随鼠标移动而移动，这个函数在进行一些连接线交互上比较有用
 */
void DAGraphicsLinkItem::setEndPositionFollowMouse(bool on)
{
    d_ptr->mIsEndPositionFollowMouse = on;
}

/**
 * @brief 绘图
 * @param painter
 * @param option
 * @param widget
 */
void DAGraphicsLinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->save();
    paintLinkLine(painter, option, widget, d_ptr->mLinePath);
    //绘制端点
    paintEndPoint(painter,
                  option,
                  d_ptr->mStartPos,
                  d_ptr->mStartEndPointType,
                  d_ptr->mSrartEndPointPainterPath,
                  d_ptr->mEndPos,
                  d_ptr->mEndEndPointType,
                  d_ptr->mEndEndPointPainterPath,
                  d_ptr->mLinePath);
    painter->restore();
}

/**
 * @brief 绘制连接线
 * @param painter
 * @param option
 * @param widget
 * @param linkPath
 */
void DAGraphicsLinkItem::paintLinkLine(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QPainterPath& linkPath)
{
    Q_UNUSED(widget);
    QPen pen = getPainterPen(option);
    painter->setPen(pen);
    painter->drawPath(linkPath);
}

/**
 * @brief 绘制箭头
 * @param painter
 * @param option
 * @param p 箭头的端点
 * @param linkPath 绘制箭头的路径线，这个路径线用来计算箭头的旋转
 */
void DAGraphicsLinkItem::paintEndPoint(QPainter* painter,
                                       const QStyleOptionGraphicsItem* option,
                                       const QPointF& pStart,
                                       EndPointType etStart,
                                       const QPainterPath& startPainterPath,
                                       const QPointF& pEnd,
                                       EndPointType etEnd,
                                       const QPainterPath& endPainterPath,
                                       const QPainterPath& linkPath)
{
    if (etStart == EndPointNone && etEnd == EndPointNone) {
        return;
    }
    //根据DANodeLinkPoint计算旋转的角度
    painter->save();
    QPen pen = getPainterPen(option);
    painter->setPen(pen);
    painter->setBrush(pen.color());
    //首先绘制开端箭头
    if (etStart != EndPointNone) {
        painter->save();
        QPointF pTrend = linkPath.pointAtPercent(0.1);
        QLineF lf(pStart, pTrend);
        //先旋转再移动
        painter->rotate(360 - lf.angle());  // painter的rotate是顺时针旋转，而line的angle是逆时针
        painter->translate(pStart);
        painter->drawPath(startPainterPath);
        painter->restore();
        //        qDebug() << "etStart,pTrend=" << pTrend << ",pStart=" << pStart
        //                 << "QLineF(pTrend, pStart).angle=" << QLineF(pTrend, pStart).angle()
        //                 << ",QLineF(pStart,pTrend).angle=" << QLineF(pStart, pTrend).angle();
    }
    //再绘制结束箭头
    if (etEnd != EndPointNone) {
        painter->save();
        QPointF pTrend = linkPath.pointAtPercent(0.9);
        QLineF lf(pEnd, pTrend);
        //先旋转再移动
        painter->drawEllipse(pTrend, 2, 2);
        painter->drawEllipse(pEnd, 4, 4);
        painter->translate(pEnd);
        painter->rotate(360 - lf.angle());  // painter的rotate是顺时针旋转，而line的angle是逆时针
        painter->drawPath(endPainterPath);
        painter->restore();
        //        qDebug() << "etEnd,pTrend=" << pTrend << ",pEnd=" << pEnd
        //                 << "QLineF(pTrend, pEnd).angle=" << QLineF(pTrend, pEnd).angle()
        //                 << ",QLineF(pEnd,pTrend).angle=" << QLineF(pEnd, pTrend).angle();
    }
    painter->restore();
}

QRectF DAGraphicsLinkItem::boundingRect() const
{
    return (d_ptr->mBoundingRect);
}

QPainterPath DAGraphicsLinkItem::shape() const
{
    return (d_ptr->mLineShapePath);
}

/**
 * @brief 计算两个点的距离
 * @param a
 * @param b
 * @return
 */
qreal DAGraphicsLinkItem::pointLength(const QPointF& a, const QPointF& b)
{
    return (pow((a.x() - b.x()) * (a.x() - b.x()) + (a.y() - b.y()) * (a.y() - b.y()), 0.5));
}

/**
 * @brief 延长线，以一个方向和距离延伸
 * @param orgPoint 原始点
 * @param d 伸出方向
 * @param externLen 伸出长度
 * @return 得到控制点
 */
QPointF DAGraphicsLinkItem::elongation(const QPointF& orgPoint, AspectDirection d, qreal externLen)
{
    switch (d) {
    case AspectDirection::East:
        return (QPointF(orgPoint.x() + externLen, orgPoint.y()));

    case AspectDirection::South:
        return (QPointF(orgPoint.x(), orgPoint.y() + externLen));

    case AspectDirection::West:
        return (QPointF(orgPoint.x() - externLen, orgPoint.y()));

    case AspectDirection::North:
        return (QPointF(orgPoint.x(), orgPoint.y() - externLen));
    }
    return (orgPoint);
}

/**
 * @brief 判断两个方向是否相对，也就是东对西，南对北就是相对，相对必定平行
 * @param d1
 * @param d2
 * @return
 */
bool DAGraphicsLinkItem::isDirectionOpposite(AspectDirection d1, AspectDirection d2)
{
    switch (d1) {
    case AspectDirection::East:
        return d2 == AspectDirection::West;
    case AspectDirection::South:
        return d2 == AspectDirection::North;
    case AspectDirection::West:
        return d2 == AspectDirection::East;
    case AspectDirection::North:
        return d2 == AspectDirection::South;
    }
    return false;
}

/**
 * @brief 判断两个方向是否平行
 * @param d1
 * @param d2
 * @return
 */
bool DAGraphicsLinkItem::isDirectionParallel(AspectDirection d1, AspectDirection d2)
{
    if (d1 == d2) {
        return true;
    }
    return isDirectionOpposite(d1, d2);
}

/**
 * @brief 在1d的方向上，点2在点1的方向的前面

 * @param p1
 * @param d1
 * @param p2
 * @return
 */
bool DAGraphicsLinkItem::isPointInFront(const QPointF& p1, AspectDirection d1, const QPointF& p2)
{
    switch (d1) {
    case AspectDirection::East:
        return p2.x() > p1.x();
    case AspectDirection::South:
        return p2.y() > p1.y();
    case AspectDirection::West:
        return p2.x() < p1.x();
    case AspectDirection::North:
        return p2.y() < p1.y();
    }
    return false;
}

/**
 * @brief 判断点能否相遇,针对垂直方向才有意义
 * @param p1
 * @param d1
 * @param p2
 * @param d2
 * @return
 */
bool DAGraphicsLinkItem::isPointCanMeet(const QPointF& p1, AspectDirection d1, const QPointF& p2, AspectDirection d2)
{
    if (isDirectionParallel(d1, d2)) {
        return false;
    }
    switch (d1) {
    case AspectDirection::East: {
        if (d2 == AspectDirection::South) {
            //南
            return (p1.x() < p2.x()) && (p1.y() > p2.y());
        } else {
            //北
            return (p1.x() < p2.x()) && (p1.y() < p2.y());
        }
    } break;
    case AspectDirection::South: {
        if (d2 == AspectDirection::East) {
            //东
            return (p1.x() > p2.x()) && (p1.y() < p2.y());
        } else {
            //西
            return (p1.x() < p2.x()) && (p1.y() < p2.y());
        }
    } break;
    case AspectDirection::West: {
        if (d2 == AspectDirection::South) {
            //南
            return (p1.x() > p2.x()) && (p1.y() > p2.y());
        } else {
            //北
            return (p1.x() > p2.x()) && (p1.y() < p2.y());
        }
    } break;
    case AspectDirection::North: {
        if (d2 == AspectDirection::East) {
            //东
            return (p1.x() > p2.x()) && (p1.y() > p2.y());
        } else {
            //西
            return (p1.x() < p2.x()) && (p1.y() > p2.y());
        }
    } break;
    }
    return false;
}

/**
 * @brief 针对平行点线，沿着方向移动可以接近，此函数只对平行点线有用
 * @param p1
 * @param d1
 * @param p2
 * @param d2
 * @return
 */
bool DAGraphicsLinkItem::isParallelPointApproachInDirection(const QPointF& p1, AspectDirection d1, const QPointF& p2, AspectDirection d2)
{
    if (!isDirectionOpposite(d1, d2)) {
        //平行同向永远接近不了
        return false;
    }
    //这里说明必定反向
    switch (d1) {
    case AspectDirection::East:
        return p1.x() < p2.x();
    case AspectDirection::South:
        return p1.y() < p2.y();
    case AspectDirection::West:
        return p1.x() > p2.x();
    case AspectDirection::North:
        return p1.y() > p2.y();
    }
    return false;
}

/**
 * @brief 翻转方向
 * @param d
 * @return
 */
AspectDirection DAGraphicsLinkItem::oppositeDirection(AspectDirection d)
{
    switch (d) {
    case AspectDirection::East:
        return AspectDirection::West;
    case AspectDirection::South:
        return AspectDirection::North;
    case AspectDirection::West:
        return AspectDirection::East;
    case AspectDirection::North:
        return AspectDirection::South;
    default:
        break;
    }
    return AspectDirection::East;
}

/**
 * @brief 计算点1相对点2的方向
 *       |
 *       |
 *   p1———p2————
 *       |
 *       |
 * 如上面示例，此函数返回West
 *
 * @param p1
 * @param p2
 * @return 如果点重合，降返回一个不可预测的方向
 */
AspectDirection DAGraphicsLinkItem::relativeDirectionOfPoint(const QPointF& p1, const QPointF& p2)
{
    qreal dx = p1.x() - p2.x();
    qreal dy = p1.y() - p2.y();
    if (qAbs(dx) > qAbs(dy)) {
        // x方向大于y方向
        if (dx > 0) {
            return AspectDirection::East;
        }
        return AspectDirection::West;
    } else {
        // x方向小于y方向
        if (dy > 0) {
            return AspectDirection::South;
        }
        return AspectDirection::North;
    }
    //不可能达到
    return AspectDirection::East;
}

/**
 * @brief 获取绘图的画笔
 *
 * 在被选中的时候，画笔会加深加粗
 * @param option
 * @return
 */
QPen DAGraphicsLinkItem::getPainterPen(const QStyleOptionGraphicsItem* option) const
{
    QPen pen = d_ptr->mLinePen;
    if (option->state.testFlag(QStyle::State_Selected)) {
        //说明选中了
        pen.setWidth(pen.width() + 2);
        pen.setColor(pen.color().darker(150));
    }
    return pen;
}

/**
 * @brief 生成箭头，所有生成的箭头箭头尖端需要再原点，箭头朝向x轴正方向
 *
 * 形如：
 *
 *     |
 *     |   *
 *     | * *
 * ----*   *-->
 *     | * *
 *     |   *
 *     |
 *
 * @param arrowType
 * @param size 箭头↑的高度
 * @return
 */
QPainterPath DAGraphicsLinkItem::generateEndPointPainterPath(DAGraphicsLinkItem::EndPointType epType, int size)
{
    QPainterPath path;
    switch (epType) {
    case EndPointTriangType:  //三角形
        path.moveTo(0, 0);
        path.lineTo(size, -size / 2);
        path.lineTo(size, size / 2);
        path.lineTo(0, 0);
        break;
    default:
        break;
    }
    return path;
}

/**
 * @brief 生成贝塞尔曲线
 * @param fromPos
 * @param toPos
 * @return
 */
QPainterPath DAGraphicsLinkItem::generateLinkLineBezierPainterPath(const QPointF& fromPos,
                                                                   AspectDirection fromDirect,
                                                                   const QPointF& toPos,
                                                                   AspectDirection toDirect)
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
    return path;
}

/**
 * @brief 生成直线
 * @param fromPos
 * @param toPos
 * @return
 */
QPainterPath DAGraphicsLinkItem::generateLinkLineStraightPainterPath(const QPointF& fromPos, const QPointF& toPos)
{
    QPainterPath path;
    path.moveTo(fromPos);
    path.lineTo(toPos);
    return path;
}

/**
 * @brief 生成直角连线
 * @param fromPos
 * @param toPos
 * @return
 */
QPainterPath DAGraphicsLinkItem::generateLinkLineKnucklePainterPath(const QPointF& fromPos,
                                                                    AspectDirection fromDirect,
                                                                    const QPointF& toPos,
                                                                    AspectDirection toDirect)
{
    const int extendLength = 15;  //延长线的长度
    QPointF extendFrom     = elongation(fromPos, fromDirect, extendLength);
    QPointF extendTo       = elongation(toPos, toDirect, extendLength);
    QPainterPath path;
    path.moveTo(fromPos);
    path.lineTo(extendFrom);
    qreal dx = extendTo.x() - extendFrom.x();
    qreal dy = extendTo.y() - extendFrom.y();
    if ((fromDirect == AspectDirection::East) || (fromDirect == AspectDirection::West)) {
        // from沿着x方向
        if (isDirectionParallel(fromDirect, toDirect)) {
            //平行
            if (isDirectionOpposite(fromDirect, toDirect)) {
                //反向
                if (isParallelPointApproachInDirection(extendFrom, fromDirect, extendTo, toDirect)) {
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
                if (isPointInFront(extendFrom, fromDirect, extendTo)) {
                    // from沿着x方向，平行，同向靠前
                    path.lineTo(extendTo.x(), extendFrom.y());
                } else {
                    // from沿着x方向，平行，同向靠后
                    path.lineTo(extendFrom.x(), extendTo.y());
                }
            }
        } else {
            //垂直
            if (isPointCanMeet(extendFrom, fromDirect, extendTo, toDirect)) {
                // from沿着x方向，垂直,能相遇使用
                path.lineTo(extendTo.x(), extendFrom.y());
            } else if (isPointCanMeet(extendFrom, fromDirect, extendTo, oppositeDirection(toDirect))) {
                // from沿着x方向，垂直,to调转后能相遇使用
                path.lineTo(extendFrom.x() + dx / 2, extendFrom.y());
                path.lineTo(extendFrom.x() + dx / 2, extendTo.y());
            } else if (isPointCanMeet(extendFrom, oppositeDirection(fromDirect), extendTo, toDirect)) {
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
        if (isDirectionParallel(fromDirect, toDirect)) {
            //平行
            if (isDirectionOpposite(fromDirect, toDirect)) {
                //反向
                if (isParallelPointApproachInDirection(extendFrom, fromDirect, extendTo, toDirect)) {
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
                if (isPointInFront(extendFrom, fromDirect, extendTo)) {
                    // from沿着y方向，平行，同向靠前
                    path.lineTo(extendFrom.x(), extendTo.y());
                } else {
                    // from沿着y方向，平行，同向靠后
                    path.lineTo(extendTo.x(), extendFrom.y());
                }
            }
        } else {
            //垂直
            if (isPointCanMeet(extendFrom, fromDirect, extendTo, toDirect)) {
                // from沿着y方向，垂直,能相遇使用
                path.lineTo(extendFrom.x(), extendTo.y());
            } else if (isPointCanMeet(extendFrom, fromDirect, extendTo, oppositeDirection(toDirect))) {
                // from沿着y方向，垂直,to调转后能相遇使用
                path.lineTo(extendFrom.x(), extendFrom.y() + dy / 2);
                path.lineTo(extendTo.x(), extendFrom.y() + dy / 2);
            } else if (isPointCanMeet(extendFrom, oppositeDirection(fromDirect), extendTo, toDirect)) {
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
    return path;
}

}  // end DA
