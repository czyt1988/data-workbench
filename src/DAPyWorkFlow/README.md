# DAPyWorkFlow 模块设计文档

> 本文档定义 DAPyWorkFlow 模块重构的设计方案、开发原则和原子操作任务分解。
> 重构范围：`src/DAPyWorkFlow`（C++ 渲染代理层）+ `src/PyScripts/DAWorkbench/DAWorkFlowPy`（Python 逻辑层）。
> 插件层 `plugins/DataAnalysis` 不在本次重构范围内。

---

DAWorkFlowPy模块类似于`Apache Hamilton`，是一个工作流节点的制定和运行模块，它目标是基于Qt的高性能QGraphicsView进行渲染，渲染由C++执行，逻辑由Python制定(Apache Hamilton UI是web端，DAWorkbench的目标一直是大规模数据场景的渲染，例如1000万条数据的大dataframe快速显示)

DAWorkFlowPy是多端口模型，Hamilton 的"函数=节点"模型只有 1 个输出（返回值），而DAWorkFlowPy做的是类似 Unreal/Blender 节点编辑器的多输入多输出端口模型

Python 生态中的多端口节点编辑器库有一个开源项目是[NodeGraphQt](https://github.com/jchanvfx/NodeGraphQt)，NodeGraphQt 是 UI + 数据模型一体化的，而 DAWorkFlowPy 是纯数据模型层 （UI 由 C++ QGraphicsView 渲染），这是更干净的分离。

## 原始问题背景

这个模块是把python端映射到c++端，python脚本中相关的内容会对应一个c++类，c++类继承`DAPyObjectWrapper`

python端代码应独立C++端运行，不依赖C++端导出的内容

C++端也会导出Python，由于此项目C++端主要负责渲染工作，因此导出的内容是让python端能控制界面操作的部分，如场景操作相关内容，C++端导出到Python的内容应该独立于`src\PyScripts\DAWorkbench\DAWorkFlowPy`部分，也就是`src\PyScripts\DAWorkbench\DAWorkFlowPy`部分不能引用C++端导出的内容

C++端导出的内容主要用于插件的二次开发，便于插件开发者通过python端进行界面上的操作

---

## 一、设计概览

### 1.1 目标

- `src/DAPyWorkFlow` 模块是 Python 脚本 `src/PyScripts/DAWorkbench/DAWorkFlowPy` 的渲染代理
- `src/PyScripts/DAWorkbench/DAWorkFlowPy` 能实现工作流节点的制定和运行
- `src/DAPyWorkFlow` 模块能基于 Qt QGraphicsView 进行渲染，渲染由 C++ 执行，逻辑由 Python 制定
- 能支持插件扩展，Python-only 是主流路径，C++ NodeFactory 子类是可选的高级扩展点
- 后续用户编写插件不需要面对 C++，只要基于 Python 即可

### 1.2 架构分层

```
┌─────────────────────────────────────────────────────┐
│                 Plugin Layer                         │
│  Python-only:  @NodeDef nodes in PyScripts          │
│  C++-optional: NodeFactory subclass + custom UI     │
│       ↕ registration ↕                              │
├─────────────────────────────────────────────────────┤
│         Python Logic Layer                           │
│  (src/PyScripts/DAWorkbench/DAWorkFlowPy)           │
│  DAWorkflow       — DAG model (topology, lifecycle) │
│  DAWorkflowNode   — node definition + execute()     │
│  NodeDef          — decorator for node declaration  │
│  DAConnection     — node-to-node data link          │
│  DANodeRegistry   — discover & register node types  │
│  DANodeFactory    — create node instances           │
│  DAWorkflowExecutor — topological execution engine   │
│  DAWorkflowSerializer — JSON save/load              │
│       ↕ attr() proxy ↕                              │
├─────────────────────────────────────────────────────┤
│         C++ Rendering Proxy Layer                    │
│  (src/DAPyWorkFlow)                                 │
│  DAPyWorkFlow      — proxy for DAWorkflow           │
│  DAPyNodeProxy     — proxy for DAWorkflowNode       │
│  DAPyNodeFactory   — proxy for DANodeFactory        │
│  DAPyWorkFlowManager — signal manager (QObject)     │
│  DAPyNodeGraphicsItem — renders node in scene       │
│  DAPyLinkGraphicsItem — renders connection          │
│  DAPyWorkFlowScene — QGraphicsScene orchestration   │
│  DAPyWorkFlowSceneSerializer — scene layout save    │
│  PythonBinding     — pybind11 module exposure       │
└─────────────────────────────────────────────────────┤
```

### 1.3 核心设计思路：Thin Proxy + Python-First

- Python 拥有 ALL 逻辑（DAG、节点定义、执行、序列化）
- C++ DAPyWorkFlow/DAPyNodeProxy/DAPyNodeFactory 是纯 thin proxy — 持有 `pybind11::object`，通过 `DAPyObjectWrapper::attr()` 转发调用，不做本地数据缓存
- 与 DAPyDataFrame 模式完全一致
- Qt 信号仅在 `DAPyWorkFlowManager`（QObject）中存在，代理类不继承 QObject

---

## 二、开发原则

### P1: Python 逻辑独立性

DAWorkFlowPy 模块可以脱离 C++ 端独立运行和测试。不导入任何 C++ 通过 pybind11 导出的模块。

- **Why**: 参考 Blender 和 KNIME 的做法——逻辑层与渲染层完全解耦，使得 Python 单元测试不需要 Qt 环境
- **How**: 所有 Python 模块的 import 只使用标准库和第三方库（pandas, numpy 等），永远不出现 `import da_xxx`（C++ 导出模块）

### P2: C++ 纯代理原则

C++ DAPyWorkFlow/DAPyNodeProxy/DAPyNodeFactory 只继承 `DAPyObjectWrapper`，不继承 `QObject`，不做本地数据缓存，所有操作通过 `attr()` 转发到 Python。

- **Why**: 与 DAPyDataFrame 模式一致，代理类保持极简，避免双端数据不一致
- **How**: 代理类的方法实现只有 `attr("method_name")(args)` 调用，不做本地存储或计算

### P3: 信号与代理分离

Qt 信号槽仅在 `DAPyWorkFlowManager`（QObject）中存在，代理类（DAPyObjectWrapper 子类）不包含任何 Qt 信号。

- **Why**: 避免在 Python 代理中引入 Qt 依赖，保持代理纯净性，信号管理集中化便于维护
- **How**: 所有 `Q_SIGNALS` 和 `Q_SLOTS` 只在 Manager/Scene 等纯 C++ Qt 类中定义

### P4: 渲染与逻辑分层

NodeDisplay（icon, render_template, style）是 Python 节点类的附加属性，仅供 C++ 渲染读取，Python 逻辑层不使用 NodeDisplay。

- **Why**: 参考 Blender DNA/RNA 模式——节点定义（逻辑）和节点显示（渲染）是两个独立关注点
- **How**: Python `execute()` 方法永远不读取 `_node_display`，C++ 渲染层只读取 `_node_display` 不干预 `execute()`

### P5: 序列化由 Python 主导

工作流序列化统一由 Python DAWorkflowSerializer 处理（JSON格式），C++ 仅保存场景布局（节点位置等 UI 元数据）。

- **Why**: Python owns all logic data（节点拓扑、参数、连接），序列化应由数据所有者负责
- **How**: 保存文件时：Python 生成逻辑 JSON + C++ 生成布局 JSON → 合并存储。加载时：Python 恢复 DAG → C++ 恢复布局

### P6: 插件 Python-first

插件开发的主流路径是纯 Python（@NodeDef），C++ NodeFactory 子类是可选的高级扩展点，仅用于渲染/UI定制。

- **Why**: 参考 ComfyUI 的 custom_nodes 模式——降低门槛，开发者只需定义 Python 类即可创建新节点
- **How**: 所有新节点建议用 `@NodeDef` 定义，C++ NodeFactory 子类只在需要自定义 QGraphicsItem 或 QDialog 时才使用

### P7: 单文件单职责

每个模块文件只承担一个明确的职责。混合多种职责的文件应拆分。

- **Why**: 参考 KNIME Node Triad（Model/View/Dialog 分离）和 Blender 文件组织
- **How**: `types.py` 只定义 Input/Output/Parameter，`node_def.py` 只定义 NodeDef 和 DAWorkflowNode，等等

### P8: 代理类不传播异常

C++ 代理类的异常处理遵循 DAPyDataFrame 模式——try/catch + 安全默认值返回，不向 Qt 应用层传播 Python 异常。

- **Why**: 保持 Qt UI 稳定性，Python 异常不应导致 C++ 程序崩溃
- **How**: 代理类每个方法用 try/catch 包裹 `attr()` 聃用，失败时返回空值/false/none，通过 `qCritical()` 记录错误

---

## 三、Python 逻辑层详细设计

### 3.1 模块划分

| 模块 | 文件 | 职责 | 对外接口 |
|------|------|------|----------|
| 类型定义 | `types.py` | `Input`, `Output`, `Parameter` 声明类，`to_dict()` 序列化 | 供 NodeDef 装饰器使用 |
| 节点定义 | `node_def.py` | `NodeDef` 装饰器，`DAWorkflowNode` 基类，`NodeDisplay` 渲染属性聚合 | `@NodeDef(name=, category=, icon=, render_template=, style=, description=)` |
| 连接关系 | `connection.py` | `DAConnection` — 描述节点间数据连接 | `DAConnection(src_node, src_channel, dst_node, dst_channel)` |
| 工作流模型 | `workflow.py` | `DAWorkflow` — DAG 模型，管理节点和连接，拓扑排序验证 | `add_node`, `remove_node`, `add_connection`, `topological_sort`, `is_valid_dag` |
| 节点注册 | `node_registry.py` | `DANodeRegistry` — 发现并注册 `@NodeDef` 装饰的节点类 | `discover(scan_paths, use_entry_points)`, `get_descriptor(qualified_name)` |
| 节点工厂 | `node_factory.py` | `DANodeFactory` — 封装 Registry 的发现+创建功能 | `discover()`, `create_node()`, `get_node_metadata()`, `get_all_metadata()` |
| 执行引擎 | `executor.py` | `DAWorkflowExecutor` — 拓扑排序执行，异步支持，状态管理 | `execute()`, `execute_async()`, `terminate()`, `pause()`, `resume()` |
| 信号管理 | `signal_manager.py` | `DASignalManager` — 节点间数据传播，`DAWorkflowState` 状态枚举 | `send_output`, `process_pending`, `is_node_ready` |
| 序列化 | `serializer.py`（新增） | `DAWorkflowSerializer` — JSON 序列化/反序列化整个工作流 | `to_json()`, `from_json()`, `to_dict()`, `from_dict()` |
| 语法糖 | `syntax.py` | `NodeProxy`, `NodeOutputProxy`, `NodeInputProxy` — `>>` 操作符语法 | `workflow[node_id]`, `node.outputs["data"] >> node2.inputs["data"]` |

### 3.2 NodeDisplay 与元数据分离

`NodeDef` 装饰器生成两类类属性：

- **逻辑属性**: `qualified_name`, `name`, `category`, `inputs`, `outputs`, `parameters` — 节点执行所需
- **渲染属性**: `_node_display`（NodeDisplay dataclass）— 仅 C++ 渲染所需，Python 逻辑层不使用

### 3.3 序列化格式

```json
{
  "name": "My Workflow",
  "version": "1.0",
  "nodes": [
    {
      "node_id": "pkg.DataFilter_1",
      "qualified_name": "pkg.DataFilter",
      "parameters": { "column": "value" }
    }
  ],
  "connections": [
    {
      "source_node_id": "pkg.DataFilter_1",
      "source_output_channel": "filtered",
      "target_node_id": "pkg.DataSort_1",
      "target_input_channel": "data"
    }
  ]
}
```

场景布局由 C++ 单独保存（节点位置、缩放级别等 UI 元数据），合并到同一文件的不同 section 中。

---

## 四、C++ 渲染代理层详细设计

### 4.1 类结构

| 类 | 职责 | 继承 | Qt信号 |
|----|------|------|---------|
| DAPyWorkFlow | DAWorkflow 的纯代理，attr() 转发 DAG 操作 | DAPyObjectWrapper | 无 |
| DAPyNodeProxy | 单个节点的纯代理，读取元数据和 NodeDisplay | DAPyObjectWrapper | 无 |
| DAPyNodeFactory | DANodeFactory 的纯代理，转发 discover/create | DAPyObjectWrapper | 无 |
| DAPyNodeMetaData | 从 Python dict 转换的节点元数据 C++ 结构体 | 纯数据 | 无 |
| DAPyWorkFlowManager | 信号管理类，持有 DAPyWorkFlow，封装操作并发射 Qt 信号 | QObject | 有 |
| DAPyNodeGraphicsItem | 节点渲染 QGraphicsItem | QGraphicsItem | 无 |
| DAPyLinkGraphicsItem | 连线渲染 QGraphicsItem | QGraphicsItem | 无 |
| DAPyWorkFlowScene | QGraphicsScene，从 Manager 监听信号更新图形项 | QGraphicsScene | 无 |
| DAPyWorkFlowSceneSerializer | 场景布局序列化 | 纯序列化 | 无 |

### 4.2 信号流转模式

```
用户操作 → DAPyWorkFlowManager.addNode(proxy)
                    ↓ 内部调用 DAPyWorkFlow.attr("add_node")(proxy)
                    ↓ Python DAWorkflow 处理，返回 node_id
                    ↓ DAPyWorkFlowManager 发射 nodeAdded(node_id, proxy)
                    ↓ DAPyWorkFlowScene 接收信号，创建 DAPyNodeGraphicsItem
```

DAPyWorkFlowManager 信号列表：

- `nodeAdded(QString nodeId, DAPyNodeProxy* proxy)` — 节点添加
- `nodeRemoved(QString nodeId)` — 节点移除
- `connectionAdded(QString connId, QString srcNodeId, QString srcChannel, QString dstNodeId, QString dstChannel)` — 连接添加
- `connectionRemoved(QString connId)` — 连接移除
- `executionStarted()` — 执行开始
- `executionFinished(bool success)` — 执行完成
- `nodeExecuted(QString nodeId, bool success)` — 单个节点执行完成
- `executorStateChanged(QString oldState, QString newState)` — 执行器状态变更

### 4.3 废弃/移除的当前类

| 当前类 | 处理 | 原因 |
|--------|------|------|
| DANodeDescriptor | 废弃 | Python NodeDef 装饰器直接生成元数据，不再需要独立的 Descriptor 类 |
| DAPortDescriptor | 废弃 | 端口信息在 DAWorkflowNode.inputs/outputs 列表中 |
| DAParameterDescriptor | 废弃 | 参数信息在 DAWorkflowNode.parameters 列表中 |
| DAPyNodeState | 保留但精简 | 仅映射 Python DAWorkflowState |
| DAPyWorkFlowLifecycle | 精简 | 执行相关逻辑全部移到 Python DAWorkflowExecutor |
| DAPyWorkFlowEnumStringUtils | 保留 | C++ ↔ Python 状态枚举映射仍需要 |
| DAPyWorkFlowUndoCommands | 保留 | Qt undo/redo 属于渲染层职责 |
| DAPyWorkFlowCommandsFactory | 保留 | undo 命令创建工厂 |
| DAPyPainterProxy | 保留 | 代理 Python 侧绘图功能 |
| DAPyNodePalette | 保留 | 节点颜色管理 |
| DAPyNodeStyleDefine / DAPyNodeStyle | 保留 | C++ 侧节点样式定义（从 NodeDisplay.style dict 映射） |
| DAPyDictConverter | 保留 | Python dict ↔ C++ 转换工具 |
| DAPyModuleWorkflow | 保留 | 模块包装器 |

---

## 五、插件扩展机制

### 5.1 Python-only 路径（主流）

```python
from DAWorkFlowPy import NodeDef, Input, Output, Parameter

@NodeDef(name="Data Filter", category="Data Processing", icon=":icons/filter.svg")
class DataFilter:
    column = Parameter(str, default="value", description="列名")

    class Inputs:
        data = Input("DataFrame", required=True)

    class Outputs:
        filtered = Output("DataFrame")

    def execute(self, inputs, params):
        df = inputs["data"]
        return {"filtered": df[df[params["column"]] > 0]}
```

节点发现：`DANodeRegistry.discover(scan_paths=[plugin_py_path])` 自动扫描，找 `@NodeDef` 装饰的类注册到 Registry。

### 5.2 C++-optional 路径（高级扩展）

C++ NodeFactory 子类（如 DataAnalysisNodeFactory）只扩展渲染/UI 层面：
- 自定义 QDialog（参数编辑对话框）
- 自定义 QGraphicsItem 渲染（如特殊表格显示）
- 节点加入 workflow 的回调（nodeAddedToWorkflow）
- 序列化扩展信息（saveExternInfoToXml/loadExternInfoFromXml）

节点逻辑（execute）始终在 Python DAWorkflowNode 中定义，C++ 不干预。

### 5.3 插件接口契约

Python 插件包需要提供：
1. 包含 `@NodeDef` 装饰的节点类
2. `qualified_name` 自动生成（模块名.类名）
3. `NodeDisplay` 提供 icon/render_template/style 供 C++ 渲染
4. `execute()` 方法定义节点逻辑

可选增强：支持 `pyproject.toml` 的 `entry_points` 声明，便于 pip 安装的插件自动被发现。

---

## 六、原子操作设计方案（AI 可执行的任务分解）

以下任务按依赖顺序排列，每个任务都是原子操作，AI 可以独立完成。

### Phase 0: Python 逻辑层重构

#### Task P0-1: 重构 types.py
- **操作**: 保持 `Input`, `Output`, `Parameter` 类不变，确认 `to_dict()` 方法输出纯 Python dict（无 C++ 类型依赖）
- **验证**: 运行 `python -c "from DAWorkFlowPy.types import Input, Output, Parameter; print(Input('DataFrame').to_dict('data'))"` 成功
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/types.py`

#### Task P0-2: 重构 node_def.py
- **操作**: 保持 `NodeDef` 装饰器、`DAWorkflowNode` 基类、`NodeDisplay` dataclass 不变，确认不依赖 C++ 导出模块，确认所有类属性是纯 Python 类型（dict, list, str）
- **验证**: 离开 C++ 环境单独 `import DAWorkFlowPy.node_def` 成功；`@NodeDef` 装饰后的类具有 `qualified_name`, `name`, `inputs`, `outputs`, `parameters`, `_node_display` 属性
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_def.py`

#### Task P0-3: 重构 connection.py
- **操作**: 保持 `DAConnection` 类不变，确认 `connection_id` 自动生成逻辑正确，确认不依赖 C++ 导出
- **验证**: `DAConnection` 可独立创建和序列化
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/connection.py`

#### Task P0-4: 重构 workflow.py
- **操作**: 保持 `DAWorkflow` 类不变，确认 DAG 操作方法（add_node, remove_node, add_connection, topological_sort, is_valid_dag）不依赖 C++ 导出；确认 `_nodes` 和 `_connections` 内部数据结构稳定
- **验证**: 可独立创建 DAWorkflow，添加节点和连接，执行拓扑排序
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/workflow.py`

#### Task P0-5: 重构 node_registry.py
- **操作**: 保持 `DANodeRegistry` 类不变，确认 `discover()` 扫描路径和 `entry_points` 发现机制正确；确认注册表存储的是 `@NodeDef` 装饰后的类本身（不是 Descriptor）
- **验证**: `DANodeRegistry.discover()` 能发现并注册 Python 节点类
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_registry.py`

#### Task P0-6: 重构 node_factory.py
- **操作**: 保持 `DANodeFactory` 类不变，确认 `discover()`, `create_node()`, `get_node_metadata()`, `get_all_metadata()` 方法不依赖 C++ 导出；确认 `_extract_metadata_from_class()` 返回纯 Python dict
- **验证**: DANodeFactory 可独立发现节点、创建实例、获取元数据
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/node_factory.py`

#### Task P0-7: 重构 executor.py
- **操作**: 保持 `DAWorkflowExecutor` 类不变，确认执行引擎不依赖 C++ 导出；移除 executor 内部对 `DAPythonSignalHandler::callInMainThread` 的直接引用，状态变更通知改为纯 Python 回调机制（on_state_change 等），C++ 侧在 binding 层通过 `DAPythonSignalHandler` 拦截 Python 回调转发到 Qt 信号
- **验证**: DAWorkflowExecutor 可独立执行工作流（同步和异步模式）
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/executor.py`

#### Task P0-8: 重构 signal_manager.py
- **操作**: 保持 `DASignalManager` 和 `DAWorkflowState` 不变，确认不依赖 C++ 导出
- **验证**: DASignalManager 可独立传播数据
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/signal_manager.py`

#### Task P0-9: 新增 serializer.py
- **操作**: 创建 `DAWorkflowSerializer` 类，实现 `to_dict()` 和 `from_dict()` 方法，将 DAWorkflow 的节点、连接、参数序列化为 JSON dict，并支持反序列化重建 DAWorkflow
- **验证**: 可序列化一个包含 3 个节点和 2 个连接的 DAWorkflow 到 JSON，再反序列化恢复
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/serializer.py`（新增）

#### Task P0-10: 重构 syntax.py
- **操作**: 保持 `NodeProxy`, `NodeOutputProxy`, `NodeInputProxy` 不变，确认 `>>` 操作符语法不依赖 C++ 导出
- **验证**: `workflow[node_id].outputs["data"] >> other.inputs["data"]` 语法可用
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/syntax.py`

#### Task P0-11: 重构 __init__.py
- **操作**: 更新导出列表，添加 `DAWorkflowSerializer`，确认所有导出类不依赖 C++ 导出模块
- **验证**: `import DAWorkFlowPy` 成功且无 C++ 依赖
- **文件**: `src/PyScripts/DAWorkbench/DAWorkFlowPy/__init__.py`

### Phase 1: C++ 渲染代理层重构

#### Task C1-1: 移除 DAPyWorkFlow 的 QObject 继承
- **操作**: 将 `DAPyWorkFlow` 从 `QObject + DAPyObjectWrapper` 双继承改为纯 `DAPyObjectWrapper` 继承；移除所有 `Q_SIGNALS`；保持 `attr()` 代理方法不变；移除 `#if 0` 屏蔽的执行方法代码块
- **验证**: DAPyWorkFlow 可通过 attr() 调用 Python DAWorkflow 的 add_node/remove_node/connectNode 等；编译通过
- **文件**: `src/DAPyWorkFlow/DAPyWorkFlow.h`, `src/DAPyWorkFlow/DAPyWorkFlow.cpp`

#### Task C1-2: 移除 DAPyNodeFactory 的 QObject 继承
- **操作**: 将 `DAPyNodeFactory` 从 `QObject` 继承改为纯 `DAPyObjectWrapper` 继承；移除 `Q_OBJECT` 宏、`DA_DECLARE_PRIVATE`（pimpl）和 `nodeDiscovered` 信号；保持 `discoverNodes()`, `createNodeProxy()` 方法为 attr() 代理调用；私有数据成员（如 node metadata 列表）改为直接存放在类中而非通过 pimpl
- **验证**: DAPyNodeFactory 可通过 attr() 调用 Python DANodeFactory 的 discover/create；编译通过
- **文件**: `src/DAPyWorkFlow/DAPyNodeFactory.h`, `src/DAPyWorkFlow/DAPyNodeFactory.cpp`

#### Task C1-3: 创建 DAPyWorkFlowManager
- **操作**: 创建新类 `DAPyWorkFlowManager`（继承 QObject），持有 `DAPyWorkFlow` 和 `DAPyNodeFactory` 实例；封装 `addNode()`, `removeNode()`, `connectNode()`, `executeWorkflow()` 等操作方法，内部调用代理类的 attr() 方法，操作完成后发射 Qt 信号
- **信号列表**: `nodeAdded(QString, DAPyNodeProxy*)`, `nodeRemoved(QString)`, `connectionAdded(...)`, `connectionRemoved(QString)`, `executionStarted()`, `executionFinished(bool)`, `nodeExecuted(QString, bool)`, `executorStateChanged(QString, QString)`
- **验证**: DAPyWorkFlowManager 的 addNode 方法能调用 Python 并发射 Qt 信号；编译通过
- **文件**: `src/DAPyWorkFlow/DAPyWorkFlowManager.h`（新增）, `src/DAPyWorkFlow/DAPyWorkFlowManager.cpp`（新增）

#### Task C1-4: 移除 DANodeDescriptor
- **操作**: 删除 `DANodeDescriptor.h` 和 `DANodeDescriptor.cpp`；所有引用 DANodeDescriptor 的代码改为从 DAPyNodeProxy 的 `attr()` 直接读取元数据；DAPyNodeMetaData 结构体的字段保持不变（从 Python dict 转换填充）
- **验证**: 编译通过，无 DANodeDescriptor 引用残留
- **文件**: 删除 `src/DAPyWorkFlow/DANodeDescriptor.h`, `src/DAPyWorkFlow/DANodeDescriptor.cpp`；修改所有引用处

#### Task C1-5: 移除 DAPortDescriptor
- **操作**: 删除 `DAPortDescriptor.h`；端口信息改为从 DAPyNodeProxy 的 `attr("inputs")` 和 `attr("outputs")` 读取 Python dict 列表
- **验证**: 编译通过，无 DAPortDescriptor 引用残留
- **文件**: 删除 `src/DAPyWorkFlow/DAPortDescriptor.h`；修改引用处

#### Task C1-6: 移除 DAParameterDescriptor
- **操作**: 删除 `DAParameterDescriptor.h` 和 `DAParameterDescriptor.cpp`；参数信息改为从 DAPyNodeProxy 的 `attr("parameters")` 读取 Python dict 列表
- **验证**: 编译通过，无 DAParameterDescriptor 引用残留
- **文件**: 删除 `src/DAPyWorkFlow/DAParameterDescriptor.h`, `src/DAPyWorkFlow/DAParameterDescriptor.cpp`；修改引用处

#### Task C1-7: 重构 DAPyNodeProxy
- **操作**: 确认 DAPyNodeProxy 只继承 `DAPyObjectWrapper`；移除任何 QObject 相关代码；所有元数据读取改为 `attr()` 调用（如 `attr("name")`, `attr("qualified_name")`, `attr("_node_display")`）；NodeDisplay 的 icon/render_template/style 从 `attr("_node_display")` 子对象中读取
- **验证**: DAPyNodeProxy 可通过 attr() 读取节点元数据和渲染属性；编译通过
- **文件**: `src/DAPyWorkFlow/DAPyNodeProxy.h`, `src/DAPyWorkFlow/DAPyNodeProxy.cpp`

#### Task C1-8: 重构 DAPyNodeGraphicsItem
- **操作**: DAPyNodeGraphicsItem 的渲染属性来源改为从 DAPyNodeProxy 的 `attr("_node_display")` 读取 NodeDisplay 子对象，再提取 icon/render_template/style；移除对 DANodeDescriptor/DAPortDescriptor 的依赖
- **验证**: DAPyNodeGraphicsItem 可从 DAPyNodeProxy 读取渲染属性并正确渲染；编译通过
- **文件**: `src/DAPyWorkFlow/DAPyNodeGraphicsItem.h`, `src/DAPyWorkFlow/DAPyNodeGraphicsItem.cpp`

#### Task C1-9: 重构 DAPyWorkFlowScene
- **操作**: DAPyWorkFlowScene 改为监听 DAPyWorkFlowManager 的信号来创建/删除 QGraphicsItem；移除对 DAPyWorkFlow 直接的信号连接（因 DAPyWorkFlow 已不继承 QObject）；Scene 的节点创建/删除逻辑改为通过 Manager 间接操作
- **验证**: DAPyWorkFlowScene 可通过 Manager 信号正确更新场景内容；编译通过
- **文件**: `src/DAPyWorkFlow/DAPyWorkFlowScene.h`, `src/DAPyWorkFlow/DAPyWorkFlowScene.cpp`

#### Task C1-10: 重构 DAPyWorkFlowSceneSerializer
- **操作**: DAPyWorkFlowSceneSerializer 只负责保存/加载场景布局（节点位置、连线位置、缩放级别等 UI 元数据），格式为 JSON section；逻辑数据序列化改为调用 Python DAWorkflowSerializer（通过 DAPyWorkFlow.attr("to_dict")() 或类似调用）；保存文件格式为合并的逻辑 JSON + 布局 JSON
- **验证**: 可保存和加载包含 3 个节点的工作流，逻辑和布局数据完整恢复；编译通过
- **文件**: `src/DAPyWorkFlow/DAPyWorkFlowSceneSerializer.h`, `src/DAPyWorkFlow/DAPyWorkFlowSceneSerializer.cpp`

#### Task C1-11: 精简 DAPyWorkFlowLifecycle
- **操作**: 精简 DAPyWorkFlowLifecycle，移除执行编排相关代码（全部移到 Python DAWorkflowExecutor）；只保留 C++ 渲染层需要的生命周期状态映射（如节点创建前后的渲染回调）
- **验证**: 编译通过，执行相关代码全部在 Python 侧
- **文件**: `src/DAPyWorkFlow/DAPyWorkFlowLifecycle.h`, `src/DAPyWorkFlow/DAPyWorkFlowLifecycle.cpp`

#### Task C1-12: 更新 PythonBinding
- **操作**: 更新 `PythonBinding/DAPyWorkFlowPythonBinding.cpp`，导出 `DAPyWorkFlowManager` 到 Python（供插件开发者通过 Python 控制界面操作）；确保导出内容独立于 DAWorkFlowPy 模块；移除不再需要的 Descriptor 类导出
- **验证**: Python 侧可 import 导出的 C++ 类并操作场景；编译通过
- **文件**: `src/DAPyWorkFlow/PythonBinding/DAPyWorkFlowPythonBinding.h`, `src/DAPyWorkFlow/PythonBinding/DAPyWorkFlowPythonBinding.cpp`

#### Task C1-13: 更新 CMakeLists.txt
- **操作**: 更新 CMakeLists.txt，添加新文件（DAPyWorkFlowManager.h/cpp），移除废弃文件（DANodeDescriptor.h/cpp, DAPortDescriptor.h, DAParameterDescriptor.h/cpp）
- **验证**: 编译通过
- **文件**: `src/DAPyWorkFlow/CMakeLists.txt`

### Phase 2: 集成验证

#### Task V-1: 编译完整项目
- **操作**: 编译整个 data-workbench 项目，确认无编译错误
- **验证**: 编译通过，无 error

#### Task V-2: Python 独立性测试
- **操作**: 在无 C++ 环境下（纯 Python），导入 DAWorkFlowPy 模块，创建 DAWorkflow，添加节点和连接，执行拓扑排序，序列化和反序列化
- **验证**: 所有 Python 模块可独立运行，无 import C++ 导出模块的依赖

#### Task V-3: C++ 代理层功能测试
- **操作**: 通过 DAPyWorkFlowManager 操作 Python DAWorkflow（添加节点、连接、执行），确认 Qt 信号正确发射，Scene 正确更新
- **验证**: Manager 的 addNode/removeNode/connectNode 操作能正确调用 Python 并触发 Qt 信号

#### Task V-4: 序列化集成测试
- **操作**: 创建一个包含多个节点的工作流，保存为文件，重新加载，确认逻辑数据和布局数据完整恢复
- **验证**: 保存→加载→对比工作流结构完全一致

#### Task V-5: 插件兼容性测试
- **操作**: 确认 DataAnalysis 插件（C++ NodeFactory 子类 + Python 节点）在重构后仍可正常工作；确认 Python-only 插件可正常发现和注册
- **验证**: DataAnalysis 插件的节点可正常添加到工作流和执行；新的 Python-only 节点可被 DANodeFactory.discover() 发现

---

## 七、参考项目

| 项目 | 核心借鉴点 |
|------|-----------|
| KNIME | Node Triad（Model/View/Dialog 分离）；Proxy Pattern 跨语言桥接；Extension Point 插件机制 |
| Blender | DNA/RNA 代理架构；Python 定义节点，C++ 纯执行；序列化由数据所有者负责 |
| ComfyUI | `@NodeDef` 装饰器模式；Registry Pattern 自动发现；custom_nodes drop-in 插件 |
| Qt NodeEditor | C++ QGraphicsScene 渲染 + Python 代理；pybind11 proxy pattern |
| DAPyDataFrame（本项目） | `DAPyObjectWrapper::attr()` 纯代理模式；try/catch 安全默认值；不继承 QObject |