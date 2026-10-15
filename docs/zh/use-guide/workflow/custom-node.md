# 开发自定义节点

软件自带的节点覆盖了读取、清洗、统计、导出和绘图，碰到没有的算法时可以自己写一个。一个节点就是一个普通的 Python 类，加上 `@NodeDef` 装饰器，声明好参数和端口，软件启动时会把它收进节点列表。这一章从什么都不做的节点写起，逐步补上输入输出、参数和执行逻辑，最后得到一个能跑通的完整节点。

## 节点在代码里由哪几样东西组成

一个节点放在一个 `.py` 文件里，结构由四部分组成。类上的 `@NodeDef(...)` 给出节点在列表里显示的名字、分组和说明。类属性形式的 `Parameter(...)` 描述这个节点有哪些可以改、会随工程保存的参数。嵌套类 `Inputs` 和 `Outputs` 用 `Input(...)`、`Output(...)` 描述端口。方法 `execute()` 里写明节点拿到输入和参数之后做什么。

这几个名字都从 `DAWorkbench.DAWorkFlowPy` 导入，它是软件自带的工作流模块，写节点时不需要额外安装依赖。装饰器在文件被导入的那一刻执行，把节点类的信息整理好交给注册表，所以节点类的写法定下来之后，界面上看到的分组、端口、参数都由这几处声明决定。

## 第一步：让节点出现在列表里

下面这个节点什么都不做，只用来验证装饰器与注册这一步是通的。存成 `empty_node.py`。

```python
from DAWorkbench.DAWorkFlowPy import NodeDef


@NodeDef(name="Empty Node", category="My Nodes")
class EmptyNode:
    """A node that does nothing yet."""

    def execute(self):
        return True
```

`from DAWorkbench.DAWorkFlowPy import NodeDef` 把装饰器引进来，这个包的位置由软件在启动时加进 Python 的模块搜索路径，节点文件不用自己改 `sys.path`。

`@NodeDef(name="Empty Node", category="My Nodes")` 里的 `name` 是节点在列表和画布上显示的名字，写英文并写定，理由见后面的翻译一节。`category` 是节点列表的分组名，软件按这个字符串原样分组，这里会出现一个名为 `My Nodes` 的分组。两个参数之外，装饰器还接受 `icon`（图标路径）、`description`（节点说明）和 `style`（外观）。

类的文档字符串也提供说明文字，鼠标停在节点列表的条目上时，提示气泡里显示的就是它；另写了 `@NodeDef(description=...)` 时以后者为准，文档字符串这段文字不参与翻译。

`execute(self)` 是框架调用的方法。框架在调用前用 Python 的签名检查数参数个数：这里只有一个 `self`，就无参调用；下一节会换成带参数的形式。方法返回 `True` 表示这个节点执行成功，画布上的状态会变成成功色。

这个节点没有输入、没有输出、也没有参数，拖到画布上单独运行也会立刻成功。要运行工作流，工程必须先保存过，这一点见[工作流基础](./basics.md)。

## 第二步：接上一个输入和一个输出

节点要参与数据流，就得有端口。下面这个节点把上游送来的数据原样转发到下游，同时演示节点类的初始化写法。

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output


@NodeDef(name="Pass Through", category="My Nodes")
class PassThroughNode:
    """Forwards the input data to the output unchanged."""

    class Inputs:
        data = Input("DataFrame", required=True, description="Data coming from upstream")

    class Outputs:
        result = Output("DataFrame", description="The same data, forwarded downstream")

    def __init__(self):
        super().__init__()

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        self._output_data["result"] = inputs.get("data")
        return True
```

`class Inputs` 和 `class Outputs` 是写在节点类里面的两个嵌套类，里面每个属性就是一个端口。属性名就是端口名，`Input(...)` 的后两个参数按顺序是 `required` 和 `description`，`Output(...)` 只有 `description`，两者的第一个参数都是数据类型标签。端口名在 `execute()` 里当作字典的键用，所以名字用英文小写、改动之前要想清楚：已存的工程按端口名恢复连线，改名会让旧连线对不上。

`Input("DataFrame", required=True, ...)` 的第一个参数是给读代码的人看的数据类型说明。软件在执行时不做类型检查，连 `int` 和 `DataFrame` 接在一起也不会拦。`required` 只写进端口的描述信息，执行和连线都不看它，一个输入端口也不限制只能接一条线。输出端口的第一个参数同样只是标签。

`__init__(self)` 里第一行固定写 `super().__init__()`。装饰器会新建一个类，继承顺序是「新类 → 框架基类 `DAWorkflowNode` → 节点类」；实例化时先跑框架基类的 `__init__`，它建好 `node_id`、`_input_data`、`_output_data`、`is_global`、`_node_state`，再把每个参数的默认值写成实例属性，然后调用 `super().__init__()` 把控制权交给节点类自己的 `__init__`。节点类里再写一行 `super().__init__()`，控制权顺着继承顺序继续往下传，链上排在节点类后面的类才会被初始化。这一行省掉时，框架负责的那部分照样会建好，但节点类一旦改成继承别的基类，那一层的初始化会被静默跳过。

`execute(self, inputs=None, params=None)` 是约定的两个参数写法，框架检测到参数个数不小于 2 就按 `execute(inputs, params)` 调用。两个入参都可能是 `None`，节点没有输入端口时 `inputs` 就是空字典或 `None`，所以每个节点都以 `if inputs is None: inputs = {}` 开头防住这种情况。`params` 同样要防 `None`，它在第三步出现。

`inputs.get("data")` 按端口名取上游数据，取不到时返回 `None`。输出通过 `self._output_data["result"] = ...` 写进字典，键是输出端口名。写成 `return inputs.get("data")` 不会有任何效果，框架不收集返回值。

## 第三步：加一个参数

参数和端口是两回事：端口在连线时接收数据，参数在右侧属性面板里用手改，随工程一起保存。下面给节点加一个浮点参数，并把它用起来。

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Scale Column", category="My Nodes")
class ScaleColumnNode:
    """Multiplies the input data by a factor."""

    factor = Parameter(
        float,
        default=1.0,
        min=-100.0,
        max=100.0,
        step=0.1,
        decimals=3,
        description="Scale factor applied to the input",
    )

    class Inputs:
        data = Input("DataFrame", required=True, description="Data coming from upstream")

    class Outputs:
        result = Output("DataFrame", description="Scaled data")

    def __init__(self):
        super().__init__()

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        df = inputs.get("data")
        if df is None:
            return False

        factor = params.get("factor", 1.0)
        self._output_data["result"] = df * factor
        return True
```

`factor = Parameter(float, default=1.0, ...)` 写在类属性上，第一个参数是 Python 的类型对象。`default=1.0` 是参数初始值，也是工程里没有存过这个参数时的取值。`min`、`max` 给属性面板上的数值框定出上下限，不写时范围是 -9999 到 9999。`step=0.1` 是上下箭头每次加减的幅度，`decimals=3` 是小数位数，不写时默认保留 2 位。`description` 是这一项在面板里的说明文字。

`params` 里装着所有参数名到当前值的映射。`params.get("factor", 1.0)` 的第二个参数是字面量默认值，不能写成 `self.factor.default`。参数声明是类级别的描述符，加载工程时序列化器会把存下来的值用 `setattr` 写到实例上，把这个描述符整个覆盖掉；覆盖之后 `self.factor` 已经是普通浮点数，再访问 `.default` 会抛 `AttributeError: 'float' object has no attribute 'default'`，而且只在加载过工程之后才出现，很难当场看出原因。字面量要和 `Parameter` 里的 `default` 保持一致，两处都改。

`df * factor` 会把整个表都乘一遍，字符串列参与乘法会变成重复拼接或者直接抛异常。这一步先用它过渡，第四步把范围收窄到指定的一列，并把改动行数一并写成第二个输出。

`if df is None: return False` 处理必填输入没到位的情况。返回 `False` 会让节点被标成出错，它的下游不会再被触发。

## 第四步：写出结果

把选择列的参数补上，输出改成两路，顺手把非法输入挡在计算之前。下面就是这个节点的完整文件，存成 `scale_column.py` 可以直接用。

```python
# -*- coding: utf-8 -*-
"""Give one column of a DataFrame a scale factor."""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(
    name="Scale Column",
    category="My Nodes",
    description="Multiplies one column of the input DataFrame by a factor and reports how many rows changed.",
)
class ScaleColumnNode:
    """Give one column of a DataFrame a scale factor."""

    column = Parameter(str, default="value", description="Name of the column to scale")
    factor = Parameter(
        float,
        default=1.0,
        min=-100.0,
        max=100.0,
        step=0.1,
        decimals=3,
        description="Scale factor applied to the column",
    )
    count_changes = Parameter(
        bool,
        default=True,
        description="Also output the number of values that changed",
        show_on_node=False,
    )

    class Inputs:
        data = Input("DataFrame", required=True, description="Data coming from upstream")

    class Outputs:
        scaled = Output("DataFrame", description="DataFrame with the scaled column")
        changed_count = Output("int", description="Number of values that changed")

    def __init__(self):
        super().__init__()

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        df = inputs.get("data")
        if df is None:
            return False

        column = params.get("column", "value")
        factor = params.get("factor", 1.0)

        if not column or column not in df.columns:
            return False

        scaled = df.copy()
        scaled[column] = scaled[column] * factor

        self._output_data["scaled"] = scaled
        if params.get("count_changes", True):
            self._output_data["changed_count"] = int((scaled[column] != df[column]).sum())
        return True
```

多行写法的 `@NodeDef(...)` 里，`description` 是显式说明，会覆盖文档字符串生成的说明文字。装饰器给的 `icon` 参数接受绝对路径或者 `:/` 开头的资源路径，写成裸文件名时图标加载不到，节点列表里会退回默认图标。

`column` 是字符串参数，`factor` 是带上下限的浮点参数，`count_changes` 是布尔参数，`show_on_node=False` 表示这一项不画在节点主体上。三个参数的默认值都写成了字面量形式，`execute()` 里的 `params.get(...)` 默认值与之对应。

`df.copy()` 先复制一份再改，输入数据本身不被修改。`scaled[column] * factor` 是 pandas 的列运算，`!` 比较得到布尔序列，`sum()` 数出 `True` 的个数，`int(...)` 把 pandas 的整数转成 Python 的整数，写进输出端口时更干净。

`changed_count` 端口在参数关闭时不写值。输出端口没有写值（或者写成 `None`）就不会往下游发送，下游节点的就绪判断按「收到的数据条数等于上游连线数」来做，所以关掉计数时，接到这个端口上的下游节点不会被触发。条件分支节点用的也是同一个机制。

`return True` 表示成功。返回 `None` 也算成功；返回 `False` 或者让异常抛出去，节点会被标成出错，出错节点的下游不会被执行，整条流程里其余不依赖它的节点照常跑完。

## Parameter 的完整说明

参数声明的完整形式是 `Parameter(param_type, default=None, description="", min=None, max=None, step=None, decimals=None, enum=None, filter=None, layout="inline", height=None, show_on_node=True)`。第一个参数既可以是 Python 类型，也可以是下表里的字符串标签，标签决定属性面板给出什么控件。

| 类型标签 | 面板里的控件 | 传进 `execute()` 的值 |
|---|---|---|
| `str` | 单行输入框 | 字符串 |
| `int` | 整数框，上下箭头按 `step` 加减 | 整数 |
| `float` | 小数框，位数由 `decimals` 决定 | 浮点数 |
| `bool` | 复选框 | 布尔值 |
| `list` | 列表控件加「Add」「Remove」两个按钮 | 字符串列表 |
| `dict` | 没有对应控件，这一项会被跳过 | 默认值原样保留 |
| `file` | 路径输入框加浏览按钮，弹出文件对话框 | 文件路径字符串 |
| `folder` | 路径输入框加浏览按钮，弹出目录对话框 | 目录路径字符串 |
| `enum` | 不可编辑的下拉框，选项来自 `enum=[...]` | 选中项的字符串 |
| `color` | 颜色按钮 | 颜色字符串，比如 `#5280c1` |
| `font` | 字体面板 | 字典，键为 `family`、`size`、`bold`、`italic`、`color` |
| `code` | 多行代码框 | 字符串，由节点自己解析 |

`dict` 这一行要留意：属性面板只认识上表里注册过的类型，遇到 `dict` 找不到控件就跳过这一项，界面上看不见也改不了，值只能靠默认值带进去。需要存一组有结构的设置，用 `font` 类型，它的值本来就是字典。

除 `default` 和 `description` 之外的关键字按顺序解释。`default=None` 表示没有默认值，实例上不会建对应属性，面板里是空的。`min`、`max`、`step`、`decimals` 只对 `int` 和 `float` 生效，其中 `step` 不写时每次加减 1。`enum` 给出 `enum` 类型参数的可选项，写成字符串列表。`filter` 只对 `file` 生效，内容是文件对话框的过滤字符串，用两个分号分隔不同分组，写法与 Qt 一致。

`layout` 只接受 `"inline"` 和 `"below"` 两个值，写别的值在声明时就抛 `ValueError`。默认的 `"inline"` 让参数名在左、控件在右排在同一行；`"below"` 让控件占满参数名下方整行，适合放长表达式，此时 `str` 类型会从单行输入框换成多行框。`height` 是控件高度，单位为像素，只在 `"below"` 下有效；不写时 `str` 按 80、`code` 按 100 处理。

`show_on_node` 控制这个参数要不要画在节点主体上。打开时节点上多出一行 `名称: 值`，最多画 10 行，参数更多的部分折成一行省略号；参数多而值长的节点可以把常用的几项打开，其余关掉。`show_on_node=False` 只影响画布上的显示，属性面板里照样能改。

下面这个节点把几种控件都用上一次：文件路径用 `file` 类型、格式用 `enum` 类型、附注用长文本、是否带索引用复选框。存成 `save_table.py`。

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Parameter


@NodeDef(name="Save Table", category="My Nodes")
class SaveTableNode:
    """Write the input DataFrame to a file."""

    file_path = Parameter(
        "file",
        default="",
        filter="CSV Files (*.csv);;Excel Files (*.xlsx);;All Files (*.*)",
        description="Output file path",
    )
    file_format = Parameter("enum", default="csv", enum=["csv", "excel"], description="Output format")
    note = Parameter(str, default="", description="Note printed to the log before saving", layout="below", height=120)
    with_index = Parameter(bool, default=False, description="Write the row index as a column")

    class Inputs:
        data = Input("DataFrame", required=True, description="Data coming from upstream")

    def __init__(self):
        super().__init__()

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        df = inputs.get("data")
        file_path = params.get("file_path", "")
        if df is None or not file_path:
            return False

        file_format = params.get("file_format", "csv")
        with_index = params.get("with_index", False)
        note = params.get("note", "")
        if note:
            print(note)

        if file_format == "excel":
            df.to_excel(file_path, index=with_index)
            return True

        with open(file_path, "w", encoding="utf-8") as f:
            df.to_csv(f, index=with_index)
        return True
```

`file_path` 用字符串标签 `"file"`，`filter` 里的分组用两个分号隔开，面板上点浏览时弹窗里就出现这几组选项。`file_format` 的选项来自 `enum=[...]`，下拉框里是 `csv` 和 `excel` 两项，取到的值就是选中项的文本。`note` 是 `"below"` 布局加 120 像素高，面板上占满一行、可以写多行，内容非空时用 `print()` 打出来，输出写进 Python 日志文件 `%APPDATA%\DAWorkBench\log\da_pyscript.log`。`with_index` 是复选框，默认关闭。

`execute()` 里两个变量一起判断：输入缺失或者路径为空都返回 `False`。`file_format` 为 `excel` 时调用 `df.to_excel`，索引要不要写进去由 `with_index` 决定；其余情况（也就是 `csv`）用 `open()` 打开文件，交给 `df.to_csv` 写入。导出 Excel 需要环境里装有 `openpyxl`，缺少时 `to_excel` 会抛异常，节点被标成出错。

## 端口：Input 与 Output

端口声明的完整形式是 `Input(data_type, required=True, description="")` 和 `Output(data_type, description="")`。输入端口多一个 `required`，输出端口没有。

`data_type` 是描述性标签，取值习惯用 `"DataFrame"`、`"int"`、`"str"`、`"bool"`、`"any"` 这几种，写在端口旁边给读的人看。软件不按它做任何检查：连线时不比较两端的标签，执行时也不检查送进来的对象是不是这个类型。一个数值输出接到声明为 `DataFrame` 的输入上，照样能连上、能跑，错在节点自己的运算里才会暴露。`required` 同样不参与拦截，它只写进端口的描述信息。

一个输入端口可以接多条线，输出端口也可以接多条线。唯一被拒绝的操作是把同一对端口连两次。端口名就是属性名，它既是 `execute()` 里取数据的键，也是工程文件里记录连线的名字。

## 执行约定

框架执行一个节点时调用的是基类的 `run()`，节点自己写的 `execute()` 由它转交。`run()` 从 `_input_data` 里整理出 `inputs`，把所有参数当前值整理成 `params`，然后按签名决定调用方式：`execute(self, inputs=None, params=None)` 这类参数个数不小于 2 的写法按两个入参调用，`execute(self)` 这类无参写法直接调用。框架两种都认，新写的节点用前一种，输入和参数都能拿到。

返回值不参与数据传递。节点写出的数据只有一条通路，就是 `self._output_data["端口名"] = 值`。`return` 一个字典或者一个 DataFrame 都会被忽略，下游只会收到 `_output_data` 里的内容。

`return True` 表示成功，`return None` 也算成功，`return False` 或者抛出异常表示失败。失败节点在画布上变成出错颜色，它的下游不会被触发，执行器继续跑完其余不依赖它的节点，运行结束后没有任何重试。节点内部可以先做几项检查再返回 `False`，也可以在算出结果后统一返回；抛异常适合留给意外情况，比如列不存在、文件读不动。

## 外观：NodeDisplay 与 LinkPointStyle

`@NodeDef` 的 `style` 参数接受一个 `NodeDisplay`，用来改节点在画布上的样子。所有字段都可以不写，不写时用软件默认值。

| 字段 | 含义与取值 |
|---|---|
| `icon` | 图标路径，节点列表和节点主体共用 |
| `render_template` | 渲染方式，`"nodestyle"` 或 `"widget"` |
| `body_shape` | 主体形状，`"RoundedRect"`、`"Ellipse"`、`"Diamond"` |
| `name_position` | 名字位置，`"Inside"` 或 `"Below"` |
| `icon_position` | 图标位置，`"LeftOfText"` 或 `"AboveText"` |
| `background_color` | 背景色，`"#rrggbb"` 字符串或者 `(r, g, b)` 元组 |
| `border_color` | 边框色，写法同上 |
| `border_width` | 边框宽度 |
| `corner_radius` | 圆角半径，形状为圆角矩形时生效 |
| `icon_size` | 图标边长 |
| `input_port_side` / `output_port_side` | 端口方位，`"West"`、`"East"`、`"North"`、`"South"` |
| `input_port_style` / `output_port_style` | 端口样式，取一个 `LinkPointStyle` |
| `layout_strategy` | 端口排布方式，`"Auto"` 或 `"Manual"` |
| `body_icon_type` / `body_icon_source` / `body_icon_scale` | 主体里再放一张图，类型取 `"None"`、`"Pixmap"`、`"Svg"` |
| `min_body_width` / `min_body_height` | 主体最小尺寸，自己画图的节点用它预留空间 |

`LinkPointStyle(shape=None, fill_color=None, border_color=None, border_width=None)` 描述端口的方块、圆点或菱形。`shape` 取 `"Rect"`、`"Circle"`、`"Diamond"`，两个颜色字段接受 `"#rrggbb"` 字符串或 `(r, g, b)` 元组。

下面这个分支节点的外观和内置的条件分支节点同一套做法：菱形主体、蓝色系配色、菱形端口。存成 `branch.py`。

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, NodeDisplay, LinkPointStyle, Input, Output


@NodeDef(
    name="Branch",
    category="My Nodes",
    style=NodeDisplay(
        body_shape="Diamond",
        background_color="#E3F2FD",
        border_color="#2196F3",
        input_port_style=LinkPointStyle(shape="Diamond"),
        output_port_style=LinkPointStyle(shape="Diamond"),
    ),
)
class BranchNode:
    """Sends the input data to one of the two outputs."""

    class Inputs:
        data = Input("any", required=True, description="Data to route")

    class Outputs:
        left = Output("any", description="First branch")
        right = Output("any", description="Second branch")

    def __init__(self):
        super().__init__()

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        self._output_data["left"] = inputs.get("data")
        return True
```

装饰器里的 `style=` 只改画出来的样子，不影响执行。`body_shape="Diamond"` 把主体换成菱形，`background_color` 和 `border_color` 用十六进制字符串给出，两个菱形端口来自 `LinkPointStyle(shape="Diamond")`。节点把数据只写进 `left`，`right` 没有值，所以接到 `right` 上的下游不会被触发，条件分支的判断就是这么做的。

节点类里定义了 `paint()` 方法时，主体绘制完全交给它：背景色、边框、形状、图标、名字、参数行、状态色都不再由软件画上去，全部要在 `paint()` 里自己处理。端口仍然由软件按 `NodeDisplay` 里的端口方位和端口样式画出来，主体最小尺寸、节点列表里的图标也照旧生效。

## 可选扩展

### 跨保存保留运行期状态

`execute()` 里算出来、供以后查看的中间结果默认不随工程保存。重新打开工程后节点是空白的，要再跑一次才有内容。把它们留下来需要覆写两个方法，返回值和入参必须是能转成 JSON 的内容。

```python
    def __init__(self):
        super().__init__()
        self._changed_summary = ""

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        df = inputs.get("data")
        if df is None:
            return False

        column = params.get("column", "value")
        factor = params.get("factor", 1.0)
        scaled = df.copy()
        scaled[column] = scaled[column] * factor

        self._changed_summary = "changed %d values" % int((scaled[column] != df[column]).sum())
        self._output_data["scaled"] = scaled
        return True

    def serialize_runtime_state(self) -> dict:
        return {"changed_summary": getattr(self, "_changed_summary", "")}

    def deserialize_runtime_state(self, state: dict) -> None:
        self._changed_summary = state.get("changed_summary", "")
```

`serialize_runtime_state()` 在保存工程时被调用，返回的字典原样写进工程文件，这里放的是 `execute()` 算出的摘要。`deserialize_runtime_state(state)` 在加载工程时被调用，把存下来的字典读回实例属性，用 `state.get(键, 默认值)` 取值，旧工程里没有这一段时会拿到空字典。两个方法都不需要调用 `super()`，基类已经把调用转了过来。DataFrame、numpy 数组这类对象不能放进返回值，只能放字符串、数字、布尔、列表、字典和 `None`。用户能改的设置仍然用 `Parameter` 声明，这两个钩子只留给执行过程中衍生的缓存。

### 自己画节点主体

节点类里定义一个 `paint(self, painter, body_rect)` 方法，画布重绘时框架会自动调用它，不需要注册。`body_rect` 是四元组 `(x, y, w, h)`，给出主体在节点局部坐标系里的位置和尺寸，`painter` 是一个绘制代理，可以调用的方法见下表。

| 方法 | 作用 |
|---|---|
| `setNoPen()` / `setNoBrush()` | 关掉描边或者填充 |
| `setPenColor(r, g, b, a=255)` | 设置描边颜色 |
| `setPenWidth(width)` | 设置描边线宽 |
| `setBrushColor(r, g, b, a=255)` | 设置填充颜色 |
| `setFont(family, size, bold=False, italic=False)` | 设置字体 |
| `setFontFromDict(font_dict)` | 用参数字典设置字体 |
| `drawText(x, y, text)` | 在指定位置画一行文字 |
| `drawRect(x, y, w, h)` | 画矩形边框 |
| `fillRect(x, y, w, h, r, g, b, a=255)` | 用颜色填充矩形 |
| `drawLine(x1, y1, x2, y2)` | 画直线 |
| `drawEllipse(x, y, w, h)` | 画椭圆 |
| `drawDiamond(x, y, w, h)` | 画菱形 |
| `drawPolygon(points)` | 按点列画多边形 |
| `textBoundingRect(text, family, size, bold=False, italic=False)` | 量出一段文字的宽高 |
| `textBoundingRectWithFont(text, font_dict)` | 用参数字典量文字宽高 |
| `setClipRect(x, y, w, h)` / `clearClip()` | 限定绘制范围，之后清除 |

下面这个方法加到 `ScaleColumnNode` 里，节点主体就换成一整块自绘的背景、边框和一行列名。

```python
    def paint(self, painter, body_rect):
        x, y, w, h = body_rect
        column = getattr(self, "column", "value")

        painter.fillRect(x, y, w, h, 253, 250, 240, 255)
        painter.setNoBrush()
        painter.setPenColor(206, 96, 67, 255)
        painter.setPenWidth(1)
        painter.drawRect(x, y, w, h)

        painter.setFont("Microsoft YaHei", 9)
        painter.setPenColor(40, 40, 40, 255)
        painter.drawText(x + 8, y + 18, column)
```

`body_rect` 在方法第一行拆成四个数。`getattr(self, "column", "value")` 取属性时留一手：重绘可能发生在节点刚拖进画布、还没执行过的时刻，也可能发生在 `__init__` 里建的缓存属性缺失的时刻，直接写 `self.column` 在这类时刻会抛异常。接下来三个调用画底色、描边和线宽，再换字体和文字颜色，最后在左上角写出一行列名。方法里不要用 `_` 当局部变量名，它被文字翻译的 `_()` 函数占用，赋值之后同一个方法里的 `_("...")` 会报未定义。

绘制方法会被频繁调用，每次重绘都要跑一遍，一次绘制的时间要控制在 50 毫秒内。`paint()` 里抛异常时，节点不在空闲状态下会画上一圈 2 像素的红边，并在主体左上角显示粗体红字 `Error`。需要长时间计算的绘制结果适合在 `execute()` 里算好、存成实例属性，绘制时直接取。

### 全局节点

把 `is_global = True` 写成节点类的普通类属性，这个节点就变成全局节点，没有对应的装饰器选项。全局节点在整条工作流开始时先执行一遍，执行完不马上把输出送往下游；它同时是起始节点（没有输入端口、有输出端口）时，写出的数据会在随后进入正常流程时传下去。适合不依赖上游输入、但要抢在其它节点之前做准备的节点。

## 从节点里访问软件本身

节点执行时可以用 `da_app`、`da_data`、`da_interface` 这几个模块访问软件本身，它们是软件用 pybind11 导出的模块，只在软件里存在。`da_app` 提供 `getCore()` 和几条往消息面板写日志的函数，`da_interface` 提供数据管理接口、界面接口、命令接口等类型，`da_data` 提供数据集对象 `DAData`。同样的模块在[脚本工作区](../script/workspace.md)里也能导入，不装软件、单独跑 Python 时会导入失败。

导入要写在 `execute()` 里面，并且包在 `try` 里；写在文件顶部会让整个文件在没有宿主程序的环境里导入失败，节点注册跟着一起失败。

```python
    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        df = inputs.get("data")
        if df is None:
            return False

        try:
            import da_app
            import da_data
        except ImportError:
            return False

        core = da_app.getCore()
        data_mgr = core.getDataManagerInterface()
        name = params.get("data_name", "workflow_output")

        existing = data_mgr.findData(name, case_sensitive=True)
        if not existing.isNull():
            existing.setPyObject(df)
        else:
            data = da_data.DAData(df)
            data.setName(name)
            data_mgr.addData_(data)
        return True
```

`import da_app` 和 `import da_data` 放在 `try` 里，导入失败就返回 `False`，节点在纯 Python 环境下仍能被导入和注册。`da_app.getCore()` 拿到应用核心对象，`getDataManagerInterface()` 从里面取出数据管理接口，`findData(名称, case_sensitive=True)` 按名字精确查找已有的数据集。找到时 `setPyObject(df)` 就地替换底层数据，没找到时用 `da_data.DAData(df)` 新建一个数据集，`setName` 起名之后 `addData_` 加进数据管理面板。

节点在非主线程上执行时不能直接碰界面对象，要切回主线程再做。下面这几行是切换的固定写法，`core.getPythonSignalHandler().callInMainThread(...)` 把一个无参函数排进主线程执行。

```python
        import threading

        def publish():
            existing = data_mgr.findData(name, case_sensitive=True)
            if not existing.isNull():
                existing.setPyObject(df)
                return
            data = da_data.DAData(df)
            data.setName(name)
            data_mgr.addData_(data)

        if threading.current_thread() is threading.main_thread():
            publish()
        else:
            core.getPythonSignalHandler().callInMainThread(publish)
```

`import threading` 放在方法里，和其他模块导入一样避免顶层依赖。`publish` 里做的是上一段同样的发布动作，写成不带参数的函数之后才能整块交给主线程。`threading.current_thread() is threading.main_thread()` 判断当前是不是主线程，是就直接调用，不是就交给信号处理器转过去。内置的「Output to DataManager」节点用的就是这一套写法，它在替换已有数据集之后还会通知一次数据变更，让已经打开的表格视图刷新。

## 节点的文字与翻译

节点包里的文字分两类。`@NodeDef(name=...)` 保持英文，不翻译：它是节点列表和画布上的显示名，又不参与翻译，写成中文之后英文界面上显示的还是中文。节点在工程文件里的身份另有一份，叫 `qualified_name`，由模块名和类名拼成，软件按它找回节点类，所以类名和节点文件的名字才是不能随意改的部分。`category`、`description`、参数和端口的 `description`、以及 `paint()` 里画出来的文字都可以翻译，写法是 `_("English text")` 加一行 `# cn:中文` 注释，中文放进 `.po` 文件，源文件里只留英文。下面是 `ScaleColumnNode` 套上 `_()` 之后的样子，执行逻辑没有变化。

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(
    name="Scale Column",
    category=_("My Nodes"),  # cn:我的节点
    description=_("Multiplies one column of the input DataFrame by a factor."),  # cn:把输入 DataFrame 的某一列乘上一个系数。
)
class ScaleColumnNode:
    """Give one column of a DataFrame a scale factor."""

    column = Parameter(str, default="value", description=_("Name of the column to scale"))  # cn:要缩放的列名
    factor = Parameter(
        float,
        default=1.0,
        min=-100.0,
        max=100.0,
        step=0.1,
        decimals=3,
        description=_("Scale factor applied to the column"),  # cn:作用在这一列上的系数
    )
    count_changes = Parameter(
        bool,
        default=True,
        description=_("Also output the number of values that changed"),  # cn:同时输出发生变化的数值个数
        show_on_node=False,
    )

    class Inputs:
        data = Input("DataFrame", required=True, description=_("Data coming from upstream"))  # cn:上游送来的数据

    class Outputs:
        scaled = Output("DataFrame", description=_("DataFrame with the scaled column"))  # cn:缩放后的 DataFrame
        changed_count = Output("int", description=_("Number of values that changed"))  # cn:发生变化的数值个数

    def execute(self, inputs=None, params=None):
        if inputs is None:
            inputs = {}
        if params is None:
            params = {}

        df = inputs.get("data")
        if df is None:
            return False

        column = params.get("column", "value")
        factor = params.get("factor", 1.0)
        scaled = df.copy()
        scaled[column] = scaled[column] * factor
        self._output_data["scaled"] = scaled
        return True
```

`_("My Nodes")` 在模块导入时求值，把英文原文交给翻译系统；装着中文的 `.po` 编成 `.mo` 之后放在包内的 `i18n/locale/<语言>/LC_MESSAGES` 目录里，域名取包名。`name="Scale Column"` 没有包 `_()`，三个参数的说明和三个端口的说明都包了，`execute()` 里的代码没有显示到界面上的文字，保持英文。

`_()` 由包自己的初始化代码装进 Python 内置命名空间，所以包的 `__init__.py` 顶部必须先把翻译装好，再去导入各个节点模块。顺序反了的话，节点模块里 `@NodeDef(category=_(...))` 在导入时执行，`_()` 还没有来源，整个文件导入失败，这个文件里定义的节点一个都不会出现在列表里；失败不会弹窗，只在日志里留一条记录。`__init__.py` 的样子见[把节点打包成插件](./node-plugin.md)，那里也说明了 `i18n` 目录和 `setup.py` 怎么摆。

## 写完之后

节点文件写好之后并不会自动出现在软件里。软件在启动时从一个固定目录里找节点包，按包逐个导入 `.py` 文件，导入成功的类进入节点列表。把这个目录、包的内部结构、依赖声明和启用方式凑齐，节点才算装进软件，这部分在下一章。

## 常见问题

参数在属性面板里看不到。先看类型标签，`dict` 这类没有对应控件的类型会被面板跳过；再看参数声明是否写成了类属性，写在 `execute()` 里的局部变量不会被收集。

加载工程后执行报 `AttributeError: 'str' object has no attribute 'default'`。参数默认值写成了 `self.参数名.default`，加载工程时描述符被存下来的值覆盖了，改用 `params.get("参数名", 字面量)`。

节点执行后画布上变成红色。`execute()` 返回了 `False` 或者抛了异常。在 `execute()` 里用 `print()` 打出关键中间值，内容写进 Python 日志文件 `%APPDATA%\DAWorkBench\log\da_pyscript.log`，也可以先把检查条件放宽，确认输入数据本身对不对。

改了节点文件，软件里没有变化。节点包在软件启动时导入，运行中的实例不会重新读取文件，改完要重启软件。

节点能拖出来但连线接不上。端口没声明在 `Inputs` / `Outputs` 嵌套类里，或者属性名拼错了；端口名取自属性名，改名后旧工程里的连线也会对不上。

## 相关章节

- [工作流基础](./basics.md)：放节点、连接口、设参数、运行。
- [把节点打包成插件](./node-plugin.md)：让节点出现在软件里，并随软件分发。
- [数据分析节点手册](./nodes-analysis.md)：二十一个现成节点的参数，可以对照里面的实现。
- [脚本工作区](../script/workspace.md)：在软件里直接跑 Python，试 `da_app`、`da_data` 这些模块。
