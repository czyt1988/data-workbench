#ifndef DAABSTRACTNODEGRAPHICSITEM_H
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
 * 以下是paintBody的例子：
 *
 * @code
 * void XXNodeGraphicsItem::paintBody(QPainter* painter,const QStyleOptionGraphicsItem* option,QWidget* widget,const QRectF& bodyRect)
 * {
 *     //! 先绘制矩形
 *     painter->setPen(getBorderPen());
 *     painter->setBrush(getBackgroundBrush());
 *     painter->drawRect(bodyRect);
 *     //! 绘制连接点
 *     paintLinkPoints(painter, option, widget);
 * }
 * @endcode
 *
 * @note 这里有个限制，一个@ref DANodeLinkPoint 只能对应一个node的input或output，无法实现一个input对应多个@ref DANodeLinkPoint
 * @sa DAGraphicsResizeableItem DAAbstractNodeLinkGraphicsItem DANodeLinkPoint
 */
class DAWORKFLOW_API DAAbstractNodeGraphicsItem : public DAGraphicsResizeableItem
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAAbstractNodeGraphicsItem)
	friend class DAAbstractNodeLinkGraphicsItem;
	friend class DAAbstractNode;

public:
	enum
	{
		Type = DA::ItemType_GraphicsNodeItem
	};
	int type() const override
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
	// 设置连接点的显示属性
	void setLinkPointShowType(LinkPointShowType t);
	LinkPointShowType getLinkPointShowType() const;
	// 设置连接点的位置
	void setLinkPointLocation(DANodeLinkPoint::Way way, LinkPointLocation l);
	LinkPointLocation getLinkPointLocation(DANodeLinkPoint::Way way) const;
	// 获取node的名字
	QString getNodeName() const;
	//

	// 获取图标，图标是节点对应的图标
	QIcon getIcon() const;
	void setIcon(const QIcon& icon);

	// 获取节点的元数据
	const DANodeMetaData& metaData() const;
	DANodeMetaData& metaData();

	// 获取连接点
	QList< DANodeLinkPoint > getLinkPoints() const;
	QList< DANodeLinkPoint > getOutputLinkPoints() const;
	QList< DANodeLinkPoint > getInputLinkPoints() const;

	// 通过名字获取连接点
	DANodeLinkPoint getLinkPoint(const QString& name) const;
	DANodeLinkPoint getInputLinkPoint(const QString& name) const;
	DANodeLinkPoint getOutputLinkPoint(const QString& name) const;
	// 对linkpoint的属性设置,linkpoint 方向设置只会影响显示，不会影响工作流的链接
	bool setLinkPointDirection(const QString& name, AspectDirection d);

	// 判断是否存在连接点
	bool isHaveLinkPoint(const DANodeLinkPoint& pl) const;
	// 判断连接点是否已经链接
	bool isLinkPointLinked(const DANodeLinkPoint& pl);
	// 绘制连接点，在paintbody中调用此函数，用于绘制连接点
	void paintLinkPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	// 获取节点对应的窗口，一般保存节点的设置
	virtual DAAbstractNodeWidget* getNodeWidget();

	// palette设置
	void setNodePalette(const DANodePalette& pl);
	const DANodePalette& getNodePalette() const;

	// 设置连接点绘制的代理，如果不设置会有一个默认代理
	void setLinkPointDrawDelegate(DANodeLinkPointDrawDelegate* delegate);
	DANodeLinkPointDrawDelegate* getLinkPointDrawDelegate() const;
	// 通过位置获取linkpoint,如果没有返回一个invalid的DANodeLinkPoint
	// 此函数作为虚函数，是scene判断是否点击到了链接点的关键函数，如果连接点是固定的
	// 是不需要继承此函数的，但对于一些特殊的连接点（如不固定的）就需要通过此函数来获取
	virtual DANodeLinkPoint getLinkPointByPos(const QPointF& p, DANodeLinkPoint::Way way = DANodeLinkPoint::Output) const;
	// 更新连接点的位置，此函数一般在setbody之后更新点的位置
	void updateLinkPointPos();
	// 更新linkitem
	void updateLinkItems();
	// 显示连接点的文字
	void showLinkPointText(bool on);
	bool isShowLinkPointText() const;
	// 获取当前链接上的LinkGraphicsItem
	QList< DAAbstractNodeLinkGraphicsItem* > getLinkItems() const;
	// 获取所有链接进来这个节点的连接线
	QList< DAAbstractNodeLinkGraphicsItem* > getInputLinkItems() const;
	// 获取这个节点链接出去的所有连接线
	QList< DAAbstractNodeLinkGraphicsItem* > getOutputLinkItems() const;
	// 获取所有链接到这个节点的节点
	QList< DAAbstractNodeGraphicsItem* > getInputItems() const;
	// 获取这个节点链接出去的所有节点
	QList< DAAbstractNodeGraphicsItem* > getOutputItems() const;
    // 获取输入的信息,把节点和连接点都获取到
    QList< std::pair< DAAbstractNodeGraphicsItem*, DANodeLinkPoint > > getInputInfos() const;
    // 获取输出的信息,把节点和连接点都获取到
    QList< std::pair< DAAbstractNodeGraphicsItem*, DANodeLinkPoint > > getOutputInfos() const;
    // 获取链接链路，上所有的item，这个链路如果有环，item不会重复出现，返回的链路不会包含自身
	QList< DAAbstractNodeGraphicsItem* > getLinkChain() const;
    // 获取输出链接链路，返回输出链路上所有的item，这个链路如果有环，item不会重复出现，返回的链路不会包含自身
    QList< DAAbstractNodeGraphicsItem* > getOutPutLinkChain() const;
    // 获取输出链接链路，返回输出链路上所有的item，这个链路如果有环，item不会重复出现，返回的链路不会包含自身
    QList< DAAbstractNodeGraphicsItem* > getInPutLinkChain() const;
	// 获取连接item
	QList< DAAbstractNodeLinkGraphicsItem* > getLinkItem(const QString& name) const;
	// 保存到xml中
	virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const override;
	virtual bool loadFromXml(const QDomElement* itemElement, const QVersionNumber& ver) override;
	// 创建连接，继承此函数可以生成连接，如果返回nullptr，scene将不会进行连接
	// 默认使用DAStandardNodeLinkGraphicsItem来进行连接的创建，如果需要自定义连接，可以继承此函数
	// 返回的link无需attach，attach过程由scene负责
	virtual DAAbstractNodeLinkGraphicsItem* createLinkItem(const DA::DANodeLinkPoint& lp);
	// 从fromPoint链接到toItem的toPoint点，如果链接失败返回nullptr
    virtual DAAbstractNodeLinkGraphicsItem* linkTo(const DA::DANodeLinkPoint& fromPoint,
                                                   DAAbstractNodeGraphicsItem* toItem,
                                                   const DA::DANodeLinkPoint& toPoint);
    DAAbstractNodeLinkGraphicsItem* linkToByName(const QString& fromPointName,
                                                 DAAbstractNodeGraphicsItem* toItem,
                                                 const QString& toPointName);
	virtual void setBodySize(const QSizeF& s) override;
	// 重置连接点，此函数会自动调用generateLinkPoint，如果想自定义，重载此函数
	// 如果重载了generateLinkPoint或changeLinkPointPos，在构造函数中调用此函数
	void resetLinkPoint();

	/// @defgroup 拖曳操作
	/// @{
	// 是否接受DANodeMetaData拖曳在此节点上
	virtual bool acceptDragOn(DANodeMetaData mime, const QPointF& scenePos);
	// 释放DANodeMetaData操作
	virtual bool drop(DANodeMetaData mime, const QPointF& scenePos);
	/// @}
public:
	// prepare系列函数，用于在改变前的回调
	// 此函数是在准备调用getLinkPointByPos之前调用的函数，用来准备输入节点
	// 准备链接回调，对于旧版的prepareLinkInput，prepareLinkOutput两个回调
	virtual void tryLinkOnItemPos(const QPointF& p, DAAbstractNodeLinkGraphicsItem* linkItem, DANodeLinkPoint::Way way);
	// 链接结束回调，对应旧版的prepareLinkInputFailed,prepareLinkInputSucceed,prepareLinkOutputFailed,prepareLinkOutputSucceed四个回调
	virtual void finishLink(const DANodeLinkPoint& p,
                            DAAbstractNodeLinkGraphicsItem* linkItem,
                            DANodeLinkPoint::Way way,
                            bool isSuccess);
	// 断开连接的回调，detach是针对已经连接上的断开
	virtual void detachLink(const DANodeLinkPoint& p, DAAbstractNodeLinkGraphicsItem* linkItem, DANodeLinkPoint::Way way);
	// 节点名字改变准备函数，通过此函数，让节点对名字进行重新绘制
	virtual void prepareNodeNameChanged(const QString& name);
	// 分组位置发生了变化
	virtual void groupPositionChanged(const QPointF& pos) override;

protected:
	// 处理一些联动事件，如和FCAbstractNodeLinkGraphicsItem的联动
	virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;

	// 鼠标事件
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
	// 此函数在DAAbstractNodeLinkGraphicsItem的attachedTo/From过程中调用
	bool recordLinkInfo(DAAbstractNodeLinkGraphicsItem* link, const DANodeLinkPoint& pl);

	// 连接的link在销毁时调用，把item记录的link信息消除
	bool removeLinkInfo(DAAbstractNodeLinkGraphicsItem* link, const DANodeLinkPoint& pl);

	// 生成linkpoint，默认会把输入设置在左边，输出设置在右边，并生成一个举行的链接点
	virtual QList< DANodeLinkPoint > generateLinkPoint() const;

	// 有新的连接点加入
	void addLinkPoint(const DANodeLinkPoint& lp);
	// 更改连接点的信息，name是连接点名字，如果有重名，只修改第一个查找到的名字的连接点
	void setLinkPoint(const QString& name, const DANodeLinkPoint& newLinkpoint);

private:
	void clearLinkData();
	// 递归获取链接的原件
	int getLinkChainRecursion(DAAbstractNodeGraphicsItem* item, QSet< DAAbstractNodeGraphicsItem* >& res) const;
    // 递归获取链接的原件
    void getOutLinkChainRecursion(DAAbstractNodeGraphicsItem* item, QSet< DAAbstractNodeGraphicsItem* >& res) const;
    // 递归获取链接的原件
    void getInLinkChainRecursion(DAAbstractNodeGraphicsItem* item, QSet< DAAbstractNodeGraphicsItem* >& res) const;
};
// DA::DAAbstractNodeGraphicsItem::LinkPointLocation的枚举转换
DAWORKFLOW_API QString enumToString(DA::DAAbstractNodeGraphicsItem::LinkPointLocation e);
// DA::DAAbstractNodeGraphicsItem::LinkPointLocation的枚举转换
DAWORKFLOW_API DA::DAAbstractNodeGraphicsItem::LinkPointLocation stringToEnum(
    const QString& s,
    DA::DAAbstractNodeGraphicsItem::LinkPointLocation defaultEnum = DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide);

}
#endif  // FCNODEGRAPHICSITEM_H
