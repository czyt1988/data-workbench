# 滑动窗表格的导航协议

表格数据源抽象层（[表格数据源抽象层](table-data-source.md)）解决了「数据不全量驻留内存」的问题；这一页讲的是它的另一半：**在一亿行级别的表上，滚动、键盘、翻页、定位这套导航语义怎么保持精确**。相关实现集中在 `DACacheWindowTableView` 与 `DAVirtualScrollBar`（DAGui）。

## 为什么需要一套协议

滑动窗（`DAAbstractCacheWindowTableModel`，默认 20000 行）把 Qt 看到的数据量压到窗口大小以内，代价是：**模型的行号不再是数据源的行号**，中间隔着一个窗口起始行。

如果把这个偏移随手加在业务代码里，会出现两个反面案例，它们都真实发生过：

- **把纵向滚动条的 value 当成「整表进度」**：滚动条的 value 在 Qt 里被硬性要求是「窗口内首个可见行」——`QHeaderViewPrivate::setScrollOffset()` 会用它反算表头的像素偏移（`setOffsetToSectionPosition(value)`）。若再把这个 value 当比例去换算窗口起始行，那么每一次细粒度滚动（方向键到视口边缘、翻页、滚轮、轨道点击）都会被放大成 `总行数 / 窗口行数` 行：一亿行的表上按一下方向键跳 5000 行，翻页键跳十几万行。
- **用「视图行号」直接写数据**：选中项给的是窗口内行号，若拿它去改 DataFrame / 样式表 / undo 命令，滚动之后就会写到别的行上去。

协议要解决的正是这两类问题。

## 位置模型

只有一个状态，一个不变量：

```text
T（视口首行的绝对行号） = W（窗口起始行 getCacheWindowStartRow()） + v（纵向滚动条 value）
```

- `v` 永远表示**窗口内行号**，这是 Qt 的固定语义：方向键、滚轮、翻页、拖拽选择的自动滚动都直接改 `v`，因此天然精确到行。
- `W` 变化时，`v` 必须同步反向变化，否则 `T` 会漂移。`DAAbstractCacheWindowTableModel::setCacheWindowStartRow()` 是唯一入口。
- 绝对行（数据源行号）与逻辑行（窗口内行号）的换算只有 `toActualRow` / `toLogicalRow` 两个函数。

## 缓冲区搬家（窗口重定心）

窗口是**数据缓冲**，不是坐标系。滚动落在缓冲区内时只改 `v`，不搬窗口；只有当 `v` 逼近缓冲区的上/下边缘（`maxV/8` 的边带）时才整体平移窗口，并且平移保持 `T` 不变：

```text
W' = W + Δ    v' = v - Δ    ⇒    T' = W' + v' = T
```

屏幕内容一动不动，只是缓冲搬到了数据源的新位置。这一步让方向键可以无限逐行遍历整表——缓冲区永远不会「到头」。每次平移的代价是一次窗口级 `dataChanged`，与一次普通滚动同量级；按默认窗口（20000 行）算，约每滚 5000 行触发一次。

## 搬窗口时必须同步的三件事

窗口一动，模型映射就变了，任何持有逻辑行号的 Qt 内部状态都会指向别的数据。`DACacheWindowTableView` 按下面的顺序处理：

1. **先提交编辑器**：编辑器持有的是逻辑行，窗口一搬它就会写到别的行上。提交不掉（找不到编辑器部件）时延后本次搬家（`mPendingRecenter`）。
2. **先改滚动条 value，再改窗口**：这样 `(W, v)` 在任何时刻都自洽，Qt 在中间步骤里重算表头偏移也不会算出错误内容。
3. **同步当前项与选区**：整体平移同一个增量，让它们仍指向同一批绝对行。当前项只是滚出视口（很常见）时，摆位会顺手把它留在缓冲区内；只有它离视口超过一个缓冲区时才放弃，把光标落到视口首行同列，保证键盘操作有落点。

> **⚠️ 必须临时关掉「当前项变化自动滚动」**
>
> `QAbstractItemView::currentChanged()` 在 `autoScroll` 打开时会 `scrollTo(current)`。搬窗口时我们正是要程序化改当前项的逻辑行，若不禁用，Qt 会把视口拽回旧位置，`v` 被改写成别的值，`T` 就漂了。搬窗口的代码块因此包在 `setAutoScroll(false)` / 恢复之间。

> **📝 已知取舍**
>
> - 选中区跨出窗口缓冲区的部分会被截掉（一亿行表上不可能在模型里表达「全表选区」）。
> - `Shift` 连续选择用的锚点是 Qt 私有的（`QAbstractItemViewPrivate::currentSelectionStartIndex`），窗口搬家后它可能陈旧；影响面很小，暂不处理。
> - 横向没有虚拟化，列相关逻辑不涉及行号偏移。

## 纵向滚动条：两层语义

`DAVirtualScrollBar` 解决的矛盾是：Qt 需要 value 是窗口内行号，用户需要滑块表示整表位置。做法是让**同一个控件承担两层语义**：

- 继承来的 `value` / `range` / `pageStep` 保持窗口内语义（Qt 的滚动机制照旧工作，`setVerticalScrollBar` 替换实例即可）。
- **绘制与鼠标/键盘交互**按整表坐标：构造一份「假坐标」的 `QStyleOptionSlider`（最大值 = 总行数 - 1、页步 = 一屏行数、滑块位置 = `T`）交给 `QStyle::drawComplexControl`，于是原生样式画出的就是诚实的整表位置；命中判定用 `hitTestComplexControl` 配同一份假坐标，于是「滑块 / 上箭头 / 下箭头 / 轨道」的点击区域也和画出来的一致。
- 交互结果通过信号交给视图执行：`seekRequested(目标行)`（拖动滑块，目标行置顶）、`stepRowsRequested(±1)`、`pageRowsRequested(±屏)`、`gotoRowRequested()`（右键菜单「定位到行」）。拖动过程用 `QToolTip` 显示「第 N 行 / 共 M 行」，拖动时记录抓取点偏移，滑块不会跳。
- 滚动条**自己不改窗口位置**，只发请求；定位一律经视图的 `placeViewportAt()`。

> **ℹ️ 比例拖动的精度上限**
>
> 拖动滑块是比例映射，精度受轨道像素数限制：一亿行、约 350 像素的轨道，1 像素 ≈ 28 万行。这是所有比例滚动条的固有事实（Excel 在 104 万行上同理）。**精确落点靠三样东西**：方向键（±1 行）、翻页键（±1 屏）、定位对话框（`Ctrl+G` 或滚动条右键「定位到行…」，直接输入行号）。滚动条的职责是「大致到达 + 告诉我现在在哪」。

## 键盘与滚轮语义

| 操作 | 语义 | 实现位置 |
| --- | --- | --- |
| 方向键 | ±1 行（跨缓冲区不跳行） | Qt 原生（缓冲区搬家由 `recenterWindowIfNeeded` 兜底） |
| 滚轮 | 系统行数（默认 3 行/格）；`Ctrl`/`Shift`+滚轮 = 一屏 | Qt 原生 |
| `PageUp` / `PageDown` | 精确一屏，**光标跟随翻页**（保持它在视口中的相对位置） | `keyPressEvent` |
| `Home` / `End` | 当前行的首列 / 末列，纵向位置不动 | `keyPressEvent` |
| `Ctrl+Home` / `Ctrl+End` | 数据首行首列 / 末行末列 | `keyPressEvent` |
| `Ctrl+G` | 「定位到行」输入框 | `keyPressEvent` |
| 滚动条拖动 | 整表比例定位，目标行置顶 | `DAVirtualScrollBar` → `placeViewportAt` |
| 滚动条上下箭头 / 轨道点击 | ±1 行 / ±1 屏（按住可连续） | `DAVirtualScrollBar` → `moveViewport` |

定位（`showActualRow` / `placeViewportAt`）统一把目标行放到**视口首行**，并在窗口里给它上方留约 1/4 窗口的余量，保证定位之后两个方向都还能继续滚；目标行靠近数据尾部、下面凑不满一屏时贴到最后一屏，此时目标仍然可见。

## 行号口径（数据操作铁律）

> **⚠️ 数据操作一律用绝对行**
>
> - 模型侧：`actualData` / `setActualData` / 全部 `notify*` 通知的参数都是**数据源绝对行**；模型内部负责换算到窗口内行号后再发 Qt 信号。
> - 视图侧：`toActualRow` / `toLogicalRow` 是唯一边界。
> - 界面侧：`DADataOperateOfDataFrameWidget` 里所有由选区派生的行号（`getSelectedDataframeRows` / `getSelectedOneDataframeRow` / `getSelectedDataframeCells` / `getFullySelectedDataframeRows`）返回的都是**绝对行**，它的私有 `toActualRow()` 是唯一换算点；插删行、粘贴、设为 NaN、样式键都直接用返回值，不要再自己加窗口偏移。
> - 列号没有虚拟化，不受影响。

## 相关实现与测试

| 关注点 | 位置 |
| --- | --- |
| 视图导航、缓冲区搬家、键位 | `src/DAGui/DACacheWindowTableView.h/.cpp` |
| 整表坐标滚动条 | `src/DAGui/DAVirtualScrollBar.h/.cpp` |
| 窗口起始行与合法上界、绝对行通知 | `src/DAGui/Models/DAAbstractCacheWindowTableModel.h/.cpp` |
| 块级取数 | `src/DAGui/Models/DADataTableModel.cpp`（`ensureBlockCached`） |
| 导航协议回归测试 | `src/tst/DATableNavigationTest/`（窗口不变量、逐行遍历、翻页、滚轮、定位、拖动、搬窗后视图状态、行号口径、结构变化自愈） |

```bash
# Windows
.\scripts\build.ps1 -Target DATableNavigationTest -Test
```

## 参考资料

- [表格数据源抽象层](table-data-source.md)：`DATableDataSource` 契约与惰性数据源接入
- `docs/zh/dev-guide/architecture/module-breakdown.md`：DAGui 模块职责
