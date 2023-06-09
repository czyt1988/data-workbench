﻿#ifndef DAABSTRACTNODEGRAPHICSITEM_H
#define DAABSTRACTNODEGRAPHICSITEM_H
#include "DAWorkFlowGlobal.h"
#include <QAbstractGraphicsShapeItem>
#include <QIcon>
#include "DANodeMetaData.h"
#include "DANodeLinkPoint.h"
#include "DAGraphicsResizeableItem.h"
#include "DAAbstractNode.h"
class QDomDocument;
class QDomElement;
namespace DA
{
class DAAbstractNodeLinkGraphicsItem;
class DANodeLinkPointDrawDelegate;
class DAAbstractNodeWidget;
class DANodePalette;
DA_IMPL_FORWARD_DECL(DAAbstractNodeGraphicsItem)

/**
 * @brief 这是节点的基类，workflow所有节点都继承此类
 * 作为一个节点的QGraphicsItem，此item应该由DAAbstractNode创建
 *
 * 此函数继承@ref DAGraphicsResizeableItem
 * 需要重载 @ref DAGraphicsResizeableItem::paintBody
 *
 * @li 通过继承@ref DAAbstractNodeGraphicsItem::getNodeWidget 实现设置窗口
 * @li 通过继承@ref DAGraphicsResizeableItem::paintBody 实现绘制
 *
 * @note 这里有个限制，一个@ref DANodeLinkPoint 只能对应一个node的input或output，无法实现一个input对应多个@ref DANodeLinkPoint
 * @sa DAGraphicsResizeableItem DAAbstractNodeLinkGraphicsItem DANodeLinkPoint
 */
class DAWORKFLOW_API DAAbstractNodeGraphicsItem : public DAGraphicsResizeableItem
{
    Q_OBJECT
    DA_IMPL(DAAbstractNodeGraphicsItem)
    friend class DAAbstractNodeLinkGraphicsItem;
    friend class DAAbstractNode;

public:
    enum
    {
        Type = DA::ItemType_GraphicsNodeItem
    };
    int type() const
    {
        return (Type);
    }
    /**
     * @brief 连接点显示状态
     */
    enum LinkPointShowType
    {
        LinkPointAlwayShow,    ///< 一直显示
        LinkPointShowOnHover,  ///< 处于焦点或者选中才显示
        LinkPointShowOnSelect  ///< 只有选中时才显示
    };
    /**
     * @brief 连接点的位置
     */
    enum LinkPointLocation
    {
        LinkPointLocationOnLeftSide,   ///< 连接点位于左边(进口连接点默认左边)
        LinkPointLocationOnTopSide,    ///< 连接点位于上边
        LinkPointLocationOnRightSide,  ///< 连接点位于右边(出口连接点默认右边)
        LinkPointLocationOnBottomSide  ///< 连接点位于下边
    };

public:
    DAAbstractNodeGraphicsItem(DAAbstractNode* n, QGraphicsItem* p = nullptr);
    virtual ~DAAbstractNodeGraphicsItem();

    DAAbstractNode* rawNode();
    const DAAbstractNode* rawNode() const;
    DAAbstractNode::SharedPointer node() const;
    //设置连接点的显示属性
    void setLinkPointShowType(LinkPointShowType t);
    LinkPointShowType getLinkPointShowType() const;
    //设置连接点的位置
    void setLinkPointLocation(DANodeLinkPoint::Way way, LinkPointLocation l);
    LinkPointLocation getLinkPointLocation(DANodeLinkPoint::Way way) const;
    //获取node的名字
    QString getNodeName() const;
    //

    //获取图标，图标是节点对应的图标
    QIcon getIcon() const;
    void setIcon(const QIcon& icon);

    //获取节点的元数据
    const DANodeMetaData& metaData() const;
    DANodeMetaData& metaData();

    //获取连接点
    QList< DANodeLinkPoint > getLinkPoints() const;

    //通过名字获取连接点
    DANodeLinkPoint getLinkPoint(const QString& name) const;
    DANodeLinkPoint getInputLinkPoint(const QString& name) const;
    DANodeLinkPoint getOutputLinkPoint(const QString& name) const;
    //对linkpoint的属性设置,linkpoint 方向设置只会影响显示，不会影响工作流的链接
    bool setNodeLinkPointDirection(const QString& name, DANodeLinkPoint::Direction d);

    //判断是否存在连接点
    bool isHaveLinkPoint(const DANodeLinkPoint& pl) const;

    //绘制连接点，如果要绘制自己的连接点，可注入DANodeLinkPointDrawDelegate
    void paintLinkPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    //获取节点对应的窗口，一般保存节点的设置
    virtual DAAbstractNodeWidget* getNodeWidget() const;

    // palette设置
    void setNodePalette(const DANodePalette& pl);
    const DANodePalette& getNodePalette() const;

    //设置连接点绘制的代理，如果不设置会有一个默认代理
    void setLinkPointDrawDelegate(DANodeLinkPointDrawDelegate* delegate);
    DANodeLinkPointDrawDelegate* getLinkPointDrawDelegate() const;
    //通过位置获取linkpoint,如果没有返回一个invalid的DANodeLinkPoint
    //此函数作为虚函数，是scene判断是否点击到了链接点的关键函数，如果连接点是固定的
    //是不需要继承此函数的，但对于一些特殊的连接点（如不固定的）就需要通过此函数来获取
    virtual DANodeLinkPoint getLinkPointByPos(const QPointF& p) const;
    //更新连接点的位置，此函数一般在setbody之后更新点的位置
    void updateLinkPointPos();
    //获取当前链接上的LinkGraphicsItem
    QList< DAAbstractNodeLinkGraphicsItem* > getLinkItems() const;
    //获取连接item
    QList< DAAbstractNodeLinkGraphicsItem* > getLinkItem(const QString& name) const;
    //保存到xml中
    virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement) const override;
    virtual bool loadFromXml(const QDomElement* itemElement) override;

public:
    // prepare系列函数，用于在改变前的回调
    //此函数是在准备调用getLinkPointByPos之前调用的函数，用来准备输入节点
    virtual void prepareLinkInput(const QPointF& p, DAAbstractNodeLinkGraphicsItem* linkItem);
    virtual void prepareLinkInputFailed(const DANodeLinkPoint& p, DAAbstractNodeLinkGraphicsItem* linkItem);
    virtual void prepareLinkInputSucceed(const DANodeLinkPoint& p, DAAbstractNodeLinkGraphicsItem* linkItem);
    //此函数是在准备调用getLinkPointByPos之前调用的函数，用来准备输出节点
    virtual void prepareLinkOutput(const QPointF& p);
    virtual void prepareLinkOutputFailed(const DANodeLinkPoint& p);
    virtual void prepareLinkOutputSucceed(const DANodeLinkPoint& p);
    //节点名字改变准备函数，通过此函数，让节点对名字进行重新绘制
    virtual void prepareNodeNameChanged(const QString& name);

protected:
    //处理一些联动事件，如和FCAbstractNodeLinkGraphicsItem的联动
    virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value);

    //鼠标事件
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
    //此函数用于FCAbstractNodeLinkGraphicsItem在调用attachedTo/From过程中调用
    bool linked(DAAbstractNodeLinkGraphicsItem* link, const DANodeLinkPoint& pl);

    //连接的link在销毁时调用，把item记录的link信息消除
    bool linkItemRemoved(DAAbstractNodeLinkGraphicsItem* link, const DANodeLinkPoint& pl);

    //生成linkpoint
    virtual QList< DANodeLinkPoint > generateLinkPoint() const;
    //重置连接点，此函数会自动调用generateLinkPoint，如果想自定义，重载此函数
    void resetLinkPoint();
    //更新连接点，传入已有的连接点和总体尺寸，通过此函数的重写可以改变连接点的位置，如果想改变连接点的绘制应该通过setLinkPointDrawDelegate实现
    virtual void changeLinkPointPos(QList< DANodeLinkPoint >& lps, const QRectF& bodyRect) const;
    //有新的连接点加入
    void addLinkPoint(const DANodeLinkPoint& lp);

private:
    void clearLinkData();
};
// DA::DAAbstractNodeGraphicsItem::LinkPointLocation的枚举转换
DAWORKFLOW_API QString enumToString(DA::DAAbstractNodeGraphicsItem::LinkPointLocation e);
// DA::DAAbstractNodeGraphicsItem::LinkPointLocation的枚举转换
DAWORKFLOW_API DA::DAAbstractNodeGraphicsItem::LinkPointLocation stringToEnum(
const QString& s,
DA::DAAbstractNodeGraphicsItem::LinkPointLocation defaultEnum = DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide);

}
#endif  // FCNODEGRAPHICSITEM_H
