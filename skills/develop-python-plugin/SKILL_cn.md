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
│    └ initPyNodeFactory()                           │  │    ├ DANodeDescriptor (描述符)              │
│        ↓                                           │  │    ├ DAWorkflow       (DAG模型)             │
│  DAPyNodeFactory                                   │  │    └ DAWorkflowExecutor (执行器)            │
│    ├ discoverNodes() ────────── GIL ──────────────→│─→│──→ DANodeRegistry().discover()             │
│    └ createNodeProxy() ──────── GIL ──────────────→│─→│──→ module.import() → Class()              │
│        ↓                                           │  │                                            │
│  DAPyModuleWorkflow (单例)                          │  │                                            │
│    └ import("DAWorkbench.DAWorkFlowPy")             │  │                                            │
│    └ 缓存: DAWorkflow, DANodeRegistry, NodeDef 类   │  │                                            │
│        ↓                                           │  │                                            │
│  DAPyWorkFlowScene                                 │  │                                            │
│    └ createPyNode() ─────────── GIL ──────────────→│─→│──→ module::import(mod).attr(cls)()         │
│        ↓                                           │  │    → workflow.add_node(instance)            │
│  DAPyNodeProxy                                     │  │                                            │
│    └ exec() ─────────────────── GIL ──────────────→│─→│──→ pyNode.execute()                       │
│        ↓                                           │  │                                            │
│  DAPyNodeGraphicsItem (渲染)                        │  │                                            │
│  DAPyWorkFlowGraphicsView (拖拽)                    │  │                                            │
│  DAPyWorkFlowNodeListWidget (节点列表面板)           │  │                                            │
└────────────────────────────────────────────────────┘  └────────────────────────────────────────────┘
```

**关键分界点**：C++ 与 Python 之间通过 pybind11 桥接，所有跨语言调用必须在 `DAPyGILGuard` RAII 作用域内完成。

## 核心类速查（C++ 侧）

| 类 | 文件路径 | 职责 | 关键方法 |
|---|---|---|---|
| DAAppPluginManager | `src/APP/DAAppPluginManager.h/.cpp` | 启动时扫描 pyplugins 目录，初始化 DAPyNodeFactory | `loadAllPlugins()`, `initPyNodeFactory()`, `scanPyPluginsDir()` |
| DAPyNodeFactory | `src/DAPyWorkFlow/DAPyNodeFactory.h/.cpp` | 调用 Python DANodeRegistry.discover() 发现节点，维护元数据列表 | `discoverNodes(scanPaths, useEntryPoints)`, `createNodeProxy(qualifiedName)`, `getNodeMetadataList()` |
| DAPyModuleWorkflow | `src/DAPyWorkFlow/DAPyModuleWorkflow.h/.cpp` | 单例，导入 DAWorkbench.DAWorkFlowPy，缓存 Python 类引用 | `getInstance()`, `import()`, `getWorkflowClass()`, `getNodeRegistryClass()`, `getNodeDefDecorator()` |
| DAPyNodeProxy | `src/DAPyWorkFlow/DAPyNodeProxy.h/.cpp` | 持有 Python 节点实例引用，执行桥接，元信息同步缓存 | `exec()`, `setPyNodeRef()`, `getPyNodeRef()`, `getQualifiedName()` |
| DAPyWorkFlowScene | `src/DAPyWorkFlow/DAPyWorkFlowScene.h/.cpp` | 场景管理，创建/移除节点并同步 Python DAWorkflow | `createPyNode()`, `createPyNode_()`, `initPyWorkflow()`, `setPyWorkflow()` |
| DAPyNodeGraphicsItem | `src/DAPyWorkFlow/DAPyNodeGraphicsItem.h/.cpp` | 节点图形项，根据描述符渲染节点外观和连接点 | `setDescriptor()`, `updateLinkPoints()`, `setRenderTemplate()` |
| DAPyWorkFlowGraphicsView | `src/DAGui/DAPyWorkFlowGraphicsView.h/.cpp` | 视图层，处理拖拽 dropEvent 触发节点创建 | `dropEvent()`, `createNode_()`, `createNode()` |
| DAPyNodeMetaData | `src/DAPyWorkFlow/DAPyNodeFactory.h` (结构体) | 节点元数据，C++ 侧存储发现的节点描述信息 | `prototype`, `name`, `group`, `inputKeys`, `outputKeys` |
| DAPyInterpreter | `src/DAPyBindQt/DAPyInterpreter.h/.cpp` | Python 解释器管理，sys.path 操作 | `appendSysPath()`, `isPythonInitialized()` |

## 核心模块速查（Python 侧）

| 模块/类 | 文件路径 | 职责 | C++ 交互接口 |
|---|---|---|---|
| NodeDef | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_def.py` | @NodeDef 装饰器，收集声明，生成 `_node_descriptor` | `cls._node_descriptor` 字典 |
| Input/Output/Parameter | `src/PyScripts/DAWorkbench/DAWorkFlowPy/types.py` | 节点端口和参数的声明类型 | `to_dict(name)` |
| DANodeDescriptor | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_descriptor.py` | 节点描述符数据类 | `to_dict()` |
| DANodeRegistry | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_registry.py` | 注册表，双模式发现（目录扫描 + entry_points） | `discover(scan_paths, use_entry_points)` |
| DAWorkflow | `src/PyScripts/DAWorkbench/DAWorkFlowPy/workflow.py` | DAG 模型，管理节点实例和连接 | `add_node(instance)`, `remove_node(node_id)` |
| DAWorkflowExecutor | `src/PyScripts/DAWorkbench/DAWorkFlowPy/executor.py` | 工作流执行器，拓扑排序执行 | `execute()` |
| DASignalManager | `src/PyScripts/DAWorkbench/DAWorkFlowPy/signal_manager.py` | 事件驱动数据传播 | `send_output()` |
| DAConnection | `src/PyScripts/DAWorkbench/DAWorkFlowPy/connection.py` | 节点间连接关系 | `to_dict()`, `from_dict()` |
| __init__.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/__init__.py` | 包入口，统一导出所有公共类 | — |

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


@NodeDef(name="Data Filter", category="Data Processing", icon="filter", render_template="rect")
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
        节点执行入口 - C++ 侧通过 DAPyNodeProxy::exec() 调用

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
| `render_template` | str | 否 | 渲染模板：`"rect"`(默认), `"svg"`, `"widget"` |

### 装饰器自动生成的属性

@NodeDef 在被装饰的类上设置 `_node_descriptor` 字典：

```python
_node_descriptor = {
    "name": name,
    "category": category,
    "icon": icon,
    "qualified_name": f"{cls.__module__}.{cls.__qualname__}",  # 自动生成
    "inputs": [...],       # 从 Inputs 嵌套类收集
    "outputs": [...],      # 从 Outputs 嵌套类收集
    "parameters": [...],   # 从类属性中的 Parameter 实例收集
    "render_template": render_template,
}
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

## 场景 B：在内置 DAWorkFlowPy 包中添加节点

与场景 A 类似，但文件放在 `src/PyScripts/DAWorkbench/DAWorkFlowPy/` 下。

**差异点**：
- 不需要 pyplugins 目录结构
- 模块名前缀为 `DAWorkbench.DAWorkFlowPy.xxx`
- 通过内置路径 `appDir + "/PyScripts"` 自动发现
- `qualified_name` 格式如 `DAWorkbench.DAWorkFlowPy.my_node.MyNode`

## 场景 C：为已有节点添加/修改端口和参数

1. **添加 Input**：在 `class Inputs:` 中添加新的 `Input` 声明
2. **添加 Output**：在 `class Outputs:` 中添加新的 `Output` 声明
3. **添加 Parameter**：在类体中添加新的 `Parameter` 类属性
4. **更新 execute()**：确保 `execute()` 方法使用了新的输入 key 和参数
5. **更新 __init__()**：确保 `_output_data` 字典包含新输出的初始值

变更后 `_node_descriptor` 由装饰器自动更新，C++ 侧下次 `discoverNodes()` 时自动刷新元数据。无需修改 C++ 代码。

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

 8.   → pyModule.getNodeRegistryClass() → registryClass()
      创建 DANodeRegistry Python 实例

 9.   → registryInstance.discover(pyScanPaths, useEntryPoints)
      Python 侧双模式: _scan_directory() + _discover_from_entry_points()
      返回 DANodeDescriptor 列表

10.   → 遍历描述符列表 → convertDescriptorToMetaData()
      转换为 DAPyNodeMetaData → emit nodeDiscovered() 通知 UI
```

### 链路二：拖拽创建链

```
 1. 用户从节点面板拖拽节点
    → DAPyWorkFlowGraphicsView::dropEvent()
    文件: src/DAGui/DAPyWorkFlowGraphicsView.cpp

 2. → DANodeMimeData 解包获取 DAPyNodeMetaData

 3. → createNode_(nodemeta, evpos)
    转换: DAPyNodeMetaData → QJsonObject descriptor
      descriptor["qualified_name"] = md.prototype
      descriptor["name"] = md.name
      descriptor["group"] = md.group

 4. → DAPyWorkFlowScene::createPyNode_(descriptor, pos) [支持 undo/redo]
    文件: src/DAPyWorkFlow/DAPyWorkFlowScene.cpp

 5.   → createPyNode(descriptor, pos)

 6.     → 检查 mPyWorkflow 是否已初始化（initPyWorkflow 在构造时调用）
        未初始化: "Python workflow is not set" 错误

 7.     → 从 descriptor 提取 qualified_name
        为空: "descriptor missing qualified_name" 错误

 8.     → 分割 qualified_name: rfind('.') → moduleName + className
        无 '.': "invalid qualified_name" 错误

 9.     → pybind11::module_::import(moduleName)
        失败: Python ImportError

10.     → pyMod.attr(className)() — 创建 Python 节点实例

11.     → workflowObj.attr("add_node")(pyNodeInstance)
        Python 侧注册节点到 DAWorkflow，自动分配 node_id

12.     → proxy->setPyNodeRef(pyNodeInstance)
        C++ 侧同步元信息: qualified_name, input_keys, output_keys 等

13.     → 创建 DAPyNodeGraphicsItem(proxy)
        配置 descriptor, renderTemplate, icon, linkPoints, pos
```

### 链路三：节点执行链

```
 1. DAPyNodeProxy::exec()
    文件: src/DAPyWorkFlow/DAPyNodeProxy.cpp

 2. → 检查 mPyNodeRef 是否有效
    无效: "Python node reference is not set" 错误

 3. → mNodeState = Running

 4. → DAPyGILGuard gilGuard（RAII 获取 GIL）
    获取失败: "Failed to acquire GIL" 错误

 5. → pyNode.attr("execute")()
    调用 Python 节点的 execute() 方法

 6. → 检查返回值
    bool(True): mNodeState = Success
    bool(False): mNodeState = Error
    非 bool 或 None: 视为成功

 7. → 异常处理
    pybind11::error_already_set: 必须在 GIL 作用域内消费
    std::exception: 存储到 mLastErrorString
```

## 关键约定

### qualified_name 格式

- 由 @NodeDef 自动生成：`f"{cls.__module__}.{cls.__qualname__}"`
- C++ 侧通过 `rfind('.')` 分割为 `moduleName` + `className`
- 示例：`DACrewAIAdapterPy.agent_node.AgentNode`

### execute() 返回值约定

| 返回值 | C++ 侧处理 |
|---|---|
| `True` | `DAPyNodeState::Success`，`exec()` 返回 `true` |
| `False` | `DAPyNodeState::Error`，`exec()` 返回 `false` |
| `None` 或无返回 | 视为成功 |
| 抛异常 | `DAPyNodeState::Error`，异常信息存入 `mLastErrorString` |

### GIL 安全规则

1. 所有 C++ → Python 调用必须在 `DAPyGILGuard` RAII 作用域内
2. `pybind11::error_already_set` 异常必须在 GIL 作用域内 catch 并消费，否则其析构时尝试获取 GIL 会死锁
3. C++ 侧持有 Python 对象使用 `DAPySafePyObjectHolder`，它在析构时检查 `Py_IsInitialized()`

### _node_descriptor 字典结构

```python
{
    "name": str,               # 节点显示名称
    "category": str,           # 节点分类
    "icon": str,               # 图标标识
    "qualified_name": str,     # 模块名.类名（唯一标识）
    "inputs": [                # 输入端口列表
        {"name": str, "data_type": str, "required": bool, "description": str}
    ],
    "outputs": [               # 输出端口列表
        {"name": str, "data_type": str, "description": str}
    ],
    "parameters": [            # 参数列表
        {"name": str, "type": str, "default": Any, "description": str}
    ],
    "render_template": str,    # "rect" | "svg" | "widget"
}
```

### C++ DAPyNodeProxy 同步读取的 Python 属性

`setPyNodeRef()` 调用时，`syncMetaFromPyNode()` 读取以下 Python 实例属性：

| Python 属性 | C++ 缓存字段 | 来源 |
|---|---|---|
| `qualified_name` | `mQualifiedName` | `_node_descriptor["qualified_name"]` |
| `node_name` | `mNodeName` | 由 NodeDef 设置 |
| `input_keys` | `mInputKeys` | 从 `_node_descriptor["inputs"]` 提取 |
| `output_keys` | `mOutputKeys` | 从 `_node_descriptor["outputs"]` 提取 |
| `node_prototype` | `mNodePrototype` | 同 `qualified_name` |
| `group` | `mNodeGroup` | `_node_descriptor["category"]` |

## 常见问题排查清单

| # | 现象/错误信息 | 可能原因 | 排查位置 |
|---|---|---|---|
| §1 | 节点不出现在面板上 | pyplugins 目录结构不正确，缺少 `PyScripts/PackageName/__init__.py` | `src/APP/DAAppPluginManager.cpp` `scanPyPluginsDir()` |
| §2 | 节点不出现（结构正确） | 节点类没有 @NodeDef 装饰器，无 `_node_descriptor` 属性 | `node_registry.py` `_find_node_classes_in_module()` |
| §3 | "无法导入 DAWorkbench.DAWorkFlowPy" | PyScripts 路径未加入 sys.path | `src/APP/DAAppPluginManager.cpp` `initPyNodeFactory()` |
| §4 | "Python workflow is not set" | `DAPyWorkFlowScene` 未初始化 Python DAWorkflow 实例 | `src/DAPyWorkFlow/DAPyWorkFlowScene.cpp` `initPyWorkflow()` |
| §5 | "descriptor missing qualified_name" | `DAPyNodeMetaData.prototype` 为空，QJsonObject 转换失败 | `src/DAGui/DAPyWorkFlowGraphicsView.cpp` `createNode_()` |
| §6 | "invalid qualified_name: xxx" | qualified_name 格式不含 '.'，无法分割模块和类名 | `src/DAPyWorkFlow/DAPyWorkFlowScene.cpp` `createPyNode()` |
| §7 | "节点实例必须有 qualified_name" | `add_node()` 收到的不是节点实例（而是 dict 等） | `workflow.py` `add_node()` 和 `DAPyWorkFlowScene.cpp` `createPyNode()` |
| §8 | Python ImportError | 节点模块路径不在 sys.path 中 | `src/DAPyWorkFlow/DAPyNodeFactory.cpp` `discoverNodes()` |
| §9 | "Python node reference is not set" | `DAPyNodeProxy` 未调用 `setPyNodeRef()` | `src/DAPyWorkFlow/DAPyNodeProxy.cpp` `exec()` |
| §10 | execute() 返回 False 或抛异常 | Python 节点 execute() 逻辑错误 | 节点的 .py 文件 `execute()` 方法 |
| §11 | GIL 死锁或程序冻结 | `pybind11::error_already_set` 在 GIL 作用域外析构 | 检查所有 catch 块是否在 `DAPyGILGuard` 作用域内 |
| §12 | 模块导入时第三方库报错 | 节点依赖的 Python 包未安装 | `requirements.txt` 和 Python 环境 |

## 参考文件总表

### C++ 侧

| 文件 | 路径 | 用途 |
|---|---|---|
| DAAppPluginManager | `src/APP/DAAppPluginManager.h/.cpp` | 插件加载入口、pyplugins 扫描、DAPyNodeFactory 初始化 |
| DAPyNodeFactory | `src/DAPyWorkFlow/DAPyNodeFactory.h/.cpp` | 节点发现、元数据管理、DAPyNodeProxy 创建 |
| DAPyModuleWorkflow | `src/DAPyWorkFlow/DAPyModuleWorkflow.h/.cpp` | 单例 Python 模块导入器，缓存类引用 |
| DAPyNodeProxy | `src/DAPyWorkFlow/DAPyNodeProxy.h/.cpp` | Python 节点代理，执行桥接，元信息同步 |
| DAPyWorkFlowScene | `src/DAPyWorkFlow/DAPyWorkFlowScene.h/.cpp` | 场景管理，节点创建/移除入口 |
| DAPyNodeGraphicsItem | `src/DAPyWorkFlow/DAPyNodeGraphicsItem.h/.cpp` | 节点图形项渲染，连接点管理 |
| DAPyLinkPoint | `src/DAPyWorkFlow/DAPyLinkPoint.h` | 连接点结构定义 |
| DAPyWorkFlowGraphicsView | `src/DAGui/DAPyWorkFlowGraphicsView.h/.cpp` | 视图层，拖拽创建入口 |
| DAPyWorkFlowNodeListWidget | `src/DAGui/DAPyWorkFlowNodeListWidget.h/.cpp` | 节点列表面板，显示发现的节点 |
| DANodeMimeData | `src/DAGui/DANodeMimeData.h/.cpp` | 拖拽数据格式 |
| DAPyInterpreter | `src/DAPyBindQt/DAPyInterpreter.h/.cpp` | Python 解释器管理 |
| DAPyGILGuard | `src/DAPyWorkFlow/DAPyGILGuard.h` | GIL RAII 守卫 |
| DAPybind11InQt | `src/DAPyBindQt/DAPybind11InQt.h` | slots 宏冲突处理 |

### Python 侧

| 文件 | 路径 | 用途 |
|---|---|---|
| __init__.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/__init__.py` | 包入口，统一导出 |
| node_def.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_def.py` | @NodeDef 装饰器实现 |
| types.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/types.py` | Input/Output/Parameter 类型定义 |
| node_descriptor.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_descriptor.py` | DANodeDescriptor 描述符类 |
| node_registry.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_registry.py` | DANodeRegistry 注册表 |
| workflow.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/workflow.py` | DAWorkflow DAG 模型 |
| executor.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/executor.py` | DAWorkflowExecutor 执行器 |
| signal_manager.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/signal_manager.py` | DASignalManager 信号管理 |
| connection.py | `src/PyScripts/DAWorkbench/DAWorkFlowPy/connection.py` | DAConnection 连接模型 |

### 示例插件

| 插件 | 路径 | 说明 |
|---|---|---|
| CrewAIAdapter | `plugins/CrewAIAdapter/PyScripts/DACrewAIAdapterPy/` | AI Agent 节点（AgentNode, TaskNode, CrewNode, ToolNode） |
| DataAnalysis | `plugins/DataAnalysis/PyScripts/DADataAnalysisPy/` | 数据分析节点（DataSourceNode, DataFilterNode 等） |
