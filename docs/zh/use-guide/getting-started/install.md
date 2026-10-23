# 安装与首次启动

Data Workbench 把数据处理的过程画成一张图，每个处理步骤是一个节点，节点之间的连线表示数据的流向，改一次参数就能把整条流程重跑一遍。软件也提供直接对着表格操作的清洗工具、面向论文的绘图功能，以及一个能读写数据、画图、跑脚本的智能体。

这一章讲软件的运行环境、安装目录里有什么、Python 解释器从哪里来，以及第一次启动时会看到什么。界面各区域怎么用，见[主界面](main-window.md)。

## 软件解决的问题

重复的数据处理有两类开销，软件对这两类开销各给出一种做法。

同一套清洗步骤要在不同批次的数据上反复执行。把步骤画成工作流之后，换一份输入只需要改动起点节点读的文件，中间节点的参数不动，见[工作流基础](../workflow/basics.md)。

单次的数据检查不想搭流程。打开数据集后，表格视图和「DataFrame」上下文标签提供插入行列、改列类型、补缺失值、按条件筛选这些操作，结果直接写回数据集，见[数据清洗](../analysis/cleaning.md)。

绘图方面，一个绘图标签页里可以放多个坐标系，每条曲线、每处标注都能单独选中和单独设置，见[创建图表](../chart/create.md)。

内置智能体把上述功能包装成一组它可以调用的工具，用自然语言描述任务，由它决定调用哪几个，见[内置工具](../agent/tools.md)。

## 运行环境

### Windows

Windows 是主要平台。各部分的要求见下表。

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows，64 位 |
| Qt | Qt 5.14 或更高，或者 Qt 6.x |
| C++ 编译器（自行编译时） | MSVC 2019 或更新（MSVC 是 Visual Studio 自带的 C++ 编译器），支持 C++17 |
| CMake（自行编译时） | 3.16 或更高 |
| Python | 3.7 或更高；要用智能体功能，建议 3.11 或更高 |

程序运行时需要 Qt 的动态库。使用预编译包时这些库应当与可执行文件在同一目录；自行构建之后如果提示缺少动态库，用 Qt 自带的 `windeployqt` 把它们复制到可执行文件所在目录。

### Linux 与 WSL

Linux 下从源码构建，用系统自带的 Qt 6 加开发包即可。Ubuntu 24.04 上的依赖安装命令如下。

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools qt6-svg-dev \
    qt6-5compat-dev qt6-tools-dev qt6-base-private-dev \
    libgl-dev libglu1-mesa-dev pkg-config libxkbcommon-dev \
    zlib1g-dev ninja-build python3-dev libpython3-dev
```

`qt6-base-dev` 提供 Qt 的核心模块，`qt6-svg-dev` 用于显示 SVG 图标，`qt6-tools-dev` 提供生成翻译文件用的 Linguist 工具，`qt6-5compat-dev` 提供 Qt 5 兼容模块。`qt6-base-private-dev` 是 Qt-Advanced-Docking-System 在 Linux 上编译时需要的私有头文件，不能省。其余几个包分别是 OpenGL（`libgl-dev`、`libglu1-mesa-dev`）、键盘映射（`libxkbcommon-dev`）、压缩库（`zlib1g-dev`）、Ninja 构建工具（`ninja-build`）、编译参数查询工具（`pkg-config`），以及 Python 开发头文件（`python3-dev`、`libpython3-dev`），编译主程序时都会用到。

依赖装好后按[构建说明](../../build/build-instructions.md)配置和编译。第三方库的编译顺序、构建选项和常见错误都在那份文档里。

## 第三方库是编译好的

仓库不自带第三方库的编译产物，第三方库编译安装到一个按构建配置命名的目录里，比如 `bin_Release_qt6.7.3_MSVC_x64`。目录名的格式是 `bin_{构建类型}_qt{Qt 版本}_{编译器}_{架构}`，同一个工作副本里可能有多个这样的目录，对应不同的 Qt 版本和构建类型。

这些目录里有 `include`、`lib`、`share` 等子目录，SARibbon、qwt、Qt-Advanced-Docking-System、quazip 这些第三方库已经安装好，编译主程序时直接链接，不需要重新下载或编译。主程序自己的产物（`DAWorkBench.exe` 和各模块的动态库）也会装进同一个目录，运行时需要的文件都在一处。

只有改动第三方库自身的源码时才需要重新编译并安装它们，日常使用和日常开发都用不到这一步。

## 安装目录里有什么

| 内容 | 说明 |
|------|------|
| `DAWorkBench.exe` | 主程序 |
| 各模块动态库 | `DAData.dll`、`DAGui.dll`、`DAFigure.dll`、`DACommonWidgets.dll` 等，按功能拆分 |
| `plugins/` | C++ 插件，每个 `.dll` 在插件管理器里占一行 |
| `pyplugins/` | Python 节点包，每个包一个子目录，必须有 `__init__.py` |
| `daAgent/` | 智能体的提示词文件（`.md`）和子智能体定义（`subagents/` 子目录） |
| `PyScripts/` | 随程序发布的内置 Python 脚本 |
| `translations/`、`translations_user/` | 界面翻译文件（`.qm`） |
| `python-config.json` | 可选，指定 Python 解释器；没有这个文件也能运行 |

`plugins/` 和 `pyplugins/` 里的条目可以在界面上启用和停用，停用记录写在各自目录下的 `.pluginignore` 文件里，一行一个名字。`daAgent/` 里的提示词出现在「AI Agent」标签页的画廊中，可以改也可以删。翻译文件放在 `translations/`（随程序安装）和 `translations_user/`（自行添加）两个目录里，程序启动时扫描这两处。

## Python 从哪来

软件不内置 Python，运行时按下面的顺序找解释器：

1. 程序目录下的 `python-config.json` 中 `interpreter` 记的路径；
2. 系统 `PATH` 里的 `python`，等价于在命令行执行 `where python`，取输出里最后一个能执行的路径。

两处都找不到时，数据集、工作流节点和智能体都无法启动，界面上会因此报错或功能不可用。设置页 Python 页里填的路径在应用时会写进同一个 `python-config.json`，所以在这一页配好一次，之后每次启动都按它走；从这一页保存会把这个文件重写成只含绝对路径的内容。

`python-config.json` 是一个 JSON 文件，`config` 段下的 `interpreter` 填解释器可执行文件的完整路径，路径中可以用 `${current-app-dir}` 表示程序所在目录。

```json
{
  "config": {
    "interpreter": "${current-app-dir}/Python/python.exe"
  }
}
```

外层的 `config` 是固定的键名，`interpreter` 指向解释器可执行文件。上面这段把解释器指向程序目录下的 `Python/python.exe`，适合把一份独立的 Python 环境与程序放在一起，整个目录整体拷贝到别的机器上仍然能用。源码仓库里有一份同名同结构的空白模板 `src/template-python-config.json`，值留空待填。文件不存在、缺少 `config` 段、路径无效时，程序会跳过这个文件继续往下找，不会因此报错退出。

选中的解释器要能提供数据处理和绘图所需的包，pandas、numpy、scipy、openpyxl、matplotlib、seaborn 都在其中。仓库根目录的 `requirements.txt` 列出了全部依赖，用一条命令装进当前解释器。

```bash
pip install -r requirements.txt
```

`pip` 必须是所选解释器自带的那个，解释器位于虚拟环境或独立目录时尤其要注意这一点，装错地方等于没装。`-r` 表示从文件读取包列表。装完后回到设置对话框，用 Test 按钮确认解释器可以启动。

设置对话框的 Python 页只有两项内容，使用频率不高，配一次即可。

| 字段 | 说明 |
|------|------|
| `Interpreter path` / 解释器路径 | 解释器可执行文件的完整路径 |
| `Extra module search paths (sys.path)` / 额外模块搜索路径 | 追加到 Python 模块搜索路径的目录，可以加多条 |

旁边的三个按钮是 `Browse` / 浏览、`Auto Detect` / 自动检测、`Test` / 测试。Test 的结果提示有 `OK` / 正常、`Please specify a Python interpreter path`、`File does not exist: %1` / 文件不存在：%1、`Failed to run: %1` / 运行失败：%1；自动检测失败时提示 `No Python interpreter found in system PATH` / 系统 PATH 中未找到 Python 解释器。页面下方注明，解释器和模块路径的改动在重启程序后生效。

## 第一次启动

双击 `DAWorkBench.exe` 启动。启动过程里屏幕上有一张启动画面，底部逐行显示当前阶段：`Initializing...` / 正在初始化、`Initializing core components...` / 正在初始化核心组件、`Loading user interface...` / 正在加载用户界面、`Ready` / 启动完成。画面上还显示程序名和版本号。

启动画面有三种处理方式：

- 启动时加上 `--no-splash` 参数，本次不显示。
- 打开设置对话框的高级页，取消勾选 `Show splash screen on startup` / 启动时显示启动画面，这一项改动下次启动生效。
- 把一张 `splash.png` 放到程序目录下，用它当背景图；图片不存在时用程序内置的背景。

主窗口出现后，界面里已经建好一个名为 `Untitled` / 未命名 的工作流标签页，画布是空的。左侧停靠区依次是节点、绘图管理、数据管理三个面板，中间是操作区，右侧是设置等面板。

启动时如果在命令行里带了一个已存在的文件路径，程序把它当作工程文件打开；带 `--import-data` 和文件路径，则先导入这些数据再进入界面。参数的完整清单见[命令行参数](../reference/command-line.md)。

## 第一次启动要做的事

按顺序做完下面三件事，后面各章的操作就都能照着做。

1. 指定 Python 解释器。打开设置对话框的 Python 页，点 `Auto Detect` / 自动检测，或者手动填路径，再点 `Test` / 测试 确认解释器可以启动，提示为 `OK` / 正常。解释器没配好时，导入数据、运行工作流和智能体都会失败。这一页的每一项见[设置项说明](../reference/settings.md)。
2. 需要中文界面就改语言。设置对话框的通用页里有 `Interface language` / 界面语言 下拉框，选好之后重启程序生效；下拉框里没有要用的语言时，把语言代码（中文是 `zh_CN`）写进配置文件 `dawork-config.json` 的 `ui.language` 项也能达到同样效果。细节见[主界面](main-window.md)的语言一节。
3. 导入一份数据。主页标签页的 `Create` / 创建 面板里有 `Add Data` / 添加数据，选一个 CSV 文件或 Excel 文件即可；支持的文件格式和导入参数见[导入数据](../data/import.md)。

想用智能体，还需要在设置里添加模型供应商并填入接口地址与密钥，否则它收不到回复，步骤见[配置模型](../agent/model-config.md)。

## 常见问题

智能体不回复任何内容。多数情况是没有配置模型供应商，界面上会提示 `LLM is not configured. Please configure LLM in settings first.`。按[配置模型](../agent/model-config.md)加上供应商即可。

导入 Excel 或 Parquet 文件失败，报找不到模块。当前解释器缺少 openpyxl 或 pyarrow，用 `pip install -r requirements.txt` 补齐。

界面语言改了没有变化。语言和字体的改动在重启程序后生效，关闭并重新启动。

启动时弹窗提示缺少某个动态库。Qt 的运行库不在可执行文件所在目录，也不在系统 `PATH` 里。在程序目录下执行 Qt 自带的 `windeployqt DAWorkBench.exe`，它会把缺的动态库复制过来。

工作流节点一运行就报 pandas 相关错误。选中的解释器里没有 pandas，或者程序找的不是所设想的那一个解释器。用设置页的 Test 按钮确认路径，再对照 `where python` 的输出确认程序实际取用的是哪一个：程序取的是这份输出里最后一项能执行的路径。

## 相关章节

- [主界面](main-window.md)：Ribbon 标签页、停靠面板、布局与界面状态的保存。
- [工程文件](project.md)：`.dapro` 文件里装了什么，什么时候必须保存。
- [导入数据](../data/import.md)：各文件格式与导入向导的参数。
- [设置项说明](../reference/settings.md)：设置对话框里每一个页面的每一项。
- [构建说明](../../build/build-instructions.md)：从源码构建的完整流程。
