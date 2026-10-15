# 把节点打包成插件

上一章写出了节点文件，它们还只是磁盘上的 Python 代码。要让软件在启动时把它们收进节点列表，需要把它们放进一个能被发现的位置，并准备最小的包结构。这一章讲软件找节点的两条路、包在磁盘上的样子、`setup.py` 里的声明、启用与停用，以及包加载失败时从哪里看原因。

## 两种让软件找到节点的方式

软件找节点包有两条路，默认两条都在走：扫描安装目录下的 `pyplugins` 目录，以及查 Python 环境里的入口点。两条路可以同时开着，但同一个包只用其中一条装上就够。两份都装会出现两份同名节点：目录扫描按文件路径生成模块名，装好的包按包名导入，同一个类在两条路下的标识（`qualified_name`）不一样，去重规则认不出它们是一回事。

### 目录扫描

把包目录整个复制到安装目录下的 `pyplugins` 里，重启软件就能用，不需要动 Python 环境。软件只认一级子目录，而且这个子目录里必须直接有 `__init__.py`，否则整目录跳过。包目录名就是软件里区分各个包的标识。

扫描会递归走进包目录下的所有子目录，把每个 `.py` 文件按路径单独导入。`__pycache__` 目录，以及名字为 `__init__.py`、`setup.py`、`conftest.py` 的文件会被跳过。导入时模块名按文件相对包目录的位置生成，比如包里的 `nodes/scale_column.py` 生成的模块名是 `nodes.scale_column`，节点的类型标识因此带上这一段；工程文件里给每个节点实例记的编号形如 `nodes.scale_column.ScaleColumnNode_1`。

这种导入方式带两个后果。包内相对导入（`from .helper import ...`、`from ..utils import ...`）在目录扫描下不可靠，能不碰就不碰；多个节点文件要共用的代码，放在一个能按名字导入的位置，比如安装目录下 `PyScripts` 里的独立包，内置的分析节点把共享的读写代码放在 `DADataAnalysisCore` 里，节点文件用 `from DADataAnalysisCore.io import read_data` 取用。另一个后果是节点里引用包内资源要用 `__file__` 拼绝对路径，图标就是一个典型例子，写法在下面的目录结构里。

### 入口点

在包的 `setup.py` 里声明一个入口点，把包注册到名为 `data_workbench.plugin` 的组，然后用 `pip install` 把包装进软件用的那个 Python 环境。软件启动时会用 Python 的 `importlib.metadata` 查这个组，逐个加载入口点。

开发阶段可以用可编辑模式安装，改源码之后不用重新安装，但软件仍然要重启。

```bash
pip install -e .
```

`-e` 表示可编辑安装，Python 环境里留下的是指向当前目录的链接，源码改动直接可见。软件启动时导入模块，改动要在下一次启动才被读到。

入口点等号右边既可以是一个包名或模块名（扫描该模块里的节点类），也可以直接指向一个类，写成 `MyNodes.nodes.scale_column:ScaleColumnNode` 的形式。这种方式下包是正常安装的，包内的相对导入、`import MyNodes.xxx` 这类写法都能用，第三方依赖也可以交给 pip 一起装。

### 怎么选

随软件一起分发、或者只是给自己和同事装几个节点，用目录扫描：拷贝一个文件夹就行，卸载就是删文件夹，`pyplugins/.pluginignore` 里留一行就能停用。要发布成可以用 pip 安装的分发包，或者包内需要多层结构、需要正常导入自己的子模块，用入口点。

安装方式选定之后不要中途更换。节点在工程文件里按标识找回，两种装法下同一个类的标识不一样，先按文件夹分发、后来改成 pip 安装，之前存过的工程打开时会对不上；反过来更换也一样。真要换，先把用到的工程迁完再停掉旧装法。

## 包放在哪里

软件只从安装目录往周围找节点包，不看正在编辑的工程放在哪里。安装目录下与节点有关的有三个子目录。

- `plugins/`：编译好的 C++ 插件文件，一个插件一个动态库。
- `pyplugins/`：Python 节点包，一个包一个一级子目录，比如 `pyplugins/MyNodes/`、`pyplugins/DASystemNodes/`。
- `PyScripts/`：随软件安装的 Python 包，软件会把这个目录加进模块搜索路径，工作流基础模块 `DAWorkbench` 就在里面。

`plugins` 目录里的插件手动拷贝进去就行，重启之后由插件管理器加载，插件管理对话框里的 `C++ Plugins` 分组列的就是它。`pyplugins` 是 Python 节点包的家，包目录名就是包在 `.pluginignore` 里的名字。节点文件里的 `from DAWorkbench.DAWorkFlowPy import ...` 靠的是 `PyScripts` 在模块搜索路径里这一步。

把这三个目录和用户自己的文档目录区分开：节点包不放工程目录，工程换机器时不会带着节点包走，接收工程的一方要自己装上同一个包。

## 把节点做成一个包

从上一章的节点文件到一个可用的包，步骤不多，按顺序走一遍即可。

1. 在开发目录里建 `MyNodes` 目录，里面建 `nodes` 子目录，把 `scale_column.py` 放进去，再补一个空的 `nodes/__init__.py`。
2. 在包根写 `__init__.py`，顶部调用 `setup_i18n()`，随后导入 `nodes` 下的节点模块。
3. 把节点的 SVG 图标放进包里的 `icon` 目录，节点文件里的 `@NodeDef(icon=...)` 用 `__file__` 拼出它的绝对路径。
4. 需要多语言时，复制内置包的 `i18n` 目录，改掉脚本顶部的 `DOMAIN`，生成 `.mo` 文件。
5. 要发布成 pip 包时写 `setup.py`，声明依赖和 `data_workbench.plugin` 入口点；只是自己用可以跳过这一步。
6. 把整个 `MyNodes` 目录复制到软件安装目录的 `pyplugins` 下，重启软件，在节点列表里找新分组。
7. 拖一个节点出来、设参数、运行、保存工程、重新打开，确认参数和结果都还在。

第 7 步不能省。参数值的存取走序列化，端口名的改动、参数的默认值和类型都会在这一步暴露出来；工程重新打开后节点显示空白，通常是运行期状态没有用 `serialize_runtime_state()` 存下来。

## 包在磁盘上的结构

下面是一个节点包的目录示例，包名取 `MyNodes`，节点来自上一章的 `scale_column.py` 和后面出现的画图节点，放在 `nodes/` 下，图标与翻译各占一个目录。

- `MyNodes/__init__.py`：包入口，先调用 `setup_i18n()`，再导入各节点模块。
- `MyNodes/setup.py`：入口点声明与依赖声明，用入口点方式时必需。
- `MyNodes/icon/scaleColumn.svg`：节点图标，200 × 200 的 SVG。
- `MyNodes/i18n/core.py`：`setup_i18n()` 的实现，域名取包名。
- `MyNodes/i18n/locale/zh_CN/LC_MESSAGES/MyNodes.mo`：由 `.po` 编译得到的翻译文件。
- `MyNodes/nodes/__init__.py`：节点子目录的入口，可以为空文件。
- `MyNodes/nodes/scale_column.py`：节点定义，上一章写的那个。

`__init__.py` 是包的门面，内容很短：顶部先调用 `setup_i18n()` 把中文翻译装进 Python 的内置命名空间，然后按顺序导入 `nodes/` 下的各个节点模块，让里面带 `@NodeDef` 的类被注册。顺序不能颠倒，节点模块在导入时就会执行 `@NodeDef(category=_(...))` 这类代码。

`setup.py` 只在用入口点方式时需要，随软件拷贝进去的包可以没有这个文件。`icon` 目录不是硬性要求，不放图标时节点列表里显示默认节点图标。`i18n` 目录也不是必须的，只有需要让同一个包在不同语言的界面上显示不同文字时才建；`i18n/core.py` 里的 `setup_i18n()` 从包内 `i18n/locale/<语言>/LC_MESSAGES/<域名>.mo` 读取翻译，域名用包名，避免和别的包撞名。`nodes/` 也不是强制的目录名，扫描会走进包目录下的任意子目录，用 `nodes/` 是内置两个包的约定。

命名上有几条约定。包目录名会当成 Python 包名，也是 `.pluginignore` 里记的名字，用英文、不加空格，避免和内置的两个包重名。节点文件用英文小写加下划线，和里面的类名对得上，日后找人方便。

包内部的层级也要留一点余地：节点类型的标识由文件位置和类名拼成，两个包如果内部有同名的文件和同名的类，标识就会撞上，后注册的那个会被去重规则丢掉、节点少一个。自己的包用独特的文件名和类名，不会碰到这种情况。

图标在节点文件里的写法值得单独说一句：路径不能用相对路径，要用 `os.path.join` 按 `__file__` 拼出绝对路径。目录扫描方式导入的文件，运行位置和当前目录都不确定，相对路径会失效，而图标加载失败不报错，只是退回默认图标，不容易发现。

```python
import os

_ICON_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "icon")

icon = os.path.join(_ICON_DIR, "scaleColumn.svg")
```

`os.path.abspath(__file__)` 拿到当前节点文件的绝对路径，外面一层 `dirname` 得到 `nodes/` 目录，再一层得到包目录，拼上 `icon` 就是图标目录。`os.path.join(_ICON_DIR, "scaleColumn.svg")` 拼出图标文件的绝对路径，把它交给 `@NodeDef(icon=...)`。`icon/scaleColumn.svg` 不存在或者路径拼错时，节点列表里会显示默认节点图标，不会弹任何提示。

### 包入口的写法

包根上的 `__init__.py` 做两件事：装好本包的翻译，再把各个节点模块导入一遍。下面是它的完整内容，两个内置节点包用的就是这个结构。

```python
# -*- coding: utf-8 -*-
"""MyNodes - extra workflow nodes for Data Workbench."""

# 必须先调 setup_i18n()，再导入节点模块：
# 节点模块在导入时就会执行 _("...")，此时翻译要已经装好
from .i18n.core import setup_i18n

setup_i18n()

from .nodes.scale_column import ScaleColumnNode
from .nodes.quick_plot import QuickPlotNode

__all__ = [
    "ScaleColumnNode",
    "QuickPlotNode",
]
```

文件开头的文档字符串说明这个包是做什么的。`from .i18n.core import setup_i18n` 把翻译初始化函数从包内取出来，紧接着调用它，把 `i18n/locale` 下对应语言的 `.mo` 装进 Python 的内置命名空间。随后的两行导入把节点类拉进这个包，`@NodeDef` 在导入的这一刻执行，节点就是在这一步进入注册表的。两个导入的先后顺序就是节点在列表里出现的顺序。`__all__` 是 `from MyNodes import *` 时的导出清单，不写也能工作，写上便于对照。

这里的导入用的是相对写法（点号开头），它要求这个文件以包的形式被导入。把包整个复制进 `pyplugins`、靠目录扫描发现时，扫描逐个导入的是 `nodes/` 下的节点文件，这个文件不参与；节点文件本身一律用绝对导入（`from DAWorkbench.DAWorkFlowPy import ...`），两种方式下都能工作。

### 翻译文件的生成

包里的 `.po` 和 `.mo` 不需要手写，包内 `i18n` 目录下带着两个脚本：`generate_i18n.py` 扫描源码里的 `_("English")` 和后面的 `# cn:中文` 注释，生成 `.pot`、按语言生成或更新 `.po`，再编译出 `.mo`；`update_po.py` 需要先装 polib，用来把 `# cn:` 注释批量写进已有的 `.po`。包根上的 `update-i18n.sh` 把这一串串起来。

```bash
cd MyNodes
python i18n/generate_i18n.py
```

脚本按文件位置定位包目录，所以在包的根目录里执行。用之前先把脚本顶部的 `DOMAIN` 改成自己的包名（它就是 `.mo` 的文件名，也是 `setup_i18n()` 认的域名），`SUPPORT_LANGUAGES` 改成要支持的语言列表。生成出来的 `.mo` 要跟着包一起走，只复制 `.py` 文件不带 `i18n/locale` 时，界面上看到的还是英文源文本。

## setup.py 的写法

下面是这个包的 `setup.py`，用的是项目里两个内置节点包相同的结构。

```python
# -*- coding: utf-8 -*-
"""Installation metadata for the MyNodes node package."""

from setuptools import setup, find_packages

setup(
    name="MyNodes",
    version="0.1.0",
    description="Extra workflow nodes for Data Workbench",
    packages=find_packages(),
    python_requires=">=3.7",
    install_requires=[
        "pandas",
    ],
    extras_require={
        "plot": ["matplotlib"],
        "export": ["openpyxl", "pyarrow"],
    },
    entry_points={
        "data_workbench.plugin": [
            "MyNodes = MyNodes",
        ],
    },
)
```

`name` 是分发包的名字，安装之后出现在 pip 的包列表里；它和包目录名保持一致时最省心，`pyplugins/.pluginignore` 里记的也是目录名。`version` 是这个包自己的版本号，和软件版本无关，改代码之后顺手加一，便于确认机器上装的是哪一版。

`packages=find_packages()` 让 setuptools 自动收集所有带 `__init__.py` 的子目录，`nodes` 和 `i18n` 这样就会被一起装进去，新增子目录不用改这一行。`python_requires=">=3.7"` 声明需要的 Python 版本。

`install_requires` 列出运行时离不开的第三方包，这里写 pandas：它在节点模块顶层被导入，缺了包就导不进来。用 pip 安装时这些依赖会被一起装上，装不动或者版本冲突会直接告诉安装的人。

`extras_require` 是可选的依赖分组，只有当安装时显式点名才会装上：`plot` 组放 matplotlib，`export` 组放 openpyxl 和 pyarrow，分别对应画图和导出表格。装这个包时写 `pip install .[plot]` 或 `pip install .[plot,export]` 就会连这些一起装。这种分组适合「不用到某个功能就不需要这个依赖」的情况，内置的数据分析节点包就是这么分组的。

`entry_points` 是注册到软件的那一行。键名 `data_workbench.plugin` 是软件固定的入口点组名，不能改；列表里每一项的等号左边是入口点名，习惯写成包名，右边是包名，表示加载这个包并扫描里面的节点类。入口点也可以直接指向一个类，写成 `"MyNodes.scale = MyNodes.nodes.scale_column:ScaleColumnNode"`，冒号后面是类名。

## 软件启动时怎么找到并导入包

启动过程分几步，顺序是固定的。软件先确认 Python 解释器已经就绪，没有就绪时跳过后面的步骤，并在消息面板里留一条警告。接着把安装目录下的 `PyScripts` 目录加进 Python 的模块搜索路径，内置节点包依赖的 `DAWorkbench` 包就在那里。

随后扫描安装目录下的 `pyplugins`，对每个一级子目录检查是否直接包含 `__init__.py`，不包含的跳过。扫描结果再对照 `pyplugins/.pluginignore` 过滤一遍：这个文件里每行写一个包目录名，比较时不区分大小写，命中的包本轮不加载。文件不存在时按全部启用处理。

过滤之后的目录列表交给 Python 侧的节点注册表，同时开启入口点查找。注册表先从目录里发现节点，再查 `data_workbench.plugin` 组里的入口点，最后按 `qualified_name` 去重。目录扫描对每个 `.py` 文件单独做导入并捕获异常，某个文件导入失败时只往 Python 日志里写一条形如 `导入模块 'nodes.scale_column' (文件: ...) 失败: ...` 的记录，这个文件里的节点本轮不出现，其余文件照常导入。入口点加载失败时写的是另一种记录，形如 `加载入口点 'MyNodes' 失败: ...`，常见原因是包没有装进软件用的 Python 环境，或者包里 `__import__` 时就抛了异常。

一整轮发现都没有结果时，消息面板里会出现 `Python node discovery failed` 这条提示。更常见的情况是大部分节点都在、个别文件里的节点不见，这时要看 Python 日志里的那条导入失败记录。Python 解释器本身没有起来时，节点发现直接被跳过，消息面板里会留下对应的警告，这时所有节点包都不可用，先解决解释器的问题。

导入成功的类会被读出名字、分组、图标、说明和端口名，进到节点列表里。这些信息在启动那一刻从类属性上读一次，改完之后要重启才会变。节点真正的实例由软件在往画布上拖的时候按 `qualified_name` 创建，所以包里的文件位置和类名在发布之后不要再改。

## 只写 Python 的包也可以配一个 C++ 外壳

随软件分发的两个节点包各自还带一个很薄的 C++ 插件，插件的 `createNodeFactory()` 直接返回空指针，因为它不做节点注册，节点全部来自 Python 包。这个插件出现在插件管理对话框的 `C++ Plugins` 分组里，带着自己的名称、版本和描述；它和 Python 包提供的节点互不影响，停用这个 C++ 插件，节点列表里的节点照旧。

自己分发的节点包可以只有 Python 部分。放进 `pyplugins` 或者用 pip 安装都能工作，不需要编 C++，也不需要写任何注册代码。只有需要随插件一起做界面设置页、或者要用 C++ 实现节点逻辑时，才需要编一个真正的 C++ 插件，那属于插件开发的范畴，和节点包是两套东西。

## 启用与停用

插件管理入口在主页标签页的 `Config` / 配置 面板里，按钮是 `Plugin Config` / 插件设置，打开后对话框标题是 `Plugin Manager` / 插件管理。列表分成两组，一组是 `C++ Plugins`，另一组是 `Python Node Packages (changes take effect after restart)`，标题里的括号写明 Python 节点包的变更要重启之后才生效。

Python 包这一组每行显示包目录名和状态：勾选时显示 `Enabled`，取消勾选并应用之后显示 `Disabled`（中文界面下显示为「禁用」）。勾选框改成想要的状态后点 `Apply` / 应用先保存，或者点 `ok` / 确认保存并关闭对话框，`cannel` / 取消放弃这次改动。应用之后消息面板里会出现一条提示，内容形如 `1 plugin change(s) applied, some changes take effect after restart`。

停用会把包目录名写进 `pyplugins/.pluginignore`，重启之后这个包不再加载，它的节点从列表里消失；重新勾上会从忽略列表里移除。正在用这些节点的工作流不会被检查，重启之后打开这类工程时，工程里用到这个包的部分会找不到节点类，所以停用之前先确认没有工程在用。删掉整个包目录更彻底，效果和停用一样，但工程里记录的节点类型会永远找不回来。

## 确认包已经加载

最直接的检查在左侧的 `Workflow Node` / 节点 停靠面板里：新包里的节点会按 `@NodeDef(category=...)` 的分组名出现在列表中，分组是新建的，比如 `My Nodes`。面板支持工具箱和树两种显示方式，切换显示方式不影响分组。把鼠标停在条目上，提示气泡显示节点的名字和说明文字，说明文字来自 `@NodeDef(description=...)`，没写时来自类的文档字符串。

分组按分类字符串原样合并，两个不同的包用了同一个分类字符串时，它们的节点会落在同一个分组里，排在一起的先后取决于包的扫描顺序。想让自己的节点独立成组，分类字符串取得独特一些，比如带上包名的前缀。

把节点拖到画布上，选中它，右侧 `Setting` / 设置 面板里会出现 `Parameters` / 参数 页，里面列出这个节点声明的全部参数。这一页能打开，说明节点类已经正确导入、参数声明也被读到了；参数控件缺失（比如某个参数在面板里没有出现）通常是类型标签没有对应的编辑器。

节点没有出现时分几种情况。整个分组都没有，说明包含这些节点的文件没有导入成功，去 `%APPDATA%\DAWorkBench\log\da_pyscript.log` 里找 `导入模块` 开头的那条记录，后面带着文件名和原因，常见原因是缩进错、模块导入失败、`@NodeDef` 写法有误。只有个别节点不见，说明那个类本身没有带 `@NodeDef`，或者类名以 `_` 开头被跳过。图标显示成默认图标，说明 `@NodeDef(icon=...)` 里的路径拼错了。装了包但列表里完全没有变化，先确认包目录里直接有 `__init__.py`，再确认 `pyplugins/.pluginignore` 里没有这个包的名字。

## 改完代码怎么让它生效

节点包在软件启动时导入，运行中的软件不会重新读源码，改完节点文件之后要把改动的文件送到软件实际加载的位置，再重启。开发目录和安装目录分开时，用一条复制命令把包同步过去即可。

```bat
xcopy /E /Y /I "D:\dev\MyNodes" "C:\DAWorkBench\pyplugins\MyNodes\"
```

`/E` 连同子目录一起复制，`/Y` 覆盖已有文件时不询问，`/I` 在目标目录不存在时按目录处理，省去手工确认。源目录是开发目录里的包，目标目录是软件安装目录下 `pyplugins` 里的同名目录。只想快速验证一个文件时，复制单个 `.py` 到目标包的对应子目录也可以，但别漏掉 `i18n/locale` 里的 `.mo`。改完重启软件，节点列表里看到的才是新代码。

用 pip 可编辑安装的包省掉复制这一步，改源码直接反映到下次启动；发布给别人的包则要把 `.py`、`icon/`、`i18n/locale/` 一起带上，只给 `.py` 会缺图标和翻译。

## 第三方依赖

节点用到 pandas、numpy、matplotlib 这类第三方包时，在包的 `setup.py` 里用 `install_requires` 声明，用 `extras_require` 分出可选功能的依赖，写法见上一节。用 pip 安装的包由 pip 负责把这些依赖装齐，命令行在包目录里执行即可。

```bash
pip install .
pip install .[plot,export]
```

第一条命令把当前目录的包装进 Python 环境，第二条额外装上 `plot` 和 `export` 两组可选依赖。前提是这条 `pip` 属于软件用的那个 Python 环境；软件用哪个解释器由它自己探测，不确定时先在同环境的命令行里跑一次 `pip --version`，看输出的路径是不是软件安装目录下的 Python。

把包直接复制进 `pyplugins` 时，pip 不参与，`setup.py` 里的依赖声明只是说明，没有任何东西会去安装它们。这时依赖要自己确认已经存在，做法是看软件自带的 Python 环境里有没有这些包，内置的 pandas、numpy、matplotlib、openpyxl、pyarrow 都在随软件安装的依赖清单里，节点直接导入即可。

依赖缺失时按导入位置分两种表现。写在节点文件顶层的 `import`（比如 pandas）失败，整个文件导入失败，只留一条日志，这个文件里的节点全部不出现。想缩小影响，把可选依赖的导入包在 `try` 里，模块里记一个布尔量，执行时发现不可用就返回失败。

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


@NodeDef(name="Quick Plot", category="My Nodes")
class QuickPlotNode:
    """Draw a chart from the input DataFrame when matplotlib is available."""

    class Inputs:
        data = Input("DataFrame", required=True, description="Data to plot")

    class Outputs:
        figure_path = Output("str", description="Path of the saved image")

    def execute(self, inputs=None, params=None):
        if not HAS_MATPLOTLIB:
            return False

        df = inputs.get("data")
        if df is None:
            return False

        fig, ax = plt.subplots(figsize=(8, 6))
        df.plot(ax=ax)
        fig.savefig("quick_plot.png", dpi=150)
        self._output_data["figure_path"] = "quick_plot.png"
        return True
```

`try` 里 `import matplotlib` 之后立刻把后端切成 `Agg`，它不依赖窗口系统，在后台跑也能出图。导入成功时 `HAS_MATPLOTLIB` 为 `True`，失败时为 `False`，两种情况下这个模块都能被导入，节点都能出现在列表里。`execute()` 开头判断这个布尔量，不可用时返回 `False`，节点被标成出错，其它节点照常运行。

还有一种做法是把整块可选节点从包入口里摘出去：在 `__init__.py` 里把可选模块的导入包在 `try` 里，`import` 失败就跳过，节点列表里干脆不出现这一类节点。内置的数据分析节点包对绘图节点同时用了这两种写法，包入口拦住导入失败，节点自己再判断一次依赖，缺依赖时错误信息清楚、影响范围也只落在这一类节点上。

## 交出去之前

包发到别人机器上之前，在另一台没装过开发环境的机器上过一遍，检查项不多，但每一条都对应一种常见的返工。

- 包目录里直接有 `__init__.py`，整个目录复制到 `pyplugins` 之后重启，分组和节点都出现。
- 每个节点都能拖到画布上，选中后 `Parameters` / 参数 页能打开，改一个参数值再运行，结果跟着变。
- 节点列表里的图标是包自己的图标，不是默认节点图标。
- 中文界面下分组名、节点说明、参数说明都是中文，`.mo` 文件跟着包一起发过去了。
- 写进 `setup.py` 的依赖在目标机器上装得到，可选功能用到的依赖在需要时才装。
- 包的版本号比上一版大，改过类名或文件位置时确认没有旧工程还在用这些节点。

## 常见问题

节点列表里出现两份同名的节点。同一个包同时被复制进 `pyplugins` 又被 pip 安装，两条路各注册了一遍。留一种装法，删掉另一种，再重启。

pip 安装成功、命令行里也能 `import`，软件里却看不到节点。装到了另一个 Python 环境里，软件用的是它自己那个解释器。确认 `pip --version` 输出的路径落在软件安装目录下，再重新安装。

改了 `setup.py` 里的依赖声明，重启后行为没变。直接复制进 `pyplugins` 的包不经过 pip，`setup.py` 里的依赖声明不参与加载，缺的包装到软件用的 Python 环境里才有用。

`pyplugins/.pluginignore` 被手工改过之后包还是在。文件里每行只写包目录名，前面带 `#` 的行是注释；写错名字不会生效，写对名字也要重启。

## 相关章节

- [开发自定义节点](./custom-node.md)：节点类的写法、参数与端口声明、执行约定。
- [插件管理](../plugin/manager.md)：插件放在哪里、列表里每一项的含义、启用与停用的完整说明。
- [数据分析节点手册](./nodes-analysis.md)：内置的二十一个节点，可以作为打包结构的对照。
