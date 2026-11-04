# 表格页架构（DADataOperateOfDataFrameWidget 重构）

> **这份文档是什么**
>
> `src/DAGui/Table/DADataOperateOfDataFrameWidget.*`（表格页）在 008 重构后的目标架构与开发规约：
> 谁负责什么、后效在哪里收口、新增一个表内功能要动哪几个文件。
> 重构方案与迁移过程见 `tmp/plan-表格页重构与职责拆分.md`（体检 → 根因 → 分期迁移）。

## 1. 分层总览

```text
                     ┌──────────────────────────────────────────────┐
 外部（APP/测试/插件） │  DADataOperateOfDataFrameWidget  （Facade）    │  只做 5 件事：
 70 个方法、86 处构造  │  ①生命周期 ②装配 ③信号转发 ④对外委托 ⑤宿主桥   │  不写业务分支
                     └───────────────┬──────────────────────────────┘
                                     │ 持有并装配（唯一装配点）
      ┌──────────────────────────────┼───────────────────────────────────────┐
      ▼                              ▼                                       ▼
┌──────────────────┐        ┌──────────────────────┐              ┌──────────────────────┐
│ DATablePageContext│◄──────│ 特性控制器（10 个）     │              │ 数据操作（Strategy）  │
│ （依赖注入的值对象）│        │ Find/Style/Sort/CF/   │              │ Split/Merge/GroupBy/ │
│ model·panes·stores│        │ Computed/Validation/   │              │ Reshape/Fill/Insert/ │
│ undo·df·数据访问   │        │ Note/ColumnView/      │              │ Remove/Rename/Cast/  │
└──────────────────┘        │ ViewState/QuickPlot   │              │ Paste …（期4）        │
      ▲                    └──────────┬───────────┘              └──────────┬───────────┘
      │                               │ 一律经唯一后效入口                     │
      │                               ▼                                    ▼
      │                    ┌────────────────────────────┐      ┌──────────────────────┐
      └────────────────────│ DATableMutationCoordinator │◄─────│ DATableSelectionService│
                           │ 编辑后效唯一实现点           │      │ 选区/坐标纯查询（const）│
                           └────────────────────────────┘      └──────────────────────┘
```

| 类 | 目录 | 职责（一句话） |
| --- | --- | --- |
| `DATablePageContext` | `Table/Core/` | 依赖注入载体：模型、主窗格、窗格容器、元数据束与各子存储、数据与撤销栈桥、宿主桥 |
| `DATableSelectionService` | `Table/Core/` | 选区/坐标纯查询：选区→数据源绝对行列、整列/整行判定、包围盒、身份表 |
| `DATableMutationCoordinator` | `Table/Core/` | 编辑后效唯一实现点：模型刷新 → 统计失效 → 计算列标脏 → 浮窗/宿主；`attach()` 一次性订阅各子存储变更 |
| `DATableNoteController` | `Table/Controllers/` | 单元格批注：编辑弹层定位、增删改、清空 |
| `DATableQuickPlotController` | `Table/Controllers/` | 表内快速绘图：选区→请求、浮窗生命周期、转入绘图区处理器注入 |
| `DATableValidationController` | `Table/Controllers/` | 数据校验：规则对话框、求值（只读）、报告窗、违规高亮、改名迁移 |
| `DATableComputedColumnController` | `Table/Controllers/` | 计算列：公式求值管线、三态维护、全部重算（宏命令）、表头角标 |
| `DATableConditionalFormatController` | `Table/Controllers/` | 条件格式：规则对话框、WPS 式菜单、差异→命令（增/删/改/移父命令） |
| `DATableFindController` | `Table/Controllers/` | 查找/替换状态机：命中集/序号/请求串号、扫描、循环导航、替换（含降级） |
| `DATableStyleController` | `Table/Controllers/` | 样式与列显示格式：读写/清除、选区层级判定、代表样式一致性（三级策略） |
| `DATableSortController` | `Table/Controllers/` | 排序：多列规格、动作统一入口、完整规格应用（自定义排序对话框）、按索引排序/恢复原始行序、选区置换重映射、表头角标 |
| `DATableViewStateController` | `Table/Controllers/` | 视图状态投影：列宽/行头宽/行高/表头高采集与投影、冻结集合查表 |
| `DATableColumnViewController` | `Table/Controllers/` | 列视图真源操作：隐藏/冻结/列序/列视图对话框（Mediator-light） |

## 2. 三条硬规则（拆分成果的守卫）

1. **控制器不持有 `DADataOperateOfDataFrameWidget*`**，只持有 `DATablePageContext&`（构造注入）。需要通知外部时经上下文的宿主桥（`std::function`），由 Facade 注入实现。
2. **Facade 的 public 签名一个都不改**（86 处测试构造 + 70 个 APP 调用点零改动），内部改成一行转发或直接搬走。
3. **所有「编辑后效」只经 `DATableMutationCoordinator::commit()`**（模型刷新 / 统计失效 / 计算列标脏 / 浮窗关闭 / 宿主通知），任何控制器不得自己挨个摸 store。

> 拆完不加规矩，半年后又会重新长回一个上帝类。新代码评审时对照这三条。

## 3. 编辑后效策略表

`commit()` 内部是一张**显式策略表**（`DATableMutation`：`Values` / `Structure` / `FullReset`），不是 if-else 长龙：

| 变更类型 | 模型刷新 | 条件格式统计 | 计算列 | 浮窗 | 宿主树 |
| --- | --- | --- | --- | --- | --- |
| 值变更（`Values`） | 逐格（或 `modelRefresh` 整列形状） | 失效受影响列 | 按列标脏 | — | 可选 |
| 行增删（`Structure`） | `notifyRowsInserted` | 全失效 | 全标脏 | 关闭 | 可选 |
| 列增删（`Structure`） | `notifyColumnsRemoved` | 全失效 | 全标脏 | 关闭 | 重建列子项 |
| 整表替换（`FullReset`） | `refreshData` | 全失效 | 全标脏 | 关闭 | 可选 |

排序是特例：**不**失效统计（列值集合不变，003 性能决策）、**不**标脏计算列（R13）——选区置换重映射在 Sort 控制器内完成。

排序对外有三个入口，最终都汇聚到控制器：`sortColumn` / `addSecondarySortColumn`（列头右键、Ribbon 升/降序）、`applySpec`（自定义排序对话框 `DATableSortDialog` 产出的完整规格）、`restoreOriginalOrder`（「清除排序」= 按索引升序还原原始行序）。规格里的列号 `DATableSortController::c_indexSortColumn`（-1）是「索引」伪列：它不对应任何真实列，表头不画角标，算子侧由 `da_sort` 的索引键分支处理（C++ 传 -1 → Python 取 `df.index`）。

## 4. 新增一个表内功能的 3 步模板

```cpp
// 1. 新建控制器方法（或新控制器文件），构造注入 DATablePageContext
bool DATableXxxController::runXxx(...)
{
    const int col = mSel.oneSelectedDataframeColumn();       // 选区经 SelectionService
    if (col < 0) { daWarning << tr("Select one column first"); return false; }
    DATableDialogXxx dlg(ctx.df(), col, mCtx.hostWidget());  // 对话框只吃数据
    if (dlg.exec() != QDialog::Accepted) { return false; }
    auto cmd = std::make_unique< DACommandDataFrame_xxx >(ctx.df(), dlg.spec());
    DATableMutation m;                                        // 后效只有一处
    m.kind = DATableMutation::Kind::Structure;
    m.changedCols = { col };
    cmd->setCallBack(mMutation->callbackFor(m));
    if (!cmd->exec()) { return false; }
    mCtx.pushCommand(cmd.release());
    return true;
}
// 2. Facade 加一个 public 转发（签名不动：一行）
// 3. 用户的 i18n 随迁（tmp/insert_i18n_context.py 纯文本插入，禁止硬编码中文）
```

对比旧模式：新功能 = 新增 1 个控制器文件 + Facade 1 个转发 + i18n 随迁，不再往 7000 行文件里塞 150 行。

## 5. 上下文宿主桥清单（`DATablePageContext`）

控制器经这些 `std::function` 触达页面级动作（不持窗口指针）：

| 桥 | 语义 | 注入方 |
| --- | --- | --- |
| `pushCommand` | 撤销栈推入（push 即激活页撤销栈） | Facade |
| `notifyHostValueChange` | `DADataManager` 树刷新（ChangeValue） | Facade |
| `notifyHostColumnRename` | 树刷新（ChangeDataframeColumnName） | Facade |
| `closeFloatingWindows` | 关闭快速绘图等页级浮窗 | Facade |
| `locateActualCell` | 定位单入口（隐藏列自动取消隐藏 + 冻结区处置） | Facade |
| `notifyDataModified` | 数据脏标（页面 `dataModified` 信号同源） | Facade |
| `isHeaderPickMode` | 表头拾取模式查询（排序守卫） | Facade |

## 6. 冻结/隐藏/列序的真源

视图状态子存储（`DATableViewStateStore`，借自元数据束）是列视图的**唯一真源**：
`DATableViewStateController::applyFromStore()` 是唯一投影路径（列宽 → 行头宽 → 行高/表头高
→ 列序/隐藏/冻结进窗格容器 → 末轮内容自适应）。`isColumnInFrozenArea` 纯查表，写入时机只在投影内。
任何「让某列可见/冻结」的需求都写 store 再重投影，**禁止**直接操作窗格。

## 7. 已知边界与不变量

- **撤销语义**：命令的 `exec() → 回调 → push()` 顺序与 directional 回调（redo/undo 分向）是测试锁定的不变量，`DATableMutationCoordinator` 只接管后效、**不接管**命令时序。
- **i18n 上下文**：控制器的用户可见文本在自己的 `tr()` 上下文里；新增时用纯文本插入工具随迁翻译，**禁止**硬编码中文。
- **性能红线**：排序不改列值集合（统计不失效）；滚动千行无卡顿（投影守卫屏蔽采集回环）；内容自适应只采样视口可见行。

## 相关文件

| 文件 | 说明 |
|------|------|
| `src/DAGui/Table/DADataOperateOfDataFrameWidget.h/.cpp` | 表格页 Facade（生命周期/装配/信号转发/对外委托/宿主桥） |
| `src/DAGui/Table/Core/` | 页面上下文 + 选区服务 + 编辑后效协调器 |
| `src/DAGui/Table/Controllers/` `Table/Controllers/` `Table/Controllers/` `Table/Controllers/` | 批注 / 快速绘图 / 校验 / 计算列控制器 |
| `src/DAGui/Table/Controllers/` `Table/Controllers/` `Table/Controllers/` `Table/Controllers/` | 条件格式 / 查找替换 / 样式 / 排序控制器 |
| `src/DAGui/Table/Controllers/` `Table/Controllers/` | 视图状态投影 / 列视图真源操作控制器 |
| `tmp/plan-表格页重构与职责拆分.md` | 重构方案全文（体检/根因/分期迁移/度量） |

> **📖 相邻主题**：[表格元数据架构](table-metadata.md)（子存储与序列化）、[表内快速绘图](table-quick-plot.md)（请求/规格/规划器）、[滑动窗表格导航协议](virtual-table-navigation.md)（行号口径）。

