# Plan-05: 子类适配

> **前置阅读**：先阅读 `REFACTOR_OVERLAY_PLAN.md`（总纲），再阅读 `plan-01.md` ~ `plan-04.md`
>
> **目标**：适配所有受影响的子类和下游模块，确保全项目编译通过
>
> **验证标准**：DAGraphicsView + DAPyWorkFlow + DAGui + APP 全模块编译通过

---

## 涉及文件

| 文件 | 操作 |
|------|------|
| `src/DAGraphicsView/DAGraphicsTextItem.cpp` | **微调**：移除 `isResizing()` 调用 |
| `src/DAPyWorkFlow/DAPyNodeGraphicsItem.cpp` | **微调**：`shape()` 中 `getBodyControlRect()` → `getBodyRect()` |
| `src/DAGui/DANodeItemSettingWidget.h` | **微调**：适配（如需要） |
| `src/DAGui/DANodeItemSettingWidget.cpp` | **微调**：适配 `createItemResized` 参数类型 |
| `src/DAGui/DAPyWorkFlowNodeItemSettingWidget.h` | **微调**：适配（如需要） |
| `src/DAGui/DAPyWorkFlowNodeItemSettingWidget.cpp` | **微调**：适配 `dynamic_cast` |
| `src/DAPyWorkFlow/AGENTS.md` | **文档更新** |
| `docs/zh/dev-guide/scalable-graphic-module.md` | **文档更新**：移除已废弃 API 引用 |

---

## 步骤 1: `DAGraphicsTextItem.cpp` — 移除 `isResizing()` 调用

### 定位

阅读 `src/DAGraphicsView/DAGraphicsTextItem.cpp` 的 `mousePressEvent` 方法，找到对 `isResizing()` 的调用。

### 修改

`isResizing()` 已从 `DAGraphicsResizeableItem` 移除。TextItem 不再需要此检查（控制点交互由 Overlay 处理）。

<!-- 评审修复: B1, B2 -->
<!-- 评审修复R3: R3-03 -->
找到 `mousePressEvent` 方法（约 308-322 行），实际源码使用 `if (!isResizing())` 守卫来控制文本编辑模式的进入。将 `isResizing()` 条件守卫移除，保留基类调用和文本编辑逻辑：

```cpp
// 旧 (DAGraphicsTextItem.cpp:308-322):
void DAGraphicsTextItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    // qDebug() << "DAGraphicsTextItem::mousePressEvent";
    DAGraphicsResizeableItem::mousePressEvent(e);
    if (!isResizing()) {
        auto br = d_ptr->mTextItem->boundingRect();
        if (br.contains(e->pos())) {
            QTextCursor cursor(d_ptr->mTextItem->document());
            cursor.movePosition(QTextCursor::End);
            d_ptr->mTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
            d_ptr->mTextItem->setTextCursor(cursor);
            d_ptr->mTextItem->setFocus();  // 确保获得焦点以显示光标
        }
    }
}

// 新: 移除 isResizing() 守卫，保留基类调用和文本编辑逻辑
<!-- 评审修复R3: R3-02 -->
void DAGraphicsTextItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    DAGraphicsResizeableItem::mousePressEvent(e);  // 保留：plan-02 移除 override 后退化为基类 QGraphicsItem::mousePressEvent，处理选中
    auto br = d_ptr->mTextItem->boundingRect();
    if (br.contains(e->pos())) {
        QTextCursor cursor(d_ptr->mTextItem->document());
        cursor.movePosition(QTextCursor::End);
        d_ptr->mTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
        d_ptr->mTextItem->setTextCursor(cursor);
        d_ptr->mTextItem->setFocus();  // 确保获得焦点以显示光标
    }
}
```

**注意**：`DAGraphicsResizeableItem::mousePressEvent(e)` 调用必须保留。plan-02 移除的是 `DAGraphicsResizeableItem::mousePressEvent` 的 override 实现（即自定义的缩放交互逻辑），但 `QGraphicsItem::mousePressEvent` 仍然存在，`DAGraphicsResizeableItem::mousePressEvent(e)` 调用仍可编译，负责处理选中行为。仅移除 `isResizing()` 条件守卫即可。

<!-- 评审修复R4: W2 -->
注：plan-02 中的说明已过时，plan-05 确认保留 `DAGraphicsResizeableItem::mousePressEvent(e)` 调用——plan-02 移除 override 后该调用经继承链解析到 `QGraphicsItem::mousePressEvent`，编译和功能均正确。plan-02 的对应说明需同步更新（见 plan-02 R4 修复）。

## 步骤 2: `DAPyNodeGraphicsItem.cpp` — `getBodyControlRect()` → `getBodyRect()`

### 定位

阅读 `src/DAPyWorkFlow/DAPyNodeGraphicsItem.cpp` 的 `shape()` 方法（约 1332-1348 行），找到 `getBodyControlRect()` 调用（2 处，约 1338 行和 1340 行）。

### 修改

<!-- 评审修复R3: R3-01 -->
```cpp
// 旧 (DAPyNodeGraphicsItem.cpp:1338):
path.addEllipse(getBodyControlRect());       // 直接内联调用，无变量名
// 旧 (DAPyNodeGraphicsItem.cpp:1340):
QRectF r = getBodyControlRect();             // 变量名为 r

// 新:
path.addEllipse(getBodyRect());
QRectF r = getBodyRect();
```

**注意**：`getBodyControlRect()` 在 plan-02 中已改为返回 `getBodyRect()`（不再膨胀），所以即使不改也功能正确。但为了代码清晰，建议显式改为 `getBodyRect()`。实际源码中第一处为内联调用（无变量名），第二处变量名为 `r`，修正时保持原变量名即可。

## 步骤 3: `DANodeItemSettingWidget.cpp` — 适配命令工厂参数

### 定位

阅读 `src/DAGui/DANodeItemSettingWidget.cpp`，找到所有 `createItemResized` 调用（约 468 行和 494 行）。

### 检查

`DANodeItemSettingWidget` 持有 `QPointer<DAGraphicsResizeableItem> mItem`。调用 `commandsFactory()->createItemResized(mItem, ...)` 时：

- 如果 `mItem` 类型是 `DAGraphicsResizeableItem*`，而工厂方法签名已改为 `DAIResizableGraphicsItem*`
- `DAGraphicsResizeableItem` 实现了 `DAIResizableGraphicsItem`，所以 `DAGraphicsResizeableItem*` → `DAIResizableGraphicsItem*` 的隐式转换可行

<!-- 评审修复: W1 -->
同时检查 `createItemRotation`（第 519 行）和 `createItemMoved`（第 536、550 行）调用。`createItemRotation` 参数类型已在 plan-04 中改为 `DAIResizableGraphicsItem*`，隐式转换可行。`createItemMoved` 签名未变（仍为 `QGraphicsItem*`），不受影响。

**预期**：无需修改即可编译通过。但如果编译报错，将调用处显式转换：

<!-- 评审修复R4: W1 -->
注：DAGui 模块有多个头文件使用 `private slots:`，统一修改为 `Q_SLOTS:` 为后续独立任务，不在本次重构范围内。总纲第三轮评审记录中的 S3 修复（`DANodeItemSettingWidget.h:62` 和 `DAPyWorkFlowNodeItemSettingWidget.h:68` 的 `private slots:` → `private Q_SLOTS:`）因此移至后续统一处理。

<!-- 评审修复R3: R3-04 -->
```cpp
auto cmd = d->mScene->commandsFactory()->createItemResized(
    dynamic_cast<DAIResizableGraphicsItem*>(d->mItem.data()),
    originSize, willSetSize
);
```

## 步骤 4: `DAPyWorkFlowNodeItemSettingWidget.cpp` — 适配 dynamic_cast

### 定位

<!-- 评审修复R3: R3-05 -->
阅读 `src/DAGui/DAPyWorkFlowNodeItemSettingWidget.cpp`，找到 `dynamic_cast<DAGraphicsResizeableItem*>` 调用（约 291 行）和信号/槽连接（约 55-91 行，含 disconnect 块 55-70 和 connect 块 75-91）。

### 检查

- `dynamic_cast<DAGraphicsResizeableItem*>(item)` — **不需要改**，`DAGraphicsResizeableItem` 类名和继承关系不变
- 信号连接 `itemBodySizeChanged(DAGraphicsResizeableItem*, ...)` — **不需要改**，Scene 信号签名保持 `DAGraphicsResizeableItem*`（总纲 §3.5 决策）

<!-- 评审修复R4: W1 -->
注：DAGui 模块有多个头文件使用 `private slots:`，统一修改为 `Q_SLOTS:` 为后续独立任务，不在本次重构范围内。

**预期**：无需修改即可编译通过。

## 步骤 5: `DAGraphicsTextItem.cpp` — 其他检查

阅读完整的 `DAGraphicsTextItem.cpp`，搜索以下已移除的方法名，确保无残留调用：

```
isResizing()
getControlPointByPos()
doItemResize()
prepareControlInfoChange()
paintSelectedBorder
paintResizeControlPoints
controlPointRect
ControlType
NotUnderAnyControlType
```

如果找到残留调用，移除或适配。

<!-- 评审修复: S3 -->
<!-- 评审修复R4: S2 -->
**可选优化**：`DAGraphicsTextItem::init()`（约 64-65 行）中的 `setAcceptHoverEvents(true)` ——移除无害（Overlay 拦截 hover 事件），保留亦无害（QGraphicsItem 默认 hover 行为为空操作）。不强制要求修改。

## 步骤 6: `DAGraphicsRectItem.cpp` / `DAGraphicsPixmapItem.cpp` — 检查残留

同样搜索上述已移除的方法名，确保无残留调用。

**预期**：这两个类仅 override 了 `paintBody`、`setBodySize`、`saveToXml`、`loadFromXml`，不应有控制点相关调用。

## 步骤 7: `DAGraphicsDrawRectSceneAction.cpp` / `DAGraphicsDrawTextItemSceneAction.cpp` — 检查

这两个文件调用 `item->setBodySize(...)`，该方法仍然存在，**预期无需修改**。

<!-- 评审修复: S1 -->
## 步骤 8: `DAPyWorkFlow/AGENTS.md` — 文档更新

**经确认无需修改。**

`DAPyNodeGraphicsItem` 的继承关系未变（仍继承 `DAGraphicsResizeableItem`），只是 `DAGraphicsResizeableItem` 新增了 `DAIResizableGraphicsItem` 接口实现。AGENTS.md 类结构表只展示基类，不展示接口，无需更新。

<!-- 评审修复: W2 -->
## 步骤 9: 更新 scalable-graphic-module.md 文档

更新 `docs/zh/dev-guide/scalable-graphic-module.md`，移除或标注为 deprecated 的 API 引用：
- `isResizing()`、`getControlPointByPos()`、`prepareControlInfoChange()`
- `ControlType`、`NotUnderAnyControlType`
- `paintSelectedBorder()`、`paintResizeControlPoints()`

<!-- 评审修复R4: S1 -->
补充以下 plan-02 §1.2 移除的废弃 API：
- `itemBodySizeChanged` 信号（item 级别，`#if DA_USE_QGRAPHICSOBJECT` 块，plan-02 §1.2 移除）
- `daGlobalGraphicsResizeableItemPalette` 全局单例（`Q_GLOBAL_STATIC`，plan-02 §1.2 移除）
- `DAGraphicsResizeableItemPalette` 类（plan-02 §1.2 移除整个类定义）

更新 Mermaid 架构图和 API 表格，说明控制点绘制和交互已移至 `DAGraphicsResizeOverlayItem`。

## 验证

```powershell
# 全量编译
.\scripts\build.ps1 -Full
```

确认以下模块全部编译通过：
- DAGraphicsView ✅
- DAPyWorkFlow ✅
- DAGui ✅
- APP ✅

<!-- 评审修复: W4 -->
手动验证：选中一个 TextItem → 控制点显示在 Overlay 上 → 点击文本 body 区域 → 应能正常进入文本编辑模式（验证 Overlay 事件传播正确）。

<!-- 评审修复: S4 -->
手动验证：选中一个 Ellipse 形状的节点 → 确认命中区域与视觉 body 区域一致（不包含控制点 padding 区域）→ 缩放正常工作。

如果有编译错误，根据错误信息逐一修复。常见问题：
<!-- 评审修复: S2 (确认已为正确字"残留"，无需修改) -->
- 残留的 `isResizing()` 调用 → 移除
- 残留的 `getControlPointByPos()` 调用 → 移除
- 残留的 `ControlType` 引用 → 移除
- `createItemResized` 参数类型不匹配 → 显式 `dynamic_cast`
