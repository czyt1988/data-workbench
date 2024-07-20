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
#include "DAWorkFlow.h"
namespace DA
{
class DAAbstractNodeLinkGraphicsItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAAbstractNodeLinkGraphicsItem)
public:
    PrivateData(DAAbstractNodeLinkGraphicsItem* p);
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
    DAAbstractNodeLinkGraphicsItem::LinkLineStyle mLinkLineStyle { DAAbstractNodeLinkGraphicsItem::LinkLineKnuckle };
    DAAbstractNodeGraphicsItem* mFromItem { nullptr };
    DAAbstractNodeGraphicsItem* mToItem { nullptr };
    DANodeLinkPoint mFromPoint;
    DANodeLinkPoint mToPoint;
    QGraphicsSimpleTextItem* mFromTextItem { nullptr };
    QGraphicsSimpleTextItem* mToTextItem { nullptr };
    QPair< int, int > mPointTextPositionOffset { 10, 10 };  ///< 记录文本和连接点的偏移量，默认为10
    QGraphicsSimpleTextItem* mTextItem { nullptr };         ///< 文本item，文本item默认为false
    QPointF mTextPosProportion { 0.5, 0.5 };                ///< 文本位置占比
    bool mAutoDetachLink { true };                          ///< 默认为true，在析构时自动detach link
    int mTextItemSpace { 5 };                               ///< 文字离中心点的距离
};

//===================================================
// DAAbstractNodeLinkGraphicsItemPrivate
//===================================================
DAAbstractNodeLinkGraphicsItem::PrivateData::PrivateData(DAAbstractNodeLinkGraphicsItem* p)
    : q_ptr(p)
    , mLinkLineStyle(DAAbstractNodeLinkGraphicsItem::LinkLineKnuckle)
    , mFromItem(nullptr)
    , mToItem(nullptr)
    , mPointTextPositionOffset(10, 10)
    , mTextItem(nullptr)
    , mTextPosProportion(0.5, 0.5)
    , mAutoDetachLink(true)
    , mTextItemSpace(5)
{
    mFromTextItem = new QGraphicsSimpleTextItem(p);
    mToTextItem   = new QGraphicsSimpleTextItem(p);
    setLinkPointNameVisible(false, DAGraphicsLinkItem::OrientationBoth);
}

DANodeGraphicsScene* DAAbstractNodeLinkGraphicsItem::PrivateData::nodeScene() const
{
    return (qobject_cast< DANodeGraphicsScene* >(q_ptr->scene()));
}

void DAAbstractNodeLinkGraphicsItem::PrivateData::setLinkPointNameVisible(bool on,
                                                                          DAAbstractNodeLinkGraphicsItem::Orientations o)
{
    switch (o) {
    case DAGraphicsLinkItem::OrientationStart:
        mFromTextItem->setVisible(on);
        break;

    case DAGraphicsLinkItem::OrientationEnd:
        mToTextItem->setVisible(on);
        break;

    default:
        mFromTextItem->setVisible(on);
        mToTextItem->setVisible(on);
        break;
    }
}

bool DAAbstractNodeLinkGraphicsItem::PrivateData::isLinkPointNameVisible(DAAbstractNodeLinkGraphicsItem::Orientations o) const
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationStart:
        return (mFromTextItem->isVisible());

    case DAAbstractNodeLinkGraphicsItem::OrientationEnd:
        return (mToTextItem->isVisible());

    default:
        break;
    }
    return (mFromTextItem->isVisible() && mToTextItem->isVisible());
}

void DAAbstractNodeLinkGraphicsItem::PrivateData::updateLinkPointNameText(QGraphicsSimpleTextItem* item,
                                                                          const QPointF& p,
                                                                          const DANodeLinkPoint& pl,
                                                                          int offset)
{
    item->setText(pl.name);
    int hoff = item->boundingRect().height();

    hoff /= 2;
    int w = item->boundingRect().width();

    switch (pl.direction) {
    case AspectDirection::East:
        item->setRotation(0);
        item->setPos(p.x() + offset, p.y() - hoff);
        break;

    case AspectDirection::South:
        item->setRotation(90);
        item->setPos(p.x() + hoff, p.y() + offset);
        break;

    case AspectDirection::West:
        item->setRotation(0);
        item->setPos(p.x() - w - offset, p.y() - hoff);
        break;

    case AspectDirection::North:
        item->setRotation(90);
        item->setPos(p.x() + hoff, p.y() - w - offset);
        break;

    default:
        break;
    }
}

void DAAbstractNodeLinkGraphicsItem::PrivateData::updateLinkPointNameText()
{
    updateLinkPointNameText(mFromTextItem, q_ptr->getStartPosition(), mFromPoint, mPointTextPositionOffset.first);
    updateLinkPointNameText(mToTextItem, q_ptr->getEndPosition(), mToPoint, mPointTextPositionOffset.second);
}

void DAAbstractNodeLinkGraphicsItem::PrivateData::setPointTextColor(const QColor& c,
                                                                    DAAbstractNodeLinkGraphicsItem::Orientations o)
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationStart:
        mFromTextItem->setBrush(c);
        break;

    case DAAbstractNodeLinkGraphicsItem::OrientationEnd:
        mToTextItem->setBrush(c);
        break;

    default:
        mFromTextItem->setBrush(c);
        mToTextItem->setBrush(c);
        break;
    }
}

QColor DAAbstractNodeLinkGraphicsItem::PrivateData::getPointTextColor(DAAbstractNodeLinkGraphicsItem::Orientations o) const
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationStart:
        return (mFromTextItem->brush().color());

    case DAAbstractNodeLinkGraphicsItem::OrientationEnd:
        return (mToTextItem->brush().color());

    default:
        break;
    }
    return (QColor());
}

void DAAbstractNodeLinkGraphicsItem::PrivateData::setPointTextPositionOffset(int offset,
                                                                             DAAbstractNodeLinkGraphicsItem::Orientations o)
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationStart:
        mPointTextPositionOffset.first = offset;
        break;

    case DAAbstractNodeLinkGraphicsItem::OrientationEnd:
        mPointTextPositionOffset.second = offset;
        break;

    default:
        mPointTextPositionOffset.first  = offset;
        mPointTextPositionOffset.second = offset;
        break;
    }
}

int DAAbstractNodeLinkGraphicsItem::PrivateData::getPointTextPositionOffset(DAAbstractNodeLinkGraphicsItem::Orientations o) const
{
    switch (o) {
    case DAAbstractNodeLinkGraphicsItem::OrientationStart:
        return (mPointTextPositionOffset.first);

    case DAAbstractNodeLinkGraphicsItem::OrientationEnd:
        return (mPointTextPositionOffset.second);

    default:
        break;
    }
    return (0);
}

/**
 * @brief 处于连接状态中，开始有，结束还未有
 * @return
 */
bool DAAbstractNodeLinkGraphicsItem::PrivateData::isStartLinking() const
{
    return ((mFromItem != nullptr) && (mToItem == nullptr));
}

/**
 * @brief 已经完成连接
 * @return
 */
bool DAAbstractNodeLinkGraphicsItem::PrivateData::isLinked() const
{
    return ((mFromItem != nullptr) && (mToItem != nullptr));
}
/**
 * @brief 更新文本位置
 */
void DAAbstractNodeLinkGraphicsItem::PrivateData::updateTextPos()
{
    if (nullptr == mTextItem) {
        return;
    }
    QPointF cp = q_ptr->getLinkLinePainterPath().pointAtPercent(0.5);
    mTextItem->setPos(cp.x() + mTextItemSpace, cp.y());
}

//////////////////////////////////////////////////////////////
/// DAAbstractNodeLinkGraphicsItem
//////////////////////////////////////////////////////////////

DAAbstractNodeLinkGraphicsItem::DAAbstractNodeLinkGraphicsItem(QGraphicsItem* p)
    : DAGraphicsLinkItem(p), DA_PIMPL_CONSTRUCT
{
    setFlags(flags() | ItemIsSelectable);
    setEndPointType(OrientationStart, EndPointNone);
    setEndPointType(OrientationEnd, EndPointTriangType);
}

DAAbstractNodeLinkGraphicsItem::DAAbstractNodeLinkGraphicsItem(DAAbstractNodeGraphicsItem* from,
                                                               const DA::DANodeLinkPoint& pl,
                                                               QGraphicsItem* p)
    : DAGraphicsLinkItem(p), d_ptr(new DAAbstractNodeLinkGraphicsItem::PrivateData(this))
{
    setFlags(flags() | ItemIsSelectable);
    attachFrom(from, pl);
    setEndPointType(OrientationStart, EndPointNone);
    setEndPointType(OrientationEnd, EndPointTriangType);
    setZValue(-1);  // 连接线在-1层，这样避免在节点上面
}

DAAbstractNodeLinkGraphicsItem::~DAAbstractNodeLinkGraphicsItem()
{
    // 析构时d调用FCAbstractNodeGraphicsItem::callItemLinkIsDestroying移除item对应记录的link
    if (d_ptr->mAutoDetachLink) {
        detachFrom();
        detachTo();
    }
}

/**
 * @brief 自动根据fromitem来更新位置
 * @note 如果设置了toitem，会调用@sa updateBoundingRect 来更新boundingRect
 */
void DAAbstractNodeLinkGraphicsItem::updatePos()
{
    DANodeGraphicsScene* sc = d_ptr->nodeScene();

    if ((nullptr == d_ptr->mFromItem) || (nullptr == sc)) {
        return;
    }
    setStartScenePosition(d_ptr->mFromItem->mapToScene(d_ptr->mFromPoint.position));
    if (d_ptr->mToItem) {
        setEndScenePosition(d_ptr->mToItem->mapToScene(d_ptr->mToPoint.position));
    }
    updateBoundingRect();
}

/**
 * @brief 更新范围
 *
 * @note 争对只有一个起始连接点的情况下，此函数的终止链接点将更新为场景鼠标所在
 */
QRectF DAAbstractNodeLinkGraphicsItem::updateBoundingRect()
{
    QRectF r = DAGraphicsLinkItem::updateBoundingRect();
    d_ptr->updateLinkPointNameText();
    d_ptr->updateTextPos();
    return r;
}

/**
 * @brief 通过两个点形成一个矩形，两个点总能形成一个矩形，如果重合，返回一个空矩形
 * @param p0
 * @param p1
 * @return
 */
QRectF DAAbstractNodeLinkGraphicsItem::rectFromTwoPoint(const QPointF& p0, const QPointF& p1)
{
    return (QRectF(QPointF(qMin(p0.x(), p1.x()), qMin(p0.y(), p1.y())),
                   QPointF(qMax(p0.x(), p1.x()), qMax(p0.y(), p1.y()))));
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
void DAAbstractNodeLinkGraphicsItem::setLinkPointNameTextColor(const QColor& c,
                                                               DAAbstractNodeLinkGraphicsItem::Orientations o)
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
void DAAbstractNodeLinkGraphicsItem::setLinkPointNamePositionOffset(int offset,
                                                                    DAAbstractNodeLinkGraphicsItem::Orientations o)
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
    return (d_ptr->mFromTextItem);
}

QGraphicsSimpleTextItem* DAAbstractNodeLinkGraphicsItem::getToTextItem() const
{
    return (d_ptr->mToTextItem);
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
        item->finishLink(pl, this, DANodeLinkPoint::Output, false);
        return (false);
    }
    if (!pl.isOutput()) {
        // from必须从out出发
        qDebug() << QObject::tr("Link from must attach an output point");
        item->finishLink(pl, this, DANodeLinkPoint::Output, false);
        return (false);
    }
    d_ptr->mFromItem  = item;
    d_ptr->mFromPoint = pl;
    d_ptr->updateLinkPointNameText();
    item->recordLinkInfo(this, pl);
    // 这里不做item->finishLink(pl, this, DANodeLinkPoint::Output, true);的回调调用，因为开始连接还不算finishlink，只有attachto结束后才算
    DAAbstractNodeGraphicsItem* ti = toNodeItem();
    if (ti) {
        // 终点已经链接
        item->finishLink(pl, this, DANodeLinkPoint::Output, true);
        ti->finishLink(toNodeLinkPoint(), this, DANodeLinkPoint::Input, true);
        finishedLink();
    }
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
    if (d_ptr->mFromItem) {
        d_ptr->mFromItem->removeLinkInfo(this, d_ptr->mFromPoint);
        auto node = d_ptr->mFromItem->node();
        if (!node) {
            qCritical() << tr("error:link item can not get from node");  // cn:错误:无法通过图元获取到对应的节点
            return;
        }
        node->detachLink(d_ptr->mFromPoint.name);  // 断开连接
        d_ptr->mFromItem->detachLink(d_ptr->mFromPoint, this, DANodeLinkPoint::Output);
        d_ptr->mFromItem = nullptr;
    }
    d_ptr->mFromPoint = DANodeLinkPoint();
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
        item->finishLink(pl, this, DANodeLinkPoint::Input, false);
        return (false);
    }
    if (!pl.isInput()) {
        // to必须到in
        qDebug() << QObject::tr("Link to must attach an input point");
        item->finishLink(pl, this, DANodeLinkPoint::Input, false);
        return (false);
    }
    // 这个函数才完成一个节点的连接
    DAAbstractNode::SharedPointer fnode = d_ptr->mFromItem->node();
    DAAbstractNode::SharedPointer tnode = item->node();
    if (!fnode) {
        qCritical() << tr("error,link item can not get from node");  // cn:异常，链接线无法获取开始节点
        return false;
    }
    if (!tnode) {
        qCritical() << tr("error,link item can not get to node");  // cn:异常，链接线无法获取结束节点
        return false;
    }
    if (!fnode->linkTo(d_ptr->mFromPoint.name, tnode, pl.name)) {
        // linkTo会触发DAAbstractNodeFactory::nodeLinkedSucceed回调，但这个回调里面如果获取graphicsitem信息，
        // 是不完整的，因为graphicsitem信息在下面才更新
        item->finishLink(pl, this, DANodeLinkPoint::Input, false);
        return (false);
    }
    d_ptr->mToItem  = item;
    d_ptr->mToPoint = pl;
    d_ptr->updateLinkPointNameText();
    item->recordLinkInfo(this, pl);  // 记录链接信息
    DAAbstractNodeGraphicsItem* fi = fromNodeItem();
    if (fi) {
        // 起点已经链接
        fi->finishLink(fromNodeLinkPoint(), this, DANodeLinkPoint::Output, true);
        item->finishLink(pl, this, DANodeLinkPoint::Input, true);
        finishedLink();
    }
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
    if (d_ptr->mToItem) {
        d_ptr->mToItem->removeLinkInfo(this, d_ptr->mToPoint);
        d_ptr->mToItem->node()->detachLink(d_ptr->mToPoint.name);  // 断开连接
        d_ptr->mToItem->detachLink(d_ptr->mToPoint, this, DANodeLinkPoint::Input);
        d_ptr->mToItem = nullptr;
    }
    d_ptr->mToPoint = DANodeLinkPoint();
}

/**
 * @brief 已经连接完成，在from和to都有节点时，返回true
 * @return
 */
bool DAAbstractNodeLinkGraphicsItem::isLinked() const
{
    return d_ptr->isLinked();
}

bool DAAbstractNodeLinkGraphicsItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
    DAGraphicsLinkItem::saveToXml(doc, parentElement, ver);
    QDomElement pointEle = doc->createElement("linkPoint");
    pointEle.setAttribute("visible", isLinkPointNameVisible());
    pointEle.setAttribute("fromTextColor", getLinkPointNameTextColor(DAGraphicsLinkItem::OrientationStart).name());
    pointEle.setAttribute("toTextColor", getLinkPointNameTextColor(DAGraphicsLinkItem::OrientationEnd).name());
    pointEle.setAttribute("fromPositionOffset", getLinkPointNamePositionOffset(DAGraphicsLinkItem::OrientationStart));
    pointEle.setAttribute("toPositionOffset", getLinkPointNamePositionOffset(DAGraphicsLinkItem::OrientationEnd));
    parentElement->appendChild(pointEle);
    return true;
}

bool DAAbstractNodeLinkGraphicsItem::loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver)
{
    bool on              = DAGraphicsLinkItem::loadFromXml(parentElement, ver);
    QDomElement pointEle = parentElement->firstChildElement("linkPoint");
    if (!pointEle.isNull()) {
        bool visible = pointEle.attribute("visible").toInt();
        QColor fromTextColor(pointEle.attribute("fromTextColor"));
        QColor toTextColor(pointEle.attribute("toTextColor"));
        int fromPositionOffset = pointEle.attribute("fromPositionOffset").toInt();
        int toPositionOffset   = pointEle.attribute("toPositionOffset").toInt();
        setLinkPointNameVisible(visible);
        setLinkPointNamePositionOffset(fromPositionOffset, DAGraphicsLinkItem::OrientationStart);
        setLinkPointNamePositionOffset(toPositionOffset, DAGraphicsLinkItem::OrientationEnd);
        setLinkPointNameTextColor(fromTextColor, DAGraphicsLinkItem::OrientationStart);
        setLinkPointNameTextColor(toTextColor, DAGraphicsLinkItem::OrientationEnd);
    }
    return on;
}

/**
 * @brief 更新连接点信息
 * @param pl
 */
void DAAbstractNodeLinkGraphicsItem::updateFromLinkPointInfo(const DANodeLinkPoint& pl)
{
    d_ptr->mFromPoint = pl;
    updatePos();
}
/**
 * @brief 更新连接点信息
 * @param pl
 */
void DAAbstractNodeLinkGraphicsItem::updateToLinkPointInfo(const DANodeLinkPoint& pl)
{
    d_ptr->mToPoint = pl;
    updatePos();
}

/**
 * @brief 设置文本
 * @param t
 */
void DAAbstractNodeLinkGraphicsItem::setText(const QString& t)
{
    // 设置null字符就销毁item
    if (t.isNull()) {
        if (d_ptr->mTextItem) {
            delete d_ptr->mTextItem;
            d_ptr->mTextItem = nullptr;
            update();
        }
        return;
    }
    if (d_ptr->mTextItem == nullptr) {
        d_ptr->mTextItem = new QGraphicsSimpleTextItem(this);
        d_ptr->mTextItem->setFlag(ItemIsSelectable, true);
        d_ptr->mTextItem->setFlag(ItemIsMovable, false);
        // 默认位置在中间
        d_ptr->updateTextPos();
    }
    d_ptr->mTextItem->setText(t);
}

/**
 * @brief 获取文本
 * @return
 */
QString DAAbstractNodeLinkGraphicsItem::getText() const
{
    if (d_ptr->mTextItem == nullptr) {
        return QString();
    }
    return d_ptr->mTextItem->text();
}

/**
 * @brief 获取文本对应的item
 * @return
 */
QGraphicsSimpleTextItem* DAAbstractNodeLinkGraphicsItem::getTextItem()
{
    return d_ptr->mTextItem;
}

/**
 * @brief 获取from node item，如果没有返回nullptr
 * @return
 */
DAAbstractNodeGraphicsItem* DAAbstractNodeLinkGraphicsItem::fromNodeItem() const
{
    return d_ptr->mFromItem;
}

/**
 * @brief 获取to node item，如果没有返回nullptr
 * @return
 */
DAAbstractNodeGraphicsItem* DAAbstractNodeLinkGraphicsItem::toNodeItem() const
{
    return d_ptr->mToItem;
}
/**
 * @brief from的连接点
 * @return
 */
DANodeLinkPoint DAAbstractNodeLinkGraphicsItem::fromNodeLinkPoint() const
{
    return d_ptr->mFromPoint;
}
/**
 * @brief to的连接点
 * @return
 */
DANodeLinkPoint DAAbstractNodeLinkGraphicsItem::toNodeLinkPoint() const
{
    return d_ptr->mToPoint;
}

/**
 * @brief 获取from的节点，如果没有返回nullptr
 * @return
 */
DAAbstractNode::SharedPointer DAAbstractNodeLinkGraphicsItem::fromNode() const
{
    if (nullptr == d_ptr->mFromItem) {
        return nullptr;
    }
    return d_ptr->mFromItem->node();
}

/**
 * @brief 获取to的节点，如果没有返回nullptr
 * @return
 */
DAAbstractNode::SharedPointer DAAbstractNodeLinkGraphicsItem::toNode() const
{
    if (nullptr == d_ptr->mToItem) {
        return nullptr;
    }
    return d_ptr->mToItem->node();
}

/**
 * @brief 完成连接
 */
void DAAbstractNodeLinkGraphicsItem::finishedLink()
{
    // 默认不做任何处理
}

/**
 * @brief 节点的连接线在确认点击链接到完成才能完成链接
 * @return
 */
bool DAAbstractNodeLinkGraphicsItem::willCompleteLink()
{
    return isLinked();
}

QPainterPath DAAbstractNodeLinkGraphicsItem::generateLinePainterPath(const QPointF& fromPoint,
                                                                     const QPointF& toPoint,
                                                                     DAGraphicsLinkItem::LinkLineStyle linestyle)
{
    QPainterPath res;
    switch (linestyle) {
    case LinkLineBezier:
        res = generateLinkLineBezierPainterPath(fromPoint, d_ptr->mFromPoint.direction, toPoint, d_ptr->mToPoint.direction);
        break;
    case LinkLineStraight:
        res = generateLinkLineStraightPainterPath(fromPoint, toPoint);
        break;
    case LinkLineKnuckle:
        res = generateLinkLineKnucklePainterPath(fromPoint, d_ptr->mFromPoint.direction, toPoint, d_ptr->mToPoint.direction);
        break;
    default:
        break;
    }
    return res;
}

QVariant DAAbstractNodeLinkGraphicsItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case QGraphicsItem::ItemSelectedHasChanged:
        setLinkPointNameVisible(value.toBool());
        break;

    case QGraphicsItem::ItemSelectedChange:
        // 在连接状态中不允许选中
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
    if ((d_ptr->mFromItem == item) && (d_ptr->mFromPoint == pl)) {
        // 说明from要取消
        d_ptr->mFromItem  = nullptr;
        d_ptr->mFromPoint = DANodeLinkPoint();
    } else if ((d_ptr->mToItem == item) && (d_ptr->mToPoint == pl)) {
        // 说明to要取消
        qDebug() << "d_ptr->_toItem = nullptr";
        d_ptr->mToItem  = nullptr;
        d_ptr->mToPoint = DANodeLinkPoint();
    }
    // 如果from和to都为空，这时就需要自动销毁
    DANodeGraphicsScene* sc = d_ptr->nodeScene();

    if (sc) {
        sc->callNodeItemLinkIsEmpty(this);
    }
}

}  // namespace DA
