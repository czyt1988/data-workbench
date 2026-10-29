# 表内快速绘图（快速图表预览）

> **📌 定位**
>
> 在看表的过程中选中一列/多列，立刻在表格页上弹出一个真正的图表浮窗；浮窗可以随手关掉，也可以一键转入绘图区变成正式图表。
>
> 方案来源：仓库根目录 `tmp/表格相关实施方案/表格快速图表预览实施方案.md`（方案文件，不在文档站导航内）。
> 本文只写实现落位与扩展点，规则细节以方案文件为准。

---

## 1. 分层与文件落位

| 层 | 文件 | 职责 |
|---|---|---|
| L2 数据层 | `src/DAData/DATableDataSource.h` | `fetchColumnRange(column, startRow, rowCount)`：按列取一段值；pandas 源走列切片快路径，其它源默认经 `fetchBlock` 抽取 |
| L3 界面层 | `src/DAGui/TableChart/DATableChartTypes.h` | 请求/规格/规划结果/构建信息等纯数据结构 |
| L3 界面层 | `src/DAGui/TableChart/DATableChartPlanner.*` | 列角色判定、图型能力矩阵、默认规格、X 轴候选 |
| L3 界面层 | `src/DAGui/TableChart/DAChartSeriesSampler.*` | 列 → 采样点/值的唯一采样入口（小表全量、大表确定性分块） |
| L3 界面层 | `src/DAGui/TableChart/DAChartQuickPlot.*` | **唯一图元编排器**：按规格构建图元并挂到图表；转入绘图区 |
| L3 界面层 | `src/DAGui/TableChart/DATableQuickPlotWindow.*` | 浮窗：图型切换、聚合/X 轴下拉、更新选区、转入、关闭 |
| L3 界面层 | `src/DAGui/Stats/DAStatsPlotCoordinator.*` | 统计绘图核心（原在 APP，下沉到 DAGui 供统计向导与快速绘图共用） |
| L3 界面层 | `src/DAGui/Chart/DAChartOperateWidgetCommands.*` | 「转入」新建 Figure 的撤销命令（摘/挂 dock 但保活 figure） |
| L3 界面层 | `src/DAGui/DADataOperateOfDataFrameWidget.*` | 表格页入口：有序选区 → 请求 → 浮窗；转入处理器由 APP 注入 |
| L5 应用层 | `src/APP/DAAppController.cpp` | Ribbon/表头动作接线、`setQuickPlotTransferHandler` 注入、动作置灰 |

> **⚠️ 依赖方向**
>
> 快速绘图全部落在 DAGui，不反向依赖 APP。转入绘图区需要 `DAChartOperateWidget`（也在 DAGui，但只有 APP 知道它属于哪个停靠区），因此由 APP 通过
> `DADataOperateOfDataFrameWidget::setQuickPlotTransferHandler()` 注入一个处理器，页面只负责调用。

---

## 2. 一条数据流

```text
表格选区（按视图顺序 + 隐藏列跳过）
  → DATableChartRequest（数据集句柄 + 有序选中列 + 角色 + 行数/惰性标记）
  → DATableChartPlanner::plan()（可选图型 + 默认规格 + 拒绝原因）
  → DATableChartSpec（图型 / X / 数值列 / 分类列 / 聚合 / Top N）
  → DAChartSeriesSampler（采样）
  → DAChartQuickPlot::render()（构建图元 → attach 到目标 DAChartWidget）
  → 浮窗显示，或 transferToFigure() 转入正式图表
```

浮窗与正式图表**共用同一份**请求、采样与构建实现；转入不是把浮窗的 chart reparent 过去，而是用同一份规格再构建一次图元。

---

## 3. 图型能力矩阵（v1）

| 选区角色 | 默认图型 | 可选图型 |
|---|---|---|
| 1 列数值 | 折线 | 直方图、箱线图 |
| 1 列分类 | 柱状（计数） | — |
| 2 列数值 | 散点 | 折线、箱线图、柱状（低基数数值「作为分类」） |
| 时间 + 数值 | 折线（x=时间） | 散点 |
| 分类 + 数值 | 柱状（分组聚合，mean） | 箱线图（按类别） |
| 3+ 列数值 | 多序列折线（x=行号） | 箱线图 |
| 时间 + 2+ 数值 | 多序列折线 | — |
| 2 分类 / 分类 + 2+ 数值 / 单列时间 | 不出图（给出可读原因） | — |

- 分类聚合口径默认 `mean`，可切 `min/max/sum/median/count/std/var`；
- 分类柱状默认按聚合值降序；唯一值超过 50 时只显示 Top 30 并在浮窗底部标注；
- 箱线图固定 `whis=1.5`、显示离群点、无 CI、无图例。

---

## 4. 采样与精确性

| 场景 | 策略 |
|---|---|
| 小表（≤ 5000 行） | 全量绘制，不标注采样 |
| 大表折线/散点 | 确定性分块均匀采样，约 2000 点（首尾都覆盖，不引入随机数） |
| 大表直方图 | 约 2 万个值 |
| 分类聚合 / 箱线图 | 内存 pandas 源且 ≤ 500 万行 → **全量精确**；惰性源或超阈值 → 采样并在浮窗底部标注近似 |
| 取数预算 | `DAChartSeriesSampler::SampleBudget`：默认最多 128 次取数调用 / 25 万行，同一次构建的各列共用一个预算；耗尽即停止采样并置 `SampleInfo::limited`，`DATableChartBuildInfo::sampleLimited` 聚合后由浮窗标注「样本受限」 |
| 忙碌状态 | `DAChartQuickPlot::render()` 全程持有等待光标；浮窗在取数前禁用交互并显示「正在取数…」，取数结束（含失败）后恢复 |

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
- 「追加到当前图表」不新建 figure，只把新图元逐个登记为既有 `addItem_` 命令。

---

## 6. 扩展点

- **新增图型**：在 `DATableChartType` 加枚举、`DATableChartPlanner::plan()` 的矩阵里登记、在
  `DAChartQuickPlot` 里加一个 `build*()` 分支；图元一律复用既有组件或 `DAChartPlotRenderer`，
  不要出现第二处 `new QwtPlot*`。
- **新增聚合口径**：`DATableChartAggregation` + `DATableChartPlanner::aggregationKey()` +
  Python `ESTIMATOR_MAP`。
- **多序列分类柱状 / 2 分类交叉柱状**：统计管线已有 hue 能力，缺的是交互与基数守卫，留待后续。

---

## 7. 已知限制

- 浮窗不随工程保存、不进撤销栈、不导出图片；这些能力在转入后由绘图区提供。
- 浮窗不自动跟随选区与数据变化，只提供「更新为当前选区」与「刷新」。
- 惰性/数据库源目前只能采样聚合（无 SQL 聚合下推）。
- 「低基数数值作为分类」用采样（起始 2000 行）估计基数，极端分布下可能误判；真正的 Top N 守卫按聚合结果执行。
