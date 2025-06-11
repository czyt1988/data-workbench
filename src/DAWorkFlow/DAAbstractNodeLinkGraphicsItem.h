#ifndef DAABSTRACTNODELINKGRAPHICSITEM_H
#define DAABSTRACTNODELINKGRAPHICSITEM_H
#include "DAWorkFlowGlobal.h"
#include <QGraphicsItem>
#include <QtCore/qglobal.h>
#include "DANodeLinkPoint.h"
#include "DAAbstractNode.h"
#include "DAGraphicsLinkItem.h"
class QGraphicsSimpleTextItem;
namespace DA
{
class DAAbstractNodeGraphicsItem;
class DAWorkFlow;
/**
 * @brief 绘制连接线的item
 *
 * 注意，boundingRect的改变前需要调用prepareGeometryChange，避免出现残影
 */
class DAWORKFLOW_API DAAbstractNodeLinkGraphicsItem : public DAGraphicsLinkItem
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DAAbstractNodeLinkGraphicsItem)
	friend class DAAbstractNodeGraphicsItem;

public:
	enum AnonymousType
	{
		anonymous = DA::ItemType_GraphicsNodeLinkItem
	};
	int type() const override
	{
		return (anonymous);
	}

public:
	DAAbstractNodeLinkGraphicsItem(QGraphicsItem* p = nullptr);
	DAAbstractNodeLinkGraphicsItem(DAAbstractNodeGraphicsItem* from, const DA::DANodeLinkPoint& pl, QGraphicsItem* p = nullptr);
	virtual ~DAAbstractNodeLinkGraphicsItem();
	// 自动根据fromitem来更新位置
	void updatePos();

	// 更新范围参数
	virtual QRectF updateBoundingRect() override;

	// 通过两个点形成一个矩形，两个点总能形成一个矩形，如果重合，返回一个空矩形
	static QRectF rectFromTwoPoint(const QPointF& p0, const QPointF& p1);

	// 设置是否显示连接点的文本
	void setLinkPointNameVisible(bool on = true, Orientations o = OrientationBoth);
	bool isLinkPointNameVisible(Orientations o = OrientationBoth) const;

	// 设置连接点显示的颜色
	void setLinkPointNameTextColor(const QColor& c, Orientations o = OrientationBoth);
	QColor getLinkPointNameTextColor(Orientations o) const;

	// 设置文本和连接点的偏移量，默认为10
	void setLinkPointNamePositionOffset(int offset, Orientations o = OrientationBoth);
	int getLinkPointNamePositionOffset(Orientations o) const;

	QGraphicsSimpleTextItem* getFromTextItem() const;
	QGraphicsSimpleTextItem* getToTextItem() const;
	// 设置文本
	void setText(const QString& t);
	QString getText() const;
	// 获取文本对应的item
	QGraphicsSimpleTextItem* getTextItem();
	// 获取from、to node item，如果没有返回nullptr
	DAAbstractNodeGraphicsItem* fromNodeItem() const;
	DAAbstractNodeGraphicsItem* toNodeItem() const;
	// from、to的连接点
	DANodeLinkPoint fromNodeLinkPoint() const;
	DANodeLinkPoint toNodeLinkPoint() const;
	// 获取from、to的节点，如果没有返回nullptr
	DAAbstractNode::SharedPointer fromNode() const;
	DAAbstractNode::SharedPointer toNode() const;

	// 完成节点连接的回调
	virtual void finishedNodeLink();

	// 在将要结束链接的回调，通过此回调可以执行完成链接后的相关操作，例如判断末端链接的图元，从而实现路径调整
	// 如果此函数返回false，将代表不接受链接，这时候，结束动作会被跳过，也就是鼠标点击是没有无法结束链接而生成连接线
	virtual bool willCompleteLink() override;
	// 生成painterpath
	virtual QPainterPath generateLinePainterPath(const QPointF& fromPoint,
												 const QPointF& toPoint,
												 LinkLineStyle linestyle = LinkLineStraight) override;

	// 开始节点连接
	bool attachFrom(DAAbstractNodeGraphicsItem* item, const QString& name);
	bool attachFrom(DAAbstractNodeGraphicsItem* item, const DANodeLinkPoint& pl);
	// 清空from节点
	void detachFrom();

	// 结束节点连接
	bool attachTo(DAAbstractNodeGraphicsItem* item, const QString& name);
	bool attachTo(DAAbstractNodeGraphicsItem* item, const DANodeLinkPoint& pl);

	// 清空to节点
	void detachTo();

	// 已经连接完成，在from和to都有节点时，返回true
	bool isLinked() const;
	// 保存到xml中
	virtual bool saveToXml(QDomDocument* doc, QDomElement* parentElement, const QVersionNumber& ver) const override;
	virtual bool loadFromXml(const QDomElement* parentElement, const QVersionNumber& ver) override;

protected:
	//
	void updateFromLinkPointInfo(const DANodeLinkPoint& pl);
	void updateToLinkPointInfo(const DANodeLinkPoint& pl);
	// 添加事件处理
	QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;

	// 连接的item在销毁，销毁过程对应的item会调用此函数，把link记录的item信息消除
	void callItemIsDestroying(DAAbstractNodeGraphicsItem* item, const DA::DANodeLinkPoint& pl);
};
}  // end of namespace DA

#endif  // FCABSTRACTNODELINKGRAPHICSITEM_H
