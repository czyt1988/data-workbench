# Plan-04: 修改 Undo Commands + Factory

> **前置阅读**：先阅读 `REFACTOR_OVERLAY_PLAN.md`（总纲），再阅读 `plan-01.md`
>
> **目标**：将 4 个 undo 命令类和 5 个工厂方法的参数类型从 `DAGraphicsResizeableItem*` 改为 `DAIResizableGraphicsItem*`
>
> **验证标准**：DAGraphicsView 模块编译通过

---

## 涉及文件

| 文件 | 操作 |
|------|------|
| `src/DAGraphicsView/DACommandsForGraphics.h` | **修改**：4 个命令类参数类型 |
| `src/DAGraphicsView/DACommandsForGraphics.cpp` | **修改**：`setPos`/`setRotation` 改为通过 `graphicsItem()` |
| `src/DAGraphicsView/DAGraphicsCommandsFactory.h` | **修改**：5 个工厂方法参数类型 |
| `src/DAGraphicsView/DAGraphicsCommandsFactory.cpp` | **修改**：适配 |
| `src/DAGraphicsView/DAGraphicsScene.cpp` | **微调**：移除 plan-03 引入的 dynamic_cast 临时适配 | <!-- 评审修复: B2 -->

---

## 前置说明

`DAIResizableGraphicsItem` 是纯虚接口（在 `plan-01.md` 中创建），提供：
- `getBodySize()` / `setBodySize()` / `getBodyRect()`
- `getBodyMinimumSize()` / `getBodyMaximumSize()`
- `graphicsItem()` → 返回 `QGraphicsItem*`

`setPos()` 和 `setRotation()` 不在接口中（是 `QGraphicsItem` 的方法），通过 `mItem->graphicsItem()->setPos()` / `mItem->graphicsItem()->setRotation()` 调用。

<!-- 评审修复: S3 -->
> 💡 **可选优化**：后续可在 `DAIResizableGraphicsItem` 接口中添加 `pos()`/`setPos()` 便捷方法（内部转发到 `graphicsItem()->pos()`/`graphicsItem()->setPos()`），减少调用方代码。当前不强制要求。

## 步骤 1: 修改 `DACommandsForGraphics.h`

### 1.1 前向声明

```cpp
// 替换:
// class DAGraphicsResizeableItem;
// 为:
class DAIResizableGraphicsItem;
```

### 1.2 `DACommandsForGraphicsItemResized`

构造函数和成员变量类型修改：

```cpp
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemResized : public QUndoCommand
{
public:
    DACommandsForGraphicsItemResized(DAIResizableGraphicsItem* item,  // ← 改类型
                                     const QPointF& oldpos,
                                     const QSizeF& oldSize,
                                     const QPointF& newpos,
                                     const QSizeF& newSize,
                                     bool skipfirst       = true,
                                     QUndoCommand* parent = nullptr);
    DACommandsForGraphicsItemResized(DAIResizableGraphicsItem* item,  // ← 改类型
                                     const QSizeF& oldSize,
                                     const QSizeF& newSize,
                                     QUndoCommand* parent = nullptr);
    // ... 其余不变
private:
    DAIResizableGraphicsItem* mItem;  // ← 改类型
    // ... 其余不变
};
```

### 1.3 `DACommandsForGraphicsItemResizeWidth`

```cpp
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemResizeWidth : public QUndoCommand
{
public:
    DACommandsForGraphicsItemResizeWidth(DAIResizableGraphicsItem* item,  // ← 改类型
                                         const qreal& oldWidth,
                                         const qreal& newWidth,
                                         QUndoCommand* parent = nullptr);
    // ... 其余不变
private:
    DAIResizableGraphicsItem* mItem;  // ← 改类型
    qreal mOldWidth;
    qreal mNewWidth;
    // mHeight 已移除 — 改用 mItem->getBodySize().height() 实时获取 <!-- 评审修复R4: W1 -->
    QDateTime mDatetime;
};
```

### 1.4 `DACommandsForGraphicsItemResizeHeight`

```cpp
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemResizeHeight : public QUndoCommand
{
public:
    DACommandsForGraphicsItemResizeHeight(DAIResizableGraphicsItem* item,  // ← 改类型
                                          const qreal& oldHeight,
                                          const qreal& newHeight,
                                          QUndoCommand* parent = nullptr);
    // ... 其余不变
private:
    DAIResizableGraphicsItem* mItem;  // ← 改类型
    qreal mOldHeight;
    qreal mNewHeight;
    // mWidth 已移除 — 改用 mItem->getBodySize().width() 实时获取 <!-- 评审修复R4: W1 -->
    QDateTime mDatetime;
};
```

### 1.5 `DACommandsForGraphicsItemRotation`

```cpp
class DAGRAPHICSVIEW_API DACommandsForGraphicsItemRotation : public QUndoCommand
{
public:
    DACommandsForGraphicsItemRotation(DAIResizableGraphicsItem* item,  // ← 改类型
                                      const qreal& oldRotation,
                                      const qreal& newRotation,
                                      QUndoCommand* parent = nullptr);
    // ... 其余不变
private:
    DAIResizableGraphicsItem* mItem;  // ← 改类型
    // ... 其余不变
};
```

## 步骤 2: 修改 `DACommandsForGraphics.cpp`

### 2.1 include

```cpp
#include "DAIResizableGraphicsItem.h"
// 移除: #include "DAGraphicsResizeableItem.h"  <!-- 评审修复R3: 01 — 原文件第 5 行确实存在此 include，必须移除 -->
```

### 2.2 `DACommandsForGraphicsItemResized` 实现

构造函数参数类型改为 `DAIResizableGraphicsItem*`，`setPos` 改为 `mItem->graphicsItem()->setPos()`：

```cpp
// redo: <!-- 评审修复: W1/W2/W3 -->
void DACommandsForGraphicsItemResized::redo()
{
    QUndoCommand::redo();  // ← 保留基类调用
    if (mSkipfirst) {
        mSkipfirst = false;
        return;
    }
    if (mItem) {  // ← 保留空指针保护
        if (mNewSize.isValid()) {  // ← 保留有效性检查
            mItem->setBodySize(mNewSize);
        }
        if (mHasPosition) {
            mItem->graphicsItem()->setPos(mNewPosition);  // ← 改
        }
    }
}

// undo:
void DACommandsForGraphicsItemResized::undo()
{
    QUndoCommand::undo();  // ← 保留基类调用
    if (mItem) {  // ← 保留空指针保护
        if (mOldSize.isValid()) {  // ← 保留有效性检查
            mItem->setBodySize(mOldSize);
        }
        if (mHasPosition) {
            mItem->graphicsItem()->setPos(mOldpos);  // ← 改
        }
    }
}
```

### 2.2b 第二个构造函数修改 <!-- 评审修复: B1 -->

<!-- 评审修复R3: 02 — 补充说明：.cpp 中所有构造函数的参数类型也需同步从 `DAGraphicsResizeableItem*` 改为 `DAIResizableGraphicsItem*`，包括第一个和第二个构造函数的签名 -->
<!-- 评审修复R4: S2 — 补充完整构造函数签名示例 -->
第二个构造函数（仅 size 参数版本）的函数体中有 `mOldpos = mNewPosition = item->pos();`，需改为：

```cpp
mOldpos = mNewPosition = item->graphicsItem()->pos();
```

`.cpp` 中所有构造函数签名也需同步修改，例如：

```cpp
// .cpp 中所有构造函数签名也需同步修改，例如：
DACommandsForGraphicsItemResized::DACommandsForGraphicsItemResized(
    DAIResizableGraphicsItem* item,  // ← 改类型
    const QPointF& oldpos,
    const QSizeF& oldSize,
    const QPointF& newpos,
    const QSizeF& newSize,
    bool skipfirst,
    QUndoCommand* parent)
    : QUndoCommand(parent), mItem(item), mOldpos(oldpos), mOldSize(oldSize),
      mNewPosition(newpos), mNewSize(newSize), mHasPosition(true), mSkipfirst(skipfirst),
      mDatetime(QDateTime::currentDateTime())
{
    setText(QObject::tr("Item Resize"));  // cn:调整图元尺寸
}
```

### 2.3 `DACommandsForGraphicsItemResizeWidth` 实现

<!-- 评审修复: W1/W2/W4 -->
> ⚠️ **行为改进**：使用 `mItem->getBodySize().height()` 而非存储的 `mHeight`，确保 width-only resize 的 undo 不回退无关的高度变更。
>
> <!-- 评审修复R3: 07 — 明确 mHeight/mWidth 处理方式 -->
> **头文件变更**：移除 `DACommandsForGraphicsItemResizeWidth::mHeight` 和 `DACommandsForGraphicsItemResizeHeight::mWidth` 成员变量。redo/undo 已改用 `mItem->getBodySize()` 实时获取，不再需要缓存。
>
> **.cpp 构造函数变更**：移除构造函数初始化列表中的 `mHeight(item->getBodySize().height()` / `mWidth(item->getBodySize().width()` 初始化。

```cpp
void DACommandsForGraphicsItemResizeWidth::redo()
{
    QUndoCommand::redo();  // ← 新增基类调用（原代码无此调用）<!-- 评审修复R3: 03 -->
    if (mItem) {  // ← 保留空指针保护
        // getBodySize() 仍在接口中
        mItem->setBodySize(QSizeF(mNewWidth, mItem->getBodySize().height()));
    }
}

void DACommandsForGraphicsItemResizeWidth::undo()
{
    QUndoCommand::undo();  // ← 新增基类调用（原代码无此调用）<!-- 评审修复R3: 03 -->
    if (mItem) {  // ← 保留空指针保护
        mItem->setBodySize(QSizeF(mOldWidth, mItem->getBodySize().height()));
    }
}
```

### 2.4 `DACommandsForGraphicsItemResizeHeight` 实现

同上模式，`mWidth` → `mItem->getBodySize().width()`。 <!-- 评审修复R3: 04 — 原文误写为 `getWidth`，实际代码使用成员变量 `mWidth` -->

```cpp
void DACommandsForGraphicsItemResizeHeight::redo()
{
    QUndoCommand::redo();  // ← 新增基类调用（原代码无此调用）<!-- 评审修复R3: 04 -->
    if (mItem) {  // ← 保留空指针保护
        mItem->setBodySize(QSizeF(mItem->getBodySize().width(), mNewHeight));
    }
}

void DACommandsForGraphicsItemResizeHeight::undo()
{
    QUndoCommand::undo();  // ← 新增基类调用（原代码无此调用）<!-- 评审修复R3: 04 -->
    if (mItem) {  // ← 保留空指针保护
        mItem->setBodySize(QSizeF(mItem->getBodySize().width(), mOldHeight));
    }
}
```

### 2.5 `DACommandsForGraphicsItemRotation` 实现

**关键修复（评审 Blocker B4）**：`setRotation` 改为 `mItem->graphicsItem()->setRotation()`： <!-- 评审修复: W1/W2 -->

```cpp
void DACommandsForGraphicsItemRotation::redo()
{
    QUndoCommand::redo();  // ← 新增基类调用（原代码无此调用）<!-- 评审修复R3: 05 -->
    if (mItem) {  // ← 保留空指针保护
        mItem->graphicsItem()->setRotation(mNewRotation);  // ← 改
    }
}

void DACommandsForGraphicsItemRotation::undo()
{
    QUndoCommand::undo();  // ← 新增基类调用（原代码无此调用）<!-- 评审修复R3: 05 -->
    if (mItem) {  // ← 保留空指针保护
        mItem->graphicsItem()->setRotation(mOldRotation);  // ← 改
    }
}
```

### 2.6 `mergeWith()` 无需修改 <!-- 评审修复: S2 -->

`mergeWith()` 函数无需修改。`mItem` 指针比较语义不变（`DAIResizableGraphicsItem` 是纯虚接口，所有方法均为 `= 0`，指针值比较语义不变）。 <!-- 评审修复R3: 06 — 原文误称"非虚继承基类"，实际为纯虚接口 -->

## 步骤 3: 修改 `DAGraphicsCommandsFactory.h`

### 3.1 前向声明

```cpp
// 替换:
// class DAGraphicsResizeableItem;
// 为:
class DAIResizableGraphicsItem;
```

### 3.2 5 个工厂方法参数类型

```cpp
virtual DACommandsForGraphicsItemResized* createItemResized(
    DAIResizableGraphicsItem* item,  // ← 改类型
    const QPointF& oldpos,
    const QSizeF& oldSize,
    const QPointF& newpos,
    const QSizeF& newSize,
    bool skipfirst = true
);
virtual DACommandsForGraphicsItemResized* createItemResized(
    DAIResizableGraphicsItem* item,  // ← 改类型
    const QSizeF& oldSize,
    const QSizeF& newSize
);
virtual DACommandsForGraphicsItemResizeWidth* createItemResizeWidth(
    DAIResizableGraphicsItem* item, const qreal& oldWidth, const qreal& newWidth  // ← 改类型
);
virtual DACommandsForGraphicsItemResizeHeight* createItemResizeHeight(
    DAIResizableGraphicsItem* item, const qreal& oldHeight, const qreal& newHeight  // ← 改类型
);
virtual DACommandsForGraphicsItemRotation* createItemRotation(
    DAIResizableGraphicsItem* item, const qreal& oldRotation, const qreal& newRotation  // ← 改类型
);
```

## 步骤 4: 修改 `DAGraphicsCommandsFactory.cpp`

<!-- 评审修复: W5 -->
<!-- 评审修复R4: S1 — 补充工厂方法完整代码示例 -->
工厂方法实现需更新 5 个函数签名的参数类型为 `DAIResizableGraphicsItem*`，函数体逻辑不变（参数透传给构造函数）。

```cpp
// DAGraphicsCommandsFactory.cpp 示例（其余 4 个方法同理）：
DACommandsForGraphicsItemResized* DAGraphicsCommandsFactory::createItemResized(
    DAIResizableGraphicsItem* item,  // ← 参数类型改为接口指针
    const QPointF& oldpos,
    const QSizeF& oldSize,
    const QPointF& newpos,
    const QSizeF& newSize,
    bool skipfirst)
{
    return new DACommandsForGraphicsItemResized(item, oldpos, oldSize, newpos, newSize, skipfirst);
}
```

**确认方法**：阅读 `DAGraphicsCommandsFactory.cpp`，确保所有 `new DACommandsForGraphicsItem*(...)` 调用的第一个参数类型与新的声明一致。

## 步骤 5: 适配 `plan-03.md` 中的 `onRequestResize`

<!-- 评审修复: S1 -->
> ⚠️ 此步骤修正 plan-03 §2.3 引入的临时 dynamic_cast 适配。

<!-- 评审修复R3: 08 — 原文使用条件句"如果"，但 plan-03 §2.3 明确使用了 dynamic_cast，应改为确定性描述 -->
`plan-03` §2.3 中使用了 `dynamic_cast<DAGraphicsResizeableItem*>(target)` 来适配旧工厂签名。现在工厂签名已改为 `DAIResizableGraphicsItem*`，可以直接传 `target`，移除 `dynamic_cast` 及其空指针防护：

<!-- 评审修复R4: W2/S3 — 补充完整函数签名及实现，移除 ... 省略 -->
```cpp
void DAGraphicsScene::onRequestResize(DAIResizableGraphicsItem* target,
                                       const QPointF& oldPos, const QSizeF& oldSize,
                                       const QPointF& newPos, const QSizeF& newSize)
{
    DA_D(d);
    auto cmd = commandsFactory()->createItemResized(
        target,  // 直接传，不需要 dynamic_cast
        oldPos, oldSize, newPos, newSize, true  // skipfirst=true
    );
    push(cmd);
    // 命令执行后 item 尺寸变化，overlay 需要同步
    if (d->mResizeOverlay) {
        d->mResizeOverlay->syncToTarget();
    }
}
```

## 验证

```powershell
.\scripts\build.ps1 -Target DAGraphicsView
```

编译通过即可。如果有下游模块（DAGui 的 `DANodeItemSettingWidget`）调用了 `createItemResized` 且传入 `DAGraphicsResizeableItem*`，由于 `DAGraphicsResizeableItem` 实现了 `DAIResizableGraphicsItem` 接口，隐式转换可行，应该能编译通过。如果不行，plan-05 会处理。

<!-- 评审修复: S4 -->
**额外确认项**：确认 `DAGraphicsResizeableItem.cpp` 中 `mouseReleaseEvent` 已被 plan-02 移除，不存在对工厂方法的直接调用。
