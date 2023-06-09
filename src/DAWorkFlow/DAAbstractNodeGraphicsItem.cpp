#include "DAAbstractNodeGraphicsItem.h"
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QDomDocument>
#include <QDomElement>
#include "DAAbstractNode.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include "DAAbstractNodeWidget.h"
#include "DANodeGraphicsScene.h"
#include "DANodePalette.h"
#include "DANodeLinkPointDrawDelegate.h"

/**
 * @def 调试打印
 */
#define DA_DAABSTRACTNODEGRAPHICSITEM_DEBUG_PRINT 0
////////////////////////////////////////////////////////////////////////
namespace DA
{
class DAAbstractNodeGraphicsItemPrivate
{
    DA_IMPL_PUBLIC(DAAbstractNodeGraphicsItem)
public:
    class LinkInfo
    {
    public:
        LinkInfo();
        LinkInfo(const DANodeLinkPoint& p);
        QList< DAAbstractNodeLinkGraphicsItem* > linkitems;
        DANodeLinkPoint point;
    };

public:
    DAAbstractNodeGraphicsItemPrivate(DAAbstractNode* n, DAAbstractNodeGraphicsItem* p);
    //获取连接点
    QList< DANodeLinkPoint > getLinkPoints() const;
    DANodeLinkPoint getLinkPoints(const QString& n) const;
    DANodeLinkPoint getInputLinkPoint(const QString& name) const;
    DANodeLinkPoint getOutputLinkPoint(const QString& name) const;
    LinkInfo& addLinkPoint(const DANodeLinkPoint& lp);

public:
    DAAbstractNode::WeakPointer _node;
    QList< LinkInfo > _linkInfos;               ///< 这里记录所有的link
    std::unique_ptr< DANodePalette > _palette;  ///< 调色板
    DAAbstractNodeGraphicsItem::LinkPointShowType _linkPointShowType;
    bool _isShowLinkPoint;                                                  ///< 标记是否显示连接点
    std::unique_ptr< DANodeLinkPointDrawDelegate > _linkPointDrawDelegate;  ///< 连接点绘制代理
    DAAbstractNodeGraphicsItem::LinkPointLocation _inputLocation;           ///< 入口连接点的位置
    DAAbstractNodeGraphicsItem::LinkPointLocation _outputLocation;          ///< 出口连接点的位置
};

bool operator==(const DAAbstractNodeGraphicsItemPrivate::LinkInfo& a, const DAAbstractNodeGraphicsItemPrivate::LinkInfo& b);

bool operator==(const DAAbstractNodeGraphicsItemPrivate::LinkInfo& a, const DAAbstractNodeGraphicsItemPrivate::LinkInfo& b)
{
    return ((a.linkitems == b.linkitems) && (a.point == b.point));
}

DAAbstractNodeGraphicsItemPrivate::LinkInfo::LinkInfo()
{
}

DAAbstractNodeGraphicsItemPrivate::LinkInfo::LinkInfo(const DANodeLinkPoint& p) : point(p)
{
}

DAAbstractNodeGraphicsItemPrivate::DAAbstractNodeGraphicsItemPrivate(DAAbstractNode* n, DAAbstractNodeGraphicsItem* p)
    : q_ptr(p)
    , _node(n->pointer())
    , _linkPointShowType(DAAbstractNodeGraphicsItem::LinkPointAlwayShow)
    , _isShowLinkPoint(true)
    , _inputLocation(DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide)
    , _outputLocation(DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide)
{
    _linkPointDrawDelegate.reset(new DANodeLinkPointDrawDelegate(p));
}

QList< DANodeLinkPoint > DAAbstractNodeGraphicsItemPrivate::getLinkPoints() const
{
    QList< DANodeLinkPoint > res;
    for (const LinkInfo& d : qAsConst(_linkInfos)) {
        res.append(d.point);
    }
    return res;
}

DANodeLinkPoint DAAbstractNodeGraphicsItemPrivate::getLinkPoints(const QString& n) const
{
    for (const LinkInfo& d : qAsConst(_linkInfos)) {
        if (d.point.name == n) {
            return d.point;
        }
    }

    return DANodeLinkPoint();
}

DANodeLinkPoint DAAbstractNodeGraphicsItemPrivate::getInputLinkPoint(const QString& name) const
{
    for (const LinkInfo& d : qAsConst(_linkInfos)) {
        if (d.point.way == DANodeLinkPoint::Input && d.point.name == name) {
            return d.point;
        }
    }
    return DANodeLinkPoint();
}

DANodeLinkPoint DAAbstractNodeGraphicsItemPrivate::getOutputLinkPoint(const QString& name) const
{
    for (const LinkInfo& d : qAsConst(_linkInfos)) {
        if (d.point.way == DANodeLinkPoint::Output && d.point.name == name) {
            return d.point;
        }
    }
    return DANodeLinkPoint();
}

DAAbstractNodeGraphicsItemPrivate::LinkInfo& DAAbstractNodeGraphicsItemPrivate::addLinkPoint(const DANodeLinkPoint& lp)
{
    _linkInfos.append(LinkInfo(lp));
    return _linkInfos.back();
}

//===================================================
// DAAbstractNodeGraphicsItem
//===================================================

DAAbstractNodeGraphicsItem::DAAbstractNodeGraphicsItem(DAAbstractNode* n, QGraphicsItem* p)
    : DAGraphicsResizeableItem(p), d_ptr(new DAAbstractNodeGraphicsItemPrivate(n, this))
{
    if (n) {
        n->registItem(this);
    }
    resetLinkPoint();
}

DAAbstractNodeGraphicsItem::~DAAbstractNodeGraphicsItem()
{
    // item在删除时要通知相关的link把记录删除，否则会出现问题
    DAAbstractNode* n = rawNode();
    if (n) {
        n->unregistItem();
    }
    clearLinkData();
}

/**
 * @brief 获取item对应的node
 * @return
 */
DAAbstractNode* DAAbstractNodeGraphicsItem::rawNode()
{
    return (d_ptr->_node.lock().get());
}

const DAAbstractNode* DAAbstractNodeGraphicsItem::rawNode() const
{
    return (d_ptr->_node.lock().get());
}

DAAbstractNode::SharedPointer DAAbstractNodeGraphicsItem::node() const
{
    DAAbstractNode::SharedPointer p = d_ptr->_node.lock();
    return p;
}

/**
 * @brief 设置连接点的显示属性
 * @param t
 */
void DAAbstractNodeGraphicsItem::setLinkPointShowType(LinkPointShowType t)
{
    d_ptr->_linkPointShowType = t;
    switch (t) {
    case LinkPointAlwayShow:
        d_ptr->_isShowLinkPoint = true;
        break;
    case LinkPointShowOnHover:
        d_ptr->_isShowLinkPoint = isUnderMouse();
        break;
    case LinkPointShowOnSelect:
        d_ptr->_isShowLinkPoint = isSelected();
        break;
    default:
        break;
    }
}

DAAbstractNodeGraphicsItem::LinkPointShowType DAAbstractNodeGraphicsItem::getLinkPointShowType() const
{
    return d_ptr->_linkPointShowType;
}
/**
 * @brief 设置连接点的位置
 *
 * @note 设置连接点位置后，需要调用@ref updateLinkPointPos 才能生效
 * @sa DAAbstractNodeGraphicsItem::LinkPointLocation
 * @param l
 */
void DAAbstractNodeGraphicsItem::setLinkPointLocation(DANodeLinkPoint::Way way, DAAbstractNodeGraphicsItem::LinkPointLocation l)
{
    if (DANodeLinkPoint::Input == way) {
        d_ptr->_inputLocation = l;
    } else {
        d_ptr->_outputLocation = l;
    }
}
/**
 * @brief 获取连接点的位置
 * @return
 */
DAAbstractNodeGraphicsItem::LinkPointLocation DAAbstractNodeGraphicsItem::getLinkPointLocation(DANodeLinkPoint::Way way) const
{
    if (DANodeLinkPoint::Input == way) {
        return d_ptr->_inputLocation;
    }
    return d_ptr->_outputLocation;
}

QString DAAbstractNodeGraphicsItem::getNodeName() const
{
    if (const DAAbstractNode* n = rawNode()) {
        return (n->getNodeName());
    }
    return (QString());
}

QIcon DAAbstractNodeGraphicsItem::getIcon() const
{
    if (const DAAbstractNode* n = rawNode()) {
        return (n->getIcon());
    }
    return (QIcon());
}

/**
 * @brief 设置图标
 * @param icon
 */
void DAAbstractNodeGraphicsItem::setIcon(const QIcon& icon)
{
    if (DAAbstractNode* n = rawNode()) {
        n->setIcon(icon);
    }
}

/**
 * @brief 获取节点元数据
 * @return
 */
const DANodeMetaData& DAAbstractNodeGraphicsItem::metaData() const
{
    return (rawNode()->metaData());
}

/**
 * @brief 获取节点元数据
 * @return
 */
DANodeMetaData& DAAbstractNodeGraphicsItem::metaData()
{
    return (rawNode()->metaData());
}

/**
 * @brief 获取连接点群
 * @return
 */
QList< DANodeLinkPoint > DAAbstractNodeGraphicsItem::getLinkPoints() const
{
    return d_ptr->getLinkPoints();
}

/**
 * @brief 获取连接点
 * @param name
 * @return
 */
DANodeLinkPoint DAAbstractNodeGraphicsItem::getLinkPoint(const QString& name) const
{
    return (d_ptr->getLinkPoints(name));
}

DANodeLinkPoint DAAbstractNodeGraphicsItem::getInputLinkPoint(const QString& name) const
{
    return (d_ptr->getInputLinkPoint(name));
}

DANodeLinkPoint DAAbstractNodeGraphicsItem::getOutputLinkPoint(const QString& name) const
{
    return (d_ptr->getOutputLinkPoint(name));
}

/**
 * @brief 设置linkpoint的方向,linkpoint 方向设置只会影响显示，不会影响工作流的链接
 * @param name
 * @param d
 */
bool DAAbstractNodeGraphicsItem::setNodeLinkPointDirection(const QString& name, DANodeLinkPoint::Direction d)
{
    for (DAAbstractNodeGraphicsItemPrivate::LinkInfo& dp : d_ptr->_linkInfos) {
        if (dp.point.name == name) {
            dp.point.direction = d;
            return true;
        }
    }
    return false;
}

/**
 * @brief 判断是否存在连接点,仅判断way和name，不对绘图方面进行判断
 * @param pl
 * @return
 */
bool DAAbstractNodeGraphicsItem::isHaveLinkPoint(const DANodeLinkPoint& pl) const
{
    for (const DAAbstractNodeGraphicsItemPrivate::LinkInfo& ld : qAsConst(d_ptr->_linkInfos)) {
        if (ld.point.isEqualWayName(pl)) {
            return (true);
        }
    }
    return (false);
}

/**
 * @brief 绘制默认连接点
 *
 * 每个连接点的绘制调用@sa paintLinkPoint 函数,此函数不作为虚函数，因为连接点的绘制修改可通过设置@sa DANodeLinkPointDrawDelegate 来调整
 */
void DAAbstractNodeGraphicsItem::paintLinkPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (!d_ptr->_isShowLinkPoint) {
        return;
    }
    d_ptr->_linkPointDrawDelegate->paintLinkPoints(painter, option, widget);
}

/**
 * @brief 获取节点对应的设置窗口
 * @return
 */
DAAbstractNodeWidget* DAAbstractNodeGraphicsItem::getNodeWidget() const
{
    return nullptr;
}

/**
 * @brief 重置链接点，会把原来的信息清空，包括已经链接的item
 * @note 此函数的调用会清空已有的链接
 *
 */
void DAAbstractNodeGraphicsItem::resetLinkPoint()
{
    QList< DANodeLinkPoint > linkPoints = generateLinkPoint();
    clearLinkData();
    for (const DANodeLinkPoint& d : qAsConst(linkPoints)) {
        addLinkPoint(d);
    }
}

/**
 * @brief 设置调色板
 * @param pl
 */
void DAAbstractNodeGraphicsItem::setNodePalette(const DANodePalette& pl)
{
    if (!(d_ptr->_palette)) {
        d_ptr->_palette.reset(new DANodePalette);
    }
    *(d_ptr->_palette) = pl;
}

/**
 * @brief 获取调色板，如果没有设置内部调色板，会返回全局调色板
 * @return
 */
const DANodePalette& DAAbstractNodeGraphicsItem::getNodePalette() const
{
    if (d_ptr->_palette) {
        return *(d_ptr->_palette);
    }
    return DANodePalette::getGlobalPalette();
}

/**
 * @brief 设置连接点绘制的代理，如果不设置会有一个默认代理
 * @param delegate
 * @note 代理的内存将交由DAAbstractNodeGraphicsItem控制，用户不要在外部进行delete操作
 */
void DAAbstractNodeGraphicsItem::setLinkPointDrawDelegate(DANodeLinkPointDrawDelegate* delegate)
{
    if (delegate) {
        if (delegate->getItem() != this) {
            delegate->setItem(this);
        }
    }
    d_ptr->_linkPointDrawDelegate.reset(delegate);
}

/**
 * @brief 获取连接点绘图代理
 * @return
 */
DANodeLinkPointDrawDelegate* DAAbstractNodeGraphicsItem::getLinkPointDrawDelegate() const
{
    return d_ptr->_linkPointDrawDelegate.get();
}

/**
 * @brief 此函数是在准备调用getLinkPointByPos之前调用的函数，用来准备输入节点
 *
 * 此函数对于固定节点作用不大，对于非固定节点，或者动态生成节点的情况，可以通过此函数用来实时生成节点
 * @param p
 * @param linkItem 将要尝试链接此节点input的linkitem
 * @note prepareXXX相关的函数，仅仅会在scene操作item时触发
 */
void DAAbstractNodeGraphicsItem::prepareLinkInput(const QPointF& p, DAAbstractNodeLinkGraphicsItem* linkItem)
{
    Q_UNUSED(p);
    Q_UNUSED(linkItem);
}
/**
 * @brief 此函数是在尝试链接失败之后调用的函数
 *
 * 此函数对于固定节点作用不大，对于非固定节点，或者动态生成节点的情况，可以通过此函数用来处理链接失败之后的情况
 * @param linkItem 将要尝试链接此节点input的linkitem
 * @note prepareXXX相关的函数，仅仅会在scene操作item时触发
 */
void DAAbstractNodeGraphicsItem::prepareLinkInputFailed(const DANodeLinkPoint& p, DAAbstractNodeLinkGraphicsItem* linkItem)
{
    Q_UNUSED(p);
    Q_UNUSED(linkItem);
}
/**
 * @brief 此函数是在尝试链接成功之后调用的函数
 *
 * 此函数对于固定节点作用不大，对于非固定节点，或者动态生成节点的情况，可以通过此函数用来处理链接成功之后的情况
 * @param linkItem 将要尝试链接此节点input的linkitem
 * @note prepareXXX相关的函数，仅仅会在scene操作item时触发
 */
void DAAbstractNodeGraphicsItem::prepareLinkInputSucceed(const DANodeLinkPoint& p, DAAbstractNodeLinkGraphicsItem* linkItem)
{
    Q_UNUSED(p);
    Q_UNUSED(linkItem);
}

/**
 * @brief 此函数是在准备调用getLinkPointByPos之前调用的函数，用来准备输出节点
 *
 * 此函数对于固定节点作用不大，对于非固定节点，或者动态生成节点的情况，可以通过此函数用来实时生成节点
 * @param p
 * @note prepareXXX相关的函数，仅仅会在scene操作item时触发
 */
void DAAbstractNodeGraphicsItem::prepareLinkOutput(const QPointF& p)
{
    Q_UNUSED(p);
}
/**
 * @brief 此函数是在尝试链接失败之后调用的函数
 *
 * 此函数对于固定节点作用不大，对于非固定节点，或者动态生成节点的情况，可以通过此函数用来处理链接失败之后的情况
 * @note prepareXXX相关的函数，仅仅会在scene操作item时触发
 */
void DAAbstractNodeGraphicsItem::prepareLinkOutputFailed(const DANodeLinkPoint& p)
{
    Q_UNUSED(p);
}
/**
 * @brief 此函数是在尝试链接成功之后调用的函数
 *
 * 此函数对于固定节点作用不大，对于非固定节点，或者动态生成节点的情况，可以通过此函数用来处理链接成功之后的情况
 * @note prepareXXX相关的函数，仅仅会在scene操作item时触发
 */
void DAAbstractNodeGraphicsItem::prepareLinkOutputSucceed(const DANodeLinkPoint& p)
{
    Q_UNUSED(p);
}
/**
 * @brief 节点名字改变准备函数，通过此函数，让节点对名字进行重新绘制
 * @param name
 * @note 此函数仅仅会在节点名字变化的时候才会触发
 */
void DAAbstractNodeGraphicsItem::prepareNodeNameChanged(const QString& name)
{
    Q_UNUSED(name);
}

/**
 * @brief 通过位置获取linkpoint,如果没有返回一个invalid的DANodeLinkPoint
 *
 * 此函数作为虚函数，是scene判断是否点击到了链接点的关键函数，如果连接点是固定的,
 * 是不需要继承此函数的，但对于一些特殊的连接点（如不固定的）就需要通过此函数来获取
 *
 * 此函数默认是调用@sa DANodeLinkPointDrawDelegate 的 @sa DANodeLinkPointDrawDelegate::getlinkPointRect
 * @param p
 * @return 如果返回一个默认构造的DANodeLinkPoint，说明没在连接点上
 *
 */
DANodeLinkPoint DAAbstractNodeGraphicsItem::getLinkPointByPos(const QPointF& p) const
{
    QList< DANodeLinkPoint > lps = getLinkPoints();
    for (const DANodeLinkPoint& lp : qAsConst(lps)) {
        if (d_ptr->_linkPointDrawDelegate->getlinkPointPainterRect(lp).contains(p)) {
            return lp;
        }
    }
    return DANodeLinkPoint();
}

/**
 * @brief 处理一些联动事件，如和FCAbstractNodeLinkGraphicsItem的联动
 * @param change
 * @param value
 * @return
 */
QVariant DAAbstractNodeGraphicsItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case ItemPositionHasChanged: {
        if (scene()) {
            // 变化了位置,需要更新link item
            // 此函数能保证item在移动时连接线跟随动
            for (const DAAbstractNodeGraphicsItemPrivate::LinkInfo& ld : qAsConst(d_ptr->_linkInfos)) {
                for (DAAbstractNodeLinkGraphicsItem* li : ld.linkitems) {
                    li->updatePos();
                }
            }
        }
    } break;
    default:
        break;
    }
    return (DAGraphicsResizeableItem::itemChange(change, value));
}

/**
 * @brief 鼠标按下
 * @param event
 */
void DAAbstractNodeGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    DAGraphicsResizeableItem::mousePressEvent(event);
    if (isResizing()) {
        return;
    }
    if (LinkPointShowOnSelect == d_ptr->_linkPointShowType) {
        d_ptr->_isShowLinkPoint = isSelected();
    }
}

void DAAbstractNodeGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    DAGraphicsResizeableItem::mouseReleaseEvent(event);
    if (isResizing()) {
        return;
    }
    if (LinkPointShowOnSelect == d_ptr->_linkPointShowType) {
        d_ptr->_isShowLinkPoint = isSelected();
    }
}

void DAAbstractNodeGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    if (getLinkPointShowType() == LinkPointShowOnHover) {
        if (!d_ptr->_isShowLinkPoint) {
            d_ptr->_isShowLinkPoint = true;
            //刷新
            update();
        }
    }
    DAGraphicsResizeableItem::hoverEnterEvent(event);
}

void DAAbstractNodeGraphicsItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    DAGraphicsResizeableItem::hoverMoveEvent(event);
}

void DAAbstractNodeGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    if (getLinkPointShowType() == LinkPointShowOnHover) {
        d_ptr->_isShowLinkPoint = false;
        //刷新
        update();
    }
    DAGraphicsResizeableItem::hoverLeaveEvent(event);
}

/**
 * @brief
 * 此函数用于FCAbstractNodeLinkGraphicsItem在调用attachedTo/From过程中调用
 * @param item
 * @param pl
 * @return 如果返回false，说明记录不成功，已经有相同的连接了
 */
bool DAAbstractNodeGraphicsItem::linked(DAAbstractNodeLinkGraphicsItem* link, const DANodeLinkPoint& pl)
{
    for (DAAbstractNodeGraphicsItemPrivate::LinkInfo& ld : d_ptr->_linkInfos) {
        if (ld.point.isEqualWayName(pl)) {
            ld.linkitems.append(link);
            return (true);
        }
    }
    //下面永远不会达到
    //没有找到，就查看node有没有
    if (rawNode()->getInputKeys().contains(pl.name) && pl.way == DANodeLinkPoint::Input) {
        //存在input
        //说明没有插入这个点
        DAAbstractNodeGraphicsItemPrivate::LinkInfo& li = d_ptr->addLinkPoint(pl);
        li.linkitems.append(link);
        return true;
    }
    if (rawNode()->getOutputKeys().contains(pl.name) && pl.way == DANodeLinkPoint::Output) {
        //存在output
        //说明没有插入这个点
        DAAbstractNodeGraphicsItemPrivate::LinkInfo& li = d_ptr->addLinkPoint(pl);
        li.linkitems.append(link);
        return true;
    }
    return false;
}

/**
 * @brief 连接的link在销毁时调用，把item记录的link信息消除
 * @param item
 * @param pl
 * @return
 */
bool DAAbstractNodeGraphicsItem::linkItemRemoved(DAAbstractNodeLinkGraphicsItem* link, const DANodeLinkPoint& pl)
{
    for (DAAbstractNodeGraphicsItemPrivate::LinkInfo& ld : d_ptr->_linkInfos) {
        if (ld.point.isEqualWayName(pl)) {
            return ld.linkitems.removeAll(link) > 0;
        }
    }
    return false;
}

/**
 * @brief 此函数根据DAAbstractNode的输入输出来生成默认的连接点
 *
 * 会调用@sa FCAbstractNode::inputKeys 获取所有的输入参数,
 * 调用@sa FCAbstractNode::outputKeys 判断是否生成输出节点
 *
 * 默认所有输入位于左边，所有输出位于右边
 * 此函数会调用@sa updateLinkPointPos 来更新点位置
 * @return
 */
QList< DANodeLinkPoint > DAAbstractNodeGraphicsItem::generateLinkPoint() const
{
    QList< DANodeLinkPoint > res;
    const DAAbstractNode* n = rawNode();
    if (nullptr == n) {
        return res;
    }
    //生成输入点
    QList< QString > ks = n->getInputKeys();
    //避免除0
    for (const QString& k : qAsConst(ks)) {
        DANodeLinkPoint lp;
        lp.direction = DANodeLinkPoint::West;
        lp.way       = DANodeLinkPoint::Input;
        lp.name      = k;
        res.append(lp);
    }

    //生成输出点
    ks = n->getOutputKeys();
    for (const QString& k : qAsConst(ks)) {
        DANodeLinkPoint lp;
        lp.direction = DANodeLinkPoint::East;
        lp.way       = DANodeLinkPoint::Output;
        lp.name      = k;
        res.append(lp);
    }
    changeLinkPointPos(res, getBodyRect());
    return (res);
}

/**
 * @brief 更新连接点
 *
 * 传入已有的连接点和总体尺寸，通过此函数的重写可以改变连接点的位置，
 * 如果想改变连接点的绘制应该通过setLinkPointDrawDelegate实现
 * @param lp
 * @param bodyRect
 */
void DAAbstractNodeGraphicsItem::changeLinkPointPos(QList< DANodeLinkPoint >& lps, const QRectF& bodyRect) const
{
    int inputCnt  = 0;
    int outputCnt = 0;
    for (const DANodeLinkPoint& lp : lps) {
        if (lp.way == DANodeLinkPoint::Output) {
            ++outputCnt;
        } else {
            ++inputCnt;
        }
    }
    //离开边界的距离
    const qreal spaceSide = 2;
    //获取出入口位置
    const LinkPointLocation inLoc  = getLinkPointLocation(DANodeLinkPoint::Input);
    const LinkPointLocation outLoc = getLinkPointLocation(DANodeLinkPoint::Output);
    //计算出入口的每次偏移量
    qreal dtIn  = 0;
    qreal dtOut = 0;
    switch (inLoc) {
    case LinkPointLocationOnLeftSide:
    case LinkPointLocationOnRightSide:
        dtIn = (bodyRect.height() - 2 * spaceSide) / (inputCnt + 1.0);
        break;
    case LinkPointLocationOnTopSide:
    case LinkPointLocationOnBottomSide:
        dtIn = (bodyRect.width() - 2 * spaceSide) / (inputCnt + 1.0);
    default:
        break;
    }
    switch (outLoc) {
    case LinkPointLocationOnLeftSide:
    case LinkPointLocationOnRightSide:
        dtOut = (bodyRect.height() - 2 * spaceSide) / (outputCnt + 1.0);
        break;
    case LinkPointLocationOnTopSide:
    case LinkPointLocationOnBottomSide:
        dtOut = (bodyRect.width() - 2 * spaceSide) / (outputCnt + 1.0);
    default:
        break;
    }
    //计数清零
    inputCnt  = 0;
    outputCnt = 0;
    for (DANodeLinkPoint& lp : lps) {
        if (lp.way == DANodeLinkPoint::Input) {
            ++inputCnt;
            switch (inLoc) {
            case LinkPointLocationOnLeftSide:
                lp.direction = DANodeLinkPoint::West;
                lp.position  = QPointF(bodyRect.left() + spaceSide, bodyRect.top() + spaceSide + (inputCnt * dtIn));
                break;
            case LinkPointLocationOnTopSide:
                lp.direction = DANodeLinkPoint::North;
                lp.position  = QPointF(bodyRect.left() + spaceSide + (inputCnt * dtIn), bodyRect.top() + spaceSide);
                break;
            case LinkPointLocationOnRightSide:
                lp.direction = DANodeLinkPoint::East;
                lp.position  = QPointF(bodyRect.right() - spaceSide, bodyRect.top() + spaceSide + (inputCnt * dtIn));
                break;
            case LinkPointLocationOnBottomSide:
                lp.direction = DANodeLinkPoint::South;
                lp.position  = QPointF(bodyRect.left() + spaceSide + (inputCnt * dtIn), bodyRect.bottom() - spaceSide);
                break;
            default:
                break;
            }
        } else {
            //出口
            ++outputCnt;
            switch (outLoc) {
            case LinkPointLocationOnLeftSide:
                lp.direction = DANodeLinkPoint::West;
                lp.position  = QPointF(bodyRect.left() + spaceSide, bodyRect.top() + spaceSide + (outputCnt * dtOut));
                break;
            case LinkPointLocationOnTopSide:
                lp.direction = DANodeLinkPoint::North;
                lp.position  = QPointF(bodyRect.left() + spaceSide + (outputCnt * dtOut), bodyRect.top() + spaceSide);
                break;
            case LinkPointLocationOnRightSide:
                lp.direction = DANodeLinkPoint::East;
                lp.position  = QPointF(bodyRect.right() - spaceSide, bodyRect.top() + spaceSide + (outputCnt * dtOut));
                break;
            case LinkPointLocationOnBottomSide:
                lp.direction = DANodeLinkPoint::South;
                lp.position = QPointF(bodyRect.left() + spaceSide + (outputCnt * dtOut), bodyRect.bottom() - spaceSide);
                break;
            default:
                break;
            }
        }
    }
}

/**
 * @brief 加入新的连接点
 * @param lp
 * @note 某些场合，加入新的连接点需要刷新一下连接点位置 @sa updateLinkPointPos
 */
void DAAbstractNodeGraphicsItem::addLinkPoint(const DANodeLinkPoint& lp)
{
    d_ptr->addLinkPoint(lp);
}

bool DAAbstractNodeGraphicsItem::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    DAGraphicsResizeableItem::saveToXml(doc, parentElement);
    QDomElement nodeEle = doc->createElement("nodeItem");
    QDomElement lpEle   = doc->createElement("linkpoints");
    lpEle.setAttribute("input-loc", enumToString(getLinkPointLocation(DANodeLinkPoint::Input)));
    lpEle.setAttribute("output-loc", enumToString(getLinkPointLocation(DANodeLinkPoint::Output)));
    QList< DANodeLinkPoint > lps = getLinkPoints();
    for (const DANodeLinkPoint& p : qAsConst(lps)) {
        QDomElement pEle = doc->createElement("lp");
        pEle.setAttribute("name", p.name);
        pEle.setAttribute("direction", enumToString(p.direction));
        lpEle.appendChild(pEle);
    }
    nodeEle.appendChild(lpEle);
    parentElement->appendChild(nodeEle);
    return true;
}

bool DAAbstractNodeGraphicsItem::loadFromXml(const QDomElement* itemElement)
{
    if (!DAGraphicsResizeableItem::loadFromXml(itemElement)) {
        return false;
    }
    QDomElement nodeEle = itemElement->firstChildElement("nodeItem");
    if (nodeEle.isNull()) {
        return false;
    }
    QDomElement lpEle = nodeEle.firstChildElement("linkpoints");
    if (lpEle.isNull()) {
        return false;
    }
    setLinkPointLocation(DANodeLinkPoint::Input, stringToEnum(lpEle.attribute("input-loc"), LinkPointLocationOnLeftSide));
    setLinkPointLocation(DANodeLinkPoint::Output, stringToEnum(lpEle.attribute("output-loc"), LinkPointLocationOnRightSide));
    updateLinkPointPos();
    //获取具体连接点
    QDomNodeList childs = lpEle.childNodes();
    for (int i = 0; i < childs.size(); ++i) {
        QDomElement le = childs.at(i).toElement();
        if (le.isNull()) {
            continue;
        }
        if (le.tagName() == "lp") {
            QString name = le.attribute("name");
            if (name.isEmpty()) {
                continue;
            }
            setNodeLinkPointDirection(name, stringToEnum(le.attribute("direction"), DANodeLinkPoint::East));
        }
    }
    return true;
}

/**
 * @brief 清除链接信息
 */
void DAAbstractNodeGraphicsItem::clearLinkData()
{
    for (DAAbstractNodeGraphicsItemPrivate::LinkInfo& d : d_ptr->_linkInfos) {
        for (DAAbstractNodeLinkGraphicsItem* li : qAsConst(d.linkitems)) {
            li->callItemIsDestroying(this, d.point);
        }
    }
    d_ptr->_linkInfos.clear();
}

/**
 * @brief 更新连接点位置，在bodyBoundingRect改变之后调用
 * @param bodyRect
 */
void DAAbstractNodeGraphicsItem::updateLinkPointPos()
{
    QList< DANodeLinkPoint > lps = d_ptr->getLinkPoints();
    changeLinkPointPos(lps, getBodyRect());
    //更新到linkData
    int s = qMin(lps.size(), d_ptr->_linkInfos.size());
    for (int i = 0; i < s; ++i) {
        d_ptr->_linkInfos[ i ].point = lps[ i ];
        for (DAAbstractNodeLinkGraphicsItem* item : qAsConst(d_ptr->_linkInfos[ i ].linkitems)) {
            if (lps[ i ].way == DANodeLinkPoint::Input) {
                item->updateToLinkPointInfo(lps[ i ]);
            } else {
                item->updateFromLinkPointInfo(lps[ i ]);
            }
        }
    }
}

/**
 * @brief 获取当前链接上的LinkGraphicsItem
 * @return
 */
QList< DAAbstractNodeLinkGraphicsItem* > DAAbstractNodeGraphicsItem::getLinkItems() const
{
    QSet< DAAbstractNodeLinkGraphicsItem* > res;
    for (DAAbstractNodeGraphicsItemPrivate::LinkInfo& d : d_ptr->_linkInfos) {
        for (DAAbstractNodeLinkGraphicsItem* li : qAsConst(d.linkitems)) {
            res.insert(li);
        }
    }
#if QT_VERSION_MAJOR >= 6
    return QList< DAAbstractNodeLinkGraphicsItem* >(res.begin(), res.end());
#else
    return res.toList();
#endif
}

QList< DAAbstractNodeLinkGraphicsItem* > DAAbstractNodeGraphicsItem::getLinkItem(const QString& name) const
{
    for (DAAbstractNodeGraphicsItemPrivate::LinkInfo& d : d_ptr->_linkInfos) {
        if (d.point.name == name) {
            return d.linkitems;
        }
    }
    return QList< DAAbstractNodeLinkGraphicsItem* >();
}

QString enumToString(DAAbstractNodeGraphicsItem::LinkPointLocation e)
{
    switch (e) {
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide:
        return "left-side";
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide:
        return "top-side";
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide:
        return "right-side";
    case DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide:
        return "bottom-side";
    default:
        break;
    }
    return "left-side";
}

DAAbstractNodeGraphicsItem::LinkPointLocation stringToEnum(const QString& s, DAAbstractNodeGraphicsItem::LinkPointLocation defaultEnum)
{
    if (0 == s.compare("left-side", Qt::CaseInsensitive)) {
        return DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide;
    } else if (0 == s.compare("top-side", Qt::CaseInsensitive)) {
        return DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide;
    } else if (0 == s.compare("right-side", Qt::CaseInsensitive)) {
        return DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide;
    } else if (0 == s.compare("bottom-side", Qt::CaseInsensitive)) {
        return DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide;
    }
    return defaultEnum;
}
}  // end namespace DA
