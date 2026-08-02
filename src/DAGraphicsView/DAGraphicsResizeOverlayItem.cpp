#include "DAGraphicsResizeOverlayItem.h"
#include "DAIResizableGraphicsItem.h"
#include "DAGraphicsScene.h"  // 评审修复R3: B2 — qobject_cast<DAGraphicsScene*> 需完整类定义
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainterPath>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QList>   // 评审修复: S4
#include <limits>
namespace DA
{

class DAGraphicsResizeOverlayItem::PrivateData
{
    DA_DECLARE_PUBLIC(DAGraphicsResizeOverlayItem)
public:
    PrivateData(DAGraphicsResizeOverlayItem* p);
    DAIResizableGraphicsItem* mTarget { nullptr };
    // 评审修复: W2 — 分离 hover 和 active 状态
    ControlType mHoverControlType { NotUnderAnyControlType };   // hover 状态下的控制点
    ControlType mActiveControlType { NotUnderAnyControlType };  // 活跃缩放的控制点
    QSizeF mControlPointSize { 10, 10 };
    QRectF mTargetBodyRect;  // 缓存 target 的 bodyRect（overlay 局部坐标）
    // 鼠标按下时记录的状态
    QPointF mPressPos;       // target 的 pos（按下时）
    QSizeF  mPressSize;      // target 的 bodySize（按下时）
};

DAGraphicsResizeOverlayItem::PrivateData::PrivateData(DAGraphicsResizeOverlayItem* p) : q_ptr(p) {}

DAGraphicsResizeOverlayItem::DAGraphicsResizeOverlayItem(DAIResizableGraphicsItem* target, QGraphicsItem* parent)
    : QGraphicsObject(parent), DA_PIMPL_CONSTRUCT
{
    Q_ASSERT(target != nullptr);  // 评审修复R3: W3
    d_ptr->mTarget = target;
    setZValue(std::numeric_limits<qreal>::max());
    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, false);
    setAcceptHoverEvents(true);
}

DAGraphicsResizeOverlayItem::~DAGraphicsResizeOverlayItem() {}

void DAGraphicsResizeOverlayItem::syncToTarget()
{
    DA_D(d);
    if (!d->mTarget) return;  // 评审修复R3: W3
    // TODO: 性能优化 — 可将 syncToTarget() 拆分为 syncTransform()（初始化时调用）和 syncBodyRect()（拖拽中仅同步 bodyRect 和 pos） <!-- 评审修复R3: S2 -->
    QGraphicsItem* target = d->mTarget->graphicsItem();
    prepareGeometryChange();
    setPos(target->pos());
    setRotation(target->rotation());
    setTransformOriginPoint(d->mTarget->getBodyTransformOriginPoint());  // 评审修复: S2
    d->mTargetBodyRect = d->mTarget->getBodyRect();
    update();
}

QRectF DAGraphicsResizeOverlayItem::boundingRect() const
{
    DA_DC(d);
    QRectF body = d->mTargetBodyRect;
    qreal wo = d->mControlPointSize.width() / 2 + 1;
    qreal ho = d->mControlPointSize.height() / 2 + 1;
    return body.adjusted(-wo, -ho, wo, ho);
}

// 评审修复: W1 — 提取共用辅助函数，paint() 和 hitTest() 共用同一组矩形
QList<QRectF> DAGraphicsResizeOverlayItem::getHandleRects() const
{
    DA_DC(d);
    QRectF body = d->mTargetBodyRect;
    QSizeF cs = d->mControlPointSize;
    qreal hw = cs.width() / 2;
    qreal hh = cs.height() / 2;
    QList<QRectF> handles;
    handles << QRectF(body.topLeft() - QPointF(hw, hh), cs)                                // TopLeft
            << QRectF(QPointF(body.center().x() - hw, body.top() - hh), cs)               // TopMid
            << QRectF(QPointF(body.right() - hw, body.top() - hh), cs)                    // TopRight
            << QRectF(QPointF(body.right() - hw, body.center().y() - hh), cs)             // RightMid
            << QRectF(QPointF(body.right() - hw, body.bottom() - hh), cs)                  // BottomRight
            << QRectF(QPointF(body.center().x() - hw, body.bottom() - hh), cs)            // BottomMid
            << QRectF(QPointF(body.left() - hw, body.bottom() - hh), cs)                 // BottomLeft
            << QRectF(QPointF(body.left() - hw, body.center().y() - hh), cs);             // LeftMid
    return handles;
}

// 评审修复: B2 — shape() 仅包含 8 个控制点的命中矩形，不包含 body 区域
QPainterPath DAGraphicsResizeOverlayItem::shape() const
{
    DA_DC(d);
    QPainterPath path;
    QList<QRectF> handles = getHandleRects();
    for (const QRectF& r : std::as_const(handles)) {
        path.addRect(r);
    }
    return path;
}

void DAGraphicsResizeOverlayItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    DA_D(d);
    if (!d->mTarget) return;  // 评审修复R3: W3
    QRectF body = d->mTargetBodyRect;
    // 绘制选中边框（虚线）
    QPen borderPen(QColor(32, 128, 240));
    borderPen.setStyle(Qt::DashLine);
    painter->setPen(borderPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(body);

    // 绘制 8 个控制点（评审修复: W1 — 使用 getHandleRects 共用计算）
    painter->setBrush(QColor(32, 128, 240));
    painter->setPen(QPen(QColor(128, 128, 147)));
    QList<QRectF> handles = getHandleRects();
    for (const QRectF& r : std::as_const(handles)) {
        painter->drawRect(r);
    }
}

DAGraphicsResizeOverlayItem::ControlType DAGraphicsResizeOverlayItem::hitTest(const QPointF& pos) const
{
    DA_DC(d);
    // 评审修复: W1 — 使用 getHandleRects 共用计算，确保与 paint() 一致
    QList<QRectF> handles = getHandleRects();
    static const ControlType types[] = {
        ControlPointTopLeft,
        ControlPointTopMid,
        ControlPointTopRight,
        ControlPointRightMid,
        ControlPointBottomRight,
        ControlPointBottomMid,
        ControlPointBottomLeft,
        ControlPointLeftMid
    };
    for (int i = 0; i < handles.size(); ++i) {
        if (handles[i].contains(pos)) {
            return types[i];
        }
    }
    return NotUnderAnyControlType;
}

Qt::CursorShape DAGraphicsResizeOverlayItem::controlTypeToCursor(ControlType ct)
{
    switch (ct) {
    case ControlPointTopLeft:
    case ControlPointBottomRight:
        return Qt::SizeFDiagCursor;
    case ControlPointTopMid:
    case ControlPointBottomMid:
        return Qt::SizeVerCursor;
    case ControlPointTopRight:
    case ControlPointBottomLeft:
        return Qt::SizeBDiagCursor;
    case ControlPointRightMid:
    case ControlPointLeftMid:
        return Qt::SizeHorCursor;
    default:
        return Qt::ArrowCursor;
    }
}

QSizeF DAGraphicsResizeOverlayItem::clampSize(const QSizeF& s) const
{
    DA_DC(d);
    QSizeF result = s;
    QSizeF minSize = d->mTarget->getBodyMinimumSize();
    QSizeF maxSize = d->mTarget->getBodyMaximumSize();
    if (result.width() < minSize.width()) result.setWidth(minSize.width());
    if (result.height() < minSize.height()) result.setHeight(minSize.height());
    if (result.width() > maxSize.width()) result.setWidth(maxSize.width());
    if (result.height() > maxSize.height()) result.setHeight(maxSize.height());
    return result;
}

QPair<QPointF, QSizeF> DAGraphicsResizeOverlayItem::computeResize(const QPointF& mouseLocalPos) const
{
    DA_DC(d);
    QRectF bodyRect = d->mTargetBodyRect;

    QPointF newTopLeft = bodyRect.topLeft();
    QPointF newBottomRight = bodyRect.bottomRight();

    switch (d->mActiveControlType) {  // 评审修复: W2
    case ControlPointTopLeft:
        newTopLeft = mouseLocalPos;
        break;
    case ControlPointBottomRight:
        newBottomRight = mouseLocalPos;
        break;
    case ControlPointTopRight:
        newTopLeft.setY(mouseLocalPos.y());
        newBottomRight.setX(mouseLocalPos.x());
        break;
    case ControlPointBottomLeft:
        newTopLeft.setX(mouseLocalPos.x());
        newBottomRight.setY(mouseLocalPos.y());
        break;
    case ControlPointTopMid:
        newTopLeft.setY(mouseLocalPos.y());
        break;
    case ControlPointBottomMid:
        newBottomRight.setY(mouseLocalPos.y());
        break;
    case ControlPointLeftMid:
        newTopLeft.setX(mouseLocalPos.x());
        break;
    case ControlPointRightMid:
        newBottomRight.setX(mouseLocalPos.x());
        break;
    default:
        break;
    }

    // 计算原始尺寸
    QSizeF rawSize(newBottomRight.x() - newTopLeft.x(),
                   newBottomRight.y() - newTopLeft.y());

    // 如果尺寸为负（鼠标拖到对角线另一侧），保持原方向不变
    // clampSize 会处理 min/max
    QSizeF newSize = clampSize(rawSize);

    // 网格对齐（评审修复: W3）— 必须在 adjustedTopLeft 计算之前执行 <!-- 评审修复R3: W1 -->
    // 评审修复R3: B1 — getGridSize() 返回 QSize 而非 qreal，需分别处理 width/height
    {
        QGraphicsScene* sc = scene();
        if (auto* daScene = qobject_cast<DAGraphicsScene*>(sc)) {
            if (daScene->isEnableSnapToGrid()) {
                QSize grid = daScene->getGridSize();
                if (grid.width() > 0) {
                    newSize.setWidth(qRound(newSize.width() / grid.width()) * grid.width());
                }
                if (grid.height() > 0) {
                    newSize.setHeight(qRound(newSize.height() / grid.height()) * grid.height());
                }
            }
        }
    }

    // clamp + 网格对齐后需要调整对角位置，保持对角不动
    // 根据控制点类型，调整 newTopLeft
    QPointF adjustedTopLeft = newTopLeft;

    // Y 调整：顶边移动的控制点，需保持底边固定 <!-- 评审修复R4: W1 -->
    switch (d->mActiveControlType) {
    case ControlPointTopLeft:
    case ControlPointTopMid:
    case ControlPointTopRight:   // 新增
        adjustedTopLeft.setY(newBottomRight.y() - newSize.height());
        break;
    default:
        break;
    }

    // X 调整：左边移动的控制点，需保持右边固定 <!-- 评审修复R4: W1 -->
    switch (d->mActiveControlType) {
    case ControlPointTopLeft:
    case ControlPointLeftMid:
    case ControlPointBottomLeft:  // 新增
        adjustedTopLeft.setX(newBottomRight.x() - newSize.width());
        break;
    default:
        break;
    }

    // 计算新的 pos（parent 坐标系）
    // 评审修复: B1 — 使用 mapToParent 将局部 delta 转换为 parent delta（考虑旋转）
    QPointF currentTargetPos = d->mTarget->graphicsItem()->pos();
    QPointF oldTopLeftParent = mapToParent(bodyRect.topLeft());
    QPointF newTopLeftParent = mapToParent(adjustedTopLeft);
    QPointF deltaParent = newTopLeftParent - oldTopLeftParent;
    QPointF newPos = currentTargetPos + deltaParent;

    return qMakePair(newPos, newSize);
}

void DAGraphicsResizeOverlayItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    DA_D(d);
    ControlType ct = hitTest(event->pos());
    if (ct != NotUnderAnyControlType) {
        setCursor(controlTypeToCursor(ct));
    } else {
        unsetCursor();
    }
    d->mHoverControlType = ct;  // 评审修复: W2
    event->accept();
}

void DAGraphicsResizeOverlayItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    unsetCursor();
    DA_D(d);
    d->mHoverControlType = NotUnderAnyControlType;  // 评审修复: W2
    event->accept();
}

void DAGraphicsResizeOverlayItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    DA_D(d);
    if (event->button() == Qt::LeftButton) {
        d->mActiveControlType = hitTest(event->pos());  // 评审修复: W2
        if (d->mActiveControlType != NotUnderAnyControlType) {
            // 记录按下状态
            d->mPressPos = d->mTarget->graphicsItem()->pos();
            d->mPressSize = d->mTarget->getBodySize();
            event->accept();
            return;
        }
    }
    QGraphicsObject::mousePressEvent(event);
}

void DAGraphicsResizeOverlayItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    DA_D(d);
    if (d->mActiveControlType != NotUnderAnyControlType) {  // 评审修复: W2
        // 实时修改目标
        auto result = computeResize(event->pos());
        d->mTarget->graphicsItem()->setPos(result.first);
        d->mTarget->setBodySize(result.second);
        syncToTarget();
        event->accept();
        return;
    }
    QGraphicsObject::mouseMoveEvent(event);
}

// 评审修复: W5 — 直接读取 target 实际状态，确保 undo/redo 一致
void DAGraphicsResizeOverlayItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    DA_D(d);
    if (d->mActiveControlType != NotUnderAnyControlType) {  // 评审修复: W2
        // 直接读取 target 的实际当前状态，确保 undo/redo 一致
        Q_EMIT requestResize(d->mTarget, d->mPressPos, d->mPressSize,
                             d->mTarget->graphicsItem()->pos(), d->mTarget->getBodySize());
        d->mActiveControlType = NotUnderAnyControlType;
        syncToTarget();
        event->accept();
        return;
    }
    QGraphicsObject::mouseReleaseEvent(event);
}

DAIResizableGraphicsItem* DAGraphicsResizeOverlayItem::targetItem() const
{
    DA_DC(d);
    return d->mTarget;
}

// 评审修复: W2 — 检查 mActiveControlType 而非 mControlType
bool DAGraphicsResizeOverlayItem::isResizing() const
{
    DA_DC(d);
    return d->mActiveControlType != NotUnderAnyControlType;
}

void DAGraphicsResizeOverlayItem::setControlPointSize(const QSizeF& s)
{
    DA_D(d);
    prepareGeometryChange();
    d->mControlPointSize = s;
    update();
}

QSizeF DAGraphicsResizeOverlayItem::controlPointSize() const
{
    DA_DC(d);
    return d->mControlPointSize;
}

// 评审修复: W6 — 恢复到按下时的状态，支持 ESC 键取消
// @note 此方法由 DAGraphicsScene 在收到 ESC 键事件时调用，Overlay 自身不处理键盘事件。 <!-- 评审修复R3: S1 -->
void DAGraphicsResizeOverlayItem::cancelResize()
{
    DA_D(d);
    if (d->mActiveControlType != NotUnderAnyControlType) {
        // 恢复到按下时的状态
        d->mTarget->graphicsItem()->setPos(d->mPressPos);
        d->mTarget->setBodySize(d->mPressSize);
        d->mActiveControlType = NotUnderAnyControlType;
        syncToTarget();
    }
}

}  // namespace DA
