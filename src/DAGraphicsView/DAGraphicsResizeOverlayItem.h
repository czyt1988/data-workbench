#ifndef DAGRAPHICSRESIZEOVERLAYITEM_H
#define DAGRAPHICSRESIZEOVERLAYITEM_H
#include "DAGraphicsViewGlobal.h"
#include <QGraphicsObject>
#include <QPair>
#include <QPointF>
#include <QSizeF>
#include <QList>
namespace DA
{
class DAIResizableGraphicsItem;
// 评审修复R3: S3 — 移除未使用的 DAGraphicsScene 前向声明（.cpp 中已 include）

/**
 * @brief 可缩放图元的 Overlay 覆盖层
 *
 * 独立的 QGraphicsObject，附着在目标 item 上方，提供 8 控制点缩放交互。
 * 由 DAGraphicsScene 在 item 被选中时创建，取消选中时销毁。
 *
 * 设计参考: QwtFigureWidgetOverlay
 * - 通过信号 requestResize 通知 Scene
 * - 控制点命中测试为纯函数
 * - 继承 target 的 pos/rotation/transformOriginPoint，旋转感知缩放自动正确
 *
 * @note 此 item 不参与序列化，不保存到 XML
 * @note 此 item 的 z-value 设置为场景最高值
 * @note 仅在单选可缩放图元时创建 Overlay（提供控制点交互）。多选时不创建 Overlay，
 *       选中边框由图元自身 paint() 绘制，确保用户能看到选中的图元。
 * @note 当前实现假设 target 为顶层 scene item（无 parent item）。非 top-level target 的支持为后续迭代功能。 <!-- 评审修复R3: W2 -->
 */
class DAGRAPHICSVIEW_API DAGraphicsResizeOverlayItem : public QGraphicsObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAGraphicsResizeOverlayItem)
public:
    enum ControlType
    {
        NotUnderAnyControlType = 0,
        ControlPointTopLeft,
        ControlPointTopMid,
        ControlPointTopRight,
        ControlPointRightMid,
        ControlPointBottomRight,
        ControlPointBottomMid,
        ControlPointBottomLeft,
        ControlPointLeftMid,
        RotationHandle  ///< 旋转控制点，位于 body 正上方
    };
    Q_ENUM(ControlType)

    explicit DAGraphicsResizeOverlayItem(DAIResizableGraphicsItem* target, QGraphicsItem* parent = nullptr);
    ~DAGraphicsResizeOverlayItem();

    enum { Type = DA::ItemType_DAGraphicsResizeOverlayItem };
    int type() const override { return Type; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    DAIResizableGraphicsItem* targetItem() const;
    bool isResizing() const;
    void setControlPointSize(const QSizeF& s);
    QSizeF controlPointSize() const;
    void syncToTarget();

public Q_SLOTS:
    void cancelResize();

Q_SIGNALS:
    void requestResize(DAIResizableGraphicsItem* target,
                       const QPointF& oldPos, const QSizeF& oldSize,
                       const QPointF& newPos, const QSizeF& newSize);

    void requestRotation(DAIResizableGraphicsItem* target,
                         qreal oldRotation, qreal newRotation);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QPair<QPointF, QSizeF> computeResize(const QPointF& mouseLocalPos) const;
    qreal computeRotation(const QPointF& mouseScenePos) const;  // 旋转角度计算（scene 坐标）
    ControlType hitTest(const QPointF& pos) const;
    static Qt::CursorShape controlTypeToCursor(ControlType ct);
    static qreal rotationHandleOffset(const QSizeF& cs);  // 旋转控制点距 body 上边的距离
    QSizeF clampSize(const QSizeF& s) const;
    QList<QRectF> getHandleRects() const;
};
}  // namespace DA
#endif  // DAGRAPHICSRESIZEOVERLAYITEM_H
