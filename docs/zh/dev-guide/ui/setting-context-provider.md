# 设置上下文提供者（插件接管设置面板与 Ribbon 上下文）

主程序里，右侧设置面板、图表管理树、Ribbon 上下文标签默认跟着三个固定操作窗口（数据、工作流、绘图）切换。插件自己的停靠页面也想复用这套设置界面时，实现 `DASettingContextProvider` 并注册到宿主即可：该页面被激活时由插件决定设置面板的编辑目标、图表管理树的内容、Ribbon 上下文标签与附属工具栏；切走时再还原给宿主。插件页面里如果也有 `DAFigureWidget`（例如信号分析窗口），provider 还可以声明绘图上下文目标，让内置「绘图操作」上下文标签一起激活，并把绘图域动作路由到插件 figure。

本文说明这套机制的接口、注册表、APP 路由、生命周期约定，以及写一个新 provider 的步骤。参考实现是信号分析插件的 `DASignalAnalysisSettingContext`。

## 机制总览

```mermaid
sequenceDiagram
    participant User as 用户
    participant ADS as ADS 停靠管理器
    participant APP as DAAppController
    participant Dock as DADockingAreaInterface
    participant Registry as DASettingContextProviderRegistry
    participant Provider as 插件 Provider

    User->>ADS: 激活插件的停靠页面
    ADS->>APP: focusedDockWidgetChanged / dockWidgetRaised
    APP->>Dock: notifyDockWidgetActivated(dockContent)
    Dock-->>APP: （广播 dockWidgetActivated，供插件监听）
    APP->>APP: activateContextCategoryForWidget(dockContent)
    APP->>APP: resolveAppContextRoutePolicy(...)
    APP->>Dock: activateSettingContextForWidget(dockContent)
    Dock->>Registry: activateFor(dockContent)
    Registry->>Provider: activateSettingContext()
    Provider->>Dock: raiseDockByWidget(设置容器) / setPlot / setFigures
    Provider-->>Registry: （返回后）contextUndoStack()->setActive()
    User->>ADS: 切回绘图/数据/工作流窗口
    APP->>Dock: deactivateSettingContextProvider()
    Registry->>Provider: deactivateSettingContext()
```

关键点：

- 路由判定由 APP 完成，provider 只负责「激活时把哪些界面拨到自己这边」，不参与路由决策。
- provider 命中时由 APP 决定固定/内置上下文标签：未声明 `chartContextFigure()` 时隐藏三个固定上下文分类，由 provider 显示自己的上下文标签；声明时显示内置「绘图操作」并把绘图域动作路由到该 figure（见下文「内置绘图操作上下文」）。
- 激活与失活都可能被重复调用，实现方必须幂等。

## 接口

`src/DAInterface/DASettingContextProvider.h`：

```cpp
namespace DA {
class DASettingContextProvider
{
public:
    virtual ~DASettingContextProvider() = default;
    // 该 dock 内容成为激活页时调用：显示/切换设置面板与 ribbon 上下文
    virtual void activateSettingContext() = 0;
    // 失活时调用：隐藏本上下文的 ribbon 页、还原只在激活期出现的 UI
    virtual void deactivateSettingContext() = 0;
    // 本上下文的撤销栈（可为空，由宿主负责 setActive）
    virtual QUndoStack* contextUndoStack() const { return nullptr; }
    // 本上下文的绘图操作目标：非空时宿主显示内置「绘图操作」上下文并把绘图域动作路由到该 figure；
    // 返回 nullptr 表示不接管绘图上下文（默认）
    virtual DAFigureWidget* chartContextFigure() const { return nullptr; }
};
}
```

| 成员 | 调用方 | 职责 |
|------|--------|------|
| `activateSettingContext()` | 注册表，在 dock 内容被激活且命中时 | 接管设置面板目标、图表管理树、Ribbon 上下文标签与附属工具栏 |
| `deactivateSettingContext()` | 注册表，在切换到其它上下文时 | 隐藏本上下文的 UI，把设置面板与图表管理树还原给宿主 |
| `contextUndoStack()` | 注册表，激活成功后 | 返回当前文档的撤销栈；返回 nullptr 表示本上下文没有撤销需求，宿主保持原栈 |
| `chartContextFigure()` | APP 路由（激活/失活、`settingContextChanged` 时） | 返回插件页当前可操作的 `DAFigureWidget`；非空时宿主激活内置「绘图操作」上下文并把绘图域动作路由到它，返回 nullptr 时不接管绘图上下文 |

接口是纯抽象类（没有 out-of-line 成员、没有静态数据），不加导出宏，派生类和跨库虚调用不需要额外处理，注册表逻辑也可以被单元测试直接编译覆盖。

生命周期约定：实现方必须保证 provider 在注册期内有效，并在自身销毁前调用 `DADockingAreaInterface::unregisterSettingContextProvider()`。注册表不接管所有权。

## 注册表 DASettingContextProviderRegistry

`src/DAInterface/DASettingContextProviderRegistry.h/.cpp`，由 `DADockingAreaInterface` 持有，与具体 dock 管理器解耦。

| 方法 | 说明 |
|------|------|
| `registerProvider(dockContent, provider)` | 注册；同一内容重复注册以最后一次为准；换 provider 时若旧的是当前项会先失活 |
| `unregisterProvider(dockContent)` | 注销；若该内容正是当前项会先失活 |
| `isRegistered(dockContent)` / `providerFor(dockContent)` | 查询；dock 内容已销毁时惰性清理条目并返回 nullptr |
| `activateFor(dockContent)` | 命中返回 true 并接管；未命中返回 false 且**不改变当前状态** |
| `deactivateCurrent()` | 失活当前提供者；没有当前项时无操作 |
| `currentProvider()` / `currentDockContent()` | 当前提供者与对应 dock 内容 |
| `providerCount()` | 有效条目数（顺带清理内容已销毁的条目） |

行为约定：

- **当前项由「内容 + 提供者」共同判定**。同一个 provider 可以注册给多个 dock 内容，只比较 provider 指针会把非当前条目的清理误判成当前项。
- **重入保护**。`activateFor()` 内部用 `mActivating` 标记：provider 在 `activateSettingContext()` 里程序化 raise 自己或其它已注册 dock 时会再次经过 APP 路由，重入调用直接返回「已注册」，不会重复激活。
- **重复激活仍会回调**。同一内容再次调用 `activateFor()` 时，注册表不会跳过 `activateSettingContext()`（只跳过失活），因此实现方要保证激活逻辑幂等。
- **失活时机由调用方决定**。`activateFor()` 未命中时不动当前状态；APP 只在焦点回到固定操作窗口或离开全部 dock 时调用 `deactivateCurrent()`。这样工具 dock（设置、日志、图表管理）激活时不会误清插件上下文。
- **内容销毁时不回调**。注册表用 `QPointer` 跟踪 dock 内容；内容先于 provider 销毁时只清指针与条目，不调用 `deactivateSettingContext()`（此时 provider 可能已经失效）。因此 provider 自身的清理要自己保证。
- **撤销栈**。激活成功后若 `contextUndoStack()` 非空且不是 active，注册表调用 `setActive()`。撤销/重做按钮因此跟随当前插件文档。

## DADockingAreaInterface 与 dockWidgetActivated

`src/DAInterface/DADockingAreaInterface.h` 暴露给插件的入口：

| 方法/信号 | 说明 |
|-----------|------|
| `registerSettingContextProvider(dockContent, provider)` | 注册 dock 内容的设置上下文 |
| `unregisterSettingContextProvider(dockContent)` | 注销（注册方在内容销毁前调用） |
| `activateSettingContextForWidget(dockContent)` | 激活对应 provider；未命中返回 false |
| `deactivateSettingContextProvider()` | 失活当前 provider |
| `settingContextProviderFor(dockContent)` | 查询 provider |
| `currentSettingContextProvider()` / `currentSettingContextDockContent()` | 当前激活的 provider 与其 dock 内容（无则 nullptr） |
| `notifySettingContextChanged()` / `settingContextChanged()` | 插件内部目标变化（切文档/切子图/注销）后通知 APP 重新同步绘图上下文标签与动作状态 |
| `dockWidgetActivated(QWidget* dockContent)` | 任意 dock 内容被激活时广播，参数是 dock 内部维护的内容窗口 |
| `raiseDockByWidget(QWidget*)` | 唤起某个控件对应的 dock（provider 激活时唤起右侧设置面板用） |

`dockWidgetActivated` 与 `dockWidgetRaised` 的区别：

- `dockWidgetRaised` 只在程序化 raise 时发出。
- `dockWidgetActivated` 覆盖焦点变化与程序化 raise 两条路径，在上下文路由**之前**发出；焦点离开全部 dock 时参数为 nullptr。插件可以监听它同步自身状态，但不要在这里做路由决策，也不要自己调用 `notifyDockWidgetActivated()`（那是 APP 的内部入口）。

## APP 上下文路由

路由判定抽成了纯函数，便于单元测试：`src/APP/Controller/DAAppContextRoutePolicy.h/.cpp`。

```cpp
enum class DAAppContextRoutePolicy
{
    SettingContextProvider,  // 插件注册的设置上下文提供者接管（内置上下文标签由 APP 按 provider 声明决定）
    DataOperate,             // 数据操作窗口
    WorkFlowOperate,         // 工作流操作窗口
    ChartOperate,            // 绘图操作窗口
    DataManage,              // 数据管理窗口
    UtilityDock              // 其它工具 dock（设置/日志/图元树等）：不切换上下文
};

DAAppContextRoutePolicy resolveAppContextRoutePolicy(QWidget* widget,
                                                     bool hasSettingContextProvider,
                                                     QWidget* dataOperateWidget,
                                                     QWidget* workFlowOperateWidget,
                                                     QWidget* chartOperateWidget,
                                                     QWidget* dataManageWidget);
```

判定顺序：provider 优先，其次依次比较数据、工作流、绘图操作窗口与数据管理窗口，其余归 `UtilityDock`。

`DAAppController::activateContextCategoryForWidget()`（`src/APP/Controller/DAAppControllerCore.cpp`）的分支：

```cpp
if (nullptr == widget) {
    // 焦点离开全部 dock 内容：先失活插件上下文（含内置绘图上下文），再清空固定上下文标签
    const bool hadProvider = mDock->currentSettingContextProvider() != nullptr;
    mDock->deactivateSettingContextProvider();
    if (hadProvider) { refreshChartContextUi(); }
    mRibbon->hideContextCategory(AllContextCategory);
    return;
}
const DAAppContextRoutePolicy route = resolveAppContextRoutePolicy(...);
if (SettingContextProvider == route) {
    // provider 声明了绘图上下文 figure：显示内置「绘图操作」上下文并把绘图动作路由到该 figure；
    // 未声明：保持原有语义，三个固定上下文分类全部收起
    DASettingContextProvider* provider = mDock->settingContextProviderFor(widget);
    if (provider && provider->chartContextFigure()) {
        hide(Data); hide(Workflow); show(Chart);
    } else {
        mRibbon->hideContextCategory(AllContextCategory);
    }
    mDock->activateSettingContextForWidget(widget);
    refreshChartContextUi();  // 同步动作勾选状态与可用性
    return;
}
// 只有回到三个固定操作窗口时才失活插件上下文；工具 dock 保持插件上下文
if (route == DataOperate || route == WorkFlowOperate || route == ChartOperate) {
    const bool hadProvider = mDock->currentSettingContextProvider() != nullptr;
    mDock->deactivateSettingContextProvider();
    if (hadProvider) { refreshChartContextUi(); }
}
switch (route) {
case DataOperate:  /* 显示 Data 上下文标签 + 数据窗口撤销栈 */ break;
case WorkFlowOperate: /* 显示 Workflow 上下文标签 + 工作流设置页 */ break;
case ChartOperate: /* 显示 Chart 上下文标签 + 绘图设置页 */ break;
case DataManage: /* 只切换数据管理撤销栈 */ break;
default: break;    // UtilityDock：不改上下文
}
```

### 路由分支的实际行为

当前各分支对上下文标签的实际行为（与基线代码一致，写明以免误判）：

- **`widget == nullptr`**（焦点离开全部 dock 内容）：失活 provider + `hideContextCategory(All)`，固定上下文标签全部清空；若 provider 曾声明绘图上下文，还会恢复宿主的绘图动作状态。
- **未识别/未路由的 widget**（`UtilityDock`）：什么都不做，当前上下文标签保持原样。
- **provider 命中且声明了 `chartContextFigure()`**：APP 显示内置「绘图操作」上下文（隐藏 Data/Workflow），随后激活 provider（provider 再显示自己的上下文标签）；绘图域动作经 `DAAppController::getChartContextFigure()/getChartContextChart()/applyToCharts()` 路由到 provider figure，不再作用于 APP 绘图区。
- **provider 命中但未声明绘图上下文**：先隐藏三个固定上下文分类（Chart/Data/Workflow），再激活 provider；provider 自己的上下文标签由 provider 显示。
- **固定操作窗口命中**（数据/工作流/绘图）：失活 provider（收起内置 Chart 上下文）并显示对应固定分类，`showContextCategory` 本身会把其余固定分类隐藏。

> **已知待验证项**：若原始症状（插件页激活后上下文标签被清空）来自嵌套 dock 抢焦点导致的
> `focusedDockWidgetChanged(nullptr)`，该路径仍会失活 provider 并 hide-all——本机制没有覆盖它。
> 需要在真实 GUI 里观察信号页激活/raise 时 `onFocusedDockWidgetChanged` 的 old/now 实参
> （`now == nullptr` 即命中该路径），再决定是否按「最近一次 provider」保持上下文。

### hide-all 陷阱

历史行为澄清（更正早先把缺陷归因为「未识别 dock 激活会清空上下文标签」的叙述）：

- 基线代码里 `activateContextCategoryForWidget()` **只有 `widget == nullptr`** 才 `hideContextCategory(All)`；
  未识别的 widget 在改动前后行为一致（什么都不做），并不会触发 hide-all。
- 真正会 hide-all 的路径是焦点离开全部 dock 内容（`focusedDockWidgetChanged(nullptr)`），
  该路径同时会失活 provider；若原始症状来自嵌套 dock 抢焦点，需要按上一节的「已知待验证项」
  在 GUI 里实测确认，而不是按「未识别 widget」排查。

现在的做法有三层：

- 已注册 provider 的 dock 内容在路由里优先命中，走 provider 分支：provider 声明了
  `chartContextFigure()` 时 APP 显示内置「绘图操作」上下文并把绘图动作路由到插件 figure
  （动作不会再落到 APP 绘图区）；未声明时隐藏三个固定上下文分类，由 provider 显示自己的
  上下文标签。
- provider 失活/注销时，APP 收起由 provider 显示的内置 Chart 上下文并恢复宿主的绘图动作
  可用性（宿主路由随后再按固定窗口显示正确分类）。
- 未识别的 dock 归 `UtilityDock`，只 `break`，不切换也不清空上下文。工具 dock（设置、日志、
  图表管理）因此不会把插件上下文顶掉。

改造路由时不要恢复「未识别就 hide-all」的行为，也不要把 `UtilityDock` 合并进 `nullptr` 分支。

## 激活 / 失活的生命周期与幂等

### activateSettingContext()

典型顺序（以信号分析为例）：

1. 页面未创建、dock 不可用时直接返回并记一条开发日志（不要假设一定处于可用状态）。
2. `raiseDockByWidget(设置容器)` 唤起右侧设置 dock。
3. 按当前子图类型切设置页并设置 target：2D 调 `showChartSettingWidget()` + `getChartSettingWidget()->setPlot(chart)`；3D 调 `showChart3DSettingWidget()` + `getChart3DSettingWidget()->setPlot3D(plot3D)`。
4. 图表管理树接管：`getChartManageWidget()->setFigures(本插件的 figure 列表, 当前 figure)`。
5. 显示插件专属 UI：Ribbon 上下文标签（`showContextCategory(objectName)`，一般同时把该上下文页设为当前页）与窗口底部工具栏。**内置「绘图操作」标签不要由 provider 自己 show/hide**：实现 `chartContextFigure()` 后由 APP 统一显示/收起并路由动作。
6. 绑定页面/视图的变化信号（先 disconnect 再 connect），保证重复激活只有一条连接。
7. 置 `mActive = true`。

幂等要求：

- 宿主可能因嵌套焦点变化对同一个 provider 连续调用激活，因此重复执行只能刷新 target，不能重复创建控件、重复连接信号或反复 raise 造成闪烁。
- 激活过程中会触发程序化 raise，再次进入 APP 路由与注册表；注册表的重入保护只保证不重复激活，provider 自己的逻辑仍要能承受重入（不要在回调里做递归调用链）。

### deactivateSettingContext()

1. 未激活（`mActive == false`）直接返回，不触碰界面。
2. 隐藏插件上下文标签与附属工具栏。
3. 图表管理树还原：`getChartManageWidget()->setChartOperateWidget(宿主绘图操作窗口)`（内部模式会清空并按宿主 figure 列表重建）。
4. 设置面板还原：`showChartSettingWidget()`，target 设为宿主当前 figure 的当前 2D 子图；取不到时 `setPlot(nullptr)`，避免继续编辑插件文档。
5. 若插件 figure 上仍有内置「绘图操作」触发的图表编辑器（选区/标记工具），调用 `endChartEditor()` 收尾。
6. 断开页面/视图信号，置 `mActive = false`。
7. 调用 `notifySettingContextChanged()`（此时注册表已清空当前 provider，APP 会收起内置 Chart 上下文并恢复宿主绘图动作状态）——插件注销/卸载路径不经过 APP 路由，这一步是必需的。

不要在 `deactivateSettingContext()` 里调用会再次触发路由的重操作（例如 raise 别的 dock）；失活只做还原。

### 撤销栈

- `contextUndoStack()` 返回当前文档 figure 的 `QUndoStack`；页面/视图/figure 不存在时返回 nullptr，宿主会保持原栈。
- 宿主在 provider 激活成功后 `setActive()`。切换插件内部文档（信号标签页）时注册表不会再次回调，`DAFigureWidget::push()` 在自己栈非 active 时会 `setActive()`，所以下一次编辑会自动切到正确栈。
- 撤销/重做命令本身由 `DAFigureWidget` 管理，provider 不参与命令入栈。

## 内置绘图操作上下文（chartContextFigure）

插件页面里如果有 `DAFigureWidget`（例如信号分析窗口），可以在 provider 里实现
`chartContextFigure()` 返回当前可操作的 figure。APP 在 provider 激活期间会：

1. 显示内置「绘图操作」上下文标签（两页：绘图样式、图表编辑），同时隐藏 Data/Workflow；
2. 把绘图域动作的解析目标从宿主绘图窗口切换到 provider figure：
   `DAAppController::getChartContextFigure()/getChartContextChart()/getChartContextCharts()`
   （`applyToCharts()`/`needOperateCharts()` 也走这条路径）；
3. 回灌插件 figure 的状态到 Ribbon（网格/缩放/拾取/图例勾选、导出可用性等）；
4. provider 失活/注销后收起内置 Chart 标签并恢复宿主的动作可用性与状态。

`chartContextFigure()` 返回 nullptr（默认）表示不接管，行为与旧版完全一致。

### 动作可用性（插件绘图上下文）

| 动作 | 行为 |
|------|------|
| 网格总开关/XY/Xmin/Ymin、缩放开关/放大/缩小/显示全部、平移、禁止 X/Y 缩放 | 目标 = provider figure 的当前 2D 子图 |
| 十字/XY/Y 拾取、联动拾取、拾取文字位置、图例显隐/位置 | 同上 |
| 绘图主题、复制绘图到剪贴板、导出当前子图 | figure 级动作，2D/3D 子图都可用（3D 导出为 PNG） |
| 选区工具、辅助工具（十字/水平/垂直标记、箭头、文本） | 目标 = provider figure，图元进插件 figure 的 undo 栈 |
| 「应用到所有绘图」 | 只作用于 provider figure 的 2D 子图（`DAFigureWidget::getCharts()`），不是全部插件 figure |
| 新建绘图、新建坐标系、调整子图大小、数据探针 | **禁用**：新建绘图会落到宿主绘图区；其余与插件 figure 的布局/生命周期或数据联动不兼容 |

3D 子图激活时（`DAFigureWidget::currentSubChartKind() == Chart3D`）：内置 Chart 标签保持显示，
主题/复制/导出保持可用，其余 2D 类动作置灰；切回 2D 子图后自动恢复。

### provider 侧约定

- 只返回「当前文档」的 figure；没有可操作文档时返回 nullptr，宿主据此隐藏内置 Chart 标签并置灰相关动作。
- 切换内部文档/子图（figure 或当前 2D 子图变化）后调用
  `DADockingAreaInterface::notifySettingContextChanged()`，让 APP 重新同步标签状态与动作可用性。
- 失活时结束插件 figure 上的图表编辑器（内置选区/标记工具留下的编辑态），再 notify。
- 不要自己 show/hide 内置 Chart 上下文标签，也不要复制一套网格/缩放/拾取控件到插件上下文页；
  交互工具统一由内置标签提供。若插件原有独立交互开关（例如信号分析的旧「缩放/平移」开关），
  应删除并改为监听 `DAChartWidget::chartPropertiesChanged` 同步自定义耦合逻辑（如信号分析的
  「点击定位」互斥）。

## 图表管理树的目标抽象

`DAChartManageWidget`（`src/DAGui/Chart/DAChartManageWidget.h`）原来只认宿主 `DAChartOperateWidget`，现在增加了外部模式：

| 方法 | 说明 |
|------|------|
| `setChartOperateWidget(cow)` | 内部模式：监听绘图操作窗口的 figure 增删与当前 figure 变化，清空后按当前 figure 列表重建 |
| `setFigures(figures, current)` | 外部模式：按给定列表重建管理树，不依赖绘图操作窗口；`current` 为空或不在列表里时取列表首项 |
| `setCurrentFigure(figure)` | 只切换管理树的当前 figure，不改变宿主绘图区的当前 figure |
| `figures()` | 当前管理树里的 figure 列表，按 combobox 顺序 |

外部模式的要点：

- `setFigures()` 会断开与 `DAChartOperateWidget` 的信号连接但保留指针，`setChartOperateWidget()` 才能切回内部模式。
- 管理树不拥有 figure；figure 销毁时管理项由 `DAChartManageWidget` 的清理逻辑移除，但 provider 应保证只传入活着的 figure。
- **外部模式下右键菜单不提供「删除图元（Delete）」与「删除坐标系（Remove Chart）」**（「转移」同样已隐藏）：
  外部 figure（如信号分析视图）的图元/子图被视图缓存为裸指针，宿主侧的删除是 detach+delete，
  会让视图悬空崩溃；恢复内部模式（`setChartOperateWidget`）后这些动作重新可见。
- 失活时必须用 `setChartOperateWidget(宿主)` 还原，否则管理树会继续显示插件 figure。
- 每次切子图都调 `setFigures()` 会重建树，可能复位展开状态。功能正确；若体验需要优化，可只在 figure 列表或当前 figure 真正变化时重建。

## 参考实现：DASignalAnalysisSettingContext

`plugins/DASignalAnalysis/Page/DASignalAnalysisSettingContext.h/.cpp` 完整实现了本文描述的流程：

- `QObject` + `DASettingContextProvider` 多重继承，用 `QPointer` 持有插件 UI 与当前视图。
- 激活时显示设置 dock、按 2D/3D 切设置页、`setFigures()` 接管管理树、显示 `ContextCategorySignalAnalysis` 与底部播放工具栏、绑定 `currentSignalChanged` 与 `currentSubChartChanged`。
- 实现 `chartContextFigure()` 返回当前信号 figure，内置「绘图操作」上下文因此随信号页激活，绘图动作作用于信号图（新建绘图/坐标系、数据探针等结构性动作由宿主置灰）。
- 切信号/切子图时 `refreshTarget()` 刷新设置页/管理树并 `notifySettingContextChanged()`，宿主据此同步 Ribbon 与动作可用性。
- 失活时隐藏插件 UI、结束信号 figure 的图表编辑器、`setChartOperateWidget` 还原管理树、把设置面板还给宿主当前子图，并 notify 宿主恢复绘图上下文。
- 交互工具不再由插件自建：旧「缩放/平移/十字线/数据拾取」开关已移除，SF-12「点击定位」互斥改为监听当前 `DAChartWidget::chartPropertiesChanged` 同步。
- `contextUndoStack()` 返回当前信号 figure 的撤销栈。
- 注册/注销在 `DASignalAnalysisUI` 的页面创建与 `finalize()` 中完成（见下节）。

## 写一个新 provider 的步骤

**1. 定义类**

```cpp
// MySettingContext.h
#include <QObject>
#include <QPointer>
#include "DASettingContextProvider.h"

namespace DA {
class MyPluginUI;
class MySettingContext : public QObject, public DASettingContextProvider
{
    Q_OBJECT
public:
    explicit MySettingContext(MyPluginUI* ui, QObject* parent = nullptr);
    void activateSettingContext() override;
    void deactivateSettingContext() override;
    QUndoStack* contextUndoStack() const override;

private:
    QPointer< MyPluginUI > mUI;
    bool mActive { false };
};
}
```

**2. 选对注册的 dock 内容**

注册的参数是 dock 管理器里保存的**内容窗口**（插件页面 widget 本身），不是外层 `CDockWidget` 或容器。信号分析插件注册的是 `DASignalAnalysisPage*`，即 `dockContent`。

**3. 在页面创建后注册**

```cpp
// DASignalAnalysisUI.cpp（页面创建路径）
mSettingContext = new DASignalAnalysisSettingContext(this);
dockingArea->registerSettingContextProvider(mPage, mSettingContext);
```

**4. 在销毁前注销**

```cpp
// finalize()：必须在页面 dock 移除之前，且先注销再 delete
dockingArea->unregisterSettingContextProvider(mPage);
delete mSettingContext;
mSettingContext = nullptr;
```

顺序不能反：注销会触发 `deactivateSettingContext()`，此时页面与 dock 还必须可用。

**5. 实现 `activateSettingContext()`**

按「唤起设置 dock → 切设置页/设 target → 接管管理树 → 显示上下文标签与工具栏 → 绑定信号 → 置激活标志」的顺序写，并保证重复调用只刷新、不重复创建。用 `QPointer` 保存页面/视图指针，页面惰性创建时要容忍「未创建」。

**6. 实现 `deactivateSettingContext()`**

只做还原：隐藏插件 UI、结束插件 figure 的图表编辑器、管理树交给宿主、设置面板交给宿主当前子图、断开信号、清激活标志，最后 `notifySettingContextChanged()`。未激活直接返回。

**7. 实现 `contextUndoStack()`**

返回当前文档 figure 的撤销栈；没有文档时返回 nullptr，不要返回宿主别的栈。

**8. 转接内部变化刷新 target**

页面切文档、切换 2D/3D 子图时调用自己的 `refreshTarget()` 重新 setPlot/setPlot3D 并刷新管理树，然后 `notifySettingContextChanged()`。信号连接先断后连，避免重复连接。

**9. 自测**

- `src/tst/APPRefactorTest/`：注册表当前项判定、重入、注销失活、路由纯函数。
- `src/tst/DAChartContextTest/`：`setFigures()` / `setCurrentFigure()` / 子图导出。
- GUI 冒烟：打开插件页面 → 设置标签页出现且 target 正确；切子图 → 设置页跟着切；切到绘图区 → 上下文标签互斥、管理树与设置面板恢复；关闭插件页面/停用插件 → 无悬空回调。

## 反模式

- 销毁 provider 前不注销：注册表后续回调会解引用已销毁对象。
- 失活时不清 target：设置面板或管理树会继续编辑插件文档。
- 在 `activateSettingContext()` 里重复创建工具栏/重复 connect：重复激活后出现多份控件与重复槽调用。
- 为了显示自己的上下文而调用 `hideContextCategory(All)`：会把固定操作窗口的上下文标签一起清掉。
- 在 `dockWidgetActivated` 信号里做上下文路由：路由是 APP 的职责，重复路由会造成互相覆盖。
- 把未识别的工具 dock 当固定操作窗口处理：设置、日志、图表管理激活时不应让插件上下文失活，也不应 hide-all。
- 用裸指针缓存宿主窗口：宿主可能先销毁，统一用 `QPointer`。
- 声明了 `chartContextFigure()` 还自己 show/hide 内置 Chart 上下文、或把网格/缩放/拾取按钮再复制一份到插件页：
  会让同一状态出现两套控件、两套同步逻辑。工具统一交给内置标签。
- 内部文档/子图切换后不 `notifySettingContextChanged()`：Ribbon 勾选与动作可用性会停留在上一个 figure。

## 涉及文件

| 文件 | 说明 |
|------|------|
| `src/DAInterface/DASettingContextProvider.h` | 提供者接口 |
| `src/DAInterface/DASettingContextProviderRegistry.h/.cpp` | 注册表：当前项、重入保护、激活/失活、撤销栈 |
| `src/DAInterface/DADockingAreaInterface.h/.cpp` | 注册/注销/激活入口，`dockWidgetActivated` 广播 |
| `src/APP/Controller/DAAppContextRoutePolicy.h/.cpp` | 路由判定纯函数 |
| `src/APP/Controller/DAAppControllerCore.cpp` | `activateContextCategoryForWidget()`：provider/固定窗口分支、内置绘图上下文显隐、`settingContextChanged` 处理 |
| `src/APP/Controller/DAAppChartActionHandler.cpp` | 绘图域动作目标切换、`syncChartContextActions()`（勾选/可用性/3D 置灰/结构性动作禁用） |
| `src/APP/Controller/DAAppControllerConnections.cpp` | `settingContextChanged` 连接 |
| `src/DAGui/Chart/DAChartManageWidget.h/.cpp` | 管理树内部/外部模式与目标还原 |
| `src/DAGui/Common/Settings/DASettingContainerWidget.h` | 设置页 getter 与 `show*()` 切换方法 |
| `plugins/DASignalAnalysis/Page/DASignalAnalysisSettingContext.h/.cpp` | 参考实现 |
| `plugins/DASignalAnalysis/DASignalAnalysisUI.cpp` | 注册/注销时机、上下文标签与底部工具栏显隐 |

## 参考资料

- [绘图窗口网格布局](../graphics/figure-grid-layout.md)：当前子图切换与网格布局的配合。
- [绘图模块概述](../graphics/figure-abstract.md)：`DAFigureWidget` / `DAChartWidget` 基础能力。
- [创建设置面板](./creating-setting-panel.md)：属性面板的创建与刷新模式。
- [绘图窗口停靠布局](../graphics/chart-dock-nesting.md)：figure 的停靠与序列化。
