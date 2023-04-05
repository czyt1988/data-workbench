#ifndef DAGRAPHICSRESIZEABLEITEMDRAWHELPER_H
#define DAGRAPHICSRESIZEABLEITEMDRAWHELPER_H
#include "DAWorkFlowGlobal.h"
#include <QGraphicsItem>
#include <memory>
#include "DANodePalette.h"
class QGraphicsSceneHoverEvent;
DA_IMPL_FORWARD_DECL(DAGraphicsResizeableItemDrawHelper)
/**
 * @brief 辅助绘制缩放框的类
 */
class DAWORKFLOW_API DAGraphicsResizeableItemDrawHelper
{
    DA_IMPL(DAGraphicsResizeableItemDrawHelper)
public:
    DAGraphicsResizeableItemDrawHelper(QGraphicsItem* i, bool on = true);
    virtual ~DAGraphicsResizeableItemDrawHelper();
    //传入原来的boundingRect，得到需要调整尺寸的boundingRect
    virtual QRectF boundingRect(const QRectF& originBRect) const;
    //会在传入的shape加上control rect，前提是boundingRect已经计算好
    virtual QPainterPath shape(const QPainterPath& originShape) const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    virtual void paintResizeBorder(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    virtual void paintResizeControlPoints(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
    QGraphicsItem* graphicsItem();
    //设置控制器的大小
    void setControlerSize(const QSize& s);
    QSize getControlerSize() const;
    //控制点的偏移
    int getControlOffset() const;
    // palette设置
    void setNodePalette(const DANodePalette& pl);
    const DANodePalette& getNodePalette() const;
    //
    void graphicsItemBoundingRectChanged(const QRectF& originBRect);
    //是否允许
    void setEnable(bool on);
    bool isEnable() const;
};

#endif  // DAGRAPHICSRESIZEABLEITEMDRAWHELPER_H
