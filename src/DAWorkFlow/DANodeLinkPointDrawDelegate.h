#ifndef DANODELINKPOINTDRAWDELEGATE_H
#define DANODELINKPOINTDRAWDELEGATE_H
#include "DAWorkFlowGlobal.h"
#include "DANodeLinkPoint.h"
#include <QPainterPath>
namespace DA
{

class DAAbstractNodeGraphicsItem;
/**
 * @brief linkpoint的绘制代理
 *
 * 想要实现自己的连接点可以重载此代理
 *
 * @sa DAAbstractNodeGraphicsItem::setLinkPointDrawDelegate
 */
class DAWORKFLOW_API DANodeLinkPointDrawDelegate
{
	DA_DECLARE_PRIVATE(DANodeLinkPointDrawDelegate)
public:
	DANodeLinkPointDrawDelegate(DAAbstractNodeGraphicsItem* i = nullptr);
	virtual ~DANodeLinkPointDrawDelegate();
	// 设置item
	void setItem(DAAbstractNodeGraphicsItem* i);
	DAAbstractNodeGraphicsItem* getItem() const;
	// 绘制连接点
	void paintLinkPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

public:
	// 获取连接点的矩形绘图区域范围
	//  此函数会影响到场景链接过程选中的状态，比较关键，决定了DANodeGraphicsScene::nodeItemLinkPointSelected能否发射,paintLinkPoint函数会调用此函数确定绘图区域
	virtual QPainterPath getlinkPointPainterRegion(const DANodeLinkPoint& pl) const;
	// 获取连接点,正常情况，不需要继承此函数，此函数只有比较特殊的情况继承
	virtual QList< DANodeLinkPoint > getLinkPoints() const;
	// 绘制某个连接点
	virtual void paintLinkPoint(const DANodeLinkPoint& pl,
								QPainter* painter,
								const QStyleOptionGraphicsItem* option,
								QWidget* widget);
};

}  // end of namespace DA
#endif  // DANODELINKPOINTDRAWDELEGATE_H
