#include "DAGraphicsResizeOverlayItem.h"
#include "DAIResizableGraphicsItem.h"
#include "DAGraphicsScene.h"  // 评审修复R3: B2 — qobject_cast<DAGraphicsScene*> 需完整类定义
#include <QPainter>
#include <QSvgRenderer>
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

// 静态加载旋转光标 — 首次调用时从 SVG 资源渲染为 QPixmap 并构造 QCursor，后续直接返回缓存
static QCursor loadRotateCursor()
{
    static QCursor sRotateCursor = []() {
        const int sz = 24;
        QPixmap pm(sz, sz);
        pm.fill(Qt::transparent);
        QPainter painter(&pm);
        QSvgRenderer renderer(QStringLiteral(":/DAGraphicsView/svg/rotate.svg"));
        if (renderer.isValid()) {
            renderer.render(&painter, QRectF(0, 0, sz, sz));
        }
        return QCursor(pm, sz / 2, sz / 2);  // hotspot 设为图标中心
    }();
    return sRotateCursor;
}

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
    qreal   mPressRotation { 0 };  // target 的 rotation（按下时，旋转控制点用）
};

DAGraphicsResizeOverlayItem::PrivateData::PrivateData(DAGraphicsResizeOverlayItem* p) : q_ptr(p) {}

DAGraphicsResizeOverlayItem::DAGraphicsResizeOverlayItem(DAIResizableGraphicsItem* target, QGraphicsItem* parent)
    : QGraphicsObject(parent), DA_PIMPL_CONSTRUCT
{
    Q_ASSERT(target != nullptr);  // 评审修复R3: W3
    d_ptr->mTarget = target;
    // 在 target 上设置移动光标，使鼠标悬停在 body 边界区域时显示 SizeAllCursor
    // Overlay 的 shape() 仅覆盖控制点，不覆盖 body 区域，因此 body 上的光标由 target 决定
    target->graphicsItem()->setCursor(Qt::SizeAllCursor);
    setZValue(std::numeric_limits<qreal>::max());
    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, false);
    setAcceptHoverEvents(true);
}

DAGraphicsResizeOverlayItem::~DAGraphicsResizeOverlayItem()
{
    DA_D(d);
    if (d->mTarget) {
        d->mTarget->graphicsItem()->unsetCursor();
    }
}

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
    // 上边需额外膨胀以容纳旋转控制点及其连线
    qreal rotTop = rotationHandleOffset(d->mControlPointSize) + d->mControlPointSize.height() / 2 + 1;
    qreal topExtra = qMax(ho, rotTop);
    return body.adjusted(-wo, -topExtra, wo, ho);
}

// 旋转控制点距 body 上边的距离
qreal DAGraphicsResizeOverlayItem::rotationHandleOffset(const QSizeF& cs)
{
    return cs.height() + 6;
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
    // 旋转控制点：body 上边中点正上方
    qreal rotOff = rotationHandleOffset(cs);
    handles << QRectF(QPointF(body.center().x() - hw, body.top() - rotOff - hh), cs);      // RotationHandle
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
    // 选中边框由图元自身 paint() 绘制，Overlay 仅绘制控制点
    // 绘制 8 个控制点（评审修复: W1 — 使用 getHandleRects 共用计算）
    painter->setBrush(QColor(32, 128, 240));
    painter->setPen(QPen(QColor(128, 128, 147)));
    QList<QRectF> handles = getHandleRects();
    // 绘制 8 个缩放控制点（方形）
    for (int i = 0; i < 8; ++i) {
        painter->drawRect(handles[i]);
    }
    // 绘制旋转控制点（圆形 + 连线）
    QRectF rotHandle = handles.last();
    QPointF handleCenter = rotHandle.center();
    QPointF bodyTopMid(body.center().x(), body.top());
    painter->setPen(QPen(QColor(128, 128, 147), 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(bodyTopMid, handleCenter);
    painter->setBrush(QColor(32, 128, 240));
    painter->drawEllipse(handleCenter, rotHandle.width() / 2, rotHandle.height() / 2);
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
        ControlPointLeftMid,
        RotationHandle
    };
    for (int i = 0; i < handles.size(); ++i) {
        if (handles[i].contains(pos)) {
            return types[i];
        }
    }
    return NotUnderAnyControlType;
}

QCursor DAGraphicsResizeOverlayItem::controlTypeToCursor(ControlType ct)
{
    switch (ct) {
    case ControlPointTopLeft:
    case ControlPointBottomRight:
        return QCursor(Qt::SizeFDiagCursor);
    case ControlPointTopMid:
    case ControlPointBottomMid:
        return QCursor(Qt::SizeVerCursor);
    case ControlPointTopRight:
    case ControlPointBottomLeft:
        return QCursor(Qt::SizeBDiagCursor);
    case ControlPointRightMid:
    case ControlPointLeftMid:
        return QCursor(Qt::SizeHorCursor);
    case RotationHandle:
        return loadRotateCursor();  // 静态缓存的旋转光标
    default:
        return QCursor(Qt::ArrowCursor);
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

qreal DAGraphicsResizeOverlayItem::computeRotation(const QPointF& mouseScenePos) const
{
    DA_DC(d);
    // 使用 scene 坐标计算角度，避免 syncToTarget() 改变 overlay 旋转后
    // 局部坐标系偏移导致的反馈抖动
    // 旋转中心 = target 的 transformOriginPoint 映射到 scene 坐标
    QPointF originScene = d->mTarget->graphicsItem()->mapToScene(
        d->mTarget->getBodyTransformOriginPoint());
    // QLineF::angle(): 0°=右(东), 逆时针为正, 范围 [0,360)
    // QGraphicsItem::setRotation(): 0°=无旋转(正上方), 顺时针为正
    // 旋转控制点位于正上方时 angle=90°, 对应 rotation=0°
    QLineF line(originScene, mouseScenePos);
    if (line.length() < 1.0) {
        // 鼠标几乎在中心，保持当前角度
        return d->mTarget->graphicsItem()->rotation();
    }
    qreal angle = line.angle();  // 0=东, 90=北(上), 逆时针为正
    qreal newRotation = 90.0 - angle;
    // 规范到 [-180, 180]
    while (newRotation > 180.0) {
        newRotation -= 360.0;
    }
    while (newRotation < -180.0) {
        newRotation += 360.0;
    }
    return newRotation;
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
            d->mPressRotation = d->mTarget->graphicsItem()->rotation();
            event->accept();
            return;
        }
    }
    QGraphicsObject::mousePressEvent(event);
}

void DAGraphicsResizeOverlayItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    DA_D(d);
    if (d->mActiveControlType == RotationHandle) {
        // 旋转操作 — 使用 scene 坐标计算角度
        qreal newRotation = computeRotation(event->scenePos());
        // Shift 吸附 15° 倍数
        if (event->modifiers() & Qt::ShiftModifier) {
            newRotation = qRound(newRotation / 15.0) * 15.0;
        }
        d->mTarget->graphicsItem()->setRotation(newRotation);
        syncToTarget();
        event->accept();
        return;
    }
    if (d->mActiveControlType != NotUnderAnyControlType
        && d->mActiveControlType != RotationHandle) {  // 评审修复: W2
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
    if (d->mActiveControlType == RotationHandle) {
        qreal newRotation = d->mTarget->graphicsItem()->rotation();
        Q_EMIT requestRotation(d->mTarget, d->mPressRotation, newRotation);
        d->mActiveControlType = NotUnderAnyControlType;
        syncToTarget();
        event->accept();
        return;
    }
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
    if (d->mActiveControlType == RotationHandle) {
        // 恢复到按下时的旋转角度
        d->mTarget->graphicsItem()->setRotation(d->mPressRotation);
        d->mActiveControlType = NotUnderAnyControlType;
        syncToTarget();
        return;
    }
    if (d->mActiveControlType != NotUnderAnyControlType) {
        // 恢复到按下时的状态
        d->mTarget->graphicsItem()->setPos(d->mPressPos);
        d->mTarget->setBodySize(d->mPressSize);
        d->mActiveControlType = NotUnderAnyControlType;
        syncToTarget();
    }
}

}  // namespace DA
