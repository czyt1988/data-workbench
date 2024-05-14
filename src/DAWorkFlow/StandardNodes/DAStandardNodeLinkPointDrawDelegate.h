#ifndef DASTANDARDNODELINKPOINTDRAWDELEGATE_H
#define DASTANDARDNODELINKPOINTDRAWDELEGATE_H
#include "DANodeLinkPointDrawDelegate.h"
#include "DANodePalette.h"
namespace DA
{
/**
 * @brief 绘制连接点的代理
 *
 * 这是此插件标准的链接点绘制代理
 */
class DAWORKFLOW_API DAStandardNodeLinkPointDrawDelegate : public DA::DANodeLinkPointDrawDelegate
{
public:
	DAStandardNodeLinkPointDrawDelegate(DA::DAAbstractNodeGraphicsItem* i = nullptr);
	virtual ~DAStandardNodeLinkPointDrawDelegate();

public:
	// 重新计算linkpoint的信息
	void layoutLinkPoints(QList< DA::DANodeLinkPoint >& lps, const QRectF& bodyRect) override;
	//  此函数会影响到场景链接过程选中的状态，比较关键，决定了DANodeGraphicsScene::nodeItemLinkPointSelected能否发射
	virtual QPainterPath getlinkPointPainterRegion(const DA::DANodeLinkPoint& pl) const override;
	// 生成一个三角形
	static QPainterPath makeTriangle(const QPoint& a, const QPoint& b, const QPoint& c);
	// 形成一个封闭图形
	static QPainterPath closePainterPath(const QPoint& a, const QPoint& b, const QPoint& c);
	static QPainterPath closePainterPath(const QPoint& a, const QPoint& b, const QPoint& c, const QPoint& d);
	static QPainterPath closePainterPath(const QPointF& a, const QPointF& b, const QPointF& c, const QPointF& d);
	// 获取连接点的尺寸
	QSize getLinkPointSize() const;
	// 允许一个节点多次链接
	void setEnableMultLink(bool on);
	bool isEnableMultLink() const;

protected:
	// 获取连接点的序号
	int getLinkPointIndex(const DA::DANodeLinkPoint& pl) const;
	DA::DANodePalette& palette() const;

private:
	QRectF mRect;
	bool mEnableMultLink { false };
};
}
#endif  // DASTANDARDNODELINKPOINTDRAWDELEGATE_H
