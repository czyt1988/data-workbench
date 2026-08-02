# Plan-01: 新增 Overlay 基础设施

> **前置阅读**：先阅读 `REFACTOR_OVERLAY_PLAN.md`（总纲）
>
> **目标**：创建 `DAIResizableGraphicsItem` 接口和 `DAGraphicsResizeOverlayItem` 类，不影响现有功能
>
> **验证标准**：编译通过，新文件可被 CMake 自动拾取

---

## 涉及文件

| 文件 | 操作 |
|------|------|
| `src/DAGraphicsView/DAIResizableGraphicsItem.h` | **新增** |
| `src/DAGraphicsView/DAGraphicsResizeOverlayItem.h` | **新增** |
| `src/DAGraphicsView/DAGraphicsResizeOverlayItem.cpp` | **新增** |
| `src/DAGraphicsView/DAGraphicsViewGlobal.h` | **修改**（新增 ItemType） |

---

## 步骤 1: 新增 `DAIResizableGraphicsItem.h`

文件路径：`src/DAGraphicsView/DAIResizableGraphicsItem.h`

```cpp
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
```

## 步骤 2: 修改 `DAGraphicsViewGlobal.h`

在 `ItemType` 枚举中，`ItemType_DAGraphicsItem_Begin` 区域内新增：

```cpp
ItemType_DAGraphicsResizeOverlayItem = ItemType_DAGraphicsItem_Begin + 6,
```
<!-- 评审修复: B3 -->

**位置**：在 `ItemType_DAGraphicsMarkItem` 之后。阅读现有文件确认具体行号后插入。

## 步骤 3: 新增 `DAGraphicsResizeOverlayItem.h`

文件路径：`src/DAGraphicsView/DAGraphicsResizeOverlayItem.h`

```cpp
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
 * @note 当前实现仅支持单选时显示 Overlay。多选时不创建 Overlay，多选缩放为后续迭代功能。
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
        ControlPointLeftMid
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

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QPair<QPointF, QSizeF> computeResize(const QPointF& mouseLocalPos) const;
    ControlType hitTest(const QPointF& pos) const;
    static Qt::CursorShape controlTypeToCursor(ControlType ct);
    QSizeF clampSize(const QSizeF& s) const;
    QList<QRectF> getHandleRects() const;  // 评审修复: W1
};
}  // namespace DA
#endif  // DAGRAPHICSRESIZEOVERLAYITEM_H
```

## 步骤 4: 新增 `DAGraphicsResizeOverlayItem.cpp`

文件路径：`src/DAGraphicsView/DAGraphicsResizeOverlayItem.cpp`

### 4.1 PrivateData

```cpp
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
```

### 4.2 构造函数

```cpp
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
```

### 4.3 syncToTarget — 继承 target 变换

```cpp
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
```

### 4.4 boundingRect / shape / paint / getHandleRects

```cpp
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
```

### 4.5 hitTest — 纯函数命中测试

```cpp
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
```

### 4.6 controlTypeToCursor

```cpp
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
```

### 4.7 clampSize

```cpp
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
```

### 4.8 computeResize — 核心缩放计算

```cpp
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
```

### 4.9 事件处理

```cpp
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
```

### 4.10 其他公共方法

```cpp
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
```

## 验证

```powershell
.\scripts\build.ps1 -Target DAGraphicsView
```

编译通过即可。新文件由 CMake `file(GLOB)` 自动拾取，无需修改 CMakeLists.txt。
