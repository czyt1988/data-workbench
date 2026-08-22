# Python 节点开发指南

本文档详细介绍如何在 DAPyWorkFlow 中开发 Python 工作流节点，包括节点定义、端口声明、参数配置、执行逻辑编写以及节点注册与发现机制。

## 导航

本系列文档包含以下章节：

- [DAPyWorkFlow 模块概述](./workflow-overview.md)
- [插件与节点发现机制](./workflow-plugin-discovery.md)
- [Python 节点开发指南](./workflow-python-node-dev.md) ← 当前页
- [工作流生命周期](./workflow-lifecycle.md)
- [C++ 集成指南](./workflow-cpp-integration.md)
- [场景操作指南](./workflow-scene-operation.md)

## 概述

Python 节点开发是 DAPyWorkFlow 的核心开发方式。通过 Python 定义节点逻辑，无需编写 C++ 代码即可创建功能完整的工作流节点。

DAPyWorkFlow 采用 Python-first 设计理念：

- **节点定义**：使用 `@NodeDef` 装饰器在 Python 中声明节点类型
- **端口声明**：通过嵌套类 `Inputs` 和 `Outputs` 声明输入输出端口
- **参数配置**：使用 `Parameter` 类声明节点参数
- **执行逻辑**：在 `execute()` 方法中实现节点功能
- **自动发现**：支持目录扫描和 entry_points 双模式自动发现节点

## 节点定义基础

### @NodeDef 装饰器

`@NodeDef` 是节点定义的核心装饰器，用于声明工作流节点类型。

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter

@NodeDef(name="节点显示名称", category="节点分类", icon="图标标识")
class MyNode:
    """节点类文档字符串"""
    ...
```

#### 装饰器参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `name` | str | 是 | 节点显示名称，用于界面展示 |
| `category` | str | 否 | 节点所属分类，默认为空字符串 |
| `icon` | str | 否 | 节点图标标识，默认为空字符串 |
| `render_template` | str | 否 | 渲染模板类型，默认为 `"nodestyle"`，支持 `"nodestyle"`、`"rect"`、`"svg"`、`"widget"`（`rect`/`svg` 会被规范化为 `nodestyle`） |
| `style` | NodeDisplay/dict | 否 | 节点样式配置，支持 `NodeDisplay` 实例或 dict（自动转换为 `NodeDisplay`），默认为 `None` |
| `description` | str | 否 | 节点说明文本，显示在 tooltip 中。推荐用 `_("English") # cn:中文` 翻译。若为 `None` 则自动回退读取类 docstring（不翻译） |

!!! tip "render_template 参数"
    `render_template` 控制节点在工作流场景中的视觉呈现方式（`node_def.py:179-183` 的 `_normalize_render_template` 会把旧值归一化）：
    
    - `"nodestyle"`（默认）：使用 `NodeDisplay`/`DAPyNodeStyle` 配置绘制节点样式（支持 `body_shape`、`background_color` 等字段）
    - `"rect"` / `"svg"`：旧值，规范化后等价于 `"nodestyle"`
    - `"widget"`：自定义 QWidget 节点

### 完整节点类结构

一个完整的节点类包含以下部分：

```python
@NodeDef(name="Data Filter", category="Data Analysis", icon="filter")
class DataFilterNode:
    """数据筛选节点"""

    # 1. 参数声明（类属性）
    column = Parameter(str, default="value", description="筛选列名")
    threshold = Parameter(float, default=0.0, description="筛选阈值")

    # 2. 输入端口声明（嵌套类）
    class Inputs:
        data = Input("DataFrame", required=True, description="输入数据")

    # 3. 输出端口声明（嵌套类）
    class Outputs:
        filtered = Output("DataFrame", description="筛选后的数据")
        count = Output("int", description="符合条件的行数")

    # 4. 构造函数
    def __init__(self):
        self._input_data = {}   # 存储输入数据
        self._output_data = {}  # 存储输出数据

    # 5. 输入数据设置方法（可选）
    def set_input_data(self, channel, data):
        """设置输入端口数据"""
        self._input_data[channel] = data

    # 6. 执行方法
    def execute(self, inputs=None, params=None):
        """执行节点逻辑"""
        # 读取输入数据
        df = self._input_data.get("data")
        # 或：df = inputs.get("data") if inputs else None

        # 获取参数值
        column = params.get("column", self.column.default) if params else self.column.default
        threshold = params.get("threshold", self.threshold.default) if params else self.threshold.default

        # 执行业务逻辑
        result = df[df[column] > threshold]

        # 设置输出数据
        self._output_data["filtered"] = result
        self._output_data["count"] = len(result)

        # 返回执行状态
        return True
```

!!! note "节点类结构说明"
    节点类必须包含：
    
    1. `@NodeDef` 装饰器
    2. `Inputs` 嵌套类（如有输入端口）
    3. `Outputs` 嵌套类（如有输出端口）
    4. `__init__` 方法（初始化 `_input_data` 和 `_output_data`）
    5. `execute` 方法（节点执行逻辑）

## 输入与输出

### Input 类

`Input` 类用于声明节点的输入端口。

```python
class Input:
    def __init__(self, data_type: str, required: bool = True, description: str = "")
```

#### 参数说明

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| `data_type` | str | 是 | - | 数据类型标签，如 `"DataFrame"`、`"int"`、`"str"` 等 |
| `required` | bool | 否 | `True` | 是否为必填输入 |
| `description` | str | 否 | `""` | 输入端口描述信息 |

#### 使用示例

```python
class Inputs:
    # 必填输入
    data = Input("DataFrame", required=True, description="输入 DataFrame")
    
    # 可选输入
    config = Input("dict", required=False, description="配置信息")
    
    # 多输入端口
    primary = Input("DataFrame", required=True, description="主数据")
    secondary = Input("DataFrame", required=False, description="辅助数据")
```

### Output 类

`Output` 类用于声明节点的输出端口。

```python
class Output:
    def __init__(self, data_type: str, description: str = "")
```

#### 参数说明

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| `data_type` | str | 是 | - | 数据类型标签，如 `"DataFrame"`、`"int"`、`"str"` 等 |
| `description` | str | 否 | `""` | 输出端口描述信息 |

#### 使用示例

```python
class Outputs:
    # 主要输出
    result = Output("DataFrame", description="处理后的数据")
    
    # 统计输出
    row_count = Output("int", description="数据行数")
    
    # 报告输出
    report = Output("str", description="处理报告")
```

### 嵌套类声明模式

!!! warning "必须使用嵌套类模式"
    DAPyWorkFlow 要求使用 `class Inputs:` 和 `class Outputs:` 嵌套类声明端口。这种模式下，端口通过类属性声明，代码结构清晰，便于 IDE 自动补全。

```python
@NodeDef(name="示例节点", category="示例")
class ExampleNode:
    """嵌套类模式示例"""

    class Inputs:
        """输入端口声明"""
        data = Input("DataFrame", required=True, description="输入数据")
        config = Input("dict", required=False, description="配置信息")

    class Outputs:
        """输出端口声明"""
        result = Output("DataFrame", description="处理结果")
        count = Output("int", description="数据行数")
```

## 参数系统

### Parameter 类

`Parameter` 类用于声明节点的可配置参数。

```python
# src/PyScripts/DAWorkbench/DAWorkFlowPy/types.py:140-167
class Parameter:
    def __init__(
        self,
        param_type,
        default=None,
        description: str = "",
        min=None,
        max=None,
        step=None,
        decimals=None,
        enum=None,
        filter=None,
        layout: str = "inline",
        height=None,
        **kwargs,
    )
```

#### 参数说明

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| `param_type` | type/str | 是 | - | 参数类型，如 `str`、`int`、`float`、`bool`、`list`、`dict`，或字符串标签 `"file"/"folder"/"enum"/"color"/"font"/"code"` |
| `default` | any | 否 | `None` | 参数默认值 |
| `description` | str | 否 | `""` | 参数描述信息 |
| `min` | any | 否 | `None` | 数值最小值，写入扩展属性 |
| `max` | any | 否 | `None` | 数值最大值 |
| `step` | any | 否 | `None` | 数值步进 |
| `decimals` | any | 否 | `None` | 浮点小数位 |
| `enum` | any | 否 | `None` | 枚举可选项 |
| `filter` | str | 否 | `None` | `file`/`folder` 类型的文件过滤器 |
| `layout` | str | 否 | `"inline"` | 编辑器布局：`"inline"`（属性名在左、编辑器在右）或 `"below"`（属性名在上、编辑器占满整行下方，`str` 自动切多行 `QPlainTextEdit`） |
| `height` | int | 否 | `None` | 编辑器高度（像素），仅 `below` 模式生效 |
| `**kwargs` | - | 否 | - | 扩展字段，键名需与 C++ `DANodeParameterFormAdapter` 读取的 attributes 键一致 |

#### 支持的参数类型

`_TYPE_LABELS`（`types.py:121-135`）支持的类型标签：

| 类型 | 标签 | 说明 |
|------|------|------|
| `str` | `"str"` | 字符串 |
| `int` | `"int"` | 整数 |
| `float` | `"float"` | 浮点数 |
| `bool` | `"bool"` | 布尔值 |
| `list` | `"list"` | 列表 |
| `dict` | `"dict"` | 字典 |
| `"file"` | `"file"` | 文件路径（由 C++ 端 `DAFormEditorRegistry` 渲染为 `DAFilePathEditWidget`） |
| `"folder"` | `"folder"` | 文件夹路径 |
| `"enum"` | `"enum"` | 枚举下拉 |
| `"color"` | `"color"` | 颜色选择 |
| `"font"` | `"font"` | 字体选择 |
| `"code"` | `"code"` | 多行代码编辑（`QPlainTextEdit`） |

!!! info "C++ 端扩展类型支持"
    Python `Parameter` 通过 `param_type` 传 Python 内置类型（6 种）或字符串标签（再 6 种扩展类型）。扩展类型由 C++ 端的 `DAFormEditorRegistry` 渲染对应控件：`str→QLineEdit, int→QSpinBox, float→QDoubleSpinBox, bool→QCheckBox, enum→QComboBox, list→QListWidget, file→DAFilePathEditWidget, folder→DAFilePathEditWidget(dir), color→DAColorPickerButton, font→DAFontEditPannelWidget, code→QPlainTextEdit`。当 Python 节点的参数通过 `DANodeParameterFormAdapter` 转换为 `DAFormSpec` 后，`DAFormEditorRegistry` 根据 `type` 字段创建对应的编辑器控件。参见 [创建属性设置面板](./creating-setting-panel.md) 和 `src/DACommonWidgets/DAFormEditorRegistry.h`。

#### 使用示例

```python
@NodeDef(name="数据筛选", category="数据处理")
class DataFilterNode:
    """参数声明示例"""

    # 字符串参数
    column = Parameter(str, default="value", description="目标列名")
    
    # 数值参数
    threshold = Parameter(float, default=0.0, description="筛选阈值")
    
    # 布尔参数
    drop_na = Parameter(bool, default=True, description="是否删除缺失值")
    
    # 整数参数
    max_rows = Parameter(int, default=1000, description="最大行数")
```

### 参数访问方式

在 `execute()` 方法中，可以通过以下方式访问参数值：

```python
def execute(self, inputs=None, params=None):
    # 方式1：从 params 字典获取（推荐）
    if params:
        column = params.get("column", self.column.default)
        threshold = params.get("threshold", self.threshold.default)
    else:
        column = self.column.default
        threshold = self.threshold.default
    
    # 方式2：直接访问 Parameter 实例的 default 属性
    column = self.column.default
    threshold = self.threshold.default
```

!!! tip "参数访问建议"
    建议优先使用 `params` 字典获取参数值，因为 `params` 包含用户在工作流界面中设置的实时值。`self.column.default` 仅作为回退值使用。

## 节点执行

### execute() 方法

`execute()` 是节点的核心执行方法，在工作流执行时被调用。

#### 方法签名

```python
def execute(self, inputs=None, params=None):
    """
    执行节点逻辑

    :param inputs: 输入数据字典，键为输入端口名称，值为输入数据
    :param params: 参数字典，键为参数名称，值为参数值
    :return: bool，True 表示执行成功，False 表示执行失败
    """
```

#### 读取输入数据

```python
def execute(self, inputs=None, params=None):
    # 方式1：从 self._input_data 获取（通过 set_input_data 设置）
    df = self._input_data.get("data")
    
    # 方式2：从 inputs 参数获取
    if inputs:
        df = inputs.get("data")
    else:
        df = self._input_data.get("data")
```

#### 写入输出数据

```python
def execute(self, inputs=None, params=None):
    # 处理逻辑...
    result = process_data(df)
    
    # 设置输出数据到 _output_data
    self._output_data["result"] = result
    self._output_data["count"] = len(result)
    
    # 返回执行状态
    return True
```

#### 返回值说明

| 返回值 | 含义 | 行为 |
|--------|------|------|
| `True` | 执行成功 | 输出数据传播到下游节点 |
| `False` | 执行失败 | 停止当前分支的数据传播 |

### set_input_data() 方法

`set_input_data()` 方法用于接收上游节点的输出数据。

```python
def set_input_data(self, channel, data):
    """
    设置输入端口数据

    :param channel: 输入端口名称
    :param data: 输入数据
    """
    self._input_data[channel] = data
```

!!! note "set_input_data 调用时机"
    `set_input_data()` 由工作流引擎在执行节点前自动调用。上游节点的输出数据通过此方法传递到当前节点的 `_input_data` 字典中。

### _input_data 和 _output_data

这两个实例属性分别用于存储输入和输出数据：

```python
def __init__(self):
    self._input_data = {}   # 存储输入数据，键为端口名，值为数据
    self._output_data = {}  # 存储输出数据，键为端口名，值为数据
```

| 属性 | 用途 | 数据流向 |
|------|------|----------|
| `_input_data` | 存储上游节点传递的数据 | 上游 → 当前节点 |
| `_output_data` | 存储当前节点生成的数据 | 当前节点 → 下游 |

## 实战示例

### DataSourceNode — 数据源节点

从 CSV 文件读取数据，无输入端口，有输出端口。

```python
# -*- coding: utf-8 -*-
"""
数据源节点 — 从 CSV 文件读取数据
"""

import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Source", category="Data Analysis", icon="data_source")
class DataSourceNode:
    """CSV 数据读取节点"""

    file_path = Parameter(str, default="", description="CSV 文件路径")
    encoding = Parameter(str, default="utf-8", description="文件编码")
    separator = Parameter(str, default=",", description="字段分隔符")

    class Outputs:
        data = Output("DataFrame", description="读取的 DataFrame 数据")
        row_count = Output("int", description="数据行数")

    def __init__(self):
        self._output_data = {}

    def execute(self, inputs=None, params=None):
        """执行 CSV 数据读取"""
        if params is None:
            params = {}
        file_path = params.get("file_path", self.file_path.default)
        encoding = params.get("encoding", self.encoding.default)
        sep = params.get("separator", self.separator.default)

        if not file_path:
            return False

        try:
            df = pd.read_csv(file_path, encoding=encoding, sep=sep)
            self._output_data["data"] = df
            self._output_data["row_count"] = len(df)
            return True
        except Exception:
            self._output_data["data"] = None
            self._output_data["row_count"] = 0
            return False
```

### DataFilterNode — 数据筛选节点

按条件筛选 DataFrame 行，展示输入端口和条件执行。

```python
# -*- coding: utf-8 -*-
"""
数据筛选节点 — 按条件筛选 DataFrame 行
"""

import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Filter", category="Data Analysis", icon="data_filter")
class DataFilterNode:
    """条件筛选节点"""

    column = Parameter(str, default="", description="筛选目标列名")
    condition = Parameter(str, default="", description="筛选条件表达式（df.query 语法）")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入 DataFrame")

    class Outputs:
        filtered = Output("DataFrame", description="筛选后的 DataFrame")
        removed_count = Output("int", description="移除的行数")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"filtered": None, "removed_count": 0}

    def set_input_data(self, channel, data):
        """设置输入端口数据"""
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """执行条件筛选"""
        if params is None:
            params = {}
        condition = params.get("condition", self.condition.default)
        column = params.get("column", self.column.default)

        df = self._input_data.get("data")
        if df is None:
            return False

        try:
            original_len = len(df)
            if condition:
                filtered_df = df.query(condition)
            elif column:
                # 无条件表达式时，筛选指定列的非空值
                filtered_df = df[df[column].notna()]
            else:
                filtered_df = df

            self._output_data["filtered"] = filtered_df
            self._output_data["removed_count"] = original_len - len(filtered_df)
            return True
        except Exception:
            self._output_data["filtered"] = df
            self._output_data["removed_count"] = 0
            return False
```

### DataTransformNode — 数据变换节点

对 DataFrame 列进行变换操作，展示多参数和多操作类型。

```python
# -*- coding: utf-8 -*-
"""
数据变换节点 — 对 DataFrame 列进行变换操作
"""

import pandas as pd
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Transform", category="Data Analysis", icon="data_transform")
class DataTransformNode:
    """列变换节点"""

    column = Parameter(str, default="", description="操作目标列名")
    operation = Parameter(str, default="rename",
                          description="操作类型：rename/drop/fillna")
    new_name = Parameter(str, default="", description="rename 操作的新列名")
    fill_value = Parameter(str, default="0", description="fillna 操作的填充值")

    class Inputs:
        data = Input("DataFrame", required=True, description="输入 DataFrame")

    class Outputs:
        transformed = Output("DataFrame", description="变换后的 DataFrame")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"transformed": None}

    def set_input_data(self, channel, data):
        """设置输入端口数据"""
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """执行列变换操作"""
        if params is None:
            params = {}
        column = params.get("column", self.column.default)
        operation = params.get("operation", self.operation.default)
        new_name = params.get("new_name", self.new_name.default)
        fill_value = params.get("fill_value", self.fill_value.default)

        df = self._input_data.get("data")
        if df is None:
            return False

        if not column:
            self._output_data["transformed"] = df
            return True

        try:
            result = df.copy()

            if operation == "rename":
                if new_name:
                    result = result.rename(columns={column: new_name})
            elif operation == "drop":
                result = result.drop(columns=[column])
            elif operation == "fillna":
                # 尝试将 fill_value 转换为数值类型
                try:
                    numeric_fill = float(fill_value)
                    if numeric_fill == int(numeric_fill):
                        numeric_fill = int(numeric_fill)
                    result[column] = result[column].fillna(numeric_fill)
                except ValueError:
                    result[column] = result[column].fillna(fill_value)

            self._output_data["transformed"] = result
            return True
        except Exception:
            self._output_data["transformed"] = df
            return False
```

### AgentNode — AI Agent 节点

展示与外部库集成和状态推送机制。

```python
# -*- coding: utf-8 -*-
"""
AI Agent 智能体节点

定义 CrewAI Agent 的角色、目标和背景故事，
在工作流中作为智能体的配置和启动节点。
"""

from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Agent", category="AI Agent", icon="agent")
class AgentNode:
    """AI Agent 智能体节点"""

    role = Parameter(str, default="分析师", description="智能体角色定义")
    goal = Parameter(str, default="分析数据并提取洞察", description="智能体目标")
    backstory = Parameter(str, default="你是一位资深数据分析师", description="智能体背景故事")

    class Inputs:
        tools = Input("Tool", required=False, description="Agent 可使用的工具列表")
        task = Input("Task", required=False, description="分配给 Agent 的任务")

    class Outputs:
        agent = Output("Agent", description="创建的 CrewAI Agent 实例")
        result = Output("String", description="Agent 执行任务的文本结果")

    def __init__(self):
        self._input_data = {}
        self._output_data = {"agent": None, "result": None}

    def set_input_data(self, channel, data):
        """设置输入端口数据"""
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """创建并执行 CrewAI Agent"""
        if params is None:
            params = {}
        if inputs is None:
            inputs = self._input_data

        role = params.get("role", "分析师")
        goal = params.get("goal", "分析数据并提取洞察")
        backstory = params.get("backstory", "你是一位资深数据分析师")

        tools = inputs.get("tools", self._input_data.get("tools", []))
        task = inputs.get("task", self._input_data.get("task", None))

        try:
            from crewai import Agent
        except ImportError:
            # crewai 未安装，以占位模式运行
            self._output_data["agent"] = {
                "role": role,
                "goal": goal,
                "backstory": backstory,
                "_placeholder": True,
            }
            self._output_data["result"] = f"[占位模式] Agent({role}) 完成了目标: {goal}"
            self._push_state("thinking")
            self._push_state("done")
            return True

        # 创建 CrewAI Agent
        agent_kwargs = {
            "role": role,
            "goal": goal,
            "backstory": backstory,
        }
        if tools:
            agent_kwargs["tools"] = tools

        self._push_state("thinking")

        agent = Agent(**agent_kwargs)
        self._output_data["agent"] = agent

        # 若有任务，执行任务
        if task is not None:
            try:
                from crewai import Task
                if isinstance(task, Task):
                    task.agent = agent
                    result = task.execute()
                    self._output_data["result"] = result
            except Exception as e:
                self._output_data["result"] = f"Agent 执行任务失败: {e}"

        self._push_state("done")
        return True

    def _push_state(self, state):
        """推送节点状态变更通知"""
        try:
            import da_app
            # cn:真实跨线程 API 是 core.getPythonSignalHandler().callInMainThread(func)
            #    （绑定在 da_interface.DAPythonSignalHandler 上，见 DAPyWorkFlowPythonBinding.cpp:178）
            #    不存在 DAWorkbench.da_interface.call_in_main_thread(...) 这种字符串分发的自由函数
            handler = da_app.getCore().getPythonSignalHandler()
            qn = self.qualified_name
            handler.callInMainThread(lambda: self._emit_state_change(qn, state))
        except (ImportError, AttributeError):
            pass
```

## 节点注册与发现

### DANodeRegistry

`DANodeRegistry` 是节点类型的注册中心，负责节点的发现和查询。

```python
from DAWorkbench.DAWorkFlowPy import DANodeRegistry

# 创建注册表实例
registry = DANodeRegistry()
```

#### 发现节点

```python
# 双模式发现：目录扫描 + entry_points
descriptors = registry.discover(
    scan_paths=["/path/to/plugins"],
    use_entry_points=True
)

# 仅目录扫描
descriptors = registry.discover(scan_paths=["/path/to/plugins"])

# 仅 entry_points 发现
descriptors = registry.discover(use_entry_points=True)
```

#### 获取节点描述符

```python
# 获取所有已注册节点的描述符
all_descriptors = registry.get_all_descriptors()

# 根据 qualifiedName 获取指定节点的描述符
descriptor = registry.get_descriptor("my_module.DataFilter")
```

### 目录扫描模式

目录扫描模式遍历指定路径下的 `.py` 文件，动态导入模块并查找带有 `@NodeDef` 装饰器的类。

```python
registry = DANodeRegistry()
descriptors = registry.discover(scan_paths=[
    "/path/to/plugins/DataAnalysis",
    "/path/to/plugins/DASystemNodes"
])
```

!!! note "目录扫描规则"
    - 扫描路径下的所有 `.py` 文件（排除 `__pycache__` 和 `__init__.py`）
    - 动态导入模块并检查类是否带有 `qualified_name` 属性（由 `@NodeDef` 装饰器设置）
    - 相同 `qualified_name` 的节点只注册一次（自动去重）

### entry_points 注册

通过 `setuptools` 的 `entry_points` 机制注册节点，使节点可以通过 `importlib.metadata` 自动发现。

#### setup.py 配置

```python
# -*- coding: utf-8 -*-
"""
DADataAnalysisPy 安装配置
"""

from setuptools import setup, find_packages

setup(
    name="DADataAnalysisPy",
    version="1.0.0",
    description="数据分析示例节点包",
    author="DA WorkBench Team",
    packages=find_packages(),
    python_requires=">=3.7",
    install_requires=[
        "pandas",
    ],
    entry_points={
        "data_workbench.plugin": [
            "DADataAnalysisPy = DADataAnalysisPy",
        ],
    },
)
```

!!! tip "entry_points 分组"
    必须使用 `data_workbench.plugin` 作为分组名称，DANodeRegistry 通过此分组查找已安装的插件包。

#### qualified_name 生成规则

节点的唯一标识 `qualified_name` 自动生成：

```
qualified_name = f"{cls.__module__}.{cls.__name__}"
```

例如：
- 模块 `DADataAnalysisPy.data_filter_node` 中的类 `DataFilterNode`
- qualifiedName: `DADataAnalysisPy.data_filter_node.DataFilterNode`

## 高级特性

### is_global 属性

全局节点是一种特殊节点，执行后不向下游传播数据，常用于配置节点或副作用节点。

```python
@NodeDef(name="全局配置", category="配置")
class GlobalConfigNode:
    """全局配置节点"""
    
    # 标记为全局节点
    is_global = True
    
    config_path = Parameter(str, default="", description="配置文件路径")
    
    def execute(self, inputs=None, params=None):
        """执行全局配置加载"""
        # 全局节点执行后不传播数据
        # 但可以通过其他机制（如全局变量）影响工作流
        return True
```

### render_template 和 style 参数

`render_template` 和 `style` 控制节点的视觉呈现方式：

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, NodeDisplay, LinkPointStyle

# 标准样式节点（默认，使用 NodeDisplay 配置样式）
@NodeDef(name="标准节点", category="基础", render_template="nodestyle")
class StandardNode:
    pass

# 使用 NodeDisplay 自定义样式
@NodeDef(name="椭圆节点", category="基础", style=NodeDisplay(
    body_shape="Ellipse",
    background_color="#4A90D9",
    corner_radius=8.0,
    input_port_style=LinkPointStyle(shape="Circle", fill_color="#ff0000"),
))
class EllipseNode:
    pass

# 自定义 QWidget 节点
@NodeDef(name="Widget 节点", category="基础", render_template="widget")
class WidgetNode:
    pass
```

### DAPythonSignalHandler.callInMainThread

在后台线程中需要操作 UI 时，使用 `callInMainThread` 将操作投递到主线程执行。`DAPythonSignalHandler` 位于 `src/DAPyBindQt/DAPythonSignalHandler.h`。

```python
def _push_state(self, state):
    """推送节点状态变更通知"""
    try:
        import da_app
        # cn:真实跨线程 API：core.getPythonSignalHandler().callInMainThread(func)
        #    func 是一个无参可调用对象，将在 Qt 主线程执行
        #    不存在 DAWorkbench.da_interface.call_in_main_thread(...) 字符串分发的自由函数
        handler = da_app.getCore().getPythonSignalHandler()
        qn = self.qualified_name
        handler.callInMainThread(lambda: self._emit_state_change(qn, state))
    except (ImportError, AttributeError):
        # 在纯 Python 测试环境中，da_app 不可用
        pass
```

!!! warning "线程安全"
    `callInMainThread` 在 C++ 绑定层已正确处理 GIL 管理，Python 脚本无需额外处理 GIL。但需注意回调函数不要在后台线程直接操作 UI。传入的 `func` 必须是**无参**可调用对象（lambda 或函数）；如需携带参数，请通过 lambda 闭包捕获（如上例 `qn`/`state`），不要试图向 `callInMainThread` 传额外位置参数。

### 运行时状态持久化（serialize_runtime_state / deserialize_runtime_state）

`execute()` 阶段产生的衍生状态（如缓存文本、中间摘要），默认在工程保存/加载时丢失——重新打开工程后节点画面会变回空白，必须再次执行才能看到内容。

`DAWorkflowNode` 基类提供一对钩子解决这个问题：

| 钩子 | 调用时机 | 默认行为 |
|------|----------|----------|
| `serialize_runtime_state(self) -> dict` | 工程保存时 | 返回 `{}`（不持久化） |
| `deserialize_runtime_state(self, state: dict) -> None` | 工程加载后 | 空实现 |

`DAWorkflowSerializer` 在 `to_dict` / `to_xml_element` 中调用 `serialize_runtime_state()`，把返回的 dict 写入工程文件；在 `from_dict` / `from_xml_element` 中调用 `deserialize_runtime_state()` 恢复状态。

#### 使用场景

| 场景 | 是否需要钩子 |
|------|--------------|
| 用户可配置的参数 | ❌ 用 `Parameter` 声明 |
| 节点执行后缓存的显示文本、摘要 | ✅ 用 `serialize_runtime_state` |
| 最近一次执行的日志/错误信息 | ✅ 用 `serialize_runtime_state` |
| DataFrame 本身 | ❌ 应存入 `_output_data` 或 DataManager |
| Python 对象引用、文件句柄 | ❌ 不可序列化 |

#### 实现示例

```python
@NodeDef(name="Text Viewer", category="System / Display")
class TextViewerNode:
    def __init__(self):
        super().__init__()
        self._display_text = ""

    def execute(self, inputs=None, params=None):
        value = (inputs or {}).get("value")
        self._display_text = str(value) if value is not None else ""
        return True

    def serialize_runtime_state(self) -> dict:
        # 把缓存的显示文本写入工程文件
        return {"display_text": getattr(self, "_display_text", "")}

    def deserialize_runtime_state(self, state: dict) -> None:
        # 工程重新加载后恢复缓存，无需 execute() 即可在 paint() 中渲染
        self._display_text = state.get("display_text", "")
```

#### XML 存储格式

运行时状态存储在 `<node>` 元素下的 `<state>` 子元素中，每个键值对一个 `<item>`：

```xml
<node node_id="DASystemNodes.TextViewer_1" qualified_name="DASystemNodes.TextViewer">
  <param name="font_color" type="str">#282828</param>
  <param name="font_size" type="int">9</param>
  <state>
    <item name="display_text" type="str">Hello World</item>
  </state>
</node>
```

类型标签复用 `Parameter` 序列化的 type 标签（`str/int/float/bool/json/none`），`None` 值会被跳过不写入。

!!! warning "使用约束"

    1. **仅存 JSON 可序列化类型**：`str/int/float/bool/list/dict/None`。DataFrame、numpy 数组、Python 对象引用等**不可**直接放入返回值。
    2. **不替代 Parameter**：用户可配置的参数必须通过 `Parameter` 声明。
    3. **防御性读取**：`deserialize_runtime_state()` 收到的 dict 可能为空（旧工程文件无此字段），必须用 `state.get(key, default)` 安全访问。
    4. **异常隔离**：序列化器在调用钩子时用 `try/except` 包裹，单个节点的钩子失败不会中断整体保存/加载流程，但会丢失该节点的运行时状态。

#### 向后兼容

旧工程文件没有 `<state>` 元素，序列化器会跳过 `deserialize_runtime_state()` 调用，节点保持 `__init__` 中设置的默认状态，行为与升级前一致。

详见 [项目序列化架构 - 运行时状态持久化](./project-serialization-architecture.md#8-运行时状态持久化)。

#### ⚠️ @NodeDef 的 MRO 遮盖陷阱

`@NodeDef` 装饰器通过 `type(cls.__name__, (DAWorkflowNode, cls), {})` 创建新类，MRO 为：

```
new_cls → DAWorkflowNode → 用户类 cls → object
```

这意味着 **`DAWorkflowNode` 基类的方法会遮盖用户类的同名方法**。例如，如果 `DAWorkflowNode` 定义了 `def foo(self): return "base"`，而用户类也定义了 `def foo(self): return "user"`，调用 `node.foo()` 会返回 `"base"` 而不是 `"user"`。

`DAWorkflowNode` 中需要被子类覆写的方法（如 `serialize_runtime_state` / `deserialize_runtime_state`）通过 `super()` 转发到用户类的实现：

```python
# DAWorkflowNode 基类中的实现
def serialize_runtime_state(self) -> dict:
    try:
        return super().serialize_runtime_state() or {}
    except AttributeError:
        return {}  # 用户类未覆写，返回默认空 dict
```

**用户类正常覆写即可，无需调用 `super()`**：

```python
# TextViewerNode 中的覆写 — 直接返回，不需要 super()
def serialize_runtime_state(self) -> dict:
    return {"display_text": self._display_text}
```

!!! warning "新增 DAWorkflowNode 基类方法的注意事项"

    如果在 `DAWorkflowNode` 基类中新增需要被子类覆写的方法，**必须**使用 `super()` 转发模式，否则用户类的覆写会被基类遮盖。`__init__` 方法不受此影响，因为它通过 `super().__init__()` 链式调用。

## 注意事项

### _input_data 和 _output_data 使用规范

| 场景 | 推荐做法 |
|------|----------|
| 读取输入数据 | 优先使用 `self._input_data.get("port_name")`，或使用 `inputs` 参数 |
| 写入输出数据 | 必须写入 `self._output_data`，键为输出端口名称 |
| 初始化 | 在 `__init__` 中初始化 `_input_data = {}` 和 `_output_data = {}` |

### 线程安全

- DAPyWorkFlow 使用 Python GIL 保证线程安全
- 长时间运行的节点应定期释放 GIL（通过 C++ 层调用）
- 后台线程操作 UI 必须使用 `callInMainThread`

### Python 异常处理

```python
def execute(self, inputs=None, params=None):
    try:
        # 业务逻辑
        result = process_data(df)
        self._output_data["result"] = result
        return True
    except Exception as e:
        # 记录错误日志
        self._output_data["result"] = None
        self._output_data["error"] = str(e)
        return False
```

!!! tip "异常处理建议"
    - 始终捕获异常并返回 `False` 表示执行失败
    - 在 `_output_data` 中设置错误信息便于调试
    - 避免抛出未捕获的异常导致工作流引擎崩溃

## 参考资料

- Python 模块源码：`src/PyScripts/DAWorkbench/DAWorkFlowPy/`
  - `__init__.py` — 模块导出：`DAWorkflowNode`、`NodeDisplay`、`LinkPointStyle` 等
  - `workflow.py` — `DAWorkflow` 工作流编排与节点拓扑管理
  - `executor.py` — `DAWorkflowExecutor` 执行引擎（节点调度、数据流转）
  - `connection.py` — 节点连接管理（端口连接关系建模）
  - `signal_manager.py` — 信号管理器（节点状态变更等信号协调）
  - `node_def.py` — `@NodeDef` 装饰器实现、`DAWorkflowNode` 基类、`NodeDisplay`、`LinkPointStyle`
  - `types.py` — `Input`、`Output`、`Parameter` 类定义
  - `node_registry.py` — `DANodeRegistry` 类定义
  - `node_factory.py` — `DANodeFactory` 节点工厂（C++ 调用入口）
  - `serializer.py` — `DAWorkflowSerializer` 工作流序列化/反序列化
  - `syntax.py` — `NodeProxy`、`NodeOutputProxy`、`NodeInputProxy`（链式连接语法糖）
- 节点示例：`plugins/` 目录下的 Python 插件
  - `plugins/DataAnalysis/PyScripts/DADataAnalysisNodes/` — 数据分析节点
  - `plugins/DASystemNodes/PyScripts/DASystemNodes/` — 系统内置节点（流程控制/数据展示）
- 相关文档
  - [DAPyWorkFlow 模块概述](./workflow-overview.md)
  - [工作流生命周期](./workflow-lifecycle.md)
  - [C++ 集成指南](./workflow-cpp-integration.md)
  - [场景操作指南](./workflow-scene-operation.md)
