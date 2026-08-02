# Plan-02: 精简 DAGraphicsResizeableItem

> **前置阅读**：先阅读 `REFACTOR_OVERLAY_PLAN.md`（总纲），再阅读 `plan-01.md`
>
> **目标**：让 `DAGraphicsResizeableItem` 实现 `DAIResizableGraphicsItem` 接口，移除控制点/鼠标交互代码
>
> **验证标准**：DAGraphicsView 模块编译通过（含对 DAGraphicsScene.cpp 控制点检测代码和 DAGraphicsTextItem.cpp isResizing() 调用的临时适配）<!-- 评审修复: W4 -->
>
> ⚠️ Overlay 的 computeResize 需要在 plan-01 中补充网格对齐逻辑（已在 plan-01 评审 W3 中修复），否则缩放尺寸不再吸附网格。<!-- 评审修复: W5 -->

---

## 涉及文件

| 文件 | 操作 |
|------|------|
| `src/DAGraphicsView/DAGraphicsResizeableItem.h` | **精简**：移除控制点相关声明，添加接口继承 |
| `src/DAGraphicsView/DAGraphicsResizeableItem.cpp` | **精简**：移除控制点实现，添加接口实现 |
| `src/DAGraphicsView/DAGraphicsScene.cpp` | **临时适配**：注释掉 ControlType/getControlPointByPos() 引用，确保编译通过（plan-03 进一步处理） |<!-- 评审修复: B1 -->
| `src/DAGraphicsView/DAGraphicsTextItem.cpp` | **临时适配**：将 isResizing() 调用替换为 if (true)（plan-05 正式处理） |<!-- 评审修复R4: W1 -->

---

## 设计原则

1. **保留** body 尺寸管理（`get/setBodySize`, `getBodyRect`, min/max）
2. **保留** `paintBody` 纯虚函数、`paintBackground`/`paintBorder` 虚函数
3. **保留** XML 序列化 (`saveToXml`/`loadFromXml`)
4. **保留** `itemChange` 中的网格对齐 + rotation 信号转发
5. **保留** `friend class DAGraphicsResizeableItem` 在 Scene 中（信号调用链依赖）
6. **移除** 控制点绘制、命中测试、鼠标交互、缩放数学
7. **移除** `DAGraphicsResizeableItemPalette`（移至 Overlay）
8. **移除** `ControlType` 枚举（移至 Overlay）
9. **精简** `boundingRect()`/`shape()`：不再包含控制点膨胀区域
10. **新增** `public DAIResizableGraphicsItem` 继承 + 接口实现

---

## 步骤 1: 修改 `DAGraphicsResizeableItem.h`

### 1.1 添加接口继承

在类声明中添加 `public DAIResizableGraphicsItem`：

```cpp
#include "DAIResizableGraphicsItem.h"

class DAGRAPHICSVIEW_API DAGraphicsResizeableItem : public DAGraphicsItem, public DAIResizableGraphicsItem
```

### 1.2 移除以下声明

- `DAGraphicsResizeableItemPalette` 类（整个类定义）
- `Q_GLOBAL_STATIC(DA::DAGraphicsResizeableItemPalette, daGlobalGraphicsResizeableItemPalette)` 行
- `ControlType` 枚举（整个枚举 + `Q_ENUM`）。控制线（ControlLine*）功能由 Overlay 架构下的 body 默认移动行为替代——点击 body 区域直接传播到 item 触发移动。<!-- 评审修复R3: S5 -->
- `testBodySize(const QSizeF& s)` 公共方法（移除公共 testBodySize 方法，setBodySize 改为直接调用 d_ptr->testBodySize()）<!-- 评审修复R4: S3 -->
- `doItemResize(const QPointF& mousescenePos)` 公共方法
- `paintSelectedBorder()` 虚函数
- `paintResizeControlPoints()` 虚函数
- `controlPointRect()` 虚函数
- `controlPointSize()` 虚函数（改为非虚，仅返回 mControlPointSize）
- `prepareControlInfoChange()` 方法
- `getControlPointByPos()` 方法
- `isResizing()` 方法
- ~~`isSnapToGrid()` / `getGridSize()` 方法~~ **保留**（`adjustPosToGrid` 依赖）<!-- 评审修复: W1 -->
- `hoverEnterEvent` / `hoverMoveEvent` / `hoverLeaveEvent` 重写声明
- `mousePressEvent` / `mouseReleaseEvent` / `mouseMoveEvent` 重写声明
- `#if DA_USE_QGRAPHICSOBJECT` 块中的 `itemBodySizeChanged` 信号

### 1.3 保留以下声明<!-- 评审修复R4: W2 -->

#### 接口方法（添加 override）

以下 6 个方法在 `DAIResizableGraphicsItem` 接口中声明，需添加 `override`：

- `setBodySize(const QSizeF& s)` — virtual override
- `getBodySize() const` — override
- `getBodyRect() const` — override
- `setBodyMinimumSize()` / `setBodyMaximumSize()` / getter — override
- `isResizable()` — override

#### 其他保留方法（不变）

以下方法不在接口中，保持原有声明不变：

- `getBodyControlRect() const` — `Q_DECL_DEPRECATED_X("Use getBodyRect() instead")` <!-- 评审修复: S3 -->
- `setControlerSize()` / `getControlerSize()` — `Q_DECL_DEPRECATED_X("Use getBodyRect() instead")` <!-- 评审修复: S3 -->
- `setEnableResize()`
- `setBodyPos()` / `setBodyScenePos()` / `getBodyCenterPoint()` / `getBodyCenterPos()` / `setBodyCenterPos()`
- `setAutoCenterTransformOriginPoint()` / `updateTransformOriginPoint()`
- `paintBody()` 纯虚函数
- `getBodyShape()` 虚函数
- `paintBackground()` / `paintBorder()` 虚函数
- `paint()` / `boundingRect()` / `shape()` / `saveToXml()` / `loadFromXml()` / `itemChange()` 重写
- `changeBodySize()` protected 方法
- `isSnapToGrid()` / `getGridSize()` — 保留（`adjustPosToGrid` 依赖） <!-- 评审修复: W1, S1 -->
- `getBodyPainterStartPos()` — `getBodyPainterStartPos()` 不在 `DAIResizableGraphicsItem` 接口中，是 `DAGraphicsResizeableItem` 的内部方法，不添加 `override`<!-- 评审修复R4: B1 -->

### 1.4 新增接口实现声明

```cpp
public:
    // DAIResizableGraphicsItem 接口实现
    // getBodyPainterStartPos() 不在接口中，已移至 §1.3 普通方法声明<!-- 评审修复R4: B1 -->
    QPointF getBodyTransformOriginPoint() const override;
    QGraphicsItem* graphicsItem() override;
    const QGraphicsItem* graphicsItem() const override;
```

## 步骤 2: 修改 `DAGraphicsResizeableItem.cpp`

### 2.1 移除以下实现

- `DAGraphicsResizeableItemControlPointInfo` 类（整个内部类）
- `PrivateData` 中的 `mCurrentControlTypeUnderMouse`、`mMousePressMouseOnScenePos`、`mMousePressItemPos`、`mMousePressItemSize`、`mControlPointInfos`、`mPalette` 成员
- `PrivateData::testBodySize` — 保留（被 `setBodySize` 使用）
- `PrivateData::getPalette` — 移除
- `PrivateData::resetResizeableItemControlPointInfo` — 移除
- `PrivateData::appendControlPointInfo` — 移除
- `PrivateData::getControlPointAndUpdateByPos` — 移除
- `PrivateData::doResize` — 移除（整个方法，约 200 行）
- `PrivateData::bodyConnerPoint` — 移除
- `PrivateData::adjustPosToGrid` — **保留**（完整保留，不简化。itemChange 中网格对齐使用。Overlay 使用局部坐标系，旋转下的缩放网格对齐在 plan-01 的 computeResize 中处理）<!-- 评审修复: W3 -->
- `PrivateData::adjustSizeToGrid` — 移除（Overlay 负责）
- `hoverEnterEvent` / `hoverMoveEvent` / `hoverLeaveEvent` 实现
- `mousePressEvent` / `mouseMoveEvent` / `mouseReleaseEvent` 实现
- `setEnableResize()` 中的 `setAcceptHoverEvents(on)` 调用块（line 1009-1018，与新架构矛盾，hover 事件由 Overlay 处理）<!-- 评审修复R3: W3 -->
- `doItemResize` 实现
- `paintSelectedBorder` 实现
- `paintResizeControlPoints` 实现
- `controlPointRect` 实现
- `controlPointSize` 实现（改为简单返回 `d_ptr->mControlPointSize`）
- `prepareControlInfoChange` 实现
- `getControlPointByPos` 实现
- `isResizing` 实现
- ~~`isSnapToGrid` / `getGridSize` 实现（移至 Overlay）~~ **保留**（`adjustPosToGrid` 依赖）<!-- 评审修复: W1 -->
- 调试宏 `DAGraphicsResizeableItemPrivateDoResizePrint` / `DAGraphicsResizeableItemPrivatePrint` 及 `Enable_DAGraphicsResizeableItemPrivateDebugPrint`
- 移除 `#include "DACommandsForGraphics.h"` 和 `#include "DAGraphicsCommandsFactory.h"`（仅在已移除的 mouseReleaseEvent 中使用）<!-- 评审修复R3: S2 -->

### 2.2 精简 `paint()`

```cpp
void DAGraphicsResizeableItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    QRectF bodyrect = getBodyRect();
    paintBackground(painter, option, widget, bodyrect);
    paintBorder(painter, option, widget, bodyrect);
    paintBody(painter, option, widget, bodyrect);
    // 非可缩放图元的选中边框（Overlay 仅对可缩放图元创建）
    if (!isResizable() && isSelected()) {
        QPen pen(QColor(32, 128, 240));
        pen.setStyle(Qt::DashLine);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(bodyrect);
    }
    // 不再绘制控制点和选中边框 — 由 Overlay 负责（可缩放图元）
}
```
<!-- 评审修复R3: W1 -->

### 2.3 精简 `boundingRect()`

```cpp
QRectF DAGraphicsResizeableItem::boundingRect() const
{
    return getBodyRect();  // 不再膨胀控制点区域
}
```

### 2.4 精简 `shape()` / `getBodyShape()`

<!-- 评审修复: S4 -->
// 经确认 DAPyNodeGraphicsItem::shape() 使用的是 getBodyControlRect() 而非 getBodyShape()，
// 精简后的 getBodyShape() 不影响子类 override。

```cpp
QPainterPath DAGraphicsResizeableItem::shape() const
{
    return getBodyShape();
}

QPainterPath DAGraphicsResizeableItem::getBodyShape() const
{
    QPainterPath p;
    p.addRect(getBodyRect());  // 不再用 getBodyControlRect()
    return p;
}
```

### 2.5 精简 `setBodyPos()` / `setBodyScenePos()`

移除控制点 offset 逻辑：

```cpp
void DAGraphicsResizeableItem::setBodyPos(const QPointF& p)
{
    setPos(p);
}

void DAGraphicsResizeableItem::setBodyScenePos(const QPointF& p)
{
    setScenePos(p);
}
```

### 2.6 精简 `getBodyControlRect()`

<!-- 评审修复: S3 -->
<!-- 评审修复R3: S3 -->
```cpp
QRectF DAGraphicsResizeableItem::getBodyControlRect() const
{
    return getBodyRect();  // 不再膨胀，等价于 getBodyRect()
}
```

### 2.7 精简构造函数

<!-- 评审修复: S2 -->
移除 `prepareControlInfoChange()` 调用，移除 `d_ptr->mMousePressItemSize` / `mMousePressItemPos` 初始化。
移除 `setAcceptHoverEvents(true)`（hover 事件由 Overlay 处理）。
注意：如果子类（如 DAGraphicsTextItem）需要 hover 事件，在子类中自行调用 `setAcceptHoverEvents(true)`。

```cpp
DAGraphicsResizeableItem::DAGraphicsResizeableItem(QGraphicsItem* parent) : DAGraphicsItem(parent), DA_PIMPL_CONSTRUCT
{
    setFlags(flags() | ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
    // setAcceptHoverEvents 已移除，hover 事件由 Overlay 处理
}
```

### 2.8 精简 `itemChange()`

<!-- 评审修复: S5 -->
保留网格对齐和 rotation 信号转发，移除其他。
代码风格统一使用 `d_ptr->` 而非 `DA_D(d)` + `d->`，与现有 itemChange 代码保持一致。

```cpp
QVariant DAGraphicsResizeableItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case QGraphicsItem::ItemPositionChange: {
        if (isSceneReadOnly()) {
            return pos();
        }
        QPointF newPos = value.toPointF();
        // 网格对齐逻辑保留
        d_ptr->adjustPosToGrid(newPos);
        return newPos;
    }
    case QGraphicsItem::ItemRotationHasChanged: {
        if (d_ptr->mSceneUndo) {
            d_ptr->mSceneUndo->emitItemRotationChanged(this, rotation());
        }
        break;
    }
    case QGraphicsItem::ItemSceneHasChanged: {
        d_ptr->mSceneUndo = qobject_cast<DAGraphicsScene*>(scene());
        break;
    }
    default:
        break;
    }
    return DAGraphicsItem::itemChange(change, value);
}
```

### 2.9 精简 `setBodySize()`

移除 `#if DA_USE_QGRAPHICSOBJECT` 信号发射块（`itemBodySizeChanged` 信号已从头文件移除）：

```cpp
void DAGraphicsResizeableItem::setBodySize(const QSizeF& s)
{
    QSizeF cs = d_ptr->testBodySize(s);
    if (cs != d_ptr->mSize) {
        QSizeF oldsize = d_ptr->mSize;
        prepareGeometryChange();
        changeBodySize(cs);
        if (d_ptr->mSceneUndo) {
            d_ptr->mSceneUndo->emitItemBodySizeChanged(this, oldsize, d_ptr->mSize);
        }
    }
}
```
<!-- 评审修复R3: S1 -->

### 2.10 精简 `PrivateData`

保留的成员：
```cpp
bool mEnableResize { true };
bool mAutoCenterTransformOriginPoint { true };
DAGraphicsScene* mSceneUndo { nullptr };
QSizeF mSize { 30, 30 };
QSizeF mMinSize { 5, 5 };
QSizeF mMaxSize { 9999, 9999 };
QPointF mPainterRectStartPos { 0, 0 };
QSizeF mControlPointSize { 10, 10 };  // 保留但 deprecated
```

### 2.11 新增接口实现

```cpp
QPointF DAGraphicsResizeableItem::getBodyPainterStartPos() const
{
    return d_ptr->mPainterRectStartPos;  //<!-- 评审修复R4: S1 -->
}

QPointF DAGraphicsResizeableItem::getBodyTransformOriginPoint() const
{
    return transformOriginPoint();
}

QGraphicsItem* DAGraphicsResizeableItem::graphicsItem()
{
    return this;
}

const QGraphicsItem* DAGraphicsResizeableItem::graphicsItem() const
{
    return this;
}
```

### 2.12 修改 `changeBodySize()` — 移除 `prepareControlInfoChange()` 调用

<!-- 评审修复: B2 -->
`changeBodySize()` 内部调用了已移除的 `prepareControlInfoChange()`，需删除该调用：

```cpp
void DAGraphicsResizeableItem::changeBodySize(const QSizeF& s)
{
    if (s != d_ptr->mSize) {
        d_ptr->mSize = s;
        updateTransformOriginPoint();
        // prepareControlInfoChange() 已移除
    }
}
```

### 2.13 修改 `setControlerSize()` — 移除 `prepareControlInfoChange()` 调用

<!-- 评审修复: B3 -->
`setControlerSize()` 内部调用了已移除的 `prepareControlInfoChange()`，需删除该调用，替换为 `update()`：

```cpp
void DAGraphicsResizeableItem::setControlerSize(const QSizeF& s)
{
    // 移除赋值前的 update()，仅保留赋值后的 update()<!-- 评审修复R4: S2 -->
    d_ptr->mControlPointSize = s;
    update();
    // prepareControlInfoChange() 已移除
}
```

### 2.14 修改 `setEnableResize()` — 移除 `setAcceptHoverEvents(on)` 调用

<!-- 评审修复R3: W3 -->
`setEnableResize()` (line 1009-1018) 内部调用了 `setAcceptHoverEvents(on)`，与 §2.7 从构造函数移除 hover 事件的新架构矛盾。需删除该调用：

```cpp
void DAGraphicsResizeableItem::setEnableResize(bool on)
{
    d_ptr->mEnableResize = on;
    update();  // 刷新选中边框显示<!-- 评审修复R4: W3 -->
    // setAcceptHoverEvents(on) 已移除，hover 事件由 Overlay 处理
}
```

### 2.15 适配 `DAGraphicsScene.cpp` — 移除 ControlType/getControlPointByPos() 引用

<!-- 评审修复: B1 -->
plan-02 移除了 `ControlType` 枚举和 `getControlPointByPos()` 方法，但同模块的 `DAGraphicsScene.cpp` 引用了这些符号。
plan-03 负责修改 Scene 但在 plan-02 之后执行，导致 plan-02 声称的"DAGraphicsView 模块编译通过"无法达成。
此处先注释掉这些控制点检测代码块确保编译通过，plan-03 中会被进一步处理。

具体修改位置：

1. **`isItemCanMove()` 约 1043-1050 行**：注释掉使用 `ControlType` 和 `getControlPointByPos()` 的控制点检测代码块
2. **`mousePressEvent()` 约 1147-1155 行**：注释掉使用 `ControlType` 和 `getControlPointByPos()` 的控制点检测代码块

> ⚠️ 这些代码在 plan-03 中会被进一步处理（重构为 Overlay 方案），此处先注释确保编译通过。

## 验证

```powershell
.\scripts\build.ps1 -Target DAGraphicsView
```

<!-- 评审修复: W4 -->
DAGraphicsView 模块编译通过，含以下临时适配：
- `DAGraphicsScene.cpp`：注释掉 `ControlType`/`getControlPointByPos()` 引用（约 1043-1050 行、1147-1155 行），plan-03 进一步处理
- `DAGraphicsTextItem.cpp` line 312 调用了已移除的 `isResizing()`，**必然编译失败**。临时修复：将 `if (!isResizing())` 替换为 `if (true)`（保留内部文本编辑逻辑，plan-05 正式移除此守卫）。<!-- 评审修复R3: W2 -->

<!-- 评审修复: W2 -->
<!-- 评审修复R4: 补充 -->
> **说明**：DAGraphicsTextItem::mousePressEvent 中的 `DAGraphicsResizeableItem::mousePressEvent(e)` 调用，此调用在 plan-05 中决定保留，因继承链解析到 QGraphicsItem::mousePressEvent，功能正确。plan-02 仅关注 isResizing() 的临时修复。
