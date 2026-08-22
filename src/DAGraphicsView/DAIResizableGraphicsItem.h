#ifndef DAIRESIZABLEGRAPHICSITEM_H
#define DAIRESIZABLEGRAPHICSITEM_H
#include "DAGraphicsViewGlobal.h"
#include <QSizeF>
#include <QRectF>
#include <QPointF>
class QGraphicsItem;
namespace DA {
/**
 * @brief 可缩放图元接口
 *
 * 任何需要被 DAGraphicsResizeOverlayItem 操作的 QGraphicsItem 都应实现此接口。
 * 无需继承特定基类即可获得缩放能力。
 */
class DAGRAPHICSVIEW_API DAIResizableGraphicsItem
{
public:
    virtual ~DAIResizableGraphicsItem() = default;
    // 获取/设置 body 尺寸
    virtual QSizeF getBodySize() const = 0;
    virtual void setBodySize(const QSizeF& s) = 0;
    // 获取 body 矩形（item 坐标系）
    virtual QRectF getBodyRect() const = 0;
    // 最小/最大尺寸
    virtual QSizeF getBodyMinimumSize() const = 0;
    virtual QSizeF getBodyMaximumSize() const = 0;
    // 是否允许缩放
    virtual bool isResizable() const = 0;
    // 获取对应的 QGraphicsItem 指针
    virtual QGraphicsItem* graphicsItem() = 0;
    virtual const QGraphicsItem* graphicsItem() const = 0;
    // 注意：getBodyPainterStartPos() 不在接口中——Overlay 通过 getBodyRect().topLeft() 获取 body 起始位置，无需单独方法。 <!-- 评审修复R4: W2 -->
    // 变换原点（用于旋转中心），Overlay 在 syncToTarget() 中调用以继承 target 的旋转中心 <!-- 评审修复R4: S1/S2 -->
    virtual QPointF getBodyTransformOriginPoint() const = 0;
};
}  // namespace DA
#endif  // DAIRESIZABLEGRAPHICSITEM_H
