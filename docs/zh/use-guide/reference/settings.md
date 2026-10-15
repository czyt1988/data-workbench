# 设置项说明

设置对话框里的每一项都影响软件的行为或外观。这一章按对话框里页面的顺序，把六个页面上的每一项列成表：标签、控件形式、取值范围或选项、初始值和改动的作用。默认值取自程序的初始配置，改动生效的时机（立即、重启后、下次打开工程）写在每张表的说明里。

表里的「作用」一列只写这一项的后果，控件怎么用、取值范围为什么这样定写在表格前后的正文里。同一项在多个入口都能改时，正文里会点明，比如主题在通用页和视图标签页各有一个入口，改的是同一项。

## 打开设置对话框

入口在 Ribbon 主页的「Config」/ 配置 面板：点 `Setting` / 设置 打开对话框，对话框标题是 `Setting` / 设置，左侧是页面列表，右侧是当前页。页面顺序是 General / 通用、Python、Log / 日志、Advanced / 高级、Agent LLM Settings / Agent LLM 设置、Agent Permission Settings / Agent 权限设置。插件可以提供追加的页面，当前随软件分发的插件都没有提供。

对话框底部三个按钮的行为如下。

| 按钮 | 行为 |
|------|------|
| `Apply` / 应用 | 把改动过的页面写入配置并立即应用，对话框不关闭 |
| `OK` / 确定 | 先做应用，再关闭对话框 |
| `Cancel` / 取消 | 直接关闭，不写入任何改动 |

只有被改动过的页面会被应用，改动会写进配置文件，下次启动时读回。页面左侧的列表显示各页的图标和名称，点击切换；应用前后的当前页不重置。另有一个名字相近的入口：视图标签页里的 `Show Setting Window` 开关的是右侧属性面板，与这里无关。

设置对话框是应用级的：打开的工程不受对话框开关的影响，改的是软件本身的行为。点 `Apply` / 应用 之后窗口不关闭，可以继续切到别的页面接着改。

对话框上没有搜索框，也没有一键恢复默认值的按钮。找某一项时先按页面分组定位，六页的职责是：通用管外观与语言，Python 管解释器，日志管记录，高级管行为、路径与脚本限额，两个智能体页管模型与权限。要恢复某一项的默认值，按本章表格里列的初始值填回去。

## General / 通用

这个页面分外观和语言两块，负责 Ribbon 样式、主题、内层标签页方位、字体和界面语言。

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| Ribbon 样式 | 四个带预览图的单选按钮 | `Standard Mode`、`Standard Mode With 2 Row`、`Compact Mode`、`Minimalist mode` | `Minimalist mode` | 换 Ribbon 的行数和高度，立即生效 |
| `Ribbon theme` / Ribbon 主题 | 下拉框 | `Windows 7`、`Office 2013`、`Office 2016 Blue`、`Office 2016 Green`、`Office 2016 Dark`、`Office 2021 Blue`、`Office 2021 Green`、`Office 2021 Dark`、`Dark`、`Dark 2` | 不套用列表里的主题，保持框架默认外观 | 换 Ribbon 配色，立即生效并写入配置 |
| `Operation window tab position` / 操作窗口标签页位置 | 下拉框 | `Top` / 上方、`Bottom` / 下方 | `Bottom` / 下方 | 换工作流、绘图、数据三个操作区内部标签页的位置，立即生效 |
| `Application font` / 应用字体 | 字体下拉框 | 系统已安装的字体 | 空，跟随系统默认字体 | 换界面字体 |
| `Size` / 字号 | 数字框 | 6 到 72 | 当前系统默认字号 | 换界面字号 |
| `Is the geometry and status of the program interface recorded` / 是否记录程序界面几何和状态 | 复选框 | 勾选或不勾选 | 不勾选 | 勾选后关闭软件时把窗口几何、停靠布局写入状态文件；不勾选时在关闭时删除这个文件 |
| `Clear the saved window state` / 清除已保存的窗口状态 | 按钮 | 弹确认框 | 无 | 删掉界面状态文件；删除成功后在「Log」/ 消息 面板里记一条 |
| `Interface language` / 界面语言 | 下拉框 | `System` / 跟随系统，加从翻译文件搜索目录得到的语言代码 | `System` | 换界面语言 |

页面底部注明语言和字体改动在重启程序后生效。字体和语言在启动阶段读取，改完要重启；Ribbon 样式、主题、标签页方位和窗口状态开关在点应用时立即生效。

Ribbon 样式四种模式的差别是功能组占三行还是两行、按钮区是宽松还是紧凑，选中时右侧的预览图跟着换。样式和主题都只影响外观，不改功能位置之外的内容。

`Ribbon theme` 的初始配置值是 −1，表示不覆盖框架的默认主题，此时界面用框架自带外观。在视图标签页的「Appearance」/ 外观 面板里有同一个主题下拉框，两处改的是同一项，改完立即生效并写进配置。

窗口状态那一项需要理解两件事：勾选时关闭软件会把窗口几何和停靠布局写进状态文件，下次启动恢复；不勾选时关闭软件会删除已经存在的状态文件，下次启动用默认布局。想回到默认布局又不想改开关，用「Clear the saved window state」/ 清除已保存的窗口状态 按钮，确认后删掉文件即可。

界面语言下拉框里除了 `System` / 跟随系统，其余条目由程序从翻译文件的搜索目录推导出来，搜索目录是可执行文件目录下的 `translations`、`translations_user`，以及可执行文件上一级的 `translations`。随软件发布的翻译文件是 `da_zh_CN` 和 `da_en_US` 两份。下拉框里没有需要的语言时，直接改 `dawork-config.xml` 的 `language` 键（取值是 `zh_CN`、`en_US` 这样的语言代码）是另一条途径，两种改法都在重启程序后生效。

这一页改错的典型表现是外观与预期不符：字体调大后对话框里的控件显得拥挤，重启后布局按新的字号重排；语言改成英文后仍有少量文字保持中文，说明这些文字没有对应的翻译条目，属于正常情况；窗口状态记录关掉后每次启动都回到默认布局，这是关闭时删除了状态文件的结果。

## Python

这个页面管脚本和工作流节点用的解释器，以及额外的模块搜索路径。

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| `Interpreter path` / 解释器路径 | 单行输入框加 `Browse`、`Auto Detect`、`Test` 三个按钮 | 一个可执行文件路径 | 空，启动时自动探测 | 指定脚本和节点使用的解释器 |
| `Extra module search paths (sys.path)` / 额外模块搜索路径 | 列表加 `Add`、`Remove` | 若干目录 | 空列表 | 解释器启动时把这些目录加进模块搜索路径 |

`Browse` 打开文件选择框挑解释器；`Auto Detect` 在系统 PATH 里查找，找不到时提示 `No Python interpreter found in system PATH`；`Test` 用所选解释器执行一次版本查询，结果直接显示在按钮下方：路径为空时提示 `Please specify a Python interpreter path`，文件不存在时提示 `File does not exist: %1`，启动失败时提示 `Failed to run: %1`，成功时显示解释器输出的版本文字。

解释器为空时的探测顺序是：读 `python-config.json`，再到系统 PATH 里找。点应用保存这一页时，路径同时写进配置文件和可执行文件目录下的 `python-config.json`，下次启动由后者提供。页面底部注明解释器和模块路径改动在重启程序后生效，两项都在启动阶段读取。

这个解释器承担的功能范围很大：导入导出数据、工作流里的 Python 节点、数据分析插件的清洗与统计按钮、智能体的全部工具、脚本工作区里的脚本，都跑在这个解释器里。解释器不可用时这些功能一起失效，界面本身还能打开。解释器对应的环境里需要装有 pandas、numpy 这些依赖包，装包方法见[脚本工作区](../script/workspace.md)的第三方包一节。

`Auto Detect` 在系统 PATH 里找到多个入口时取第一个。同时装了多个 Python 版本的机器上，探测结果不一定是要用的那个，装完软件之后用 `Test` 确认一次输出的版本号。

额外模块搜索路径用于把包装在非标准位置的情况：目录加进列表并重启后，脚本和节点就能 import 里面的模块，不必把包复制进解释器的 site-packages。

这一页改错的典型表现是 Python 相关的功能整体失效：导入数据报错、数据分析面板的按钮点下去没有结果、工作流节点执行失败、智能体的工具全部不可用，而界面本身正常。遇到这类表现先到这一页点一次 `Test`，再按[常见故障](../reference/troubleshooting.md)的 Python 一节处理。

## Log / 日志

这个页面控制日志写到哪里、写多细、保留多少，以及消息面板里显示多少条。

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| `File log level` / 文件日志级别 | 下拉框 | `Trace`、`Debug`、`Info`、`Warning`、`Error`、`Critical`、`Off` | `Trace` | 写进日志文件的最低级别，比它低的记录被丢掉 |
| `UI queue level` / 界面队列级别 | 下拉框 | 同上 | `Info` | 进入「Log」/ 消息 面板的最低级别 |
| `Output log to stdout` / 输出到标准输出 | 复选框 | 勾选或不勾选 | 勾选 | 是否把日志同时写到控制台 |
| `Rotation mode` / 轮转方式 | 下拉框 | `Rotating` / 按大小轮转、`Daily` / 按日期分割、`Console only` / 仅控制台 | `Rotating` / 按大小轮转 | 日志文件的分割方式 |
| `Max size per file` / 单文件最大体积 | 数字框 | 1 到 1024，单位 MB | 10 | 单个日志文件的上限，超过后开始新的一个 |
| `Max files to keep` / 保留文件数 | 数字框 | 1 到 100 | 5 | 轮转后保留的历史文件个数 |
| `Display number of logs` / 显示日志条数 | 数字框 | 10 到 99999，步长 1000 | 5000 | 消息面板里保留并显示的日志条数 |

页面底部注明文件级别、输出到标准输出和轮转方式在重启程序后生效。轮转方式、单文件大小和保留文件数在启动阶段读取并创建日志文件，改完要重启；界面队列级别和显示条数在点应用时立即调整。

七个级别从低到高是 `Trace`、`Debug`、`Info`、`Warning`、`Error`、`Critical`、`Off`，设置成某一级表示这一级及更高的记录才写出。文件级别和界面级别各管一处：文件级别管磁盘上的日志文件，界面级别只管「Log」/ 消息 面板。默认把文件级别放得最低，文件里能看到详细过程，面板里只显示信息以上，避免界面被刷屏。

`Console only` 不出日志文件，适合临时排查；`Daily` 每天生成一个新文件，保留个数同样受 `Max files to keep` 限制。日志文件写在配置目录旁边的 `log` 子目录里，文件名和读取方法见[常见故障](../reference/troubleshooting.md)。`Off` 会连错误记录一起丢掉，只在确定不需要日志时使用。

`Display number of logs` / 显示日志条数 控制消息面板里保留的条数，超过后最旧的记录被丢掉。它只影响面板显示，日志文件里的内容不受影响。

这一页改错的典型表现是排查时看不到证据：消息面板里只有提示性文字，没有开发诊断内容，通常是文件级别调到了 `Warning` 以上；日志文件长时间没有变化，检查轮转方式是不是 `Console only`；面板里的记录很快被冲掉，把显示条数调大。

## Advanced / 高级

这个页面分行为、插件路径、节点脚本路径和脚本工作区四块。

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| `Workflow execution timeout` / 工作流执行超时 | 小数数字框 | −1 到 86400，单位秒，−1 显示为 `Unlimited` / 无限 | −1 | 保存进配置；当前版本的程序没有读取它，运行工作流不会因此中断 |
| `Max recent files` / 最近文件数上限 | 数字框 | 1 到 100 | 10 | 应用菜单里「Recent Files」/ 最近文件 列表最多保留几条 |
| `Crash dump retention days` / 崩溃转储保留天数 | 数字框 | 1 到 365，单位天 | 7 | 点应用时清理超过这个天数的崩溃转储文件 |
| `Auto-save interval` / 自动保存间隔 | 数字框 | 0 到 1440，单位分钟，0 显示为 `Disabled` / 禁用 | 0，不自动保存 | 按间隔自动保存有改动的工程 |
| `Show splash screen on startup` / 启动时显示启动画面 | 复选框 | 勾选或不勾选 | 勾选 | 启动时先显示启动画面 |
| `Plugin search path` / 插件搜索路径 | 单行输入框加 `Browse` | 一个目录 | 空 | 保存进配置；当前版本没有读取它，插件仍从固定目录加载 |
| `Node script search paths` / 节点脚本搜索路径 | 列表加 `Add`、`Remove` | 若干目录 | 空列表 | 保存进配置；当前版本没有读取它，节点仍从固定目录发现 |
| `Workspace directory` / 工作区目录 | 单行输入框加 `Browse`，占位文字 `Empty for system temporary directory` | 一个目录 | 空，用系统临时目录 | 脚本工作区本地目录的根目录 |
| `Script execution timeout` / 脚本执行超时 | 数字框 | 0 到 3600，单位秒，0 显示为 `Disabled` / 禁用 | 300 | 单次脚本执行的时限 |
| `Script result max characters` / 脚本结果最大字符数 | 数字框 | 1000 到 100000 | 10000 | 脚本返回值回传文本的截断长度 |

页面底部注明插件路径、节点脚本路径和启动画面的改动在重启后生效，工作区目录在下次打开工程时生效。启动画面在启动阶段读取；脚本超时和结果长度在点应用时立即设置给执行引擎；自动保存间隔在点应用时重建定时器；崩溃转储清理在点应用时立即执行一次。

行为一块里几项的后果比较直接：`Max recent files` / 最近文件数上限 改小之后应用菜单里的历史列表立即裁剪到新条数；`Crash dump retention days` / 崩溃转储保留天数 在点应用时清理一次超龄文件，转储目录的位置见[常见故障](../reference/troubleshooting.md)；`Auto-save interval` / 自动保存间隔 大于 0 时软件按间隔保存有改动的工程，工程还没保存过时会先弹出保存对话框，行为细节见[工程文件](../getting-started/project.md)；`Show splash screen on startup` / 启动时显示启动画面 关掉后启动阶段直接进主窗口。

工作区一块的两项只管脚本，不影响工作流节点的运行。`Script execution timeout` / 脚本执行超时 填 0 表示不限时，脚本卡住时只能停整轮智能体回复；`Script result max characters` / 脚本结果最大字符数 只截断回传文本，不改脚本里的数据。两项的作用见[脚本工作区](../script/workspace.md)。

工作流执行超时、插件搜索路径、节点脚本搜索路径三项写进配置但不改变程序行为，界面上没有任何提示说明这一点，按实际表现理解即可。工作区目录的细节见[脚本工作区](../script/workspace.md)，插件目录的细节见[插件管理](../plugin/manager.md)。

## Agent LLM Settings / Agent LLM 设置

这个页面分两个标签页：模型供应商和运行参数。各项的含义、拉取模型的操作、密钥的保存方式在[配置模型](../agent/model-config.md)里说明，这里只列控件和默认值。

### Model Providers / 模型供应商

列表只用于选中，增删改都在弹出的对话框里完成。按钮是 `Add Provider`、`Edit Provider`、`Remove Provider`。

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| `Name` | 单行输入框 | 非空且不重复 | 空 | 供应商标识名 |
| `Base URL` | 单行输入框 | 非空 | 空，占位文字 `https://api.openai.com/v1` | 接口地址前缀 |
| `API Key` | 密码输入框 | 任意文本 | 空，详情区显示 `not set` 或 `set (hidden)` | 访问密钥，保存时加密 |
| `Model Name` | 模型表格中的一列 | 任意文本 | 空 | 模型标识 |
| `Context Size` | 模型表格中的一列 | 1024 到 2097152，步长 1024，单位 tokens | 262144 | 上下文窗口大小 |
| `Max Output Tokens` | 模型表格中的一列 | 1 到 1048576，步长 128 | 8192 | 单次回复的输出上限 |

供应商对话框里的 `Fetch Available Models` 按 `Base URL` 和密钥拉取模型列表，选中的模型补上默认的上下文大小 262144 和输出上限 8192。双击模型行可以编辑该行。

这一块的界面是三段式：左边的列表框只用来看和选，新增、修改、删除都要进对话框；右侧上半是所选供应商的三个字段，下半是它的模型表格。密钥在详情区只显示 `not set` 或 `set (hidden)` 两种状态，不回显内容；想换密钥就重新输入一次。`Fetch Available Models` 的状态文字会经历 `Fetching...`、`✓ %1 models fetched` 或失败提示，拉到列表后默认全选。

### Agent Settings / Agent 设置

运行参数在同一页的第二个标签页里。

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| `Ready Timeout` / 就绪超时 | 数字框 | 5 到 300 秒 | 60 | 等待智能体进程就绪的上限 |
| `Stop Timeout` / 停止超时 | 数字框 | 1 到 60 秒 | 5 | 等待停止完成的上限 |
| `Compaction Threshold` / 压缩阈值 | 小数数字框 | 0.50 到 1.0，步长 0.05 | 0.85 | 上下文用到多少比例时压缩历史 |
| `Max Recent Messages` / 保留最近消息数 | 数字框 | 4 到 50 | 10 | 压缩时原样保留的最近消息条数 |
| `Tool Result Max Chars` / 工具结果截断阈值 | 数字框 | 1000 到 500000 | 20000 | 工具结果超过后截断 |
| `Tool Result Preview Chars` / 工具结果预览长度 | 数字框 | 100 到 10000 | 2000 | 卡片里预览的工具结果长度 |
| `Max Sessions` / 最大会话数 | 数字框 | 5 到 200 | 20 | 保留的会话数上限 |
| `Session Retention Days` / 会话保留天数 | 数字框 | 1 到 365 天 | 30 | 超过天数的旧会话被清理 |
| `Max retries` / 最大重试次数 | 数字框 | 0 到 20 | 5 | 模型调用失败后的重试次数 |
| `Retry interval` / 重试间隔 | 数字框 | 0 到 300 秒 | 5 | 第一次重试前的等待 |
| `Retry interval increment` / 重试间隔递增 | 数字框 | 0 到 60 秒 | 1 | 每次重试等待时间的增量 |
| `Request timeout` / 请求超时 | 数字框 | 10 到 600 秒 | 120 | 单次模型调用的上限 |
| `Inactivity timeout` / 无活动超时 | 数字框 | 60 到 600 秒 | 240 | 长时间没有输出时判定卡住的上限 |
| `Max process restarts` / 最大进程重启次数 | 数字框 | 0 到 10 | 3 | 智能体进程崩溃后的重启次数 |
| `Reasoning iteration limit` / 推理迭代上限 | 数字框加 `No limit` 复选框 | −1 到 1000000 | 复选框勾选，表示不限 | 一轮回复里模型推理迭代的次数上限 |
| `Subagent Timeout` / 子 Agent 超时 | 数字框 | 60 到 3600 秒 | 600 | 单个子智能体任务的时限 |
| `Subagent Reasoning Limit` / 子 Agent 推理上限 | 数字框 | −1 到 1000000，−1 表示不限 | 60 | 子智能体的推理迭代上限 |
| `Auto prestart on launch` / 启动时自动预热 | 复选框 | 勾选或不勾选 | 勾选 | 启动后提前把智能体进程准备好 |

模型供应商、密钥和这些参数保存在 `agent-config.json` 里，保存后立即对下一次对话生效。

这一页没有温度、top-p、采样惩罚这类生成参数，模型连接相关的可调项就是供应商的地址、密钥、模型标识、上下文窗口和输出上限。模型供应商和模型可以在会话状态栏上随时切换，切换方法见[会话与对话](../agent/session.md)。子智能体的并发数与批量上限存在配置文件里，设置页上没有对应的输入项，需要调整时直接改配置并重启。

## Agent Permission Settings / Agent 权限设置

这个页面管审批模式、判官模型、路径规则和工具分级。三种模式的区别、审批卡片的处理、规则的匹配顺序在[权限控制](../agent/permission.md)里说明，这里只列控件和默认值。

### Default Permission Mode / 默认权限模式

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| 权限模式 | 下拉框 | `Auto (rules + judge)`、`Manual (ask every write/code)`、`Full Auto (yolo)` | `Full Auto (yolo)` | 决定哪些工具调用需要审批 |

### Approval / 审批

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| `Approval timeout:` 审批超时 | 数字框 | 10 到 86400 秒 | 600 | 审批卡片等待回答的上限 |
| `In manual mode, also ask before in-app chart edits` / 在手动模式下，应用内图表编辑也要询问 | 复选框 | 勾选或不勾选 | 不勾选 | 手动模式下把应用内图表修改也纳入询问范围 |

### Code Judge (optional) / 代码判官（可选）

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| `Judge model:` 判官模型 | 单行输入框 | 任意模型标识，占位文字 `Leave empty to disable the judge` | 空，不启用判官 | 自动模式下用哪个模型判定代码风险 |
| `Judge timeout:` 判官超时 | 数字框 | 1 到 600 秒 | 30 | 单次判定的上限 |

页面说明里写明：没有配置判官时，自动模式对每次代码执行都会询问。

### File Path Rules / 文件路径规则

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| 规则表 | 三列表格 `Tool`、`Scope (glob)`、`Action` | `Action` 可选 allow、deny、ask | 表里有一条锁定的全局拒绝规则 | 按工具名和路径通配符决定放行、拒绝或询问 |
| `Add Rule`、`Remove Selected` | 按钮 | 无 | 无 | 增删规则行 |

说明里列出可以在规则里使用的变量：`${workspace}`、`${project}`、`${data}`、`${exe}`、`${home}`。匹配规则是第一条命中的行生效。工具填 `*`、动作填 deny 的那一行是全局硬性拒绝，不可编辑也不可删除。

### Code Danger Patterns (Auto mode) / 代码危险模式（自动模式）

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| `Deny patterns (matched => reject):` | 多行文本 | 若干正则，一行一条 | 内置一组 | 命中即拒绝的代码模式 |
| `Escalate patterns (matched => consult judge):` | 多行文本 | 若干正则，一行一条 | 内置一组 | 命中后交给判官判定的代码模式 |

说明里带一句免责声明，并指出需要严格管控时应改用每次询问模式。

### Tool Tier Overrides / 工具分级覆盖

| 设置项 | 控件 | 取值范围或选项 | 默认值 | 作用 |
|--------|------|----------------|--------|------|
| 覆盖表 | 两列表格 `Tool`、`Tier` | `Tier` 可选 read、inapp_mutate、file_write、code_exec、unknown | 空表 | 给某个工具指定风险级别 |
| `Add Override`、`Remove Selected` | 按钮 | 无 | 无 | 增删覆盖行 |

未在内置分级表里的插件工具默认是 unknown，在自动和每次询问模式下都会询问；需要例外时在这里加一行。

这一页的改动保存后立即下发给正在运行的智能体，不需要重启。标量存在 `agent-config.json` 的 permission 分组里，模式、规则和覆盖存在 `agent-permissions.json` 里。

模式的选择大致对应三种用法：全自动适合信任来源的数据，代码和写文件都不打断；自动模式适合日常使用，读操作和界面内的图表修改直接放行，代码执行交给判官或每次都问；每次询问适合处理来源不明或要严格留痕的数据。改动模式之后已经在等待回答的审批卡片不受影响，按旧模式处理完即可。

页面自上而下是默认权限模式、审批、代码判官、文件路径规则、代码危险模式、工具分级覆盖六个分组。规则表和覆盖表都靠分组下方的按钮增删行：`Add Rule`、`Remove Selected`、`Add Override`、`Remove Selected`，行的内容在表格单元格里直接编辑。硬性拒绝行在界面上是锁定的，编辑和删除都会被拒绝，这是内置的安全边界。

规则和覆盖在改完保存后对新的工具调用立即生效，对话不需要重开；危险模式列表里的正则在判官或拒绝判定时逐条匹配，改动同样立即生效。

## 其它与设置相邻的入口

有一批「看起来像设置」的控件不在这个对话框里，改动同样会持久化，容易找错地方。

| 入口 | 位置 | 改什么 |
|------|------|--------|
| 主题下拉框 | 视图标签页的「Appearance」/ 外观 面板 | 与通用页的 `Ribbon theme` 是同一项 |
| `Manage Layouts`、`Reset Layout` | 视图标签页的「Layout」/ 布局 面板 | 界面布局方案的保存、应用、删除与重置 |
| `Plugin Config` / 插件设置 | 主页「Config」/ 配置 面板 | 打开插件管理器，改插件的启用状态 |
| 模型选择器与权限模式选择器 | 智能体会话的状态栏 | 切换当前使用的模型与权限模式，改动写进 `agent-config.json` 并对所有会话生效 |
| `Apply All Charts` / 应用到所有绘图 | 绘图相关的 Ribbon 面板 | 决定设置面板的改动作用在当前绘图还是全部绘图 |

## 改动生效时机一览

同一页上的项目生效时间不同，下表按页面汇总，方便对照。

| 页面 | 设置项 | 生效时机 |
|------|--------|----------|
| 通用 | Ribbon 样式 | 立即 |
| 通用 | `Ribbon theme` / Ribbon 主题 | 立即并写入配置 |
| 通用 | `Operation window tab position` / 操作窗口标签页位置 | 立即 |
| 通用 | `Application font` / 应用字体、`Size` / 字号 | 重启程序 |
| 通用 | `Is the geometry and status ... recorded` / 是否记录界面几何和状态 | 关闭时写文件，下次启动恢复 |
| 通用 | `Clear the saved window state` / 清除已保存的窗口状态 | 立即删除文件 |
| 通用 | `Interface language` / 界面语言 | 重启程序 |
| Python | `Interpreter path` / 解释器路径 | 重启程序 |
| Python | `Extra module search paths (sys.path)` / 额外模块搜索路径 | 重启程序 |
| 日志 | `File log level` / 文件日志级别 | 立即设置，重启后按配置重新读取 |
| 日志 | `UI queue level` / 界面队列级别 | 立即 |
| 日志 | `Output log to stdout` / 输出到标准输出 | 重启程序 |
| 日志 | `Rotation mode` / 轮转方式 | 重启程序 |
| 日志 | `Max size per file` / 单文件最大体积 | 重启程序 |
| 日志 | `Max files to keep` / 保留文件数 | 重启程序 |
| 日志 | `Display number of logs` / 显示日志条数 | 立即 |
| 高级 | `Workflow execution timeout` / 工作流执行超时 | 无作用，程序没有读取这一项 |
| 高级 | `Max recent files` / 最近文件数上限 | 立即 |
| 高级 | `Crash dump retention days` / 崩溃转储保留天数 | 立即清理一次 |
| 高级 | `Auto-save interval` / 自动保存间隔 | 立即重建定时器 |
| 高级 | `Show splash screen on startup` / 启动时显示启动画面 | 重启程序 |
| 高级 | `Plugin search path` / 插件搜索路径 | 无作用，程序没有读取这一项 |
| 高级 | `Node script search paths` / 节点脚本搜索路径 | 无作用，程序没有读取这一项 |
| 高级 | `Workspace directory` / 工作区目录 | 下次打开工程 |
| 高级 | `Script execution timeout` / 脚本执行超时 | 立即，下一次脚本运行起 |
| 高级 | `Script result max characters` / 脚本结果最大字符数 | 立即，下一次脚本运行起 |
| Agent LLM 设置 | 供应商、模型、运行参数 | 保存后写入配置文件，对之后开始的会话生效 |
| Agent 权限设置 | 模式、审批、判官、规则、覆盖 | 保存后立即下发给正在运行的智能体 |

## 常见问题

**改完设置没有任何变化。** 先确认点的是 `Apply` / 应用 或 `OK` / 确定，`Cancel` / 取消 不写入任何改动。再对照上表确认这一项属于哪一类：标着重启的要重启软件，工作区目录要重新打开工程，标着无作用的三项本身不影响行为。

**换了一台机器，模型配置里的密钥失效。** 密钥用 Windows 数据保护接口按当前用户加密保存，配置文件拷到别的机器或别的 Windows 账户下无法解密，需要在那一台重新填写。模型名称和地址可以照抄。

**配置文件损坏后程序启动异常。** 日志和消息面板里会出现 `Cannot load config file` 或 `Cannot open config file` 加文件路径。关闭软件，把 `dawork-config.xml` 改名备份，重新启动让程序生成一份初始配置，再按需要逐项设置。

**设置界面的主题换了，Ribbon 上的图标没有变。** 主题只换 Ribbon 的配色，界面图标是随软件发布的固定资源，不随主题变化。

**点了清除窗口状态，重启后布局还是老样子。** 检查「Is the geometry and status ... recorded」是否处于勾选状态：勾选时关闭窗口会立刻写一份新的状态文件，把刚刚清掉的记录补回来。要回到默认布局，取消勾选再关闭软件。

**自动保存没有生效。** 确认间隔不是 0（界面显示 `Disabled` / 禁用）；间隔大于 0 时还要注意工程是否保存过，工程没有保存过时自动保存会先弹出保存对话框，行为见[工程文件](../getting-started/project.md)。

**崩溃转储保留天数改了，旧文件还在。** 这项清理只在 Windows 的 MSVC 构建里执行，而且只在点应用时跑一次，不按天定时；其它平台的构建不清理，手工删除转储目录里的文件即可，目录位置见[常见故障](../reference/troubleshooting.md)。

**`Workflow execution timeout` 填了数字，工作流还是跑很久不被打断。** 这一项在当前版本里没有被程序读取，填任何值都不改变运行行为，需要限制工作流时长时用脚本超时或节点内部的检查。

**在配置文件里手改了某一项，打开设置对话框还是旧值。** 设置对话框只在第一次打开时按配置文件构建页面，之后一直复用这一份；手改配置文件后要重启程序才会读回新值，正常途径是在对话框里改。

**`Reasoning iteration limit` / 推理迭代上限 的数字框是灰的。** `No limit` 复选框勾选时数字框被禁用，表示不限制迭代次数；取消勾选后才能填数值。

**消息面板被关掉之后找不到日志。** 到视图标签页的「Display」/ 视图显示 面板里点 `Show Information Window` / 信息窗口，把这个停靠面板找回来。

**Ribbon 主题列表里没有想要的配色。** 主题是固定的十项，没有自定义颜色的入口，挑一个接近的使用。

**同一个工程在两台机器上打开的界面不一样。** 界面布局、主题、字体属于应用级设置，两台机器各自的配置不同；工程里带的是工作流、数据、绘图这些内容，见前一节。

**想要某个工程用一套单独的日志级别或超时。** 设置对话框里的项不能按工程区分，全部是应用级；需要按工程区分的参数写进工作流节点，或者写成脚本参数，那两部分随工程保存。

## 设置存在哪里

Windows 上应用设置放在用户 AppData 目录下的 `DAWorkBench\config` 里，路径是 `%APPDATA%\DAWorkBench\config`。Linux 与 macOS 上传在各自的应用程序数据目录下，写法见[安装与首次启动](../getting-started/install.md)。这个目录只保存设置和状态，删掉它不影响安装目录里的插件与节点包。

| 文件 | 内容 |
|------|------|
| `dawork-config.xml` | 通用、Python、日志、高级四页的应用设置，加一个没有界面的工作区覆盖策略 `workspace-overwrite-policy` |
| `dawork-layouts.json` | 界面布局方案，见[主界面](../getting-started/main-window.md)的布局一节 |
| `.dawork-ui-state` | 关闭时保存的窗口几何、停靠布局快照 |
| `recent-files.ini` | 最近打开过的工程文件列表 |
| `agent-config.json` | 模型供应商、运行参数、权限标量；密钥加密后以文本保存 |
| `agent-permissions.json` | 权限模式、路径规则、代码危险模式、工具分级覆盖 |

相邻的几个目录也在这里：日志目录 `log` 下有 `da_log.log` 和 Python 侧的 `da_pyscript.log`，崩溃转储目录 `dumps`，会话存储目录 `sessions`。可执行文件目录下另有一个 `python-config.json`，保存解释器路径的副本，Python 页点应用时会同步写入。

`dawork-config.xml` 由各个设置页在点应用时写入，文件结构是一个 `<configs>` 根节点加若干键值项，带一个版本号用于升级时的兼容处理。文件不存在时启动会生成一份带全部初始值的配置；文件损坏时启动日志里出现 `Cannot load config file` 或 `Cannot open config file`，程序按内置初始值继续运行。

`dawork-layouts.json` 存的是界面布局方案，内置的 `Default` 与 `Focus Analysis` 之外的自定义方案都在这里；`.dawork-ui-state` 只有勾选了界面状态记录才会存在，随时可以删除。`recent-files.ini` 是标准 ini 格式，内容就是路径列表加最大条数。

`agent-config.json` 里保存供应商列表、每个供应商的模型清单、运行参数和权限相关的标量，密钥字段存的是加密后的文本；`agent-permissions.json` 单独保存权限模式、路径规则、代码危险模式和工具分级覆盖。两个文件都由智能体模块直接读写，手工编辑要在软件关闭时进行。

把整个配置目录拷到另一台机器上可以带走大部分偏好：主题、字体、布局方案、日志参数都随文件过去，路径类设置要按新机器的目录改，模型密钥不能跨机器使用。想恢复出厂状态，关闭软件后把配置目录改名或删除，下次启动会重新生成。

旧版本的设置存在系统注册表里，升级后的第一次启动会把智能体相关的键迁到 `agent-config.ini`、把最近文件迁到 `recent-files.ini`；`agent-config.ini` 在新版第一次加载时迁移成 `agent-config.json`，原文件改名加 `.bak` 保留。迁移只在第一次运行新版时发生，之后注册表里不再读写这些内容。

手工编辑 `dawork-config.xml` 时按下面的键名定位，改完保存文件并重启程序。编辑前关闭软件，运行期间程序会按当前状态回写这个文件。

| 页面 | 设置项 | 键名 |
|------|--------|------|
| 通用 | Ribbon 样式 | `ribbon-style` |
| 通用 | `Ribbon theme` / Ribbon 主题 | `ribbon-theme` |
| 通用 | `Operation window tab position` / 操作窗口标签页位置 | `dock-tab-position` |
| 通用 | `Application font` / 应用字体 | `app-font-family` |
| 通用 | `Size` / 字号 | `app-font-point-size` |
| 通用 | `Interface language` / 界面语言 | `language` |
| 通用 | 是否记录界面几何和状态 | `save-ui-state-on-close` |
| Python | `Interpreter path` / 解释器路径 | `python-interpreter-path` |
| Python | `Extra module search paths (sys.path)` / 额外模块搜索路径 | `python-extra-paths` |
| 日志 | `File log level` / 文件日志级别 | `log-level` |
| 日志 | `UI queue level` / 界面队列级别 | `log-queue-level` |
| 日志 | 输出到标准输出 | `log-output-stdout` |
| 日志 | `Rotation mode` / 轮转方式 | `log-rotation-mode` |
| 日志 | `Max size per file` / 单文件最大体积 | `log-max-size` |
| 日志 | `Max files to keep` / 保留文件数 | `log-max-files` |
| 日志 | `Display number of logs` / 显示日志条数 | `show-log-num` |
| 高级 | `Workflow execution timeout` / 工作流执行超时 | `workflow-timeout` |
| 高级 | `Max recent files` / 最近文件数上限 | `recent-files-max` |
| 高级 | `Crash dump retention days` / 崩溃转储保留天数 | `dump-retention-days` |
| 高级 | 自动保存间隔 | `autosave-interval` |
| 高级 | 启动时显示启动画面 | `show-splash` |
| 高级 | `Plugin search path` / 插件搜索路径 | `plugin-extra-paths` |
| 高级 | `Node script search paths` / 节点脚本搜索路径 | `node-script-paths` |
| 高级 | `Workspace directory` / 工作区目录 | `workspace-dir` |
| 高级 | 脚本执行超时 | `script-timeout` |
| 高级 | 脚本结果最大字符数 | `script-result-max-chars` |
| 无界面 | 工作区覆盖策略 | `workspace-overwrite-policy` |

同一个文件里还有一项没有界面入口的策略键 `workspace-overwrite-policy`，取值 `ask`、`always-local`、`always-zip`，默认 `ask`，控制打开工程时本地脚本工作区与包内版本不一致时的处理方式，改完需要重启程序，三个取值的后果见[工程文件](../getting-started/project.md)。

## 哪些设置跟着工程走

设置对话框里的每一项都是应用级的，存在配置目录里，对这台机器上的所有工程生效。想把某个工程的计算参数单独带走，用工作流节点的参数，那部分存在工程里。

工程级的「设置」不在这个对话框里，它们随 `.dapro` 保存，打开工程时恢复：

| 内容 | 存在工程的哪个成员 |
|------|--------------------|
| 工作流、节点参数、连接关系 | `workflow.xml` 与 `workflow-data.xml` |
| 数据集 | `data-manager.xml` 与 `datas/` |
| 绘图与图表 | `charts.xml` 与 `chart-data/` |
| 表格样式 | `table-styles.xml` |
| 数据操作区打开过哪些页 | `data-operate.xml` |
| 智能体会话记录 | `agent_sessions/` |
| 脚本工作区文件 | `workspace/` |

工程成员清单和打开时恢复的内容见[工程文件](../getting-started/project.md)。把工程拷到另一台机器上时，工程里的内容跟着走，界面偏好、模型配置、权限规则这些应用级设置需要在那边重新设一遍；模型供应商的密钥在本机加密保存，换机器后要重新填写。

判断某一项属于哪一类的方法很简单：改动会影响所有工程、只跟这台机器绑定的项在设置对话框或配置目录里；改动只影响这一个工程、随文件搬家的内容在 `.dapro` 里。拿不准时看这项是「软件的偏好」还是「这次工作的一部分」。

反过来，下面这些内容不随工程走，换机器要重新准备：解释器路径与额外模块路径、模型供应商与运行参数、权限模式与规则、插件启用状态（记在安装目录的 `.pluginignore` 里）、提示词库与子智能体定义（在安装目录的 `daAgent` 下）、界面布局方案与最近文件列表。工作区目录这一项也是应用级的：工程带着脚本和产物走，打开时落在新机器设置的工作区根目录里。

同一台机器上把工程拷到另一个目录再打开时，工作区本地目录按新的工程路径重新算，等于一份新的本地副本，包内的脚本和产物照常解压出来，不会互相影响。

## 相关章节

- [工程文件](../getting-started/project.md)：压缩包成员、界面状态文件、工作区策略。
- [主界面](../getting-started/main-window.md)：布局方案与界面状态的保存和恢复。
- [配置模型](../agent/model-config.md)：模型供应商的添加、拉取与测试。
- [权限控制](../agent/permission.md)：三种权限模式、工具分级、路径规则、代码判官。
- [脚本工作区](../script/workspace.md)：工作区目录、脚本超时与结果上限的作用。
- [插件管理](../plugin/manager.md)：插件目录与启用状态文件。
- [常见故障](../reference/troubleshooting.md)：设置看起来没生效时的排查。
