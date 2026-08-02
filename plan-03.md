# Plan-03: 修改 DAGraphicsScene

> **前置阅读**：先阅读 `REFACTOR_OVERLAY_PLAN.md`（总纲），再阅读 `plan-01.md` 和 `plan-02.md`
>
> **⚠️ 依赖 plan-01 B2 修复**：Overlay 的 `shape()` 必须仅包含 8 个控制点区域，不含 body。此修复在 plan-01 中完成。执行 plan-03 前确保 plan-01 已包含此修复。<!-- 评审修复: B2 -->
>
> <!-- 评审修复R3: B1 --> **⚠️ 跨计划依赖**：确保 plan-01 §4.8 的网格对齐代码已修复 `getGridSize()` 返回类型（`QSize` 而非 `qreal`），否则 plan-03 无法编译通过。
>
> **目标**：让 Scene 管理 Overlay 的创建/销毁生命周期，处理 `requestResize` 信号
>
> **验证标准**：DAGraphicsView 模块编译通过，选中图元时 Overlay 显示

---

## 涉及文件

| 文件 | 操作 |
|------|------|
| `src/DAGraphicsView/DAGraphicsScene.h` | **修改**：新增 Overlay 成员、槽函数、friend |
| `src/DAGraphicsView/DAGraphicsScene.cpp` | **修改**：Overlay lifecycle、requestResize 处理、移除控制点检测 |

---

## 步骤 1: 修改 `DAGraphicsScene.h`

### 1.1 新增前向声明和 include

```cpp
// 在头文件已有的前向声明区域添加:
class DAIResizableGraphicsItem;
class DAGraphicsResizeOverlayItem;
```

### 1.2 新增 friend 声明

在 `friend class DAGraphicsResizeableItem;` 之后添加：

```cpp
friend class DAGraphicsResizeOverlayItem;  // 保留以备 Overlay 需要直接访问 Scene 的 protected 方法（如网格对齐配置）<!-- 评审修复: S1 -->
```

**注意**：`friend class DAGraphicsResizeableItem;` **保留不移除**（`setBodySize` 和 `itemChange` 通过 friend 调用 `emitItemBodySizeChanged` / `emitItemRotationChanged`）。

### 1.3 新增私有成员

在 PrivateData 区域或类内部添加（取决于 PIMPL 结构）：

<!-- 评审修复R3: S4 --> `DAGraphicsScene::PrivateData` 定义在 `DAGraphicsScene.cpp` 第 39 行起。在 PrivateData 的 public 成员区域添加 `QPointer<DAGraphicsResizeOverlayItem> mResizeOverlay;` 和 `QMetaObject::Connection mResizeConn;`。

如果 `DAGraphicsScene` 使用 PIMPL（`DA_DECLARE_PRIVATE`），则在 PrivateData 中添加：
```cpp
QPointer<DAGraphicsResizeOverlayItem> mResizeOverlay;
QMetaObject::Connection mResizeConn;
```

如果不使用 PIMPL，直接在类的 private 区域添加上述成员。

<!-- 评审修复R4: S4 --> QPointer 仅在 .cpp 的 PrivateData 中使用，头文件无需 include `<QPointer>`。

**确认方法**：阅读 `DAGraphicsScene.h` 确认是否有 `DA_DECLARE_PRIVATE`。

### 1.4 新增私有槽

<!-- 评审修复R4: S1 -->
将现有的 `private slots:` 改为 `private Q_SLOTS:`，并在其中添加 `onRequestResize` 声明（与 `onSelectionChanged` 同一 section）。不要创建新的 Q_SLOTS section。

```cpp
private Q_SLOTS:
    void onSelectionChanged();  // 已有
    void onRequestResize(DAIResizableGraphicsItem* target,
                          const QPointF& oldPos, const QSizeF& oldSize,
                          const QPointF& newPos, const QSizeF& newSize);
```

<!-- 评审修复: S3 -->
**注意**：在修改 `DAGraphicsScene.h` 时，顺便检查文件中现有的 `private slots:` 声明，统一改为 `private Q_SLOTS:`，保持文件内风格一致（项目规范要求使用 `Q_SLOTS` 而非 `slots`）。

## 步骤 2: 修改 `DAGraphicsScene.cpp`

### 2.1 添加 include

```cpp
#include "DAGraphicsResizeOverlayItem.h"
#include "DAIResizableGraphicsItem.h"
#include <QPointer>
```

### 2.1b 修改析构函数 `~DAGraphicsScene()`

<!-- 评审修复R4: B1 -->

plan-03 新增的 4 个 lambda 连接（xChanged/yChanged/rotationChanged/destroyed）在 Scene 析构期间可能访问已释放的 d_ptr。C++ 析构顺序：d_ptr 先释放，`~QGraphicsScene()` 后删除 item 时可能触发信号回调。

修改 `DAGraphicsScene.cpp` 的析构函数（约 199-201 行），添加 overlay 清理和 selectionChanged 断开：

```cpp
DAGraphicsScene::~DAGraphicsScene()
{
    // 断开 selectionChanged，防止 ~QGraphicsScene() 删除 item 时回调访问已释放的 d_ptr
    disconnect(this, &QGraphicsScene::selectionChanged, this, &DAGraphicsScene::onSelectionChanged);
    DA_D(d);
    if (d->mResizeOverlay) {
        delete d->mResizeOverlay;
        d->mResizeOverlay = nullptr;
    }
}
```

**说明**：`DAGraphicsScene.h` 中析构函数声明 `~DAGraphicsScene()` 已存在，无需修改头文件。

### 2.2 修改 `onSelectionChanged()`

在现有的 `onSelectionChanged()` 函数中，**在原有逻辑之前**插入 Overlay 管理逻辑：

```cpp
void DAGraphicsScene::onSelectionChanged()
{
    DA_D(d);  // 或直接访问成员，取决于 PIMPL

    // === Overlay 生命周期管理 ===
    // 销毁旧 overlay
    if (d->mResizeOverlay) {
        if (d->mResizeConn) {
            disconnect(d->mResizeConn);  // <!-- 评审修复R3: S2 --> delete overlay 会自动断开 mResizeConn，显式 disconnect 为代码清晰
            d->mResizeConn = QMetaObject::Connection();
        }
        delete d->mResizeOverlay;
        d->mResizeOverlay = nullptr;
    }

    // 检查新选中的 item
    // <!-- 评审修复: W4 --> 复用 selected 变量，避免重复调用 selectedItems()
    // <!-- 评审修复R3: W5 --> 注意：overlay 使用 selected.first()（单选时与 last() 相同），
    // checkSelectItem 保持 selected.last() 不变（原有行为）
    auto selected = selectedItems();  // <!-- 评审修复R4: S3 --> selected.first() 用于 overlay，selected.last() 用于 checkSelectItem
    if (selected.size() == 1) {
        QGraphicsItem* selectedItem = selected.first();
        // 尝试获取 DAIResizableGraphicsItem 接口
        DAIResizableGraphicsItem* resizable = dynamic_cast<DAIResizableGraphicsItem*>(selectedItem);
        // <!-- 评审修复: W6 --> 添加 isReadOnly() 检查，只读模式下不创建 overlay
        if (resizable && resizable->isResizable() && !isReadOnly()) {
            // 创建 overlay
            d->mResizeOverlay = new DAGraphicsResizeOverlayItem(resizable);
            addItem(d->mResizeOverlay);
            d->mResizeOverlay->syncToTarget();
            // 连接信号
            d->mResizeConn = connect(d->mResizeOverlay, &DAGraphicsResizeOverlayItem::requestResize,
                                     this, &DAGraphicsScene::onRequestResize);

            // <!-- 评审修复: W1 --> 连接 target 的位置/旋转变化信号，确保 target 通过键盘、undo/redo、
            // 外部代码改变位置或旋转时 Overlay 自动同步
            // <!-- 评审修复R3: W3 --> 活跃缩放期间跳过信号触发的同步，避免重复调用
            // （mouseMoveEvent 末尾已显式调用 syncToTarget）
            // <!-- 评审修复R3: S3 --> 这些连接依赖 overlay 作为 context 对象自动断开（overlay delete 时自动清理）
            QGraphicsItem* targetGI = resizable->graphicsItem();
            if (auto* obj = dynamic_cast<QGraphicsObject*>(targetGI)) {
                connect(obj, &QGraphicsObject::xChanged, d->mResizeOverlay.data(), [this]() {
                    DA_D(d);
                    if (d->mResizeOverlay && !d->mResizeOverlay->isResizing())
                        d->mResizeOverlay->syncToTarget();
                });
                connect(obj, &QGraphicsObject::yChanged, d->mResizeOverlay.data(), [this]() {
                    DA_D(d);
                    if (d->mResizeOverlay && !d->mResizeOverlay->isResizing())
                        d->mResizeOverlay->syncToTarget();
                });
                connect(obj, &QGraphicsObject::rotationChanged, d->mResizeOverlay.data(), [this]() {
                    DA_D(d);
                    if (d->mResizeOverlay && !d->mResizeOverlay->isResizing())
                        d->mResizeOverlay->syncToTarget();
                });

                // <!-- 评审修复: W2 --> target 被删除时清理 overlay，避免悬垂指针
                connect(obj, &QObject::destroyed, d->mResizeOverlay.data(), [this](QObject*) {
                    DA_D(d);
                    if (d->mResizeOverlay) {
                        // <!-- 评审修复R4: S5 --> 重置连接，避免悬垂 Connection 对象
                        d->mResizeConn = QMetaObject::Connection();
                        delete d->mResizeOverlay;
                        d->mResizeOverlay = nullptr;
                    }
                });
            }

            // <!-- 评审修复R4: W2 --> 注意：此连接依赖 target 的 graphicsItem() 返回 QGraphicsObject 指针。
            // 当前所有 DAIResizableGraphicsItem 实现都通过 DAGraphicsItem → QGraphicsObject，
            // dynamic_cast 总会成功。若未来有非 QGraphicsObject 实现，需另行处理位置/旋转同步。
        }
    }
    // <!-- 评审修复R3: W1 --> 给出完整的合并后函数体，替换原有代码片段
    // === 原有逻辑 ===
    if (selected.isEmpty()) {
        return;
    }
    checkSelectItem(selected.last());
}
```

**注意**：
- 阅读现有 `onSelectionChanged()` 的完整实现，确保新逻辑插入在正确位置，不破坏原有的 `selectItemChanged` / `selectLinkChanged` 信号发射逻辑。
- <!-- 评审修复: W1 --> 销毁 overlay 时，所有通过 `connect` 建立的信号连接会自动断开（因为 overlay 作为 receiver 被 delete），无需手动断开 target→overlay 的连接。
- <!-- 评审修复R3: B2 --> ⚠️ **必须在 plan-01 §4.3 `syncToTarget()` 和 §4.4 `paint()` 开头添加 `mTarget` null check（`if (!d->mTarget) return;`）。此为执行前置条件，若 plan-01 未包含此修复，需在此步骤中补充修改 plan-01 创建的文件。**

### 2.2b 修改 `setReadOnly()`

<!-- 评审修复R4: W1 -->

overlay 创建时检查 `!isReadOnly()`，但如果场景在 overlay 已创建后变为只读，overlay 不会被销毁，用户仍可通过控制点缩放图元。在 `setReadOnly()` 方法中添加 overlay 销毁逻辑：

```cpp
void DAGraphicsScene::setReadOnly(bool on)
{
    DA_D(d);
    // ... 原有逻辑 ...
    if (on && d->mResizeOverlay) {
        delete d->mResizeOverlay;
        d->mResizeOverlay = nullptr;
    }
}
```

### 2.3 实现 `onRequestResize()`

```cpp
void DAGraphicsScene::onRequestResize(DAIResizableGraphicsItem* target,
                                       const QPointF& oldPos, const QSizeF& oldSize,
                                       const QPointF& newPos, const QSizeF& newSize)
{
    DA_D(d);
    // <!-- 评审修复: W3 --> dynamic_cast 失败时防护
    // <!-- 评审修复R3: W4 --> ⚠️ 临时适配：plan-04 将工厂签名改为 DAIResizableGraphicsItem* 后移除此 dynamic_cast
    auto* resizeable = dynamic_cast<DAGraphicsResizeableItem*>(target);
    if (!resizeable) {
        return;
    }
    // 通过 undo command 应用变更
    // skipfirst=true，因为 resize 已经在 mouseMove 中实时执行了
    auto cmd = commandsFactory()->createItemResized(
        resizeable,  // 保持现有工厂方法签名
        oldPos, oldSize, newPos, newSize, true  // skipfirst=true
    );
    push(cmd);
    // 命令执行后 item 尺寸变化，overlay 需要同步
    // <!-- 评审修复: S2 --> syncToTarget 确保命令 push 后 overlay 状态一致
    // <!-- 评审修复R4: S2 --> 此处在信号同步调用链中为冗余调用（mouseReleaseEvent 已调用 syncToTarget），保留以应对未来异步信号场景
    // <!-- 评审修复R3: S1 --> undo/redo 场景下 item 状态变化由信号触发同步，此处为安全冗余
    if (d->mResizeOverlay) {
        d->mResizeOverlay->syncToTarget();
    }
}
```

**注意**：`commandsFactory()->createItemResized()` 当前签名是 `DAGraphicsResizeableItem*`。在 plan-04 中会改为 `DAIResizableGraphicsItem*`。此阶段用 `dynamic_cast` 适配。

### 2.4 修改 `mousePressEvent`

阅读 `DAGraphicsScene.cpp` 的 `mousePressEvent`（约 1096-1161 行），做以下修改：

**修改 a**：<!-- 评审修复: B1 --> **不添加** `isAccepted()` 检查。

> **⚠️ B1 修复说明**：原计划在 `QGraphicsScene::mousePressEvent(mouseEvent)` 之后添加 `if (mouseEvent->isAccepted()) { return; }`。这会导致普通 item accept 事件后 scene 提前 return，`commandsFactory()->sceneMousePressEvent()` 永远不被调用，移动 undo 命令不被创建，影响**所有** item 的移动 undo/redo，不仅限于 resizable。
>
> Overlay 的 `shape()` 已在 plan-01 B2 修复中修改为仅覆盖控制点区域，点击 body 区域时事件直接传播到 target item，不影响移动记录。因此无需 isAccepted() 检查。

```cpp
QGraphicsScene::mousePressEvent(mouseEvent);
// 不添加 isAccepted() 检查 — 见上方 B1 修复说明
```

**修改 b**：移除 `mousePressEvent` 中遍历选中 item 检查控制点的逻辑（约 1148-1155 行）：

<!-- 评审修复R3: W2 --> plan-02 §2.14 已将这些代码块注释掉，此处删除这些注释块。

找到类似以下注释块并删除：
```cpp
// 移除这段:
// for (QGraphicsItem* item : selectedMovableItems) {
//     DAGraphicsResizeableItem* ri = dynamic_cast<DAGraphicsResizeableItem*>(item);
//     if (ri) {
//         QPointF itemPos = item->mapFromScene(mouseEvent->scenePos());
//         if (ri->getControlPointByPos(itemPos) != DAGraphicsResizeableItem::NotUnderAnyControlType) {
//             return;
//         }
//     }
// }
```

### 2.5 修改 `isItemCanMove()`

阅读 `isItemCanMove()`（约 1043-1050 行），移除控制点检测：

```cpp
// 旧代码中类似:
// DAGraphicsResizeableItem* ri = qgraphicsitem_cast<DAGraphicsResizeableItem*>(positem);
// if (ri) {
//     QPointF itemPos = positem->mapFromScene(scenePos);
//     if (ri->getControlPointByPos(itemPos) != DAGraphicsResizeableItem::NotUnderAnyControlType) {
//         return false;
//     }
// }

// 新代码: 删除上述注释块
// <!-- 评审修复R3: W2 --> plan-02 §2.14 已将此代码块注释掉，此处删除这些注释块
// Overlay 作为独立 item 会优先接收鼠标事件，Scene 不再需要控制点检测
```

保留 `isItemCanMove` 中的其他逻辑（如只读检查等）。

## 注意事项

### 注意事项：DAPyWorkFlowScene

<!-- 评审修复: W5 / W7 -->
`DAPyWorkFlowScene` 继承 `DAGraphicsScene` 并重写了 `mousePressEvent`。plan-03 的修改对 `DAPyWorkFlowScene` 透明：

1. `isItemCanMove` 是 virtual 函数，修改对子类透明
2. `DAPyWorkFlowScene::mousePressEvent` 调用 `DAGraphicsScene::mousePressEvent` 的方式不受影响
3. `DAPyWorkFlowScene` 的多选拖拽逻辑不依赖被移除的控制点检测

### 后续优化

<!-- 评审修复: W8 / S4 -->
<!-- 评审修复R4: W3 -->
- **多选闪烁优化**：如果新选中的 item 与旧 Overlay 的 target 相同，则不销毁重建，仅调用 `syncToTarget()` 同步。这可避免多选切换时 Overlay 销毁/重建导致的视觉闪烁。

  **具体方案**：在 overlay 销毁前检查 target 是否相同——如果新选中 item 与旧 overlay 的 target 相同且仍是单选，跳过销毁/重建，仅调用 `syncToTarget()`。

## 验证

```powershell
.\scripts\build.ps1 -Target DAGraphicsView
```

编译通过后，可手动启动程序测试：选中一个矩形图元 → 控制点应显示在 Overlay 上（而非 item 自身）。

### 运行时验证步骤

<!-- 评审修复: S5 -->
1. 验证选中 resizable item 后控制点出现在 Overlay 上
2. 验证拖拽控制点能实时缩放 item
3. 验证释放鼠标后 undo 栈有"调整图元尺寸"命令
4. 验证 Ctrl+Z 能正确回退缩放
5. 验证点击 body 区域仍能移动 item（不受 Overlay 影响）
6. 验证点击空白区域取消选中后 Overlay 消失
