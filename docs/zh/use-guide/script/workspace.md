# 脚本工作区

工程自带一个放脚本的目录，称为脚本工作区。它随工程文件保存，打开工程时解压到磁盘上的一个本地目录，脚本运行时的当前目录就是这个本地目录。这一章讲工作区的位置和保存规则、脚本的执行环境与可用模块，再用三个完整例子演示怎么读写数据集、怎么把图画到软件自己的绘图里。

软件里写和运行 Python 的位置有两处：智能体的 `run_code` 与 `run_script` 工具，以及工作流里的 Python 节点。插件对话框也是由 Python 支撑的，参数通过表单填写，属于可配置而不是可编程，不在这两处之内。软件没有独立的脚本编辑器，也没有断点、单步这类调试功能，脚本需要用外部编辑器写好放进工作区，再通过智能体运行。两个工具的完整参数见[内置工具](../agent/tools.md)，节点里的 Python 怎么执行见[开发自定义节点](../workflow/custom-node.md)。

## 工作区在哪里

工程文件是一个 ZIP 压缩包。工作区就是压缩包里的 `workspace/` 成员，里面是一棵普通目录树，放什么、怎么分目录由使用者决定，常见的做法是建一个 `scripts/` 放脚本、一个 `output/` 放脚本生成的图片和数据文件。保存工程时本地工作区整棵打包进这个成员，状态栏上的任务名是 `Save script workspace` / 保存脚本工作区，说明文字是 `Pack local script workspace into the project file` / 把本地脚本工作区打包进工程文件；打开工程时再解压回本地。压缩包成员的完整清单见[工程文件](../getting-started/project.md)。

程序不直接在压缩包内的成员上工作。打开工程时，包里的 `workspace/` 解压到磁盘上一个本地缓存目录，脚本看到的是这个本地目录，状态栏上的任务名是 `Load script workspace` / 加载脚本工作区。解压这一步失败不会阻止工程打开，工程其余部分照常加载，只是本地目录里缺文件，之后运行脚本时会报找不到脚本。

缓存目录的位置按两条规则算出来。

- 根目录取自设置对话框高级页的 `Workspace directory` / 工作区目录，这一项留空时用操作系统的临时目录（Windows 上就是 `%TEMP%`）。
- 目录名是 `<工程文件基名>_<工程完整路径的 sha1 前 8 位>`。工程文件是 `D:/proj/myproject.dapro`、工作区目录设为 `D:/da-workspace` 时，本地目录是 `D:/da-workspace/myproject_` 加八位十六进制哈希。

哈希取自工程文件的完整路径，所以工程改名、移动目录或另存为之后，本地目录会换成另一个；同一个工程在同一台机器上打开多次，用的是同一个本地目录。工作区目录这一项的改动在下次打开工程时生效，当前打开的工程仍然用旧的根目录，设置页上也注明了这一点。

本地目录同时是脚本的工作目录，工作区根目录还会被追加进 `sys.path`，放在工作区根目录下的模块文件可以直接 `import`；放在子目录里的模块仍需脚本自己把子目录写进 `sys.path`。工作区里的文件用智能体的 `write_file` 工具或外部编辑器写入，脚本产生的输出文件留在这个目录里，随工程一起保存。本地目录不会被软件自动清理，脚本反复运行时按自己的命名规则覆盖旧文件；要清空它，把里面的文件删掉再重新打开一次工程即可。

### 工程保存之前

本地目录名要用工程文件的完整路径算哈希，工程没有保存过时算不出这个路径，工作区此时不存在。这个阶段的差别是：

| 操作 | 未保存的工程 | 已保存的工程 |
|------|--------------|--------------|
| `run_code` 内联代码 | 可用 | 可用 |
| `run_script` 运行工作区里的脚本 | 拒绝，返回 `the project has not been saved yet, script workspace unavailable; save the project first` | 可用 |
| 运行工作流 | 拒绝，弹出需要先保存工程的提示 | 可用 |
| 工作区目录 | 不存在 | 打开工程时建立 |

工作流在没有保存的工程里不能运行，原因相同，见[工作流基础](../workflow/basics.md)。先保存一次，后面就不受这条限制。

### 打开工程时的覆盖策略

本地缓存目录里可能留着上一次的文件，与工程包内保存的版本不一致时，程序在加载之前弹出标题为 `Script Workspace Conflict` / 脚本工作区冲突 的对话框，正文显示本地目录的完整路径，让人选一边：

| 按钮 | 后果 |
|------|------|
| `Keep Local` / 保留本地 | 跳过解压，本地文件原样保留；工程被标记为有未保存改动，下次保存把本地版本写回包内 |
| `Overwrite With Project Version` / 用工程内版本覆盖 | 先清空本地目录，再按包内版本解压 |
| `Cancel` / 取消 | 不加载工程，保持打开之前的状态 |

两边逐文件比对下来内容一致时不弹窗，直接按包内版本解压；比对按文件内容算，与修改时间无关。默认每次都问，想固定成一种做法，在 `dawork-config.json` 的 `script` 组里把 `workspace-overwrite-policy` 改成 `always-local`（一直保留本地）或 `always-zip`（一直用包内版本覆盖）。设置对话框里没有这一项的入口，改完配置文件需要重启程序。

不一致的来源有两类：本地目录里留着上一次运行脚本写出的新文件，而工程包内的版本是保存时的旧样子；或者工程文件在别处被改过（另一台机器上运行并保存过），本地目录反而是旧的。第一种情况通常选保留本地，第二种情况通常用包内版本覆盖。拿不准时先取消，手动打开本地目录看一眼文件再说，取消不会改动任何一边。

### 脚本文件怎么进工作区

工作区有两份副本：工程包里的 `workspace/`，和磁盘上的本地缓存目录。运行脚本时用的是本地目录，编辑和增删也都在本地目录上做，改完保存工程才会同步回包内。往工作区里放文件常用三种方式：

- 让智能体用 `write_file` 工具把脚本内容写到工作区里的相对路径下，适合由对话生成脚本。
- 在外部编辑器里直接编辑本地缓存目录里的文件，适合反复修改同一份脚本。
- 把已经写好的脚本文件夹整个复制进本地缓存目录，再保存一次工程把它收进包里。

工作区不限于 `.py` 文件，数据文件、配置文件、脚本生成的图片和表格都可以放在里面，它们随工程保存，但不会被执行，只有 `run_script` 指向的 `.py` 文件会被运行。放进去的文件越大，工程文件越大，大量中间结果留在工作区里会让保存和打开变慢。

## 执行环境

### 持久的命名空间

脚本在一个持久的命名空间里执行，同一个位置多次运行，变量会保留下来：上一次留下的 `df`、`model` 这些名字，下一次直接可用。命名空间按智能体会话隔离，两个会话各有一份变量表，会话 A 里的变量在会话 B 里看不到，会话删除时它那份命名空间一并释放。

工作流里的 Python 节点与没有会话上下文的调用方共用另一份默认命名空间。打开另一个工程时，默认命名空间里的用户符号被清掉，回到下面这份基线。

保留的不只是变量，import 进来的模块、函数定义、全局常量一样留在命名空间里，第二次运行可以直接用，省去重复读文件和重复计算。代价是脚本之间互相影响：前一次运行里改过全局状态（比如 pandas 的显示选项），后一次运行会继承这个改动。要拿到干净的环境，重启软件，或者在脚本开头显式重置用到的设置。

两个会话的隔离是双向的：会话 A 里算了一半的中间结果，会话 B 拿不到。需要跨会话传东西，把结果写成工作区里的文件，另一个会话从文件读；需要长期复用的逻辑写成工作区里的模块文件，两边都 import 它。

命名空间建立时预置了这些名字：

| 名字 | 内容 |
|------|------|
| `da_app` | 应用入口，`getCore()` 返回核心对象 |
| `da_interface` | 界面与数据管理接口 |
| `da_data` | 数据集对象与数据管理器类型 |
| `__name__` | 固定为 `"__main__"` |
| `args` | 调用方传入的参数，字典，没有参数时为空字典 |
| `__result__` | 脚本的返回值，初始为空 |

前三个模块开箱即用，不用写 `import`。脚本也可以自己再 import 一次，两者是同一个模块对象。

### 两个运行入口

智能体通过两个工具运行 Python 代码，它们的区别是代码从哪里来：

| 工具 | 代码来源 | 前提 | 参数 |
|------|----------|------|------|
| `run_code` | 工具调用里内联的一段代码 | 无，工程未打开也能用 | `code`、可选的 `args` |
| `run_script` | 工作区里的一个 `.py` 文件 | 工程已保存且文件在工作区内 | `path`、可选的 `args` |

`run_script` 的 `path` 是相对工作区根目录的路径，比如 `scripts/analyze.py`，也可以写成工作区里的绝对路径，但不能越出工作区，越界的路径被拒绝。脚本必须以 `.py` 结尾，内容按 UTF-8 读取。工具会重新读一次文件内容核对与安全判定时看到的是否一致，两者不同时拒绝执行。

两个工具的参数 `args` 会作为同名字典注入命名空间，脚本里用 `args["键名"]` 取值。两个工具都属于代码执行一级，受权限设置约束：全自动模式直接运行，自动模式在没有配置代码判官时每次都会弹审批卡片，每次询问模式每次都问。审批卡片的样子与判定规则见[权限控制](../agent/permission.md)。

两个工具返回的都是文本结果，智能体读这段文本而不是拿到可以继续操作的对象。执行引擎返回的结果里有这几个字段：

| 字段 | 内容 |
|------|------|
| `success` | 是否执行成功 |
| `result` | `__result__` 序列化后的文本 |
| `stdout` | 脚本 print 写出的内容 |
| `stderr` | 标准错误上写出的内容 |
| `error` | 失败原因，或超时信息 |

脚本中途抛出异常时 `success` 为假，异常文本放在 `error` 里，异常之前已经写进标准输出的内容仍然会回传。

### 运行脚本时的审批

`run_code` 和 `run_script` 属于代码执行一级，受权限设置约束。默认权限模式是全自动，两者直接运行；自动模式下没有配置代码判官时，每次代码执行都会弹出审批卡片；每次询问模式下每次都问，与脚本内容无关。代码执行的审批卡片显示代码的前 15 行，更多行折叠成一行提示，按钮是 `Approve` 和 `Deny`。`Approve & remember for this session` / 批准并本会话记住 只对写文件一级生效，代码执行不记忆，下一次执行仍然要问。模式和规则在设置对话框的 Agent Permission Settings 页调整，判定过程见[权限控制](../agent/permission.md)。

### 可以导入的模块

除了三个预置模块，脚本里还可以用下面几类模块。

| 类别 | 模块 | 说明 |
|------|------|------|
| 内嵌模块 | `da_pyplot` | 画到软件绘图里的 pyplot 风格接口，用法见下文第三个例子 |
| 内嵌模块 | `da_figure` | 当前坐标系与图元操作的底层接口 |
| 内嵌模块 | `da_py_workflow` | 工作流相关的 Python 接口 |
| 软件自带包 | `DAWorkbench.DAPyBase`、`DAWorkbench.DAStatistics`、`DAWorkbench.DAWorkflowPy` | 由软件安装进 Python 环境，随版本更新 |
| 插件包 | `DASystemNodes`、`DADataAnalysisCore`、`DADataAnalysisGui`、`DADataAnalysisNodes` | 随插件安装 |
| 第三方包 | `pandas`、`numpy`、`scipy`、`matplotlib` 等 | 详见下文第三方包一节 |

内嵌模块只在软件进程里存在，用系统里另装的 Python 单独跑同一段代码会导入失败。`da_figure` 提供当前坐标系的句柄和给图元设置画笔、画刷、符号样式的方法，`da_pyplot` 是在它上面封装的绘图接口，日常画图用后者；`da_py_workflow` 面向工作流节点脚本，节点里通过它读写端口数据，用法见[开发自定义节点](../workflow/custom-node.md)。插件包与插件目录下的节点包同名时，导入到的是当前启用的那一份。

### 读写工程里的数据

脚本读写的是内存里的数据集对象，不经过文件。数据管理接口上的这些方法在脚本里都可以直接调用：

| 方法 | 返回 | 用途 |
|------|------|------|
| `getAllDataframes()` | 字典，键是名称，值是 DataFrame | 一次拿到全部数据框的数据集 |
| `getAllDatas()` | 数据集对象列表 | 包含数据框和 Series |
| `getData(索引)` | 一个数据集对象 | 按位置取 |
| `findData(名字)` | 一个数据集对象 | 按完整名字取，默认区分大小写 |
| `addDataframe(df, 名称)` | 无 | 把 DataFrame 登记成新数据集 |
| `removeData(数据集对象)` | 无 | 删除数据集 |

`getAllDataframes()` 返回的是内存里那个 DataFrame 对象本身，没有再复制一份。数据集对象的 `toPyObject()` 拿到底层 DataFrame，`getName()`、`getDescribe()` 取名称和描述。把 DataFrame 加回数据管理面板用 `addDataframe(df, 名称)`；名称与已有数据集重复时，软件会在后面补 `_1` 这样的后缀保证唯一，实际加进去的名字可能与传入的不同。

直接改一个已有数据集的内容之后，打开着的表格视图不会自己知道数据变了，需要补一次变更通知。下面这段代码取到名为 `exp1` 的数据集，把第一行的 `mass` 列改成 10.0，再通知界面刷新。`findData` 按完整名字查找；`data.getDataManager()` 返回这个数据集所属的数据管理器；`da_data.DataChangeType.Value` 表示数据内容发生了变化，枚举的其它取值是 `Name`、`Describe`、`ColumnName`。

```python
import da_app
import da_data

manager = da_app.getCore().getDataManagerInterface()
data = manager.findData("exp1")
df = data.toPyObject()
df.loc[0, "mass"] = 10.0
data.getDataManager().notifyDataChangedSignal(data, da_data.DataChangeType.Value)
```

运行后数据管理面板里这个数据集的内容已经变了，打开着的表格视图会重读单元格，窗口标题上出现未保存标记。省掉最后一行时数据一样被改掉，只是表格视图停在旧内容上，直到重新打开这个数据集。

一次要加好几个数据集时，逐个添加会让面板和树模型刷新多次。用 `setSuppressSignals(True)` 压住逐条通知，全部加完再调 `emitDatasBatchAdded()` 通知一次，界面只刷新一遍。下面这段代码把两个 DataFrame 作为两个数据集一次加完。

```python
import da_app

manager = da_app.getCore().getDataManagerInterface()
manager.setSuppressSignals(True)
manager.addDataframe(df_a, "run_a")
manager.addDataframe(df_b, "run_b")
manager.setSuppressSignals(False)
manager.emitDatasBatchAdded()
```

`setSuppressSignals` 传 True 之后数据照常登记，只是不逐条发通知；传回 False 恢复。`emitDatasBatchAdded()` 一个调用补发这期间累积的通知，数据管理面板随后一次性列出新加的数据集。忘了调最后一行时数据集在内存里已经存在，但面板上要等下一次刷新才能看到。

删除数据集用接口的 `removeData(数据集对象)`，对象从 `findData` 或 `getAllDatas()` 拿；名字不存在时 `findData` 返回空对象，直接往下用会报错，取之前先判空。

数据集里也有 Series 类型，`getAllDataframes()` 不包含它们。用 `getAllDatas()` 取到全部数据集对象，用对象上的 `isDataFrame()`、`isSeries()` 判断类型，再用 `toPyObject()` 拿到 pandas 的 DataFrame 或 Series。要加一个 Series 数据集，用接口上的 `addSeries(序列, 名称)`，参数顺序与 `addDataframe` 相同。

工作表节点通过端口收发 DataFrame，要把结果发回数据管理面板，用 `Output to DataManager` 节点，见[数据分析节点手册](../workflow/nodes-analysis.md)。

### 返回一个结果值

脚本最后的表达式不会被自动返回，要给出返回值，把结果赋给 `__result__`。下面这段代码统计名为 `exp1` 的数据集有多少行多少列，并把结果做成一个字典返回。

```python
import da_app

df = da_app.getCore().getDataManagerInterface().getAllDataframes()["exp1"]
__result__ = {"rows": int(df.shape[0]), "columns": int(df.shape[1])}
```

字典属于内建类型，序列化时先走 JSON，调用方看到的是 `{"rows": 120, "columns": 5}` 这样的文本；DataFrame 直接赋给 `__result__` 时，调用方看到的是形状加前 100 行的表格文本。不赋值时返回值为空。

### 运行方式与限制

脚本在界面线程上同步执行。运行期间软件的窗口不响应鼠标和键盘，长脚本会让整个界面停住，和运行工作流时的表现一样。

执行超时由设置对话框高级页的 `Script execution timeout` / 脚本执行超时 控制，单位秒，取值范围 0 到 3600，0 显示为 `Disabled` / 禁用，初始值 300。到达时限时软件向解释器注入一次 `KeyboardInterrupt`，脚本随之中断，调用方收到的错误是 `script execution timed out after <秒数> seconds`。脚本如果捕获了这个中断并继续跑完，错误字段里记的是 `timeout fired but script swallowed KeyboardInterrupt`，流程仍然正常结束。

同一个执行引擎一次只跑一个脚本。前一个还没结束时再执行一次，会直接返回错误 `another script is running`，不会排队。软件也没有中途停止单个脚本的按钮，能停的只有整轮智能体回复，见[会话与对话](../agent/session.md)。

需要反复修改的代码用 `run_script` 跑工作区里的文件，把代码放在文件里改一处跑一次，比在对话里反复重发内联代码清楚。`run_code` 适合十几行以内、用一次就丢的片段，比如查一下某个数据集有哪些列。两者共用同一个命名空间，先用 `run_code` 试探的数据，接着用 `run_script` 跑正式脚本时可以继续用。

脚本返回值放在 `__result__` 里，由软件序列化成文本回给调用方。序列化结果超过 `Script result max characters` / 脚本结果最大字符数 时截断，末尾加 `... (truncated)`；这一项取值范围 1000 到 100000，初始值 10000。截断只影响回传的文本，不影响脚本内的数据。

脚本用 `print` 写出的内容会被捕获：标准输出和标准错误分别收集，随执行结果一起返回，智能体的工具调用卡片里能看到这段文本。捕获用内存缓冲实现，调用 `sys.stdout.fileno()` 的库在脚本里会报错。工作流节点里的 print 写进 Python 日志文件，与这里的回传是两条路。

运行中的脚本没有调试手段：没有断点、单步，也不能在软件里打开脚本编辑。要看中间结果，用 `print` 或者给 `__result__` 赋值，再让智能体把结果读出来。需要反复调试的脚本建议在外部编辑器里用一个独立的 Python 环境先跑通逻辑，再把数据和绘图相关的调用换成这里的接口。

## 一个典型的脚本流程

因为没有脚本编辑器，日常使用按下面这个顺序走。前三步只需要做一次。

1. 保存工程，让工程文件落到磁盘上。工作区目录由工程路径推算，没有保存过的工程没有工作区。
2. 准备脚本。两种情况：在外部编辑器里写好，再用智能体的 `write_file` 工具把内容写进工作区；或者直接让智能体按描述把脚本写进工作区，自己再用编辑器打开本地缓存目录里的这份文件核对。
3. 记住本地缓存目录的完整路径，打开工程时的冲突对话框里显示过它，工程文件路径和工作区根目录也能自己推出来。
4. 让智能体运行脚本，用工作区里的相对路径指定，比如 `scripts/analyze.py`。
5. 看工具调用卡片里的标准输出、返回值或错误文本，按结果改脚本，回到第 4 步。
6. 收尾时保存工程。工作区里的脚本和脚本生成的输出文件一起进包，换台机器打开时工作区跟着走，不用单独拷贝这些文件。

脚本运行需要审批时，卡片会显示要执行的代码或脚本路径，批准之后才开始跑，见[权限控制](../agent/permission.md)。

## 脚本结果与工程保存

脚本在命名空间里留下的变量只活在本次运行期间，保存工程不会把它们存进去，重启软件或删掉会话就没了。下面三类东西会跟着工程保存：

| 产物 | 保存到哪里 |
|------|------------|
| 加进数据管理面板的数据集 | 工程的 `data-manager.xml` 与 `datas/`，重新打开后还在面板里 |
| 画进坐标系里的图元 | 工程的 `charts.xml` 与 `chart-data/`，重新打开后图表照旧显示 |
| 工作区目录里的文件 | 工程的 `workspace/`，重新打开工程时解压回本地 |

想让一段计算下次还能用，把它写成工作区里的脚本文件，或者把结果登记成数据集；只在命名空间里留一个变量是不够的。

### 什么时候用脚本、什么时候用节点

两处都跑 Python，分工按结果要不要重复得到来分。脚本适合一次性的探索：试几种算法、临时看一下数据的分布、把一批文件的处理写死成循环，跑完看输出即可。工作流节点适合要反复运行、要交给别人的流程：参数在节点面板上填，整条流程一键重跑，结果用 `Output to DataManager` 节点发布成数据集，过程可以被别人复核。

两者也能配合：在脚本里把一段逻辑调通，再按[开发自定义节点](../workflow/custom-node.md)把它包成节点放进工作流；或者反过来，用工作流把数据准备好，再用脚本对数据管理面板里的结果做探索性分析。脚本里的 import 和函数定义不进入工作流的序列化内容，节点不会调用工作区里的脚本文件。

## 例一：列出所有数据集

第一个例子只读数据。`da_app.getCore()` 拿到应用核心对象，`getDataManagerInterface()` 从核心对象里取出数据管理接口，`getAllDataframes()` 返回全部 DataFrame 数据集组成的字典。

```python
import da_app

core = da_app.getCore()
manager = core.getDataManagerInterface()

for name, df in manager.getAllDataframes().items():
    print(name, df.shape)
```

`items()` 每次给出一对名称和 DataFrame，`df.shape` 是 pandas 的（行数，列数）元组。字典里只有数据框类型的数据集，Series 类型不在其中，用接口的 `getAllDatas()` 可以拿到包括 Series 在内的全部数据集对象。

运行后工具调用卡片里出现每个数据集的名称和括号里的行列数，比如 `exp1 (120, 5)`。数据管理面板里的数据集个数与打印出来的行数一致。

改这段代码的两个位置就能得到别的信息：把 `df.shape` 换成 `list(df.columns)` 打印列名，换成 `df.dtypes` 打印每列的类型；把 `df.shape` 换成 `df.head(3)` 时，print 会输出前三行内容，行数多的时候输出很快变长，受结果字符数上限截断。

## 例二：算一个新列并加回数据集

第二个例子读一个数据集，逐个元素算出一个新列，再把结果作为新数据集加进数据管理面板。假设数据集名为 `exp1`，里面有 `mass` 和 `volume` 两列，要加一列 `ratio` 等于两者相除。

```python
import da_app

manager = da_app.getCore().getDataManagerInterface()
df = manager.getAllDataframes()["exp1"]
df = df.copy()
df["ratio"] = df["mass"] / df["volume"]
manager.addDataframe(df, "exp1_ratio")
```

第一行取出数据管理接口；第二行按名字取数据集里的 DataFrame，名字写错时抛出 `KeyError`；第三行先复制一份，避免改动影响原数据集；第四行把两列逐行相除，得到的新列直接挂在 `df` 上；第五行把这份 DataFrame 登记成新数据集，名字是 `exp1_ratio`。

运行后数据管理面板里多出一个名为 `exp1_ratio` 的数据集，双击打开表格视图，末尾多出 `ratio` 列；原来的 `exp1` 内容不变。如果 `exp1_ratio` 已经存在，新数据集会被改名为 `exp1_ratio_1` 这样的形式。

换一个计算时改第四行的表达式，参与运算的列名按数据集的真实列名写，除法、乘法、函数调用都是普通的 pandas 写法。想去掉第三行的复制、让结果改在 `exp1` 上，得改成前面变更通知的写法，先拿 `DAData` 对象、改完再通知刷新；不通知时表格视图不会跟着变。按条件算值用 `numpy.where`，逐行写自定义逻辑用 `df.apply`，这些函数在脚本里直接可用。

## 例三：用 da_pyplot 把图画到当前绘图里

第三个例子把图画出到软件的绘图标签页上。前提是已经有一个绘图并且里面有一个坐标系：在「Figure」/ 绘图 标签页的「Add Chart」/ 添加绘图 面板里点 `Add \nFigure` / 添加绘图 建绘图，再用「New \nXY Axis」/ 新建坐标系 建一个坐标系，并把这个绘图标签页切到前面。没有打开任何坐标系时，`da_pyplot.gca()` 抛出 `da_pyplot.gca: no active chart available`。

```python
import da_pyplot

plt = da_pyplot.gca()
plt.setTitle("Decay of sample A")
plt.setXLabel("time / s")
plt.setYLabel("amplitude / mV")
plt.plot([0.0, 1.0, 2.0, 3.0, 4.0], [10.0, 4.1, 1.8, 0.9, 0.5], "r-o", label="sample A")
plt.setYScale("log")
plt.grid(True)
plt.legend("best")
```

`da_pyplot.gca()` 取得当前坐标系（当前绘图标签页里活动的那个坐标系）的 PyPlot 对象，后面所有调用都作用在它上面，不需要指定坐标系。`setTitle`、`setXLabel`、`setYLabel` 设置标题和两个坐标轴的标签，文字直接显示在坐标系上。`plot` 的前两个参数是 x 和 y 两组数，第三个参数是格式串：`r` 是红色、`-` 是实线、`o` 是圆点标记，第四个参数是图例里显示的名字。`setYScale("log")` 把 Y 轴换成对数刻度，参数写 `"linear"` 换回线性刻度。`grid(True)` 打开网格，`legend("best")` 加上图例、位置自动选择。

这段脚本刻意没有调用 `savefig`，原因见下一节。

绘图方法返回的是图元对象，可以在脚本里继续用 `da_figure` 模块设置颜色、线型这些属性，也可以在画完之后在属性面板里选中这个图元改参数，两种方式作用在同一批对象上。

### 对数坐标只能从脚本打开

坐标系属性面板的坐标轴页里只有 `Normal` / 普通 和 `DateTime` / 日期时间 两种刻度样式，`setXScale` 和 `setYScale` 是打开对数坐标的唯一途径：取值写 `"log"` 时该轴换成对数刻度，写其它值换回线性刻度。X 轴和 Y 轴各调一次。

### 保存成图片文件：这条路当前走不通

`da_pyplot` 的 `savefig` 在界面上取到的对象上不起作用。它要求 PyPlot 对象带有一整块绘图画布，而 `da_pyplot.gca()` 给出的是单个坐标系形式的对象，调用时直接返回假值，日志文件里留下 `QwtPyPlot::savefig: no figure available for saving`，目标路径上不会出现文件。

脚本里要把图表存成图片，改用智能体的 `save_chart_image` 工具，它按坐标系控件渲染，PNG、PDF、SVG 都能出，尺寸也能指定，见[内置工具](../agent/tools.md)。绘图标签页自身没有导出图片的菜单项，界面上能做的只有复制到剪贴板，两条路的分工见[复制与保存图表](../chart/save.md)。

运行后的结果是：绘图标签页上出现一条带圆点标记的红色曲线，Y 轴变成对数刻度，坐标系顶部是标题、两轴带标签，网格和图例同时出现。在属性面板的图元下拉框里能看到这条曲线，可以在那里继续改它的画笔和标记样式。

脚本重复运行会在同一个坐标系上叠加同样的曲线，每跑一次多一条。命名空间里的变量会保留，但绘图里的图元不会自动替换。要重画，先在属性面板的图表区条目里删掉旧曲线，或者用智能体的 `remove_chart_item` 工具按标题删除，再运行脚本。

在这段代码上加第二条曲线，再调一次 `plot` 即可，两条曲线的图例名不同就会在图例里各占一行。把 `"r-o"` 换成 `"b--"` 之类的格式串改线型和颜色；`setXLim`、`setYLim` 手工指定坐标轴范围，适合自动缩放看不清楚的地方；`setXTicks`、`setYTicks` 传一组位置和一组标签，把刻度换成需要的文字。这些调用都在画完数据之后执行，按脚本里的先后顺序生效。

### da_pyplot 的常用方法

下面按用途列出这个模块的常用方法。所有绘图方法都作用在当前坐标系上，参数里的 x、y 是数字序列，二维数据用嵌套列表。

| 用途 | 方法 | 说明 |
|------|------|------|
| 折线 | `plot(x, y, fmt, label)` | 只给一组数时按序号做 x |
| 散点 | `scatter(x, y, size=20, color, label)` | 只有标记 |
| 柱状 | `bar(values)` 或 `bar(x, values, width=0.8)` | 分类数据的柱状图 |
| 直方图 | `hist(data, bins=10)` | 原始数据自动分箱 |
| 箱线图 | `boxplot(字典列表)` | 每项给 `position`、`whiskerLower`、`q1`、`median`、`q3`、`whiskerUpper` |
| 区间与误差 | `fillBetween(x, y1, y2, alpha=0.3)`、`errorbar(x, y, yerr)` | 填充带和误差棒 |
| 矩阵类 | `imshow(二维列表, cmap)`、`contour(二维列表, levels, cmap)`、`quiver(x, y, u, v)` | 图像、等高线、向量场 |
| 辅助线 | `grid(True)`、`axhline(y)`、`axvline(x)`、`axhspan`、`axvspan` | 网格与标记线 |
| 文字 | `annotate(text, xy, xytext)`、`legend(loc)` | 标注与图例 |
| 坐标轴 | `setTitle`、`setXLabel`、`setYLabel`、`setXLim`、`setYLim`、`setXScale`、`setYScale`、`setXTicks`、`setYTicks`、`invertXAxis`、`invertYAxis` | 标题、范围、刻度与对数刻度 |
| 外观 | `setFaceColor`、`setAxesColor`、`colorbar` | 背景色与色条 |
| 输出与交互 | `show`、`enablePan`、`enableZoom` | 刷新与平移缩放开关（`savefig` 在当前对象上不起作用，见上文） |

`plot`、`scatter` 这类方法的 `fmt` 参数用 matplotlib 风格的格式串，一个字符组里可以同时写颜色、线型和标记：

| 组成 | 取值 | 含义 |
|------|------|------|
| 颜色 | `b`、`g`、`r`、`c`、`m`、`y`、`k`、`w` | 蓝、绿、红、青、品红、黄、黑、白 |
| 线型 | `-`、`--`、`-.`、`:` | 实线、虚线、点划线、点线 |
| 标记 | `o`、`s`、`^`、`v`、`D`、`x`、`+`、`*`、`.` | 圆、方、上三角、下三角、菱形、叉、加号、星、点 |

格式串里不写线型时只有标记，比如 `"ko"` 得到黑色圆点；什么都不写时用默认外观。颜色也可以单独用 `color` 参数传。

### 多坐标系

从脚本用 `gca()` 拿到的 PyPlot 作用在一个坐标系上。类上还有一组多子图方法，`subplot(rows, cols, index)`、`addAxes(rect)`、`twinx()`、`twiny()`、`tightLayout()` 以及 `gcf()`、`sca()`，这组方法要求对象带整块绘图画布，从脚本拿到的单坐标系对象调用它们不会有结果，日志里留下 `no figure available`。需要多个坐标系时，在绘图标签页上用「New \nXY Axis」新建，再对当前坐标系作图。

## 第三方包的来路

软件启动时找到一个系统里的 Python 解释器，脚本就在这个解释器里执行，因此能用哪些第三方包取决于这个环境里装了什么。解释器路径在设置对话框的 Python 页指定，页面上有 `Interpreter path` / 解释器路径 加 `Browse`、`Auto Detect`、`Test` 三个按钮，改完需要重启程序。

仓库根目录的 `requirements.txt` 列出一组常用包：`numpy`、`pandas`、`scipy`、`statsmodels`、`openpyxl`、`chardet`、`PyWavelets`、`pyarrow`、`matplotlib`、`seaborn`、`langgraph`、`langchain-openai`、`pydantic`、`tiktoken`、`loguru`、`typing_extensions`。这些包要装到上面那个解释器环境里，不随软件安装包分发。统计绘图、数据导入导出这些内置功能也依赖其中的 pandas、scipy、openpyxl、pyarrow。

脚本里 import 一个没装的包时，Python 抛出 `ModuleNotFoundError`，整段脚本停止执行，错误文本随执行结果回给调用方。补装时用设置里那个解释器执行 pip，下面的命令行把 pandas 装进这个解释器。路径按实际安装位置替换，引号只在路径含空格时需要。

```bash
"C:/Python312/python.exe" -m pip install pandas
```

用 `-m pip` 而不是直接调 `pip`，可以避免 PATH 上有多个 Python 时把包装进了另一个环境。装完之后脚本里直接 import，不需要重启软件；解释器仍提示找不到模块时，重启一次软件让解释器重新启动再试。

一次装齐清单里的常用包，把仓库根目录的 `requirements.txt` 传给 pip。

```bash
"C:/Python312/python.exe" -m pip install -r requirements.txt
```

`-r` 后面接清单文件路径，清单里每一行一个包名，部分带最低版本要求。软件自带的那些包由安装程序放进解释器环境，不用出现在这份清单里。装在别处的包可以加到设置对话框 Python 页的 `Extra module search paths (sys.path)` / 额外模块搜索路径 列表里，让解释器启动时就带上这些目录，这一项在重启后生效。

确认某个包在当前解释器里是否可用，在脚本里 import 它并打印版本，比在命令行里查更直接。

```python
import pandas as pd

print(pd.__version__)
```

打印出的是这个解释器里 pandas 的版本号；抛出 `ModuleNotFoundError` 表示这个环境里没有装。同一个包在系统里有多个版本时，看到的版本号就是脚本实际加载的那一份。

## matplotlib 的两条路线

脚本里可以 `import matplotlib`，也可以 `import da_pyplot`，两者画出的东西落在不同地方。

用 matplotlib 画图走的是离屏渲染：图保存在内存里的画布上，用 `savefig` 写成一个图片文件，软件里不会出现对应的绘图标签页，图上也没有交互。工作流的 `Data Plot` 节点就是这条路线，它用 matplotlib 的 Agg 后端出图，固定按 150 DPI 写成文件，输出的是图片路径，适合批量出图。

用 `da_pyplot` 画图时图元直接加进软件自己的坐标系，出现在绘图标签页里，能缩放、能用数据探针取值、能在属性面板里逐项调整，也能随工程保存。需要在软件里看、继续编辑的图用这条路线；一次要出几十张不需要交互的图，用 matplotlib 或 `Data Plot` 节点更直接。

## 常见问题

**变量在下一次运行里不见了。** 检查是不是换了智能体会话，命名空间按会话隔离；工作流节点和智能体工具也不共用同一份命名空间。同一个工具、同一个会话里连续两次执行，变量会保留。

**脚本运行后界面卡住不动。** 执行发生在界面线程上，运行期间窗口不响应输入，脚本越慢卡得越久。用 `Script execution timeout` / 脚本执行超时 限制上限，默认 300 秒；需要中途停下，只能停掉整轮智能体回复。

**print 的内容在「Log」/ 消息 面板里找不到。** 脚本的输出随执行结果回给调用方，智能体在工具调用卡片里展示这段文本，不写进消息面板；工作流节点里的 print 写进 Python 日志文件，位置见[常见故障](../reference/troubleshooting.md)。

**改了工作区目录，当前工程没有换目录。** 这一项在下次打开工程时生效，重新打开工程之后才会用新根目录。

**脚本里打不开工作区里的文件。** 脚本的工作目录是工作区根目录，用相对路径写的 `output/data.csv` 落在这个目录下；同时注意 `run_script` 只接受工作区以内的脚本路径，越出工作区的路径会被拒绝。

**脚本能导入工作区根目录下的模块，导入子目录里的模块却失败。** `sys.path` 里加的是工作区根目录，不是每个子目录。在脚本开头把子目录路径加进 `sys.path`，或者把要复用的模块放在工作区根目录。

**脚本里的写文件代码在别的机器上报路径错误。** 工作区根目录由本机设置和工程路径算出来，两台机器的目录不同。脚本里用相对路径，或者从当前位置推导路径，不要写死盘符。

**脚本超时中断之后，输出目录里留下了一个写了一半的文件。** 中断落在哪条语句不能预先确定，已经写出的内容留在磁盘上。检查输出目录，把不完整的产物删掉再重新运行整个脚本。

**同一个脚本在智能体里能读到数据集，放到工作流节点里就报错。** 两处用的命名空间各自独立，工作流节点也从输入端口接收上游节点送来的 DataFrame，不经数据管理接口取数。节点里的取数写法见[开发自定义节点](../workflow/custom-node.md)。

**执行结果只看到开头一段，后面是截断标记。** 回传文本受 `Script result max characters` / 脚本结果最大字符数 限制，初始 10000 个字符。提高这一项，或者让脚本把完整结果写成工作区里的文件，再从文件分次读取。

**重启软件后，脚本里定义的函数和变量没有了。** 命名空间只在本次运行期间存在，不随工程保存。把要复用的逻辑写成工作区里的脚本文件，把结果登记成数据集，见前文的「脚本结果与工程保存」。

**脚本报 Python 解释器相关的错误，界面上的数据分析按钮也没有反应。** 解释器没有配好，处理步骤见[常见故障](../reference/troubleshooting.md)的 Python 一节，配置位置见[设置项说明](../reference/settings.md)的 Python 页。

## 影响脚本的设置项

与脚本有关的设置分散在设置对话框的两页，汇总如下。

| 设置 | 页面 | 默认值 | 作用 |
|------|------|--------|------|
| `Interpreter path` / 解释器路径 | Python | 空，启动时自动探测 | 脚本用哪个解释器 |
| `Extra module search paths (sys.path)` / 额外模块搜索路径 | Python | 空列表 | 补充可导入模块的目录 |
| `Workspace directory` / 工作区目录 | 高级 | 空，用系统临时目录 | 本地工作区根目录 |
| `Script execution timeout` / 脚本执行超时 | 高级 | 300 秒 | 单次脚本的时限，0 为禁用 |
| `Script result max characters` / 脚本结果最大字符数 | 高级 | 10000 | 回传文本的截断长度 |

前两项在重启程序后生效，工作区目录在下次打开工程时生效，后两项对下一次运行生效。每一项的控件形式和取值范围的完整说明见[设置项说明](../reference/settings.md)。

`da_app` 还提供 `addInfoLogMessage`、`addWarningLogMessage`、`addCriticalLogMessage` 三个函数，分别按信息、警告、严重级别写进日志文件。脚本运行在后台时，把过程信息写进日志比只靠回传文本更容易事后核对，日志文件的位置见[常见故障](../reference/troubleshooting.md)。

## 脚本能操作的对象

把这一章里出现过的入口按对象归到一起，方便对照：

| 对象 | 从哪拿 | 能做什么 |
|------|--------|----------|
| 数据集 | `getAllDataframes()`、`getAllDatas()`、`findData(名字)` | 读改内容，加删数据集，通知界面刷新 |
| 绘图与坐标系 | `da_pyplot.gca()`、`da_figure.getCurrentChart()` | 加图元、改标题与坐标轴、换对数刻度 |
| 工作区文件 | 相对路径加内置的 `open`、`pandas.read_*` | 读写脚本和产物文件，随工程保存 |
| 日志 | `da_app` 的三个写日志函数 | 把过程信息记进日志文件 |

界面本身的排布不在这套接口里：停靠面板的开关、Ribbon 上的按钮、设置对话框里的项都在界面上操作，脚本面向的是数据和绘图。

## 相关章节

- [工程文件](../getting-started/project.md)：压缩包成员、工作区冲突的完整规则、设置存放位置。
- [内置工具](../agent/tools.md)：`run_code`、`run_script`、`write_file`、`save_chart_image` 的参数。
- [权限控制](../agent/permission.md)：代码执行一级的审批规则。
- [开发自定义节点](../workflow/custom-node.md)：节点里的 Python 怎么拿到模块、怎么收发数据。
- [数据分析节点手册](../workflow/nodes-analysis.md)：`Data Plot` 节点和其它节点的参数。
- [设置项说明](../reference/settings.md)：脚本超时、结果上限、工作区目录的默认值。
- [常见故障](../reference/troubleshooting.md)：脚本卡住、导入失败、Python 找不到。
