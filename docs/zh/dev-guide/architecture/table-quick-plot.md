# 表内快速绘图（快速图表预览）

> **📌 定位**
>
> 在看表的过程中选中一列/多列，立刻在表格页上弹出一个真正的图表子窗口；子窗口可以随手关掉，也可以一键转入绘图区变成正式图表。
>
> 方案来源：仓库根目录 `tmp/表格相关实施方案/表格快速图表预览实施方案.md`（v2.1：数据流/采样/转入模型）
> 与 `tmp/表格相关实施方案/表格快速绘图改造实施方案.md`（v3：右键子菜单、设置区、12 图型与 3D）。
> v4 起设置区移出浮窗：浮窗只保留绘图区，全部设置项集中在右侧「设置」Dock 的快速绘图设置页。
> 本文只写实现落位与扩展点，规则细节以方案文件为准。

---

## 1. 分层与文件落位

| 层 | 文件 | 职责 |
|---|---|---|
| L2 数据层 | `src/DAData/DATableDataSource.h` | `fetchColumnRange(column, startRow, rowCount)`：按列取一段值；pandas 源走列切片快路径，其它源默认经 `fetchBlock` 抽取 |
| L3 界面层 | `src/DAGui/Table/Views/DATableChildWindow.*` | **表格子窗口框架**（Excel 式浮层）：标题栏拖动移动、4 角抓取块 + 4 边拖拽带缩放、双击最大化/还原、标题栏右键菜单 |
| L3 界面层 | `src/DAGui/Table/Views/DATableChildWindowContainer.*` | **表格子窗口容器**：任意 QWidget 贴入成表格里的子窗口；允许区域夹取、几何源监听、关闭默认隐藏 |
| L3 界面层 | `src/DAGui/Table/Chart/DATableChartTypes.h` | 请求/规格/规划结果/构建信息等纯数据结构 |
| L3 界面层 | `src/DAGui/Table/Chart/DATableChartPlanner.*` | 列角色判定、图型能力矩阵、默认规格、X 轴候选 |
| L3 界面层 | `src/DAGui/Table/Chart/DAChartSeriesSampler.*` | 列 → 采样点/值的唯一采样入口（小表全量、大表确定性分块） |
| L3 界面层 | `src/DAGui/Table/Chart/DAChartQuickPlot.*` | **唯一图元编排器**：按规格构建图元并挂到图表；转入绘图区 |
| L3 界面层 | `src/DAGui/Table/Chart/DATableQuickPlotWindow.*` | **绘图区部件**：只承载 2D/3D 图表堆叠区（位置/尺寸/标题由子窗口框架承载）；负责取数绘制、把状态回推设置页、接收设置页回写的请求/规格 |
| L3 界面层 | `src/DAGui/Common/Settings/DASettingContainerWidget.*` | 右侧「设置」Dock 的页面容器（QStackedWidget）：挂工作流节点/绘图/3D/Figure/快速绘图设置页，按当前编辑对象切页 |
| L3 界面层 | `src/DAGui/Table/Chart/DATableQuickPlotSettingWidget.*` | **快速绘图设置页**（右侧「设置」Dock 内）：图型下拉、按图型动态生成的角色行、更新为当前选区/刷新/转入绘图区、只读采样/错误提示；单实例绑定一个绘图区 |
| L3 界面层 | `src/DAGui/Table/Chart/DATableChartRolePanel.*` | 角色行生成器（QObject）：按 12 种图型把「角色名 + 列下拉」行写进设置页「数据角色」分组的通用属性面板（`setRowPanel()`），换列即时重绘、可换整表任意列 |
| L3 界面层 | `src/DAGui/Table/Controllers/DATableQuickPlotController.*` | 选区→请求、绘图区与子窗口生命周期、设置页绑定/解绑时机（经 APP 注入的 binder/releaser）、转入处理与关闭 |
| L3 界面层 | `src/DAGui/Chart/DAStatsPlotCoordinator.*` | 统计绘图核心（原在 APP，下沉到 DAGui 供统计向导与快速绘图共用） |
| L3 界面层 | `src/DAGui/Chart/DAChartOperateWidgetCommands.*` | 「转入」新建 Figure 的撤销命令（摘/挂 dock 但保活 figure） |
| L3 界面层 | `src/DAGui/Table/DADataOperateOfDataFrameWidget.*` | 表格页入口：有序选区 → 请求 → 贴入子窗口；转入处理器由 APP 注入 |
| L5 应用层 | `src/APP/Controller/DAAppControllerPage.cpp` | Ribbon/表头动作接线；注入转入处理器、追加可用性与设置页绑定/解绑器（`setQuickPlotSettingBinder` / `setQuickPlotSettingReleaser`：绑定 + 切页 + `raiseDockByWidget`）；动作置灰 |

> **⚠️ 依赖方向**
>
> 快速绘图全部落在 DAGui，不反向依赖 APP。转入绘图区需要 `DAChartOperateWidget`（也在 DAGui，但只有 APP 知道它属于哪个停靠区），因此由 APP 通过
> `DADataOperateOfDataFrameWidget::setQuickPlotTransferHandler()` 注入一个处理器，页面只负责调用。
>
> 快速绘图设置页虽然在 DAGui 的 `DASettingContainerWidget` 里，但承载它的「设置」Dock 由 APP 管理、DAGui 无法自己抬起，因此「绑定设置页 + 切页 + 抬起 Dock」同样由 APP 经
> `setQuickPlotSettingBinder()` / `setQuickPlotSettingReleaser()` 注入；DAGui 只负责在窗口激活/隐藏时发 `activated` / `hidden` 信号。

---

## 1.5 表格子窗口容器（DATableChildWindowContainer）

快速绘图子窗口不再自管位置与尺寸，由通用子窗口容器承载——**任意**部件都能贴入表格变成子窗口：

- **贴入/摘除**：`attach(content, title, preferredSize, preferredPos)` 把任意 QWidget 重父化进一个
  `DATableChildWindow` 框架（框架接管内容所有权），`takeAndDetach` 取出内容并销毁框架；
- **交互（Excel 式）**：标题栏拖动移动、双击最大化/还原（充满允许区域）、4 角抓取块 + 4 边拖拽带
  缩放、标题栏右键菜单（最大化/还原/重置位置与大小/关闭）、Esc 请求关闭；
- **允许区域夹取**：`setBoundsProvider` 给出允许矩形（页面用**主视口**：数据网格可视区，不含表头与
  滚动条）——移动/缩放全程夹取其中，浮层永不压住滚动条与表头；`watchGeometrySource(视口/窗格容器)`
  监听几何源，滚动条显隐、冻结分栏位移、页面缩放后全部子窗口自动夹回；
- **关闭语义**：容器把 `closeRequested` 默认接到 `hide`（实例复用）；需要销毁的宿主自行接 `detach`；
- **快速绘图的用法**：页内单实例，贴在主视口右上角；关闭 → 重开保持用户上次的位置与大小（不再重置），
  标题栏由框架绘制（列名摘要经 `titleChanged` 同步）；框架的 `activated()`（标题栏/缩放手柄/内容区按下）
  与绘图区自身的 `activated()` 一起接到控制器，用于抬起右侧「设置」Dock 的快速绘图设置页。

> **📌 为什么不用 QSizeGrip**：QSizeGrip 只作用于顶层窗口（`startSystemResize` 作用于 `window()`），
> 表格页内的子控件浮层用它拖不动自己（旧实现「无法调整大小」的根因）。子窗口框架必须自绘 8 向手柄。

---

## 2. 一条数据流

```text
表格选区（按视图顺序 + 隐藏列跳过）
  → DATableChartRequest（数据集句柄 + 有序选中列 + 角色 + 行数/惰性标记）
  → DATableChartPlanner::plan()（可选图型 + 默认规格 + 拒绝原因）
  → DATableChartSpec（图型 / X / 数值列 / 分类列 / 聚合 / Top N）
  → DAChartSeriesSampler（采样）
  → DAChartQuickPlot::render()（构建图元 → attach 到目标 DAChartWidget）
  → 绘图区显示（容器贴入框架），或 transferToFigure() 转入正式图表
  → 重画/状态变化后：contextChanged / statusChanged（把图型/角色/手动模式与采样/错误状态回推右侧设置页）
```

绘图区与正式图表**共用同一份**请求、采样与构建实现；转入不是把绘图区的 chart reparent 过去，而是用同一份规格再构建一次图元。

设置页的编辑走反方向：设置页只调用绘图区的公开 API（`setChartType()` / `applyEditedContext()` / `refresh()` 等），
绘图区按新状态重画，并把最新的结构（图型、角色、手动模式）经 `contextChanged()`、只读状态（采样提示、错误、数据变化）
经 `statusChanged()` 回推设置页——**状态只有绘图区一个来源**。

v3 起请求额外携带 `allColumns`（整表列清单，只按静态类型判定角色）：设置页的列下拉据此可换整表任意列，
用户选到选区外的列时由角色面板把它追加进 `request.columns`，规格仍用「请求列下标」表达。

右键「快速绘图」子菜单与设置页图型下拉共用 `DATableChartPlanner::plan()`：

- 子菜单**只列当前选区能画的图型**（严格角色门槛），首项为默认图型；直接绘图段与聚合绘图段用分隔线分组；
- 设置页图型下拉列**全部 12 种图型**，不满足当前选区的置灰，悬浮提示缺什么（`chartTypeRequirementText()`）。

---

## 3. 图型能力矩阵（v3，12 种图型）

| 选区角色 | 子菜单出现（首项为默认） |
|---|---|
| 1 列数值 | 折线、直方图、箱线图 |
| 1 列分类 | 柱状（计数） |
| 2 列数值 | 散点、折线、箱线图、柱状（低基数数值「作为分类」） |
| 时间 + 数值 | 折线（x=时间）、散点 |
| 分类 + 数值 | 柱状（聚合，mean）、箱线图（按类别） |
| 3 列数值 | 散点、多序列折线、箱线图、**等高线、误差棒、3D 曲面、3D 线图** |
| 时间 + 2+ 数值 | 多序列折线（时间列不参与 3D/等高线坐标轴） |
| 4+ 列数值 | 上述 3 列项 + **向量场** |
| 分类 + 2+ 数值 | **多重柱状**（v3 新增）、箱线图（按类别） |
| 2 分类 / 单列时间 / 无数值 | 不出图（菜单置灰并给出可读原因） |

- **严格角色门槛**：3 列数值才出等高线/误差棒/3D 曲面/3D 线图，4 列数值才出向量场；
  多出来的列不阻碍出图（默认取前 N 个数值列，可在设置页更换）；
- 分类聚合口径默认 `mean`，可切 `min/max/sum/median/count/std/var`；
- 分类柱状默认按聚合值降序；唯一值超过 50 时只显示 Top 30 并在设置页标注；
- 箱线图固定 `whis=1.5`、显示离群点、无 CI、无图例。

---

### 3.1 浮窗结构

浮窗拆成两个界面：表格页里的绘图区，和右侧「设置」Dock 里的设置页。

```text
表格页内：DATableChildWindow 框架
┌ 标题栏（框架绘制）：列摘要 [（手动配置）]                          ✕ ┐
└ 绘图区：2D（DAChartWidget）/ 3D（DAChart3DWidget）堆叠，占满内容区  ┘

右侧「设置」Dock：DATableQuickPlotSettingWidget
└ 通用属性面板（DAPropertyPanelContainerWidget，与图表设置页同一套）
   ├ 图表（折叠组）：[图型 ▼]（12 种图型，不可用项置灰）、列摘要（只读）
   ├ 数据角色（折叠组）：按图型动态生成的角色行（列/聚合/分箱，多序列可增删）
   └ 底栏：采样/错误只读提示 + [⭮ 更新为当前选区] [↻ 刷新] [□→ 转入绘图区 ▾]
```

- 绘图窗口只保留绘图区（标题栏由子窗口框架绘制）：原工具栏、设置区与状态行全部移入设置页；
  推荐尺寸 560×420（原 640×560）、最小 320×220，图表铺满整个内容区；
- 点在绘图区（图表画布/3D 视图）或子窗口框架（标题栏/缩放手柄/内容区）上会触发 `activated()`，
  宿主随即把右侧设置 Dock 抬到顶层并切到快速绘图设置页——不需要用户先去右侧找设置；
- 设置页主体复用图表/工作流设置页的通用属性面板（`DAPropertyPanelContainerWidget`：可折叠分组 +
  `DAPropertyItemWidget` 属性行），行高、属性名列宽、分组样式与图表设置页完全一致；
- 设置页图型下拉列全部 12 种；不满足当前选区的置灰，悬浮提示缺什么；
- 聚合、X 轴等角色配置只在设置页出现；角色行换列即重绘，并进入**手动模式**：标题后缀「（手动配置）」；
- 点「更新为当前选区」让绘图区回填表格当前选区（窗口发 `currentSelectionRequested` 给控制器）并退出手动模式；
  「刷新」按当前数据立即重画，「转入绘图区」新建 Figure 或追加到当前图表；底栏按钮均带图标
  （更新选区用新增的 `updateSelection.svg`，刷新复用 `refresh.svg`，转入复用统计窗口的
  `stats/statsTransferToFigure.svg`，菜单项复用 `chart.svg` / `right-add.svg`）；
- 角色行仍由轻量的 `DATableChartRolePanel` 生成，但它只负责把「角色名 + 列下拉」行写进「数据角色」分组的
  内嵌属性面板（`setRowPanel()`），不嵌入添加绘图向导页面（向导页没有配置变化信号且按对话框尺寸设计）。

### 3.2 3D 图型

- 绘图区是 `QStackedWidget`，2D `DAChartWidget` 与 3D `DAChart3DWidget` 按图型切换；
  `DAChart3DWidget` 懒创建（首次切到 3D 才建 GL 上下文）；
- 3D 预览沿用 2D 的采样预算（约 2000 点，采样说明显示在设置页状态区）；
- 3D 曲面固定「散点 + scipy 插值 30×30 网格」；scipy 不可用时不出图并在设置页给出可读原因；
- 3D 线图直接用采样后的 `(x, y, z)` 轨迹；
- 转入绘图区走 `DAChartQuickPlot::transferToFigure3D()`：新建/复用 Figure 的 3D 图表并全量重建图元；
- 3D 曲面/柱状的「整表作矩阵」网格模式不进快速绘图（与选区模型不符，属添加绘图）。

### 3.3 设置页（右侧 Dock）

设置页 `DATableQuickPlotSettingWidget` 是 `DASettingContainerWidget` 里的一个单例页（与工作流节点/绘图/3D/Figure 设置页同一个
`QStackedWidget`），由容器初始化时创建并 `addWidget`，经 `getQuickPlotSettingWidget()` / `showQuickPlotSettingWidget()` 取得与切换。
它的生命周期是「一次只绑定一个绘图区」：

- **绑定**：`setTarget(window)` 断开旧目标、连上新目标；重复绑定同一窗口只重新同步一次（宿主每次抬起页面都会调用，保证显示的是最新状态）；
- **解绑**：`releaseTarget(window)` 仅当传入窗口就是当前目标时清空（避免别的表格页误清当前绑定）；目标被销毁时 `destroyed` 也会自动清空；
- **空态**：没有目标或目标不可见时显示「请先在数据表格中打开快速绘图，再在此处设置」，设置内容整体隐藏；
- **界面组成**：主体是 `DAPropertyPanelContainerWidget`（与图表设置页同一套属性面板：可滚动、可折叠分组、
  `DAPropertyItemWidget` 属性行，行高/属性名列宽/分组样式由面板统一）；两个分组——
  「图表」放图型下拉与只读列摘要，「数据角色」放角色行；底部固定底栏放只读提示与三个动作按钮（带图标）；
- **图型下拉**：自定义 `QComboBox`（条目不可用置灰 + 悬浮原因），因此用 `addProperty()` 作为属性行编辑器加入，
  而不是 `addEnumProperty()`；
- **角色行**：`DATableChartRolePanel` 在设置页创建「数据角色」分组后拿到该分组的内嵌属性面板
  （`DAPropertyPanelContainerWidget::getGroupPanel()`），通过 `setRowPanel()` 把「角色名 + 列下拉」行
  `addProperty()` 进去；换图型时先按记录的属性 ID 逐个 `removeProperty()` 再重建（`deleteLater`，可在下拉框自己的信号栈上安全调用）；
- **同步契约**：绑定后监听绘图区两个信号——`contextChanged()` 做整体同步（列摘要、图型下拉的可用状态与当前项、角色行），
  `statusChanged()` 只刷新状态文字与按钮；解绑时断开全部连接。设置页自身的编辑（图型、角色行）经窗口公开 API 回写并触发重绘，
  回写期间不重建角色行，避免正在操作的下拉框被销毁；
- **驱动信号**：绘图区本体、图表子控件或子窗口框架被按下时发 `activated()` → 控制器 `activateSetting()` → APP binder；
  绘图区隐藏（关闭/切页）时发 `hidden()` → 控制器 `releaseSetting()` → APP releaser；
- **APP 注入**：设置页在 DAGui，但「设置」Dock 由 APP 的停靠系统管理，DAGui 不能自己抬起 Dock；APP 在
  `onDataOperatePageCreated` 里注入 binder（`setTarget` + `showQuickPlotSettingWidget` + `raiseDockByWidget`）与 releaser
  （`releaseTarget`），控制器负责在打开、点击、隐藏与析构时调用。

> **📌 为什么不用 DAFormSpec/DAPropertyFormWidget**
>
> 角色行随图型动态变化（多序列折线/多重柱状要增删行，每行还有列、聚合、分箱等不同控件，
> 列候选还依赖当前请求的运行期数据），静态表单规格表达不了。因此角色行由 `DATableChartRolePanel`
> 按图型动态生成，但**行本身写进通用属性面板**（`DAPropertyPanelWidget` 的 `addProperty()`），
> 折叠分组与属性行都是图表设置页同款封装，不做自绘行、不自造控件外观；固定表单项（图型、列摘要）同样走 `addProperty()`。

## 4. 采样与精确性

| 场景 | 策略 |
|---|---|
| 小表（≤ 5000 行） | 全量绘制，不标注采样 |
| 大表折线/散点 | 确定性分块均匀采样，约 2000 点（首尾都覆盖，不引入随机数） |
| 大表直方图 | 约 2 万个值 |
| 分类聚合 / 箱线图 | 内存 pandas 源且 ≤ 500 万行 → **全量精确**；惰性源或超阈值 → 采样并在设置页标注近似 |
| 取数预算 | `DAChartSeriesSampler::SampleBudget`：默认最多 128 次取数调用 / 25 万行，同一次构建的各列共用一个预算；耗尽即停止采样并置 `SampleInfo::limited`，`DATableChartBuildInfo::sampleLimited` 聚合后由设置页标注「样本受限」 |
| 忙碌状态 | `DAChartQuickPlot::render()` 全程持有等待光标；绘图区在取数前禁用交互，设置页状态区显示「正在取数…」，取数结束（含失败）后恢复 |

精确路径走 `DAPyScriptsStatistics` 的 DataFrame 接口（按列序号取列，重名列不歧义）；
采样路径把采样后的标签/数值交给 `aggregate_by_category_from_lists` / `compute_boxplot_stats_from_lists`，
仍然是同一套 Python 聚合/分位数实现，不存在第二套 group by。

> **⚠️ 统计向导的估计器口径**
>
> 统计绘图向导的分组柱状图参数面板（`DAChartAddStatsBarplotWidget`）已同步补上 `min` / `max`
> （默认仍是 `mean`）。该下拉的**显示文本会被翻译**，所以估计器 key 必须放在 item 的
> `userData` 里、用 `currentData()` 读取；用 `currentText()` 会在中文界面把 median/sum/std/var
> 全部落到 `ESTIMATOR_MAP` 的默认值 mean。uic 不为 combo item 生成 `setItemData`，因此 key
> 在构造函数里按下标写入（顺序须与 `.ui` 一致），`DAStatsGuideTest` 对此有回归用例。

---

## 5. 转入绘图区的撤销模型

一次「转入」= 一个新 Figure 自己的撤销宏：

```text
QUndoStack::beginMacro("quick plot")
  DAChartOperateWidgetCommandDetachFigure   // undo: 摘 dock 但保活 figure；redo: 挂回
  DAFigureWidget::createChart_()            // 既有命令
  DAFigureWidget::addItem_(chart, item, true)  // 既有命令（已 attach，只登记）
endMacro()
```

- 一次 Ctrl+Z 撤销整个转入；Ctrl+Y 整体恢复；
- `DAChartOperateWidget::detachFigureDock()/reattachFigureDock()` 只摘/挂 dock，不销毁 figure，
  因此图元、chart 与命令都还在；
- 「追加到当前图表」不新建 figure，只把新图元逐个登记为既有 `addItem_` 命令；
- 3D 图型走 `transferToFigure3D()`：宏内用 `create3DChart_()` + 先 `attach` 再
  `add3DItem_(chart3d, item, true)`（与 2D 的「已 attach，只登记」同一模式）；
  「追加」时复用当前 Figure 的当前 3D 图表，没有则在同一 Figure 内新建。

---

## 6. 扩展点

- **新增设置项**：控件一律加在设置页 `DATableQuickPlotSettingWidget` 的通用属性面板里
  （`mPanel->addCollapsibleGroup()` → `mPanel->addProperty(name, editor)` → `endGroup()`，
  与图表设置页写法一致；编辑器记得 `setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed)` 才会横向填满），
  **不要**把控件加回绘图区 `DATableQuickPlotWindow`；设置页只调用绘图区的公开 API 驱动（`setChartType()` /
  `applyEditedContext()` / `refresh()` / `requestTransfer()` 这类），不要直接操作窗口内部的图表。
  绘图区状态改完必须发 `contextChanged()`（图型/角色/手动模式等结构变化）或 `statusChanged()`（采样提示/错误/数据变化），
  设置页据此刷新；需要新的窗口能力时先在 `DATableQuickPlotWindow` 上加 public 方法，再让设置页调用。
- **新增图型**：在 `DATableChartType` 加枚举、`DATableChartPlanner::plan()` 的矩阵与
  `defaultSpec()` 里登记、在 `DATableChartRolePanel::rebuildRows()` 里加角色行、在
  `DAChartQuickPlot` 里加一个 `build*()` 分支（3D 图型走 `create3DItem()/render3D()`）；
  图元一律复用既有添加绘图页（如 `DAChartAddContourWidget` 的 `setProgrammatic*` 入口）
  或 `DAChartPlotRenderer`，不要出现第二处 `new QwtPlot*`。设置页的图型下拉与角色行会按
  `DATableChartPlanner::plan()` 自动跟上，无需单独改设置页。
- **新图型的编程式入口**：无界面调用方需要的添加页补 `setProgrammaticXxx()`，
  让 `createPlotItem()/create3DPlotItem()` 在编程式数据非空时跳过界面取值；
  取数仍走 `DAChartSeriesSampler`，不在页面里重复采样。

> **📌 关于 `DATableChartType`**
>
> 它不是新的图表类型系统：`DA::DAChartTypes` 仍是图型身份的唯一枚举（决定用哪个添加页/渲染管线）。
> `DATableChartType` 只描述「设置页里用户能选的那几个预览图型」，是选层而非身份层；
> 选中的每一项最终都落到既有管线：折线/散点/多序列走 `DAChartAddCurveWidget`，
> 直方图走 `DAChartAddHistogramWidget`，分类柱状与箱线图走统计管线的 `DAChartPlotRenderer`。
> 两者不能 1:1 映射（设置页的「柱状」是统计 barplot 管线，而 `DAChartTypes::Bar` 是数值 X 的柱状），
> 所以没有也不应该有转换函数。
- **新增聚合口径**：`DATableChartAggregation` + `DATableChartPlanner::aggregationKey()` +
  Python `ESTIMATOR_MAP`。
- **多序列分类柱状 / 2 分类交叉柱状**：统计管线已有 hue 能力，缺的是交互与基数守卫，留待后续。

---

## 7. 已知限制

- 子窗口不随工程保存、不进撤销栈、不导出图片；这些能力在转入后由绘图区提供。
- 子窗口不自动跟随选区与数据变化，只提供「更新为当前选区」与「刷新」；位置与大小也不持久化，页内关闭重开保持上次几何，数据页重建后回默认。
- 快速绘图设置页是单例，只绑定当前活动的快速绘图窗口：每个数据页同时只有一个快速绘图（页内单实例），
  关闭绘图区或切到别的数据页后设置页回到空态，下次在该页打开或点击绘图区时重新绑定；设置页不能同时展示多个快速绘图的设置。
- 惰性/数据库源目前只能采样聚合（无 SQL 聚合下推）。
- 「低基数数值作为分类」用采样（起始 2000 行）估计基数，极端分布下可能误判；真正的 Top N 守卫按聚合结果执行。
- 3D 曲面依赖 scipy 插值；无 scipy 时只给可读原因、不出图。
- 多重柱状对每个数值列各做一次分类聚合（精确路径为每列一次 pandas group-by）；列很多时属于已知的耗时点。
- 「2 分类交叉柱状」仍不支持（统计管线 hue 能力缺交互与基数守卫）。
