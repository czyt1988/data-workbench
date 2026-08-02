---
name: develop-python-plugin
description: 当需要为data-workbench创建新的Python工作流节点插件、修改现有Python节点、调试Python节点发现/创建/执行链路问题时使用此技能。触发词：Python节点、Python插件、pyplugins、节点开发、NodeDef、节点发现失败、节点执行失败、工作流节点、创建节点、拖拽节点报错。
---

# 开发与调试 Python 工作流节点插件

本技能指导在 data-workbench 中开发新的 Python 工作流节点插件，以及在节点发现、创建、执行过程中遇到问题时如何系统排查。

## 决策树：你要做什么？

```
你的任务是什么？
├── 创建一个全新的 Python 节点插件
│   ├── 放在 pyplugins/ 目录（外部插件）→ 场景 A
│   └── 放在 DAWorkFlowPy 内置包中（内置节点）→ 场景 B
├── 为已有节点添加/修改 Input/Output/Parameter → 场景 C
├── 节点未被发现 / 不出现在面板上 → 排查清单 §1-§3
├── 节点拖入场景后报错 → 排查清单 §4-§7
└── 节点执行失败 → 排查清单 §8-§10
```

## 整体架构概览

```
┌───────────────────── C++ 侧 ─────────────────────┐  ┌────────────── Python 侧 ──────────────────┐
│                                                    │  │                                            │
│  DAAppPluginManager                                │  │  DAWorkbench.DAWorkFlowPy                  │
│    ├ loadAllPlugins()                              │  │    ├ NodeDef          (@装饰器)             │
│    ├ scanPyPluginsDir()  ← pyplugins/ 扫描         │  │    ├ DANodeRegistry   (注册表)              │
│    └ initPyNodeFactory()                           │  │    ├ DANodeFactory    (工厂类)              │
│        ↓                                           │  │    ├ DAWorkflow       (DAG模型)             │
│  DAPyNodeFactory                                   │  │    └ DAWorkflowExecutor (执行器)            │
│    ├ discoverNodes() ────────── GIL ──────────────→│─→│──→ DANodeRegistry().discover()             │
│    └ createNode() ───────────── GIL ──────────────→│─→│──→ module.import() → Class()              │
│        ↓                                           │  │                                            │
│  DAPyModuleWorkflow (单例)                          │  │                                            │
│    └ import("DAWorkbench.DAWorkFlowPy")             │  │                                            │
│    └ 缓存: DAWorkflow, DANodeRegistry, NodeDef, DANodeFactory, DAWorkflowExecutor, DASignalManager, DAWorkflowSerializer 类引用 │  │                                            │
│        ↓                                           │  │                                            │
│  DAPyWorkFlowScene                                 │  │                                            │
│    └ createPyNode() ─────────── GIL ──────────────→│─→│──→ module::import(mod).attr(cls)()         │
│        ↓                                           │  │    → workflow.add_node(instance)            │
│  DAPyNode (C++ 纯代理)                              │  │                                            │
│    └ attr("run")() ──────────── GIL ──────────────→│─→│──→ pyNode.run() → execute()               │
│        ↓                                           │  │                                            │
│  DAPyNodeGraphicsItem (渲染)                        │  │                                            │
│  DAPyWorkFlowGraphicsView (拖拽)                    │  │                                            │
│  DAPyWorkFlowNodeListWidget (节点列表面板)           │  │                                            │
│  DAPyWorkFlowNodeItemSettingWidget (属性设置容器)     │  │                                            │
│    └── DANodeParamSettingPanelWidget (面板调度器)     │  │                                            │
│         └── DANodeParamSettingPanel (参数面板)        │  │                                            │
│              └── DAFormEditorRegistry (11类型编辑器) │  │                                            │
└────────────────────────────────────────────────────┘  └────────────────────────────────────────────┘
```

**关键分界点**：C++ 与 Python 之间通过 pybind11 桥接，所有跨语言调用必须在 `DAPyGILGuard` RAII 作用域内完成。

## 核心类速查（C++ 侧）

| 类 | 文件路径 | 职责 | 关键方法 |
|---|---|---|---|
| DAAppPluginManager | `src/APP/DAAppPluginManager.h/.cpp` | 启动时扫描 pyplugins 目录，初始化 DAPyNodeFactory | `loadAllPlugins()`, `initPyNodeFactory()`, `scanPyPluginsDir()` |
| DAPyNodeFactory | `src/DAPyWorkFlow/DAPyNodeFactory.h/.cpp` | 调用 Python DANodeRegistry.discover() 发现节点，维护元数据列表，继承 DAPyObjectWrapper（非 QObject） | `discoverNodes(scanPaths, useEntryPoints)`, `createNode(qualifiedName)`, `getNodeMetadataList()` |
| DAPyModuleWorkflow | `src/DAPyWorkFlow/DAPyModuleWorkflow.h/.cpp` | 单例，导入 DAWorkbench.DAWorkFlowPy，缓存 Python 类引用 | `getInstance()`, `import()`, `getWorkflowObject()`, `getNodeRegistryObject()`, `getNodeDefDecoratorObject()`, `getNodeFactoryObject()`, `getWorkflowExecutorObject()`, `getSignalManagerObject()`, `getWorkflowSerializerObject()` |
| DAPyNode | `src/DAPyWorkFlow/DAPyNode.h/.cpp` | Python 节点的 C++ 纯代理，继承 DAPyObjectWrapper，通过 attr() 实时读取 Python 对象属性 | `getNodeId()`, `getQualifiedName()`, `getNodeName()`, `getNodeDescription()`, `getNodeStyle()`, `getNodeState()`, `setPyInputData()`, `getPyOutputData()`, `getParameters()`, `setParameterValue()` |
| DAPyWorkFlowScene | `src/DAPyWorkFlow/DAPyWorkFlowScene.h/.cpp` | 场景管理，创建/移除节点并同步 Python DAWorkflow | `createPyNode()`, `createPyNode_()`, `initPyWorkflow()`, `setPyWorkflow()` |
| DAPyNodeGraphicsItem | `src/DAPyWorkFlow/DAPyNodeGraphicsItem.h/.cpp` | 节点图形项，根据描述符渲染节点外观和连接点 | `setNodeStyle()`, `nodeStyle()`, `setRenderTemplate(DAPyNodeStyle::NodeRenderTemplate)` |
| DAPyWorkFlowGraphicsView | `src/DAGui/DAPyWorkFlowGraphicsView.h/.cpp` | 视图层，处理拖拽 dropEvent 触发节点创建 | `dropEvent()`, `createNode_()`, `createNode()` |
| DAPyNodeMetaData | `src/DAPyWorkFlow/DAPyNodeMetaData.h/.cpp` | 节点元数据类 | `name`, `qualifiedName`, `category`, `iconPath`, `tooltip`, `isValid()` |
| DAPyInterpreter | `src/DAPyBindQt/DAPyInterpreter.h/.cpp` | Python 解释器管理，sys.path 操作 | `appendSysPath()`, `isPythonInitialized()` |

## 核心模块速查（Python 侧）

| 模块/类 | 文件路径 | 职责 | C++ 交互接口 |
|---|---|---|---|
| NodeDef | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_def.py` | @NodeDef 装饰器，收集声明，设置类属性（qualified_name/name/category/inputs/outputs/parameters/_node_display） | 类属性直接读取 |
| Input/Output/Parameter | `src/PyScripts/DAWorkbench/DAWorkFlowPy/types.py` | 节点端口和参数的声明类型 | `to_dict(name)` |
| NodeDisplay | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_def.py` | 节点渲染属性 dataclass（icon, render_template, body_shape, colors 等） | `to_dict()` 风格属性读取 |
| DANodeRegistry | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_registry.py` | 注册表，双模式发现（目录扫描 + entry_points） | `discover(scan_paths, use_entry_points)` |
| DAWorkflow | `src/PyScripts/DAWorkbench/DAWorkFlowPy/workflow.py` | DAG 模型，管理节点实例和连接 | `add_node(instance)`, `remove_node(node_id)` |
| DAWorkflowExecutor | `src/PyScripts/DAWorkbench/DAWorkFlowPy/executor.py` | 工作流执行器，拓扑排序执行 | `execute()` |
| DASignalManager | `src/PyScripts/DAWorkbench/DAWorkFlowPy/signal_manager.py` | 事件驱动数据传播 | `send_output()` |
| DAConnection | `src/PyScripts/DAWorkbench/DAWorkFlowPy/connection.py` | 节点间连接关系 | `to_dict()`, `from_dict()` |
| __init__.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/__init__.py` | 包入口，统一导出所有公共类 | — |
| DANodeFactory | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_factory.py` | 节点工厂，封装发现和实例化（C++ 调用入口） | `discover()`, `create_node()`, `get_metadata()` |
| DAWorkflowSerializer | `src/PyScripts/DAWorkbench/DAWorkFlowPy/serializer.py` | 工作流序列化器 | `to_dict()`, `from_dict()` |
| NodeProxy | `src/PyScripts/DAWorkbench/DAWorkFlowPy/syntax.py` | 节点代理语法糖，支持 A >> B 链式连接 | — |

## 插件目录结构约定

### pyplugins 目录结构

```
{AppDir}/
├── PyScripts/                              ← 内置脚本（自动加入 sys.path）
│   └── DAWorkbench/
│       └── DAWorkFlowPy/                   ← 框架核心包
│
├── pyplugins/                              ← 外部插件扫描目录
│   ├── MyPlugin/                           ← 插件名（一级子目录）
│   │   └── PyScripts/                      ← 必须存在（二级子目录）
│   │       └── my_plugin_nodes/            ← Python 包名
│   │           ├── __init__.py             ← 必须存在（识别标志）
│   │           ├── setup.py                ← 可选，配置 entry_points
│   │           ├── filter_node.py          ← 节点定义文件
│   │           └── merge_node.py
│   │
│   └── CrewAIAdapter/                      ← 已有示例插件
│       └── PyScripts/
│           └── DACrewAIAdapterPy/
│               ├── __init__.py
│               ├── setup.py
│               ├── agent_node.py
│               ├── task_node.py
│               ├── crew_node.py
│               └── tool_node.py
```

### 扫描生效条件

C++ 扫描函数 `scanPyPluginsDir()` 位于 `src/APP/DAAppPluginManager.cpp`，判断逻辑：

1. `pyplugins/` 目录存在
2. 遍历一级子目录 `PluginName/`
3. `PluginName/PyScripts/` 目录存在
4. PyScripts 下至少有一个子目录包含 `__init__.py`
5. 满足条件的 `PyScripts/` 路径被加入 `sys.path` 和发现扫描列表

### sys.path 注册规则

- 内置路径：`DAPyInterpreter::appendSysPath(appDir + "/PyScripts")` 在 `initPyNodeFactory()` 中执行
- 插件路径：每个合法插件的 `PyScripts/` 目录通过 `DAPyNodeFactory::discoverNodes()` 加入 sys.path
- 效果：`import PackageName` 和 `import DAWorkbench.DAWorkFlowPy` 均可正常工作

## 场景 A：创建外部 Python 节点插件（pyplugins）

### 步骤 1：创建目录结构

```
pyplugins/
└── MyPlugin/
    └── PyScripts/
        └── my_plugin_py/
            └── __init__.py
```

### 步骤 2：编写节点类

在 `my_plugin_py/` 下创建节点文件，如 `data_filter.py`：

```python
from DAWorkbench.DAWorkFlowPy import NodeDef, Input, Output, Parameter


@NodeDef(name="Data Filter", category="Data Processing", icon="filter", render_template="nodestyle")
class DataFilterNode:
    """条件筛选节点 - 根据条件过滤 DataFrame 行"""

    # === 参数声明（类属性） ===
    column = Parameter(str, default="", description="筛选目标列名")
    condition = Parameter(str, default="", description="筛选条件表达式")

    # === 输入端口声明（嵌套类） ===
    class Inputs:
        data = Input("DataFrame", required=True, description="输入 DataFrame")

    # === 输出端口声明（嵌套类） ===
    class Outputs:
        filtered = Output("DataFrame", description="筛选后的 DataFrame")
        removed_count = Output("int", description="移除的行数")

    def __init__(self):
        """初始化输入输出数据字典"""
        self._input_data = {}
        self._output_data = {"filtered": None, "removed_count": 0}

    def set_input_data(self, channel, data):
        """C++ 侧通过此方法传入输入数据"""
        self._input_data[channel] = data

    def execute(self, inputs=None, params=None):
        """
        节点执行入口 - DAWorkflowExecutor 通过 node.run() 调用

        :param inputs: 输入数据字典（可选，默认使用 _input_data）
        :param params: 参数字典（可选）
        :return: True=成功, False=失败
        """
        if params is None:
            params = {}
        if inputs is None:
            inputs = self._input_data

        condition = params.get("condition", self.condition.default if hasattr(self.condition, 'default') else "")
        df = inputs.get("data")
        if df is None:
            return False

        try:
            import pandas as pd
            original_len = len(df)
            if condition:
                filtered_df = df.query(condition)
            else:
                filtered_df = df
            self._output_data["filtered"] = filtered_df
            self._output_data["removed_count"] = original_len - len(filtered_df)
            return True
        except Exception:
            return False
```

### 步骤 3：在 `__init__.py` 中导出节点

```python
"""my_plugin_py - 自定义数据处理节点包"""

from .data_filter import DataFilterNode

__all__ = ["DataFilterNode"]
```

### 步骤 4（可选）：配置 entry_points

创建 `setup.py` 支持 pip 安装模式的发现：

```python
from setuptools import setup, find_packages

setup(
    name="my_plugin_py",
    version="1.0.0",
    packages=find_packages(),
    install_requires=["DAWorkbench>=1.0.0"],
    entry_points={
        "data_workbench.plugin": [
            "my_plugin_py = my_plugin_py",
        ],
    },
)
```

入口点分组名称必须为 `"data_workbench.plugin"`（硬编码在 `node_registry.py` 第 30 行）。

### @NodeDef 装饰器参数说明

| 参数 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `name` | str | 是 | 节点显示名称 |
| `category` | str | 否 | 节点分组/分类 |
| `icon` | str | 否 | 图标标识或路径 |
| `style` | `NodeDisplay`/`dict` | 否 | 节点渲染样式，NodeDisplay 实例或 dict |
| `render_template` | str | 否 | 渲染模板：`"nodestyle"`(默认), `"widget"` |
| `description` | str | 否 | 节点说明文本，显示在 tooltip 中。若为 `None` 则回退读取类 docstring |

### 装饰器自动生成的类属性

@NodeDef 在被装饰的类上直接设置以下类属性（纯 Python，非 C++ struct）：

```python
cls.qualified_name       # str:  "module.ClassName" (自动生成: cls.__module__ + "." + cls.__name__)
cls.name                 # str:  节点显示名称
cls.category             # str:  节点分类
cls.icon                 # str:  图标路径
cls.__node_description   # str:  节点说明文本（来自 description 参数或 docstring 降级）
cls.inputs               # list[dict]: 从 Inputs 嵌套类收集的端口描述
cls.outputs              # list[dict]: 从 Outputs 嵌套类收集的端口描述
cls.parameters           # dict[str, Parameter]: 从类属性收集的参数描述映射
cls._node_display        # NodeDisplay: 渲染属性聚合（icon、render_template、样式字段）
cls.input_keys           # list[str]: 输入端口名称列表
cls.output_keys          # list[str]: 输出端口名称列表
```

### 声明类型 API

**Input**（输入端口）：
```python
Input(data_type: str, required: bool = True, description: str = "")
```

**Output**（输出端口）：
```python
Output(data_type: str, description: str = "")
```

**Parameter**（参数）：
```python
Parameter(param_type: type, default=None, description: str = "")
# param_type 支持: str, int, float, bool, list, dict
```

> 参数声明后，节点的参数描述符（`parameters` 类属性）会自动传递给 C++ 通用参数面板进行界面渲染。详见下方「Python 节点属性设置」章节。

## Python 节点属性设置

当用户在工作流场景中**选中 Python 节点**或**双击 Python 节点**时，C++ 侧通过 `DAPyWorkFlowNodeItemSettingWidget` 展示一个通用参数设置面板。该面板根据 Python 节点类的 `Parameter` 声明自动生成属性编辑界面，无需为每个节点类型单独编写设置窗口。

### 总体流程

```
Python 节点 (Parameter 声明)
    │
    ▼ cls.parameters (类属性)
DAPyNode::getParameters()               ← C++ 侧读取参数
    │
    ▼ QList<DAPyNodeParameter>
DANodeParameterFormAdapter::toFormSpec() ← 转换为 DAFormSpec
    │
    ▼ DAFormSpec
DAPropertyFormWidget::setFormSpec()     ← 由 DAFormEditorRegistry 创建编辑器
    │
    ▼ 用户编辑 → fieldValueChanged
DANodeParamSettingPanel                 ← 写回代理
    │
    ▼ collectConfig() → QVariantMap
DAPyNode::setParameterValue()           ← 实时写入代理
    │
    ▼ 反射到 Python 节点实例
Python 节点实例属性                      ← execute() 中读取
```

### C++ 核心类

| 类 | 路径 | 职责 |
|---|---|---|
| `DAAbstractNodeSettingWidget` | `src/DAGui/DAAbstractNodeSettingWidget.h` | 节点设置基类，持有 `DAPyNode*`，提供 `getParameters()` |
| `DANodeParamSettingPanel` | `src/DAGui/NodeSetting/DANodeParamSettingPanel.h` | 通用参数面板，继承基类，持有 `DAPropertyFormWidget`，字段变化即时写回代理 |
| `DANodeParameterFormAdapter` | `src/DAGui/NodeSetting/DANodeParameterFormAdapter.h` | `DAPyNodeParameter` → `DAFormSpec` 适配器，含类型归一化 |
| `DAFormEditorRegistry` | `src/DACommonWidgets/DAFormEditorRegistry.h` | 11 种字段类型编辑器注册表（create/read/write/connect 适配器） |
| `DAPropertyFormWidget` | `src/DACommonWidgets/DAPropertyFormWidget.h` | 统一表单渲染容器，驱动 `DAFormSpec` + `DAFormRuleEvaluator` 联动 |
| `DANodeParamSettingPanelFactory` | `src/DAGui/NodeSetting/DANodeParamSettingPanelFactory.h` | 单例工厂，`qualifiedName → createPanel` 映射，支持插件扩展 |
| `DANodeParamSettingPanelWidget` | `src/DAGui/NodeSetting/DANodeParamSettingPanelWidget.h` | `QStackedWidget` 调度器，惰性加载缓存，根据 `qualifiedName` 切换面板 |
| `DAPyWorkFlowNodeItemSettingWidget` | `src/DAGui/DAPyWorkFlowNodeItemSettingWidget.h` | 工作流节点设置容器，参数面板作为主标签页（index 0，"参数"） |

### 支持的编辑器类型（11 种）

| Python type | JSON type | C++ 编辑器控件 | 说明 |
|---|---|---|---|
| `str` | `"str"` | `QLineEdit` | 文本输入，占位符显示 description |
| `int` | `"int"` | `QSpinBox` | 整数输入，范围 0-999999 |
| `float` | `"float"` | `QDoubleSpinBox` | 浮点输入，精度 6 位 |
| `bool` | `"bool"` | `QCheckBox` | 开关选择 |
| `list` | `"list"` | `QListWidget` + 添加/删除按钮 | 列表编辑 |
| — | `"enum"` | `QComboBox` | 下拉枚举（需 C++ 侧提供 `enum_values` 字段） |
| — | `"file"` | `DAFilePathEditWidget` | 文件路径选择 |
| — | `"folder"` | `DAFilePathEditWidget` (dir mode) | 文件夹路径选择 |
| — | `"color"` | `DAColorPickerButton` | 颜色拾取 |
| — | `"font"` | `DAFontEditPannelWidget` | 字体选择 |
| — | `"code"` | `QPlainTextEdit` | 代码输入 |

> **注意**：前 6 种类型（str/int/float/bool/list）由 Python `Parameter` 原生支持；后 5 种（enum/file/folder/color/font/code）为 C++ 扩展类型，需通过描述符 JSON 的 `type` 字段显式指定。

### 参数描述符格式

Python 节点的 `parameters` 类属性是一个参数描述映射，每个元素描述一个参数：

```json
{
  "name": "column",
  "type": "str",
  "description": "筛选目标列名",
  "default": ""
}
```

C++ 扩展类型可携带额外字段：

```json
{
  "name": "file_path",
  "type": "file",
  "description": "CSV 文件路径",
  "file_filter": "CSV (*.csv)"
}
```

描述符 JSON 包含以下标准字段：

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `name` | string | 是 | 参数名称，对应 Python `Parameter` 的变量名 |
| `type` | string | 是 | 类型标签：`"str"` / `"int"` / `"float"` / `"bool"` / `"list"` / `"enum"` / `"file"` / `"folder"` / `"color"` / `"font"` / `"code"` |
| `description` | string | 否 | 参数描述，显示为占位符或 tooltip |
| `default` | any | 否 | 默认值 |

### 属性修改的信号链路（SceneB 3-hop）

```
编辑器值变化
    │
    ▼ ① mPanel::propertyValueChanged(propertyId)
onPanelPropertyValueChanged(propertyId)              ← 第一跳：内部转发
    │
    ▼ emit propertyValueChanged(propertyId)
    │
    ▼ ② this::propertyValueChanged(propertyId)
onPropertyValueChanged(propertyId)                   ← 第二跳：外部可监听
    │
    ▼ ③ collectConfig() → proxy->setConfig()
DAPyNode 配置更新                                     ← 第三跳：写入代理
```

每次编辑器值变化立即触发 3-hop 信号链，**实时写入** `DAPyNode::setParameterValue()`，无需"应用"按钮。`updateUI()` 使用 `QSignalBlocker` 阻断回写信号，避免从代理读取配置时触发不必要的写入。

### 自定义特定节点的设置面板

如果需要为某个 Python 节点类型创建**专用设置面板**（而非通用参数面板），可使用工厂注册机制：

```cpp
// 注册自定义面板
DANodeParamSettingPanelFactory::instance().registerPanel(
    "DADataAnalysisPy.data_source_node.DataSourceNode",   // qualifiedName
    [](QWidget* parent) -> DANodeParamSettingPanel* {
        return new MyCustomDataSourcePanel(parent);       // 自定义面板子类
    }
);
```

工厂根据节点的 `qualifiedName` 路由：如果有专用注册，返回专用面板；否则回退到通用 `DANodeParamSettingPanel`（默认行为）。

### 双击行为

双击 Python 工作流节点 → `DAPyWorkFlowGraphicsScene` 发射 `nodeDoubleClicked(const DAPyNode&)` 信号 → `DAPyWorkFlowNodeItemSettingWidget` 自动切换到"参数"标签页。

### 参考文件

| 文件 | 说明 |
|---|---|
| `src/DACommonWidgets/DAFormEditorRegistry.h/.cpp` | 11 种类型编辑器注册 + 适配器（create/read/write/connect） |
| `src/DAGui/NodeSetting/DANodeParamSettingPanel.h/.cpp` | 基于 DAPropertyFormWidget 的参数面板 |
| `src/DAGui/NodeSetting/DANodeParameterFormAdapter.h/.cpp` | DAPyNodeParameter → DAFormSpec 适配器 |
| `src/DAGui/NodeSetting/DANodeParamSettingPanelFactory.h/.cpp` | 单例工厂 + qualifiedName 路由 |
| `src/DAGui/NodeSetting/DANodeParamSettingPanelWidget.h/.cpp` | QStackedWidget 调度器 |
| `src/DAGui/DAAbstractNodeSettingWidget.h/.cpp` | 设置基类 |
| `src/DAGui/DAPyWorkFlowNodeItemSettingWidget.h/.cpp` | 节点设置容器（含参数标签页） |
| `skills/create-setting-panel/SKILL_cn.md` | DAPropertyPanelWidget 面板创建指南 |

## 场景 B：在内置 DAWorkFlowPy 包中添加节点

与场景 A 类似，但文件放在 `src/PyScripts/DAWorkbench/DAWorkFlowPy/` 下。

**差异点**：
- 不需要 pyplugins 目录结构
- 模块名前缀为 `DAWorkbench.DAWorkFlowPy.xxx`
- 通过内置路径 `appDir + "/PyScripts"` 自动发现
- `qualifiedName` 格式如 `DAWorkbench.DAWorkFlowPy.my_node.MyNode`

## 场景 C：为已有节点添加/修改端口和参数

1. **添加 Input**：在 `class Inputs:` 中添加新的 `Input` 声明
2. **添加 Output**：在 `class Outputs:` 中添加新的 `Output` 声明
3. **添加 Parameter**：在类体中添加新的 `Parameter` 类属性
4. **更新 execute()**：确保 `execute()` 方法使用了新的输入 key 和参数
5. **更新 __init__()**：确保 `_output_data` 字典包含新输出的初始值

变更后类属性由装饰器自动更新，C++ 侧下次 `discoverNodes()` 时自动刷新元数据。无需修改 C++ 代码。

## 完整调用链

### 链路一：启动发现链

```
 1. APP 启动
    → DAAppPluginManager::loadAllPlugins()
    文件: src/APP/DAAppPluginManager.cpp

 2. → initPyNodeFactory()
    前置: DAPyInterpreter::isPythonInitialized() 检查

 3. → DAPyInterpreter::appendSysPath(appDir + "/PyScripts")
    将内置 PyScripts 目录加入 sys.path

 4. → scanPyPluginsDir(appDir + "/pyplugins")
    遍历一级子目录 → 检查 PyScripts/PackageName/__init__.py
    输出: QStringList（有效的 PyScripts 路径列表）

 5. → DAPyNodeFactory::discoverNodes(scanPaths, useEntryPoints=true)
    文件: src/DAPyWorkFlow/DAPyNodeFactory.cpp
    GIL: 整个方法在 DAPyGILGuard 保护下

 6.   → DAPyInterpreter::appendSysPath(path) [循环]
      将每个插件 PyScripts 路径加入 sys.path

 7.   → DAPyModuleWorkflow::getInstance().import()
      导入 "DAWorkbench.DAWorkFlowPy" 模块

 8.   → pyModule.getNodeRegistryObject() → registryClass()
      创建 DANodeRegistry Python 实例

 9.   → registryInstance.discover(pyScanPaths, useEntryPoints)
      Python 侧双模式: _scan_directory() + _discover_from_entry_points()
      返回节点类列表（带 _node_display 属性）

10.   → 遍历描述符列表 → convertDescriptorToMetaData()
      转换为 DAPyNodeMetaData 存入列表
```

### 链路二：拖拽创建链

```
 1. 用户从节点面板拖拽节点
    → DAPyWorkFlowGraphicsView::dropEvent()
    文件: src/DAGui/DAPyWorkFlowGraphicsView.cpp

 2. → DANodeMimeData 解包获取 DAPyNodeMetaData

 3. → createNode_(nodemeta, evpos)
     转换: DAPyNodeMetaData → Python 模块路径
       qualifiedName = md.qualifiedName
       moduleName + className = qualifiedName.rsplit('.', 1)

 4. → DAPyWorkFlowScene::createPyNode_(metaData, pos) [支持 undo/redo]
     文件: src/DAPyWorkFlow/DAPyWorkFlowScene.cpp

 5.   → createPyNode(metaData, pos)

 6.     → 检查 mPyWorkflow 是否已初始化（initPyWorkflow 在构造时调用）
         未初始化: "Python workflow is not set" 错误

 7.     → 从 metaData.qualifiedName 提取
         为空: "metaData 无效" 错误

 8.     → 分割 qualifiedName: rfind('.') → moduleName + className
         无 '.': "invalid qualifiedName" 错误

 9.     → pybind11::module_::import(moduleName)
        失败: Python ImportError

10.     → pyMod.attr(className)() — 创建 Python 节点实例

11.     → workflowObj.attr("add_node")(pyNodeInstance)
        Python 侧注册节点到 DAWorkflow，自动分配 node_id

12.     → DAPyNode(pyNodeInstance) 构造 C++ 代理
        通过 attr() 实时读取: qualified_name, input_keys, output_keys 等

13.     → 创建 DAPyNodeGraphicsItem(node)
        配置 nodeStyle, renderTemplate, icon, linkPoints, pos
```

### 链路三：节点执行链

```
 1. DAWorkflowExecutor 调用 node.run()
    文件: src/PyScripts/DAWorkbench/DAWorkFlowPy/executor.py

 2. → DAWorkflowNode.run() 构建 inputs/params
    从 _input_data 构建 inputs dict
    从实例属性构建 params dict

 3. → 根据 execute() 签名自动适配
    execute(self, inputs, params) 或 execute(self)

 4. → C++ 侧 DAPyNode 通过 attr() 读取结果
    getNodeState() → _node_state
    getPyOutputData() → _output_data

 5. → 节点状态更新
    True: _node_state = "success"
    False: _node_state = "error"
    异常: _node_state = "error", 异常信息记录
```

## 关键约定

### qualifiedName 格式

- 由 @NodeDef 自动生成：`cls.__module__ + "." + cls.__name__`
- C++ 侧通过 `rfind('.')` 分割为 `moduleName` + `className`
- 示例：`DACrewAIAdapterPy.agent_node.AgentNode`

### execute() 返回值约定

| 返回值 | C++ 侧处理 |
|---|---|
| `True` | `DAPyNodeState::Success`，`run()` 返回 `True` |
| `False` | `DAPyNodeState::Error`，`run()` 返回 `False` |
| `None` 或无返回 | 视为成功 |
| 抛异常 | `DAPyNodeState::Error`，异常信息存入 `mLastErrorString` |

### GIL 安全规则

1. 所有 C++ → Python 调用必须在 `DAPyGILGuard` RAII 作用域内
2. `pybind11::error_already_set` 异常必须在 GIL 作用域内 catch 并消费，否则其析构时尝试获取 GIL 会死锁
3. C++ 侧持有 Python 对象使用 `DAPySafePyObjectHolder`，它在析构时检查 `Py_IsInitialized()`

### _node_display 结构（NodeDisplay dataclass）

```python
# _node_display 是 NodeDisplay dataclass 实例
# C++ 侧通过 DAPyNode::getNodeStyle() → attr("_node_display") 读取
_node_display.icon              # str:  图标路径
_node_display.render_template   # str:  "nodestyle" | "widget"
_node_display.body_shape        # Optional[str]: "RoundedRect" | "Ellipse"
_node_display.name_position     # Optional[str]: "Inside" | "Below"
_node_display.icon_position     # Optional[str]: "LeftOfText" | "AboveText"
_node_display.background_color  # Optional[ColorType]: hex "#rrggbb" 或 RGB 元组
_node_display.border_color      # Optional[ColorType]
_node_display.border_width      # Optional[float]
_node_display.corner_radius     # Optional[float]
_node_display.icon_size         # Optional[float]
_node_display.input_port_side   # Optional[str]: "West"/"East"/"North"/"South"
_node_display.output_port_side  # Optional[str]
_node_display.input_port_style  # Optional[LinkPointStyle]
_node_display.output_port_style # Optional[LinkPointStyle]
_node_display.layout_strategy   # Optional[str]: "Auto" | "Manual"
_node_display.body_icon_type    # Optional[str]: "None"/"Pixmap"/"Svg"
_node_display.body_icon_source  # Optional[str]
_node_display.body_icon_scale   # Optional[float]
```

> **注意:** 旧版 `_node_descriptor`（DANodeDescriptor C++ struct）已被移除。`@NodeDef` 装饰器现在将元数据设为类属性（`qualified_name`、`name` 等），渲染属性聚合到 `_node_display`（NodeDisplay dataclass）。

### 节点渲染样式

从 T7 版本开始，节点渲染由 `NodeDisplay` 统一控制。`@NodeDef` 的 `style` 参数接收 `NodeDisplay` 实例，C++ 侧通过 PY::toNodeStyle() 读取 NodeDisplay 属性转换为 DAPyNodeStyle，据此渲染节点外观（形状、颜色、端口样式等）。

`render_template` 参数控制模板类型：
- `"nodestyle"`（默认）：使用 `NodeDisplay` 中的样式配置进行绘制
- `"widget"`：嵌入自定义 Qt Widget

旧版参数 `"rect"` 和 `"svg"` 自动映射到 `"nodestyle"`，无需修改现有代码。

详细说明请参阅 [节点渲染设置文档](../../docs/zh/dev-guide/node-rendering-settings.md)。

### C++ DAPyNode 通过 attr() 读取的 Python 属性

| Python 属性 | C++ 获取方式 | 说明 |
|---|---|---|
| `qualified_name` | `attr("qualified_name")` → `getQualifiedName()` | 节点唯一标识 |
| `name` | `attr("name")` → `getNodeName()` | 节点显示名称 |
| `category` | `attr("category")` → `getNodeCategory()` | 节点分类 |
| `icon` | `attr("icon")` → `getIcon()` | 图标路径 |
| `input_keys` | `attr("input_keys")` → `getInputKeys()` | 输入端口名称列表 |
| `output_keys` | `attr("output_keys")` → `getOutputKeys()` | 输出端口名称列表 |
| `_node_display` | `attr("_node_display")` → `getNodeStyle()` | 渲染属性 → DAPyNodeStyle |
| `_node_state` | `attr("_node_state")` → `getNodeState()` | 执行状态字符串 |

> **注意:** `DAPyNode` 通过 `attr()` 实时读取 Python 对象属性，不缓存。每次调用 C++ 方法都会跨语言访问 Python 属性。

## 常见问题排查清单

| # | 现象/错误信息 | 可能原因 | 排查位置 |
|---|---|---|---|
| §1 | 节点不出现在面板上 | pyplugins 目录结构不正确，缺少 `PyScripts/PackageName/__init__.py` | `src/APP/DAAppPluginManager.cpp` `scanPyPluginsDir()` |
| §2 | 节点不出现（结构正确） | 节点类没有 @NodeDef 装饰器，无 `qualified_name` 类属性 | `node_registry.py` `_find_node_classes_in_module()` |
| §3 | "无法导入 DAWorkbench.DAWorkFlowPy" | PyScripts 路径未加入 sys.path | `src/APP/DAAppPluginManager.cpp` `initPyNodeFactory()` |
| §4 | "Python workflow is not set" | `DAPyWorkFlowScene` 未初始化 Python DAWorkflow 实例 | `src/DAPyWorkFlow/DAPyWorkFlowScene.cpp` `initPyWorkflow()` |
| §5 | "descriptor missing qualifiedName" | `DAPyNodeMetaData.qualifiedName` 为空，元数据转换失败 | `src/DAGui/DAPyWorkFlowGraphicsView.cpp` `createNode_()` |
| §6 | "invalid qualifiedName: xxx" | qualifiedName 格式不含 '.'，无法分割模块和类名 | `src/DAPyWorkFlow/DAPyWorkFlowScene.cpp` `createPyNode()` |
| §7 | "节点实例必须有 qualifiedName" | `add_node()` 收到的不是节点实例（而是 dict 等） | `workflow.py` `add_node()` 和 `DAPyWorkFlowScene.cpp` `createPyNode()` |
| §8 | Python ImportError | 节点模块路径不在 sys.path 中 | `src/DAPyWorkFlow/DAPyNodeFactory.cpp` `discoverNodes()` |
| §9 | "Python node reference is not set" | `DAPyNode` 构造时 Python 对象引用无效 | `src/DAPyWorkFlow/DAPyNode.cpp` |
| §10 | execute() 返回 False 或抛异常 | Python 节点 execute() 逻辑错误 | 节点的 .py 文件 `execute()` 方法 |
| §11 | GIL 死锁或程序冻结 | `pybind11::error_already_set` 在 GIL 作用域外析构 | 检查所有 catch 块是否在 `DAPyGILGuard` 作用域内 |
| §12 | 模块导入时第三方库报错 | 节点依赖的 Python 包未安装 | `requirements.txt` 和 Python 环境 |

## 参考文件总表

### C++ 侧

| 文件 | 路径 | 用途 |
|---|---|---|
| DAAppPluginManager | `src/APP/DAAppPluginManager.h/.cpp` | 插件加载入口、pyplugins 扫描、DAPyNodeFactory 初始化 |
| DAPyNodeFactory | `src/DAPyWorkFlow/DAPyNodeFactory.h/.cpp` | 节点发现、元数据管理、DAPyNode 创建 |
| DAPyModuleWorkflow | `src/DAPyWorkFlow/DAPyModuleWorkflow.h/.cpp` | 单例 Python 模块导入器，缓存类引用 |
| DAPyNode | `src/DAPyWorkFlow/DAPyNode.h/.cpp` | Python 节点 C++ 纯代理，继承 DAPyObjectWrapper，通过 attr() 实时读取 |
| DAPyWorkFlowScene | `src/DAPyWorkFlow/DAPyWorkFlowScene.h/.cpp` | 场景管理，节点创建/移除入口 |
| DAPyNodeGraphicsItem | `src/DAPyWorkFlow/DAPyNodeGraphicsItem.h/.cpp` | 节点图形项渲染，连接点管理 |
| DAPyLinkPoint | `src/DAPyWorkFlow/DAPyLinkPoint.h` | 连接点结构定义 |
| DAPyWorkFlowGraphicsView | `src/DAGui/DAPyWorkFlowGraphicsView.h/.cpp` | 视图层，拖拽创建入口 |
| DAPyWorkFlowNodeListWidget | `src/DAGui/DAPyWorkFlowNodeListWidget.h/.cpp` | 节点列表面板，显示发现的节点 |
| DANodeMimeData | `src/DAGui/DANodeMimeData.h/.cpp` | 拖拽数据格式 |
| DAPyInterpreter | `src/DAPyBindQt/DAPyInterpreter.h/.cpp` | Python 解释器管理 |
| DAPyGILGuard | `src/DAPyBindQt/DAPyGILGuard.h` | GIL RAII 守卫 |
| DAPybind11InQt | `src/DAPyBindQt/DAPybind11InQt.h` | slots 宏冲突处理 |
| DAFormEditorRegistry | `src/DACommonWidgets/DAFormEditorRegistry.h/.cpp` | 11 种字段类型编辑器注册 + 适配器 |
| DAAbstractNodeSettingWidget | `src/DAGui/DAAbstractNodeSettingWidget.h/.cpp` | 节点设置基类，持有 DAPyNode* |
| DANodeParameterFormAdapter | `src/DAGui/NodeSetting/DANodeParameterFormAdapter.h/.cpp` | DAPyNodeParameter → DAFormSpec 适配器 |
| DANodeParamSettingPanel | `src/DAGui/NodeSetting/DANodeParamSettingPanel.h/.cpp` | 通用参数面板，基于 DAPropertyFormWidget |
| DANodeParamSettingPanelFactory | `src/DAGui/NodeSetting/DANodeParamSettingPanelFactory.h/.cpp` | 面板单例工厂，qualifiedName 路由 |
| DANodeParamSettingPanelWidget | `src/DAGui/NodeSetting/DANodeParamSettingPanelWidget.h/.cpp` | QStackedWidget 调度器，惰性缓存 |
| DAPyWorkFlowManager | `src/DAPyWorkFlow/DAPyWorkFlowManager.h/.cpp` | 工作流管理 QObject，编排节点操作，发射 Qt 信号 |
| DAPyNodeStyle | `src/DAPyWorkFlow/DAPyNodeStyle.h/.cpp` | 节点视觉样式配置结构体 |

### Python 侧

| 文件 | 路径 | 用途 |
|---|---|---|
| __init__.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/__init__.py` | 包入口，统一导出 |
| node_def.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_def.py` | @NodeDef 装饰器实现 |
| types.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/types.py` | Input/Output/Parameter 类型定义 |
| node_registry.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_registry.py` | DANodeRegistry 注册表 |
| workflow.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/workflow.py` | DAWorkflow DAG 模型 |
| executor.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/executor.py` | DAWorkflowExecutor 执行器 |
| signal_manager.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/signal_manager.py` | DASignalManager 信号管理 |
| connection.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/connection.py` | DAConnection 连接模型 |
| node_factory.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_factory.py` | DANodeFactory 节点工厂 |
| serializer.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/serializer.py` | DAWorkflowSerializer 序列化器 |
| syntax.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/syntax.py` | NodeProxy/NodeOutputProxy/NodeInputProxy 语法糖 |

### 示例插件

| 插件 | 路径 | 说明 |
|---|---|---|
| CrewAIAdapter | `plugins/CrewAIAdapter/PyScripts/DACrewAIAdapterPy/` | AI Agent 节点（AgentNode, TaskNode, CrewNode, ToolNode） |
| DataAnalysis | `plugins/DataAnalysis/PyScripts/DADataAnalysisPy/` | 数据分析节点（DataSourceNode, DataFilterNode 等） |
