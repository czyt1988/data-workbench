#include "DAAbstractNodeGraphicsItem.h"
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QDomDocument>
#include <QDomElement>
#include <memory>
#include "DAQtContainerUtil.hpp"
#include "DAAbstractNode.h"
#include "DAAbstractNodeLinkGraphicsItem.h"
#include "DAAbstractNodeWidget.h"
#include "DANodeGraphicsScene.h"
#include "DANodePalette.h"
#include "DANodeLinkPointDrawDelegate.h"
#include "DAStandardNodeLinkGraphicsItem.h"

/**
 * @def 调试打印
 */
#define DA_DAABSTRACTNODEGRAPHICSITEM_DEBUG_PRINT 0
////////////////////////////////////////////////////////////////////////
namespace DA
{
class DAAbstractNodeGraphicsItem::PrivateData
{
	DA_DECLARE_PUBLIC(DAAbstractNodeGraphicsItem)
public:
	class LinkInfo
	{
	public:
		LinkInfo();
		bool operator==(const LinkInfo& a) const;
		LinkInfo(const DANodeLinkPoint& p);
		QList< DAAbstractNodeLinkGraphicsItem* > linkitems;
		DANodeLinkPoint point;
	};

public:
	PrivateData(DAAbstractNode* n, DAAbstractNodeGraphicsItem* p);
	// 获取连接点
	QList< DANodeLinkPoint > getLinkPoints() const;
	QList< DANodeLinkPoint > getOutputLinkPoints() const;
	QList< DANodeLinkPoint > getInputLinkPoints() const;
	DANodeLinkPoint getLinkPoints(const QString& n) const;
	DANodeLinkPoint getInputLinkPoint(const QString& name) const;
	DANodeLinkPoint getOutputLinkPoint(const QString& name) const;
	LinkInfo& addLinkPoint(const DANodeLinkPoint& lp);

public:
	DAAbstractNode::WeakPointer mNode;
	QList< LinkInfo > mLinkInfos;               ///< 这里记录所有的link
	std::unique_ptr< DANodePalette > mPalette;  ///< 调色板
	DAAbstractNodeGraphicsItem::LinkPointShowType mLinkPointShowType;
	bool mIsShowLinkPoint;                                                  ///< 标记是否显示连接点
	std::unique_ptr< DANodeLinkPointDrawDelegate > mLinkPointDrawDelegate;  ///< 连接点绘制代理
	DAAbstractNodeGraphicsItem::LinkPointLocation mInputLocation;           ///< 入口连接点的位置
	DAAbstractNodeGraphicsItem::LinkPointLocation mOutputLocation;          ///< 出口连接点的位置
};

// bool operator==(const DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& a,
//                const DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& b);

// bool operator==(const DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& a, const DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& b)
//{
//    return ((a.linkitems == b.linkitems) && (a.point == b.point));
//}

DAAbstractNodeGraphicsItem::PrivateData::LinkInfo::LinkInfo()
{
}

bool DAAbstractNodeGraphicsItem::PrivateData::LinkInfo::operator==(const DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& a) const
{
    return ((a.linkitems == this->linkitems) && (a.point == this->point));
}

DAAbstractNodeGraphicsItem::PrivateData::LinkInfo::LinkInfo(const DANodeLinkPoint& p) : point(p)
{
}

DAAbstractNodeGraphicsItem::PrivateData::PrivateData(DAAbstractNode* n, DAAbstractNodeGraphicsItem* p)
    : q_ptr(p)
    , mNode(n->pointer())
    , mLinkPointShowType(DAAbstractNodeGraphicsItem::LinkPointAlwayShow)
    , mIsShowLinkPoint(true)
    , mInputLocation(DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide)
    , mOutputLocation(DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide)
{
    mLinkPointDrawDelegate.reset(new DANodeLinkPointDrawDelegate(p));
}

QList< DANodeLinkPoint > DAAbstractNodeGraphicsItem::PrivateData::getLinkPoints() const
{
	QList< DANodeLinkPoint > res;
	for (const LinkInfo& d : qAsConst(mLinkInfos)) {
		res.append(d.point);
	}
	return res;
}

QList< DANodeLinkPoint > DAAbstractNodeGraphicsItem::PrivateData::getOutputLinkPoints() const
{
	QList< DANodeLinkPoint > res;
	for (const LinkInfo& d : qAsConst(mLinkInfos)) {
		if (d.point.way == DANodeLinkPoint::Output) {
			res.append(d.point);
		}
	}
	return res;
}

QList< DANodeLinkPoint > DAAbstractNodeGraphicsItem::PrivateData::getInputLinkPoints() const
{
	QList< DANodeLinkPoint > res;
	for (const LinkInfo& d : qAsConst(mLinkInfos)) {
		if (d.point.way == DANodeLinkPoint::Input) {
			res.append(d.point);
		}
	}
	return res;
}

DANodeLinkPoint DAAbstractNodeGraphicsItem::PrivateData::getLinkPoints(const QString& n) const
{
	for (const LinkInfo& d : qAsConst(mLinkInfos)) {
		if (d.point.name == n) {
			return d.point;
		}
	}

	return DANodeLinkPoint();
}

DANodeLinkPoint DAAbstractNodeGraphicsItem::PrivateData::getInputLinkPoint(const QString& name) const
{
	for (const LinkInfo& d : qAsConst(mLinkInfos)) {
		if (d.point.way == DANodeLinkPoint::Input && d.point.name == name) {
			return d.point;
		}
	}
	return DANodeLinkPoint();
}

DANodeLinkPoint DAAbstractNodeGraphicsItem::PrivateData::getOutputLinkPoint(const QString& name) const
{
	for (const LinkInfo& d : qAsConst(mLinkInfos)) {
		if (d.point.way == DANodeLinkPoint::Output && d.point.name == name) {
			return d.point;
		}
	}
	return DANodeLinkPoint();
}

DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& DAAbstractNodeGraphicsItem::PrivateData::addLinkPoint(const DANodeLinkPoint& lp)
{
	mLinkInfos.append(LinkInfo(lp));
	return mLinkInfos.back();
}

//===================================================
// DAAbstractNodeGraphicsItem
//===================================================

DAAbstractNodeGraphicsItem::DAAbstractNodeGraphicsItem(DAAbstractNode* n, QGraphicsItem* p)
    : DAGraphicsResizeableItem(p), d_ptr(std::make_unique< DAAbstractNodeGraphicsItem::PrivateData >(n, this))
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
    return (d_ptr->mNode.lock().get());
}

const DAAbstractNode* DAAbstractNodeGraphicsItem::rawNode() const
{
    return (d_ptr->mNode.lock().get());
}

DAAbstractNode::SharedPointer DAAbstractNodeGraphicsItem::node() const
{
	DAAbstractNode::SharedPointer p = d_ptr->mNode.lock();
	return p;
}

/**
 * @brief 设置连接点的显示属性
 * @param t
 */
void DAAbstractNodeGraphicsItem::setLinkPointShowType(LinkPointShowType t)
{
	d_ptr->mLinkPointShowType = t;
	switch (t) {
	case LinkPointAlwayShow:
		d_ptr->mIsShowLinkPoint = true;
		break;
	case LinkPointShowOnHover:
		d_ptr->mIsShowLinkPoint = isUnderMouse();
		break;
	case LinkPointShowOnSelect:
		d_ptr->mIsShowLinkPoint = isSelected();
		break;
	default:
		break;
	}
}

DAAbstractNodeGraphicsItem::LinkPointShowType DAAbstractNodeGraphicsItem::getLinkPointShowType() const
{
    return d_ptr->mLinkPointShowType;
}
/**
 * @brief 设置连接点的位置
 *
 * @note 设置连接点位置后，需要调用@ref updateLinkPointPos 才能生效
 * @sa DAAbstractNodeGraphicsItem::LinkPointLocation
 * @param l
 */
void DAAbstractNodeGraphicsItem::setLinkPointLocation(DANodeLinkPoint::Way way,
                                                      DAAbstractNodeGraphicsItem::LinkPointLocation l)
{
	if (DANodeLinkPoint::Input == way) {
		d_ptr->mInputLocation = l;
	} else {
		d_ptr->mOutputLocation = l;
	}
}
/**
 * @brief 获取连接点的位置
 * @return
 */
DAAbstractNodeGraphicsItem::LinkPointLocation DAAbstractNodeGraphicsItem::getLinkPointLocation(DANodeLinkPoint::Way way) const
{
	if (DANodeLinkPoint::Input == way) {
		return d_ptr->mInputLocation;
	}
	return d_ptr->mOutputLocation;
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
 * @brief 获取所有输出的连接点群
 * @return
 */
QList< DANodeLinkPoint > DAAbstractNodeGraphicsItem::getOutputLinkPoints() const
{
    return d_ptr->getOutputLinkPoints();
}

/**
 * @brief 获取所有输入的连接点群
 * @return
 */
QList< DANodeLinkPoint > DAAbstractNodeGraphicsItem::getInputLinkPoints() const
{
    return d_ptr->getInputLinkPoints();
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
bool DAAbstractNodeGraphicsItem::setLinkPointDirection(const QString& name, AspectDirection d)
{
	for (DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& dp : d_ptr->mLinkInfos) {
		if (dp.point.name == name) {
            auto oldlp         = dp.point;
			dp.point.direction = d;
            // 对应的link也要跟着变换
            //  已经连接到的link信息也要变更
            for (auto link : qAsConst(dp.linkitems)) {
                if (oldlp == link->fromNodeLinkPoint()) {
                    link->updateFromLinkPointInfo(dp.point);
                } else if (oldlp == link->toNodeLinkPoint()) {
                    link->updateToLinkPointInfo(dp.point);
                }
            }
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
	for (const DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& ld : qAsConst(d_ptr->mLinkInfos)) {
		if (ld.point.isEqualWayName(pl)) {
			return (true);
		}
	}
	return (false);
}

/**
 * @brief 判断连接点是否已经链接
 * @param pl
 * @return
 */
bool DAAbstractNodeGraphicsItem::isLinkPointLinked(const DANodeLinkPoint& pl)
{
	for (const DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& ld : qAsConst(d_ptr->mLinkInfos)) {
		if (ld.point.isEqualWayName(pl)) {
			return !(ld.linkitems.isEmpty());
		}
	}
	return false;
}

/**
 * @brief 绘制默认连接点
 *
 * 在paintbody中调用此函数，用于绘制连接点
 *
 * 此函数会调用代理的paintLinkPoints函数
 */
void DAAbstractNodeGraphicsItem::paintLinkPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if (!d_ptr->mIsShowLinkPoint) {
		return;
	}
	d_ptr->mLinkPointDrawDelegate->paintLinkPoints(painter, option, widget);
}

/**
 * @brief 获取节点对应的设置窗口
 * @return
 */
DAAbstractNodeWidget* DAAbstractNodeGraphicsItem::getNodeWidget()
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
	if (!(d_ptr->mPalette)) {
		d_ptr->mPalette.reset(new DANodePalette);
	}
	*(d_ptr->mPalette) = pl;
}

/**
 * @brief 获取调色板，如果没有设置内部调色板，会返回全局调色板
 * @return
 */
const DANodePalette& DAAbstractNodeGraphicsItem::getNodePalette() const
{
	if (d_ptr->mPalette) {
		return *(d_ptr->mPalette);
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
	d_ptr->mLinkPointDrawDelegate.reset(delegate);
}

/**
 * @brief 获取连接点绘图代理
 * @return
 */
DANodeLinkPointDrawDelegate* DAAbstractNodeGraphicsItem::getLinkPointDrawDelegate() const
{
    return d_ptr->mLinkPointDrawDelegate.get();
}

/**
   @brief 此函数是在连接线点击item时调用的回调，无论点击的位置是否存在连接点，都会调用此回调

   通过此函数可以实现动态增加连接点功能，例如一个item的连接点是随着点击而根据某个逻辑判断是否需要存在的，可以重装此函数，在点击时进行判断
   @param p
   @param linkItem 注意，way=Output，linkItem为nullptr，这时连接线还没创建
   @param way 判断是输入还是输出，如果way=Input，就是输入过程的点击（被连接），way=Output，就是输出过程的点击（链接出去）
 */
void DA::DAAbstractNodeGraphicsItem::tryLinkOnItemPos(const QPointF& p,
                                                      DA::DAAbstractNodeLinkGraphicsItem* linkItem,
                                                      DA::DANodeLinkPoint::Way way)
{
	Q_UNUSED(p);
	Q_UNUSED(linkItem);
	Q_UNUSED(way);
}

/**
   @brief 链接结束的回调，无论有没有成功都会调用此回调函数
   @param p
   @param linkItem
   @param way 判断是输入还是输出，如果way=Input，就是输入过程的点击（被连接），way=Output，就是输出过程的点击（链接出去）
   @param isSuccess 链接成功或失败
 */
void DAAbstractNodeGraphicsItem::finishLink(const DANodeLinkPoint& p,
                                            DAAbstractNodeLinkGraphicsItem* linkItem,
                                            DANodeLinkPoint::Way way,
                                            bool isSuccess)
{
	Q_UNUSED(p);
	Q_UNUSED(linkItem);
	Q_UNUSED(way);
	Q_UNUSED(isSuccess);
}

void DAAbstractNodeGraphicsItem::detachLink(const DANodeLinkPoint& p,
                                            DAAbstractNodeLinkGraphicsItem* linkItem,
                                            DANodeLinkPoint::Way way)
{
	Q_UNUSED(p);
	Q_UNUSED(linkItem);
	Q_UNUSED(way);
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
   @brief 分组位置发生了变化

   分组位置发生了变化也要刷新链接
   @param pos
 */
void DAAbstractNodeGraphicsItem::groupPositionChanged(const QPointF& pos)
{
    updateLinkItems();
}

/**
 * @brief 通过位置获取linkpoint,如果没有返回一个invalid的DANodeLinkPoint
 *
 * 此函数作为虚函数，是scene判断是否点击到了链接点的关键函数，如果连接点是固定的,
 * 是不需要继承此函数的，但对于一些特殊的连接点（如不固定的）就需要通过此函数来获取
 *
 * 此函数默认是调用@sa DANodeLinkPointDrawDelegate 的 @sa DANodeLinkPointDrawDelegate::getlinkPointRect
 *
 * @param p
 * @return 如果返回一个默认构造的DANodeLinkPoint，说明没在连接点上
 *
 */
DANodeLinkPoint DAAbstractNodeGraphicsItem::getLinkPointByPos(const QPointF& p, DANodeLinkPoint::Way way) const
{
	QList< DANodeLinkPoint > lps;
	if (DANodeLinkPoint::Output == way) {
		lps = getOutputLinkPoints();
	} else {
		lps = getInputLinkPoints();
	}
	for (const DANodeLinkPoint& lp : qAsConst(lps)) {
		if (d_ptr->mLinkPointDrawDelegate->getlinkPointPainterRegion(lp).contains(p)) {
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
	QVariant r = DAGraphicsResizeableItem::itemChange(change, value);
	switch (change) {
	case ItemPositionHasChanged: {
		if (scene()) {
			// 变化了位置,需要更新link item
			// 此函数能保证item在移动时连接线跟随动
			updateLinkItems();
		}
	} break;
	default:
		break;
	}
	return r;
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
	if (LinkPointShowOnSelect == d_ptr->mLinkPointShowType) {
		d_ptr->mIsShowLinkPoint = isSelected();
	}
}

void DAAbstractNodeGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	DAGraphicsResizeableItem::mouseReleaseEvent(event);
	if (isResizing()) {
		return;
	}
	if (LinkPointShowOnSelect == d_ptr->mLinkPointShowType) {
		d_ptr->mIsShowLinkPoint = isSelected();
	}
}

void DAAbstractNodeGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	if (getLinkPointShowType() == LinkPointShowOnHover) {
		if (!d_ptr->mIsShowLinkPoint) {
			d_ptr->mIsShowLinkPoint = true;
			// 刷新
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
		d_ptr->mIsShowLinkPoint = false;
		// 刷新
		update();
	}
	DAGraphicsResizeableItem::hoverLeaveEvent(event);
}

/**
 * @brief
 * 此函数在DAAbstractNodeLinkGraphicsItem的attachedTo/From过程中调用
 * @param item
 * @param pl
 * @return 如果返回false，说明记录不成功，已经有相同的连接了
 */
bool DAAbstractNodeGraphicsItem::recordLinkInfo(DAAbstractNodeLinkGraphicsItem* link, const DANodeLinkPoint& pl)
{
	for (DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& ld : d_ptr->mLinkInfos) {
		if (ld.point.isEqualWayName(pl)) {
			ld.linkitems.append(link);
			return (true);
		}
	}
	// 下面永远不会达到
	// 没有找到，就查看node有没有
	if (rawNode()->getInputKeys().contains(pl.name) && pl.way == DANodeLinkPoint::Input) {
		// 存在input
		// 说明没有插入这个点
		DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& li = d_ptr->addLinkPoint(pl);
		li.linkitems.append(link);
		return true;
	}
	if (rawNode()->getOutputKeys().contains(pl.name) && pl.way == DANodeLinkPoint::Output) {
		// 存在output
		// 说明没有插入这个点
		DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& li = d_ptr->addLinkPoint(pl);
		li.linkitems.append(link);
		return true;
	}
	qDebug() << "can not record linked info";
	return false;
}

/**
 * @brief 连接的link在销毁时调用，把item记录的link信息消除
 * @param item
 * @param pl
 * @return
 */
bool DAAbstractNodeGraphicsItem::removeLinkInfo(DAAbstractNodeLinkGraphicsItem* link, const DANodeLinkPoint& pl)
{
	for (DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& ld : d_ptr->mLinkInfos) {
		if (ld.point.isEqualWayName(pl)) {
			return ld.linkitems.removeAll(link) > 0;
		}
	}
	return false;
}

/**
 * @brief 此函数根据DAAbstractNode的输入输出来生成默认的连接点
 *
 * 会调用@sa DAAbstractNode::inputKeys 获取所有的输入参数,
 * 调用@sa DAAbstractNode::outputKeys 判断是否生成输出节点
 *
 * 默认所有输入位于左边，所有输出位于右边
 * 此函数会调用@sa updateLinkPointPos 来更新点位置
 *
 * 连接点的方向属性仅仅是为了辅助绘制常规连接点，针对一些特别的连接点，
 * 可以通过@sa DANodeLinkPointDrawDelegate 绘制任意形状的连接点
 * @return
 */
QList< DANodeLinkPoint > DAAbstractNodeGraphicsItem::generateLinkPoint() const
{
	QList< DANodeLinkPoint > res;
	const DAAbstractNode* n = rawNode();
	if (nullptr == n) {
		return res;
	}
	// 生成输入点
	QList< QString > ks = n->getInputKeys();
	// 避免除0
	for (const QString& k : qAsConst(ks)) {
		DANodeLinkPoint lp;
		lp.direction = AspectDirection::West;
		lp.way       = DANodeLinkPoint::Input;
		lp.name      = k;
		res.append(lp);
	}

	// 生成输出点
	ks = n->getOutputKeys();
	for (const QString& k : qAsConst(ks)) {
		DANodeLinkPoint lp;
		lp.direction = AspectDirection::East;
		lp.way       = DANodeLinkPoint::Output;
		lp.name      = k;
		res.append(lp);
	}
	getLinkPointDrawDelegate()->layoutLinkPoints(res, getBodyRect());
	return (res);
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

/**
 * @brief 更改连接点的信息，name是连接点名字，如果有重名，只修改第一个查找到的名字的连接点
 * @param name
 * @param newLinkpoint
 */
void DAAbstractNodeGraphicsItem::setLinkPoint(const QString& name, const DANodeLinkPoint& newLinkpoint)
{
    for (auto& v : d_ptr->mLinkInfos) {
        if (v.point.name != name) {
            continue;
        }
        DANodeLinkPoint oldLp = v.point;
        v.point               = newLinkpoint;
        // 已经连接到的link信息也要变更
        for (auto link : qAsConst(v.linkitems)) {
            if (oldLp == link->fromNodeLinkPoint()) {
                link->updateFromLinkPointInfo(newLinkpoint);
            } else if (oldLp == link->toNodeLinkPoint()) {
                link->updateToLinkPointInfo(newLinkpoint);
            }
        }
    }
}

bool DAAbstractNodeGraphicsItem::saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const
{
    DAGraphicsResizeableItem::saveToXml(doc, parentElement, ver);
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

bool DAAbstractNodeGraphicsItem::loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver)
{
    if (!DAGraphicsResizeableItem::loadFromXml(itemElement, ver)) {
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
	// 获取具体连接点
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
            setLinkPointDirection(name, stringToEnum(le.attribute("direction"), AspectDirection::East));
		}
	}
	return true;
}

/**
 * @brief 创建连接，继承此函数可以生成连接，如果返回nullptr，scene将不会进行连接
 *
 * 默认使用DAStandardNodeLinkGraphicsItem来进行连接的创建，如果需要自定义连接，可以继承此函数
 *
 * 此函数创建的连接默认已经连接上lp
 *
 * 如果重写了自己的link，需要如下操作：
 * @code
 * DAAbstractNodeLinkGraphicsItem* MyNodeItem::createLinkItem(const DANodeLinkPoint& lp)
 * {
 *   Q_UNUSED(lp);
 *   return new MyLinkGraphicsItem();
 * }
 * @endcode
 *
 * @note 返回的link无需attach，attach过程由scene负责
 * @param lp 连接点，可以根据不同连接点返回不同的连接线
 * @return
 */
DAAbstractNodeLinkGraphicsItem* DAAbstractNodeGraphicsItem::createLinkItem(const DANodeLinkPoint& lp)
{
	Q_UNUSED(lp);
	return new DAStandardNodeLinkGraphicsItem();
}

/**
 * @brief 从fromPoint链接到toItem的toPoint点
 * @param fromPoint 出口链接点
 * @param toItem 链接到的item
 * @param toPoint 链接到的链接点
 * @return 如果链接失败返回nullptr
 */
DAAbstractNodeLinkGraphicsItem* DAAbstractNodeGraphicsItem::linkTo(const DANodeLinkPoint& fromPoint,
                                                                   DAAbstractNodeGraphicsItem* toItem,
                                                                   const DANodeLinkPoint& toPoint)
{
	// 创建链接线
    auto linkitem = std::unique_ptr< DAAbstractNodeLinkGraphicsItem >(createLinkItem(fromPoint));
	if (nullptr == linkitem) {
		return nullptr;
	}
	if (!linkitem->attachFrom(this, fromPoint)) {
		qDebug() << QObject::tr("link item can not attach from node item(%1) with key=%2")
                        .arg(this->getNodeName(), fromPoint.name);  // cn:无法在节点(%1)的连接点%2上建立链接
		return nullptr;
	}
	if (!linkitem->attachTo(toItem, toPoint)) {
		qDebug() << QObject::tr("link item can not attach to node item(%1) with key=%2")  // cn:无法链接到节点(%1)的连接点%2
                        .arg(toItem->getNodeName(), toPoint.name);

		return nullptr;
	}
	return linkitem.release();
}

/**
 * @brief 从fromPointName链接到toItem的toPointName点
 * @param fromPointName
 * @param toItem
 * @param toPointName
 * @return 如果链接失败返回nullptr
 */
DAAbstractNodeLinkGraphicsItem* DAAbstractNodeGraphicsItem::linkTo(const QString& fromPointName,
                                                                   DAAbstractNodeGraphicsItem* toItem,
                                                                   const QString& toPointName)
{
	DANodeLinkPoint fromlp = getOutputLinkPoint(fromPointName);
	DANodeLinkPoint tolp   = toItem->getInputLinkPoint(toPointName);
	if (!fromlp.isValid()) {
		qDebug() << QObject::tr("Node %1 cannot find a connection point named %2")  // cn:节点%1无法找到名字为%2的连接点
                        .arg(getNodeName(), fromPointName);
		return nullptr;
	}
	if (!tolp.isValid()) {
		qDebug() << QObject::tr("Node %1 cannot find a connection point named %2")  // cn:节点%1无法找到名字为%2的连接点
                        .arg(toItem->getNodeName(), toPointName);
		return nullptr;
	}
	return linkTo(fromlp, toItem, tolp);
}

/**
 * @brief 设置尺寸，这里要随之更新连接点
 * @param s
 */
void DAAbstractNodeGraphicsItem::setBodySize(const QSizeF& s)
{
	DAGraphicsResizeableItem::setBodySize(s);
	// 最后重新计算连接点必须在setBodySize之后
	updateLinkPointPos();
}

/**
 * @brief 清除链接信息
 */
void DAAbstractNodeGraphicsItem::clearLinkData()
{
	for (DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& d : d_ptr->mLinkInfos) {
		for (DAAbstractNodeLinkGraphicsItem* li : qAsConst(d.linkitems)) {
			li->callItemIsDestroying(this, d.point);
		}
	}
	d_ptr->mLinkInfos.clear();
}

/**
   @brief 递归获取item相关的所有连接的节点.
   @param $PARAMS
   @return 此节点下连接到此节点的个数
 */
int DAAbstractNodeGraphicsItem::getLinkChainRecursion(DAAbstractNodeGraphicsItem* item,
                                                      QSet< DAAbstractNodeGraphicsItem* >& res) const
{
    int finded = 0;

	// 找出口
	QList< DAAbstractNodeGraphicsItem* > items = item->getOutputItems();
	for (DAAbstractNodeGraphicsItem* d : qAsConst(items)) {
		if (res.contains(d)) {
			continue;
		}
		// 必须先在结果插入后再递归，否则会无限循环
		res.insert(d);
		++finded;
		getLinkChainRecursion(d, res);
	}

	// 找进口
	items = item->getInputItems();
	for (DAAbstractNodeGraphicsItem* d : qAsConst(items)) {
		if (res.contains(d)) {
			continue;
		}
		// 必须先在结果插入后再递归，否则会无限循环
		res.insert(d);
		++finded;
		getLinkChainRecursion(d, res);
	}
	return finded;
}

/**
 * @brief 更新连接点位置，在bodyBoundingRect改变之后调用
 * @param bodyRect
 */
void DAAbstractNodeGraphicsItem::updateLinkPointPos()
{
	QList< DANodeLinkPoint > lps = d_ptr->getLinkPoints();
	getLinkPointDrawDelegate()->layoutLinkPoints(lps, getBodyRect());
	// 更新到linkData
	int s = qMin(lps.size(), d_ptr->mLinkInfos.size());
	for (int i = 0; i < s; ++i) {
		// 把更新过后的linkpoint info设置回去
		d_ptr->mLinkInfos[ i ].point = lps[ i ];
		for (DAAbstractNodeLinkGraphicsItem* item : qAsConst(d_ptr->mLinkInfos[ i ].linkitems)) {
			if (lps[ i ].isInput()) {
				item->updateToLinkPointInfo(lps[ i ]);
			} else {
				item->updateFromLinkPointInfo(lps[ i ]);
			}
		}
	}
}

/**
 * @brief 更新链接到这个item的linkitem
 */
void DAAbstractNodeGraphicsItem::updateLinkItems()
{
	for (const DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& ld : qAsConst(d_ptr->mLinkInfos)) {
		for (DAAbstractNodeLinkGraphicsItem* li : ld.linkitems) {
			li->updatePos();
		}
	}
}

/**
 * @brief 显示连接点的文字
 * @param on
 */
void DAAbstractNodeGraphicsItem::showLinkPointText(bool on)
{
	if (auto delegate = getLinkPointDrawDelegate()) {
		delegate->showLinkPointText(on);
	}
}

bool DAAbstractNodeGraphicsItem::isShowLinkPointText() const
{
	if (auto delegate = getLinkPointDrawDelegate()) {
		return delegate->isShowLinkPointText();
	}
	return false;
}

/**
 * @brief 获取当前链接上的LinkGraphicsItem
 * @return
 */
QList< DAAbstractNodeLinkGraphicsItem* > DAAbstractNodeGraphicsItem::getLinkItems() const
{
	QSet< DAAbstractNodeLinkGraphicsItem* > res;
	for (DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& d : d_ptr->mLinkInfos) {
		for (DAAbstractNodeLinkGraphicsItem* li : qAsConst(d.linkitems)) {
			res.insert(li);
		}
	}
	return qset_to_qlist(res);
}

/**
 * @brief 获取所有链接进来这个节点的连接线
 * @return 将会去重，也就返回的内容不会有重复
 */
QList< DAAbstractNodeLinkGraphicsItem* > DAAbstractNodeGraphicsItem::getInputLinkItems() const
{
	QSet< DAAbstractNodeLinkGraphicsItem* > res;
	for (DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& d : d_ptr->mLinkInfos) {
		if (DANodeLinkPoint::Input != d.point.way) {
			// 不是输出节点就跳过
			continue;
		}
		for (DAAbstractNodeLinkGraphicsItem* li : qAsConst(d.linkitems)) {
			res.insert(li);
		}
	}
	return qset_to_qlist(res);
}

/**
 * @brief 获取这个节点链接出去的所有连接线
 * @return 将会去重，也就返回的内容不会有重复
 */
QList< DAAbstractNodeLinkGraphicsItem* > DAAbstractNodeGraphicsItem::getOutputLinkItems() const
{
	QSet< DAAbstractNodeLinkGraphicsItem* > res;
	for (DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& d : d_ptr->mLinkInfos) {
		if (DANodeLinkPoint::Output != d.point.way) {
			// 不是输出节点就跳过
			continue;
		}
		for (DAAbstractNodeLinkGraphicsItem* li : qAsConst(d.linkitems)) {
			res.insert(li);
		}
	}
	return qset_to_qlist(res);
}

/**
 * @brief 获取所有链接到这个节点的节点
 * @return 将会去重，也就返回的内容不会有重复
 */
QList< DAAbstractNodeGraphicsItem* > DAAbstractNodeGraphicsItem::getInputItems() const
{
	QSet< DAAbstractNodeGraphicsItem* > res;
	QList< DAAbstractNodeLinkGraphicsItem* > inputLinks = getInputLinkItems();
	for (DAAbstractNodeLinkGraphicsItem* li : qAsConst(inputLinks)) {
		if (DAAbstractNodeGraphicsItem* fi = li->fromNodeItem()) {
			res.insert(fi);
		}
	}
	return qset_to_qlist(res);
}

/**
 * @brief 获取这个节点链接出去的所有节点
 * @return  将会去重，也就返回的内容不会有重复
 */
QList< DAAbstractNodeGraphicsItem* > DAAbstractNodeGraphicsItem::getOutputItems() const
{
	QSet< DAAbstractNodeGraphicsItem* > res;
	QList< DAAbstractNodeLinkGraphicsItem* > outputLinks = getOutputLinkItems();
	for (DAAbstractNodeLinkGraphicsItem* li : qAsConst(outputLinks)) {
		if (DAAbstractNodeGraphicsItem* ti = li->toNodeItem()) {
			res.insert(ti);
		}
	}
	return qset_to_qlist(res);
}

QList< DAAbstractNodeLinkGraphicsItem* > DAAbstractNodeGraphicsItem::getLinkItem(const QString& name) const
{
	for (DAAbstractNodeGraphicsItem::PrivateData::LinkInfo& d : d_ptr->mLinkInfos) {
		if (d.point.name == name) {
			return d.linkitems;
		}
	}
	return QList< DAAbstractNodeLinkGraphicsItem* >();
}

/**
   @brief 获取链接链路，上所有的item

   这个链路如果有环，item不会重复出现，返回的链路不会包含自身

         F

         |

   A->B->C->D

   如上图，返回{A,B,C,D,F},注意是乱序

   @return 整个链路的item,注意是乱序
 */
QList< DAAbstractNodeGraphicsItem* > DAAbstractNodeGraphicsItem::getLinkChain() const
{
	QSet< DAAbstractNodeGraphicsItem* > res;
	// 先插入一个，避免回环
	res.insert(const_cast< DAAbstractNodeGraphicsItem* >(this));

	QList< DAAbstractNodeGraphicsItem* > items = getOutputItems();
	for (DAAbstractNodeGraphicsItem* d : qAsConst(items)) {
		if (res.contains(d)) {
			continue;
		}
		// 必须先在结果插入后再递归，否则会无限循环
		res.insert(d);
		getLinkChainRecursion(d, res);
	}
	// 找进口
	items = getInputItems();
	for (DAAbstractNodeGraphicsItem* d : qAsConst(items)) {
		if (res.contains(d)) {
			continue;
		}
		// 必须先在结果插入后再递归，否则会无限循环
		res.insert(d);
		getLinkChainRecursion(d, res);
	}
	// 把自身去掉
	res.remove(const_cast< DAAbstractNodeGraphicsItem* >(this));
	return qset_to_qlist(res);
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

DAAbstractNodeGraphicsItem::LinkPointLocation stringToEnum(const QString& s,
                                                           DAAbstractNodeGraphicsItem::LinkPointLocation defaultEnum)
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
