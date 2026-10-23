# 主界面

主窗口自上而下分成四块：顶部的 Ribbon 功能区、中间的三个操作区、两侧可以收起的停靠面板、底部的状态栏。这一章讲清每一块的作用、面板怎么摆、布局怎么存下来，以及下一次启动时界面会恢复到什么样子。界面上每一个面板里的具体操作，分散在后面的章节里讲。

## 窗口标题

标题栏显示程序名和当前工程名。没有打开工程时标题是 `DA`，打开工程 `myproject.dapro` 之后变成 `DA-myproject`。标题里出现星号表示工程有改动尚未保存，保存之后星号消失。关闭窗口时如果还有未保存的改动，会弹出询问 `Do you need to save the project?` / 是否保存工程？，三个按钮分别是不保存直接关闭、保存后关闭、取消关闭；取消之后程序继续运行。

## 功能区分区

Ribbon 左上角是应用按钮 `File` / 文件，点开是应用菜单；右上角是快速访问栏，放着撤销和重做。Ribbon 其余部分是一排标签页，每个标签页下面分若干面板，面板里成组排着动作按钮。

中间的操作区有三个标签页：工作流操作（画布）、绘图操作（绘图与坐标系）、数据操作（表格视图）。三个操作区共用一个停靠区，都在其中以标签页形式打开。

左侧停靠区放管理和工具箱面板，右侧停靠区放设置类面板，面板可以拖动、浮动和收起，细节见下文。

## Ribbon 标签页

### 常驻标签页

五个标签页一直存在，见下表。

| 标签页 | 面板 | 面板里的动作 |
|--------|------|--------------|
| `Main` / 主页 | `File` / 文件 | `Open` / 打开、`Save` / 保存、`Save As` / 另存为 |
| | `Clipboard` / 剪贴板 | `Undo` / 撤销、`Redo` / 重做、`Cut` / 剪切、`Copy` / 复制、`Paste` / 粘贴、`Delete` / 删除、`Select All` / 全选 |
| | `Create` / 创建 | `Add Data` / 添加数据、`Add Figure` / 添加绘图、`New Workflow` / 新建工作流 |
| | `Config` / 配置 | `Setting` / 设置、`Plugin Config` / 插件设置、`About` / 关于 |
| `Data` / 数据 | `Data Operation` / 数据操作 | `Add Data` / 添加数据、`Remove Data` / 移除数据、`Rename Data` / 重命名数据 |
| | `Export` / 导出 | `Export Data` / 导出数据，菜单 `Export Format` / 导出格式 下有 `Export CSV`、`Export Excel`、`Export Pickle`、`Export Parquet` |
| `View` / 视图 | `Display` / 视图显示 | `Show Workflow Area` / 工作流操作视图、`Show Workflow Manager` / 工作流管理视图、`Show Chart Area` / 绘图操作视图、`Show Chart Manager` / 绘图管理视图、`Show Table Area` / 数据操作区域、`Show Data Manager` / 数据管理视图、`Show Information Window` / 信息窗口、`Show Setting Window` / 设置窗口、`Show Left Side Bar` / 左侧边栏、`Show Right Side Bar` / 右侧边栏、`Show Agent Area` / Agent 助手 |
| | `Layout` / 布局 | `Manage Layouts` / 布局管理、`Reset Layout` / 恢复默认布局 |
| | `Appearance` / 外观 | 主题下拉框，前缀 `Theme` / 主题 |
| `Figure` / 绘图 | `Add Chart` / 添加绘图 | `Add Curve`、`Add Scatter`、`Add Error Bar`、`Add Box Plot`、`Add Bar`、`Add MultiBar`、`Add Histogram`、`Add Contour Map`、`Add Cloud Map`、`Add Vector Field`、`Add Surface 3D`、`Add Bar 3D`、`Add Line 3D` |
| | `Stats Plot` / 统计绘图 | `Histplot`、`KDE 1D`、`KDE 2D`、`Boxplot`、`Heatmap`、`Scatter`、`Barplot`、`Regplot`、`ECDF` |
| `AI Agent` / AI智能体 | 同名面板 | `Agent Manager` / agent 管理、提示词画廊、`Run Agent` / 执行 agent、`New Session` / 新建会话、`Session Manager` / 会话管理 |

`Clipboard` / 剪贴板 面板里的动作按当前获得焦点的窗口分派：焦点在画布上就作用于画布上的图元，焦点在表格上就作用于选中的单元格。绘图获得焦点时 `Copy` / 复制 把当前绘图整体复制到剪贴板，`Cut` / 剪切、`Paste` / 粘贴、`Delete` / 删除、`Select All` / 全选 不响应。撤销和重做在三个操作区之间共享同一个动作，作用对象随焦点切换。

`Figure` / 绘图 标签页的两组动作都是往绘图里加图表。选中一项之后会弹出建图向导，向导里选数据列和参数；当前没有绘图时，向导会先新建一个，见[创建图表](../chart/create.md)。`Stats Plot` / 统计绘图 组按列自动成图，不需要逐个选参数。

### 上下文标签页

三个上下文标签按当前获得焦点的操作区出现，焦点移开之后自动隐藏。

| 上下文标签 | 出现时机 | 页与面板 |
|------------|----------|----------|
| `DataFrame` | 数据操作区获得焦点 | 页 `Operate` / 操作：面板 `Axes` / 坐标、`Column` / 列、`Type` / 类型、`Format` / 格式；页 `Table Style` / 表格样式：面板 `Fill` / 底色、`Font` / 字体、`Clear` / 清除 |
| `Workflow` / 工作流 | 工作流操作区获得焦点 | 页 `Workflow Edit` / 工作流编辑：面板 `Clipboard` / 剪切板、`Item` / 图元、`Text` / 文本、`Background` / 背景、`Group` / 分组；页 `Workflow View` / 工作流视图：面板 `View` / 视图、`Export` / 导出（面板里是导出图片的菜单）；页 `Workflow Run` / 工作流运行：面板 `Run` / 运行 |
| `Chart` / 绘图 | 绘图操作区获得焦点 | 页 `Chart Style` / 绘图样式：面板 `Chart Setting` / 图表设置、`Figure Theme` / 绘图主题；页 `Chart Edit` / 图表编辑：面板 `Figure Setting` / 绘图窗口设置、`Select Tool` / 选区工具、`Chart Assist Tool` / 图表辅助工具 |

`Axes` / 坐标 面板管行列的增删和列名，`Column` / 列 面板里有定位到最大值、最小值这些按列的操作，`Type` / 类型 面板改列的数据类型，`Format` / 格式 面板管单元格里数字和日期的显示样子。

工作流相关的运行和停止只放在上下文标签的 `Run` / 运行 面板里。`Chart Edit` / 图表编辑 页里的辅助工具包含十字光标、标记线、数据探针这些读数工具，用法见[数据探针与数据联动表](../chart/data-probe.md)。

## 应用菜单与快速访问栏

应用按钮 `File` / 文件 点开之后依次是 `Open` / 打开、`Open Markdown` / 打开 Markdown、`Save` / 保存、`Save As` / 另存为，下面一组是 `Recent Files` / 最近打开的文件，列出最近打开过的工程，条目数上限在设置的高级页里改，默认 10 条。`Open Markdown` / 打开 Markdown 打开的 Markdown 文件显示在中间区域的一个标签页里，便于一边看说明一边操作。

快速访问栏里只有两个动作：撤销和重做。它们和主页 `Clipboard` / 剪贴板 面板里的两个按钮是同一对动作。

## 停靠面板

左侧、右侧和中央一共十一个停靠面板，见下表。

| 面板 | 内容 |
|------|------|
| `Workflow Node` / 节点 | 节点工具箱，列出所有可用节点，含 C++ 节点和 Python 节点包提供的节点 |
| `Charts Manager` / 绘图管理 | 所有绘图、坐标系和图表的分层列表，带一列可见性开关 |
| `Data Manager` / 数据管理 | 所有数据集的列表 |
| `Workflow Operate` / 工作流操作 | 工作流画布所在的操作区，内部按标签页打开多个工作流 |
| `Chart Operate` / 绘图操作 | 绘图所在的操作区，内部按标签页打开多个绘图 |
| `Data Operate` / 数据操作 | 表格视图所在的操作区，内部按标签页打开多个数据集 |
| `Setting` / 设置 | 属性面板容器，按当前对象显示对应的设置面板 |
| `Log` / 消息 | 消息日志，程序的操作反馈和警告记在这里 |
| `Agent Assistant` / Agent 助手 | 智能体对话区，内部按标签页打开多个会话 |
| `Data Link Table` / 数据联动表 | 汇总多个坐标系上数据探针读数的表格 |
| `Markdown Viewer` / Markdown 查看器 | 显示 Markdown 文件，只在打开 Markdown 文件时创建 |

面板标题栏可以拖动：拖到另一个停靠区上，高亮提示出现后松开，面板就停到新位置；拖出主窗口，或者双击标题栏，面板变成独立的浮动窗口。浮动窗口可以再拖回主窗口，回到停靠状态。

标题栏上的图钉按钮把面板收进最近的侧边栏（自动隐藏），只留下一个竖排标签，点击标签即可重新展开。程序启动时 `Log` / 消息 面板就是这种收起状态。

标题栏右侧的关闭按钮关闭面板。管理类和操作类面板没有这个按钮：`Workflow Node` / 节点、`Charts Manager` / 绘图管理、`Data Manager` / 数据管理，以及中央的三个操作区，都只能通过显示和隐藏侧边栏的方式收起，避免误关之后找不到入口。

## 左右侧边栏

左侧边栏由三个面板组成，它们同处一个停靠区，以标签页形式排列：`Workflow Node` / 节点、`Charts Manager` / 绘图管理、`Data Manager` / 数据管理。

右侧边栏由 `Setting` / 设置、`Agent Assistant` / Agent 助手、`Data Link Table` / 数据联动表 三个面板组成，同样共用一个停靠区。`Log` / 消息 是右侧的自动隐藏面板，鼠标移到它露出的标签上不会自动展开，点击才展开。

侧边栏的显示与隐藏有两个入口，效果一样：视图标签页 `Display` / 视图显示 面板里的 `Show Left Side Bar` / 左侧边栏 和 `Show Right Side Bar` / 右侧边栏；状态栏最左边和最右边的两个按钮。右侧边栏的开关会同时收起 `Setting` / 设置 和 `Log` / 消息。

## 状态栏

状态栏从左到右依次是四个部分：

- 左侧边栏开关按钮。
- 三个功能域切换按钮，文字是 `Workflow` / 工作流、`Data` / 数据、`Chart` / 图表，分别把对应的管理面板和操作区提到前面，同时把 `Setting` / 设置 面板一起提上来。
- 消息区和进度指示，消息显示几秒后自动淡出，进度条和进度文字只在长时间任务执行时出现，比如保存和加载工程。
- 右侧边栏开关按钮。

## 布局管理

视图标签页 `Layout` / 布局 面板里的 `Manage Layouts` / 布局管理 打开布局管理对话框，窗口标题是 `Layout Manager` / 布局管理。列表里默认有两套预置方案，预置方案不能删除也不能覆盖：

- `Default` / 默认布局，程序启动时的布局。
- `Focus Analysis` / 专注分析，在默认布局的基础上收起两侧边栏。

对话框右侧四个按钮各有明确用途。

| 按钮 | 作用 | 相关提示 |
|------|------|----------|
| `Save Current` / 保存当前布局 | 把当前窗口布局存成一套命名方案 | 输入框标题 `Save Layout` / 保存布局方案，提示 `Layout scheme name:` / 布局方案名称：；名字与预置方案重名时提示 `Cannot overwrite preset layout schemes, please use another name` |
| `Apply Selected` / 应用选中布局 | 把选中的方案套到当前窗口上 | 列表提示 `Double-click a scheme to apply it` / 双击方案可直接应用，双击列表项等同于点这个按钮 |
| `Delete Selected` / 删除选中布局 | 删除选中的自定义方案 | 删除前确认一次，提示里带上方案名字，确认后从磁盘删除 |
| `Reset Default` / 恢复默认布局 | 恢复默认窗口布局，需确认 | 确认语 `This will restore the default window layout. Continue?` / 将恢复默认窗口布局，是否继续？ |

自定义方案保存在配置保存路径下的 `dawork-layouts.json` 里，程序退出不丢。`Layout` / 布局 面板里的 `Reset Layout` / 恢复默认布局 与对话框里的 `Reset Default` / 恢复默认布局 是同一件事：先确认一次，再在当前会话里直接恢复默认快照，成功后提示 `Default layout restored` / 已恢复默认布局。

## 主题与 Ribbon 样式

主题改变 Ribbon 和窗口控件的配色，共 10 个，列在视图标签页 `Appearance` / 外观 面板的下拉框里，也列在设置对话框通用页的 `Ribbon theme` / Ribbon 主题 下拉框里：`Windows 7` / Windows 7、`Office 2013` / Office 2013、`Office 2016 Blue` / Office 2016 蓝色、`Office 2016 Green` / Office 2016 绿色、`Office 2016 Dark` / Office 2016 深色、`Office 2021 Blue` / Office 2021 蓝色、`Office 2021 Green` / Office 2021 绿色、`Office 2021 Dark` / Office 2021 深色、`Dark` / 深色、`Dark 2` / 深色2。选中后立即生效，并写进配置文件，下次启动沿用。

Ribbon 样式控制标签和面板的行数与紧凑程度，共四种，在设置对话框通用页里以带预览图的单选按钮呈现：`Standard Mode` / 标准模式、`Standard Mode With 2 Row` / 2 行标准模式、`Compact Mode` / 紧凑模式、`Minimalist mode` / 最小模式。程序首次运行时最后一种处于选中状态。

设置对话框通用页里还有 `Operation window tab position` / 操作窗口标签页位置，决定工作流、绘图、数据三个操作区内部标签页排在上方还是下方，改动立即生效。

## 界面语言

设置对话框通用页里的 `Interface language` / 界面语言 下拉框决定界面文字用哪种语言。第一项是 `System` / 跟随系统，表示跟随操作系统语言。翻译文件的名字是 `da_<语言代码>.qm`，程序在程序目录的 `translations/` 和 `translations_user/` 两个目录里查找，随程序发布的翻译有 `da_en_US`（英文）和 `da_zh_CN`（简体中文）；语言代码也可以直接写进配置文件 `dawork-config.json` 的 `ui.language` 项。

页面上注明语言和字体的改动在重启程序后生效，改完需要关闭并重新启动程序。Ribbon 和插件页面上的文字也会跟着一起切换。

## 界面状态的保存与恢复

窗口位置、窗口大小、各停靠面板的位置与浮动态、工具栏状态可以记录下来，下次启动照原样打开。这一功能由设置对话框通用页里的复选框 `Is the geometry and status of the program interface recorded` / 是否记录程序界面几何和状态 控制，默认不勾选。

勾选时，关闭程序会把界面状态写进配置保存路径下的 `.dawork-ui-state` 文件；不勾选时，关闭程序会删掉这个文件，下次启动用默认布局。窗口状态在启动时、插件初始化之前恢复，插件随后对界面做的改动不会被打回原样。同一页里的 `Clear the saved window state` / 删除窗口状态记录文件 按钮手动删除该文件，确认语是 `This operation will delete the file that records the window state information. After deleting the file, if the window state information recording is not enabled, the window will open in the default layout` / 此操作将删除记录窗口位置信息的文件，删除文件后，如果不开启窗口位置信息记录，窗口将以默认布局打开。删除成功后在消息面板里记一条 `Successfully removed window state record file` / 成功删除窗口状态记录文件。

## 常见问题

面板不见了。先看视图标签页 `Display` / 视图显示 面板里的显示动作，能逐项把面板提出来；仍然找不回来时用 `Reset Layout` / 恢复默认布局 重置整个布局。

每次启动窗口布局都变回默认。通用页里的界面状态复选框没有勾选，程序的默认行为是关闭即删除 `.dawork-ui-state`。勾上这个复选框再重启一次。

中文界面里仍然夹杂英文。部分动作和字段的翻译尚未补全，界面语言本身已经切换生效，可以对照正文里的英文名找到对应项。

## 相关章节

- [安装与首次启动](install.md)：运行环境、Python 解释器、第一次启动要做的事。
- [工程文件](project.md)：`.dapro` 文件里装了什么，什么时候必须保存。
- [设置项说明](../reference/settings.md)：设置对话框里每一个页面的每一项。
- [表格视图](../data/table.md)：数据操作区里能做的编辑操作。
- [工作流基础](../workflow/basics.md)：工作流操作区里怎么放节点、连端口、运行。
- [会话与对话](../agent/session.md)：Agent 助手面板的用法。
