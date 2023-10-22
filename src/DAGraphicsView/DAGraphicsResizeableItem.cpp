#include "DAGraphicsResizeableItem.h"
#include <memory>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include <QtMath>
#include <QGraphicsSceneHoverEvent>
#include <QDomDocument>
#include <QDomElement>
#include "DAGraphicsSceneWithUndoStack.h"
#include "DACommandsForGraphics.h"

#define Enable_DAGraphicsResizeableItemPrivateDebugPrint 0
#if Enable_DAGraphicsResizeableItemPrivateDebugPrint
#define DAGraphicsResizeableItemPrivateDoResizePrint(mousePressItemPos, mousescenePos, currentControlPointTypeUnderMouse, newPos, newSize) \
    do {                                                                                                                                   \
        qDebug() << "mousePressItemPos=" << mousePressItemPos << ",mousescenePos=" << mousescenePos                                        \
                 << ",currentControlPointTypeUnderMouse=" << currentControlPointTypeUnderMouse << ",newPos=" << newPos                     \
                 << ",newSize=" << newSize << ",current pos=" << q_ptr->pos();                                                             \
    } while (0)
#else
#define DAGraphicsResizeableItemPrivateDoResizePrint(mousePressItemPos, mousescenePos, currentControlPointTypeUnderMouse, newPos, newSize)
#endif

#if Enable_DAGraphicsResizeableItemPrivateDebugPrint
#define DAGraphicsResizeableItemPrivatePrint(msg, ...) qDebug(msg, __VA_ARGS__)
#else
#define DAGraphicsResizeableItemPrivatePrint(msg, ...)
#endif

namespace DA
{
/**
 * @brief 控制点信息
 */
class DAGraphicsResizeableItemControlPointInfo
{
public:
    DAGraphicsResizeableItemControlPointInfo(const QRectF& r, DAGraphicsResizeableItem::ControlType t)
        : rect(r), isHighlight(false), controlPointType(t)
    {
    }
    QRectF rect;
    bool isHighlight;
    DAGraphicsResizeableItem::ControlType controlPointType;
};

//===================================================
// DAGraphicsResizeableItem::PrivateData
//===================================================
class DAGraphicsResizeableItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAGraphicsResizeableItem)
public:
    PrivateData(DAGraphicsResizeableItem* p);
    //计算合理的尺寸
    QSizeF testBodySize(const QSizeF& ts) const;
    bool testBodySize(QSizeF& ts) const;
    //获取样式
    const DAGraphicsResizeableItemPalette& getPalette() const;
    //重置控制点位置信息
    void resetResizeableItemControlPointInfo();
    //
    void appendControlPointInfo(DAGraphicsResizeableItem::ControlType t, const QRectF& body);
    //
    QPair< DAGraphicsResizeableItem::ControlType, QRectF > getControlPointAndUpdateByPos(const QPointF& pos);
    //执行变换，返回需要移动的位置和尺寸，尺寸会考虑最大最小
    QPair< QPointF, QSizeF > doResize(const QPointF& mousescenePos);
    //按照记录鼠标按下的参数计算不同角度的点
    QPointF bodyConnerPoint(DAGraphicsResizeableItem::ControlType t);
    //位置坐标匹配网格
    void adjustPosToGrid(QPointF& pos);
    //位置坐标匹配网格
    void adjustSizeToGrid(QSizeF& s);

public:
    bool mEnableResize { true };                    ///< 是否允许调整大小
    bool mAutoCenterTransformOriginPoint { true };  ///< 自动更新TransformOriginPoint
    DAGraphicsResizeableItem::ControlType mCurrentControlTypeUnderMouse { DAGraphicsResizeableItem::NotUnderAnyControlType };  ///< 鼠标当前在的控制点
    DAGraphicsSceneWithUndoStack* mSceneUndo { nullptr };  ///保存secene
    QSizeF mSize { 30, 30 };                               ///< 尺寸
    QSizeF mMinSize { 5, 5 };                              ///< 最小尺寸
    QSizeF mMaxSize { 9999, 9999 };                        ///< 最大尺寸
    QPointF mPainterRectStartPos { 0, 0 };                 ///< 绘图范围的开始位置
    QSizeF mControlPointSize { 10, 10 };                   ///< 控制点的大小
    QList< DAGraphicsResizeableItemControlPointInfo > mControlPointInfos;
    //下面3个参数是鼠标点击后记录的三个状态
    QPointF mMousePressMouseOnScenePos;                           ///< 鼠标点击的scene位置
    QPointF mMousePressItemPos;                                   ///< 记录鼠标点击时item的位置
    QSizeF mMousePressItemSize;                                   ///< 记录鼠标按下时候的尺寸
    std::unique_ptr< DAGraphicsResizeableItemPalette > mPalette;  ///< 记录样式
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

const DAGraphicsResizeableItemPalette& DAGraphicsResizeableItem::PrivateData::getPalette() const
{
    if (mPalette) {
        return *mPalette;
    }
    return *(daGlobalGraphicsResizeableItemPalette);
}

void DAGraphicsResizeableItem::PrivateData::resetResizeableItemControlPointInfo()
{
    mControlPointInfos.clear();
    QRectF bd = q_ptr->getBodyRect();
    appendControlPointInfo(DAGraphicsResizeableItem::ControlPointTopLeft, bd);
    appendControlPointInfo(DAGraphicsResizeableItem::ControlPointTopMid, bd);
    appendControlPointInfo(DAGraphicsResizeableItem::ControlPointTopRight, bd);
    appendControlPointInfo(DAGraphicsResizeableItem::ControlPointRightMid, bd);
    appendControlPointInfo(DAGraphicsResizeableItem::ControlPointBottomRight, bd);
    appendControlPointInfo(DAGraphicsResizeableItem::ControlPointBottomMid, bd);
    appendControlPointInfo(DAGraphicsResizeableItem::ControlPointBottomLeft, bd);
    appendControlPointInfo(DAGraphicsResizeableItem::ControlPointLeftMid, bd);
    //控制线，注意控制线一定要比控制点设置靠后，否则会覆盖控制点的捕获
    appendControlPointInfo(DAGraphicsResizeableItem::ControlLineLeft, bd);
    appendControlPointInfo(DAGraphicsResizeableItem::ControlLineTop, bd);
    appendControlPointInfo(DAGraphicsResizeableItem::ControlLineRight, bd);
    appendControlPointInfo(DAGraphicsResizeableItem::ControlLineBottom, bd);
}

void DAGraphicsResizeableItem::PrivateData::appendControlPointInfo(DAGraphicsResizeableItem::ControlType t, const QRectF& body)
{
    mControlPointInfos.append(DAGraphicsResizeableItemControlPointInfo(q_ptr->controlPointRect(t, body), t));
}

QPair< DAGraphicsResizeableItem::ControlType, QRectF > DAGraphicsResizeableItem::PrivateData::getControlPointAndUpdateByPos(const QPointF& pos)
{
    QPair< DAGraphicsResizeableItem::ControlType, QRectF > res = qMakePair(DAGraphicsResizeableItem::NotUnderAnyControlType,
                                                                           QRectF());
    //    qDebug() << "getControlPointAndUpdateByPos(" << pos << ")";
    for (int i = 0; i < mControlPointInfos.size(); ++i) {
        DAGraphicsResizeableItemControlPointInfo& r = mControlPointInfos[ i ];
        if (r.rect.contains(pos)) {
            // qDebug() << "controlPointType=" << r.controlPointType << ",rect=" << r.rect << ",pos=" << pos;
            r.isHighlight = true;
            //如果位置在某个控制点里面
            res.first  = r.controlPointType;
            res.second = r.rect;
            //把剩下的设置为false
            for (int j = i + 1; j < mControlPointInfos.size(); ++j) {
                mControlPointInfos[ j ].isHighlight = false;
            }
            return res;
        } else {
            r.isHighlight = false;
        }
    }
    return res;
}

/**
 * @brief 执行尺寸的改变
 * @param mousescenePos
 * @return 返回<位移，尺寸>，如果这两个为isNull，则不执行对应的操作
 */
QPair< QPointF, QSizeF > DAGraphicsResizeableItem::PrivateData::doResize(const QPointF& mousescenePos)
{
    // df是鼠标按下到移动的距离
    QPointF df(mousescenePos - mMousePressMouseOnScenePos);
    DAGraphicsResizeableItemPrivatePrint("DAGraphicsResizeableItemPrivate::doResize(mousescenePos=QPointF(%g,%g))\n"
                                         "_mousePressMouseOnScenePos=QPointF(%g,%g)\n_mousePressItemSize=QSize(%g,%g)",
                                         mousescenePos.x(),
                                         mousescenePos.y(),
                                         mMousePressMouseOnScenePos.x(),
                                         mMousePressMouseOnScenePos.y(),
                                         mMousePressItemSize.width(),
                                         mMousePressItemSize.height());
    switch (mCurrentControlTypeUnderMouse) {
    case DAGraphicsResizeableItem::ControlPointTopLeft: {
        // topleft的移动，需要改变pos和size,且保证bottomright位置不变
        //右下角位置
        // ■-□-□
        // |   |
        // □   □
        // |   |
        // □-□-× <- fix
        QPointF bottomRight = bodyConnerPoint(DAGraphicsResizeableItem::ControlPointBottomRight);
        adjustPosToGrid(bottomRight);  //把坐标点转换为网格点
        QPointF newPos = mousescenePos;
        adjustPosToGrid(newPos);
        QSizeF newSize(bottomRight.x() - newPos.x(), bottomRight.y() - newPos.y());
        //        DAGraphicsResizeableItemPrivatePrint("doResize:ControlPointTopLeft\n"
        //                                             "bottomright=QPointF(%g,%g),newPos=QPointF(%g,%g),newSize=QSizeF(%g,%g)",
        //                                             bottomright.x(),
        //                                             bottomright.y(),
        //                                             newPos.x(),
        //                                             newPos.y(),
        //                                             newSize.width(),
        //                                             newSize.height());
        //执行变换
        q_ptr->setBodySize(newSize);
        //获取变换后的大小
        newSize = mSize;
        //以变换后的大小，重新调整位置
        newPos.setX(bottomRight.x() - newSize.width());
        newPos.setY(bottomRight.y() - newSize.height());
        q_ptr->setPos(newPos);
        return qMakePair(newPos, newSize);
    } break;
    case DAGraphicsResizeableItem::ControlPointTopMid: {
        //上下位置改变x不动
        //右下角位置
        //且保证bottomleft,bottomright位置不变
        // □-■-□
        // |   |
        // □   □
        // |   |
        // □-×-□
        //   ↑
        //  fix
        QPointF bottomMid = bodyConnerPoint(DAGraphicsResizeableItem::ControlPointBottomMid);
        adjustPosToGrid(bottomMid);
        QPointF newPos(mMousePressItemPos.x(), mousescenePos.y());
        //通过newpos计算size
        QSizeF newSize(mMousePressItemSize.width(), bottomMid.y() - newPos.y());
        adjustSizeToGrid(newSize);
        //执行变换
        q_ptr->setBodySize(newSize);
        //获取变换后的大小，有可能和设置的不一样，这样要调整一下newPos
        newSize = mSize;
        //以变换后的大小，重新调整位置
        newPos.setX(bottomMid.x() - newSize.width() / 2);
        newPos.setY(bottomMid.y() - newSize.height());
        q_ptr->setPos(newPos);
        return qMakePair(newPos, newSize);
    } break;
    case DAGraphicsResizeableItem::ControlPointTopRight: {
        // topright的移动，需要改变pos和size,且保证bottomleft位置不变
        // 要完全做到相对位置不变，需要都要以top-left的adjustPosToGrid位置作为参考
        // □-□-■
        // |   |
        // □   □
        // |   |
        // ×-□-□
        // ↑
        // fix
        QPointF bottomLeft = bodyConnerPoint(DAGraphicsResizeableItem::ControlPointBottomLeft);
        adjustPosToGrid(bottomLeft);
        QPointF newPos(mMousePressItemPos.x(), mousescenePos.y());
        QSizeF newSize(mousescenePos.x() - newPos.x(), bottomLeft.y() - mousescenePos.y());
        adjustSizeToGrid(newSize);
        //执行变换
        q_ptr->setBodySize(newSize);
        //获取变换后的大小
        newSize = mSize;
        //以变换后的大小，重新调整位置
        newPos.setX(bottomLeft.x());
        newPos.setY(bottomLeft.y() - newSize.height());
        q_ptr->setPos(newPos);
        return qMakePair(newPos, newSize);
    } break;
    case DAGraphicsResizeableItem::ControlPointRightMid: {
        // RightMid的移动，需要改变size,且保证bottomleft位置不变
        // fix
        // ↓
        // ×-□-□
        // |   |
        // □   ■
        // |   |
        // □-□-□
        QPointF topleft = bodyConnerPoint(DAGraphicsResizeableItem::ControlPointTopLeft);
        QSizeF newSize(mousescenePos.x() - topleft.x(), mMousePressItemSize.height());
        //执行变换
        q_ptr->setBodySize(newSize);
        //获取变换后的大小
        newSize = mSize;
        //由于位置不变，pos不变
        return qMakePair(topleft, newSize);
    } break;
    case DAGraphicsResizeableItem::ControlPointBottomRight: {
        // fix
        // ↓
        // ×-□-□
        // |   |
        // □   □
        // |   |
        // □-□-■
        QPointF topLeft = bodyConnerPoint(DAGraphicsResizeableItem::ControlPointTopLeft);
        QSizeF newSize(mousescenePos.x() - topLeft.x(), mousescenePos.y() - topLeft.y());
        //执行变换
        q_ptr->setBodySize(newSize);
        //获取变换后的大小
        newSize = mSize;
        //由于位置不变，pos不变
        return qMakePair(topLeft, newSize);
    } break;
    case DAGraphicsResizeableItem::ControlPointBottomMid: {
        // fix
        // ↓
        // ×-□-□
        // |   |
        // □   □
        // |   |
        // □-■-□
        QPointF topLeft = bodyConnerPoint(DAGraphicsResizeableItem::ControlPointTopLeft);
        QSizeF newSize(mMousePressItemSize.width(), mousescenePos.y() - topLeft.y());
        //执行变换
        q_ptr->setBodySize(newSize);
        //获取变换后的大小
        newSize = mSize;
        //由于位置不变，pos不变
        return qMakePair(topLeft, newSize);
    } break;
    case DAGraphicsResizeableItem::ControlPointBottomLeft: {
        //    fix
        //     ↓
        // □-□-×
        // |   |
        // □   □
        // |   |
        // ■-□-□
        QPointF topRight = bodyConnerPoint(DAGraphicsResizeableItem::ControlPointTopRight);
        adjustPosToGrid(topRight);
        QPointF newPos(mousescenePos.x(), mMousePressItemPos.y());
        //只要保证size也和grid倍数贴合，就能保证移动后不越出网格
        QSizeF newSize(topRight.x() - newPos.x(), mousescenePos.y() - newPos.y());
        adjustSizeToGrid(newSize);
        //执行变换
        q_ptr->setBodySize(newSize);
        //获取变换后的大小
        newSize = mSize;
        //以变换后的大小，重新调整位置
        newPos.setX(topRight.x() - newSize.width());
        newPos.setY(topRight.y());
        q_ptr->setPos(newPos);
        return qMakePair(newPos, newSize);
    } break;
    case DAGraphicsResizeableItem::ControlPointLeftMid: {
        //    fix
        //     ↓
        // □-□-×
        // |   |
        // ■   □
        // |   |
        // □-□-□
        QPointF topRight = bodyConnerPoint(DAGraphicsResizeableItem::ControlPointTopRight);
        adjustPosToGrid(topRight);
        QPointF newPos(mousescenePos.x(), mMousePressItemPos.y());
        QSizeF newSize(topRight.x() - mousescenePos.x(), mMousePressItemSize.height());
        adjustSizeToGrid(newSize);
        //执行变换
        q_ptr->setBodySize(newSize);
        //获取变换后的大小
        newSize = mSize;
        //以变换后的大小，重新调整位置
        newPos.setX(topRight.x() - newSize.width());
        newPos.setY(topRight.y());
        q_ptr->setPos(newPos);
        return qMakePair(newPos, newSize);
    } break;
    case DAGraphicsResizeableItem::ControlLineLeft:
    case DAGraphicsResizeableItem::ControlLineTop:
    case DAGraphicsResizeableItem::ControlLineRight:
    case DAGraphicsResizeableItem::ControlLineBottom: {
        //线都是移动处理
        QPointF newPos = mMousePressItemPos + df;
        q_ptr->setPos(newPos);
        return qMakePair(newPos, QSizeF());
    }
    default:
        break;
    }
    return qMakePair(QPointF(), QSizeF());
}

/**
 * @brief 通过鼠标按下记录的数据，计算不同角度的点
 * @param t
 * @return
 */
QPointF DAGraphicsResizeableItem::PrivateData::bodyConnerPoint(DAGraphicsResizeableItem::ControlType t)
{
    switch (t) {
    case DAGraphicsResizeableItem::ControlPointTopLeft:
        return mMousePressItemPos;
    case DAGraphicsResizeableItem::ControlPointTopMid:
        return QPointF(mMousePressItemPos.x() + mMousePressItemSize.width() / 2, mMousePressItemPos.y());
    case DAGraphicsResizeableItem::ControlPointTopRight:
        return QPointF(mMousePressItemPos.x() + mMousePressItemSize.width(), mMousePressItemPos.y());
    case DAGraphicsResizeableItem::ControlPointRightMid:
        return QPointF(mMousePressItemPos.x() + mMousePressItemSize.width(),
                       mMousePressItemPos.y() + mMousePressItemSize.height() / 2);
    case DAGraphicsResizeableItem::ControlPointBottomRight:
        return QPointF(mMousePressItemPos.x() + mMousePressItemSize.width(),
                       mMousePressItemPos.y() + mMousePressItemSize.height());
    case DAGraphicsResizeableItem::ControlPointBottomMid:
        return QPointF(mMousePressItemPos.x() + mMousePressItemSize.width() / 2,
                       mMousePressItemPos.y() + mMousePressItemSize.height());
    case DAGraphicsResizeableItem::ControlPointBottomLeft:
        return QPointF(mMousePressItemPos.x(), mMousePressItemPos.y() + mMousePressItemSize.height());
    case DAGraphicsResizeableItem::ControlPointLeftMid:
        return QPointF(mMousePressItemPos.x(), mMousePressItemPos.y() + mMousePressItemSize.height() / 2);
    default:
        break;
    }
    return QPointF();
}

/**
 * @brief 位置坐标匹配网格
 * @param pos
 */
void DAGraphicsResizeableItem::PrivateData::adjustPosToGrid(QPointF& pos)
{
    if (!q_ptr->isEnableSnapToGrid()) {
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

void DAGraphicsResizeableItem::PrivateData::adjustSizeToGrid(QSizeF& s)
{
    if (!q_ptr->isEnableSnapToGrid()) {
        return;
    }
    QSize gridsize = q_ptr->getGridSize();
    if (gridsize.isValid()) {
        s.setWidth(qRound(s.width() / gridsize.width()) * gridsize.width());
        s.setHeight(qRound(s.height() / gridsize.height()) * gridsize.height());
    }
}

//////////////////////////////////////////////////////////////////////////////
// DAGraphicsResizeableItem
//////////////////////////////////////////////////////////////////////////////

DAGraphicsResizeableItem::DAGraphicsResizeableItem(QGraphicsItem* parent) : DAGraphicsItem(parent), DA_PIMPL_CONSTRUCT
{
    setFlags(flags() | ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges  //确保位置改变时能发出QGraphicsItem::ItemPositionHasChanged
    );
    setAcceptHoverEvents(true);
    //初始化控制点
    prepareControlInfoChange();
    //
    d_ptr->mMousePressItemSize = getBodySize();
    d_ptr->mMousePressItemPos  = pos();
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
    return getBodyControlRect();
}

/**
 * @brief 对setPos的封装
 * @param p
 */
void DAGraphicsResizeableItem::setBodyPos(const QPointF& p)
{
    qreal wo = d_ptr->mControlPointSize.width() + 1;
    qreal ho = d_ptr->mControlPointSize.height() + 1;
    setPos(p.x() - wo, p.y() - ho);
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
 * @brief 设置TransformOriginPoint自动设置为bodysize的中心,否则为用户自己指定
 * @param on
 */
void DAGraphicsResizeableItem::setAutoCenterTransformOriginPoint(bool on)
{
    if (!d_ptr->mAutoCenterTransformOriginPoint && on) {
        //从否转为true，需要立即重写计算一下
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
 * @brief 执行尺寸的改变
 * @param mousescenePos
 * @return 返回<位移，尺寸>，如果为null，则不执行
 */
QPair< QPointF, QSizeF > DAGraphicsResizeableItem::doItemResize(const QPointF& mousescenePos)
{
    return d_ptr->doResize(mousescenePos);
}

void DAGraphicsResizeableItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsItem::hoverEnterEvent(event);
}

void DAGraphicsResizeableItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (isSelected()) {
        if (isEnableResize()) {
            QPair< ControlType, QRectF > pt = d_ptr->getControlPointAndUpdateByPos(event->pos());
            if (NotUnderAnyControlType == pt.first) {
                //这里说明都没在控制点上
                if (hasCursor()) {
                    unsetCursor();
                }
            } else {
                //说明在控制点上
                switch (pt.first) {
                case ControlPointTopLeft:
                case ControlPointBottomRight:
                    setCursor(Qt::SizeFDiagCursor);
                    break;
                case ControlPointTopMid:
                case ControlPointBottomMid:
                    setCursor(Qt::SizeVerCursor);
                    break;
                case ControlPointTopRight:
                case ControlPointBottomLeft:
                    setCursor(Qt::SizeBDiagCursor);
                    break;
                case ControlPointRightMid:
                case ControlPointLeftMid:
                    setCursor(Qt::SizeHorCursor);
                    break;
                case ControlLineBottom:
                case ControlLineLeft:
                case ControlLineRight:
                case ControlLineTop:
                    setCursor(Qt::SizeAllCursor);
                    break;
                default:  //不会达到
                {
                    if (hasCursor()) {
                        unsetCursor();
                    }
                } break;
                }
                if (d_ptr->mCurrentControlTypeUnderMouse != pt.first) {
                    update(pt.second);
                }
            }
            d_ptr->mCurrentControlTypeUnderMouse = pt.first;
            event->accept();  // accept该事件，停止对事件的转发
            return;
        } else {
            //非resize状态都把_currentControlTypeUnderMouse设置为NotAtControlPoint
            if (d_ptr->mCurrentControlTypeUnderMouse != NotUnderAnyControlType) {
                d_ptr->mCurrentControlTypeUnderMouse = NotUnderAnyControlType;
            }
        }
    } else {
        //非resize状态都把_currentControlTypeUnderMouse设置为NotAtControlPoint
        if (d_ptr->mCurrentControlTypeUnderMouse != NotUnderAnyControlType) {
            d_ptr->mCurrentControlTypeUnderMouse = NotUnderAnyControlType;
        }
    }
    QGraphicsItem::hoverMoveEvent(event);
}

void DAGraphicsResizeableItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    if (isEnableResize()) {
        if (NotUnderAnyControlType != d_ptr->mCurrentControlTypeUnderMouse) {
            d_ptr->mCurrentControlTypeUnderMouse = NotUnderAnyControlType;
            if (hasCursor()) {
                unsetCursor();
                event->accept();
                return;
            }
        }
    }
    QGraphicsItem::hoverLeaveEvent(event);
}

void DAGraphicsResizeableItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    //点击的时候判断点击的是哪里
    d_ptr->mMousePressMouseOnScenePos = event->scenePos();
    d_ptr->mMousePressItemPos         = pos();
    if (event->buttons().testFlag(Qt::LeftButton)) {
        if (isSelected() && isEnableResize()) {
            //开始改变尺寸
            if (d_ptr->mCurrentControlTypeUnderMouse != NotUnderAnyControlType) {
                //在控制点上
                d_ptr->mMousePressItemSize = d_ptr->mSize;
                event->accept();
                return;
            }
        }
    }
    QGraphicsItem::mousePressEvent(event);
}

void DAGraphicsResizeableItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (d_ptr->mCurrentControlTypeUnderMouse != NotUnderAnyControlType) {
        //说明在改变大小状态
        // doItemResize是估算要改变尺寸的位置和大小，由于改变大小可能和估算的不一样，因此要进行第二次估算
        doItemResize(event->scenePos());
        //接受鼠标移动事件，避免产生移动效果
        event->accept();
        return;
    }

    QGraphicsItem::mouseMoveEvent(event);
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
        QPointF newPos = value.toPointF();
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
        //记录scene
        d_ptr->mSceneUndo = qobject_cast< DAGraphicsSceneWithUndoStack* >(scene());
        break;
    }
    default:
        break;
    }
    return QGraphicsItem::itemChange(change, value);
}

void DAGraphicsResizeableItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (d_ptr->mCurrentControlTypeUnderMouse != NotUnderAnyControlType) {
        //说明在改变大小状态
        QPair< QPointF, QSizeF > res = doItemResize(event->scenePos());
        if (d_ptr->mSceneUndo) {
            DA::DACommandsForGraphicsItemResized* cmd = new DA::DACommandsForGraphicsItemResized(this,
                                                                                                 d_ptr->mMousePressItemPos,
                                                                                                 d_ptr->mMousePressItemSize,
                                                                                                 res.first,
                                                                                                 res.second,
                                                                                                 true  //已经执行了移动，第一次跳过执行
            );
            d_ptr->mSceneUndo->push(cmd);
        }
        //接受鼠标移动事件，避免产生移动效果
        event->accept();
        return;
    }
    QGraphicsItem::mouseReleaseEvent(event);
}
/**
 * @brief 用户不要继承此shape函数，而是继承bodyShape函数
 * @return
 */
QPainterPath DAGraphicsResizeableItem::shape() const
{
    QPainterPath p = getBodyShape();
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
    //绘制缩放用的
    QRectF bodyrect = getBodyRect();
    paintBackground(painter, option, widget, bodyrect);
    paintBorder(painter, option, widget, bodyrect);
    paintBody(painter, option, widget, bodyrect);
    if (isEnableResize()) {
        if (isSelected()) {
            //绘制缩放的边框
            paintSelectedBorder(painter, option, widget);
            //绘制缩放的点
            paintResizeControlPoints(painter, option, widget);
        }
    } else {
        //不能改变大小但可选中
        if (flags().testFlag(ItemIsSelectable)) {
            //可选择
            if (isSelected()) {
                //绘制缩放的边框
                paintSelectedBorder(painter, option, widget);
            }
        }
    }
}

bool DAGraphicsResizeableItem::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    DAGraphicsItem::saveToXml(doc, parentElement);
    QDomElement rsinfoEle = doc->createElement("resize-info");
    rsinfoEle.setAttribute("enableResize", isEnableResize());
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
    bs                 = getControlerSize();
    conEle.setAttribute("width", bs.width());
    conEle.setAttribute("height", bs.height());

    rsinfoEle.appendChild(conEle);
    parentElement->appendChild(rsinfoEle);
    return true;
}

bool DAGraphicsResizeableItem::loadFromXml(const QDomElement* itemElement)
{
    if (!DAGraphicsItem::loadFromXml(itemElement)) {
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
            setControlerSize(QSizeF(v1, v2));
        }
    }
    return true;
}

/**
 * @brief 测试一下setBodySize之后getBodySize能得到的尺寸
 * @param s
 * @return
 */
QSizeF DAGraphicsResizeableItem::testBodySize(const QSizeF& s)
{
    return d_ptr->testBodySize(s);
}

/**
 * @brief 设置尺寸
 * @note setBodySize是虚函数，在scene鼠标动作的时候会触发此函数，
 * 如果仅仅想改变bodysize的尺寸，可以调用@sa changeBodySize
 * @param s
 */
void DAGraphicsResizeableItem::setBodySize(const QSizeF& s)
{
    QSizeF cs = testBodySize(s);
    if (cs != d_ptr->mSize) {
        QSizeF oldsize = d_ptr->mSize;
        changeBodySize(s);
        prepareGeometryChange();
        if (d_ptr->mSceneUndo) {
            d_ptr->mSceneUndo->emitItemBodySizeChanged(this, oldsize, d_ptr->mSize);
        }
#if DA_USE_QGRAPHICSOBJECT
        emit itemBodySizeChanged(oldsize, d_ptr->mSize);
#endif
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
 * @brief 获取body包含控制窗口大小，就是在改变尺寸时包含那8个控制点的最大尺寸
 * @return
 */
QRectF DAGraphicsResizeableItem::getBodyControlRect() const
{
    QRectF body = getBodyRect();
    qreal wo    = d_ptr->mControlPointSize.width() + 1;
    qreal ho    = d_ptr->mControlPointSize.height() + 1;
    body.adjust(-wo, -ho, wo, ho);
    return body;
}

/**
 * @brief 获取绘图的shape
 * @return
 */
QPainterPath DAGraphicsResizeableItem::getBodyShape() const
{
    QPainterPath p;
    p.addRect(getBodyControlRect());
    return p;
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
 * @brief 获取控制器的大小
 * @param s
 */
void DAGraphicsResizeableItem::setControlerSize(const QSizeF& s)
{
    d_ptr->mControlPointSize = s;
    prepareControlInfoChange();
}

QSizeF DAGraphicsResizeableItem::getControlerSize() const
{
    return d_ptr->mControlPointSize;
}
/**
 * @brief 设置是否Delegate可用
 * @param on
 */
void DAGraphicsResizeableItem::setEnableResize(bool on)
{
    d_ptr->mEnableResize = on;
    if (on) {
        if (!acceptHoverEvents()) {
            setAcceptHoverEvents(on);
        }
    }
}
/**
 * @brief 判断是否允许
 * @return
 */
bool DAGraphicsResizeableItem::isEnableResize() const
{
    return d_ptr->mEnableResize;
}

/**
 * @brief 绘制resize边框
 * @param painter
 * @param option
 * @param widget
 */
void DAGraphicsResizeableItem::paintSelectedBorder(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    painter->save();
    qreal wo = d_ptr->mControlPointSize.width() / 2;
    qreal ho = d_ptr->mControlPointSize.height() / 2;
    QPen pen(d_ptr->getPalette().resizeBorderColor);
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    //绘制缩放边框
    painter->drawRect(option->rect.adjusted(wo, ho, -wo, -ho));
    painter->restore();
}

/**
 * @brief  绘制resize控制点
 * @param painter
 * @param option
 * @param widget
 * @param bodyRect
 */
void DAGraphicsResizeableItem::paintResizeControlPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);

    const DAGraphicsResizeableItemPalette& palette = d_ptr->getPalette();
    QPen pen(palette.resizeControlPointBorderColor);
    pen.setStyle(Qt::SolidLine);
    painter->save();
    painter->setBrush(palette.resizeControlPointBrush);
    for (const DAGraphicsResizeableItemControlPointInfo& r : qAsConst(d_ptr->mControlPointInfos)) {
        switch (r.controlPointType) {
        case ControlPointTopLeft:
        case ControlPointTopMid:
        case ControlPointTopRight:
        case ControlPointRightMid:
        case ControlPointBottomRight:
        case ControlPointBottomMid:
        case ControlPointBottomLeft:
        case ControlPointLeftMid:
            painter->drawRect(r.rect);
        default:
            break;
        }
    }
    painter->restore();
}

/**
 * @brief 绘制背景
 * @param painter
 * @param option
 * @param widget
 * @param bodyRect
 */
void DAGraphicsResizeableItem::paintBackground(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
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
void DAGraphicsResizeableItem::paintBorder(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QRectF& bodyRect)
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
 * @brief 生成control points
 * @param tp
 * @return
 */
QRectF DAGraphicsResizeableItem::controlPointRect(ControlType tp, const QRectF& bodyRect) const
{
    QSizeF ss = controlPointSize();
    QRectF cr(QPointF(0, 0), ss);
    //说明在控制点上
    switch (tp) {
    case ControlPointTopLeft:
        cr.moveTo(bodyRect.topLeft() - QPointF(ss.width(), ss.height()));
        break;
    case ControlPointTopMid:
        cr.moveTo(QPointF(bodyRect.left() + bodyRect.width() / 2 - ss.width() / 2, bodyRect.top() - ss.height()));
        break;
    case ControlPointTopRight:
        cr.moveTo(bodyRect.topRight() - QPointF(0, ss.height()));
        break;
    case ControlPointRightMid:
        cr.moveTo(QPointF(bodyRect.right(), bodyRect.top() + bodyRect.height() / 2 - ss.height() / 2));
        break;
    case ControlPointBottomRight:
        cr.moveTo(bodyRect.bottomRight());
        break;
    case ControlPointBottomMid:
        cr.moveTo(QPointF(bodyRect.left() + bodyRect.width() / 2 - ss.width() / 2, bodyRect.bottom()));
        break;
    case ControlPointBottomLeft:
        cr.moveTo(bodyRect.bottomLeft() - QPointF(ss.width(), 0));
        break;
    case ControlPointLeftMid:
        cr.moveTo(QPointF(bodyRect.left() - ss.width(), bodyRect.top() + bodyRect.height() / 2 - ss.height() / 2));
        break;
    case ControlLineLeft:
        //控制线
        cr = QRectF(bodyRect.topLeft() - QPointF(ss.width(), 0), QSizeF(ss.width(), bodyRect.height()));
        break;
    case ControlLineTop:
        //控制线
        cr = QRectF(bodyRect.topLeft() - QPointF(0, ss.height()), QSizeF(bodyRect.width(), ss.height()));
        break;
    case ControlLineRight:
        //控制线
        cr = QRectF(bodyRect.topRight(), QSizeF(ss.width(), bodyRect.height()));
        break;
    case ControlLineBottom:
        //控制线
        cr = QRectF(bodyRect.bottomLeft(), QSizeF(bodyRect.width(), ss.height()));
        break;
    default:
        break;
    }
    return cr;
}

/**
 * @brief 控制点的大小
 * @return
 */
QSizeF DAGraphicsResizeableItem::controlPointSize() const
{
    return d_ptr->mControlPointSize;
}

/**
 * @brief 在尺寸发生变化后调用，刷新控制点的位置
 */
void DAGraphicsResizeableItem::prepareControlInfoChange()
{
    d_ptr->resetResizeableItemControlPointInfo();
}
/**
 * @brief 测试位置是否在控制点上，如果是返回控制点的类型，如果不在返回NotAtControlPoint
 * @param pos
 * @return
 */
DAGraphicsResizeableItem::ControlType DAGraphicsResizeableItem::getControlPointByPos(const QPointF& pos) const
{
    for (const DAGraphicsResizeableItemControlPointInfo& r : qAsConst(d_ptr->mControlPointInfos)) {
        if (r.rect.contains(pos)) {
            //如果位置在某个控制点里面
            return r.controlPointType;
        }
    }
    return NotUnderAnyControlType;
}
/**
 * @brief 判断当前是否处于调整大小的状态中
 * @return 如果当前在调整大小，返回true
 */
bool DAGraphicsResizeableItem::isResizing() const
{
    return d_ptr->mCurrentControlTypeUnderMouse != NotUnderAnyControlType;
}

/**
 * @brief 设置为是否可移动
 * @param on
 */
void DAGraphicsResizeableItem::setMovable(bool on)
{
    setFlag(ItemIsMovable, on);
}

/**
 * @brief 判断是否可以移动
 * @return
 */
bool DAGraphicsResizeableItem::isMovable() const
{
    return flags().testFlag(ItemIsMovable);
}
/**
 * @brief 是否允许对齐网格
 * @return
 */
bool DAGraphicsResizeableItem::isEnableSnapToGrid() const
{
    DAGraphicsSceneWithUndoStack* sc = d_ptr->mSceneUndo;
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
    DAGraphicsSceneWithUndoStack* sc = qobject_cast< DAGraphicsSceneWithUndoStack* >(scene());
    if (sc) {
        return sc->getGridSize();
    }
    return QSize();
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
        prepareControlInfoChange();
    }
}

}  // end namespace DA
