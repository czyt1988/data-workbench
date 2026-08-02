# DAGraphicsResizeOverlayItem 重构总纲

> **目标**：将 `DAGraphicsResizeableItem` 从继承模式重构为 Overlay 模式，根治 z-order 覆盖、旋转缩放数学错误、强制继承三大问题。
>
> **参考实现**：项目内 Qwt `QwtFigureWidgetOverlay`（`src/3rdparty/qwt/src/plot/qwt_figure_widget_overlay.h`）

---

## 一、当前问题

| 问题 | 严重度 | 根因 |
|------|--------|------|
| 控制点被上层 item 遮挡 | 严重 | 控制点在 item 自身 `paint()` 中绘制，与 item 同一 z-level |
| 旋转下缩放数学错误 | 严重 | `bodyConnerPoint()` 用 scene 坐标 `pos + size` 算 anchor，未考虑旋转矩阵 |
| 强制继承 | 中等 | 缩放逻辑耦合在基类中，非 `DAGraphicsResizeableItem` 子类无法获得缩放能力 |
| `boundingRect()` 膨胀 | 中等 | `shape()` 始终包含控制点区域，影响碰撞和选中 |

## 二、目标架构：三层分离

```
┌──────────────────────────────────────┐
│ DAGraphicsResizeableItem             │  ← 保留为 body 尺寸管理基类
│  - body 尺寸管理 (get/setBodySize)    │     继承 DAGraphicsItem, 实现 DAIResizableGraphicsItem
│  - paintBody 虚函数                   │
│  - XML 序列化                         │
└──────────────────────────────────────┘
         ▲ 继承
    ┌────┼────────────────────────────────────┐
    │ DAGraphicsRectItem  DAGraphicsTextItem   │
    │ DAGraphicsPixmapItem  DAPyNodeGraphicsItem│
    └─────────────────────────────────────────┘

┌──────────────────────────────────────┐  ← 新增: 独立 Overlay
│ DAGraphicsResizeOverlayItem           │     继承 QGraphicsObject
│  - 控制点绘制 + 命中测试               │     最高 z-value
│  - 鼠标交互 (hover/press/move/release)│     由 Scene 管理 lifecycle
│  - 缩放数学计算 (支持旋转)             │     通过信号 requestResize 通知 Scene
│  - 继承 target 的 pos/rotation        │     不参与 XML 序列化
└──────────────────────────────────────┘
         │ emit requestResize(target, oldPos, oldSize, newPos, newSize)
         ▼
┌──────────────────────────────────────┐
│ DAGraphicsScene                       │
│  - selectionChanged → 创建/销毁 Overlay│
│  - requestResize → 创建 undo command  │
│  - 信号 itemBodySizeChanged 保留      │
└──────────────────────────────────────┘
```

### 新增类

| 类名 | 基类 | 所在模块 | 职责 |
|------|------|----------|------|
| `DAIResizableGraphicsItem` | (纯虚接口) | DAGraphicsView | 定义可缩放 item 的契约 |
| `DAGraphicsResizeOverlayItem` | `QGraphicsObject` | DAGraphicsView | 独立 overlay，渲染控制点 + 鼠标交互 |

## 三、核心设计原则

### 3.1 Overlay 继承 target 的变换

Overlay 在 `syncToTarget()` 中继承 target 的 `pos`、`rotation`、`transformOriginPoint`。这使 Overlay 的局部坐标系与 target 完全对齐，控制点直接画在 `getBodyRect()` 的 8 个角上。**旋转感知缩放自动正确**——无需在 scene 空间做旋转数学。

### 3.2 Overlay 基类为 `QGraphicsObject`

不是 `QGraphicsItem`。`QGraphicsItem` 不继承 `QObject`，无法使用 `Q_OBJECT`/信号槽。项目现有 `DAGraphicsItem` 也继承 `QGraphicsObject`。

### 3.3 保留 `friend class DAGraphicsResizeableItem`

`DAGraphicsScene` 的 `emitItemBodySizeChanged` / `emitItemRotationChanged` 是 protected 方法，`DAGraphicsResizeableItem::setBodySize` 和 `itemChange` 通过 friend 调用它们。移除 friend 会导致编译失败。**保留 friend，同时新增 `friend class DAGraphicsResizeOverlayItem`**。

### 3.4 实时修改目标（非预览模式）

`mouseMoveEvent` 中直接调用 `target->setBodySize/setPos` 实时修改，`mouseReleaseEvent` 时创建 undo command。与当前行为一致，且连线 (`updateLinkItems`) 依赖实时 `ItemPositionHasChanged` 刷新。

### 3.5 信号签名暂不变更

Scene 信号 `itemBodySizeChanged` / `itemRotationChanged` 参数保持 `DAGraphicsResizeableItem*`，不改为 `QGraphicsItem*` 或 `DAIResizableGraphicsItem*`。减少下游波及，后续泛化。

### 3.6 XML 格式完全不变

`saveToXml` / `loadFromXml` 的 `resize-info` 节点结构保留，旧工程文件可正常加载。

### 3.7 `getBodyControlRect()` 废弃

控制点不再由 item 绘制，`getBodyControlRect()` 改为返回 `getBodyRect()`（不再膨胀）。`DAPyNodeGraphicsItem::shape()` 中的调用自动适配。

## 四、旋转感知缩放算法

Overlay 跟随了 target 的 rotation 后，鼠标事件的 `event->pos()` 是 Overlay 局部坐标，等价于 target 局部坐标。在局部空间中 `getBodyRect()` 是轴对齐的，缩放计算与无旋转时完全相同——8 个控制点的 case 分支只需修改 `newTopLeft` / `newBottomRight` 的 x 或 y 分量。

详见 `plan-01.md` §3.2.2。

## 五、受影响文件清单

**总计约 19 个文件**（3 个新增 + 16 个修改/微调），跨 4 个模块：DAGraphicsView、DAPyWorkFlow、DAGui、docs。其余全部不受影响（APP、DAFigure、DAInterface、DAPluginSupport、plugins、tests 等零影响）。

完整清单见各 plan 文件。

## 六、实施阶段与子计划

| 子计划 | 阶段 | 可编译验证 | 依赖前置 |
|--------|------|-----------|----------|
| `plan-01.md` | 新增 Overlay 基础设施（接口 + Overlay 类 + ItemType） | ✅ 新文件不影响现有代码 | 无 |
| `plan-02.md` | 精简 `DAGraphicsResizeableItem`（实现接口 + 移除控制点代码） | ✅ 子类适配后可编译 | plan-01 |
| `plan-03.md` | 修改 `DAGraphicsScene`（Overlay lifecycle + requestResize） | ✅ Scene 管理 Overlay | plan-01, plan-02 |
| `plan-04.md` | 修改 Undo Commands + Factory（参数类型适配） | ✅ 命令编译通过 | plan-01 |
| `plan-05.md` | 子类适配（TextItem / PixmapItem / NodeGraphicsItem / DAGui） | ✅ 全模块编译通过 | plan-01~04 |

## 七、执行说明

1. **逐个执行**：按 plan-01 → plan-05 顺序执行，每个 plan 执行后编译验证再进入下一个
2. **先阅读总纲**：每个子 agent 执行前必须先阅读本文件（`REFACTOR_OVERLAY_PLAN.md`）
3. **可单独执行**：每个 plan 文件包含完整的上下文、文件路径、代码示例，可独立交给 agent 执行
4. **编译验证**：每个 plan 结束后必须编译通过（`scripts/build.ps1 -Target DAGraphicsView` 或全量编译）

## 八、评审修改日志

### 第一轮评审（初始评审）

修复了 5 个 Blocker + 7 个 Suggestion：

- **B1**: `QGraphicsItem` + `Q_OBJECT` 不兼容 → 改为 `QGraphicsObject`
- **B2**: 移除 friend 破坏信号调用链 → 保留 friend
- **B3**: 旋转 body 控制点定位错误 → Overlay 继承 target 变换
- **B4**: `setRotation` 遗漏适配 → 补充 `graphicsItem()->setRotation()`
- **B5**: 坐标系混用 → 重写 `computeResize` 使用局部坐标
- **S1-S7**: 统一实时修改决策、Scene 检查 isAccepted、setBodyPos offset 清理、getBodyShape 更新、多选策略、单元测试、事件顺序验证

### 第二轮评审（逐计划深度审核）

5 个子 agent 分别审核 plan-01 ~ plan-05，共发现 12 个 Blocker + 28 个 Warning + 22 个 Suggestion，已全部修复：

**plan-01（3B + 6W + 4S）**：
- **B1**: `computeResize` 位置 delta 未做旋转变换 → 改用 `mapToParent` 转换局部 delta 到 parent delta
- **B2**: `shape()` 覆盖整个 body 阻断交互 → 改为仅包含 8 个控制点命中矩形
- **B3**: 枚举常量名 `DAGraphicsItem_Begin` → `ItemType_DAGraphicsItem_Begin`
- **W1**: paint/hitTest 位置不匹配 → 提取 `getHandleRects()` 共用
- **W2**: hover/活跃缩放状态混用 → 分离 `mHoverControlType` / `mActiveControlType`
- **W3**: 缺少网格对齐 → `computeResize` 中补充网格对齐逻辑
- **W4**: 多选策略未说明 → 添加文档说明
- **W5**: mouseReleaseEvent 重算 result → 改为读取 target 实际状态
- **W6**: cancelResize() 不恢复状态 → 实现状态恢复
- **S1-S4**: 移除未使用变量、统一接口方法使用、添加 `#include <QList>`

**plan-02（3B + 5W + 5S）**：
- **B1**: Scene 引用移除的 API → 将 Scene 控制点检测移除纳入 plan-02 范围
- **B2/B3**: `changeBodySize()`/`setControlerSize()` 调用移除的方法 → 移除 `prepareControlInfoChange()` 调用
- **W1**: `isSnapToGrid()`/`getGridSize()` 移除与保留矛盾 → 明确保留
- **W2**: TextItem mousePressEvent 调用链变化 → 添加说明
- **W3**: `adjustPosToGrid` 简化方案不明确 → 保留完整实现
- **W4**: 验证标准修正
- **W5**: 网格对齐丢失说明
- **S1-S5**: 添加 override 关键字、移除 setAcceptHoverEvents、明确 deprecated 标记、确认 getBodyShape 子类 override、统一代码风格

**plan-03（2B + 8W + 5S）**：
- **B1**: `isAccepted()` 检查破坏全局移动 undo/redo → 移除该检查
- **B2**: Overlay shape 覆盖 body 阻断 itemAt → 依赖 plan-01 B2 修复
- **W1**: target 变化时 Overlay 未同步 → 连接 xChanged/yChanged/rotationChanged 信号
- **W2**: target 删除时悬垂指针 → 连接 destroyed 信号 + null check
- **W3**: dynamic_cast 失败无防护 → 添加 null check
- **W4-W8**: selectedItems 复用、DAPyWorkFlowScene 说明、只读模式检查、多选闪烁
- **S1-S5**: friend 注释、syncToTarget 说明、Q_SLOTS 统一、运行时验证步骤

**plan-04（2B + 5W + 4S）**：
- **B1**: 第二个构造函数 `item->pos()` 未适配 → 改为 `graphicsItem()->pos()`
- **B2**: 涉及文件表遗漏 `DAGraphicsScene.cpp` → 添加
- **W1-W3**: 删除安全保护 → 保留 `if (mItem)`、`QUndoCommand::redo()`、`isValid()` 检查
- **W4**: `mHeight`→`getBodySize()` 行为变更 → 标注为有意改进
- **W5**: §4 描述误导 → 修正
- **S1-S4**: 跨计划依赖说明、mergeWith 无需修改说明、验证节点补充

**plan-05（2B + 4W + 4S）**：
- **B1**: 示例代码与实际源码不匹配 → 替换为实际源码结构
- **B2**: "基类已移除"误导 → 明确保留基类调用
- **W1**: 遗漏 createItemRotation 调用 → 补充检查清单
- **W2**: 遗漏文档更新 → 新增步骤 9
- **W3**: 变量名不一致 → 修正为实际变量名 `r`
- **W4**: TextItem 交互验证 → 添加手动测试项
- **S1-S4**: AGENTS.md 确认无需修改、错别字修正、setAcceptHoverEvents 说明、Shape 测试

### 第三轮评审（深度交叉审核）

5 个子 agent 分别审核 plan-01 ~ plan-05，共发现 4 个 Blocker + 12 个 Warning + 20 个 Suggestion，已全部修复：

**plan-01（2B + 3W + 4S）**：
- **B1**: `getGridSize()` 返回 `QSize` 非 `qreal` → 修正为分别取 `width()`/`height()`
- **B2**: 缺少 `#include "DAGraphicsScene.h"` → 添加（`qobject_cast` 需完整定义）
- **W1**: 网格对齐在 `adjustedTopLeft` 之后执行导致对角漂移 → 移到 `adjustedTopLeft` 之前
- **W2**: `mapToParent` 假设 target 为顶层 item → 添加限制说明
- **W3**: `syncToTarget()`/`paint()` 缺少 `mTarget` null check → 添加
- **S1-S4**: cancelResize 调用方说明、syncToTarget 性能优化 TODO、移除多余前向声明、接口方法用途说明

**plan-02（0B + 3W + 6S）**：
- **W1**: 非可缩放图元丢失选中边框 → paint() 中保留内联边框绘制
- **W2**: `isResizing()` 临时修复不明确 → 改为确定语气和明确指令
- **W3**: `setEnableResize()` 仍调用 `setAcceptHoverEvents` → 移除
- **S1-S6**: setBodySize 代码风格统一、移除无用 include、Q_DECL_DEPRECATED_X 仅声明处标注、prepareGeometryChange 简化、控制线功能移除说明、"保留"标签修正

**plan-03（2B + 5W + 4S）**：
- **B1**: 跨计划依赖 plan-01 `getGridSize()` 类型错误 → 添加依赖说明
- **B2**: 跨计划依赖 plan-01 mTarget null check → 改为明确前置条件
- **W1**: `onSelectionChanged()` 合并代码不完整 → 给出完整合并函数体
- **W2**: plan-02 "注释掉" vs plan-03 "移除"不一致 → 明确说明
- **W3**: 活跃缩放期间 `syncToTarget()` 被调用 3 次 → lambda 添加 `isResizing()` 检查
- **W4**: `dynamic_cast` 未标注临时 → 添加行内注释
- **W5**: `first()` vs `last()` 语义差异 → 添加说明
- **S1-S4**: syncToTarget 冗余说明、disconnect 冗余说明、信号连接自动断开说明、PrivateData 位置明确

**plan-04（0B + 3W + 3S）**：
- **W1**: `QUndoCommand::redo()/undo()` 标注"保留"但实为"新增" → 修正注释
- **W2**: §5 `onRequestResize` 代码遗漏 `syncToTarget()` → 补充
- **W3**: `mHeight`/`mWidth` 处理不明确 → 明确移除方案
- **S1-S3**: 构造函数签名修改说明、Factory cpp 示例、前向声明冗余注释

**plan-05（0B + 1W + 3S）**：
- **W1**: Step 9 文档更新遗漏 6 类已移除 API → 扩展清单（`itemBodySizeChanged` 信号、`daGlobalGraphicsResizeableItemPalette` 单例、`ControlType` 枚举、Mermaid 图、`prepareControlInfoChange` 警告框等）
- **S1**: Step 2 行号偏差 170 行 → 修正
- **S2**: plan-02 W2 vs plan-05 step 1 决策矛盾 → 统一
- **S3**: DAGui 头文件 `private slots:` 违反规范 → 标注为 out-of-scope（后续统一处理）

### 第四轮评审（深度交叉审核）

5 个子 agent 分别审核 plan-01 ~ plan-05，共发现 2 个 Blocker + 10 个 Warning + 15 个 Suggestion，已全部修复：

**plan-01（0B + 1W + 2S）**：
- **W1**: `adjustedTopLeft` switch 遗漏 `TopRight`/`BottomLeft` → 拆分为 Y/X 两个独立 switch
- **W2**: 跨计划 `getBodyPainterStartPos()` 不在接口中 → 明确标注，plan-02 移除 override
- **S1-S2**: 接口注释修正（孤儿注释清理、`getBodyTransformOriginPoint()` 描述修正）

**plan-02（1B + 3W + 4S）**：
- **B1**: `getBodyPainterStartPos() const override` 编译失败 → 移除 override，移至非接口方法区域
- **W1**: `DAGraphicsTextItem.cpp` 未列入涉及文件表 → 添加
- **W2**: §1.3 标题误导 → 拆分为接口方法和其他方法两个列表
- **W3**: `setEnableResize()` 移除了 `update()` → 保留
- **S1-S4**: 代码风格统一、重复 update 移除、testBodySize 说明明确化、plan-02/05 决策矛盾修正

**plan-03（1B + 3W + 5S）**：
- **B1**: Scene 析构函数缺少 Overlay 清理 → 新增析构函数步骤（断开 selectionChanged + 删除 overlay）
- **W1**: 只读模式下已创建 Overlay 未销毁 → `setReadOnly()` 中添加销毁逻辑
- **W2**: 非 QGraphicsObject target 限制 → 添加注释说明
- **W3**: overlay 销毁/重建未做 target 相同性检查 → 后续优化方案补充
- **S1-S5**: Q_SLOTS 合并说明、syncToTarget 冗余注释精确化、first/last 注释、QPointer include 说明、mResizeConn 重置

**plan-04（0B + 2W + 3S）**：
- **W1**: §1.3/1.4 `// ... 其余不变` 与移除 mHeight/mWidth 矛盾 → 替换为显式成员列表
- **W2**: §5 `// ...` 占位符 → 替换为完整代码（含 syncToTarget）
- **S1-S3**: Factory cpp 代码示例补充、构造函数签名示例补充、§5 参数列表完整化

**plan-05（0B + 2W + 2S）**：
- **W1**: S3 `Q_SLOTS` 修复声称落地但未实施 → 标注为 out-of-scope（后续统一处理）
- **W2**: plan-02 W2 vs plan-05 Step 1 决策矛盾 → 添加注释说明 plan-05 决定保留
- **S1-S2**: Step 9 废弃 API 清单补充（itemBodySizeChanged 信号等）、setAcceptHoverEvents 说明明确化

## 九、参考文件

- Qwt Overlay 头文件: `src/3rdparty/qwt/src/plot/qwt_figure_widget_overlay.h`
- Qwt Overlay 实现: `src/3rdparty/qwt/src/plot/qwt_figure_widget_overlay.cpp`
- 当前 ResizeableItem: `src/DAGraphicsView/DAGraphicsResizeableItem.h` / `.cpp`
- Scene: `src/DAGraphicsView/DAGraphicsScene.h` / `.cpp`
- Commands: `src/DAGraphicsView/DACommandsForGraphics.h` / `.cpp`
- Factory: `src/DAGraphicsView/DAGraphicsCommandsFactory.h` / `.cpp`
- 可缩放图元文档: `docs/zh/dev-guide/scalable-graphic-module.md`
