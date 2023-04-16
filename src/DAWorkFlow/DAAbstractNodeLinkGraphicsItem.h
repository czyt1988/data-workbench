#ifndef DAABSTRACTNODELINKGRAPHICSITEM_H
#define DAABSTRACTNODELINKGRAPHICSITEM_H
#include "DAWorkFlowGlobal.h"
#include <QGraphicsItem>
#include <QtCore/qglobal.h>
#include "DANodeLinkPoint.h"
#include "DAAbstractNode.h"
class QGraphicsSimpleTextItem;
namespace DA
{
class DAAbstractNodeGraphicsItem;
/**
 * @brief 绘制连接线的item
 *
 * 注意，boundingRect的改变前需要调用prepareGeometryChange，避免出现残影
 */
class DAWORKFLOW_API DAAbstractNodeLinkGraphicsItem : public QGraphicsItem
{
    DA_DECLARE_PRIVATE(DAAbstractNodeLinkGraphicsItem)
    friend class DAAbstractNodeGraphicsItem;
    friend class FCNodeGraphicsScene;

public:
    enum AnonymousType
    {
        anonymous = DA::ItemType_GraphicsLinkItem
    };
    int type() const override
    {
        return (anonymous);
    }
    /**
     * @brief 连接点的样式
     */
    enum LinkLineStyle
    {
        LinkLineBezier,    ///< 贝塞尔曲线连接
        LinkLineStraight,  ///< 直线连接
        LinkLineKnuckle    ///< 肘形连接（直角）
    };

    /**
     * @brief 标记方向
     */
    enum Orientations
    {
        OrientationFrom = 0,  ///< 连接的开始点
        OrientationTo,        ///< 连接的结束点
        OrientationBoth       ///< 两个都包含
    };

    /**
     * @brief 端点样式
     */
    enum EndPointType
    {
        EndPointNone,       ///< 没有箭头
        EndPointTriangType  ///< 三角形箭头 ▲
    };

    //连线方式
    void setLinkLineStyle(LinkLineStyle s);
    LinkLineStyle getLinkLineStyle() const;
    //自动根据fromitem来更新位置
    void updatePos();

    //更新范围参数
    void updateBoundingRect();
    // boundingRect改变的回调,此函数可以用户重载实现一些附加item的绘制
    virtual void boundingRectChanged(const QRectF& boundrect);
    //通过两个点形成一个矩形，两个点总能形成一个矩形，如果重合，返回一个空矩形
    static QRectF rectFromTwoPoint(const QPointF& p0, const QPointF& p1);

    //延长线，以一个方向和距离延伸
    static QPointF elongation(const QPointF& orgPoint, DANodeLinkPoint::Direction d, qreal externLen);

    //计算两个点的距离
    static qreal pointLength(const QPointF& a, const QPointF& b);

    //设置贝塞尔曲线的控制点的缩放比例，FCAbstractNodeLinkGraphicsItem在连线时按照控制点的方向延伸出贝塞尔曲线的控制点，延伸的控制点的长度w = length * bezierControlScale
    void setBezierControlScale(qreal rate = 0.25);
    qreal getBezierControlScale() const;

    //设置线的画笔
    void setLinePen(const QPen& p);
    QPen getLinePen() const;

    //设置是否显示连接点的文本
    void setLinkPointNameVisible(bool on = true, Orientations o = OrientationBoth);
    bool isLinkPointNameVisible(Orientations o = OrientationBoth) const;

    //设置连接点显示的颜色
    void setLinkPointNameTextColor(const QColor& c, Orientations o = OrientationBoth);
    QColor getLinkPointNameTextColor(Orientations o) const;

    //设置文本和连接点的偏移量，默认为10
    void setLinkPointNamePositionOffset(int offset, Orientations o = OrientationBoth);
    int getLinkPointNamePositionOffset(Orientations o) const;

    QGraphicsSimpleTextItem* getFromTextItem() const;
    QGraphicsSimpleTextItem* getToTextItem() const;

    //生成painterpath
    virtual QPainterPath generateLinkLineBezierPainterPath(const QPointF& fromPos,
                                                           DANodeLinkPoint::Direction fromDirect,
                                                           const QPointF& toPos,
                                                           DANodeLinkPoint::Direction toDirect);
    //生成直线
    virtual QPainterPath generateLinkLineStraightPainterPath(const QPointF& fromPos,
                                                             DANodeLinkPoint::Direction fromDirect,
                                                             const QPointF& toPos,
                                                             DANodeLinkPoint::Direction toDirect);
    //生成直角线
    virtual QPainterPath generateLinkLineKnucklePainterPath(const QPointF& fromPos,
                                                            DANodeLinkPoint::Direction fromDirect,
                                                            const QPointF& toPos,
                                                            DANodeLinkPoint::Direction toDirect);
    //设置文本
    void setText(const QString& t);
    QString getText() const;
    //获取文本对应的item
    QGraphicsSimpleTextItem* getTextItem();
    //获取from、to node item，如果没有返回nullptr
    DAAbstractNodeGraphicsItem* fromNodeItem() const;
    DAAbstractNodeGraphicsItem* toNodeItem() const;
    // from、to的连接点
    DANodeLinkPoint fromNodeLinkPoint() const;
    DANodeLinkPoint toNodeLinkPoint() const;
    //获取from、to的节点，如果没有返回nullptr
    DAAbstractNode::SharedPointer fromNode() const;
    DAAbstractNode::SharedPointer toNode() const;
    //设置连接点的形式
    void setEndPointType(Orientations o, EndPointType epType);
    EndPointType getEndPointType(Orientations o) const;
    //设置端点的大小
    void setEndPointSize(int size);
    int getEndPointSize() const;
    //完成链接的回调
    virtual void finishedLink();

public:
    DAAbstractNodeLinkGraphicsItem(QGraphicsItem* p = nullptr);
    DAAbstractNodeLinkGraphicsItem(DAAbstractNodeGraphicsItem* from, const DA::DANodeLinkPoint& pl, QGraphicsItem* p = nullptr);
    virtual ~DAAbstractNodeLinkGraphicsItem();
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    //绘制连接线
    virtual void paintLinkLine(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget, const QPainterPath& linkPath);
    //绘制箭头
    virtual void paintEndPoint(QPainter* painter, const QStyleOptionGraphicsItem* option, const QPointF& p, const DANodeLinkPoint& pl);

    //生成箭头，所有生成的箭头都是尖朝上（↑），绘制的时候需要根据情况进行旋转
    virtual QPainterPath generateEndPointPainterPath(EndPointType epType, int size);

    //开始节点连接
    bool attachFrom(DAAbstractNodeGraphicsItem* item, const QString& name);
    bool attachFrom(DAAbstractNodeGraphicsItem* item, const DANodeLinkPoint& pl);
    //清空from节点
    void detachFrom();

    //结束节点连接
    bool attachTo(DAAbstractNodeGraphicsItem* item, const QString& name);
    bool attachTo(DAAbstractNodeGraphicsItem* item, const DANodeLinkPoint& pl);

    //清空to节点
    void detachTo();

    //已经连接完成，在from和to都有节点时，返回true
    bool isLinked() const;

protected:
    //
    void updateFromLinkPointInfo(const DANodeLinkPoint& pl);
    void updateToLinkPointInfo(const DANodeLinkPoint& pl);
    //添加事件处理
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;

    //连接的item在销毁，销毁过程对应的item会调用此函数，把link记录的item信息消除
    void callItemIsDestroying(DAAbstractNodeGraphicsItem* item, const DA::DANodeLinkPoint& pl);

private:
    //生成painterpath
    void generatePainterPath();
};
}  // end of namespace DA

namespace DA
{
// DAAbstractNodeLinkGraphicsItem::EndPointType的枚举转换
DAWORKFLOW_API QString enumToString(DA::DAAbstractNodeLinkGraphicsItem::EndPointType e);
// DAAbstractNodeLinkGraphicsItem::EndPointType的枚举转换
DAWORKFLOW_API DA::DAAbstractNodeLinkGraphicsItem::EndPointType stringToEnum(
        const QString& s,
        DA::DAAbstractNodeLinkGraphicsItem::EndPointType defaultEnum = DA::DAAbstractNodeLinkGraphicsItem::EndPointNone);
// DAAbstractNodeLinkGraphicsItem::LinkLineStyle的枚举转换
DAWORKFLOW_API QString enumToString(DA::DAAbstractNodeLinkGraphicsItem::LinkLineStyle e);
// DAAbstractNodeLinkGraphicsItem::LinkLineStyle的枚举转换
DAWORKFLOW_API DA::DAAbstractNodeLinkGraphicsItem::LinkLineStyle stringToEnum(
        const QString& s,
        DA::DAAbstractNodeLinkGraphicsItem::LinkLineStyle defaultEnum = DA::DAAbstractNodeLinkGraphicsItem::LinkLineKnuckle);

}  // end of namespace DA

#endif  // FCABSTRACTNODELINKGRAPHICSITEM_H
