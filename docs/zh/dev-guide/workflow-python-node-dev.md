# Python 节点开发指南

本文档详细介绍如何在 DAPyWorkFlow 中开发 Python 工作流节点，包括节点定义、端口声明、参数配置、执行逻辑编写以及节点注册与发现机制。

## 导航

本系列文档包含以下章节：

- [DAPyWorkFlow 模块概述](./workflow-overview.md)
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
| `render_template` | str | 否 | 渲染模板类型，默认为 `"rect"`，支持 `"rect"`、`"svg"`、`"widget"` |

!!! tip "render_template 参数"
    `render_template` 控制节点在工作流场景中的视觉呈现方式：
    
    - `"rect"`（默认）：标准矩形节点
    - `"svg"`：SVG 矢量图形节点
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
class Parameter:
    def __init__(self, param_type: type, default=None, description: str = "")
```

#### 参数说明

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| `param_type` | type | 是 | - | 参数类型，如 `str`、`int`、`float`、`bool`、`list`、`dict` |
| `default` | any | 否 | `None` | 参数默认值 |
| `description` | str | 否 | `""` | 参数描述信息 |

#### 支持的参数类型

| Python 类型 | 类型标签 | 说明 |
|-------------|----------|------|
| `str` | `"str"` | 字符串 |
| `int` | `"int"` | 整数 |
| `float` | `"float"` | 浮点数 |
| `bool` | `"bool"` | 布尔值 |
| `list` | `"list"` | 列表 |
| `dict` | `"dict"` | 字典 |

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
            import DAWorkbench
            DAWorkbench.da_interface.call_in_main_thread(
                "node_state_change",
                self._node_descriptor["qualified_name"],
                state,
            )
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

# 根据 qualified_name 获取指定节点的描述符
descriptor = registry.get_descriptor("my_module.DataFilter")
```

### 目录扫描模式

目录扫描模式遍历指定路径下的 `.py` 文件，动态导入模块并查找带有 `@NodeDef` 装饰器的类。

```python
registry = DANodeRegistry()
descriptors = registry.discover(scan_paths=[
    "/path/to/plugins/DataAnalysis",
    "/path/to/plugins/CrewAIAdapter"
])
```

!!! note "目录扫描规则"
    - 扫描路径下的所有 `.py` 文件（排除 `__pycache__` 和 `__init__.py`）
    - 动态导入模块并检查类是否带有 `_node_descriptor` 属性
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
qualified_name = f"{cls.__module__}.{cls.__qualname__}"
```

例如：
- 模块 `DADataAnalysisPy.data_filter_node` 中的类 `DataFilterNode`
- qualified_name: `DADataAnalysisPy.data_filter_node.DataFilterNode`

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

### render_template 参数

`render_template` 控制节点的视觉呈现方式：

```python
# 标准矩形节点（默认）
@NodeDef(name="标准节点", category="基础", render_template="rect")
class StandardNode:
    pass

# SVG 矢量图形节点
@NodeDef(name="SVG 节点", category="基础", render_template="svg")
class SvgNode:
    pass

# 自定义 QWidget 节点
@NodeDef(name="Widget 节点", category="基础", render_template="widget")
class WidgetNode:
    pass
```

### DAPythonSignalHandler.callInMainThread

在后台线程中需要操作 UI 时，使用 `callInMainThread` 将操作投递到主线程执行。

```python
def _push_state(self, state):
    """推送节点状态变更通知"""
    try:
        import DAWorkbench
        DAWorkbench.da_interface.call_in_main_thread(
            "node_state_change",
            self._node_descriptor["qualified_name"],
            state,
        )
    except (ImportError, AttributeError):
        # 在纯 Python 测试环境中，DAWorkbench 不可用
        pass
```

!!! warning "线程安全"
    `callInMainThread` 在 C++ 绑定层已正确处理 GIL 管理，Python 脚本无需额外处理 GIL。但需注意回调函数不要在后台线程直接操作 UI。

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
  - `node_def.py` — `@NodeDef` 装饰器实现
  - `types.py` — `Input`、`Output`、`Parameter` 类定义
  - `node_descriptor.py` — `DANodeDescriptor` 类定义
  - `node_registry.py` — `DANodeRegistry` 类定义
- 节点示例：`plugins/` 目录下的 Python 插件
  - `plugins/DataAnalysis/PyScripts/DADataAnalysisPy/` — 数据分析节点
  - `plugins/CrewAIAdapter/PyScripts/DACrewAIAdapterPy/` — AI Agent 节点
- 相关文档
  - [DAPyWorkFlow 模块概述](./workflow-overview.md)
  - [工作流生命周期](./workflow-lifecycle.md)
  - [C++ 集成指南](./workflow-cpp-integration.md)
  - [场景操作指南](./workflow-scene-operation.md)
